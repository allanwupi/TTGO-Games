// Water Ripple Simulation
// Allan Wu (23810308)
// 24 September 2025
#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT 0
#define RIGHT 14

// simulation parameters
#define DELAY_MILLIS 50
#define XDIM 160
#define YDIM 85
#define L 2
#define NUM_SHADES 15

TFT_eSPI tft = TFT_eSPI(); // 170 x 320 pixels

bool sourceOn = true;
bool pulseOn = false;
int sourceRow = YDIM/2;
int sourceCol = XDIM/2;
uint16_t buffer1[XDIM * YDIM];
uint16_t buffer2[XDIM * YDIM];
int shades[] = {
  0x0007,
  0x0009,
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

  for (int i = 1; i < XDIM-1; i++) {
    for (int j = 1; j < YDIM-1; j++) {
      uint16_t height = random(0, 0xffff);
      buffer1[j*XDIM + i] = height;
      buffer2[j*XDIM + i] = height;
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

  if (!prevLeft && currLeft) {
    sourceOn = !sourceOn;
    if (sourceOn){
      sourceRow = random(0, YDIM);
      sourceCol = random(0, XDIM);
    }
  }
  if (millis() - lastUpdateTime > DELAY_MILLIS) {
    processWater(p1, p2);
    if (currLeft && currRight) {
      for (int i = 1; i < XDIM-1; i++) {
        for (int j = 1; j < YDIM-1; j++) {
          if (p2[j*XDIM + i] > 0x5000 && p2[j*XDIM + i] < 0xa000)
            p2[j*XDIM + i] = p2[j*XDIM + i] + random(0,0x5000);
          p1[j*XDIM + i] = p2[j*XDIM + i];
        }
      }
    } else if (currRight) {
        pulseOn = true;
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
      tft.fillRect(i*L, j*L, L, L, shades[shadeIndex]);
    }
  }
}

void processWater(uint16_t *source, uint16_t *dest) {
  for (int i = 1; i < XDIM-1; i++) {
    for (int j = 1; j < YDIM-1; j++) {
      uint16_t smoothed =
        source[((j-1)*XDIM + i)]
        + source[((j+1)*XDIM + i)]
        + source[(j*XDIM + i-1)]
        + source[(j*XDIM + i+1)] >> 1;
      if (smoothed > dest[j*XDIM + i]) dest[j*XDIM + i] = smoothed - dest[j*XDIM + i];
      else dest[j*XDIM + i] = 0;
      uint16_t dampening = dest[j*XDIM + i] >> 11;
      if (dampening < dest[j*XDIM + i]) dest[j*XDIM + i] -= dampening;
      else dest[j*XDIM + i] = 0;
    }
  }
  if (sourceOn) {
    sourceRow += random(-1, 2);
    sourceCol += random(-1, 2);
    if (sourceRow <= 0 || sourceRow >= YDIM-1) sourceRow = YDIM/2;
    if (sourceCol <= 0 || sourceCol >= XDIM-1) sourceRow = XDIM/2;
    dest[sourceRow*XDIM + sourceCol] = 0xffff;
  }
  if (pulseOn) {
    int row = random(20, XDIM-20);
    int col = random(20, YDIM-20);
    int d = random(0, 3);
    for (int i = row-d; i < row+d+1; i++) {
      for (int j = col-d; j < col+d+1; j++) {
        dest[j*XDIM + i] = 0xffff;
        dest[j*XDIM + i] = 0xffff;
      }
    }
    pulseOn = false;
  }
}
