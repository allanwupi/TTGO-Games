#include<Arduino.h>
//#include<TFT_eSPI.h>

#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

// LEDC

// setting PWM properties
const int PWM_FREQ = 1000; // what is the maximum????
const int LED_CHANNEL = 0;
const int PWM_RESOLUTION = 8; // 0 - 255
const int LED_PIN = 1;
const int MAX_DUTY_CYCLE = 30;
const int DELAY_MILLIS = 100;

void setup() {
    pinMode(LEFT_BUTTON, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON, INPUT_PULLUP);
    //pinMode(LED_PIN, OUTPUT);
    ledcSetup(LED_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, LED_CHANNEL);
    Serial.begin(9600); //
}

void loop() {   
    // increase the LED brightness
    static int dutyCycle;
    for(dutyCycle = 0; dutyCycle <= MAX_DUTY_CYCLE; dutyCycle++) {
        ledcWrite(LED_CHANNEL, dutyCycle);
        Serial.println(dutyCycle);
        delay(DELAY_MILLIS);
    }
    //Serial.println("max brightness");
    // decrease the LED brightness
    for(dutyCycle = MAX_DUTY_CYCLE; dutyCycle > 0; dutyCycle--) {
        ledcWrite(LED_CHANNEL, dutyCycle);
        Serial.println(dutyCycle);
        delay(DELAY_MILLIS);
    }
    delay(DELAY_MILLIS);
}
