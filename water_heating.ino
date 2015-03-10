#include <Adafruit_MotorShield.h>

/*********************************************************************

This is the code for running a Solar hot water system, with backup Rocket 
stove water heating for the greyer days!

It makes use of several libraries:
OneWire
Dallas Temperature
SPI
Adafruit Graphics library

The system uses 4 onewire temperature sensors, 3 relays, and an Adafruit 1306 
Monochrome OLED display

It should be fairly easy to tweak this to your needs - code is commented, and 
makes use of only trivial constructs (if, for, do etc) and glyph building 
capabilities of the graphics library.

No liability or responsibility can be taken for those who use this code - it is
supplied without warrantee or guarantee of suitability. Proper care and 
attention must be taken when designing your system - there are multiple 
possible dangers, for example:
Water
Electricity
Heat
Pressure

Do your homework, and above all, have fun and good luck!

*********************************************************************/

#include <Aduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
 
// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Declare this variable
// TODO: Not sure if this is required - looks like it isn't - only other ref 
//  is near line 147
int testRoutine = 0; 

// This section disabled to all use with Motorshield v2.0
// Declare relay variables
// int relayTank = 4;
// int relaySolar = 5;
// int relayRocket = 6;

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 
 
Adafruit_DCMotor *motorTank = AFMS.getMotor(1);
Adafruit_DCMotor *motorSolar = AFMS.getMotor(2);
Adafruit_DCMotor *motorRocket = AFMS.getMotor(3);
 
// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
 
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

/* Uncomment this block to use hardware SPI
#define OLED_DC     6
#define OLED_CS     7
#define OLED_RESET  8
Adafruit_SSD1306 display(OLED_DC, OLED_RESET, OLED_CS);
*/

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

// Declare the glyph for the Tank loop 

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B00000000,
  B00000001, B10000000,
  B00001110, B01110000,
  B00110000, B00001100,
  B01000000, B00000010,
  B01000000, B00000010,
  B01000000, B00000010,
  B01000011, B11111110,
  B01001100, B00000010,
  B01000011, B11000010,
  B01000000, B00110010,
  B01000011, B11000010,
  B01001100, B00000010,
  B01000011, B11111110,
  B01000000, B00000010,
  B01111111, B11111110 };

// Declare the glyph for the Solar loop

#define LOGO16_SOL_HEIGHT 16 
#define LOGO16_SOL_WIDTH  16 
static const unsigned char PROGMEM logo16_sol_bmp[] =
{ B00000000, B00000000,
  B01111111, B11111111,
  B01000000, B00000001,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01010101, B01010101,
  B01000000, B00000001,
  B01111111, B11111111,
  B00000000, B00000000 };


// Declare the glyph for the Rocket loop

#define LOGO16_ROCK_HEIGHT 16 
#define LOGO16_ROCK_WIDTH  16 
static const unsigned char PROGMEM logo16_rock_bmp[] =
{ B00000010, B00000000,
  B00000011, B00000000,
  B00000001, B10000000,
  B00000001, B11000000,
  B00000001, B11100000,
  B00000111, B11110000,
  B00001111, B11110000,
  B00001111, B11111000,
  B00111111, B11111100,
  B00111111, B11111100,
  B00111111, B10111110,
  B00111111, B10111110,
  B00111111, B10011110,
  B00111011, B10001110,
  B00111001, B00001110,
  B00011000, B00011100 };
  
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// End preamble

void setup()
{
// initialize the digital pins for Relays as outputs.
  pinMode(relayTank, OUTPUT);
  pinMode(relaySolar, OUTPUT);
  pinMode(relayRocket, OUTPUT);

// initialize this Input pin
// TODO: Do we need this?
  pinMode(testRoutine, INPUT);

// Start with Relays turned off (state:HIGH) apart from Rocket, which default 
//  to on (state:LOW)
  digitalWrite(relayTank, HIGH);
  digitalWrite(relaySolar, HIGH);
  digitalWrite(relayRocket, LOW);

  
  // start serial port
  // TODO: Do we actually need this?
  Serial.begin(9600);

  // Start up the library
  sensors.begin();

  // by default, we'll generate the high voltage from the 3.3v line internally!
  //  (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();
  
// End setup loop
  }

void loop(void)
{
// call sensors.requestTemperatures() to issue a global temperature
// request to all devices on the bus
 sensors.requestTemperatures(); 

//
// Set variables for referring to each sensor on the bus
//

 float tempTank = sensors.getTempCByIndex(1);
 float tempLLH = sensors.getTempCByIndex(2);
 float tempSolar = sensors.getTempCByIndex(0);
 float tempRocket = sensors.getTempCByIndex(3);

//
// If LLH is warmer than Tank+5 degrees, then start the Tank pump
//

if (tempLLH>(tempTank+5))
{
digitalWrite(relayTank, LOW);
}
else
{
digitalWrite(relayTank, HIGH);
};

//
// If Solar is warmer than LLH+10 degrees, then start the Solar pump
//

if (tempSolar>(tempLLH+10))
{
digitalWrite(relaySolar, LOW);
}
else
{
digitalWrite(relaySolar, HIGH);
};

//
// If Rocket is warmer than LLH+10 degrees, then start the Rocket pump
//

if (tempRocket>(tempLLH+10))
{
digitalWrite(relayRocket, LOW);
}
else
{
digitalWrite(relayRocket, HIGH);
};

// Sanity check for Tank temperature higher than Solar or Rocket to be sure
// and switch pump off if true - mainly of use if LLH is taken out of circuit as 
// LLH acts as mediator in other circumstances

if (tempTank > tempRocket)
{
digitalWrite(relayRocket, HIGH);
};

if (tempTank > tempSolar)
{
digitalWrite(relaySolar, HIGH);
};

//
// Error detection algorithm
// Check all sensors - if any are exactly the same value, declare an error 
//  state (can happen if there is a bad connection to a sensor)
// Error state of the onewire temp sensor is 85 degrees, however, I have also 
//  seen temps of -127 reported: check for both and declare an error state
// Error state is the activation of all pumps for safety reasons.
// TODO: Even in an error state, allow pumps to rest for 30 seconds in every 5
//  minutes
//

if (tempTank==-127.00 || 
    tempLLH==-127.00 || 
    tempSolar==-127.00 || 
    tempRocket==-127.00 || 
    tempTank==85.00 || 
    tempLLH==85.00 || 
    tempSolar==85.00 || 
    tempRocket==85.00 || 
    tempTank==tempLLH || 
    tempTank==tempSolar || 
    tempTank==tempRocket || 
    tempLLH==tempSolar || 
    tempLLH==tempRocket || 
    tempSolar==tempRocket)
{
digitalWrite(relayTank, LOW);
digitalWrite(relaySolar, LOW);
digitalWrite(relayRocket, LOW);

// Briefly display current error state

  display.setTextWrap(false);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Oops! We've");
  display.println("a problem.");
  display.println("Please call");
  display.println("@farmhouse");
  display.display();  
  
  delay(5000);
  
  display.setTextWrap(false);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("Tan: ");
  display.println(tempTank);
  display.print("LLH: ");
  display.println(tempLLH);
  display.print("Sol: ");
  display.println(tempSolar);
  display.print("Roc: ");
  display.println(tempRocket);
  display.display();  
  
  delay(1000);

// Jump to end of loop
  
  goto bailloop;

};

//
// Standard display loop
//
// Now display temperature of the water tank
//

  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
  display.setCursor(4,48);
  display.print("Water Temp.");
  display.display();
 
  delay(6000);

//
// Show all temperatures as reported (2 decimal places)
//

  display.setTextWrap(false);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("Tan: ");
  display.println(tempTank);
  display.print("LLH: ");
  display.println(tempLLH);
  display.print("Sol: ");
  display.println(tempSolar);
  display.print("Roc: ");
  display.println(tempRocket);
  display.display();  
  
  delay(2000);

//
// Detect if any pumps are currently running
//

if ((digitalRead(relayTank) == LOW) || 
    (digitalRead(relaySolar) == LOW) || 
    (digitalRead(relayRocket) == LOW))
{

// Animated display routine
// Display the pump running animation for the relevant pump(s) and pump
//   glyphs for 16 cycles


 for (int i=0; i <= 20; i++){
  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
if (digitalRead(relayTank) == LOW){
  display.setCursor(20,48);
  display.print("/");
};
if (digitalRead(relaySolar) == LOW){
  display.setCursor(64,48);
  display.print("-");
};
if (digitalRead(relayRocket) == LOW){
  display.setCursor(112,48);
  display.print("\\");
};
  display.drawBitmap(0, 46,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(44, 46,  logo16_sol_bmp, 16, 16, 1);
  display.drawBitmap(92, 46,  logo16_rock_bmp, 16, 16, 1);
  display.display();

  delay(50);
 
  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
if (digitalRead(relayTank) == LOW){
  display.setCursor(20,48);
  display.print("-");
};
if (digitalRead(relaySolar) == LOW){
  display.setCursor(64,48);
  display.print("\\");
};
if (digitalRead(relayRocket) == LOW){
  display.setCursor(112,48);
  display.print("|");
};
  display.drawBitmap(0, 46,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(44, 46,  logo16_sol_bmp, 16, 16, 1);
  display.drawBitmap(92, 46,  logo16_rock_bmp, 16, 16, 1);
  display.display();

  delay(50);
 
  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
if (digitalRead(relayTank) == LOW){
  display.setCursor(20,48);
  display.print("\\");
};
if (digitalRead(relaySolar) == LOW){
  display.setCursor(64,48);
  display.print("|");
};
if (digitalRead(relayRocket) == LOW){
  display.setCursor(112,48);
  display.print("/");
};
  display.drawBitmap(0, 46,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(44, 46,  logo16_sol_bmp, 16, 16, 1);
  display.drawBitmap(92, 46,  logo16_rock_bmp, 16, 16, 1);
  display.display();

  delay(50);
 
  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
if (digitalRead(relayTank) == LOW){
  display.setCursor(20,48);
  display.print("|");
};
if (digitalRead(relaySolar) == LOW){
  display.setCursor(64,48);
  display.print("/");
};
if (digitalRead(relayRocket) == LOW){
  display.setCursor(112,48);
  display.print("-");
};
  display.drawBitmap(0, 46,  logo16_glcd_bmp, 16, 16, 1);
  display.drawBitmap(44, 46,  logo16_sol_bmp, 16, 16, 1);
  display.drawBitmap(92, 46,  logo16_rock_bmp, 16, 16, 1);
  display.display();

};
};

//
// Hack to ensure smooth looping of the animation and no fixed frame while 
//   restarting arduino main loop
// TODO: check whether this is still necessary
//

  display.setTextWrap(false);
  display.setTextSize(5);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.print(tempTank,0);
  display.write(9);
  display.println("C");
  display.setTextSize(2);
  display.setCursor(4,48);
  display.print("Water Temp.");
  display.display();

bailloop:
// End Main loop
;
};
