# MIDIplayer

An Arduino library for the **ESP32** that plays melodies through a piezo buzzer (or any PWM-capable output) using a compact text-based note encoding. Playback runs on a dedicated FreeRTOS task so your main loop stays fully responsive.

---

## Features

- Simple string-based melody encoding — no binary MIDI files needed
- Non-blocking playback via a pinned FreeRTOS task
- Drift-free timing using `vTaskDelayUntil`
- Configurable BPM, task core, and task priority
- 85-note frequency table (C1 – C8)
- Runtime BPM changes and full playback control (`terminate`, `is_playing`, `mute`, `unmute`, `pause`, `resume`)

---

## Hardware Requirements

- **ESP32** (any variant with the Arduino-ESP32 core)
- A piezo buzzer or speaker connected to any GPIO pin that supports `tone()`

---

## Installation

Via the Arduino IDE Library Manager: search for MIDIplayer and click Install.

Manual install:

1. Download or clone this repository.
2. Copy the folder into your Arduino libraries/ directory.
3. Restart the Arduino IDE.

---

## Note Encoding

Melodies are passed as plain `String` values. Each character represents either a note, a rest, or an octave selector.
The site [Piano Letter Notes](https://pianoletternotes.blogspot.com/) has many such sheet music available in a compatible format.

| Character | Meaning |
|-----------|---------|
| `c` | C (natural) |
| `C` | C# |
| `d` | D (natural) |
| `D` | D# |
| `e` | E |
| `f` | F (natural) |
| `F` | F# |
| `g` | G (natural) |
| `G` | G# |
| `a` | A (natural) |
| `A` | A# |
| `b` | B |
| `-` | Rest (silence for one beat) |
| `0` – `6` | Select octave (no beat consumed) |
| `space` | No-op / separator (no beat consumed) |

Every **note** and every **rest** advances the clock by exactly one beat (`60 000 ms / BPM`). Octave selectors and spaces are instantaneous — they take no time.

### Octave reference

| Number | Octave | Range |
|--------|--------|-------|
| `0` | 0 | C1 – B1 |
| `1` | 1 | C2 – B2 |
| `2` | 2 | C3 – B3 |
| `3` | 3 | C4 – B4 |
| `4` | 4 | C5 – B5 (default) |
| `5` | 5 | C6 – B6 |
| `6` | 6 | C7 – B7 |

---

## API

### Constructor

```cpp
MIDIplayer player(outputPin, BPM, core, priority);
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `outputPin` | `int` | — | GPIO pin connected to the buzzer |
| `BPM` | `unsigned int` | — | Tempo in beats per minute |
| `core` | `BaseType_t` | `0` | FreeRTOS core to run playback on |
| `priority` | `UBaseType_t` | `1` | FreeRTOS task priority |

### Methods

```cpp
void begin();
```
Configures the output pin and calculates the initial beat length. Call once in `setup()`.

---

```cpp
void set_BPM(unsigned int BPM);
```
Updates the tempo. Takes effect at the next note played.

---

```cpp
void play_MIDI_string(const String& MIDI_string);
```
Starts playback of the given melody string on a background FreeRTOS task. If a melody is already playing it is stopped first. The mute state is preserved across calls so you can pre-mute before starting playback.

---

```cpp
void terminate();
```
Stops playback immediately, silences the output, and deletes the background task. Safe to call while paused.

---

```cpp
bool is_playing();
```
Returns `true` while a melody is actively playing, including while it is paused.

---

```cpp
void mute();
```
Silences the buzzer immediately without affecting the beat clock. The melody continues to advance in the background — notes and rests are still processed and timed normally, just with no sound output. Calling `unmute()` while a note is mid-beat resumes that note's tone immediately with no gap.

---

```cpp
void unmute();
```
Restores sound. If a note (not a rest) is currently being played it is re-enabled on the buzzer straight away so the remainder of that beat is audible. Has no effect if nothing is playing.

---

```cpp
void pause();
```
Freezes playback and silences the buzzer. The buzzer is silenced immediately; the internal beat clock stops at the next beat boundary (at most one beat later). Calling `resume()` continues from exactly where the melody left off — no beats are skipped and no double-notes are played.

---

```cpp
void resume();
```
Continues a paused melody. The beat clock re-anchors to the moment of resumption so `vTaskDelayUntil` does not try to catch up on time elapsed during the pause. Has no effect if playback is not paused.

---

```cpp
bool is_paused();
```
Returns `true` if the melody is active but currently frozen by `pause()`.

---

## Examples

### Simple scale

```cpp
#include <MIDIplayer.h>

// Buzzer on pin 25, 120 BPM, run on core 0
MIDIplayer player(25, 120);

void setup() {
  player.begin();
  // Play C-major scale in octave 4
  player.play_MIDI_string("4cdefgab5c");
}

void loop() {
  // Main loop is free to do other work
}
```

### Jingle Bells (opening phrase)

```cpp
#include <MIDIplayer.h>

MIDIplayer player(25, 160);

// e e e  e e e  e g c d e
const String jingleBells = "4eee-eee-egcde";

void setup() {
  player.begin();
  player.play_MIDI_string(jingleBells);
}

void loop() {
  if (!player.is_playing()) {
    delay(1000);
    player.play_MIDI_string(jingleBells);  // loop the melody
  }
}
```

### Changing tempo at runtime

```cpp
#include <MIDIplayer.h>

MIDIplayer player(25, 120);

const String melody = "4cdeccdec";

void setup() {
  player.begin();
  player.play_MIDI_string(melody);

  delay(3000);
  player.set_BPM(200);          // speed up mid-song
  player.play_MIDI_string(melody);
}

void loop() {}
```

### Mute / unmute with a button

```cpp
#include <MIDIplayer.h>

MIDIplayer player(25, 120);

const String melody = "4cdefgab5c4bagfedce";
const int buttonPin = 0;  // BOOT button on most ESP32 dev boards

void setup() {
  player.begin();
  pinMode(buttonPin, INPUT_PULLUP);
  player.play_MIDI_string(melody);
}

void loop() {
  // Toggle mute on button press (simple debounce omitted for brevity)
  if (digitalRead(buttonPin) == LOW) {
    if (player.is_paused()) return;   // ignore while paused
    if (player.is_playing()) {
      // Determine current mute state by attempting to mute; check via
      // a small helper flag in your own sketch for a real implementation.
      player.mute();
    }
    delay(300);
  }
}
```

### Pause / resume with a button

```cpp
#include <MIDIplayer.h>

MIDIplayer player(25, 120);

const String melody = "4cdefgab5c4bagfedce";
const int buttonPin = 0;

void setup() {
  player.begin();
  pinMode(buttonPin, INPUT_PULLUP);
  player.play_MIDI_string(melody);
}

void loop() {
  if (digitalRead(buttonPin) == LOW) {
    if (player.is_paused()) {
      player.resume();   // continue from where it stopped
    } else if (player.is_playing()) {
      player.pause();    // freeze; buzzer goes silent immediately
    }
    delay(300);          // simple debounce
  }
}
```

### Mute while paused

```cpp
// Mute and pause are fully independent — you can apply them in any order.
player.play_MIDI_string("4cdefgab5c");
delay(1000);
player.pause();    // freeze and silence
delay(2000);
player.mute();     // enable mute so the song resumes silently
player.resume();   // beat clock restarts; notes advance but buzzer stays off
delay(1000);
player.unmute();   // sound comes back mid-note
```

---

## Notes & Limitations

- Only one melody can play at a time. Calling `play_MIDI_string()` while a melody is running will stop the current one first.
- All notes are the same length (one beat). There is no per-note duration modifier.
- The frequency table covers C1 – C8 (85 notes). Notes outside this range are silently ignored.
- `pause()` silences the buzzer immediately. Internally the playback task stops advancing at the next beat boundary (at most one full beat after `pause()` is called), so the timing of `resume()` is correct regardless of when within a beat `pause()` was invoked.
- `mute()` does not stop the beat clock. If you mute and unmute quickly within one beat, the note is heard only for the remaining portion of that beat.
- `terminate()` is safe to call while paused or muted.
- Playback runs on a separate FreeRTOS task. Avoid accessing the `MIDIplayer` object from multiple tasks simultaneously without synchronisation.
- Designed and tested on the **ESP32**. Not compatible with AVR-based boards (Uno, Mega, etc.) as it relies on FreeRTOS.

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

## Author

**Nemes Dániel**