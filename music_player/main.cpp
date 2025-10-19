#include <Arduino.h>
#include <TFT_eSPI.h>
#include "songs.h"

#define BUZZER_PIN 1
#define LEFT_BUTTON 0
#define RIGHT_BUTTON 14

#define MENU_X_DATUM    10
#define MENU_Y_DATUM    30
#define BUFFER_CHARS    50

#define LEFT_BUTTON     0
#define RIGHT_BUTTON    14

#define PRIMARY_TEXT_COLOUR     TFT_WHITE
#define HIGH_EMPHASIS_COLOUR    TFT_GOLD
#define LOW_EMPHASIS_COLOUR     0x2965
#define BACKGROUND_COLOUR       TFT_BLACK

TFT_eSPI tft = TFT_eSPI();

int chosenSong;

void playSong(Song_t song, int barsToDisplay = 2);

void selectSong();

void setup() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    tft.init();
    tft.setTextColor(PRIMARY_TEXT_COLOUR, BACKGROUND_COLOUR);
    tft.setTextFont(0);
    tft.setTextSize(2);
    tft.setRotation(3);
    tft.fillScreen(BACKGROUND_COLOUR);
    selectSong();
}

void loop() {
    switch (chosenSong) {
        case (0):
            playSong(Megalovania, 1);
            break;
        case (1):
            playSong(TheLegend0, 4);
            playSong(TheLegend1, 4);
            playSong(TheLegend2, 2);
            playSong(TheLegend3, 4);
            break;
        default:
            playSong(FreedomMotif, 2);
    }
}

void selectSong() {
    int prevLeft = 0, prevRight = 0;
    int currLeft = 0, currRight = 0;
    int prevChoice = -1, currChoice = 0;
    bool startPlayer = false;

    tft.setCursor(0, 0);
    tft.printf(" WORST MUSIC PLAYER EVER ");
    tft.drawFastHLine(0, 20, 320, TFT_WHITE);

    while (!startPlayer) {
    currLeft = !digitalRead(LEFT_BUTTON);
    currRight = !digitalRead(RIGHT_BUTTON);
    if (prevLeft && !currLeft) {
            startPlayer = true;
        } else if (prevRight && !currRight) {
            currChoice = (currChoice + 1) % 3;
        }

        if (prevChoice != currChoice) {
            tft.setTextColor(LOW_EMPHASIS_COLOUR, BACKGROUND_COLOUR);

            for (int i = 0; i < 3; i++) {
            tft.drawString(SONG_DESCRIPTIONS[i], MENU_X_DATUM, MENU_Y_DATUM+23*i);
            }

            tft.setTextColor(HIGH_EMPHASIS_COLOUR, BACKGROUND_COLOUR);
            tft.drawString(SONG_DESCRIPTIONS[currChoice], MENU_X_DATUM, MENU_Y_DATUM+23*currChoice);
        }

        tft.setTextColor(PRIMARY_TEXT_COLOUR, BACKGROUND_COLOUR);
        prevChoice = currChoice;

        prevLeft = currLeft;
        prevRight = currRight;
    }
    tft.fillScreen(BACKGROUND_COLOUR);
    chosenSong = currChoice;
}

void playSong(Song_t song, int barsToDisplay)
{
    const int songLength = song.numNotes;
    const int minN = song.minFreqIndex;
    const int maxN = song.maxFreqIndex;
    const int dx = 320/barsToDisplay/song.bar;
    const int dy = 150/(maxN - minN);
    const int T0 = song.period;

    int freq, n, T;
    int periods = 0;
    unsigned long startTime;
    int act_delay;
    tft.setCursor(0, 0);
    tft.printf("000/%-3d: %-3s %.13s", songLength, TONE_NAMES[n], song.name);
    tft.drawFastHLine(0, 20, 320, TFT_WHITE);
    for (int i = 0, k = 0; i < songLength; i++) {
        startTime = millis();
        n = song.notes[i].freqIndex;
        T = song.notes[i].noteLength * T0;
        tft.setCursor(0, 0);
        if (periods % (barsToDisplay*song.bar) == 0) {
            tft.fillRect(0, 21, 320, 149, TFT_BLACK); 
            periods = 0;
            k = !k;
        }
        if (n > 0) {
            freq = TONE_FREQS[n];
            tone(BUZZER_PIN, freq);
            tft.drawFastHLine(j*dx, 169-dy*(n-minN), dx*(T/T0)-2, HIGH_EMPHASIS_COLOUR);
            if (song.overflow)
                tft.printf("%3d/%-3d: %-3s %.13s", i+1, songLength, TONE_NAMES[n], (k) ? song.name : song.overflow);
            else
                tft.printf("%3d/%-3d: %-3s %.13s", i+1, songLength, TONE_NAMES[n], song.name);
        } else {
            noTone(BUZZER_PIN);
            tft.drawFastHLine(j*dx, 169, dx*(T/T0)-2, LOW_EMPHASIS_COLOUR);
            tft.printf("%3d/%-3d: ", i+1, songLength);
        }
        periods += song.notes[i].noteLength;
        act_delay = T - (millis() - startTime);
        if (act_delay > 0) delay(act_delay);
    }
    noTone(BUZZER_PIN);
}
