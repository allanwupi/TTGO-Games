// Time Data Plot with Circular Buffer
// Author: Allan Wu (23810308)
// Date: 21 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ultrasonic.h"

// Set update speed (don't set too fast or the TTGO will overheat)
int delayMillis = 100;
// Vertical bound for symmetrical functions e.g. sin(x)
int funcAmplitude = 1; 
bool autoRanging = true;
bool enableScrolling = true;
bool enableUserSelect = true;
bool enableGridlines = false;

int AUTO_STEP = 20;
int X_STEP = 4;
int MIN_Y_RANGE = 40;

const float ALPHA = 0.0421489;
const float OMEGA = 0.1570796;

int maxY = MIN_Y_RANGE;
int oldMaxY = 0;

#define ANALOG_INPUT_PIN 10
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

// Configure plot colours
uint16_t BACKGROUND_COLOUR = TFT_BLACK;
uint16_t GRIDLINES_COLOUR = 0x2965;
uint16_t AXIS_COLOUR = TFT_SILVER;
uint16_t DATA_COLOUR = TFT_GOLD;

#define X_DATUM 32
#define Y_DATUM 20
#define X_LENGTH 280
#define Y_HEIGHT 140
int X_TICK_SIZE = 14;
int Y_TICK_SIZE = 14;
int NUM_X_TICKS = (X_LENGTH / X_TICK_SIZE) + 1;
int NUM_Y_TICKS = (Y_HEIGHT / Y_TICK_SIZE) + 1;
int NUM_DATA_POINTS = (X_LENGTH / X_STEP) + 1;

#define MAX_BUFFER_CAPACITY 320
#define CHAR_BUFFER_SIZE 50
enum function {
  ANALOG_READ,
  PURE_SINUSOID,
  COSINE_SINE_SUM,
  FREQUENCY_MODULATION,
  AMPLITUDE_MODULATION,
  CUSTOM_FUNCTION
};
char functionNames[6][CHAR_BUFFER_SIZE] = {
  "0. ANALOG READ",
  "1. SINE FUNCTION",
  "2. SUM OF SINUSOIDS",
  "3. FREQUENCY MODULATED WAVE",
  "4. AMPLITUDE MODULATED WAVE",
  "5. USER DEFINED FUNCTION"
};
function functionSelect = ANALOG_READ;
int buffer[MAX_BUFFER_CAPACITY];
char functionName[CHAR_BUFFER_SIZE];
int prevLeft = 0, prevRight = 0, currLeft, currRight;
TFT_eSPI tft = TFT_eSPI(); // (320 x 170)

void userSelectFunction();

int getCustomData(int sampleIndex);

int getDataPoint(int sampleIndex, float alpha = ALPHA, float omega = OMEGA, int (*op)(int)=getCustomData);

void drawGrid(bool bufferFull = false, int start = 0, int end = 0);

char customFunctionName[CHAR_BUFFER_SIZE] = "Distance to object (cm)";

int getCustomData(int sampleIndex) {
  // return (2 * sampleIndex) % 50;
  static bool sensorSetupDone = false;
  if (!sensorSetupDone) {
    setupUltrasonicSensor();
    sensorSetupDone = true;
  }
  pollUltrasonicSensor();
  return ultrasonicDistanceNearestCm;
}

void setup() {
  tft.init();
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND_COLOUR);
  if (enableUserSelect) userSelectFunction();
  getDataPoint(0); // Sets function name
  tft.setTextDatum(CC_DATUM);
  tft.drawString(functionName, X_DATUM + (X_LENGTH / 2), 10);
  tft.setTextDatum(TR_DATUM);
  drawGrid();
  if (!autoRanging) { // Override labels for math functions
    tft.drawNumber(+funcAmplitude, X_DATUM-5, Y_DATUM-3);
    tft.drawNumber(0, X_DATUM-5, Y_DATUM-3 + Y_HEIGHT/2);
    tft.drawNumber(-funcAmplitude, X_DATUM-5, Y_DATUM-3 + Y_HEIGHT);
  }
}

void loop() {
  static unsigned long lastUpdateTime = millis();
  static unsigned long sampleIndex = 0;
  static unsigned long startSampleIndex = sampleIndex - NUM_DATA_POINTS + 1;
  static int prevX = -1, prevY = -1;
  static int runningMax = 0, runningSum = 0;
  if (millis() - lastUpdateTime >= delayMillis) {
    int rawData = getDataPoint(sampleIndex);
    if (rawData > maxY) { // Increase maxY if necessary
      maxY = (rawData / AUTO_STEP + 1)* AUTO_STEP;
      drawGrid();
    }
    int startIndex = startSampleIndex % NUM_DATA_POINTS;
    int writeIndex = sampleIndex % NUM_DATA_POINTS;
    buffer[writeIndex] = rawData; // Store raw value in buffer, overwrites old value
    runningSum += rawData;
    if (rawData > runningMax) runningMax = rawData;
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);
    if (prevLeft && !currLeft) { // Press left button to turn off scrolling and refresh grid
      enableGridlines = true;
      drawGrid();
      enableScrolling = false;
    } else if (prevRight && !currRight) { // Press right button to enable scrolling
      enableGridlines = false;
      enableScrolling = true;
    }
    if (sampleIndex > 0 && writeIndex == 0) { // Refresh grid when looping back to start of buffer
      if (autoRanging) { // Auto-ranging: take double the average or 1.5x the running maximum 
        int doubleAverage = (2*(runningSum / NUM_DATA_POINTS) / AUTO_STEP + 1) * AUTO_STEP;
        maxY = (3 * runningMax / 2) / AUTO_STEP * AUTO_STEP;
        if (doubleAverage > maxY) maxY = doubleAverage;
        maxY = constrain(maxY, MIN_Y_RANGE, 4100);
      }
      if (!enableScrolling) drawGrid();
      runningMax = 0;
      runningSum = 0;
      prevX = -1;
      prevY = -1;
    }
    int normalisedY = constrain(map(rawData, 0, maxY, 0, Y_HEIGHT), 0, Y_HEIGHT);
    int currX = X_DATUM + writeIndex * X_STEP;
    int currY = Y_DATUM + (Y_HEIGHT - normalisedY);
    // Plot data point or line
    if (!enableScrolling || sampleIndex < NUM_DATA_POINTS) { // If buffer is not full
      if (prevX > 0 && prevY > 0) {
        tft.drawLine(prevX, prevY, currX, currY, DATA_COLOUR);
      } else {
        tft.drawPixel(currX, currY, DATA_COLOUR);
      }
    } else {
      drawGrid(true, startIndex, writeIndex); // Note that this is end-point inclusive
    }
    prevX = currX;
    prevY = currY;
    prevLeft = currLeft;
    prevRight = currRight;
    sampleIndex++;
    startSampleIndex++;
    lastUpdateTime = millis();
  }
}

void userSelectFunction() {
  int prevChoice = -1, currChoice = 0;
  bool startPlotting = false;
  tft.setTextFont(2);
  tft.drawString("[ SELECT FUNCTION TO PLOT ]", X_DATUM, Y_DATUM-5);
  while (!startPlotting) {
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);
    if (prevRight && !currRight) currChoice = (currChoice + 1) % 6;
    if (prevChoice != currChoice) {
      tft.setTextColor(GRIDLINES_COLOUR, BACKGROUND_COLOUR);
      for (int i = 0; i < 6; i++) {
        tft.drawString(functionNames[i], X_DATUM, Y_DATUM+20*(i+1));
      }
      tft.setTextColor(DATA_COLOUR, BACKGROUND_COLOUR);
      tft.drawString(functionNames[currChoice], X_DATUM, Y_DATUM+20*(currChoice+1));
      switch (currChoice) {
        case (0):
          functionSelect = ANALOG_READ; break;
        case (1):
          functionSelect = PURE_SINUSOID; break;
        case (2):
          functionSelect = COSINE_SINE_SUM; break;
        case (3):
          functionSelect = FREQUENCY_MODULATION; break;
        case (4):
          functionSelect = AMPLITUDE_MODULATION; break;
        case (5):
          functionSelect = CUSTOM_FUNCTION; break;
      }
    }
    tft.setTextColor(AXIS_COLOUR, BACKGROUND_COLOUR);
    prevChoice = currChoice;
    if (prevLeft && !currLeft) {
      if (functionSelect != ANALOG_READ && functionSelect != CUSTOM_FUNCTION)
        autoRanging = false;
      if (functionSelect == COSINE_SINE_SUM)
        funcAmplitude = 2;
      startPlotting = true;
    }
    prevLeft = currLeft;
    prevRight = currRight;
  }
  tft.fillScreen(BACKGROUND_COLOUR);
  tft.setTextFont(1);
}

int getDataPoint(int sampleIndex, float alpha, float omega, int (*op)(int)) {
  float omegaAct = 1000 * omega / delayMillis;
  float alphaAct = 1000 * alpha / delayMillis;
  switch (functionSelect) {
  case (ANALOG_READ):
    sprintf(functionName, "Reading Analog Pin %d", ANALOG_INPUT_PIN);
    return analogRead(ANALOG_INPUT_PIN);
  case (PURE_SINUSOID):
    sprintf(functionName, "sin(%.4ft)", omegaAct);
    return maxY/2 * (1 + sin(sampleIndex * omega));
  case (COSINE_SINE_SUM):
    sprintf(functionName, "cos(%.2ft)+sin(%.2ft)", alphaAct, 2*omegaAct);
    return maxY/4 * (2 + cos(sampleIndex * alpha) + sin(sampleIndex * 2*omega));
  case (FREQUENCY_MODULATION):
    sprintf(functionName, "cos(%.2ft+%.1fsin(%.2ft))", omegaAct, 10*omegaAct, 10*alphaAct);
    return maxY/2 * (1 + cos(sampleIndex * omega + 10 * omega * sin(sampleIndex * 10*alpha)));
  case (AMPLITUDE_MODULATION):
    sprintf(functionName, "cos(%.2ft)sin(%.2ft)", 1.5*alphaAct, 5*omegaAct);
    return maxY/2 * (1 + cos(sampleIndex * 1.5*alpha) * sin(sampleIndex * 5*omega));
  case (CUSTOM_FUNCTION):
    sprintf(functionName, customFunctionName);
    return op(sampleIndex);
  default:
    return 0;
  }
}

void drawGrid(bool bufferFull, int start, int end) {
  // Refresh grid
  tft.fillRect(X_DATUM-1, Y_DATUM-1, X_LENGTH+2, Y_HEIGHT+2, BACKGROUND_COLOUR);
  if (enableGridlines) {
    for (int i = 1; i < NUM_X_TICKS-1; i++)
      tft.drawFastVLine(X_DATUM + i*X_TICK_SIZE, Y_DATUM, Y_HEIGHT, GRIDLINES_COLOUR);
    for (int i = 1; i < NUM_Y_TICKS-1; i++)
      tft.drawFastHLine(X_DATUM, Y_DATUM + i*Y_TICK_SIZE, X_LENGTH, GRIDLINES_COLOUR);
  }
  tft.drawRect(X_DATUM-2, Y_DATUM-2, X_LENGTH+4, Y_HEIGHT+4, AXIS_COLOUR); // with padding
  // Update y-range if auto ranging enabled
  if (autoRanging && oldMaxY != maxY) {
    tft.fillRect(0, 0, X_DATUM-5, 170, BACKGROUND_COLOUR);
    tft.drawNumber(maxY, X_DATUM-5, Y_DATUM-3); // labels with padding (right indent)
    tft.drawNumber(maxY/2, X_DATUM-5, Y_DATUM-3 + Y_HEIGHT/2);
    tft.drawNumber(0, X_DATUM-5, Y_DATUM-3 + Y_HEIGHT);
    oldMaxY = maxY;
  }
  // Draw plot from buffer
  if (bufferFull) {
    int prevIndex = start;
    int prevX = X_DATUM;
    int normalisedPrevY = constrain(map(buffer[prevIndex], 0, maxY, 0, Y_HEIGHT), 0, Y_HEIGHT);
    int prevY = Y_DATUM + (Y_HEIGHT - normalisedPrevY);
    int x = 1;
    for (int i = (start+1) % NUM_DATA_POINTS; prevIndex != end; i = (i+1) % NUM_DATA_POINTS, x++) {
      int normalisedY = constrain(map(buffer[i], 0, maxY, 0, Y_HEIGHT), 0, Y_HEIGHT);
      int currX = X_DATUM + x * X_STEP;
      int currY = Y_DATUM + (Y_HEIGHT - normalisedY);
      tft.drawLine(prevX, prevY, currX, currY, DATA_COLOUR);
      prevX = currX;
      prevY = currY;
      prevIndex = i;
    }
  }
}
