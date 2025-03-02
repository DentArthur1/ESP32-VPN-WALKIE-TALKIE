#ifndef LED_H   // Se LED_H non è definito
#define LED_H   // Definiscilo
#include <Arduino.h>

// Dichiarazioni e definizioni
#define RED_PIN 32
#define GREEN_PIN 33
#define BLUE_PIN 27

#define RECEIVE_LED_TIMEOUT 25 
#define CONNECTION_LED_TIMEOUT 7000
#define NO_CONNECTION_LED_BLINK_TIME 500

//variabili ereditate dal main per gestire
//il blink del led
static unsigned long transmit_on_time;
static bool transmit_led = false;

static unsigned long connection_led_time;
static bool connection_led = false;

static unsigned long no_connection_led_time;
static bool no_connection_led = false;
static unsigned long no_connection_led_off_time;

void setup_led();
void write_rgb(int red, int green, int blue);

#endif  // Fine della guardia