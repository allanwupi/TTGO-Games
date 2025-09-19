// Basic Joystick Example
// Keyestudio Joystick Module
// 19 September 2025

#include <Arduino.h>
#include <TFT_eSPI.h>

int joyStickX = 1; //x
int joyStickY = 2; //y
int joyStickZ = 3; //z

TFT_eSPI tft = TFT_eSPI();

char grid[5][6] = {{0}};

void setup() 
{
  Serial.begin(9600); // 9600 bps
  pinMode(joyStickZ, INPUT);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(3);
  tft.setRotation(3);
  tft.setTextDatum(CC_DATUM);
}

void loop() 
{
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int x,y,z;
  int xDir, yDir;
  x=analogRead(joyStickX);
  y=analogRead(joyStickY);
  z=digitalRead(joyStickZ);
  Serial.print(x,DEC);
  Serial.print(",");
  Serial.print(y,DEC);
  Serial.print(",");
  Serial.println(z,DEC);

  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      grid[i][j] = ' ';
    }
  }

  grid[2][2] = '+';

  // Median value is (1980, 2060)

  if (x > 3900) {
    xDir = 4;
  } else if (x > 2100) {
    xDir = 3;
  } else if (x < 200) {
    xDir = 0;
  } else if (x < 1900) {
    xDir = 1;
  } else {
    xDir = 2;
  }

  if (y > 3900) {
    yDir = 0;
  } else if (y > 2120) {
    yDir = 1;
  } else if (y < 200) {
    yDir = 4;
  } else if (y < 2000) {
    yDir = 3;
  } else {
    yDir = 2;
  }

  if (z == HIGH) tft.setTextColor(TFT_GOLD, TFT_BLACK);

  grid[yDir][xDir] = 'X';

  tft.drawString(grid[0],160,35);
  tft.drawString(grid[1],160,60);
  tft.drawString(grid[2],160,85);
  tft.drawString(grid[3],160,110);
  tft.drawString(grid[4],160,135);
  delay(50);
}
