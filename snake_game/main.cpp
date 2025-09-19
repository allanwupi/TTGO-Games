// Snake game
// Last update: 26/08/2025
#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT 0
#define RIGHT 14

// simulation parameters
#define STEPSIZE 100
#define DEFTIMER 12
#define PERIOD_DEC 1
#define PERIOD_INC 1
#define MAXPERIOD 500
#define MINPERIOD 20
#define INVINCIBILITY 50
#define XDIM 17
#define YDIM 32
#define L 10
#define NUM_DIRECTIONS 4
#define NUM_COLOURS 10
#define NUM_SHADES 32

TFT_eSPI tft = TFT_eSPI(); // 170 x 320 pixels

const int xDir[NUM_DIRECTIONS] = {0,1,0,-1}; //{1,1,0,-1,-1,-1,0,1};
const int yDir[NUM_DIRECTIONS] = {1,0,-1,0}; //{0,1,1,1,0,-1,-1,-1};
int grid[17][32];

unsigned long shades[NUM_SHADES];
int size = 8;
int enemySize = 12;
int s = 0;
int e = 0;
int c = -1;

void gameOver();

void setup()
{
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);

  shades[0] = 0x0000;
  for (int i = 1; i < NUM_SHADES-1; i++) {
    if (i < 2) {
      shades[i] = ((2 << 11) | (2*2 << 5) | 2);
    } else {
      shades[i] = ((i << 11) | (2*i << 5) | i);
    }
  }

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  Serial.begin(115200);
}

void loop()
{
  static int xPos = 9; // 9
  static int yPos = 0; // 31
  static int xEnemy = 2;
  static int yEnemy = 17;
  static unsigned long lastStep = 0;
  static unsigned long clock = 0;
  static int round = 0;
  static int fruits = 1;
  static int slowdown = 0;
  static int speedup = 0;
  static int invincible = 0;
  static int treasure = 75;
  static int period = STEPSIZE;
  static int timer = DEFTIMER;

  if (!digitalRead(LEFT) && !digitalRead(RIGHT)) {
    s = 2;
  } else if (!digitalRead(LEFT)) {
    s = 3;
  } else if (!digitalRead(RIGHT)) {
    s = 1;
  } else s = 0;

  if (fruits > 0) {
    int xFruit = random(0,XDIM);
    int yFruit = random(0,YDIM);
    int fruitType = random(0,100);
    fruits--;
    if (fruitType > 25 && fruitType <= 89) {
      grid[xFruit][yFruit] = TFT_RED;
      tft.fillRoundRect(xFruit*L, yFruit*L, L, L, 3, TFT_RED);
    } else if (fruitType > 90 && invincible == 0) {
      grid[xFruit][yFruit] = TFT_CYAN;
      tft.fillRoundRect(xFruit*L, yFruit*L, L, L, 3, TFT_CYAN);
    } else {
      if (fruitType > treasure) {
        if (treasure < 94) treasure += 5;
        grid[xFruit][yFruit] = TFT_GOLD;
        tft.fillRoundRect(xFruit*L, yFruit*L, L, L, 3, TFT_GOLD);
      } else {
        grid[xFruit][yFruit] = TFT_GREENYELLOW;
        tft.fillRoundRect(xFruit*L, yFruit*L, L, L, 3, TFT_GREENYELLOW);
      }
    }
    if (slowdown > 0) {
      round = -slowdown * timer;
      period = min(MAXPERIOD, period+slowdown*PERIOD_INC);
      slowdown = 0;
    } else if (speedup > 0) {
      period = min(MAXPERIOD, period+speedup*PERIOD_INC);
      speedup = 0;
    } else {
      period = min(MAXPERIOD, period+PERIOD_INC);
    }
  }

  if (round >= timer) {
    timer = DEFTIMER * (100/period) + (size / 8);
    period = max(MINPERIOD, period-PERIOD_DEC);
    round = 0;
  }

  if (millis() - clock > STEPSIZE) {
    round++;
    clock += STEPSIZE;
  }

  if (millis() - lastStep > period) {
    tft.setTextDatum(TR_DATUM);
    if (round < 0) tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
    else tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString(" " + String(1000.0/period) + " ",L*XDIM,0);
    tft.setTextDatum(TL_DATUM);
    if (invincible > 0) {
      if (invincible != INVINCIBILITY) invincible--;
      if (invincible >= 0.2 * INVINCIBILITY) tft.setTextColor(TFT_CYAN, TFT_BLACK);
      if (invincible < 0.2*INVINCIBILITY) tft.setTextColor(TFT_DARKCYAN, TFT_BLACK);
    }
    else tft.setTextColor(TFT_WHITE, TFT_BLACK);
    Serial.println(period);
    Serial.println(timer);
    tft.drawString(" " + String(size) + " ",0,0);
    int food = grid[xPos][yPos];
    int eFood = grid[xEnemy][yEnemy];
    if (eFood == TFT_RED || eFood == TFT_GREENYELLOW || eFood == TFT_CYAN || eFood == TFT_GOLD) {
      enemySize += 1;
      fruits++;
    }
    if (food == TFT_RED || food == TFT_GREENYELLOW || food == TFT_CYAN || food == TFT_GOLD) {
      size += 1;
      if (food == TFT_GREENYELLOW) {
        slowdown = 10;
      } else if (food == TFT_CYAN) {
        invincible = INVINCIBILITY;
        speedup = 5;
      } else if (food == TFT_GOLD) {
        if (period <= 50) size += 55-period;
        else size += 4;
        if (treasure < 95) fruits++;
      }
      fruits++;
    } else if (grid[xPos][yPos] > 0) {
      if (invincible == 0) {
        tft.fillScreen(TFT_BLACK);
        gameOver();
      }
      invincible--;
      grid[xPos][yPos] = 0;

      size = max(4, size-1);
    }
    grid[xEnemy][yEnemy] = enemySize;
    grid[xPos][yPos] = size;
    for (int i = 0; i < XDIM; i++) {
      for (int j = 0; j < YDIM; j++) {
        int food = grid[i][j];
        if (food <= 0) {
          continue;
        } else if (food == TFT_RED || food == TFT_GREENYELLOW || food == TFT_CYAN || food == TFT_GOLD) {
          continue;
        } else if (grid[i][j]-- >= NUM_SHADES) {
          tft.fillRoundRect(i*L, j*L, L, L, 3, TFT_WHITE);
        } else
          tft.fillRoundRect(i*L, j*L, L, L, 3, shades[grid[i][j]]);
      }
    }
    xPos = (xPos + xDir[s]) % XDIM;
    if (xPos < 0) xPos += XDIM;
    yPos = (yPos + yDir[s]) % YDIM;
    if (yPos < 0) yPos += YDIM;
    e = (e + random(-1, 2))%NUM_DIRECTIONS;
    if (e < 0) e += NUM_DIRECTIONS;
    xEnemy = (xEnemy + xDir[e]) % XDIM;
    if (xEnemy < 0) xEnemy += XDIM;
    yEnemy = (yEnemy + yDir[e]) % YDIM;
    if (yEnemy < 0) yEnemy += YDIM;
    lastStep += period;
  }
}

void gameOver() {
  tft.setTextSize(2);
  tft.setTextDatum(CC_DATUM);
  tft.drawString("Score: " + String(size), L*XDIM/2, L*YDIM/2);
  delay(10000);
  gameOver();
}
