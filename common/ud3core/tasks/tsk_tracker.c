// TODO: CLI-Integration für Tracker-Task (Start/Stop/Status)
// TODO: Schnittstelle zu globalen CLI-Kommandos bereitstellen
// TODO: Task-Status und Fehlerausgabe

#include "FreeRTOS.h"
#include "task.h"
#include "tracker.h"
#include "cli_common.h"

void tsk_tracker_Start(void) {
    tracker_start();
}
