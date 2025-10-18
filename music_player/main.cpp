#include <Arduino.h>
#include <TFT_eSPI.h>
#include "songs.h"

#define BUZZER_PIN 1

TFT_eSPI tft = TFT_eSPI();
void playSong(Song_t song);

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
    playSong(TheLegend0);
    playSong(TheLegend1);
    playSong(TheLegend2);
    playSong(TheLegend3);
    delay(1000);
}

void playSong(Song_t song)
{
    const int songLength = song.numNotes;
    const int dx = 320/song.barPeriod;
    const int dy = 150/(song.maxFreqIndex - song.minFreqIndex);
    const int pause = 20;

    int freq, n, T;
    int periods = 0;
    unsigned long startTime;
    int act_delay;
    tft.drawFastHLine(0, 20, 320, TFT_WHITE);
    for (int i = 0, j = 0; i < songLength; i++) {
        startTime = millis();
        n = song.notes[i].freqIndex;
        T = song.notes[i].noteLength * song.period;
        tft.setCursor(0, 0);
        if (periods % (song.barPeriod) == 0) {
            tft.fillRect(0, 21, 320, 149, TFT_BLACK); 
            j = 0; 
        }
        if (n > 0) {
            freq = TONE_FREQS[n];
            tone(BUZZER_PIN, freq);
            tft.drawFastHLine(j*dx, 169-dy*(n-song.minFreqIndex), dx*(T/song.period)-3, TFT_GOLD);
            tft.printf("%02d/%2d:%-2s %-4d %s", i+1, songLength, TONE_NAMES[n % 12], freq, song.name);
            delay(T-pause);
        } else {
            noTone(BUZZER_PIN);
            tft.drawFastHLine(j*dx, 169, dx*(T/song.period)-2, 0x2104);
            tft.printf("%02d/%2d:--      %s", i+1, songLength, song.name);
            delay(T-pause);
        }
        periods += song.notes[i].noteLength;
        j += T/song.period;
        act_delay = T - (millis() - startTime);
        if (act_delay > 0) delay(act_delay);
    }
    noTone(BUZZER_PIN);
}
