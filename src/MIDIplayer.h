// MIDIplayer library header file
// ESP32
// Nemes Dániel

/*
	Encoding:
		-"c, C, d, D, e, f, F, g, G, a, A, b" ==> notes for _beatLength time
		-"numbers 0-6" ==> octave select with no delay
		-"-" ==> no note for _beatLength time
*/


#ifndef MIDIplayer_h
#define MIDIplayer_h

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

class MIDIplayer
{
  public:
    MIDIplayer(int outputPin, unsigned int BPM, BaseType_t core = 0, UBaseType_t priority = 1);
    void begin();
    void set_BPM(unsigned int BPM);
    void play_MIDI_string(const String& MIDI_string);
    void terminate();
    bool is_playing();

    // Mute / unmute: silence or restore the buzzer while the beat clock
    // keeps running. Unmuting mid-note re-enables the tone immediately.
    void mute();
    void unmute();
    bool is_muted();

    // Pause / resume: freeze playback at the current beat boundary. The
    // buzzer is silenced immediately. Resuming restores the beat clock
    // from the point where it was frozen so no beats are skipped.
    void pause();
    void resume();
    bool is_paused();

  private:
    int _outputPin;
    unsigned int _BPM;

    // _beatLength is a 32-bit aligned value. On the Xtensa LX6 (ESP32),
    // aligned 32-bit loads and stores are atomic, so volatile is sufficient
    // for safe cross-core visibility without a mutex.
    volatile unsigned int _beatLength;

    BaseType_t _core;
    UBaseType_t _priority;

    static const float _frequencies[85];

    unsigned int _octave;
    volatile bool _terminated;

    // Mute state flag. Changing it does not affect the beat clock.
    volatile bool _muted;

    // The frequency currently "assigned" to the output (regardless of
    // whether the buzzer is muted). 0.0 means a rest is active.
    // On the Xtensa LX6, aligned 32-bit (float) loads/stores are atomic,
    // so volatile is sufficient for safe cross-core reads in unmute().
    volatile float _currentFreq;

    // Pause state flag. The playback task checks this at every beat
    // boundary and blocks on _pauseSem when true.
    volatile bool _paused;

    String _midiString;
    volatile TaskHandle_t _taskHandle;

    // Binary semaphore used to synchronise terminate() with the playback task.
    // It is "given" (= 1) whenever no task is running, and "taken" (= 0) while
    // one is active. This eliminates the race between vTaskDelete() in
    // terminate() and vTaskDelete(nullptr) inside the task itself.
    SemaphoreHandle_t _doneSem;

    // Binary semaphore used as a "pause gate".
    // Normally "given" (= 1) — the playback loop takes and immediately
    // gives it back each beat, so it never blocks.
    // pause() "takes" it (drives it to 0); the loop blocks at the gate.
    // resume() "gives" it back (drives it to 1); the loop unblocks.
    SemaphoreHandle_t _pauseSem;

    static void _playTask(void* pvParameters);
    void _playLoop();
};

#endif