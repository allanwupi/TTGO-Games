// Time-Data Plotter
// Author: Allan Wu (23810308)
// Date: 20 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>

// Set update speed (don't set too fast or the TTGO will overheat)
int delay_millis = 80;

// Initial estimate of maximum analogue reading
int max_y_value = 100;
bool auto_y_range = false;
bool interpolate_between_points = true;

// Define spacing between data points
const int X_STEP = 2;

enum function {
  ANALOG_READ,
  SINE,
  DECAYING_SINE,
  COSINE_SINE_SUM,
  COSINE_SINE_PRODUCT,
};

function functionSelect = ANALOG_READ;
char functionName[50];

// For the ANALOG_READ option, read analog values from this pin:
const int ANALOG_INPUT_PIN = 1;

#define X_DATUM 35
#define Y_DATUM 10
#define X_LENGTH 270
#define Y_HEIGHT 150

const int X_TICK_SIZE = 15;
const int Y_TICK_SIZE = 15;
const int NUM_X_TICKS = (X_LENGTH / X_TICK_SIZE) + 1;
const int NUM_Y_TICKS = (Y_HEIGHT / Y_TICK_SIZE) + 1;
const int NUM_DATA_POINTS = (X_LENGTH / X_STEP) + 1;

// Configure plot colours
#define BACKGROUND_COLOUR TFT_BLACK
#define GRIDLINES_COLOUR 0x2965
#define AXIS_COLOUR TFT_SILVER
#define DATA_COLOUR TFT_GOLD

int data[NUM_DATA_POINTS];

int next_data_point(int sample_index, float omega = 0.1, float alpha = 0.009);

void drawGrid();

TFT_eSPI tft = TFT_eSPI(); // 320 x 170

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(3);
  tft.setTextColor(AXIS_COLOUR, BACKGROUND_COLOUR);
  tft.setTextSize(1);
  tft.setTextDatum(TR_DATUM);
  tft.fillScreen(BACKGROUND_COLOUR);
  next_data_point(0); // Initialise title
  drawGrid();
}

void loop() {
  static int sample_index = 0;
  static int running_max = 0;
  static int running_sum = 0;
  static int prev_x = -1;
  static int prev_y = -1;
  int array_index = sample_index % (NUM_DATA_POINTS);

  int analog_value = next_data_point(sample_index);

  running_sum += analog_value;
  if (analog_value > running_max) running_max = analog_value;
  int normalised_y = constrain(map(analog_value, 0, max_y_value, 0, Y_HEIGHT), 0, Y_HEIGHT);
  data[array_index] = normalised_y;

  if (sample_index > 0 && array_index == 0) {
    if (auto_y_range)
      max_y_value = constrain(running_max + running_sum / NUM_DATA_POINTS, 0, 4095) / 10 * 10;
    drawGrid();
    running_max = 0;
    running_sum = 0;
    prev_x = -1;
    prev_y = -1;
  }

  // Draw data line / point
  Serial.printf("(%d, %d)\n", sample_index, normalised_y);
  int curr_x = X_DATUM + array_index * X_STEP;
  int curr_y = Y_DATUM + (Y_HEIGHT - normalised_y);
  if (interpolate_between_points && prev_x != -1 && prev_y != -1)
    tft.drawLine(prev_x, prev_y, curr_x, curr_y, DATA_COLOUR);
  else
    tft.drawPixel(curr_x, curr_y, DATA_COLOUR);

  prev_x = curr_x;
  prev_y = curr_y;
  sample_index++;
  delay(delay_millis);
}

int next_data_point(int sample_index, float omega, float alpha) {
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
  default:
    return 0;
  }
}

void drawGrid() {
  tft.fillScreen(BACKGROUND_COLOUR);
  tft.drawNumber(max_y_value, X_DATUM-8, Y_DATUM-3); // labels with padding (right indent)
  tft.drawNumber(max_y_value/2, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT/2);
  tft.drawNumber(0, X_DATUM-8, Y_DATUM-3 + Y_HEIGHT);
  for (int i = 1; i < NUM_X_TICKS-1; i++) {
    tft.drawFastVLine(X_DATUM + i*X_TICK_SIZE, Y_DATUM, Y_HEIGHT, GRIDLINES_COLOUR);
  }
  for (int i = 1; i < NUM_Y_TICKS-1; i++) {
    tft.drawFastHLine(X_DATUM, Y_DATUM + i*Y_TICK_SIZE, X_LENGTH, GRIDLINES_COLOUR);
  }
  tft.drawRect(X_DATUM-2, Y_DATUM-2, X_LENGTH+4, Y_HEIGHT+4, AXIS_COLOUR); // with padding
  tft.drawString(functionName, X_DATUM + X_LENGTH-8, Y_DATUM + 7); // with padding
}
