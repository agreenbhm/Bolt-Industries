
// Written Feb 2022 by Brian DiDonna. 

//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation version 3.0.
//This firmware is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

// This sketch is for the BMK! Bolt (Industries) Mechanical keyboard.
// The assignment of keyboard columns and rows to pins is defined below.
// The wiring has diodes pointed from column to row.
// We set the columns to INPUT_HIGH to pull them to 5V. When a row is
// also HIGH and the key for that row and column is pressed then there
// is no voltage difference and nothing happens. If the row is set LOW and
// the key is pressed then is pulls the column to LOW and we can read it out
// for the column. We look for keypresses by setting one row to LOW and
// all others to HIGH and looking for a column that is LOW. We scan the
// LOW row to read the whole keyboard.

// If flashed exactly as it is to your Pico, this firmware will function as a regular
// keyboard with a few keys modified as shortcuts.
// You can change the function of any key to do whatever you'd like. Scroll to the
// bottom of the code for some suggested macros, and custom keys.

// to purchase a keyboard kit, visit www.boltind.com

//TO  INSTALL  THIS FIRMWARE

//This firmware must be installed through the Arduino IDE.
//You must first install a third party board. The RP  2040 support in the Arduino IDE ddoes
//not  include  support for the HID.h library that this sketch reqires. Fortunantly, there's a work around.
//You will need to install a third party board written by Earle F. Philhower, III from Github.
//You can read more about this third party board at: https://github.com/earlephilhower/arduino-pico

//In the Arduino IDE: go to File > preferances, and paste the following URL  under the additional boards manager URL's:
// https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

//Now, navigate over to the boards manager. Go to: Tools > Board > Board Manager. Search for "Pico"
//You will see "Raspberry Pi Pico/RP2040." Install this, it's the thrid party board you just added.
//Now, you should be able to select your newly installed Raspberry Pi Pico driver and upload your sketch!

#include <Keyboard.h>
#include <limits.h>

// Uncomment this to switch the modifier keys used for macros and other OS dependent behavior
// When MACOSX is defined we send Command-c to copy instead of Control-c, etc.
//#define MACOSX

// assign the collums to pins.
#define Col_0 0
#define Col_1 1
#define Col_2 2
#define Col_3 3
#define Col_4 4
#define Col_5 5
#define Col_6 6
#define Col_7 7
#define Col_8 8
#define Col_9 9
#define Col_10 10
#define Col_11 11
#define Col_12 12
#define Col_13 13
#define Col_14 14
#define Col_15 15
#define Col_16 16
#define Col_17 17

// assign the rows to pins.
#define Row_0 18
#define Row_2 19
#define Row_3 20
#define Row_4 21
#define Row_5 22
#define Row_6 26

// For the caps lock
#define LED_1 27
#define LED_2 28

// The pin numbers of rows 2 through 6 do not increase linearly. This vector
// lets us loop over those indice efficiently
const int rowIndices[ 5 ] = { Row_2, Row_3, Row_4, Row_5, Row_6 };

// These are the times before a key is repeated. There is an initial long time then after that a shorter time so they repeat faster
const int repeatDelayInitial = 300;
const int repeatDelayRepeat = 40;

//Adding a slight delay before sendding the key press helps with the modifier keys or key sequences. Without it, The
//keypresses tend to be missed.
const int preDelay = 10;

//The key needs to be pressed for at least this long to register. This could be set to zero but that might allow unintended keypresses
const int minimumKeypressDelay = 10;

// This is the mapping from row, column to key for rows 2 through 6. If you want to override a key, just enter the new key here.
// If you want to write your own special handling, enter a 0x00 here to skip the key loop then write your special handling elsewhere.
// 
//                                     0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17
const char sKeysForRows[5][18] = { {  '7',  '8',  '9', 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                   {  '4',  '5',  '6', 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                   {  '1',  '2',  '3', 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                                   {  '0', 0x00, 0xD4, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                   { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

// This is the mapping from column to key for the top function keys (Rw_0). If you want to override a key, just enter the new key here.
// If you want to write your own special handling, enter a 0x00 here to skip the key loop then write your special handling elsewhere.
// Note that this is simply the top row of keys on the Pico 14 macro pad.
//                                 0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15    16    17
const char sKeysForRowsFn[18] = { 0x00, '/', '*', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Here we keep track of the last time the various keys were pressed. This tracks rows 2 through 6. This is
// used to control the timing of key repeats
unsigned long sLastPress[5][18];
unsigned long sLastClear[5][18];

// Global flags for whether modifier keys are active
static bool sBoolShift = false;
static bool sBoolCtrl = false;
static bool sBoolAlt = false;
static bool sBoolGui = false;
static bool sBoolCapsLock = false;

// RAII class to conditionally toggle modifier when class object goes in and out of scope.
// If the modifier is already pressed then we don't do anything.
// This class is templated on the variables we pass in to allow us to declare types associated with the keys -
// see the section below that defines these types with the "using" keyword
// Usually the object is deleted when it goes out of scope (e.g., you have a closing '}' or you finish the current
// iterationm of the for loop contianing this class instantiation.
template< bool* modifierBool, char key >
class ScopedModifier {
  public:
    ScopedModifier()
    {
      if ( !*modifierBool ) {
        Keyboard.press(key); // press the Modifier key
        delay( preDelay );
      }
    }

    ~ScopedModifier()
    {
      if ( !*modifierBool ) {
        delay( preDelay );
        Keyboard.release(key); //release the Modifier key
      }
    }
};

// define the usual modifier classes
using ScopedControl = ScopedModifier< &sBoolCtrl, KEY_LEFT_CTRL >;
using ScopedAlt = ScopedModifier< &sBoolAlt, KEY_LEFT_ALT >;
using ScopedGui = ScopedModifier< &sBoolGui, KEY_LEFT_GUI >;
using ScopedShift = ScopedModifier< &sBoolShift, KEY_LEFT_SHIFT >;

// OS dependent modifier key - define MACOSX to toggle (on line42)
#ifdef MACOSX
using ScopedOsModifier = ScopedGui;
#else
using ScopedOsModifier = ScopedControl;
#endif

// Here we track whether the function key was down or up on the last loop. We use this
// to prevent key repeat of function keys
bool sFnPressed[18];

// Get the time delta in milliseconds between first and second, accounting for a possible
// integer overflow of the millis() function (this happens every 50 days or so)
// NOTE this will ALWAYS return a positive number
unsigned long timeDelta( unsigned long first, unsigned long second )
{
  if ( second >=  first ) {
    return second - first;
  } else {
    return ( ULONG_MAX - first ) + second + 1;
  }
}

// Helper funtion for keys that do not repeat
// Only accept these keys if the last time through they were not down.
// Also check that they are held down for a minimum period of time.
// Need to supply a pressState bool for tracking state
bool nonRepeatingKeyPress( int column, bool& pressState ) {
  if ( digitalRead( column ) == LOW ) {
    if ( !pressState ) {
      delay (minimumKeypressDelay);
      if ( digitalRead( column ) == LOW ) {
        pressState = true;
        return true;
      }
    }
  } else {
    pressState = false;
  }
  return false;
};

// Helper function to check whether a key has been pressed for minimumKeypressDelay
bool checkMinimumKeyPress( int column ) {
  if (digitalRead(column) == LOW) {
    delay (minimumKeypressDelay);
    if ( digitalRead( column ) == LOW ) {
      return true;
    }
  }
  return false;
}

// Check if modifier keys are down and change state accordingly. Usually two keys
// send the same modifier, so check them both.
// Only toggle once. No key press delay
// User must supply state bool to track modifier state
void modifierFunc( int column1, int column2, bool& state, char key ) {
  auto columnState1 = digitalRead(column1);
  auto columnState2 = digitalRead(column2);
  if ( !state && ( columnState1 == LOW || columnState2 == LOW ) ) {
    Keyboard.press(key);
    state = true;
  }
  if ( state && columnState1 == HIGH && columnState2 == HIGH ) {
    Keyboard.release(key);
    state = false;
  }
};

// Helper function to write a unicode key code (Linux only)
void writeUnicode( const char* unicodeString )
{
  {
    ScopedControl sc;
    ScopedShift ss;
    Keyboard.write( 'u' );
  }
  delay( preDelay );
  Keyboard.print ( unicodeString );
  delay( preDelay );
  Keyboard.write (0xB0); //return
}

//void writeUnicode( const char* unicodeString ) // Windows only
//{
//    Keyboard.print ( unicodeString );
//    delay( preDelay );
//    ScopedAlt sa;
//    Keyboard.write ('x');
//}

// This function wraps the simple Keyboard.write( key ) function and properly handles
// the caps lock key. The Keyboard library does not handle caps lock correctly, so we wrote our
// own. The caps lock key will only affect letters. Also, if the shift key is held down while
// caps lock is active, it will still type a lower case letter.
// We implement the caps lock by actually toggling the shift key, because sending an upper case
// character in teh Keyboard library also will toggle shift. So we only ever
// send lower case characters to write.
void writeToKeyboardRespectCapLock( char key )
{
  if ( !sBoolCapsLock || !isalpha( key ) ) {
    Keyboard.write( key );
  } else {
    if ( sBoolShift ) {
      Keyboard.release(KEY_LEFT_SHIFT);
      delay( preDelay );
      Keyboard.write( key );
      delay( preDelay );
      Keyboard.press(KEY_LEFT_SHIFT);
    } else {
      Keyboard.press(KEY_LEFT_SHIFT);
      delay( preDelay );
      Keyboard.write( key );
      delay( preDelay );
      Keyboard.release(KEY_LEFT_SHIFT);
    }
  }
}

// RAII class to make a row active when the class object is created, then make the row inactive when the object is deleted.
// Usually the object is deleted when it goes out of scope (e.g., you have a closing '}' or you finish the current
// iterationm of the for loop contianing this class instantiation.
class ScopedRowActive
{
  public:
    ScopedRowActive( int rowIndex) : mRowIndex( rowIndex )
    {
      digitalWrite( mRowIndex, LOW);
    }

    ~ScopedRowActive()
    {
      digitalWrite( mRowIndex, HIGH);
    }

  private:
    int mRowIndex;
};


void setup() {
  unsigned long currentTimeMs = millis();

  for ( int j = 0; j < 18; ++j ) {
    sFnPressed[ j ] = false;
  }

  for ( int i = 0; i < 5; ++i ) {
    for ( int j = 0; j < 18; ++j ) {
      sLastPress[ i ][ j ] = currentTimeMs;
      sLastClear[ i ][ j ] = currentTimeMs;
    }
  }

  // Set the column pins as inputs and pull them high
  for ( int j = 0; j < 18; ++j ) {
    pinMode(Col_0 + j, INPUT_PULLUP);
  }
  // Set the row pins as inputs and set them high
  pinMode (Row_0, OUTPUT);
  for ( int i = 0; i < 5; ++i ) {
    pinMode (rowIndices[ i ], OUTPUT);
    digitalWrite( rowIndices[ i ], HIGH);
  }

  // Set the LED pins as outputn and set them low
  pinMode (LED_1, OUTPUT);
  digitalWrite( LED_1, LOW);
  pinMode (LED_2, OUTPUT);
  digitalWrite( LED_2, LOW);

  // begin keyboard input
  Keyboard.begin();
}

void loop() {

  //------------------------------------------------------
  //------------------------------------------------------
  // Initial loop to check the modifier keys
  //------------------------------------------------------
  //------------------------------------------------------

  // CAPS LOCK
  // The behavior of the keyboard library capslock is weird
  // we are just going to roll our own
  static bool capsLockPressed = false;
//  {
//    ScopedRowActive rowLow( Row_4 );
//    if ( nonRepeatingKeyPress( Col_0, capsLockPressed ) ) {
//      sBoolCapsLock = !sBoolCapsLock;
//      auto ledState = sBoolCapsLock ? HIGH : LOW;
//      digitalWrite( LED_1, ledState);
//    }
//  } // after this brace row 4 is inactive

  //SHIFT
  {
    // activate row 5
    ScopedRowActive rowLow( Row_5 );
    modifierFunc( Col_1, Col_13, sBoolShift, KEY_LEFT_SHIFT );
  } // after this brace row 5 is inactive

  //Ctrl, ALT, Windows key
  {
    // activate row 6
    ScopedRowActive rowLow( Row_6 );
    modifierFunc( Col_0, Col_14, sBoolCtrl, KEY_LEFT_CTRL );
    modifierFunc( Col_3, Col_10, sBoolAlt, KEY_LEFT_ALT );
    modifierFunc( Col_1, Col_1, sBoolGui, KEY_LEFT_GUI );
  } // after this brace row 6 is inactive
  

  //------------------------------------------------------
  //------------------------------------------------------
  // Do all the standard characters from our array
  //------------------------------------------------------
  //------------------------------------------------------

  unsigned long currentTimeMs = millis();
  for ( int i = 0; i < 5; ++i ) {
    // activate row rowIndices[ i ]
    ScopedRowActive rowLow( rowIndices[ i ] );
    for ( int j = 0; j < 18; ++j ) {
      if ( sKeysForRows[ i ][ j ] != 0x00 ) {
        if ( digitalRead(Col_0 + j) == LOW ) {
          currentTimeMs = millis();
          unsigned long deltaPress = timeDelta( sLastPress[ i ][ j ], currentTimeMs );
          unsigned long deltaClear = timeDelta( sLastClear[ i ][ j ], currentTimeMs );
          // Choose delay so that there is a long pause before the first repeat, but it is faster after that
          int delayLocal = deltaClear > repeatDelayInitial ? repeatDelayRepeat : repeatDelayInitial;
          // If any of these are met then accept the keypress:
          // First term checks to see if the button was released more recently than it was pressed
          // Second term checks to see if the button was held down for at least a delay period before we send it again (repeat)
          if ( deltaClear < deltaPress || deltaPress > delayLocal ) {
            // Check again if the key is still pressed after a short delay. This fixes some latency in the keyboard
            delay (minimumKeypressDelay);
            if ( digitalRead(Col_0 + j) == LOW ) {
              writeToKeyboardRespectCapLock( sKeysForRows[ i ][ j ] );
              currentTimeMs = millis();
              sLastPress[ i ][ j ] = currentTimeMs;
            }
          }
        } else {
          sLastClear[ i ][ j ] = currentTimeMs;
          // set sLastPress to some recent time just before sLastClear, so time deltas never get big
          if ( currentTimeMs > 1 ) {
            sLastPress[ i ][ j ] = currentTimeMs - 1;
          }
        }
      }
    }// after this brace rowIndices[ i ] is inactive
  }

  // Now do the function keys (row 0). These do not repeat
  {
    // activate row 0
    ScopedRowActive rowLow( Row_0 );

    for ( int j = 0; j < 18; ++j ) {
      if ( sKeysForRowsFn[ j ] != 0x00 ) {
        if ( nonRepeatingKeyPress( Col_0 + j, sFnPressed[ j ] ) ) {
          Keyboard.write( sKeysForRowsFn[ j ] );
        }
      }
    }

    // What good are the function keys nowadays? These can be easily converted to macro keys!
    // See the bottom of the page for some suggested uses such as Copy, Paste, Print, Save, Unicode characters and EMOJIS.
    // I've replaced F1, F2, F3, and F4 with copy, paste, undu and redo respectively.

    // F1 (cut)
    if ( nonRepeatingKeyPress( Col_2, sFnPressed[ 2 ] ) ) {
      ScopedOsModifier sm;
      Keyboard.write( 'x' );
    }

    //F2 (Copy) 
    if ( nonRepeatingKeyPress( Col_3, sFnPressed[ 3 ] ) ) {
      ScopedOsModifier sm;
      Keyboard.write( 'c' );
    }

    // F3 (Paste)
    if ( nonRepeatingKeyPress( Col_4, sFnPressed[ 4 ] ) ) {
      ScopedOsModifier sm;
      Keyboard.write( 'v' );
    }

    //F4 (Undo)
    if ( nonRepeatingKeyPress( Col_5, sFnPressed[ 5 ] ) ) {
      ScopedOsModifier sm;
      Keyboard.write( 'z' );
    }

    // F5 (Redo)
    if ( nonRepeatingKeyPress( Col_7, sFnPressed[ 7 ] ) ) {
      ScopedOsModifier sm;
      Keyboard.write( 'y' );
    }

    //Print screen, Scrl and Pause don't have a hexidecimal value,
    //so they can be used for custom shortcuts. See the bottom of the sketch for some suggestions.


    // Print Screen. As stated above, print screen doest't  havea hexidecimal value but we can send Ctrl+P to accomplish the same thing.
    //    if ( nonRepeatingKeyPress( Col_15, sFnPressed[ 15 ] ) ) {
    //      ScopedOsModifier sm;
    //      Keyboard.write( 'p' );
    //    }

    // Print Screen: Indtead of Print Screen this types a degree symbol
    if ( checkMinimumKeyPress( Col_15 ) ) {
      writeUnicode( "00b0" );
    }

    //Scrl:  Instead of Scroll lock, this types an ohm symbol.
    if (checkMinimumKeyPress( Col_16 ) ) {
      writeUnicode( "03a9" );
    }

    //Pause: Instead of pause, this types a micro symbol.
    if ( checkMinimumKeyPress( Col_17 ) ) {
      writeUnicode( "03bc" );
    }
  } // after this brace row 0 is inactive

  // Now remap the menu and function keys
  { 
    // activate row 6
    ScopedRowActive rowLow( Row_6 );

    static bool sMenuKeyPressed = false;
    if ( nonRepeatingKeyPress( Col_13, sMenuKeyPressed ) ) {
      // add some macro for menu key
    }

    static bool sFunctionKeyPressed = false;
    if ( nonRepeatingKeyPress( Col_11, sFunctionKeyPressed ) ) {
      // add some macro for function key
    }
  } // after this brace row 6 is inactive
}
