// Rocket Game
// Last update: 23/08/2025

#include <Arduino.h>
#include <TFT_eSPI.h>

#define LEFT 0 // accelerate
#define RIGHT 14 // impulse

// simulation parameters
#define SIMULATION_STEP 120
#define CLEAR_STEP 10
#define DRAW_STEP 20
#define XBOUND 170
#define YBOUND 320

int sky_color;
int atmosphere[] = {TFT_BLUE, TFT_BLUE, TFT_NAVY, TFT_BLACK}; //{TFT_BLACK, TFT_BLACK, TFT_BLACK};
int health_bar[] = {TFT_WHITE, TFT_GREEN, TFT_GOLD, TFT_RED, TFT_MAROON};
int ground_color = TFT_GOLD; // TFT_ORANGE
int max_fuel = 100;
int y_offset = 30;
int x_offset = 0;
String player = "@";

TFT_eSPI tft = TFT_eSPI(); // 170 x 320 pixels

void endScreen(bool state, unsigned long time, int money);

void setup()
{
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
  sky_color = atmosphere[0];

  Serial.begin(115200);

  tft.init();
  tft.setTextColor(health_bar[0]);
  tft.setTextSize(1);
  tft.fillScreen(sky_color);
  tft.fillRect(x_offset, (YBOUND - y_offset), XBOUND, y_offset, ground_color);
}

void loop()
{
  static int x = XBOUND/2, y = YBOUND - y_offset -8;
  static int ground = YBOUND - y_offset - 8;
  static bool above = false;
  static int speed = 0;
  static double grav = 1;
  static double accel = 1;
  static int level = 0;
  static int prev_level = 0;
  static int prev_y = 0;
  static int lastState = 0;
  static int fuel = max_fuel;
  static unsigned long lastSim = 0;
  static unsigned long lastClear = 0;
  static unsigned long lastDrawn = 0;
  static int height = 0;
  static int gameOver = 0;
  static int completed = 0;

  if (speed > 999 && completed == 0) {
    tft.setTextSize(3);
    completed = 1;
    endScreen(true, millis(), max_fuel);
  }
  if (gameOver > 50) {
    tft.setTextSize(3);
    endScreen(false, millis(), max_fuel);
  }
  if (millis() - lastSim > SIMULATION_STEP) {
    if (y >= ground && fuel <= max_fuel) {
      fuel += min(10, max_fuel - fuel);
      accel = grav;
    }
    if (fuel > 0 && !lastState && !digitalRead(RIGHT)) {
      if (fuel < 0.10 * max_fuel) fuel = 0;
      else fuel -= 0.10 * max_fuel;
      speed += min(5.0, accel) * max_fuel / 20;
      if (accel > 1) accel = max(accel-1.0, 1.0);
    }
    if (fuel > 0 && !digitalRead(LEFT)) {
      fuel -= 1;
      if (speed < 200) speed++;
      if (accel > 1) accel = max(accel-0.1, 1.0);
    }
    if (y != ground && !completed && (fuel <= 0 || digitalRead(LEFT))) {
      accel += 0.02;
      if (accel < grav) accel += 0.18;
      speed -= round(accel);
    }
    if (-(y-ground) < 300) {
      accel = grav;
      if (speed < -16) speed += 9;
      else if (speed < -7) speed += 2;
      else if (speed == -7) speed++;
    }
    Serial.println(accel);

    lastState = !digitalRead(RIGHT);

    tft.setCursor(0,0);
    tft.setTextSize(2);
    tft.drawString(String(fuel*100.0/max_fuel) + "%  ",0,0);
    tft.setTextDatum(TR_DATUM);
    tft.drawString(" $" + String(max_fuel),170,0);
    tft.setTextDatum(TL_DATUM);
    tft.setTextSize(1);
    tft.drawString(String(-(y-ground)) + " m ",0,24);
    tft.drawString(String(speed) + " m/s ",0,36);

    lastSim += SIMULATION_STEP;
  }

  if (millis() - lastClear > CLEAR_STEP) {
    if (-(y-ground) < 300) {
      level = 0;
      grav = 1.0;
    } else if (-(y-ground) < 9900) {
      level = 1;
      grav = 3.0; // 3.0
    } else if (-(y-ground) < 33300) {
      level = 2;
      grav = 2.0; // 2.0
    } else {
      level = 3;
      grav = 1.0; // 1.0
    }
    sky_color = atmosphere[level];

    if (y != prev_y) {
      if (level != prev_level) tft.fillScreen(sky_color);
      if (!above) {
        tft.fillRect(x-2, 0, 9, (YBOUND - y_offset), sky_color);
      }
      else tft.fillRect(x-2, 0, 9, YBOUND, sky_color);
    }
    prev_y = y;
    prev_level = level;
  }

  if (millis() - lastDrawn > DRAW_STEP) {
    if (speed > 0) height += speed;
    y -= speed;
    if (y > ground) {
      y = ground;
      if (speed <= -7) {
        fuel += ((6 + speed) * max_fuel)/40;
        max_fuel += ((6 + speed) * max_fuel)/40;
      } else if (speed < 0 && height > 100) {
        tft.setTextSize(2);
        tft.setTextDatum(TR_DATUM);
        if (speed > -3) {
          tft.drawString("++$" + String(height/100),170,0);
          height *= 2;
        } else {
          tft.drawString(" +$" + String(height/100),170,0);
        }
        tft.setTextDatum(TL_DATUM);
        delay(500);
        tft.setTextSize(1);
        max_fuel += height/100;
        fuel = max_fuel;
      }
      height = 0;
      speed = 0;
    }

    if (fuel > 0.75 * max_fuel) {
      tft.setTextColor(health_bar[0], sky_color);
    } else if (fuel > 0.5 * max_fuel) {
      tft.setTextColor(health_bar[1], sky_color);
    } else if (fuel > 0.25 * max_fuel) {
      tft.setTextColor(health_bar[2], sky_color);
    } else if (fuel > 0) {
      tft.setTextColor(health_bar[3], sky_color);
    } else if (fuel > -max_fuel) {
      tft.setTextColor(health_bar[4], sky_color);
    } else {
      tft.setTextColor(sky_color);
      gameOver++;
    }
    lastDrawn += DRAW_STEP;
    if (!above && y < 0) {
      tft.fillRect(x_offset, (YBOUND - y_offset), XBOUND, y_offset, sky_color);
      above = true;
    }
    if (y > 0) {
      tft.fillRect(x_offset, (YBOUND - y_offset), XBOUND, y_offset, ground_color);
      //tft.drawEllipse(XBOUND/2,YBOUND-15, 20, 7, TFT_SILVER);
      above = false;
    }
  }
  tft.drawString(player, x, YBOUND - abs(YBOUND - y) % YBOUND);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(String(millis()/1000.0) + " s",170,24);
  tft.setTextDatum(TL_DATUM);
}

void endScreen(bool state, unsigned long time, int money) {
  static int counter = 0;
  switch (state) {
    case (false):
      tft.fillScreen(sky_color);
      tft.setTextColor(TFT_WHITE, sky_color);
      tft.drawString("GAME OVER", 4, YBOUND/2);
      delay(1000);
      tft.fillScreen(sky_color);
      tft.setTextColor(TFT_LIGHTGREY, sky_color);
      tft.drawString("GAME OVER", 8, YBOUND/2);
      delay(1000);
      break;
    case (true):
      tft.fillScreen(sky_color);
      tft.setTextColor(TFT_GOLD, sky_color);
      tft.drawString("YOU WIN!", 10, YBOUND/2);
      tft.drawString(String(time/1000.0) + " s",10,YBOUND/2+30);
      delay(1000);
      tft.fillScreen(sky_color);
      tft.setTextColor(TFT_GREEN, sky_color);
      tft.drawString("YOU WIN!", 15, YBOUND/2);
      tft.drawString("$" + String(max_fuel) + "  ",15,YBOUND/2+30);
      delay(1000);
      counter++;
  }
  if (false || counter < 5) endScreen(state, time, money);
  if (counter == 5) tft.fillScreen(sky_color);
}

