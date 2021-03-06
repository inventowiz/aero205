#include <IRremote.h>

#define RECV_PIN     2
#define MOTOR_PIN   10
#define POT_PIN     A0
#define LED         13

const int time = 15*1000;

IRrecv irrecv(RECV_PIN);
decode_results results;

unsigned long int trigger = 0xC1AA0DF2; //default to power button
unsigned int count = 0;

void setup()
{
  Serial.begin(115200);
  irrecv.enableIRIn(); // Start the receiver
  
  pinMode(MOTOR_PIN,OUTPUT);
  pinMode(POT_PIN,INPUT);
  pinMode(LED,OUTPUT);
  
  Serial.println("PROGRAMMING MODE");
  while(millis() < 3000){
    //programming mode 
    digitalWrite(LED,HIGH);
    if (irrecv.decode(&results)) {
      Serial.print("New Code: ");
      Serial.println(results.value, HEX);
      trigger = results.value;
      irrecv.resume(); // Receive the next value
      
      //Dignal that we got a new value
      digitalWrite(LED,LOW);
      delay(250);
      digitalWrite(LED,HIGH);
      delay(250);
      digitalWrite(LED,LOW);
      delay(250);
      digitalWrite(LED,HIGH);
      delay(250);
      digitalWrite(LED,LOW);
    }
  }
  Serial.println("DEVICE READY TO FLY");
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    
    if(results.bits == 0x20) count++;
    
    irrecv.resume(); // Receive the next value
    digitalWrite(LED,HIGH);
  }
  delay(10);
  digitalWrite(LED,LOW);
  
  if(count>=1 or (millis() >= time)){
    analogWrite(MOTOR_PIN,0);
  }else{
    double pot = (double)analogRead(POT_PIN);
    pot = map(pot,0,1023,0,255);
    analogWrite(MOTOR_PIN,255);
    Serial.println(pot);
  }
}
