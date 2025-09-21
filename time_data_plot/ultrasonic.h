// Ultrasonic header file
// Allan Wu (23810308)
// Updated 21 September 2025
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

extern int ULTRASONIC_TRIGGER;
extern int ULTRASONIC_ECHO;

// Global data variables that can be used in other files
// Note that sentinel value -1 means "NO ENEMY FOUND"
extern double ultrasonicDistanceCm;
extern int ultrasonicDistanceNearestCm;

// Set ultrasonicSerialEnable to 1 to enable printout to the serial monitor
extern bool ultrasonicSerialEnable;

// Set the maximum time we will wait for the echo pulse: this determines what is "OUT OF RANGE" for the sensor!
// From my testing, 15000 us timeout limits the distane range to ~200 cm
extern int ultrasonicTimeoutMicroseconds;

// Time to wait before running the loop again, note that maximum polling rate is 50 Hz => 20,000 us delay between reads
// (currently not used)
extern int pollDelayMicroseconds;

// Must run this before polling the ultrasonic sensor, can use different pins and set a longer timeout if needed
void setupUltrasonicSensor(int triggerPin = 1, int echoPin = 2, int timeoutMicroseconds = 15000, bool serialEnable = false);

// Currently this returns the elapsed time in microseconds and updates global variables (implementation can be changed)
unsigned long pollUltrasonicSensor();

#endif
