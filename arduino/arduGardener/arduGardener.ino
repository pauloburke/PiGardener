#include <SoftwareSerial.h>

#define RxD 2
#define TxD 3

SoftwareSerial BTserial(RxD, TxD); // RX | TX

const int WS = A0;
const long baudRate = 9600; 
char c=' ';
const int water_threshold = 700;
int have_water = 0;
int last_water_check = 0;
const int water_check_interval = 1000;

int check_water(int wt){
  int measure = 0;
  measure = analogRead(WS);
  last_water_check = millis();
  Serial.println(measure);
  if((measure)< wt){
    return(1);
  }else{
    return(0);
  }
};

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT);

  BTserial.begin(baudRate);

  have_water = check_water(water_threshold);

  Serial.begin(9600);

}

void loop() {

  if((millis()-last_water_check) > water_check_interval){
    have_water = check_water(water_threshold);
    Serial.println(have_water);
  }
  
  if (BTserial.available()){
    c = BTserial.read();
    if(c=='w'){
      if(have_water){
        BTserial.println("Tem agua");
      }else{
        BTserial.println("Nao tem agua");
      }
    }    
  }

  

}
