// MIDIplayer library source file
// ESP32
// Nemes Dániel


#include <Arduino.h>
#include <MIDIplayer.h>


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants

// All the notes frequencies
const float MIDIplayer::_frequencies[85] = {32.703, 34.648, 36.708, 38.891, 41.203, 43.654, 46.249, 48.999, 51.913, 55, 58.913, 61.735, 65.406, 69.296, 73.416, 77.782, 82.407, 87.307,92.499, 97.999, 103.826, 110, 116.471, 123.471, 130.813, 138.591, 146.832, 155.563, 164.814, 174.614, 184.997, 195.998, 207.652, 220, 233.082, 246.942, 261.626, 277.183, 293.665, 311.127, 329.628, 349.228, 369.994, 391.995, 415.305, 440, 466.164, 493.883, 523.251, 554.365, 587.33, 622.254, 659.255, 698.456, 739.989, 783.991, 830.609, 880, 932.328, 987.767, 1046.502, 1108.731, 1174.659, 1244.508, 1318.51, 1396.913, 1479.978, 1567.982, 1661.219, 1760, 1864.655, 1975.533, 2093.005, 2217.461, 2349.318, 2489.016, 2637.021, 2793.826, 2959.955, 3135.964, 3322.438, 3520, 3729.31, 3951.066, 4186.009};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup functions

// Constructor function
MIDIplayer::MIDIplayer(int outputPin, unsigned int BPM, BaseType_t core, UBaseType_t priority)
{
  _outputPin  = outputPin;
  _BPM        = BPM;
  _beatLength = 60000 / _BPM;
  _core       = core;
  _priority   = priority;
  _terminated = true;
  _taskHandle = nullptr;

  // Create the completion semaphore and leave it in the "given" (idle) state.
  // On ESP32 the FreeRTOS heap is available before setup(), so this is safe
  // even when the object is a global.
  _doneSem = xSemaphoreCreateBinary();
  configASSERT(_doneSem != nullptr);
  xSemaphoreGive(_doneSem);
}

// Initializer function
void MIDIplayer::begin()
{
  pinMode(_outputPin, OUTPUT);
  _beatLength = 60000 / _BPM;
}

// Set BPM after initialization
void MIDIplayer::set_BPM(unsigned int BPM)
{
  _BPM = BPM;
  // Atomic 32-bit write on Xtensa LX6 – visible to the playback core via volatile.
  _beatLength = 60000 / _BPM;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Music functions

// Play a MIDI string on a separate core using FreeRTOS
void MIDIplayer::play_MIDI_string(const String& MIDI_string)
{
  // Stop any currently running playback and wait for its task to exit cleanly.
  terminate();

  _midiString = MIDI_string;
  _terminated = false;
  _octave     = 4;             // sensible default octave

  // Take the semaphore to mark the player as busy before creating the task,
  // so that a terminate() call that arrives immediately after play_MIDI_string()
  // returns finds the semaphore already taken and waits correctly.
  xSemaphoreTake(_doneSem, 0);

  // xTaskCreatePinnedToCore expects TaskHandle_t* but _taskHandle is volatile,
  // so passing &_taskHandle directly is a [-fpermissive] error. Use a plain
  // local, then store the result into the volatile member.
  TaskHandle_t h = nullptr;
  xTaskCreatePinnedToCore(
    _playTask,       // task function
    "MIDITask",      // name (for debugging)
    4096,            // stack size in bytes - String ops need a bit of headroom
    this,            // pass instance pointer as parameter
    _priority,       // priority
    &h,              // receives the new handle
    _core            // core 0 or 1
  );
  _taskHandle = h;
}

// Terminate currently playing MIDI string
//
// Design: setting _terminated asks the task to exit at the next beat boundary.
// We then wait up to (_beatLength + 100) ms for it to signal completion via
// _doneSem. If it does not respond in time (it is blocked inside
// vTaskDelayUntil) we force-delete it — FreeRTOS safely removes a task from
// any wait list. In the clean-exit path the task suspends itself and we delete
// the suspended task. Both paths converge on the same vTaskDelete() call so
// there is no double-delete race.
void MIDIplayer::terminate()
{
  _terminated = true;
  noTone(_outputPin);

  TaskHandle_t h = _taskHandle;
  if (h == nullptr) return;   // nothing is playing

  // Wait for the task to signal completion. The timeout covers the worst case:
  // the task is sleeping inside vTaskDelayUntil for a full beat.
  xSemaphoreTake(_doneSem, pdMS_TO_TICKS(_beatLength + 100));

  // Whether we timed out (task is mid-delay) or the task signalled cleanly
  // (task is suspended), vTaskDelete() is safe in both cases:
  //  - Mid-delay:  FreeRTOS removes the task from the delay list.
  //  - Suspended:  Deleting a suspended task is always safe.
  vTaskDelete(h);
  _taskHandle = nullptr;
  xSemaphoreGive(_doneSem);   // restore to idle state for the next call
}

// Determine if there is a MIDI string playing currently
bool MIDIplayer::is_playing()
{
  return (_taskHandle != nullptr && !_terminated);
}

// Static trampoline — FreeRTOS needs a plain function pointer,
// so we bounce into the instance method via the void* parameter.
void MIDIplayer::_playTask(void* pvParameters)
{
  MIDIplayer* self = static_cast<MIDIplayer*>(pvParameters);
  self->_playLoop();

  // Signal that playback has finished, then suspend.
  // We deliberately do NOT call vTaskDelete(nullptr) here: doing so would race
  // with terminate() calling vTaskDelete(h) on the same handle. By suspending
  // instead, we leave ourselves in a well-defined state and let terminate() be
  // the single owner of task deletion.
  xSemaphoreGive(self->_doneSem);
  vTaskSuspend(nullptr);   // terminate() will vTaskDelete us
}

// Play MIDI string
void MIDIplayer::_playLoop()
{
  int noteID = 0;
  int length = _midiString.length();

  // Anchor point for drift-free beat timing
  TickType_t xLastWakeTime = xTaskGetTickCount();

  for (int i = 0; i < length; i++)
  {
    if (_terminated) break;

    char letter = _midiString.charAt(i);

    switch (letter)
    {
      case '0': _octave = 0; noteID = 2000; break;
      case '1': _octave = 1; noteID = 2000; break;
      case '2': _octave = 2; noteID = 2000; break;
      case '3': _octave = 3; noteID = 2000; break;
      case '4': _octave = 4; noteID = 2000; break;
      case '5': _octave = 5; noteID = 2000; break;
      case '6': _octave = 6; noteID = 2000; break;
      case ' ': noteID = 2000; break;

      case 'c': noteID = (_octave * 12) + 0;  break;
      case 'C': noteID = (_octave * 12) + 1;  break;
      case 'd': noteID = (_octave * 12) + 2;  break;
      case 'D': noteID = (_octave * 12) + 3;  break;
      case 'e': noteID = (_octave * 12) + 4;  break;
      case 'f': noteID = (_octave * 12) + 5;  break;
      case 'F': noteID = (_octave * 12) + 6;  break;
      case 'g': noteID = (_octave * 12) + 7;  break;
      case 'G': noteID = (_octave * 12) + 8;  break;
      case 'a': noteID = (_octave * 12) + 9;  break;
      case 'A': noteID = (_octave * 12) + 10; break;
      case 'b': noteID = (_octave * 12) + 11; break;

      case '-': noteID = 1000; break;  // rest
      default:  noteID = 2000; break;  // unrecognized --> no-op, no beat consumed
    }

    
    if (noteID < 85)
    {
      tone(_outputPin, _frequencies[noteID]);  // play note
    }
    else if (noteID == 1000)
    {
      noTone(_outputPin);                      // rest: silence output for one beat
    }
    // noteID == 2000 (octave select / space): leave output unchanged

    // Only notes and rests advance the clock; octave selectors and spaces are
    // instantaneous so the beat grid is never disrupted by them.
    if (noteID <= 1000)
    {
      // vTaskDelayUntil keeps beats locked to a fixed grid:
      // any processing time inside the loop is automatically compensated.
      vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(_beatLength));
    }
  }

  noTone(_outputPin);
  _terminated = true;
}