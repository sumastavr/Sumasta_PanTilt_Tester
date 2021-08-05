#include <Arduino.h>

// Function prototype
void initServo();
int getUsServo(boolean axis, int targetAngle);
int getServoDelay();
void moveFast(boolean axis, int value);

// SW Version
const String SWVersion = "V1.50 (05-08-21)";