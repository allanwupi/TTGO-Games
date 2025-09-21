// Time-Data Plotter (Circular Buffer)
// Author: Allan Wu (23810308)
// Date: 21 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "ultrasonic.h"

// Set update speed (don't set too fast or the TTGO will overheat)
int delayMillis = 80; 

// Initial estimate of maximum analogue reading
int max_y_value = 200;
int old_max_y_value = 0;
bool auto_y_range = false;

const int AUTO_Y_STEP = 20;

// Define spacing between data points
const int X_STEP = 2;

bool enable_user_select = true;
enum function {
  ANALOG_READ,
  SINE,
  DECAYING_SINE,
  COSINE_SINE_SUM,
  COSINE_SINE_PRODUCT,
  ULTRASONIC_SIGNAL
};
function functionSelect = ANALOG_READ;
char functionName[50];

// For the ANALOG_READ option, read analog values from this pin:
#define ANALOG_INPUT_PIN 3
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

// Configure plot colours
bool draw_gridlines = true;
#define BACKGROUND_COLOUR TFT_BLACK
#define GRIDLINES_COLOUR 0x2965
#define AXIS_COLOUR TFT_SILVER
#define DATA_COLOUR TFT_GOLD

#define X_DATUM 35
#define Y_DATUM 10
#define X_LENGTH 270
#define Y_HEIGHT 150

const int X_TICK_SIZE = 15;
const int Y_TICK_SIZE = 15;
const int NUM_X_TICKS = (X_LENGTH / X_TICK_SIZE) + 1;
const int NUM_Y_TICKS = (Y_HEIGHT / Y_TICK_SIZE) + 1;
const int NUM_DATA_POINTS = (X_LENGTH / X_STEP) + 1;

int buffer[NUM_DATA_POINTS];

void user_select();

int get_data_point(int sample_index, float omega = 0.12, float alpha = 0.008);

void drawGrid(bool buffer_full = false, int first_sample_index = 0, int last_sample_index = 0);

TFT_eSPI tft = TFT_eSPI(); // 320 x 170

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(3);
  tft.setTextSize(1);
  tft.fillScreen(BACKGROUND_COLOUR);
  setupUltrasonicSensor();
  if (enable_user_select) user_select();
  get_data_point(0); // sets function name
  tft.setTextDatum(TR_DATUM);
  drawGrid();
}

void loop() {
  static unsigned long sample_index = 0;
  static int running_max = 0;
  static int running_sum = 0;
  static int prev_x = -1;
  static int prev_y = -1;
  unsigned long first_sample_index = sample_index - NUM_DATA_POINTS + 1;
  int start_index = first_sample_index % NUM_DATA_POINTS;
  int write_index = sample_index % (NUM_DATA_POINTS); // end-point inclusive
  int analog_value = get_data_point(sample_index);
  buffer[write_index] = analog_value; // store in buffer, overwrites old value
  Serial.printf("%d: %d\n", sample_index, analog_value);

  running_sum += analog_value;
  if (analog_value > running_max) running_max = analog_value;

  if (sample_index > 0 && write_index == 0) {
    if (auto_y_range) {
      max_y_value = constrain(running_max + running_sum / NUM_DATA_POINTS, 0, 4095) / AUTO_Y_STEP * AUTO_Y_STEP;
    }
    running_max = 0;
    running_sum = 0;
    prev_x = -1;
    prev_y = -1;
  }
  int normalised_y = constrain(map(analog_value, 0, max_y_value, 0, Y_HEIGHT), 0, Y_HEIGHT);
  // Draw data line / point
  int curr_x = X_DATUM + write_index * X_STEP;
  int curr_y = Y_DATUM + (Y_HEIGHT - normalised_y);

  if (sample_index < NUM_DATA_POINTS) { // buffer not full yet
    if (prev_x != -1 && prev_y != -1)
      tft.drawLine(prev_x, prev_y, curr_x, curr_y, DATA_COLOUR);
    else
      tft.drawPixel(curr_x, curr_y, DATA_COLOUR);
  } else { // buffer is full
    Serial.printf("start: %3d, write: %3d\n", start_index, write_index);
    drawGrid(true, start_index, write_index);
  }

  prev_x = curr_x;
  prev_y = curr_y;
  sample_index++;

  delay(delayMillis);
}

void user_select() {
  int prevLeft = 0, prevRight = 0, currLeft, currRight;
  unsigned long lastUpdateTime = millis();
  int prev_choice = -1;
  int user_choice = 0;
  tft.setTextFont(2);
  tft.drawString("USER SELECT", X_DATUM, Y_DATUM+5);
  while (true) {
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);
    if (prevRight && !currRight && millis() - lastUpdateTime > delayMillis) {
      if (user_choice < 5) user_choice++;
      else user_choice = 0;
      lastUpdateTime = millis();
    }
    if (prev_choice != user_choice) {
      tft.setTextColor(GRIDLINES_COLOUR, BACKGROUND_COLOUR);
      tft.drawString("0. ANALOG SIGNAL", X_DATUM, Y_DATUM+30);
      tft.drawString("1. SINUSOID", X_DATUM, Y_DATUM+50);
      tft.drawString("2. DECAYING SINUSOID", X_DATUM, Y_DATUM+70);
      tft.drawString("3. SINE + COSINE", X_DATUM, Y_DATUM+90);
      tft.drawString("4. SINE * COSINE", X_DATUM, Y_DATUM+110);
      tft.drawString("5. ULTRASONIC SENSOR", X_DATUM, Y_DATUM+130);
    }
    tft.setTextColor(DATA_COLOUR, BACKGROUND_COLOUR);
    switch (user_choice) {
    case (0):
      functionSelect = ANALOG_READ;
      tft.drawString("0. ANALOG SIGNAL", X_DATUM, Y_DATUM+30);
      break;
    case (1):
      functionSelect = SINE;
      tft.drawString("1. SINUSOID", X_DATUM, Y_DATUM+50);
      break;
    case (2):
      functionSelect = DECAYING_SINE; 
      tft.drawString("2. DECAYING SINUSOID", X_DATUM, Y_DATUM+70);
      break;
    case (3):
      functionSelect = COSINE_SINE_SUM;
      tft.drawString("3. SINE + COSINE", X_DATUM, Y_DATUM+90);
      break;
    case (4):
      functionSelect = COSINE_SINE_PRODUCT;
      tft.drawString("4. SINE * COSINE", X_DATUM, Y_DATUM+110);
      break;
    case (5):
      functionSelect = ULTRASONIC_SIGNAL;
      tft.drawString("5. ULTRASONIC SENSOR", X_DATUM, Y_DATUM+130);
      break;
    }
    tft.setTextColor(AXIS_COLOUR, BACKGROUND_COLOUR);
    prev_choice = user_choice;
    if (prevLeft && !currLeft && millis() - lastUpdateTime > delayMillis) {
      break;
    }
    prevLeft = currLeft;
    prevRight = currRight;
  }
  tft.fillScreen(BACKGROUND_COLOUR);
  tft.setTextFont(1);
}

int get_data_point(int sample_index, float omega, float alpha) {
  switch (functionSelect) {
  case (ANALOG_READ):
    if (!auto_y_range) auto_y_range = true;
    sprintf(functionName, "Reading Analog Pin %d", ANALOG_INPUT_PIN);
    return analogRead(ANALOG_INPUT_PIN);
  case (SINE):
    sprintf(functionName, "sin(%.3ft)", omega);
    return max_y_value/2 * (1 + sin(sample_index * omega));
  case (DECAYING_SINE):
    sprintf(functionName, "exp(-%.3ft)sin(%.3ft)", alpha, omega);
    return max_y_value/2 * (1 + pow(2.71828f, -sample_index * alpha) * sin(sample_index * omega));
  case (COSINE_SINE_SUM):
    sprintf(functionName, "cos(%.3ft)+sin(%.3ft)", alpha, omega);
    return max_y_value/4 * (2 + cos(sample_index * alpha) + sin(sample_index * omega));
  case (COSINE_SINE_PRODUCT):
    sprintf(functionName, "cos(%.3ft)sin(%.3ft)", alpha, omega);
    return max_y_value/2 * (1 + cos(sample_index * alpha) * sin(sample_index * omega));
  case (ULTRASONIC_SIGNAL):
    sprintf(functionName, "Ultrasonic Sensor Data");
    pollUltrasonicSensor();
    return ultrasonicDistanceNearestCm;
  default:
    return 0;
  }
}

void drawGrid(bool buffer_full, int start, int end) {
  // refresh grid
  tft.fillRect(X_DATUM-1, Y_DATUM-1, X_LENGTH+2, Y_HEIGHT+2, BACKGROUND_COLOUR);
  if (draw_gridlines) {
    for (int i = 1; i < NUM_X_TICKS-1; i++)
      tft.drawFastVLine(X_DATUM + i*X_TICK_SIZE, Y_DATUM, Y_HEIGHT, GRIDLINES_COLOUR);
    for (int i = 1; i < NUM_Y_TICKS-1; i++)
      tft.drawFastHLine(X_DATUM, Y_DATUM + i*Y_TICK_SIZE, X_LENGTH, GRIDLINES_COLOUR);
  }
  tft.drawRect(X_DATUM-2, Y_DATUM-2, X_LENGTH+4, Y_HEIGHT+4, AXIS_COLOUR); // with padding
  tft.drawString(functionName, X_DATUM + X_LENGTH-8, Y_DATUM + 7); // with padding

  if (old_max_y_value != max_y_value) {
    tft.fillRect(0, 0, X_DATUM-8, 170, BACKGROUND_COLOUR);
    tft.drawNumber(max_y_value, X_DATUM-8, Y_DATUM-3); // labels with padding (right indent)
    tft.drawNumber(max_y_value/2, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT/2);
    tft.drawNumber(0, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT);
    old_max_y_value = max_y_value;
  }
  
  if (buffer_full) {
    int prev_index = start;
    int prev_x = X_DATUM;
    int normalised_prev_y = constrain(map(buffer[prev_index], 0, max_y_value, 0, Y_HEIGHT), 0, Y_HEIGHT);
    int prev_y = Y_DATUM + (Y_HEIGHT - normalised_prev_y);
    int x = 1;
    for (int i = (start+1) % NUM_DATA_POINTS; prev_index != end; i = (i+1) % NUM_DATA_POINTS, x++) {
      int normalised_y = constrain(map(buffer[i], 0, max_y_value, 0, Y_HEIGHT), 0, Y_HEIGHT);
      int curr_x = X_DATUM + x * X_STEP;
      int curr_y = Y_DATUM + (Y_HEIGHT - normalised_y);
      tft.drawLine(prev_x, prev_y, curr_x, curr_y, DATA_COLOUR);
      // Serial.printf("PREV: (%3d,%3d), CURR: (%3d, %3d)\n", prev_x, prev_y, curr_x, curr_y);
      prev_x = curr_x;
      prev_y = curr_y;
      prev_index = i;
    }
  }
}
