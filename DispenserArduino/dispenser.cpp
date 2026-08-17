#include "timerISR.h"
#include "helper.h"
#include "periph.h"
#include "serialATmega-4.h"
#include "ShiftReg.h"
#include <Arduino.h>

#define NUM_TASKS 3

typedef struct _task{
signed char state;
unsigned long period;
unsigned long elapsedTime;
int (*TickFct)(int);
} task;

const unsigned long GCD_PERIOD = 1;
task tasks[NUM_TASKS];

enum stepper{start_stepper, ccw, cw, go_up, refillAids};

int phases[8] = {0b0001, 0b0011, 0b0010, 0b0110, 0b0100, 0b1100, 0b1000,
0b1001};
int phases_rev[8] = {
 0b1001,
 0b1000,
 0b1100,
 0b0100,
 0b0110,
 0b0010,
 0b0011,
 0b0001
};



#define SERVO1 PB1   // Arduino pin 9
#define SERVO2 PB2   // Arduino pin 10
uint8_t motor1_out = 0;   // Q0–Q3
uint8_t motor2_out = 0;   // Q4–Q7


bool clockwise = 0;
bool cclockwise = 0;
bool right = 1;
bool left = 1;
bool round1 = 1;



unsigned long displacement = 0;
bool bandaid_var;
bool pill_var;
int numAids = 7;

enum pillStates { pill_start , dispensePill};
enum bandaidStates { start_bandaid, open_dispenser};


unsigned long rotation = 0;


bool stepper_idle = 0;
unsigned char j1 = 0;
unsigned char j2 = 0;

unsigned long countB = 0;
unsigned long countA = 0;
int bandaidAmount[8] = {
3200,//1
3200,//2
2800,//3
2800,//4
2550,//5
2550, // 6
2350,//7
2300//8
};



unsigned long config = 0;
unsigned char j = 8;
unsigned char num_pills = 8;
int TickFct_pill(int stateNUM){
    switch (stateNUM) {
        case pill_start:
          if(pill_var){
            stateNUM = dispensePill;
            pill_var = false;
            break;
          }
          break;
          case dispensePill:
          config++;
          int8_t p = phases[j];
          j = (j == 0) ? 7 : j - 1;

          PORTB &= ~((1<<0)|(1<<3)|(1<<4)|(1<<5));

          PORTB |= ((p & 0b0001) << 3);
          PORTB |= ((p & 0b0010) << 3);
          PORTB |= ((p & 0b0100) << 3);
          PORTB |= ((p & 0b1000) >> 3);
          if(config == 1500){
            stateNUM = start_stepper;
            //serial_println(config);
            num_pills--;
            //serial_println(num_pills);
            config = 0;
            break;
          }
        break;
        // case configPill:
        //   config++;

          
        //   j = (j == 0) ? 7 : j - 1;

        //   uint8_t p = phases[j];

        //   PORTB &= ~((1<<0)|(1<<3)|(1<<4)|(1<<5));

        //   PORTB |= ((p & 0b0001) << 3);
        //   PORTB |= ((p & 0b0010) << 3);
        //   PORTB |= ((p & 0b0100) << 3);
        //   PORTB |= ((p & 0b1000) >> 3);

        //   if(config == 1000){
        //       stateNUM = start_stepper;
        //       config = 0;
        //       serial_println("finished config");
        //   }
        //   break;
        
        // case refill:
        // config++;
        // j = (j + 1) % 8;
        // PORTB &= ~((1<<0)|(1<<3)|(1<<4)|(1<<5));

        //   PORTB |= ((p & 0b0001) << 3);
        //   PORTB |= ((p & 0b0010) << 3);
        //   PORTB |= ((p & 0b0100) << 3);
        //   PORTB |= ((p & 0b1000) >> 3);
        // if(config == 2800){
        //   stateNUM = start_stepper;
        //   config = 0;
        //   num_pills = 4;
        //   break;
        // }
        default:
            //stateNUM = start_stepper;
            break;
    }
    return stateNUM;

}

enum receiveStates{receive_start};
int TickFct_receiver(int stateNUM){
    switch (stateNUM) {
        case receive_start:
          if(Serial.available()){
            char receivedChar = Serial.read();
            if(receivedChar == 'p' && num_pills > 0){
              pill_var = true;
            }
            else if(receivedChar == 'b' && numAids >= 0){
              bandaid_var = true;
            }
          }

        break;
        // }
        default:
            //stateNUM = start_stepper;
            break;
    }
    return stateNUM;

}



// 1 bandaid  -  OCR1A = 3200;
// 2 bandaid  -  OCR1A = 3200;
// 3 bandaid -  OCR1A = 2800;
// 4 bandaid -  OCR1A = 2800;
 // 5 bandaid  - 2550
 // 6 bandaid  - 2550
  //7 bandaid  - 2200
 // 8 bandaid  - 2200

 unsigned long bandAngle = bandaidAmount[numAids];
int TickFct_stepper(int stateNUM){

    switch(stateNUM){
        case start_stepper:
            OCR1A = bandAngle;//push down badaids // 999 - goes up , 4999 - hella down
            //serial_println(bandAngle);
            OCR1B = 4100; // change to 4100 lift banaids
            if(bandaid_var){
              stateNUM = ccw;// un comment
              bandaid_var = false;
            }
        break;
        
        case go_up:
        countB--;
        countA--;
        OCR1B = countB;
        OCR1A = countA;
        //serial_println(count);
        if (countB == 2800){
          stateNUM = cw;
          //OCR1A = bandAngle;
          break;
        }
        break;
        case ccw:
            OCR1A = bandAngle; // go down every time bandaids come out // put in 10 
            rotation ++;
            //serial_println(bandAngle);
            displacement--;
            j2= (j2 == 0) ? 7 : j2 - 1;
            motor2_out = phases[j2];
            j1 = (j1 == 0) ? 7 : j1 - 1;
            //j2= (j2 == 0) ? 7 : j2 - 1;
            motor1_out = phases[j1];   // stepper 1
                    // stepper 2 OFF
            write_74HC595(motor1_out | motor2_out << 4);
          if(rotation == 7000){
            stateNUM = go_up;
            OCR1A = 3800;
            countB = OCR1B;
            countA = OCR1A;
            //OCR1B = 3000;
          }
        break;
        case cw:
            //OCR1A = 999;//up
            
            rotation --;
            j1= (j1 + 1) % 8;
            j2 = (j2 + 1) % 8;
            displacement++;
            
            motor2_out = phases[j2];   // update motor 2
            motor1_out = phases[j1]; 
            write_74HC595(motor1_out | motor2_out << 4);
            if(rotation == 0){
              stateNUM = start_stepper;
              numAids--;
              bandAngle = bandaidAmount[numAids];
              //serial_println(bandAngle);
              //OCR1A = 2999;
              //OCR1A = bandAngle;
            }
        break;
        
    }

    return stateNUM;
}






void TimerISR() {
  for ( unsigned int i = 0; i < NUM_TASKS; i++ ) {
    if ( tasks[i].elapsedTime == tasks[i].period ) {
      tasks[i].state = tasks[i].TickFct(tasks[i].state);
      tasks[i].elapsedTime = 0;
    }
    tasks[i].elapsedTime += GCD_PERIOD;
  }
}
/* ---------------------------------------- */

int main(void) {

DDRC = 0x00;
PORTC = 0x00;
DDRB = 0x00;
PORTB = 0x00;
//DDRD = 0x00;
//PORTD = 0x00;
Serial.begin(9600);

DDRB |= (1<<SERVO1) | (1<<SERVO2);
initialise_74HC595();
ADC_init();
//serial_init(9600);

TCCR1A = (1<<COM1A1) | (1<<COM1B1) | (1<<WGM11);
TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS11);

ICR1 = 39999;      // 20ms period (50Hz)

DDRB |= (1<<PB3) | (1<<PB4) | (1<<PB5) | (1<<PB0);

unsigned char i = 0;

tasks[i].state = start_stepper;
tasks[i].period = 5;//fix
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_stepper;
i++;

tasks[i].state = receive_start;
tasks[i].period = 5;
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_receiver;
i++;
tasks[i].state = pill_start;
tasks[i].period = 5;//fix
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_pill;


TimerSet(GCD_PERIOD);
TimerOn();

while (1) {}
return 0;
}