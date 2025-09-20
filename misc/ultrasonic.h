#ifndef ULTRASONIC_H
#define ULTRASONIC_H

// Ultrasonic header file

// Global variable that can be referenced elsewhere, note that a negative value indicates no enemy found
extern int ultrasonicDistanceNearestCm;

// Set ultrasonicSerialEnable to 1 to enable printout to the serial monitor
extern int ultrasonicSerialEnable;

void setupUltrasonicSensor(bool outputEnable = false);

unsigned long pollUltrasonicSensor();

#endif