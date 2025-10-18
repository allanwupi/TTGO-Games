#include <Arduino.h>
#include <TFT_eSPI.h>
#include "songs.h"

#define BUZZER_PIN 1

TFT_eSPI tft = TFT_eSPI();
void playSong(Song_t song, int barsToDisplay = 2);

void setup() {
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(15, OUTPUT);
    digitalWrite(15, HIGH);
    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setRotation(3);
}

void loop() {
    playSong(TheLegend0, 2);
    playSong(TheLegend1, 2);
    playSong(TheLegend2, 2);
    playSong(TheLegend3, 2);
    playSong(Megalovania1, 1);
    playSong(Megalovania2, 1);
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
    tft.printf("00/%2d:%-2s %-4d %.12s", songLength, TONE_NAMES[n % 12], freq, song.name);
    tft.drawFastHLine(0, 20, 320, TFT_WHITE);
    for (int i = 0, j = 0, k = 0; i < songLength; i++) {
        startTime = millis();
        n = song.notes[i].freqIndex;
        T = song.notes[i].noteLength * T0;
        tft.setCursor(0, 0);
        if (periods % (barsToDisplay*song.bar) == 0) {
            tft.fillRect(0, 21, 320, 149, TFT_BLACK); 
            j = 0;
            k = !k;
        }
        if (n > 0) {
            freq = TONE_FREQS[n];
            tone(BUZZER_PIN, freq);
            tft.drawFastHLine(j*dx, 169-dy*(n-minN), dx*(T/T0)-2, TFT_GOLD);
            if (song.overflow)
                tft.printf("%02d/%2d:%-2s %-4d %.12s", i+1, songLength, TONE_NAMES[n % 12], freq, (k) ? song.name : song.overflow);
            else
                tft.printf("%02d/%2d:%-2s %-4d %s", i+1, songLength, TONE_NAMES[n % 12], freq, song.name);
        } else {
            noTone(BUZZER_PIN);
            tft.drawFastHLine(j*dx, 169, dx*(T/T0)-2, 0x2104);
            tft.printf("%02d/%2d:", i+1, songLength);
        }
        periods += song.notes[i].noteLength;
        j += T/T0;
        act_delay = T - (millis() - startTime);
        if (act_delay > 0) delay(act_delay);
    }
    noTone(BUZZER_PIN);
}
