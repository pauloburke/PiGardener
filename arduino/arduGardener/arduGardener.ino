#include <SoftwareSerial.h>

#define sensorInterrupt 0
#define sensorPin 2
#define RxD 4
#define TxD 3
#define LED2 6
#define LED3 5
#define PUMP 10
#define WS 9
#define BTN 11

SoftwareSerial BTserial(RxD, TxD); // RX | TX

const long baudRate = 9600; 
const int water_check_interval = 1000;
float calibrationFactor = 4.5;  


char c=' ';
String command = "";
int have_water = 0;
int last_water_check = 0;
float vol = 0.0;
volatile byte pulseCount;


void read_bluetooth(){ //
  while(BTserial.available()){
    c = BTserial.read();
    command.concat(c);
    if(c == '\r' || c == '\n'){ // find if there is carriage return
      break;
    }
    while(!BTserial.available()){};
  }
  command = command.substring(0);
  //if(command.length()>0) BTserial.println(command);
}

int check_water(){
  if(digitalRead(WS) == LOW) 
  {
    return(1);
  } 
  else 
  {
    return(0);
  }
}

void pulseCounter()
{
  pulseCount++;
}

bool readFluxSensor(float v){

  float ml_v = v*1000;  
  float flowRate = 0.0;
  unsigned int flowMilliLitres = 0;
  unsigned long totalMilliLitres = 0;  
  unsigned long oldTime = 0;
  unsigned long initTime = millis();
  pulseCount = 0;
  
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);

  while(totalMilliLitres < ml_v && have_water){
    if((millis() - initTime) > 1000*60*5) break;
    if((millis() - oldTime) > 1000){
        detachInterrupt(sensorInterrupt);
        flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
        oldTime = millis();
        flowMilliLitres = (flowRate / 60) * 1000;
        totalMilliLitres += flowMilliLitres;
        pulseCount = 0;
        attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
        have_water = check_water();
    }    
  }
  detachInterrupt(sensorInterrupt);

  return(totalMilliLitres >= ml_v);

}

bool check_pump(){
  bool v = false;
  if(have_water){
    digitalWrite(PUMP,HIGH);
    digitalWrite(LED3,HIGH);
    v = readFluxSensor(0.1);
    digitalWrite(PUMP,LOW);
    digitalWrite(LED3,LOW);
  }
  return(v);  
}

int pump_water(float volume){
  bool flag = false;
  if(have_water){
    digitalWrite(PUMP,HIGH);
    digitalWrite(LED3,HIGH);
    flag = readFluxSensor(volume);
    digitalWrite(PUMP,LOW);
    digitalWrite(LED3,LOW);    
  }
  return(flag);
}

void check_leds(){
  digitalWrite(LED2,HIGH);
  digitalWrite(LED3,HIGH);
  delay(1000);
  digitalWrite(LED2,LOW);
  digitalWrite(LED3,LOW);
  delay(400);
  for(int i=0;i<10;i++){
    digitalWrite(LED2,HIGH);
    delay(100);
    digitalWrite(LED2,LOW);
    digitalWrite(LED3,HIGH);
    delay(100);
    digitalWrite(LED3,LOW);
  }
  digitalWrite(LED2,LOW);
  digitalWrite(LED3,LOW);
}

void setup() {
  
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(WS, INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  BTserial.begin(baudRate);

  check_leds();

  have_water = check_water();
  
}


void loop() {

  if((millis()-last_water_check) > water_check_interval){
    have_water = check_water();
    if(have_water){
      digitalWrite(LED2,HIGH);
    }else{
      digitalWrite(LED2,LOW);
    }
  }

  if(digitalRead(BTN) == LOW) 
  {
    digitalWrite(LED3,HIGH);
    digitalWrite(PUMP,HIGH);
  } 
  else 
  {
    digitalWrite(LED3,LOW);
    digitalWrite(PUMP,LOW);
  }

  read_bluetooth();
  
  if (command.length()>0){
    
    c = command[0];

    switch(c){
      
      case 't':
        BTserial.println("ok");
        break;
      
      case 'w':
        if(have_water){
          BTserial.println("Have water");
        }else{
          BTserial.println("No water");
        };
        break;
        
      case 'c':
        if(check_pump()){
          BTserial.println("Pump Ok");
        }else{
          BTserial.println("Pump not Ok");
        };
        break;

      case 'p':

        vol = command.substring(1).toFloat();
                
        if(check_pump()){
          BTserial.println("Pump Ok");
        }else{
          BTserial.println("Pump not Ok");
          break;
        };
        BTserial.println("Pumping water...");
        if(pump_water(vol)){
          BTserial.print("Irrigation of ");
          BTserial.print(vol);
          BTserial.println(" liters done!");
        }else{
          BTserial.println("Out of water!");
        }
        break;
    }
    command = "";
  }

}
