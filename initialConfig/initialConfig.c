#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define LEDPIN 15
#define BUTTONPIN 14

enum LIGHTS {ON, OFF, HOLD};

int state = OFF;
bool on = false;

void lightUp(){ //turns on and off the light when button is pressed
    switch(state){
        case OFF:
            if(!gpio_get(BUTTONPIN)){
                state = HOLD;
                break;
            }
            break;
        case ON:
            if(!gpio_get(BUTTONPIN)){
                state = HOLD;
            }
            break;
        case HOLD:
            if(gpio_get(BUTTONPIN)){
                if(!on){
                    printf("LIGHT ON\n");
                    on = true;
                    state = ON;
                    gpio_put(LEDPIN, 1);
                }
                else{
                    printf("LIGHT OFF\n");
                    on = false;
                    state = OFF;
                    gpio_put(LEDPIN, 0);
                }
            }
            
            break;

        default:
            printf("-----------UNASSIGNED STATE----------\n");
            break;
    }
}

int main()
{
    stdio_init_all(); //initialize pico
    
    gpio_init(LEDPIN); //initalize gpio pin
    gpio_init(BUTTONPIN); //initalize gpio pin

    gpio_set_dir(LEDPIN, GPIO_OUT); //Set pin as output
    gpio_set_dir(BUTTONPIN, GPIO_IN); //Set pin as input

    gpio_pull_up(BUTTONPIN); //set button pin to pull up 

    while(true){

        lightUp(); //call light up sm
    }

    return 0;
}

/*
sio_hw->gpio_in & (1u << gpio);
*/