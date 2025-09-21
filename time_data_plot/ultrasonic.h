#ifndef ULTRASONIC_H
#define ULTRASONIC_H

// Ultrasonic header file

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

void setupUltrasonicSensor(int triggerPin = 1, int echoPin = 2, int timeoutMicroseconds = 11500, bool serialEnable = false);

unsigned long pollUltrasonicSensor();

#endif
