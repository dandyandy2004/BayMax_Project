#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/uart.h" 
#include "hardware/pwm.h"

//Source for "changeNote()" function coding help: https://forums.raspberrypi.com/viewtopic.php?f=145&t=307912#p1844432 

//List of usable notes and frequencies-> Source: https://github.com/robsoncouto/arduino-songs/blob/master/happybirthday/happybirthday.ino
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0


//Pin define
#define Jy 26
#define Jx 27
#define Jbutton 22
#define TX 0 
#define RX 1 
#define buzzerPin 15

//CHANGE AS NEEDED
#define idleMax 32
#define idleMin 28
#define UpRightBarrier 27
#define DownLeftBarrier 33
#define baudRate 9600 
#define MAX_VOLUME 65535

enum JOYSTICK_ORIENT {UP, DOWN, LEFT, RIGHT, IDLE, ERR};
enum JOYSTICK_BUTTON {RELEASE, HOLD};
enum buzzerOptions{ON, OFF};

int buzzerCondition = OFF;
int buttonCondition = RELEASE;
int continuingX = IDLE;
int continuingY = IDLE;
bool trigger = false;

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


void buzzerOn(uint pwmSlice){ //turns on the buzzer
    pwm_set_gpio_level(buzzerPin, 50); //set volume
    pwm_set_enabled(pwmSlice, true); //for enabling and disabling the pwm signal for the buzzer
}

void buzzerOff(uint pwmSlice){ //turns off the buzzer
    pwm_set_gpio_level(buzzerPin, 0); //set sound to low
    pwm_set_enabled(pwmSlice, false); //disable pwm after completion
}


//changes the note being played
//NOTE: REQUIRES THAT BUZZER IS TURNED BACK ON AFTER THE NOTE IS CHANGED
void changeNote(uint pwmSlice, pwm_config config, int note){
    float divisor = SYS_CLK_HZ / (note*10000);
    pwm_config_set_clkdiv(&config, divisor);
    pwm_init(pwmSlice, &config, false); //initializes buzzer as off again
    pwm_set_wrap(pwmSlice, 10000);
}

void beepBeep(uint pwmSlice, pwm_config config){ //put buzzer response here
    changeNote(pwmSlice, config, NOTE_E6); //changes note to E6
    buzzerOn(pwmSlice); 
    sleep_ms(400); //pause to let note play

    changeNote(pwmSlice, config, NOTE_F1); //Switches note to F1
    buzzerOn(pwmSlice); //Turns buzzer back on after note change
    sleep_ms(500);

    buzzerOff(pwmSlice); //turns off buzzer after completion
    return;
}

void buzzerResponse(bool trigger, uint pwmSlice, pwm_config config){ //trigger = true whenever buzzer needs to go off
    switch(buzzerCondition){
        case ON: //runs response once and instancely turns off
            beepBeep(pwmSlice, config);
            buzzerCondition = OFF;
            break;

        case OFF:
            if(trigger){
                printf("\n-------BUZZER ON-------\n");
                buzzerCondition = ON;
            }

            break;

        default:
            printf("\n-------BUZZER ERR-------\n");
            break;
    }

}

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
                uart_puts(uart0, "m");
                printf("BUTTON PRESSED\n");
                trigger = true;
                buttonCondition = RELEASE;
            }
            break;
        case RELEASE:
            printf("BUTTON IDLE\n");
            trigger = false;
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
    stdio_init_all(); //initializes pico
    adc_init(); //initializes analog to digital converters on pico
    
    
    gpio_init(buzzerPin); //initialize buzzer pin
    gpio_pull_up(buzzerPin); //no resistor needed

    pwm_config config = pwm_get_default_config(); //use default pwm config
    gpio_set_function(buzzerPin, GPIO_FUNC_PWM); //set pin to function as a PWM for buzzer

    uint pwmSlice = pwm_gpio_to_slice_num(buzzerPin); //get the right pwm slice to mess with


    //change pin from gpio to adc (only for pins 26 27 28)
    adc_gpio_init(Jx);
    adc_gpio_init(Jy);
    gpio_init(Jbutton);    
    
    // Do this before calling uart_init to avoid losing data 
    gpio_set_function(GPIO_OUT, UART_FUNCSEL_NUM(uart0, TX)); 
    gpio_set_function(GPIO_IN, UART_FUNCSEL_NUM(uart0, RX)); 
    
    uart_init(uart0, baudRate); 
    uart_set_fifo_enabled(uart0, true); 

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

        printf("X: %d, Y: %d\n", xVal, yVal);

        int joystickOrientationCode = returnJoystickOrientation(xVal, yVal); //get orientation code
        char* orientation;

        if(joystickOrientationCode == -1){
            orientation = "ER";
            uart_puts(uart0, "i");
        }
        else if(joystickOrientationCode == 0){
            orientation = "ID";
            uart_puts(uart0, "i");
        }
        else if(joystickOrientationCode == 1){
            orientation = "UP";
            uart_puts(uart0, "u");
        }
        else if(joystickOrientationCode == 2){
            orientation = "UR";
            uart_puts(uart0, "u");
        }
        else if(joystickOrientationCode == 3){
            orientation = "RT";
        }
        else if(joystickOrientationCode == 4){
            orientation = "RD";
            uart_puts(uart0, "d");
        }
        else if(joystickOrientationCode == 5){
            orientation = "DN";
            uart_puts(uart0, "d");
        }
        else if(joystickOrientationCode == 6){
            orientation = "DL";
            uart_puts(uart0, "d");
        }
        else if(joystickOrientationCode == 7){
            orientation = "LT";
        }
        else{
            orientation = "UL";
            uart_puts(uart0, "u");
        }

        printf("Orientation Code: %d, Orientation: %c%c\n", joystickOrientationCode, *orientation, *(orientation + 1)); //print code and orientation (see key on top of file)

        joystickPress();
        buzzerResponse(trigger, pwmSlice, config);

        sleep_ms(1000);


    }



    return 0;
}