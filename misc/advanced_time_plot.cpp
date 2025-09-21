// Time Plotter with Circular Buffer
// Author: Allan Wu (23810308)
// Date: 21 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ultrasonic.h"

// Set update speed (don't set too fast or the TTGO will overheat)
int delayMillis = 100;

// Initial estimate of maximum analogue reading
int maxY = 100;
int oldMaxY = 0;
int funcAmplitude = 1; // Vertical bound for symmetrical functions e.g. sin(x)
bool autoRanging = true;
bool disableScrolling = true;

const int AUTO_RANGE_STEP = 20;

// Define spacing between data points
const int X_STEP = 3;

bool enableUserSelect = true;
enum function {
  ANALOG_READ,
  PURE_SINUSOID,
  COSINE_SINE_SUM,
  FREQUENCY_MODULATION,
  AMPLITUDE_MODULATION,
  ULTRASONIC_SENSOR
};
function functionSelect = ANALOG_READ;
char functionName[50];

// For the ANALOG_READ option, read analog values from this pin:
#define ANALOG_INPUT_PIN 3
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14
int prevLeft = 0, prevRight = 0, currLeft, currRight;

// Configure plot colours
bool enableGridlines = true;
#define BACKGROUND_COLOUR TFT_BLACK
#define GRIDLINES_COLOUR 0x2965
#define AXIS_COLOUR TFT_SILVER
#define DATA_COLOUR TFT_GOLD

#define X_DATUM 35
#define Y_DATUM 20
#define X_LENGTH 270
#define Y_HEIGHT 140

const int X_TICK_SIZE = 15;
const int Y_TICK_SIZE = 14;
const int NUM_X_TICKS = (X_LENGTH / X_TICK_SIZE) + 1;
const int NUM_Y_TICKS = (Y_HEIGHT / Y_TICK_SIZE) + 1;
const int NUM_DATA_POINTS = (X_LENGTH / X_STEP) + 1;

int buffer[NUM_DATA_POINTS];

void userSelectFunction();

int getDataPoint(int sampleIndex, float alpha = 0.023333, float omega = 0.157059);

void drawGrid(bool bufferFull = false, int start = 0, int end = 0);

TFT_eSPI tft = TFT_eSPI(); // 320 x 170

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND_COLOUR);
  setupUltrasonicSensor();
  if (enableUserSelect) userSelectFunction();
  getDataPoint(0); // Sets function name
  tft.setTextDatum(CC_DATUM);
  tft.drawString(functionName, X_DATUM + (X_LENGTH / 2), 10);
  tft.setTextDatum(TR_DATUM);
  drawGrid();
  if (!autoRanging) { // Override labels for math functions
    tft.drawNumber(+funcAmplitude, X_DATUM-8, Y_DATUM-3);
    tft.drawNumber(0, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT/2);
    tft.drawNumber(-funcAmplitude, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT);
  }
}

void loop() {
  static unsigned long lastUpdateTime = millis();
  static unsigned long sampleIndex = 0;
  unsigned long firstSample = sampleIndex - NUM_DATA_POINTS + 1;
  int startIndex = firstSample % NUM_DATA_POINTS;
  int writeIndex = sampleIndex % (NUM_DATA_POINTS); // end-point inclusive
  int rawData = getDataPoint(sampleIndex);
  buffer[writeIndex] = rawData; // store in buffer, overwrites old value

  static int runningMax = 0, runningSum = 0;
  static int prevX = -1, prevY = -1;
  runningSum += rawData;
  if (rawData > runningMax) runningMax = rawData;

  currLeft = !digitalRead(LEFT_BUTTON);
  currRight = !digitalRead(RIGHT_BUTTON);

  if (prevLeft && !currLeft) { // Clear background and turn off scrolling (TODO)
    drawGrid();
    disableScrolling = true;
  }
  if (prevRight && !currRight) { // Turn on scrolling (this works already)
    disableScrolling = false;
  }

  if (sampleIndex > 0 && writeIndex == 0) {
    if (autoRanging) { // Auto-ranging: take double the average or 1.5x the running maximum 
      int doubleAverage = constrain(2 * (runningSum / NUM_DATA_POINTS), 0, 4095) / AUTO_RANGE_STEP * AUTO_RANGE_STEP;
      maxY = constrain((3 * runningMax / 2), 0, 4095) / AUTO_RANGE_STEP * AUTO_RANGE_STEP;
      if (doubleAverage > maxY) maxY = doubleAverage;
    }
    if (disableScrolling) {
      drawGrid();
    }
    runningMax = 0;
    runningSum = 0;
    prevX = -1;
    prevY = -1;
  }
  int normalisedY = constrain(map(rawData, 0, maxY, 0, Y_HEIGHT), 0, Y_HEIGHT);
  int currX = X_DATUM + writeIndex * X_STEP;
  int currY = Y_DATUM + (Y_HEIGHT - normalisedY);

  // Plot data
  if (disableScrolling || sampleIndex < NUM_DATA_POINTS) { // If buffer is not full
    if (prevX != -1 && prevY != -1)
      tft.drawLine(prevX, prevY, currX, currY, DATA_COLOUR);
    else
      tft.drawPixel(currX, currY, DATA_COLOUR);
  } else {
    drawGrid(true, startIndex, writeIndex);
  }

  prevX = currX;
  prevY = currY;
  prevLeft = currLeft;
  prevRight = currRight;
  sampleIndex++;

  int actualDelay = delayMillis - (millis()-lastUpdateTime);
  if (actualDelay > 0) delay(actualDelay);
  lastUpdateTime = millis();
}

void userSelectFunction() {
  int prevChoice = -1;
  int currChoice = 0;
  tft.setTextFont(2);
  tft.drawString("SELECT FUNCTION", X_DATUM, Y_DATUM-5);
  while (true) {
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);
    if (prevRight && !currRight) currChoice = (currChoice + 1) % 5;
    if (prevChoice != currChoice) {
      tft.setTextColor(GRIDLINES_COLOUR, BACKGROUND_COLOUR);
      tft.drawString("0. ANALOG READ", X_DATUM, Y_DATUM+20);
      tft.drawString("1. PURE SINE WAVE", X_DATUM, Y_DATUM+40);
      tft.drawString("2. COSINE + SINE", X_DATUM, Y_DATUM+60);
      tft.drawString("3. FREQUENCY MODULATED WAVE", X_DATUM, Y_DATUM+80);
      tft.drawString("4. AMPLITUDE MODULATED WAVE", X_DATUM, Y_DATUM+100);
      tft.drawString("5. ULTRASONIC SENSOR", X_DATUM, Y_DATUM+120);
    }
    tft.setTextColor(DATA_COLOUR, BACKGROUND_COLOUR);
    switch (currChoice) {
    case (0):
      functionSelect = ANALOG_READ;
      tft.drawString("0. ANALOG READ", X_DATUM, Y_DATUM+20);
      break;
    case (1):
      functionSelect = PURE_SINUSOID;
      tft.drawString("1. PURE SINE WAVE", X_DATUM, Y_DATUM+40);
      break;
    case (2):
      functionSelect = COSINE_SINE_SUM;
      tft.drawString("2. COSINE + SINE", X_DATUM, Y_DATUM+60);
      break;
    case (3):
      functionSelect = FREQUENCY_MODULATION;
      tft.drawString("3. FREQUENCY MODULATED WAVE", X_DATUM, Y_DATUM+80);
      break;
    case (4):
      functionSelect = AMPLITUDE_MODULATION;
      tft.drawString("4. AMPLITUDE MODULATED WAVE", X_DATUM, Y_DATUM+100);
      break;
    case (5):
      functionSelect = ULTRASONIC_SENSOR;
      tft.drawString("5. ULTRASONIC SENSOR", X_DATUM, Y_DATUM+120);
      break;
    }
    tft.setTextColor(AXIS_COLOUR, BACKGROUND_COLOUR);
    prevChoice = currChoice;
    if (prevLeft && !currLeft) {
      if (functionSelect != ANALOG_READ && functionSelect != ULTRASONIC_SENSOR)
        autoRanging = false;
      if (functionSelect == COSINE_SINE_SUM) {
        funcAmplitude = 2;
      }
      break;
    }
    prevLeft = currLeft;
    prevRight = currRight;
  }
  tft.fillScreen(BACKGROUND_COLOUR);
  tft.setTextFont(1);
}

int getDataPoint(int sampleIndex, float alpha, float omega) {
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
    sprintf(functionName, "cos(%.2ft+%.1fsin(%.2ft))", omegaAct, 10*omegaAct, 16*alphaAct);
    return maxY/2 * (1 + cos(sampleIndex * omega + 10 * omega * sin(sampleIndex * 16*alpha)));
  case (AMPLITUDE_MODULATION):
    sprintf(functionName, "cos(%.2ft)sin(%.2ft)", alphaAct, 4*omegaAct);
    return maxY/2 * (1 + cos(sampleIndex * alpha) * sin(sampleIndex * 4*omega));
  case (ULTRASONIC_SENSOR):
    sprintf(functionName, "Distance to Object (cm)");
    pollUltrasonicSensor();
    return ultrasonicDistanceNearestCm;
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
    tft.fillRect(0, 0, X_DATUM-8, 170, BACKGROUND_COLOUR);
    tft.drawNumber(maxY, X_DATUM-8, Y_DATUM-3); // labels with padding (right indent)
    tft.drawNumber(maxY/2, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT/2);
    tft.drawNumber(0, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT);
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
      // Serial.printf("PREV: (%3d,%3d), CURR: (%3d, %3d)\n", prevX, prevY, currX, currY);
      prevX = currX;
      prevY = currY;
      prevIndex = i;
    }
  }
  Serial.println(millis());
}
