#include <SoftwareSerial.h>

#define RxD 2
#define TxD 3
#define LED1 5
#define LED2 6
#define LED3 7
#define PUMP 9
#define WS A0
#define FLUX A2

SoftwareSerial BTserial(RxD, TxD); // RX | TX

//const int WS = A0;
const long baudRate = 9600; 
const int water_threshold = 700;
const int water_check_interval = 1000;

char c=' ';
String s="";
int have_water = 0;
int last_water_check = 0;
float vol = 0.0;


int check_water(int wt){
  int measure = 0;
  measure = analogRead(WS);
  last_water_check = millis();
  if((measure)< wt){
    return(1);
  }else{
    return(0);
  }
};

int readFluxSensor(){
  return(1);  
}

int check_pump(){
  if(have_water){
    int v = 0;
    digitalWrite(PUMP,HIGH);
    digitalWrite(LED3,HIGH);
    delay(2000);
    v = readFluxSensor();
    digitalWrite(PUMP,LOW);
    digitalWrite(LED3,LOW);
    if(v){
      return(1);
    }
  }
  return(0);  
}

int pump_water(float volume){
  if(have_water){
    float v = 0.0;
    digitalWrite(PUMP,HIGH);
    digitalWrite(LED3,HIGH);
    while(have_water && v<volume){
      have_water = check_water(water_threshold);
      v += readFluxSensor();
      delay(500);
    }
    digitalWrite(PUMP,LOW);
    digitalWrite(LED3,LOW);
    if(have_water){
      return(1);
    }    
  }
  return(0);
}

void setup() {
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(PUMP, OUTPUT);

  BTserial.begin(baudRate);

  have_water = check_water(water_threshold);
  check_pump();
  
}

float btReadFloat(){
  float n;
  c = BTserial.read();
  while(c!='\n'){
    s+=c;
    c = BTserial.read();
  }
  n = s.toFloat();
  s="";
  return(n);
}

void loop() {

  if((millis()-last_water_check) > water_check_interval){
    have_water = check_water(water_threshold);
    if(have_water){
      digitalWrite(LED2,HIGH);
    }else{
      digitalWrite(LED2,LOW);
    }
  }
  
  if (BTserial.available()){
    
    digitalWrite(LED1,HIGH);
    
    c = BTserial.read();

    switch(c){
      
      case 'w':
        if(have_water){
          BTserial.println("Tem agua");
        }else{
          BTserial.println("Nao tem agua");
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
        
        vol = btReadFloat();
                
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
    }
    
  }else{
    digitalWrite(LED1,LOW);
  }

}
