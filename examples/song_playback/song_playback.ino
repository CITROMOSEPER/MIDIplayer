// MIDIplayer library demonstration
// ESP32
// Nemes Dániel
//
// This file demonstrates the MIDIplayer library's capabilities


#define BUZZER_PIN 21     // GPIO pin the PASSIVE piezo buzzer is connected to
#define TERMINATE_PIN 14  // GPIO pin to terminate the music

#include <MIDIplayer.h>
MIDIplayer MIDI(BUZZER_PIN, 220);  // This is the constructor function. Its arguments are: buzzer pin, starting BPM. for advanced users there are two extra: core, priority, but these are not required by default.


//https://pianoletternotes.blogspot.com/2018/06/doom-theme.html   <--   Where DOOM E1M1 sheet music is taken from. This site has many other music compatible with this format.
String DOOM_E1M1 = 
  "3DD4D3DD4C3DDbDDaaDAbDD4D3DD4D3DDbD"
  "3DDaaaaDD4D3DD4C3DDbDDaDDAbDD4D3D"
  "3D4C3DDbDDaaaaaaD4D3DD4C3DDbDDaDD"
  "3AbDD4D3DD4C3DDbDDaaaaaDD4D3DD4C3DD"
  "3bDDaDDAbDD4D3DD4C3DD5dGffGGf3DD"
  "4D3DD4C3DDbDDaDDAbDD4D3DD4C3DDbDDa"
  "3aaaaDD4D3DD4C3DDbDDaDDAbDD4D3DD4C3"
  "3DD5AFFFAFFF3GG4G3GG4F3GG4e3GG4d3GG4De"
  "3GG4G3GG4F3GG4e3GG4ddddd3GG4G3GG4F3GG4e3G"
  "3G4d3GG4De3GG4G3GG4F3GGG4bbGGbbb3DDDD"
  "3D4C3DDbDDaDDAbDD4D3DD4C3DDbDDaaa"
  "3aaDD4D3DD4C3DDbDDaDDAbDD4D3DD4C3DD"
  "3bDDaaaaa4cccccAccGccFccgG3AA"
  "4A3AA4G3AA4F3aG4eeeee3DD4D3DD4C3DDbDDa"
  "3DDAbDD4D3DD4C3DD5DDDFFFDA3DDDDDD"
  "3DDbDDaDDAbDD4D3DD4C3DDbDDDaaaa"
  "3DD4D3DD4C3DDbDDaDDAbDD4D3DD4C3DD4bA"
  "4AAAAAA3DD4F3DD4f3DD4d3DDDDD4CD3DD4F3D"
  "3D4f3DD4D3DDD4cccc3DD4F3DD4f3DD4d3DD4CCC"
  "CC3DD4F3DD4f3DD5AFDADAAF3GG4b3GG4A3GG"
  "4g3GG4F3GG4FG3GG4b3GG4A3GG4G3GGG4FFFF3GG"
  "4b3GG4A3GG4g3GG4F3GG4FG3GG4b3GG4A3GGG4bbb"
  "G4bbb3DD4FDD4f3DD4d3DD4C3DD4CD3DD4F3DD4f"
  "3DD4D3DDD4cccc3DD4F3DD4f3DD4d3DDDDD4CD"
  "3DD4F3DD4f3DD4D3DD4ccccccc5F4cc5f4cc5D4c"
  "4c5c4cc5cD3AA5f3AA5D3AA5d3aG4AAAAA3DD4F3D"
  "3D4f3DD4d3DD4C3DD4CD3DD4F3DD4f3DDDD4Af3AA"
  "4AGddD--"
;

//https://pianoletternotes.blogspot.com/2017/10/tetris-theme-by-korobeiniki.html    <--   Where TETRIS sheet music is taken from. This site has many other music compatible with this format.
String TETRIS_OST =
  "5eeee4bb5ccddedcc4bbaaaaaa5ccec"
  "5ccddcc4bbbbbb5ccddddeeeecccc"
  "4aaaaaaaaaaaa5ddddddffaaaagg"
  "5ffeeeeeecceeeeddcc4bbbbbb5cc"
  "5ddddeeeecccc4aaaaaaaaaaaa5ee"
  "eebb5ccddedcc4bbaaaaaa5cceeee"
  "5ddcc4bbbbbb5ccddddeeeecccc4aa"
  "aaaaaaaaaa5ddddddffaaaaggff"
  "5eeeeeecceeeeddcc4bbbbbb5ccdd"
  "ddeeeecccc4aaaaaaaaaaaa5eeee"
  "eeee5ccccccccdddddddd4GGGGGG"
  "GG5cccccccc4aaaaaaaaGGGGGGGG"
  "4GGGGGGGG5eeeeeeeccccccccddd"
  "4ddddddGGGGGGGG5cccceeeeaaaa"
  "5aaaaGGGGGGGGGGGGGGGGeeee4GG"
  "5ccddedcc4GGeeeeaaa5ccccccddcc"
  "4GGeeGG5cc4bbbb5cccc4aaaaeeeeee"
  "eeeeee5ddddddffccccggffeeee"
  "4ggeeggggffeeGGeeGG5cc4bbbb5c"
  "cc4aaaaeeeeeeeeeeeebbbbGGaa"
  "4bb5ed4aaGGeeeeeeaa5cccc4bbaaGG"
  "4eeGG5cc4bbbb5cccc4aaaaeeeeeeee"
  "eeeeffffffaa5cccc4bbaagggggg"
  "eeggggffeeGGeeGG5cc4bbbb5cccc"
  "4aaaaeeeeeeeeeeeecccccccccc"
  "cccccc3bbbbbbbbGGGGGGGGaaaa"
  "3aaaaaaaaaaaaGGGGGGGGbbbbbb"
  "bb4ccccccccccccccccdddddddd"
  "3bbbbbbbb4ccccccccceeeeeeeeeee--"
;



void setup()
{
  pinMode(TERMINATE_PIN, INPUT);  // Initialize terminate pin as input (since it is not pulled low or high, it must always be connected to 3.3v OR GND for it to be not floating)
  MIDI.begin();                   // Initialize MIDIplayer
  MIDI.set_BPM(220);              // Set a BPM value for MIDIplayer (just for demonstration as this has already been done in the constructor)
}

void loop()
{
  // If not already playing and terminate key is not pressed, start playing music
  if ((!MIDI.is_playing()) && (digitalRead(TERMINATE_PIN) == LOW))
  {
    // At any one time only one of these can be used
    MIDI.play_MIDI_string(DOOM_E1M1);
    //MIDI.play_MIDI_string(TETRIS_OST);
  }

  // If terminate key is pressed, mute, and reset song playback
  if (digitalRead(TERMINATE_PIN) == HIGH)
  {
    MIDI.terminate();
  }
}
