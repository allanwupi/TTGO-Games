// Water Ripple Simulation
// Allan Wu (23810308)
// 24 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT 0
#define RIGHT 14

// simulation parameters
#define DELAY_MILLIS 50
#define XDIM 180
#define YDIM 95
#define NUM_SHADES 16

TFT_eSPI tft = TFT_eSPI(); // 170 x 320 pixels

const bool randomStart = false;

bool sourceOn = true;
bool rainfallOn = false;
unsigned long rainTimer = DELAY_MILLIS;
unsigned long directionTimer = DELAY_MILLIS;
int sourceRow = YDIM/2;
int sourceCol = XDIM/2;
uint16_t buffer1[XDIM * YDIM];
uint16_t buffer2[XDIM * YDIM];
int shades[] = {
  0x0008,
  0x0009,
  0x0010,
  0x0012,
  0x0015,
  0x0018,
  0x001a,
  0x001c,
  0x001f,
  0x541f,
  0x65bf,
  0xc71f,
  0xd75f,
  0xefbf,
  0xf7df,
  0xffff
};

void renderWater(uint16_t *dest);
void processWater(uint16_t *source, uint16_t *dest);

void setup()
{
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  if (randomStart) {
    for (int i = 1; i < XDIM-1; i++) {
      for (int j = 1; j < YDIM-1; j++) {
        uint16_t height = random(0, 0xffff);
        buffer1[j*XDIM + i] = height;
        buffer2[j*XDIM + i] = height;
      }
    }
  }
  renderWater(buffer2);
}

void loop()
{
  static unsigned long lastUpdateTime = millis();
  static int prevLeft = 0, prevRight = 0;
  static int currLeft = 0, currRight = 0;
  static uint16_t *p1 = buffer1;
  static uint16_t *p2 = buffer2;
  static uint16_t *temp;
  currLeft = !digitalRead(LEFT);
  currRight = !digitalRead(RIGHT);

  if (prevLeft && !currLeft) {
    sourceOn = !sourceOn;
    if (sourceOn) {
      sourceRow = random(10, YDIM-10);
      sourceCol = random(10, XDIM-10);
      directionTimer = 0;
    }
  } else if (prevRight && !currRight) {
    rainfallOn = !rainfallOn;
    if (rainfallOn) rainTimer = 0;
  }

  if (millis() - lastUpdateTime > DELAY_MILLIS) {
    processWater(p1, p2);
    if (currLeft && currRight) {
      for (int i = 1; i < XDIM-1; i++) {
        for (int j = 1; j < YDIM-1; j++) {
          if (p2[j*XDIM + i] > 0x2000 && p2[j*XDIM + i] < 0x5000)
            p2[j*XDIM + i] += random(0,0xa000);
          p1[j*XDIM + i] = p2[j*XDIM + i];
        }
      }
    }
    renderWater(p2);
    temp = p1;
    p1 = p2;
    p2 = temp;
    lastUpdateTime = millis();
  }
  prevLeft = currLeft;
  prevRight = currRight;
}

void renderWater(uint16_t *dest) {
  for (int i = 0; i < XDIM; i++) {
    for (int j = 0; j < YDIM; j++) {
      int colorVal = dest[j*XDIM + i];
      int shadeIndex = map(colorVal, 0, 0xffff, 0, NUM_SHADES);
      //tft.drawPixel(i-5, j-5, shades[shadeIndex]);
      tft.fillRect(i*2-5, j*2-5, 2, 2, shades[shadeIndex]);
    }
  }
}

void processWater(uint16_t *source, uint16_t *dest) {
  static unsigned long lastRainfall = millis();
  static unsigned long lastDirection = millis();
  for (int i = 1; i < XDIM-1; i++) {
    for (int j = 1; j < YDIM-1; j++) {
      uint16_t smoothed =
        source[((j-1)*XDIM + i)]
        + source[((j+1)*XDIM + i)]
        + source[(j*XDIM + i-1)]
        + source[(j*XDIM + i+1)] >> 1;
      if (smoothed > dest[j*XDIM + i]) dest[j*XDIM + i] = smoothed - dest[j*XDIM + i];
      else dest[j*XDIM + i] = 0;
      uint16_t dampening = dest[j*XDIM + i] >> 13;
      if (dampening < dest[j*XDIM + i]) dest[j*XDIM + i] -= dampening;
      else dest[j*XDIM + i] = 0;
    }
  }
  // Randomly moving bullet
  static int offsetX = 0;
  static int offsetY = 0;
  if (sourceOn) {
    if (millis() - lastDirection > directionTimer) {
      offsetX = random(-6, 4);
      offsetY = random(-6, 4);
      if (offsetX < -3 || offsetY < -3) {
        offsetX = 0;
        offsetY = 0;
      }
      directionTimer = 1000*DELAY_MILLIS;
      lastDirection = millis();
    }
    sourceRow += (offsetX + random(-1, 2));
    sourceCol += (offsetY + random(-1, 2));
    if (sourceRow < 5) {
      sourceRow = 10;
      directionTimer = 0;
    } else if (sourceRow > YDIM-6) {
      sourceRow = YDIM-10;
      directionTimer = 0;
    }
    if (sourceCol < 5) {
      sourceCol = 10;
      directionTimer = 0;
    } else if (sourceCol > XDIM-6) {
      sourceCol = XDIM-10;
      directionTimer = 0;
    }
    dest[sourceRow*XDIM + sourceCol] = 0xffff;
  }
  // Raindrops
  if (rainfallOn && millis() - lastRainfall > rainTimer) {
    int repeats = random(-3, 9);
    if (repeats < 1) repeats = 1;
    rainTimer = random(10*repeats*DELAY_MILLIS, 30*repeats*DELAY_MILLIS);
    while (repeats > 0) {
      int row = random(10, XDIM-10);
      int col = random(10, YDIM-10);
      int d = random(-8, 4);
      if (d < -6) d = 0;
      else if (d < 0) d = 1;
      for (int i = row-d; i < row+d+1; i++) {
        for (int j = col-d; j < col+d+1; j++) {
          if (random (-1, 3) < 0) {
            i += random(-1, 2);
            j += random(-1, 2);
          }
          dest[j*XDIM + i] = 0xffff;
        }
      }
      repeats--;
    }
    lastRainfall = millis();
  }
}
