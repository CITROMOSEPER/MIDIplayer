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

    String _midiString;
    volatile TaskHandle_t _taskHandle;

    // Binary semaphore used to synchronise terminate() with the playback task.
    // It is "given" (= 1) whenever no task is running, and "taken" (= 0) while
    // one is active. This eliminates the race between vTaskDelete() in
    // terminate() and vTaskDelete(nullptr) inside the task itself.
    SemaphoreHandle_t _doneSem;

    static void _playTask(void* pvParameters);
    void _playLoop();
};

#endif