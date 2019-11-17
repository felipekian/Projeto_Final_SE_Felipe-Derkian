#include <EmonLib.h>
#include <Ultrasonic.h>
#include <DHT_U.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <dummy.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(13, 15, 5, 4, 3, 2);

int ja_configurado = 0;

const int pino_trigger = 12;
const int pino_echo    = 14;
//Inicializa o sensor nos pinos definidos acima
Ultrasonic ultrasonic(pino_trigger, pino_echo);
float cmMsec=0.0;
int maxDistUS = 0;
 
const int DHTPIN = 10; // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11
uint32_t delayMS;
DHT_Unified dht(DHTPIN, DHTTYPE);
int umidade=0, temperatura=0;

// Sensor Rele
const int Rele_R1 = 16;


//sensor de presença PIR
const int PIR = 0;
int lerPIR;


int segundosEconomia = 0;


//sensor de corrente
int totalCorrente = 0;
//#define CURRENT_CAL 18.40 //VALOR DE CALIBRAÇÃO (DEVE SER AJUSTADO EM PARALELO COM UM MULTÍMETRO MEDINDO A CORRENTE DA CARGA)
#define CURRENT_CAL 20 //VALOR DE CALIBRAÇÃO (DEVE SER AJUSTADO EM PARALELO COM UM MULTÍMETRO MEDINDO A CORRENTE DA CARGA)
const int sensorCorrente = A0; //PINO ANALÓGICO EM QUE O SENSOR ESTÁ CONECTADO 
EnergyMonitor emon1; //CRIA UMA INSTÂNCIA


void configLCD(){
  lcd.begin(16, 2);
  //lcd.print("UMI TMP DIS P E");

}


void configLCDConfigurando(int tempSec){
  lcd.clear();
  lcd.begin(16, 2);
  lcd.print("Configurando ...");
  lcd.setCursor(0, 1);
  lcd.printf("%d segundos", 15-tempSec);
}

void print_lcd(){
  lcd.clear();
  lcd.begin(16, 2);
  lcd.printf("%d%%   %dC   P%d", umidade, temperatura, lerPIR);
  lcd.setCursor(0, 1);
  //lcd.printf("%d  %d",millis() / 1000, (millis() / 1000) % 100);
  int dist = cmMsec;

  if(segundosEconomia < 60){
    lcd.printf("%dCm  %dS   %dA",dist, segundosEconomia, totalCorrente);
  }else if( segundosEconomia < 3600 ){
    lcd.printf("%dCm  %dM   %dA",dist, segundosEconomia/60,totalCorrente);
  }else if( segundosEconomia < 86400 ){
    lcd.printf("%dCm  %dH   %dA",dist, segundosEconomia/3600, totalCorrente);
  }else{
    lcd.printf("%dCm  %dD   %dA",dist, segundosEconomia/86400, totalCorrente);
  }
}

void Rele_R1_Ligado(){
  digitalWrite(Rele_R1, LOW);  //Liga rele 1  
}

void Rele_R1_Desligado(){
  digitalWrite(Rele_R1, HIGH); //Desliga rele 1  
}


void umidadeEtemperatura(){
  sensors_event_t event;
  
  dht.humidity().getEvent(&event);
  umidade = event.relative_humidity;

  dht.temperature().getEvent(&event);
  temperatura = event.temperature;

}

void sensor_ultrassonico(){
  long microsec = ultrasonic.timing();
  cmMsec = ultrasonic.convert(microsec, Ultrasonic::CM);  
}

void presenca_dectected(){
    lerPIR = digitalRead(PIR);
    Serial.printf("PIR = %d\n", lerPIR);    
}


void setMaxUS(){
  int tempSec = 0;
  
    while( tempSec < 15 ){                
        tempSec = millis() / 1000;        
        
        sensor_ultrassonico();
        int distanceNow = cmMsec;

        if( distanceNow > maxDistUS ){
          maxDistUS = distanceNow;  
        }
        configLCDConfigurando(tempSec);
        delay(1000);
    }
}


void actions(int tempo){
  umidadeEtemperatura();
  sensor_ultrassonico();
  presenca_dectected();
  print_lcd();
  delay(tempo);
}

void calcularCorrente(){
  emon1.calcVI(20,100); //FUNÇÃO DE CÁLCULO (20 SEMICICLOS / TEMPO LIMITE PARA FAZER A MEDIÇÃO)
  double currentDraw = emon1.Irms; //VARIÁVEL RECEBE O VALOR DE CORRENTE RMS OBTIDO 
  totalCorrente += currentDraw;
  Serial.print("Corrente medida: "); //IMPRIME O TEXTO NA SERIAL
  Serial.print(currentDraw); //IMPRIME NA SERIAL O VALOR DE CORRENTE MEDIDA
  Serial.print("  -  ");
  Serial.print(totalCorrente); //IMPRIME NA SERIAL O VALOR DE CORRENTE MEDIDA
  Serial.println("A"); //IMPRIME O TEXTO NA SERIAL
}


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


void loop() {
  
  if(ja_configurado==0){
    Rele_R1_Desligado();
    setMaxUS();
    ja_configurado = 1;
  }
  
  while(1){        
    Rele_R1_Desligado();    
    while( cmMsec > (maxDistUS*0.95)){                    
        segundosEconomia = millis() / 1000;
        calcularCorrente();
        actions(5000);        
    }

    Rele_R1_Ligado();
    if( cmMsec < (maxDistUS*0.95)){
      while(lerPIR==HIGH && cmMsec < (maxDistUS*0.95) ){
        calcularCorrente();
        actions(5000);        
      }
    }
  }  
}


/*
void loop() { 
  Rele_R1_Desligado();
  setMaxUS();

  while(1){
    actions(3000);  
  
    if( lerPIR == LOW || cmMsec > (maxDistUS*0.9)){
        Rele_R1_Desligado();      
        segundosEconomia = millis() / 1000;
        //delay(1000);
        
    }else if(lerPIR == HIGH || cmMsec < (maxDistUS*0.9)){
      Rele_R1_Ligado();         
      
      while( cmMsec < (maxDistUS*0.9) ){
        actions(4000);
        if( lerPIR == LOW || cmMsec > (maxDistUS*0.9)){
          break;  
        }
      }
    }
  }
}

*/
