#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <stdio.h>
#include "stdlib.h"
#include "hardware/adc.h"


//Pin define
#define Jy 26
#define Jx 27
#define Jbutton 22

//CHANGE AS NEEDED
#define idleMax 32
#define idleMin 28
#define UpRightBarrier 27
#define DownLeftBarrier 33

enum JOYSTICK_ORIENT {UP, DOWN, LEFT, RIGHT, IDLE, ERR};
enum JOYSTICK_BUTTON {RELEASE, HOLD};

int buttonCondition = RELEASE;
int continuingX = IDLE;
int continuingY = IDLE;

// -------- LEFT TFT --------
#define CS_L   10
#define DC_L    9
#define RST_L   7

// -------- RIGHT TFT --------
#define CS_R    8
#define DC_R    9
#define RST_R   6

Adafruit_ST7735 tftL(CS_L, DC_L, RST_L);
Adafruit_ST7735 tftR(CS_R, DC_R, RST_R);

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

void setup() {

  SPI.begin();

  pinMode(CS_L, OUTPUT);
  pinMode(CS_R, OUTPUT);

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

  // draw open eyes once
  selectLeft();
  drawEye(tftL, 60);

  selectRight();
  drawEye(tftR, 60);
}

unsigned long timer = 0;

//KEY
//  -1 = error (1 or more axis)
//  0 = idle
//  1 = up
//  2 = up right
//  3 = right
//  4 = right down
//  5 = down
//  6 = down left
//  7 = left
//  8 = up left

int returnJoystickOrientation(uint16_t movementX, uint16_t movementY){ //returns joystick orientation in a numeric code
    switch(continuingX){
        case LEFT: //input 0
            if(movementX < idleMax && movementX > idleMin){ //halt
                continuingX = IDLE;
            }
            else if(movementX <= UpRightBarrier){ //Right
                continuingX = RIGHT;
            }
            break;

        case IDLE:
            if(movementX <= UpRightBarrier){ //Up + right
                continuingX = RIGHT;
            }
            else if(movementX >= DownLeftBarrier){ //down + left
                continuingX = LEFT;
            }
            break;

        case RIGHT: //input 0
            if(movementX < idleMax && movementX > idleMin){ //halt
                continuingX = IDLE;
            }
            else if(movementX >= DownLeftBarrier){ //Left
                continuingX = LEFT;
            }
            break;

        default:
            printf("AXIS X ERR\n");
            continuingX = ERR;
    }
 
    switch(continuingY){
        case UP: //input 1
            if(movementY < idleMax && movementY > idleMin){ //halt
                continuingY = IDLE;
            }
            else if(movementY >= DownLeftBarrier){ //down
                continuingY = DOWN;
            }
            break;

        case DOWN: //input 1
            if(movementY < idleMax && movementY > idleMin){ //halt
                continuingY = IDLE;
            }
            else if(movementY <= UpRightBarrier){ //up
                continuingY = UP;
            }
            break;

        case IDLE:
            if(movementY <= UpRightBarrier){ //Up + right
                continuingY = UP;
            }
            else if(movementY >= DownLeftBarrier){ //down + left
                continuingY = DOWN;
            }
            break;

        default:
            printf("AXIS Y ERR\n");
            continuingY = ERR;
    }

    if(continuingY == UP){ //up
        if(continuingX == LEFT){ //left
            return 8;//UP LEFT
        }
        else if(continuingX == IDLE){ //idle
            return 1;//UP
        }
        else if(continuingX == RIGHT){ //right
            return 2; //UP RIGHT
        }
    }
    else if(continuingY == IDLE){ //idle
        if(continuingX == LEFT){ //left
            return 7;//LEFT
        }
        else if(continuingX == IDLE){ //idle
            return 0; //full idle
        }
        else if(continuingX == RIGHT){ //right
            return 3;//RIGHT
        }
    }
    else if(continuingY == DOWN){ //down
        if(continuingX == LEFT){ //left
            return 6;//DOWN LEFT
        }
        else if(continuingX == IDLE){ //idle
            return 5;//DOWN
        }
        else if(continuingX == RIGHT){ //right
            return 4;//DOWN RIGHT
        }
    }

    return -1;
}

void joystickPress(){
    switch(buttonCondition){
        case HOLD:
            printf("HOLD\n");

            if(gpio_get(Jbutton) == 1){ //add things button should do here
                printf("BUTTON PRESSED\n");
                buttonCondition = RELEASE;
            }
            break;
        case RELEASE:
            printf("BUTTON IDLE\n");
            if(gpio_get(Jbutton) == 0){
                buttonCondition = HOLD;
            }
            break;
        default:
            printf("\n--------------------------------JOYSTICK PRESS ERR--------------------------------\n");
            break;
    }
}

int main(){ //pico has its own ADC readers but they are somewhat off without a voltage ref
    //stdio_init_all(); //initializes pico
    adc_init(); //initializes analog to digital converters on pico
    

    //change pin from gpio to adc (only for pins 26 27 28)
    adc_gpio_init(Jx);
    adc_gpio_init(Jy);
    _gpio_init(Jbutton);

    gpio_set_dir(Jbutton, GPIO_IN);
    gpio_pull_up(Jbutton);

    while(true){
        //GATHER X VALUES
        adc_select_input(0);
        uint16_t xVal = adc_read();

        xVal += adc_read();
        xVal += adc_read();
        xVal += adc_read();
        xVal += adc_read();
        xVal += adc_read();
        xVal += adc_read();
        xVal += adc_read();
        xVal = xVal/7; //due to error we will take an avg of vals before sm

        //GATHER Y VALUES
        adc_select_input(1);
        uint16_t yVal = adc_read();

        yVal += adc_read();
        yVal += adc_read();
        yVal += adc_read();
        yVal += adc_read();
        yVal += adc_read();
        yVal += adc_read();
        yVal += adc_read();
        yVal = yVal/7; //due to error we will take an avg of vals before sm

        //printf("X: %d, Y: %d\n", xVal, yVal);

        int joystickOrientationCode = returnJoystickOrientation(xVal, yVal); //get orientation code
        char* orientation;

        if(joystickOrientationCode == -1){
            orientation = "ER";
        }
        else if(joystickOrientationCode == 0){
            orientation = "ID";
        }
        else if(joystickOrientationCode == 1){
            orientation = "UP";
        }
        else if(joystickOrientationCode == 2){
            orientation = "UR";
        }
        else if(joystickOrientationCode == 3){
            orientation = "RT";
        }
        else if(joystickOrientationCode == 4){
            orientation = "RD";
        }
        else if(joystickOrientationCode == 5){
            orientation = "DN";
        }
        else if(joystickOrientationCode == 6){
            orientation = "DL";
        }
        else if(joystickOrientationCode == 7){
            orientation = "LT";
        }
        else{
            orientation = "UL";
        }

        //printf("Orientation Code: %d, Orientation: %c%c\n", joystickOrientationCode, *orientation, *(orientation + 1)); //print code and orientation (see key on top of file)

        joystickPress();

        sleep_ms(500);
    }

    
    return 0;
}

// void loop() {

//   if (millis() - timer > 3000) {

//     // close
//     selectLeft();
//     drawEye(tftL, 6);

//     selectRight();
//     drawEye(tftR, 6);

//     delay(120);

//     // open
//     selectLeft();
//     drawEye(tftL, 60);

//     selectRight();
//     drawEye(tftR, 60);

//     timer = millis();
//   }
// }
