#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <LiquidCrystal.h>
#include "timerISR.h"
#include "serialATmega-4.h"
#include "periph.h"
#include <DHT.h>
#define DHTPIN A3
#define DHTTYPE DHT22
#define NUM_TASKS 4//TODO: Change to the number of tasks being used
//Task struct for concurrent synchSMs implmentations
typedef struct _task{
signed char state; //Task's current state
unsigned long period; //Task period
unsigned long elapsedTime; //Time elapsed since last task tick
int (*TickFct)(int); //Task tick function
} task;
//TODO: Define Periods for each task
// e.g. const unsined long TASK1_PERIOD = <PERIOD>
const unsigned long GCD_PERIOD = 5;//TODO:Set the GCD Period
task tasks[NUM_TASKS]; // declared task array with 5 tasks



bool pulse_var;




byte m0[8] = { B00000,
 B00000,
 B00000,
 B00000,
 B00000,
 B11111,
 B00000,
 B00000};
byte m1[8] = {
 B00000,
 B00001,
 B00011,
 B00110,
 B01100,
 B11000,
 B00000,
 B00000
};
byte m2[8] = {B00000,
 B00000,
 B00000,
 B00001,
 B00011,
 B00110,
 B01100,
 B11000};


byte m3[8] = {
 B00000,
 B10000,
 B11000,
 B01100,
 B00110,
 B00011,
 B00000,
 B00000
};
byte m4[8] = {
 B00000,
 B00000,
 B00000,
 B10000,
 B11000,
 B01100,
 B00110,
 B00011
};




DHT dht(DHTPIN, DHTTYPE);






// -------- LCD1602 --------
#define rs 2
#define enable 3
#define d4 4
#define d5 5
#define d6 6
#define d7 A5
unsigned char num_pills = 8;
unsigned char num_Aids = 8;


LiquidCrystal lcd(rs,enable,d4,d5,d6,d7);


// -------- LEFT TFT --------
#define CS_L   10
#define DC_L    9
#define RST_L   7


// -------- RIGHT TFT --------
#define CS_R    8
#define DC_R    9
#define RST_R   12//fix this


Adafruit_ST7735 tftL(CS_L, DC_L, RST_L);
Adafruit_ST7735 tftR(CS_R, DC_R, RST_R);


// ---------- FSM STATES ----------
enum EyeStates {EYE_OPEN, EYE_WAIT, EYE_CLOSE};
enum menuStates{menu_start, menuON, pulse, pill, bandAID, temp, dispay_pulse, display_temp};
volatile unsigned long msCount = 0;
unsigned long lastBeatTime = 0;
unsigned int BPM = 0;
unsigned int threshold = 530;
unsigned int lastDisplayedBPM = 0;


// ---------- TFT SELECT ----------
void selectLeft() {
 digitalWrite(CS_R, HIGH);
 digitalWrite(CS_L, LOW);
}


void selectRight() {
 digitalWrite(CS_L, HIGH);
 digitalWrite(CS_R, LOW);
}


// ---------- DRAW ONE EYE ----------
void drawEye(Adafruit_ST7735 &tft, int height)
{
 tft.fillScreen(ST77XX_BLACK);


 int w = 70;
 int x = (128 - w) / 2;
 int y = (128 - height) / 2;


 tft.fillRoundRect(x, y, w, height, 12, ST77XX_WHITE);
}




const uint8_t font5x7[10][5] = {
   {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
   {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
   {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
   {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
   {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
   {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
   {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
   {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
   {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
   {0x06, 0x49, 0x49, 0x29, 0x1E}  // 9
};








const uint8_t GLYPH_A[5] = {0x7E, 0x09, 0x09, 0x09, 0x7E};
const uint8_t GLYPH_B[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
const uint8_t GLYPH_D[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
const uint8_t GLYPH_E[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
const uint8_t GLYPH_I[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
const uint8_t GLYPH_L[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
const uint8_t GLYPH_M[5] = {0x7F, 0x02, 0x04, 0x02, 0x7F};
const uint8_t GLYPH_N[5] = {0x7F, 0x02, 0x04, 0x08, 0x7F};
const uint8_t GLYPH_P[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
const uint8_t GLYPH_S[5] = {0x26, 0x49, 0x49, 0x49, 0x32};
const uint8_t GLYPH_T[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
const uint8_t GLYPH_U[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
const uint8_t GLYPH_MINUS[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
const uint8_t GLYPH_SPACE[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

float humidity;
float temperature;

const uint8_t* getGlyph(char c) {
   switch (c) {
       case 'A': return GLYPH_A;
       case 'B': return GLYPH_B;
       case 'D': return GLYPH_D;
       case 'E': return GLYPH_E;
       case 'I': return GLYPH_I;
       case 'L': return GLYPH_L;
       case 'M': return GLYPH_M;
       case 'N': return GLYPH_N;
       case 'P': return GLYPH_P;
       case 'S': return GLYPH_S;
       case 'T': return GLYPH_T;
       case 'U': return GLYPH_U;
       case '-': return GLYPH_MINUS;
       case ' ': return GLYPH_SPACE;
       default:  return GLYPH_SPACE;
   }
}


// -----------------------------
// Pixel + rectangle
// -----------------------------
static inline void drawPixel(uint8_t x, uint8_t y, uint16_t color) {
   selectRight();
   tftR.drawPixel(x, y, color);
}


void fillRect(uint8_t x0, uint8_t x1, uint8_t y0, uint8_t y1, uint16_t color) {
   selectRight();
   tftR.fillRect(x0, y0, (x1 - x0 + 1), (y1 - y0 + 1), color);
}
void drawChar(uint8_t x, uint8_t y, char c, uint8_t scale, uint16_t color) {
   const uint8_t* glyph = getGlyph(c);


   for (uint8_t col = 0; col < 5; col++) {
       uint8_t bits = glyph[col];
       for (uint8_t row = 0; row < 7; row++) {
           if (bits & (1 << row)) {
               for (uint8_t dx = 0; dx < scale; dx++) {
                   for (uint8_t dy = 0; dy < scale; dy++) {
                       drawPixel(x + col * scale + dx,
                                 y + row * scale + dy,
                                 color);
                   }
               }
           }
       }
   }
}


void drawString(uint8_t x, uint8_t y, const char* str, uint8_t scale, uint16_t color) {
   while (*str) {
       drawChar(x, y, *str, scale, color);
       x += (5 * scale) + scale;
       str++;
   }
}
void drawArrow(uint8_t x, uint8_t y, uint16_t color)
{
   for(uint8_t i = 0; i < 12; i++)
   {
       for(uint8_t j = 0; j <= i; j++)
       {
           drawPixel(x + j, y + i, color);
           drawPixel(x + j, y + (11 - i), color);
       }
   }
}


void drawMenu() {


   fillRect(0,127,0,31, ST77XX_BLUE);
   fillRect(0,127,32,63, ST77XX_RED);
   fillRect(0,127,64,95, ST77XX_YELLOW);
   fillRect(0,127,96,127, ST77XX_GREEN);


   // border
   fillRect(0,127,0,0, ST77XX_BLACK);
   fillRect(0,127,127,127, ST77XX_BLACK);
   fillRect(0,0,0,127, ST77XX_BLACK);
   fillRect(127,127,0,127, ST77XX_BLACK);


   // separators
   fillRect(0,127,31,31, ST77XX_BLACK);
   fillRect(0,127,63,63, ST77XX_BLACK);
   fillRect(0,127,95,95, ST77XX_BLACK);


   drawString(34, 9, "PULSE", 2, ST77XX_CYAN);
   drawString(34, 41, "PILLS", 2, ST77XX_WHITE);
   drawString(34, 73, "B-AID", 2, ST77XX_WHITE);
   drawString(40, 105, "TEMP", 2, ST77XX_WHITE);


   //drawArrow(8, 12, ST77XX_WHITE);
}




void CanvasCreate(uint8_t XS, uint8_t XE, uint8_t YS, uint8_t YE, uint16_t color)
{
 selectRight();


 int w = XE - XS + 1;
 int h = YE - YS + 1;


 tftR.fillRect(XS, YS, w, h, color);
}
void drawDigit(uint8_t x, uint8_t y, uint8_t digit, uint16_t color) {
    if (digit > 9) return;

    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = font5x7[digit][col];

        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                drawPixel(x + col, y + row, color);
            }
        }
    }
}




void displayBPM_topRight(unsigned int bpm) {

    CanvasCreate(95, 127, 0, 15, ST77XX_BLACK);

    uint8_t x = 100;
    uint8_t y = 4;

    if (bpm >= 100) {
        drawDigit(x,     y, (bpm / 100) % 10, ST77XX_WHITE);
        drawDigit(x + 6, y, (bpm / 10)  % 10, ST77XX_WHITE);
        drawDigit(x + 12,y, bpm % 10,        ST77XX_WHITE);
    }
    else if (bpm >= 10) {
        drawDigit(x,     y, (bpm / 10) % 10, ST77XX_WHITE);
        drawDigit(x + 6, y, bpm % 10,        ST77XX_WHITE);
    }
    else {
        drawDigit(x, y, bpm % 10, ST77XX_WHITE);
    }
}


void TimerISR() {
msCount += GCD_PERIOD;
for ( unsigned int i = 0; i < NUM_TASKS; i++ ) { // Iterate
//through each task in the task array
if ( tasks[i].elapsedTime == tasks[i].period ) { // Check if
tasks[i].state = tasks[i].TickFct(tasks[i].state); // Tick and
tasks[i].elapsedTime = 0; // Reset the
}
tasks[i].elapsedTime += GCD_PERIOD; // Increment
//the elapsed time by GCD_PERIOD
}
}





enum LCD1602{start_LCD};



int tempread;


bool menu = false;
int TickFct_MENU(int stateNUM){//have one task reading uart
   switch (stateNUM) {
       case menu_start:
           //Serial.write("pill");// write to pico
               //lcd.print("Number of Band Aids Left:");     
         if(Serial.available() ){


           char receivedChar = Serial.read();
           if(receivedChar == 'm'){//m
             menu = true;
             stateNUM = pulse;
             drawMenu();
             drawArrow(8, 12, ST77XX_WHITE);
             lcd.clear();
             lcd.setCursor(3, 0);//disiplay inventory
             lcd.print("Inventory:");
             lcd.setCursor(0, 1);
             lcd.print(" Pills:");
             lcd.print(num_pills);
             lcd.print("  BAs:");
             lcd.print(num_Aids);

             break;
           }
          
         }
         break;
       case pulse:
        if(Serial.available() ){
          
           char receivedChar = Serial.read();
           if(receivedChar == 'd'){//down
             stateNUM = pill;
             drawArrow(8, 12, ST77XX_BLUE);
             drawArrow(8, 40, ST77XX_WHITE);
             break;
           }
       
           else if(receivedChar == 'm'){
             stateNUM = dispay_pulse;
             tftR.fillScreen(ST77XX_BLACK);
             break;
           
           }
        }
       break;
       case dispay_pulse:
        
         displayBPM_topRight(BPM);
         if(Serial.available() ){
           char receivedChar = Serial.read();
           if(receivedChar == 'm'){//m
             stateNUM = menu_start;
             //drawMenu();
              lcd.clear();
              lcd.setCursor(3,0);
              lcd.write(byte(4));
              lcd.setCursor(4,1);
              lcd.write(byte(3));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(1));
              lcd.setCursor(12,0);
              lcd.write(byte(2));
              menu = false;
             break;
           }
          
         }
       break;
       case pill:
       if(Serial.available() ){
           char receivedChar = Serial.read();
           if(receivedChar == 'd'){//down
             stateNUM = bandAID;
             drawArrow(8, 40, ST77XX_RED);
             drawArrow(8, 74, ST77XX_BLACK);
             break;
           }
           else if(receivedChar == 'u'){//up
           stateNUM = pulse;
           drawArrow(8, 40, ST77XX_RED);
           drawArrow(8, 12, ST77XX_WHITE);
           break;
           }
           else if(receivedChar == 'm' && num_pills > 0){
             num_pills--;
             Serial.write('p');
             stateNUM = menu_start;
             lcd.clear();
              lcd.setCursor(3,0);
              lcd.write(byte(4));
              lcd.setCursor(4,1);
              lcd.write(byte(3));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(1));
              lcd.setCursor(12,0);
              lcd.write(byte(2));
              menu = false;
             break;
           }
       }
       break;
      case display_temp: {
    int tempread = int(temperature);

    drawDigit(88, 4, (tempread / 10) % 10, ST77XX_WHITE);
    drawDigit(94, 4, tempread % 10, ST77XX_WHITE); 
    //drawChar(100, 4, '°', 1, ST77XX_WHITE);
    drawChar(106, 4, 'C', 1, ST77XX_WHITE);

    if(Serial.available()){
        char receivedChar = Serial.read();
        if(receivedChar == 'm'){
            lcd.clear();
            lcd.setCursor(3,0);
            lcd.write(byte(4));
            lcd.setCursor(4,1);
            lcd.write(byte(3));
            lcd.write(byte(0));
            lcd.write(byte(0));
            lcd.write(byte(0));
            lcd.write(byte(0));
            lcd.write(byte(0));
            lcd.write(byte(0));
            lcd.write(byte(1));
            lcd.setCursor(12,0);
            lcd.write(byte(2));

            menu = false;
            stateNUM = menu_start;
        }
    }
    break;
}

       case bandAID:
       if(Serial.available() ){
           char receivedChar = Serial.read();
           if(receivedChar == 'u'){//up
           stateNUM = pill;
           drawArrow(8, 74, ST77XX_YELLOW);
           drawArrow(8, 40, ST77XX_WHITE);
           }
           else if(receivedChar == 'd'){
             stateNUM = temp;
             drawArrow(8, 74, ST77XX_YELLOW);
             drawArrow(8, 108, ST77XX_BLACK);
             break;
           }
           else if(receivedChar == 'm' && num_Aids > 0){
              Serial.write('b');
              num_Aids--;
              lcd.clear();
              lcd.setCursor(3,0);
              lcd.write(byte(4));
              lcd.setCursor(4,1);
              lcd.write(byte(3));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(0));
              lcd.write(byte(1));
              lcd.setCursor(12,0);
              lcd.write(byte(2));
              menu = false;
              stateNUM = menu_start;

           }
       }
       break;
       case temp:
       if(Serial.available() ){
           char receivedChar = Serial.read();
           if(receivedChar == 'u'){//up
           stateNUM = bandAID;
           drawArrow(8, 108, ST77XX_GREEN);
           drawArrow(8, 74, ST77XX_BLACK);
           }
           else if(receivedChar == 'm' ){
             stateNUM = display_temp;
             tftR.fillScreen(ST77XX_BLACK);
             break;
           }
       }
       break; 

       }
     
         return stateNUM;
}




unsigned char time_eye = 0;


int TickFct_Eyes(int eyeState)
{
 switch(eyeState)
 {
   case EYE_OPEN:
    
     selectLeft();
     drawEye(tftL, 60);
    
     if (!menu){
     selectRight();
     drawEye(tftR, 60);//if(menu not open)
     }
    // timer = millis();
     eyeState = EYE_WAIT;
     break;


   case EYE_WAIT:
     time_eye++;
     if(time_eye == 60  )
     {
       eyeState = EYE_CLOSE;
       time_eye = 0;
       break;
     }
     break;


   case EYE_CLOSE:
     selectLeft();
     drawEye(tftL, 6);


     if (!menu){
     selectRight();
     drawEye(tftR, 60);//if(menu not open)
     }
    //
     eyeState = EYE_OPEN;
     break;
 }
 return eyeState;
}

enum tempStates { temp_Start };


int tempTick(int state) {
   switch(state) {
    case temp_Start:
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    //serial_println(temperature);
    break;
    }
   return state;
}

enum PulseStates { P_WAIT, P_DETECTED };


int PulseTick(int state) {


   unsigned int signal = ADC_read(0);


   switch(state) {


       case P_WAIT:
           if(signal > threshold) {


               state = P_DETECTED;


               unsigned long delta = msCount - lastBeatTime;
               lastBeatTime = msCount;


               if(delta > 300) {   // ignore unrealistic beats
                   BPM = 60000 / delta;
               }
           }
           break;


       case P_DETECTED:
           if(signal < threshold) {
               state = P_WAIT;
           }
           break;
   }
   return state;
}
//TODO: Create your tick functions for
//each task
int main(void) {
 init();
 ADC_init();
 SPI.begin();
 dht.begin();

 //serial_init(9600);
 pinMode(rs, OUTPUT);
 pinMode(enable, OUTPUT);
 pinMode(d4, OUTPUT);
 pinMode(d5, OUTPUT);
 pinMode(d6, OUTPUT);
 pinMode(d7, OUTPUT);
 //DDRC = DDRC & 0xFE;
 lcd.begin(16,2);
 pinMode(CS_L, OUTPUT);
 pinMode(CS_R, OUTPUT);
 pinMode(A0,INPUT);
 pinMode(A3,INPUT);
 Serial.begin(9600);
 // LEFT
 selectLeft();
 tftL.initR(INITR_GREENTAB);
 tftL.setRotation(0);


 // RIGHT
 selectRight();
 tftR.initR(INITR_GREENTAB);
 tftR.setRotation(0);


 digitalWrite(CS_L, HIGH);
 digitalWrite(CS_R, HIGH);


 lcd.createChar(0,m0);
 lcd.createChar(1,m1);
 lcd.createChar(2,m2);
 lcd.createChar(3,m3);
 lcd.createChar(4,m4);




 lcd.setCursor(3,0);
  lcd.write(byte(4));
 lcd.setCursor(4,1);
 lcd.write(byte(3));
 lcd.write(byte(0));
 lcd.write(byte(0));
 lcd.write(byte(0));
 lcd.write(byte(0));
 lcd.write(byte(0));
 lcd.write(byte(0));
 lcd.write(byte(1));
 lcd.setCursor(12,0);
 lcd.write(byte(2));
 // lcd.setCursor(3, 0);
 // lcd.print("Inventory:");
 // lcd.setCursor(0, 1);
 // lcd.print(" Pills:");
 // lcd.print(num_pills);
 // lcd.print("  BAs:");
 // lcd.print(num_Aids);
 //drawMenu();


//TODO: initialize all your inputs and ouputs
//Serial.begin(9600);
//mySerial.begin(9600);
//Serial.begin(9600);
//serial_init(9600);
//TODO: Initialize the buzzer timer/pwm(timer0)
//TODO: Initialize the servo timer/pwm(timer1)
//TODO: Initialize tasks here
// e.g.
unsigned char i = 0;




tasks[i].state = EYE_OPEN;
tasks[i].period = 50;//fix
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_Eyes;
 i++;
tasks[i].state = menu_start;
tasks[i].period = 50;//fix
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_MENU;
i++;
tasks[i].state = P_WAIT;
tasks[i].period = 5;          // read ADC every 5ms
tasks[i].elapsedTime = tasks[i].period;;
tasks[i].TickFct = &PulseTick;
i++;
tasks[i].state = temp_Start;
tasks[i].period = 100;          // read ADC every 5ms
tasks[i].elapsedTime = tasks[i].period;;
tasks[i].TickFct = &tempTick;
/*
tasks[i].state = start_servo;
tasks[i].period = 50;//fix
tasks[i].elapsedTime = tasks[i].period;
tasks[i].TickFct = &TickFct_servo;
i++;
*/




// tasks[0].period = ;
// tasks[0].state = ;
// tasks[0].elapsedTime = ;
// tasks[0].TickFct = ;
TimerSet(GCD_PERIOD);
TimerOn();
while (1) {}
return 0;
}
