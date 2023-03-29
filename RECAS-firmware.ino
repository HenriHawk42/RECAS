#include "TinyGPS++.h"
#include <LiquidCrystal.h>
TinyGPSPlus gps;

/*
 * This code allows you to check if the UART communication between the TeraRanger Evo and the Arduino Mega board works.
 * You just need to upload the code in the board with the TeraRanger connected on pins TX1, RX1, GND and 5V. Make sure to connect the signal RX of TeraRanger to TX1 of your Arduino and vice versa TX to RX1).
 * If the supply of the TeraRanger is stopped or interrupted while the code is running, just press the reset button on the board.
 * To check if the communication is working the best is to open the serial monitor and check if the distance is printed like this: "Distance in mm: XXX";
 */

// Terabee code that does something idk what
// Create a Cyclic Redundancy Checks table used in the "crc8" function
static const uint8_t crc_table[] = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x38, 0x3f, 0x36, 0x31,
  0x24, 0x23, 0x2a, 0x2d, 0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d, 0xe0, 0xe7, 0xee, 0xe9,
  0xfc, 0xfb, 0xf2, 0xf5, 0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85, 0xa8, 0xaf, 0xa6, 0xa1,
  0xb4, 0xb3, 0xba, 0xbd, 0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea, 0xb7, 0xb0, 0xb9, 0xbe,
  0xab, 0xac, 0xa5, 0xa2, 0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32, 0x1f, 0x18, 0x11, 0x16,
  0x03, 0x04, 0x0d, 0x0a, 0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a, 0x89, 0x8e, 0x87, 0x80,
  0x95, 0x92, 0x9b, 0x9c, 0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec, 0xc1, 0xc6, 0xcf, 0xc8,
  0xdd, 0xda, 0xd3, 0xd4, 0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44, 0x19, 0x1e, 0x17, 0x10,
  0x05, 0x02, 0x0b, 0x0c, 0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b, 0x76, 0x71, 0x78, 0x7f,
  0x6a, 0x6d, 0x64, 0x63, 0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13, 0xae, 0xa9, 0xa0, 0xa7,
  0xb2, 0xb5, 0xbc, 0xbb, 0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb, 0xe6, 0xe1, 0xe8, 0xef,
  0xfa, 0xfd, 0xf4, 0xf3
};

/*
 * Brief : Calculate a Cyclic Redundancy Checks of 8 bits
 * Param1 : (*p) pointer to receive buffer
 * Param2 : (len) number of bytes returned by the TeraRanger
 * Return : (crc & 0xFF) checksum calculated locally
 */
uint8_t crc8(uint8_t *p, uint8_t len) {
  uint8_t i;
  uint8_t crc = 0x0;
  while (len--) {
    i = (crc ^ *p++) & 0xFF;
    crc = (crc_table[i] ^ (crc << 8)) & 0xFF;
  }
  return crc & 0xFF;
}

// List of commands
const byte PRINTOUT_BINARY[4] = {0x00,0x11,0x02,0x4C};
const byte PRINTOUT_TEXT[4]   = {0x00,0x11,0x01,0x45};

// Initialize variables for distance sensor
const int BUFFER_LENGTH = 10;
uint8_t Framereceived[BUFFER_LENGTH];// The variable "Framereceived[]" will contain the frame sent by the TeraRanger
uint8_t index;// The variable "index" will contain the number of actual bytes in the frame to treat in the main loop
uint16_t distance;// The variable "distancex" will contain the distance value in millimeter

// Variables for calculations
double velocity;
double safeDistance;
double distanceDifference;

// User-modifiable variables
double reactionSpeed = 1.0;

// Maps values of distance to integers
// int range = map(distance);

// User-readable indicators
int safetyIndicator;
char safetySuggestion[8];

// Initalize Display
const int rs = 8, en = 9, d4 = 10, d5 = 16, d6 = 14, d7 = 15;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
// lcd(8, 9, 10, 16, 14, 15)

void setup() {
  pinMode(13, OUTPUT);// Initialize digital pin 13 as an output (ERASABLE if the communication works)
  Serial.begin(115200);// Open serial port 0 (which corresponds to USB port and pins TX0 and RX0), set data rate to 115200 bps
  Serial1.begin(115200);// Open serial port 1 (which corresponds to pins TX1 and RX1), set data rate to 115200 bps

  Serial1.write(PRINTOUT_BINARY, 4);// Set the TeraRanger in Binary mode
  //Serial1.write(PRINTOUT_TEXT, 4);// Set the TeraRanger in Binary mode
  index = 0;

  // LCD Setup
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.leftToRight();
  lcd.print("Initializing....");
}

// GPS pin initialization
//HardwareSerial ss(3, 2); //rx d3 tx d2

// The main loop starts here
void loop() {

  // GPS stuff
  //gps.encode(ss.read);

  // Clears LCD
  //lcd.clear();

  if (Serial1.available() > 0) {
    // Send data only when you receive data
    uint8_t inChar = Serial1.read();
    if ((inChar == 'T') && (index == 0)) {
      //Looking for frame start 'T' (start of the frame)
      Framereceived[index] = inChar;
      index++;
    }
    else {
      if (Framereceived[0] == 'T') {
        if ((index < 3) || ((index > 3) && (index < 9))) {
          //Gathering frame data
          Framereceived[index] = inChar;
          index++;
        }
        //Check if the frame in single-range
        else if (index == 3){
          Framereceived[index] = inChar;
          index++;
          if (crc8(Framereceived, 3) == Framereceived[3]) {
            //Convert bytes to distance
            distance = (Framereceived[1]<<8) + Framereceived[2];
            Serial.print("Distance in mm : ");
            Serial.println(distance);

            index = 0;
            Framereceived[0] = 0;
          }
        }
      }
    }
  }
  
  // Calculates safe following distance
  safeDistance = gps.speed.mps() * reactionSpeed;

  // Logic that compares the measured distance to the safe distance
  distanceDifference = (distance / 1000) - safeDistance;

  // Logic that turns the distance difference into an integer that can be read by the safety indicator
  safetyIndicator = round(distanceDifference / 4);

  // Sets cursor for bar readout
  lcd.leftToRight();
  lcd.setCursor(0, 1);

  switch (safetyIndicator) {
    case 0:
      lcd.print("                ");
      safetySuggestion[8] = "Good";
      break;
    case 1:
      lcd.print("|               ");
      safetySuggestion[8] = "Good";
      break;
    case 2:
      lcd.print("||              ");
      safetySuggestion[8] = "Coast";
      break;
    case 3:
      lcd.print("|||             ");
      safetySuggestion[8] = "Coast";
      break;
    case 4:
      lcd.print("||||            ");
      safetySuggestion[8] = "Coast";
      break;
    case 5:
      lcd.print("|||||           ");
      safetySuggestion[8] = "Slow";
      break;
    case 6:
      lcd.print("||||||          ");
      safetySuggestion[8] = "Slow";
      break;
    case 7:
      lcd.print("|||||||         ");
      safetySuggestion[8] = "Slow";
      break;
    case 8:
      lcd.print("||||||||        ");
      safetySuggestion[8] = "Brake";
      break;
    case 9:
      lcd.print("|||||||||       ");
      safetySuggestion[8] = "Brake";
      break;
    case 10:
      lcd.print("||||||||||      ");
      safetySuggestion[8] = "Brake";
      break;
    case 11:
      lcd.print("|||||||||||     ");
      safetySuggestion[8] = "Brake";
      break;
    case 12:
      lcd.print("||||||||||||    ");
      safetySuggestion[8] = "BRAKE";
      break;
    case 13:
      lcd.print("|||||||||||||   ");
      safetySuggestion[8] = "BRAKE";
      break;
    case 14:
      lcd.print("||||||||||||||  ");
      safetySuggestion[8] = "BRAKE";
      break;
    case 15:
      lcd.print("||||||||||||||| ");
      safetySuggestion[8] = "STOP";
      break;
    case 16:
      lcd.print("||||||||||||||||");
      safetySuggestion[8] = "STOP";
      break;
    default:
      lcd.print("lmao code better");
    break;
  }

  // Print distance on display
  lcd.leftToRight();
  lcd.setCursor(0, 0);
  lcd.print(round(distance / 1000));
  lcd.print("M");

  // Print safety suggestion on display
  lcd.rightToLeft();
  lcd.setCursor(15, 0);
  lcd.print(safetySuggestion[8]);

}