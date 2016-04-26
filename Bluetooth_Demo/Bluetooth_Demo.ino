//---------------------------------------------------------------------------------
// Arduino Program - Bluetooth HC-06 Demo
// @author - A man of wealth and taste
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

// @brief: Bluetooth Demo that receives commands via serial connection
// and prints a mode to the LCD.  Users can modify the commands if more are sought.

//---------------------------------------------------------------------------------
// Hardware Pin Setup

// All pinouts affiliated with Arduino Sensor Shield v5.0 
// @https://www.google.com/search?q=arduino+sensor+shield+v5+0+schematic&espv=2&biw=1366&bih=643&tbm=isch&imgil=dD7uc3MRnrh25M%253A%253B_gEggaphUxAm8M%253Bhttp%25253A%25252F%25252Fforum.arduino.cc%25252Findex.php%25253Ftopic%2525253D114307.0&source=iu&pf=m&fir=dD7uc3MRnrh25M%253A%252C_gEggaphUxAm8M%252C_&usg=__18YXtbn8lLZ1tRWzxEtq1WrUFrs%3D&ved=0ahUKEwizyKb-hqvMAhXI7iYKHZpDDJQQyjcINA&ei=QbceV_OjOMjdmwGah7GgCQ#imgrc=dD7uc3MRnrh25M%3A

// On bluetooth interface:
// Vcc -> Vcc
// RX -> D1
// TX -> D0
// Gnd -> Gnd

//---------------------------------------------------------------------------------
// Required Libraries - THESE MUST BE INSTALLED OR YOU WILL GET ERRORS!

// - J. MacVey's LCDManager class that wraps around the Malpartida's 
// to simplify LCD writing and display
//  @https://github.com/jmacvey/Robotics
// Download the zip, extract the LCDManager folder, 
// and install in your Arduino/libraries folder
// Edit -> Include Libaries -> Manage Libaries... to update paths

//  - F. Malpartida's LiquidCrystal_I2C library for LCD-I2C bus
//  @https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home
// Extract contents of zipped file and place into your local Arduino/libraries folder.
// Be sure to either restart the Arduino IDE or click
// Edit -> Include Libaries -> Manage Libaries... to update paths

// - S. Monk's Timer library to demonstrate "pseudo-asynchronous" threading in Arduino
//  @http://playground.arduino.cc/Code/Timer
// Scroll down to installation and click download, 
// follow same steps as you did with LCD_I2C lib

//---------------------------------------------------------------------------------
// Instructions

// Upload code to the board and connect to the HC-06 bluetooth module with a bluetooth terminal (default pw 1234 or 0000)
// emulator application.  Type commands to the device as specified in the command list.
// This demo was tested with Android's free "Bluetooth Terminal" by Qwerty (free from the Google Play Store)

//---------------------------------------------------------------------------------
// Potential Errors and Solutions:

// Error: 
//    out of sync
// Explanation: 
//    You are likely trying to upload to the board with the sensor shield attached
// This is a problem insofar the upload process utilizes D0/D1 (Rx/Tx) on the Arduino
// and if the sensor rx/tx pins are plugged in during upload, you get async error
// Solution:
//    Unplug the motor shield while you upload code to the board.  Then replug the board in.

// Error:
//    Compilation error -- "Wire.h" not found or undefined references for I2C library
// Explanation:
//    Arduino linkers on IDE's before 1.6.6 are fairly stupid - they don't know how to find 
// "hardware-associated" libraries when included in custom libaries or files
// Solution:
//    Update your IDE to latest version via Arduino website

// Error:
//    Connected to bluetooth module but not receiving any response in the terminal when typing 
// commands.
// Explanation:
//    (1) Faulty or broken device (these devices are easily damaged if over 3.3V is supplied to rx/tx pins for
// a long time)
//    (2) Tried to use software serial through the sensor board on pins other than 0/1 (doesn't work)
// Solutions
//    (1) Replace the device
//    (2) Ensure the BLUETOOTH_RX and BLUETOOTH_TX in Config.h is defined as 1 and 0, respectively.

// ALL OTHER ERRORS:
// Explanation:
//    You've likely wired something up wrong
// Solution 1:
//    Close your eyes, breath deeply, admit your mistake, and reflect on how you can 
// correct your mistakes.
// Solution 2:
//    Yell "God damnit," drink 3 beers, and forget about this until someone else fixes
// it for you.

//---------------------------------------------------------------------------------
// Things to note when modifying the file

// You can't enable virtual serial ports through the sensor shield.  
// Tried with nearly all the digital I/O pins and the only one that 
// seems to work is a software serial throughpins D1 and D0, which 
// technically already hardwired from the sensor shield to the Arduino.

// Also, users should use the Timer class to facilitate "async" checking
// of the bluetooth module.  This is good practice since you can't tie the 
// TX/RX pins to interrupts in any way that is convenient.

// What this demo will do is allow you to control the robot based on string
// commands through a simple bluetooth terminal emulator that can be found on 
// iOS or Android stores (i.e. you don't need to write their own mobile app to 
// incorporate bluetooth stuff). 

// To amend the demo to facilitate other commands, it's as simple as adding a string 
// to the command and extra else-if case to the "updateBluetooth" method.  
// Calling timer.update() in the PID loop will also enable that "pseudo-interrupt" 
// style to stop/start the robot or put it into different modes.  

//---------------------------------------------------------------------------------
// Includes

#include <Wire.h>
#include <SoftwareSerial.h>
#include "LCDManager.h"
#include "Config.h"
#include "Timer.h"

//---------------------------------------------------------------------------------
// Bluetooth Commands - To implement more

const String BLUETOOTH_HELP   = "HELP";
const String WALL_FOLLOW_MODE = "WFM";
const String LINE_FOLLOW_MODE = "LFM";

//---------------------------------------------------------------------------------
// Forward Declarations

void updateBluetooth();

//---------------------------------------------------------------------------------
// Globals

// Creates a virtual serial port
// You can change the pins to D0-D12 if you are not using the v5.0 sensor shield
// Otherwise you should keep values for Bluetooth RX and TX at 1 and 0, respectively
SoftwareSerial BT(BLUETOOTH_RX, BLUETOOTH_TX); 
LCDManager* lcdManager;
Timer* timer; // use timer since interrupt pins are inconvenient, kinda like margarine

//---------------------------------------------------------------------------------
// Initialization

void setup()  
{
  BT.begin(9600);
  lcdManager = new LCDManager();
  timer = new Timer();
  timer->every(BLUETOOTH_DELAY, updateBluetooth);
}


//---------------------------------------------------------------------------------
// Main Program

void loop() 
{
  // required.  (takes ~1-20us to execute)
  // Stupid question: why use the timer?
  // Simple answer: because you can always pass a reference to it
  // to other modules to enable a pseudo-interrupt at little cost
  // to memory and execution time
  timer->update();
}


void updateBluetooth() {
  // if text arrived in from BT serial...
  if (BT.available())
  {
    // note we use if/else here because arduino can't switch/case over strings
    String stringBuffer = BT.readString();
    if (stringBuffer.equals(LINE_FOLLOW_MODE))
    {
      lcdManager->printStringToLCD("Line Follower Mode Enabled");
      BT.println("Line Follower Mode Enabled");
    }
    else if (stringBuffer.equals(WALL_FOLLOW_MODE))
    {
      lcdManager->printStringToLCD("Wall Follower Mode Enabled");
     // digitalWrite(A1, LOW);
      BT.println("Wall Follower Mode Enabled");
    }
    else if (stringBuffer.equals(BLUETOOTH_HELP))
    {
      BT.println("Commands:");
      BT.println("WFM -> Enable Wall Follow Mode");
      BT.println("LFM -> Enable Line Follow Mode");
    }
    // TO DO : Implement your own commands
    else
    {
      BT.println("Command not recognized.");
      BT.println("Type \"HELP\" to see available commands");
    }
  }
}

