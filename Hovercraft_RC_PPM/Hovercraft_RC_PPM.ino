#include <Servo.h>

// Impeller Motor
#define MOTOR_OUT_A       3
#define MOTOR_OUT_DIRA_1  8
#define MOTOR_OUT_DIRA_2  9

// Propulsion Motor
#define MOTOR_OUT_B       6 //AND 12
#define MOTOR_OUT_DIRB_1  5
#define MOTOR_OUT_DIRB_2  11

#define SERVO_OUT_PIN 10

#define LED_OUT_PIN 13

// RC receiver Data
#define PPM_PIN    2
volatile uint16_t curPulse = 0;
volatile uint32_t PULSESTART = 0;
volatile uint32_t PULSES[3] = {0,0,0};

// Globals
Servo fins;
int curAngle = 0;
int curPropSpeed = 0;
int propDir = 0;
int curImpSpeed = 0;
int lastAngle = 90;
int lastPropSpeed = 0;
int lastImpSpeed = 0;

// Watchdog, reset in ISR
#define WD_TIMEOUT  500
volatile uint32_t wdStart = 0;

void setup()
{
 
  fins.attach(SERVO_OUT_PIN,600,2400);
  
  pinMode(MOTOR_OUT_A,OUTPUT);
  pinMode(MOTOR_OUT_B,OUTPUT);
  pinMode(MOTOR_OUT_DIRA_1,OUTPUT);
  pinMode(MOTOR_OUT_DIRA_2,OUTPUT);
  pinMode(MOTOR_OUT_DIRB_1,OUTPUT);
  pinMode(MOTOR_OUT_DIRB_2,OUTPUT);
  pinMode(LED_OUT_PIN,OUTPUT);
  
  // Interrupts for RC
  attachInterrupt(0,ISRPIN2,CHANGE);
  
  for(int i=0;i<3;i++){
    // Flash LED 3 times when ready
    digitalWrite(LED_OUT_PIN,HIGH);
    delay(100);
    digitalWrite(LED_OUT_PIN,LOW);
    delay(100);
  }
  wdStart = millis();
}

void loop()
{
  if(!(micros() % 1000)){ // every 1 ms
    digitalWrite(LED_OUT_PIN,HIGH);
    pulseToVal();
    impellerMotor(curImpSpeed);
    propMotor(curPropSpeed,propDir);
    finsServo(curAngle);
    digitalWrite(LED_OUT_PIN,LOW);
  }
  
  if(millis() - wdStart > WD_TIMEOUT){
    // Controller disconnected. 
    
    // Flash LED
    digitalWrite(LED_OUT_PIN,HIGH);
    delay(250);
    digitalWrite(LED_OUT_PIN,LOW);
    delay(250);
    
    // Disable Prop and Servo, leave impeller going.
    propMotor(0,0);
    finsServo(90);
  }
}

void motorMove(int motor, int spd, int dir){
//Move specific motor at speed and direction
//motor: MOTOR_OUT_A or MOTOR_OUT_B
//speed: 0 is off, and 255 is full speed
//direction: 0 clockwise, 1 counter-clockwise

  boolean inPin1 = LOW;
  boolean inPin2 = HIGH;

  if(dir == 1){
    inPin1 = HIGH;
    inPin2 = LOW;
  }

  if(motor == MOTOR_OUT_A){
    digitalWrite(MOTOR_OUT_DIRA_1, inPin1);
    digitalWrite(MOTOR_OUT_DIRA_2, inPin2);
    analogWrite(motor, spd);
  }else{
    digitalWrite(MOTOR_OUT_DIRB_1, inPin1);
    digitalWrite(MOTOR_OUT_DIRB_2, inPin2);
    analogWrite(motor, spd);
  }
}

void impellerMotor(int vel){
  //Ramp up the impeller
  //if(abs(vel - lastImpSpeed) < 10) return;
  if(vel==0){
    motorMove(MOTOR_OUT_B,0,0);
    lastImpSpeed = 0;
    return;
  }
  motorMove(MOTOR_OUT_B,vel,0); //always reverse
}
void propMotor(int vel,int dir){
  //dir: 1 forward, 0 backward
  motorMove(MOTOR_OUT_A,vel,dir);
  lastPropSpeed = vel;  
}
void finsServo(unsigned int angle){
  const int MAX = 600;
  const int MIN = 2400;
  unsigned int pulseWidth = 0;
  
  if(abs(lastAngle - angle) < 5){
    // Filtering
    return;
  }
  
  pulseWidth = map(angle,0,180,MIN,MAX);
  
  fins.writeMicroseconds(pulseWidth);

  lastAngle = angle;
} 
 
void ISRPIN2(){
  // CHAN_1_PIN
   if(digitalRead(PPM_PIN) == HIGH){
     //rising
     PULSESTART = micros();
   }else{
     //falling
     uint16_t time = micros() - PULSESTART;
     if(time > 5000){
       // Greater than 5ms, therefore it's the gap.
       curPulse = 0;
       return;
     }
     if(curPulse > 2){
       // we have a problem, controller disconnected?
       // continue current state
       return; 
     }
     // Assign the pulse and increment the count.
     PULSES[curPulse++] = time;
   }
   wdStart = millis();
}
 
 
void pulseToVal(){
  
  //Impeller (CHAN_3_PIN) PULSES[2]
  if(PULSES[2] < 410){
    curImpSpeed = 0;
  }else{
    curImpSpeed = map(PULSES[2],410,1400,1,0.85*255);
  }
  
  //Propeller (CHAN_2_PIN) PULSES[1]
  if(PULSES[1] > 1050){ //forward
    propDir = 0;
    curPropSpeed = map(PULSES[1],1051,1500,0,255);
  }else if(PULSES[1] < 950){ //reverse
    propDir = 1;
    curPropSpeed = map(PULSES[1],949,400,0,255);
  }else{ //nothing
    propDir = 0;
    curPropSpeed = 0;
  }
  //Servo (CHAN_1_PIN) PULESE[0]
  curAngle = map(PULSES[0],600,1400,140,40);
}


