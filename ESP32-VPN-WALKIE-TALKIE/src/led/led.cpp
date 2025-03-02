#include "led.h"


void write_rgb(int red, int green, int blue){
    //Serial.printf("\nScrivo il colore (%d, %d, %d)",red,green,blue);
    analogWrite(RED_PIN, red);
    analogWrite(GREEN_PIN, green);
    analogWrite(BLUE_PIN, blue);
}

void setup_led(){
    pinMode(RED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    write_rgb(255,255,0);
    Serial.println("(LED) Setup led OK.");
}

