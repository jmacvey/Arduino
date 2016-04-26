//---------------------------------------------------------------------------------
// Arduino Program - LCD Display Demo using I2C Bus and F. Malpartida's
// LiquidCrystal_I2C libary
// @author - J. MacVey
//---------------------------------------------------------------------------------

/* This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
    */

// @brief: prints whatever you type in the Arduino Serial console
// to the LCD display using the I2C bus.  Change baud_rate, i2c pins,
// and num chars to reflect your device layout
// If you have trouble compiling, you probably need to upgrade your Arduino IDE
// to the latest version -- old version linkers are t-e-r-r-i-b-l-e

// Download F. Malpartida's LiquidCrystal_I2C libary 
// @https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// Extract contents and place into your local Arduino/libraries folder.
// Be sure to either restart the Arduino IDE or click
// Edit -> Include Libaries -> Manage Libaries... to update paths
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>

//---------------------------------------------------------------
// Forward Declarations

// note uint8_t = unsigned (i.e. > 0) 8-bit integer
// Microcontrollers of limited storage and dynamic memory should use this
// type liberally to avoid unnecessary memory demands
void printWord(const String& serialBuffer, uint8_t& indexSpace, uint8_t& lastSpace,
  uint8_t& wordLength, uint8_t& currRow, uint8_t& offset);

//---------------------------------------------------------------
// I2C Bus to LCD Pins

#define Rs_pin         0
#define Rw_pin         1
#define En_pin         2
#define BACKLIGHT_PIN  3 
#define D4_pin         4
#define D5_pin         5
#define D6_pin         6
#define D7_pin         7

//---------------------------------------------------------------
// Other Constants

#define LED_OFF        0
#define LED_ON         1
#define BAUD_RATE      9600
#define CHARS_PER_LINE 16
#define NUM_LINES      2
#define DEVICE_NOT_FOUND -1

//---------------------------------------------------------------
// Globals
//---------------------------------------------------------------

// initialize the lcd display in setup 
LiquidCrystal_I2C* lcd = NULL;
String serialBuffer = "";
const int FRAME_DELAY = 3000;

//---------------------------------------------------------------
// Initialization Code
//---------------------------------------------------------------

void setup()
{
  Serial.begin(BAUD_RATE);
  uint8_t deviceAddress = findDeviceAddress();
  // findDeviceAddress() DEVICE_NOT_FOUND if the SDA/SCL pins aren't set up correctly.
  if (deviceAddress != DEVICE_NOT_FOUND) {
      lcd = new LiquidCrystal_I2C(deviceAddress,
        En_pin,
        Rw_pin,
        Rs_pin,
        D4_pin,
        D5_pin,
        D6_pin,
        D7_pin);      
      lcd->begin(CHARS_PER_LINE, NUM_LINES);
  // backlight pin, positive polarity
  // If the LED flickers in and out, the likely fix is a change 
  // of the polarity from positive to NEGATIVE
      lcd->setBacklightPin(BACKLIGHT_PIN, POSITIVE);
      lcd->setBacklight(LED_ON);
      lcd->clear();
      lcd->setCursor(0, 0);
   }
}

// Because of course no one knows their own address these days...
uint8_t findDeviceAddress() {
  // I2C does 7-bit addressing.. 2^7
  // Dr. Kwon would know this sooo fast. 
  // "What is 01111111?"  "You are EE!  You should know this!"
  uint8_t POSSIBLE_ADDRESSES = 128; 
  Wire.begin();
  uint8_t addressOfDevice = 1;
  bool addressFound = false;
  for (addressOfDevice; addressOfDevice < POSSIBLE_ADDRESSES; addressOfDevice++)
  {
    Wire.beginTransmission(addressOfDevice);
    if (Wire.endTransmission() == 0)
    {
      addressFound = true;
      break;
    }
  }
  if (addressFound)
  {
    Serial.print("Device Address: ");
    Serial.println(addressOfDevice);
    return addressOfDevice;
  }
  else return DEVICE_NOT_FOUND;
}

//---------------------------------------------------------------
// Some C/++ stuff for optimal chick magnetization
//---------------------------------------------------------------

// Bruce Willis and Joseph Gordon Levitt
void loop()
{
  if (Serial.available())
  {
    lcd->clear();
    lcd->setCursor(0, 0);
    serialBuffer = Serial.readString();
    printStringToLCD(serialBuffer);
  }
}

// Prints strings of whatever length to the LCD display
// every | one | take | lesson |
// to | pass | by | const | reference
// else | pay | the | runtime | and | execute | sllllooooooooooow . w. w. w.
void printStringToLCD(const String& serialBuffer) {
  // O(n) algorithm -> It's the best... around... no one's ever gonna bring it down...
  uint8_t wordLength = 0, currRow = 0, lastSpace = 0, indexSpace = 0, offset = 0;
  uint8_t strLength = serialBuffer.length();
  char stringBuffer[strLength + 1]; serialBuffer.toCharArray(stringBuffer, strLength);
  for (offset; offset < strLength + 1; offset++)
  {
    if (stringBuffer[offset] == ' ')
    {
      printWord(serialBuffer, indexSpace, lastSpace, wordLength, currRow, offset);
    }
    wordLength++;
  }
  printWord(serialBuffer, indexSpace, lastSpace, wordLength, currRow, offset);
}


// every | one | take | lesson |
// you | can | pass | just| by | reference
// to | manipulate | values
void printWord(const String& serialBuffer, uint8_t& indexSpace, uint8_t& lastSpace,
  uint8_t& wordLength, uint8_t& currRow, uint8_t& offset) {
    // space enough for the word
  if ((indexSpace + wordLength - 1) < CHARS_PER_LINE)
  {
    lcd->print(serialBuffer.substring(lastSpace, lastSpace + wordLength));
  }
    // not enough space for this word
    // move to next line
  else
  {
    if (currRow == 1) {
      delay(FRAME_DELAY);
      lcd->clear();
    }
    // shorthand syntax for comparison
    currRow = currRow == 1 ? 0 : 1;
    lcd->setCursor(0, currRow);
    lcd->print(serialBuffer.substring(lastSpace + 1, lastSpace + wordLength));
    indexSpace = 0;
  }
  indexSpace += wordLength;
  lastSpace = offset;
  wordLength = 0;
}

