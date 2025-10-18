#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define BUZZER_PIN 1

#define NUM_FREQS 37 // Range from G3 to F#5
const unsigned int TONE_FREQS[NUM_FREQS] = {
    196, 208, 220, 233, 247, 262, 277, // G3
    294, 311, 330, 349, 370, 392, 415, // D4
    440, 466, 494, 523, 554, 587, 622, // A4
    659, 698, 740, 784, 831, 880, 932, // E5
    988, 1047, 1109, 1175, 1245, 1319, 1397, // B5
    1480, 1568, // G6
};

const char *TONE_NAMES[12] = {"G", "Ab", "A", "Bb", "B", "C", "C#", "D", "D#", "E", "F", "F#"};

typedef struct {
    int freqIndex;
    unsigned int noteLength;
} Note;

typedef struct {
    const char *name;
    Note *notes;
    int numNotes;
    int period;
    int barPeriod;
    int minFreqIndex;
    int maxFreqIndex;
} Song_t;

Note megalovaniaNotes[] = {
{7,1},{7,1},{19,2},{14,2},{-1,1},{13,1},{-1,1},{12,1},{-1,1},{10,2},{7,1},{10,1},{12,1},
{5,1},{5,1},{19,2},{14,2},{-1,1},{13,1},{-1,1},{12,1},{-1,1},{10,2},{7,1},{10,1},{12,1},
{4,1},{4,1},{19,2},{14,2},{-1,1},{13,1},{-1,1},{12,1},{-1,1},{10,2},{7,1},{10,1},{12,1},
{3,1},{3,1},{19,2},{14,2},{-1,1},{13,1},{-1,1},{12,1},{-1,1},{10,2},{7,1},{10,1},{12,1},
};

Note legendPreludeNotes[] = {
{-1,2},{17,1},{16,1},{17,1},{19,1},{21,2},
{21,1},{19,1},{21,1},{22,1},{24,4},
{-1,2},{26,1},{24,1},{26,1},{28,1},{29,2},
{28,4},{24,4},
{22,2},{21,2},{22,2},{26,2},
{24,2},{22,2},{21,2},{17,2},
{19,4},{17,2},{21,2},
{19,4},{21,4},
{-1,8},
};

Note legend1Notes[] = {
{2,2},{5,1},{9,1},{17,2},{16,2},
{12,2},{7,2},{9,4},
{9,2},{10,1},{12,1},{10,2},{9,2},
{7,2},{5,2},{9,4},

{2,2},{5,1},{9,1},{17,2},{16,2},
{12,2},{7,2},{9,4},
{9,2},{10,1},{12,1},{10,2},{9,2},
{7,2},{5,2},{9,4},

{14,2},{17,1},{21,1},{29,2},{28,2},
{24,2},{19,2},{21,4},
{21,2},{22,1},{24,1},{22,2},{21,2},
{19,2},{17,2},{21,4},

{14,2},{17,1},{21,1},{29,2},{28,2},
{24,2},{19,2},{21,4},
{33,2},{34,1},{36,1},{34,2},{33,2},
{31,2},{29,2},{33,4},
};

Note legend2Notes[] = {
{-1,4},{17,2},{16,2},{17,2},{19,2},{21,4},
{21,3},{19,1},{21,3},{22,1},{24,4},{26,2},{28,2},
{29,6},{31,2},{29,6},{28,2},
{24,16},

{22,2},{21,2},{22,2},{26,2},{24,2},{22,2},{21,2},{22,2},
{24,4},{21,2},{19,2},{21,4},{17,2},{21,2},
{22,6},{21,2},{19,4},{17,4},
{16,8},{21,8},
};

Note legend3Notes[] = {
{-1,2},{28,1},{29,1},{28,1},{26,1},
{21,2},{17,2},{21,2},
{-1,2},{28,1},{29,1},{28,1},{26,1},
{31,6},
{-1,2},{28,1},{29,1},{28,1},{26,1},
{28,4},{29,2},
{24,2},{24,1},{22,1},{21,1},{22,1},
{21,6},
{-1,2},{21,1},{22,1},{21,1},{19,1},
{15,2},{10,2},{22,2},
{21,3},{19,1},{17,1},{19,1},
{21,4},{17,2},
{20,2},{16,2},{14,2},
{8,4},{20,2},
{21,12},
{-1,6},
};

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
    static Song_t Megalovania = {"MEGALOVANIA", megalovaniaNotes, 52, 125, 32, 0, 22};
    static Song_t TheLegend0 = {"THE LEGEND 0", legendPreludeNotes, 33, 300, 32, 0, 36};
    static Song_t TheLegend1 = {"THE LEGEND 1", legend1Notes, 64, 300, 32, 0, 36};
    static Song_t TheLegend2 = {"THE LEGEND 2", legend2Notes, 38, 150, 32, 0, 36};
    static Song_t TheLegend3 = {"THE LEGEND 3", legend3Notes, 48, 200, 24, 0, 36};
    playSong(TheLegend0);
    playSong(TheLegend1);
    playSong(TheLegend2);
    playSong(TheLegend3);
}

void playSong(Song_t song)
{
    const int N = song.numNotes;
    const int dx = 320/song.barPeriod;
    const int dy = 150/(song.maxFreqIndex - song.minFreqIndex);
    const int pause = 10;

    int freq, n, T;
    int bars = 0, drawTime = millis();
    tft.drawFastHLine(0, 20, 320, TFT_WHITE);
    for (int i = 0, j = 0; i < N; i++) {
        n = song.notes[i].freqIndex;
        T = song.notes[i].noteLength * song.period;
        tft.setCursor(0, 0);
        if (bars % (song.barPeriod) == 0) {
            tft.fillRect(0, 21, 320, 149, TFT_BLACK); 
            j = 0; 
        }
        if (n > 0) {
            freq = TONE_FREQS[n];
            tone(BUZZER_PIN, freq);
            tft.drawFastHLine(j*dx, 169-dy*(n-song.minFreqIndex), dx*(T/song.period)-2, TFT_GOLD);
            tft.printf("%02d/%2d:%-2s %-4d %s", i+1, N, TONE_NAMES[n % 12], freq, song.name);
            delay(T-pause);
        } else {
            noTone(BUZZER_PIN);
            tft.drawFastHLine(j*dx, 169, dx*(T/song.period)-2, 0x2104);
            tft.printf("%02d/%2d:-- ---- %s", i+1, N, song.name);
            delay(T-pause);
        }
        bars += song.notes[i].noteLength;
        j += T/song.period;
        delay(constrain(pause - (millis() - drawTime), 0, pause));
        drawTime = millis();
    }
    noTone(BUZZER_PIN);
}
