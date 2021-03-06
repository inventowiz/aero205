#include <SoftwareSerial.h>
#include <Servo.h>

//#include <RCArduinoFastLib.h>
// Used in accordance with GNU GPL
//   https://github.com/scottjgibson/RCArduinoFastLib

// Impeller Motor
#define MOTOR_OUT_A       3
#define MOTOR_OUT_DIRA_1  8
#define MOTOR_OUT_DIRA_2  9

// Propulsion Motor
#define MOTOR_OUT_B       6 //AND 12
#define MOTOR_OUT_DIRB_1  5
#define MOTOR_OUT_DIRB_2  11

#define SERVO_OUT_PIN 10

#define RC_IN_PIN   2    // Don't need to define, just for reference (hard-coded).
#define LED_OUT_PIN 13

#define BTRXIN     A5
#define BTTXOUT   A4

// Global data
volatile uint32_t ulCounter = 0;
uint16_t PULSEIN[5];

SoftwareSerial bluetooth(BTTXOUT,BTRXIN);
Servo fins;

int lastAngle = 0;
int lastPropSpeed = 0;
int lastImpSpeed = 0;

char command[4];
char oldCommand[4];

void setup()
{
  Serial.begin(115200);
  
  bluetooth.begin(115200);
  bluetooth.print("$");  // Print three times individually
  bluetooth.print("$");
  bluetooth.print("$");  // Enter command mode
  delay(100);  // Short delay, wait for the Mate to send back CMD
  bluetooth.println("U,9600,N");  // Temporarily Change the baudrate to 9600, no parity
  // 115200 can be too fast at times for NewSoftSerial to relay the data reliably
  bluetooth.begin(9600);  // Start bluetooth serial at 9600
  
  fins.attach(SERVO_OUT_PIN,600,2400);
  
  memset(PULSEIN,sizeof(uint16_t)*5,0);
  
  pinMode(MOTOR_OUT_A,OUTPUT);
  pinMode(MOTOR_OUT_B,OUTPUT);
  pinMode(MOTOR_OUT_DIRA_1,OUTPUT);
  pinMode(MOTOR_OUT_DIRA_2,OUTPUT);
  pinMode(MOTOR_OUT_DIRB_1,OUTPUT);
  pinMode(MOTOR_OUT_DIRB_2,OUTPUT);
  pinMode(LED_OUT_PIN,OUTPUT);
  
  for(int i=0;i<3;i++){
    // Flash LED 3 times when ready
    digitalWrite(LED_OUT_PIN,HIGH);
    delay(100);
    digitalWrite(LED_OUT_PIN,LOW);
    delay(100);
  }
}

void loop()
{
  checkBT();  
  if(!(millis()%100)) sendData(lastAngle,0,lastImpSpeed,lastPropSpeed,0,millis()/1000); // every 100ms
  //if(!(millis()%200)) finsServo(lastAngle); // every 200ms
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
  if(abs(vel - lastImpSpeed) < 20) return;
  if(vel==0){
    motorMove(MOTOR_OUT_B,0,0);
    lastImpSpeed = 0;
    return;
  }
  // ramp up over 20*50 ms
  int tempvel = lastImpSpeed;
  while((tempvel += ((vel/20) % 255)) < vel){
    motorMove(MOTOR_OUT_B,tempvel,0); //always reverse
    lastImpSpeed = tempvel;
    delay(50);
  }
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
  
  if(abs(lastAngle - angle) < 10){
    return;
  }
  
  pulseWidth = map(angle,0,180,MIN,MAX);
  
  fins.writeMicroseconds(pulseWidth);
  //bit bang it.
  //noInterrupts();
  //for(int i=0;i<20;i++){
  //  digitalWrite(SERVO_OUT_PIN,HIGH);
  //  delayMicroseconds(pulseWidth);
  //  digitalWrite(SERVO_OUT_PIN,LOW);
  //  delayMicroseconds(2000);
  //}
  //interrupts();
  lastAngle = angle;
}

void buttonByteDecode(char buttons){
   // 7  6  5  4  3  2  1  0
   // Ft Bk H  A  B  C  D  E
   
   int ABCoffcount = 0;
   
   if(buttons & 0x80){
     // Front Lights
   }else{
     
   }
   if(buttons & 0x40){
     // Back Lights
   }else{
     
   }
   if(buttons & 0x20){
     // Horn, resets device
     //asm volatile ("  jmp 0");
   }else{
     
   }
   if(buttons & 0x10){
     // Channel A
     impellerMotor(0.2*255);
   }else{
     ABCoffcount++;
   }
   if(buttons & 0x08){
     // Channel B
     impellerMotor(0.35*255);
   }else{
     ABCoffcount++;
   }
   if(buttons & 0x04){
     // Channel C
     impellerMotor(0.60*255);
   }else{
     ABCoffcount++;
   }
   
   if(ABCoffcount >=3)
     impellerMotor(0);
}

void checkBT(){
  if(bluetooth.available() >= 4){ 
    //while((bluetooth.peek() > 0xf5) && (bluetooth.peek() < 0xf1)) bluetooth.read();
    // Flash LED every time we get a BT packet
    digitalWrite(LED_OUT_PIN,HIGH);
    // Read in command
    for(int i=0;i<4;i++){
      oldCommand[i] = command[i];
      command[i] = bluetooth.read();
    }
    
    switch(command[0]){
      case 0xF1:
        if(oldCommand[1] != command[1]) propMotor(command[1],0); // forward speed
        break; 
      case 0xF2:
        if(oldCommand[1] != command[1]) propMotor(command[1],1); // reverse speed
        break;
      case 0xF3:
        propMotor(0,1); // zero speed
        break;
      case 0xf5:
        propMotor(0,1); // zero speed
        finsServo(90);
        impellerMotor(0);
        break;
      default:
        //not used
        bluetooth.flush();
        digitalWrite(LED_OUT_PIN,LOW);
        return; //out of function
        
    }
    if(oldCommand[2] != command[2]) finsServo((unsigned char)command[2]); // command[2] is steering angle
    if(oldCommand[3] != command[3]) buttonByteDecode(command[3]);
    digitalWrite(LED_OUT_PIN,LOW);
  }
}

void sendData(float a0,float a1,char a2,char a3, char a4, char a5){
  union _analog0{
    byte a[4];  
    float b;
  }analog0;
  union _analog1{
    byte a[4];  
    float b;
  }analog1;
  
  analog0.b = a0;
  analog1.b = a1;
  noInterrupts();
  bluetooth.write(0xee);
  bluetooth.write(analog0.a[0]); // Analog 0
  bluetooth.write(analog0.a[1]); // Analog 0
  bluetooth.write(analog0.a[2]); // Analog 0
  bluetooth.write(analog0.a[3]); // Analog 0
  bluetooth.write(analog1.a[0]); // Analog 1
  bluetooth.write(analog1.a[1]); // Analog 1
  bluetooth.write(analog1.a[2]); // Analog 1
  bluetooth.write(analog1.a[3]); // Analog 1
  bluetooth.write(a2); // Analog 2
  bluetooth.write(a3); // Analog 3
  bluetooth.write(a4); // Analog 4
  bluetooth.write(a5); // Analog 5
  bluetooth.write(0xcc);
  interrupts();
}
