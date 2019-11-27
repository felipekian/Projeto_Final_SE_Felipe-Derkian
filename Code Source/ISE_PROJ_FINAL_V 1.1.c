/* Bibliotecas */
#include <EmonLib.h>
#include <Ultrasonic.h>
#include <DHT_U.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <dummy.h>
#include <LiquidCrystal.h>

/* Configuração LCD */
LiquidCrystal lcd(13, 15, 5, 4, 3, 2);

/* Configuração */
int ja_configurado = 0;
const int tempoDeConfig = 10;

/* Contagem de tempo */
int TempoLigado = 0;
int segundosEconomia = 0;

/* Pegar distância do ambiente para calcular uma média e usar como maior distância */
int maxDistUS = 0;


/* Configuração do Sensor Ultrassonico */
const int pino_trigger = 12;
const int pino_echo    = 14;
Ultrasonic ultrasonic(pino_trigger, pino_echo);
float cmMsec=0.0;


/* Configuração do Sensor de temperatura e umidade*/
const int DHTPIN = 10; // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11
uint32_t delayMS;
DHT_Unified dht(DHTPIN, DHTTYPE);
int umidade=0, temperatura=0;

/* Rele */
const int Rele_R1 = 16;

/* PIR */
const int PIR = 0;
int lerPIR = 0;


/* Configuração do sensor de corrente */
#define CURRENT_CAL 20
const int sensorCorrente = A0;
EnergyMonitor emon1;
float totalCorrente = 0;


/* Configuração do tamanho do LCD */
void configLCD(){
  lcd.begin(16, 2);
}


/* Configuração da fase de calibragem do sistema */
void configLCDConfigurando(int tempSec){
  //lcd.clear();
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("CONFIGURANDO ...");
  lcd.setCursor(0, 1);
  lcd.printf("%d SEGUNDOS", tempoDeConfig-tempSec);
}


/* Configuração da fase de imprimir as informações no LCD */
void print_lcd(){
  lcd.clear();
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  
  if(TempoLigado < 60){
    lcd.printf("%dC L%dS E%dS",temperatura , TempoLigado, segundosEconomia); 
  }else if(TempoLigado < 3600){
    lcd.printf("%dC L%dM E%dM", temperatura, TempoLigado/60, segundosEconomia/60); 
  }else if(TempoLigado < 86400){
    lcd.printf("%dC L%dH E%dH", temperatura, TempoLigado/3600, segundosEconomia/3600); 
  }else{
    lcd.printf("%dC L%dD E%dD", temperatura, TempoLigado/86400, segundosEconomia/86400); 
  }
   
  
  lcd.setCursor(0, 1);  
  //lcd.printf("%d  %d",millis() / 1000, (millis() / 1000) % 100);
  int distancia = cmMsec;

  lcd.printf("%d%% %dCm %.1fA",umidade , distancia, totalCorrente/1000);
  

}


/* setar porta logica do rele */
void Rele_R1_Ligado(){
  digitalWrite(Rele_R1, LOW);  //Liga rele 1  
}
void Rele_R1_Desligado(){
  digitalWrite(Rele_R1, HIGH); //Desliga rele 1  
}

/* get umidade e temperatura */
void umidadeEtemperatura(){
  sensors_event_t event;  
  dht.humidity().getEvent(&event);
  umidade = event.relative_humidity;
  
  dht.temperature().getEvent(&event);
  temperatura = event.temperature;

}

/* get distancia */
void sensor_ultrassonico(){
  long microsec = ultrasonic.timing();
  cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);
  
    Serial.print("DISTANCIA: ");
    Serial.print(maxDistUS);
    Serial.print(" -> ");
    Serial.println(cmMsec);
}

/* get presença */
void LerSensorPIR(){
    lerPIR = digitalRead(PIR);
    
}



/* capturação de distancias para calibragem */
void setMaxUS(){
  int tempSec = 0;
  int v[20] = {0};
  int i = 0;
  while( tempSec < tempoDeConfig ){                
    tempSec = millis() / 1000;        
    
    sensor_ultrassonico();
    int distanceNow = cmMsec;

    v[i] = distanceNow;    

    i=(++i)%20 ;
    
    /*
    if( distanceNow > maxDistUS ){
      maxDistUS = distanceNow;  
    }
    */

    
    configLCDConfigurando(tempSec);
    
    delay(200);
  }
  maxDistUS = v[10];
}

/* função que ativa uma rodada de captura de dados dos sensores */
void actions(){
  umidadeEtemperatura();
  sensor_ultrassonico();
  LerSensorPIR();
  print_lcd();
}

/* calcular corrente */
void calcularCorrente(){
  emon1.calcVI(20,100);
  double currentDraw = emon1.Irms;
  totalCorrente += (currentDraw * 3);
}


/* configuração inicial do sitema */
void setup() {
  //configura o LCD
  configLCD();
  
  //sensor de umidade e temperatura
  dht.begin(); 
  
  //config RELE_R1
  pinMode(Rele_R1, OUTPUT);

  //config PIR
  pinMode(PIR, INPUT);
  
  //serial
  Serial.begin(9600);
 
  //sensor de corrente
  emon1.current(sensorCorrente, CURRENT_CAL);
}

/* Programa principal */
void loop() {
    
  if(ja_configurado==0){
    ja_configurado = 1;
    Rele_R1_Desligado();
    setMaxUS();    
  }
    
  while(1){        
    Rele_R1_Desligado();    
    while( cmMsec > (maxDistUS*0.95)){                    
        segundosEconomia = millis() / 1000;
        TempoLigado = millis() / 1000;
        calcularCorrente();
        actions();
        delay(3000);        
    }

    Rele_R1_Ligado();
    if( cmMsec < (maxDistUS*0.95)){
      while(lerPIR==HIGH && cmMsec<(maxDistUS*0.95) ){
        calcularCorrente();
        TempoLigado = millis() / 1000;
        actions();
        delay(3000);        
      }
    }
  }  
}

