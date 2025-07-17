// TODO: Transport- und Playback/Live-Modus-Steuerung über CLI zugänglich machen
// TODO: CLI-Befehle für Start, Stop, Pause, Songauswahl, Pattern-Edit, Live-Note, etc. implementieren
// TODO: Tracker-Statusabfrage und Debug-Ausgaben über CLI ermöglichen
// TODO: Integration mit MIDI-Input und MPE
// TODO: Song-/Pattern-/Instrumentenverwaltung (Laden/Speichern)
// TODO: Playback-Engine vervollständigen (Pattern-Stepping, Effekte, SID-Frame-Generierung)
// TODO: Live-Modus (direktes Triggern von Noten/Parametern über CLI/MIDI)
// TODO: Dokumentation und Beispiel-CLI-Kommandos

#include "tracker.h"
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "SidProcessor.h"
#include "cli_common.h"

// --- Tracker Data Structures (GoatTracker-inspired) ---
#define NUM_CHANNELS 3
#define PATTERN_ROWS 64
#define MAX_PATTERNS 128
#define MAX_ORDER_LENGTH 256
#define MAX_INSTRUMENTS 32
#define ENVELOPE_MACRO_LEN 32
#define WAVEFORM_MACRO_LEN 32
#define PULSE_MACRO_LEN    32
#define FILTER_MACRO_LEN   32

// Pattern step
typedef struct {
    uint8_t note;         // 0 = rest, 1-96 = note
    uint8_t instrument;   // instrument index
    uint8_t effect;       // effect command
    uint8_t param;        // effect parameter
} PatternStep;

// Pattern
typedef struct {
    PatternStep steps[PATTERN_ROWS][NUM_CHANNELS];
} Pattern;

// Song order
typedef struct {
    uint8_t order[MAX_ORDER_LENGTH];
    uint8_t length;
} SongOrder;

// Instrument macros
typedef struct {
    uint8_t envelope[ENVELOPE_MACRO_LEN];
    uint8_t waveform[WAVEFORM_MACRO_LEN];
    uint8_t pulse[PULSE_MACRO_LEN];
    uint8_t filter[FILTER_MACRO_LEN];
    uint8_t envelopeLen, waveformLen, pulseLen, filterLen;
    uint8_t attack, decay, sustain, release;
    uint8_t filterType, resonance, cutoff;
} Instrument;

// Song container
typedef struct {
    Pattern patterns[MAX_PATTERNS];
    SongOrder order;
    Instrument instruments[MAX_INSTRUMENTS];
} TrackerSong;

// Playback state
typedef struct {
    uint8_t currentOrder;
    uint8_t currentRow;
    uint8_t tick;
    struct {
        uint8_t note;
        uint8_t instrument;
        uint8_t effect;
        uint8_t param;
        uint8_t envStep, wavStep, pulseStep, filterStep;
        uint8_t sidReg[7];
    } channel[NUM_CHANNELS];
} TrackerState;

// SID frame
typedef struct {
    uint8_t reg[25];
} SIDFrame;

// --- Tracker Task API ---
void tracker_init(void);
void tracker_start(void);
void tracker_process_tick(void);

// --- Internal State ---
static TrackerSong trackerSong;
static TrackerState trackerState;
static QueueHandle_t qSID = NULL;


// --- Minimal working core ---
typedef struct {
    uint8_t running;
    uint8_t song;
    uint8_t pattern;
    uint8_t mode;
} tracker_status_t;

static tracker_status_t tracker_status;

void tracker_init(void) {
    memset(&trackerSong, 0, sizeof(trackerSong));
    memset(&trackerState, 0, sizeof(trackerState));
    tracker_status.running = 0;
    tracker_status.song = 0;
    tracker_status.pattern = 0;
    tracker_status.mode = 0;
    // Minimal demo: fill pattern 0, channel 0 with C-4 notes
    for (int i = 0; i < PATTERN_ROWS; ++i) {
        trackerSong.patterns[0].steps[i][0].note = 60; // MIDI C-4
        trackerSong.patterns[0].steps[i][0].instrument = 1;
        trackerSong.patterns[0].steps[i][0].effect = 0;
        trackerSong.patterns[0].steps[i][0].param = 0;
    }
    trackerSong.order.order[0] = 0;
    trackerSong.order.length = 1;
}


// Minimal pattern stepping and note triggering
void tracker_process_tick(void) {
    if (!tracker_status.running) return;
    uint8_t pat_idx = trackerSong.order.order[tracker_status.pattern];
    Pattern* pat = &trackerSong.patterns[pat_idx];
    PatternStep* step = &pat->steps[trackerState.currentRow][0];
    if (step->note) {
        // Minimal: trigger note as pulse
        SigGen_pulseData_t pulse = {0};
        pulse.current = 100; // fixed value for demo
        pulse.onTime = 1000; // fixed duration
        pulse.period = 1000; // fixed period
        SignalGenerator_addPulse(&pulse);
    }
    // Advance row
    trackerState.currentRow++;
    if (trackerState.currentRow >= PATTERN_ROWS) {
        trackerState.currentRow = 0;
        tracker_status.pattern++;
        if (tracker_status.pattern >= trackerSong.order.length) {
            tracker_status.pattern = 0; // loop
        }
    }
}


void tracker_task(void *pvParameters) {
    tracker_init();
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t tickRate = pdMS_TO_TICKS(20); // 50Hz
    while (1) {
        tracker_process_tick();
        vTaskDelayUntil(&lastWakeTime, tickRate);
    }
}


void tracker_start(void) {
    tracker_status.running = 1;
    tracker_status.pattern = 0;
    trackerState.currentRow = 0;
    xTaskCreate(tracker_task, "Tracker", configMINIMAL_STACK_SIZE + 256, NULL, 3, NULL);
}

void tracker_stop(void) {
    tracker_status.running = 0;
}

void tracker_set_mode(uint8_t mode) {
    tracker_status.mode = mode;
}

void tracker_set_song(uint8_t idx) {
    tracker_status.song = idx;
    tracker_status.pattern = 0;
    trackerState.currentRow = 0;
}

void tracker_set_pattern(uint8_t idx) {
    tracker_status.pattern = idx;
    trackerState.currentRow = 0;
}

void tracker_live_note(uint8_t note, uint8_t velocity) {
    if (tracker_status.mode == 1) {
        SigGen_pulseData_t pulse = {0};
        pulse.current = velocity;
        pulse.onTime = 1000;
        pulse.period = 1000;
        SignalGenerator_addPulse(&pulse);
    }
}

tracker_status_t tracker_get_status(void) {
    return tracker_status;
}
