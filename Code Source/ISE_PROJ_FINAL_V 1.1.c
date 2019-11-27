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
int jaConfigurado = 0;
const int tempoDeConfig = 10;

/* Contagem de tempo */
int TempoLigado = 0;
int segundosEconomia = 0;

/* Pegar distância do ambiente para calcular uma média e usar como maior distância */
int DistanciaMaximaSensorUltrassonico = 0;


/* Configuração do Sensor Ultrassonico */
const int pino_trigger = 12;
const int pino_echo    = 14;
Ultrasonic ultrasonic(pino_trigger, pino_echo);
int distanciaAtual=0.0;


/* Configuração do Sensor de temperatura e umidade*/
const int DHTPIN = 10; // pino que estamos conectado
#define DHTTYPE DHT11 // DHT 11
uint32_t delayMS;
DHT_Unified dht(DHTPIN, DHTTYPE);
int umidade=0;
int temperatura=0;

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
void configLCDConfigurando(int tempoSegundos){
  //lcd.clear();
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("CONFIGURANDO ...");
  lcd.setCursor(0, 1);
  lcd.printf("%d SEGUNDOS", tempoDeConfig-tempoSegundos);
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
  int distancia = distanciaAtual;

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
  distanciaAtual = ultrasonic.convert(microsec, Ultrasonic::CM);
  
    Serial.print("DISTANCIA: ");
    Serial.print(DistanciaMaximaSensorUltrassonico);
    Serial.print(" -> ");
    Serial.println(distanciaAtual);
}

/* get presença */
void LerSensorPIR(){
    lerPIR = digitalRead(PIR);    
}



/* capturação de distancias para calibragem */
void setMaiorDistancia(){
  int tempoSegundos = 0;
  int v[20] = {0};
  int i = 0;
  
  while( tempoSegundos < tempoDeConfig ){                
    tempoSegundos = millis() / 1000;        
    
    sensor_ultrassonico();
    int distanciaAgora = distanciaAtual;
    v[i] = distanciaAgora;    
    i=(++i)%20 ;
    configLCDConfigurando(tempoSegundos);
    delay(200);
  }
  DistanciaMaximaSensorUltrassonico = ((v[4]+v[9]+v[14]+v[19])/4);
}

/* função que ativa uma rodada de captura de dados dos sensores */
void verificarSensores(){
  umidadeEtemperatura();
  sensor_ultrassonico();
  LerSensorPIR();
  print_lcd();
}

/* calcular corrente */
void calcularConsumo(int tempo){
  emon1.calcVI(20,100);
  double currentDraw = emon1.Irms;
  totalCorrente += (currentDraw * tempo);
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

void verificaDistancia(){
  if(distanciaAtual > DistanciaMaximaSensorUltrassonico){
    int t = 0;
    int valuesD[10];  
    while(t < 20){
      sensor_ultrassonico();      
      valuesD[ t%10 ] = distanciaAtual;
      delay(10);
    }
    
    distanciaAtual = (valuesD[0]+valuesD[5])/2;
  }
}

void modoConfiguracao(){
  Serial.println("MODO CONFIGURACAO");
  if(!jaConfigurado){
    jaConfigurado = 1;
    Rele_R1_Desligado();
    setMaiorDistancia();
  } 
}

void modoStadby(){
  Serial.println("NAO ATIVO");
  while(distanciaAtual > (DistanciaMaximaSensorUltrassonico*0.90)){                    
    segundosEconomia = millis() / 1000;
    TempoLigado = millis() / 1000;    
    verificarSensores();
    calcularConsumo(5);
    delay(5000);    
  }
}

void modoAtivo(){
  Serial.println("ATIVO");
  while(lerPIR==HIGH && distanciaAtual<(DistanciaMaximaSensorUltrassonico*0.90) ){
    TempoLigado = millis()/1000;
    verificarSensores();
    calcularConsumo(20);
    verificaDistancia();
    delay(10000);
  }
}

/* Programa principal */
void loop(){    
  if(!jaConfigurado)
    modoConfiguracao();

  while(1){      
    Rele_R1_Desligado();
    modoStadby();
  
    Rele_R1_Ligado();
    modoAtivo();      
  }  
}
