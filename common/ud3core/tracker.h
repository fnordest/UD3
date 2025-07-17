// TODO: CLI-API für Tracker-Transport und Status
// TODO: Funktionsprototypen für CLI-Kommandos (z.B. tracker_start, tracker_stop, tracker_status, tracker_live_note, ...)
// TODO: Erweiterung für Pattern/Instrumentenverwaltung

#ifndef TRACKER_H
#define TRACKER_H


#ifndef TRACKER_H
#define TRACKER_H

#include <stdint.h>

typedef struct {
    uint8_t running;
    uint8_t song;
    uint8_t pattern;
    uint8_t mode;
} tracker_status_t;

void tracker_init(void);
void tracker_start(void);
void tracker_stop(void);
void tracker_process_tick(void);
void tracker_set_mode(uint8_t mode);
void tracker_set_song(uint8_t idx);
void tracker_set_pattern(uint8_t idx);
void tracker_live_note(uint8_t note, uint8_t velocity);
tracker_status_t tracker_get_status(void);

#endif // TRACKER_H

#endif // TRACKER_H
