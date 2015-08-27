
/*********************************************************************
 * 
 * Water_Heating.ino
 * 
 * This is the code for running a Solar hot water system, with backup Rocket 
 * stove water heating for the greyer days! It also runs a light sensor to turn 
 * on campsite stair lighting when it gets dark. It adjusts the level of light as 
 * it gets darker to avoid dazzling campers and disturbing animal life too much.
 * Using a loop counter it activates a relay with a 4% duty cycle to control a
 * grey water recycling system, and prevent overfilling. It also uses this 
 * counter and the light sensor to determine when to turn off the freezer 
 * overnight to help save battery power. The freezer automatically turns on again
 * at dawn.
 * 
 * It makes use of several libraries:
 * 
 * OneWire
 * Dallas Temperature
 * SPI
 * Adafruit Graphics library
 * Adafruit Motorshield (v2.3)
 * TSL2561 Light Sensor
 * 
 * (You will need to install all of these if you want the code below to compile!)
 * 
 * The system uses 4 onewire temperature sensors, all 4 PWM outputs of an Adafruit 
 * Motorshield (v2.3), a TSL2561 Light sensor from Sparkfun and an Adafruit 1306 
 * Monochrome OLED display, as well as two relay shields.
 * 
 * It should be fairly easy to tweak this to your needs - code is commented, and 
 * makes use of only trivial constructs (if, for, do etc) and basic glyph building 
 * capabilities of the graphics library.
 * 
 * No liability or responsibility can be taken for those who use this code - it is
 * supplied without warrantee or guarantee of suitability. Proper care and 
 * attention must be taken when designing your system - there are multiple 
 * possible dangers, for example:
 * 
 * Water
 * Electricity
 * Heat
 * Pressure
 * 
 * Do your homework, design with safety in mind, and build carefully.
 * 
 * Above all, have fun and good luck!
 * 
 *********************************************************************/

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_PWMServoDriver.h"
#include <SFE_TSL2561.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Create an SFE_TSL2561 object, here called "light":

SFE_TSL2561 light;

// Global variables:

boolean gain;     // Gain setting, 0 = X1, 1 = X16;
unsigned int ms;  // Integration ("shutter") time in milliseconds

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 2

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 

Adafruit_DCMotor *motorTank = AFMS.getMotor(1);
Adafruit_DCMotor *motorSolar = AFMS.getMotor(2);
Adafruit_DCMotor *motorRocket = AFMS.getMotor(3);
Adafruit_DCMotor *stairLights = AFMS.getMotor(4);

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Declare the glyph for the Tank loop 

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ 
  B00000000, B00000000,
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
{ 
  B00000000, B00000000,
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
{ 
  B00000010, B00000000,
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

boolean tank = false;
boolean solar = false;
boolean rocket = false;
boolean lights = false;
int loopno = 3;
int loopFreezer = 7;
int relayGrow = 3;
int relayFreezer = 4;

// End preamble

void setup()
{

  // intialize the motorshield with a pwm frequency of 20
  AFMS.begin(20);

  // initialize the light sensor
  light.begin();

  // set gain to low
  gain = 0;

  // integration time of light sensor set to 402ms
  unsigned char time = 2;

  // setTiming() will set the third parameter (ms) to the
  // requested integration time in ms (this will be useful later):

  light.setTiming(gain,time,ms);

  // To start taking measurements, power up the sensor:

  light.setPowerUp();

  // Start up the library
  sensors.begin();

  // by default, we'll generate the high voltage from the 3.3v line internally!
  //  (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  // init done
  display.clearDisplay();

  // declare the relay pin3 an output for the grey water pump
  pinMode(relayGrow, OUTPUT);

  // declare the relay pin4 an output for the freezer power
  pinMode(relayFreezer, OUTPUT);

  // set relays to off initially
  digitalWrite(relayGrow,HIGH);
  digitalWrite(relayFreezer,HIGH);

  // Test all pumps at maximum, and the lights (at 50%) for 2s
  motorTank->setSpeed(255);
  motorSolar->setSpeed(255);
  motorRocket->setSpeed(255);
  stairLights->setSpeed(128);
  motorTank->run(FORWARD);  // turn on motor
  motorSolar->run(FORWARD);  // turn on motor
  motorRocket->run(FORWARD);  // turn on motor
  stairLights->run(FORWARD);  // turn on lights
  digitalWrite(relayGrow,LOW);
  delay(2000);
  motorTank->run(RELEASE);  // turn off motor
  motorSolar->run(RELEASE);  // turn off motor
  motorRocket->run(RELEASE);  // turn off motor
  stairLights->run(RELEASE);  // turn off lights
  digitalWrite(relayGrow,HIGH);


  // End setup loop
}

void loop(void)
{

  //////////////////////////  
  // Start with the logic  
  //////////////////////////

  uint8_t i;

  // There are two light sensors on the device, one for visible light
  // and one for infrared. Both sensors are needed for lux calculations.

  // Retrieve the data from the device:

  unsigned int data0, data1;
  double lux;    // Resulting lux value

  if (light.getData(data0,data1))
  {
    // getData() returned true, communication was successful

    // To calculate lux, pass all your settings and readings
    // to the getLux() function.

    // The getLux() function will return 1 if the calculation
    // was successful, or 0 if one or both of the sensors was
    // saturated (too much light). If this happens, you can
    // reduce the integration time and/or gain.
    // For more information see the hookup guide at: https://learn.sparkfun.com/tutorials/getting-started-with-the-tsl2561-luminosity-sensor

    boolean good;  // True if neither sensor is saturated

      // Perform lux calculation:

    good = light.getLux(gain,ms,data0,data1,lux);

    if (good == true && lux < 30) 
    {
      stairLights->setSpeed((lux*3)+30);
      stairLights->run(FORWARD);
      lights = true;
    }
    else
    {
      stairLights->setSpeed(0);
      stairLights->run(RELEASE);
      lights = false;
    }
  }
  else
  {
    // getData() returned false because of an I2C error, inform the user.

    byte error = light.getError();
    printError(error);
    stairLights->setSpeed(0);
    stairLights->run(RELEASE);
    lights = false;
  }

  // Set up the Grow timing loop - pump should be on for the first 3 and middle 3 cycles in 75.

  // loop counter should be reset after 75 loops.

  if (loopno >= 75) {
    loopno = 0;
  }

  // add 1 to the loop counter, and return new value

  ++loopno;

  // now check if we are at the first 4 cycles, if so, turn relay on, otherwise, leave switched off

  if ((loopno <= 3 || (loopno > 37 && loopno <= 40)) && lights == false) {
    digitalWrite(relayGrow,LOW); // switch the relay and run the pump
  }
  else
  {
    digitalWrite(relayGrow,HIGH); // don't switch the relay (no pump)
  }

  // Freezer will be turned off and on with a cycle of 6 complete loopno cycles (about 1 hour). 
  // Once stair lights are lit, freezer will run on a new cycle...

  // reset the counter the moment the stair lights are off and after the counter has 
  // completed a complete cycle (12 count)
  if (lights == false && loopFreezer >= 12) {
    loopFreezer = 0;
  }

  // now set our freezer loop counter using loopno as a base
  if (loopno == 1) {
    ++loopFreezer;
  }

  // if the freezer loop count is less than or equal to 8 then turn the freezer on
  // else turn the freezer off. This is roughly equal to 60 minutes in every 2 hours.
  // As freezer loop is only ever reset during the day, this means freezer will not 
  // turn on overnight apart for quick boosts - roughly 30 minutes in every 2 hours)
  if (loopFreezer <= 6 || 
       (loopFreezer > 18 && loopFreezer <=21) || 
       (loopFreezer > 30 && loopFreezer <=33) || 
       (loopFreezer > 42 && loopFreezer <=45) ||  
       (loopFreezer > 54 && loopFreezer <=57) ||  
       (loopFreezer > 66 && loopFreezer <=69)) {
    digitalWrite(relayFreezer,LOW); // switch the relay and turn on freezer   
  }
  else 
  {
    digitalWrite(relayFreezer,HIGH); // switch the relay and turn off freezer
  }

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
    motorTank->run(FORWARD);
    for (i=255; i>50; i=i-25) {
      motorTank->setSpeed(i);  
      delay(20);
    }
    motorTank->setSpeed(50);  
    tank = true;
  }
  else
  {
    motorTank->setSpeed(0);  
    motorTank->run(RELEASE);
    tank = false;
  };

  //
  // If Solar is warmer than LLH+10 degrees, then start the Solar pump
  //

  if (tempSolar>(tempLLH+10) && tempSolar>(tempTank+5))
  {
    motorSolar->run(FORWARD);
    for (i=255; i>50; i=i-25) {
      motorSolar->setSpeed(i);  
      delay(20);
    }
    if (tempSolar<65)
    {
      motorSolar->setSpeed(55);
      solar = true;
    }
    else if (tempSolar>=90)
    {
      motorSolar->setSpeed(255);
      solar = true;
    }
    else
    {
      // Calculation of of how speed adjusts with temperature is as follows:
      // ((Detected temp - ramping start temp)*Ramp factor)+ramping start speed)
      // Where Ramp factor = Maximum possible speed (255) - start speed (60) all divided by maximum temp (95) - ramping start temp (80) i.e. 195/15
      motorSolar->setSpeed(round(((tempSolar-65)*8)+55));
      solar = true;
    }
  }
  else
  {
    motorSolar->setSpeed(0);  
    motorSolar->run(RELEASE);
    solar = false;
  };

  //
  // If Rocket is warmer than LLH+5 degrees, then start the Rocket pump
  //

  if (tempRocket>(tempLLH+10) && tempRocket>(tempTank+10))
  {
    motorRocket->run(FORWARD);
    for (i=255; i>50; i=i-25) {
      motorRocket->setSpeed(i);  
      delay(20);
    }
    if (tempRocket<55)
    {
      motorRocket->setSpeed(30);
      rocket = true;
    }
    else if (tempRocket>=55 && tempRocket<80)
    {
      motorRocket->setSpeed(round(tempRocket-25));
      rocket = true;
    }
    else if (tempRocket>=90)
    {
      motorRocket->setSpeed(255);
      rocket = true;
    }
    else
    {
      // Calculation of of how speed adjusts with temperature is as follows:
      // ((Detected temp - ramping start temp)*Ramp factor)+ramping start speed)
      // Where Ramp factor = Maximum possible speed (255) - start speed (e.g.45) all divided by maximum temp (e.g.90) - ramping start temp (e.g.70) i.e. 210/20
      motorRocket->setSpeed(round(((tempRocket-80)*20)+55));
      rocket = true;
    }

  }
  else
  {
    motorRocket->setSpeed(0);  
    motorRocket->run(RELEASE);
    rocket = false;
  };

  ///////////////////////////////  
  // Error detection algorithm
  ///////////////////////////////

  //
  // Check all sensors - if any are exactly the same value, declare an error 
  //  state (can happen if there is a bad connection to a sensor)
  // Error state of the onewire temp sensor is 85 degrees, however, I have also 
  //  seen temps of -127 reported: check for both and declare an error state
  // Error state declared when the rocket stove is reporting a temperature above 
  //  95degrees
  // Error state is the activation of all pumps at high speed for safety reasons.
  // Even in an error state, pumps are allowed to rest for roughly 30 seconds in
  //  every 5 minutes (95% duty cycle) using the loop counter.
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
    tempSolar==tempRocket ||
    tempRocket>=95.00)
  {
    if (loopno < 5) {

      motorTank->setSpeed(0);
      motorSolar->setSpeed(0);
      motorRocket->setSpeed(0);
      motorTank->run(RELEASE);  // turn off motor
      motorSolar->run(RELEASE);  // turn off motor
      motorRocket->run(RELEASE);  // turn off motor

      tank = false;
      solar = false;
      rocket = false;

    }
    else
    {

      motorTank->setSpeed(255);
      motorSolar->setSpeed(255);
      motorRocket->setSpeed(255);
      motorTank->run(FORWARD);  // turn on motor
      motorSolar->run(FORWARD);  // turn on motor
      motorRocket->run(FORWARD);  // turn on motor

      tank = true;
      solar = true;
      rocket = true;
    }
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

    delay(3000);

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

  ///////////////////////////////  
  // Main Display Loop
  ///////////////////////////////

  //
  // Display temperature of the water tank
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

  delay(2000);

  //
  // Display campsite lights state
  //

  if (lights == true) {
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Campsite");
    display.println("lights are");
    display.print("on @ ");
    display.print(round((((lux*3)+20)/255)*100));
    display.println("%");
    display.print("brightness.");
    display.display();
    delay(500);  
  }
  else
  {
    display.setTextWrap(false);
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Campsite");
    display.println("lights are");
    display.println("off @ ");
    display.print(round(lux));
    display.println("lux");
    display.display();
    delay(500);  
  };

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

  delay(1000);

  //
  // Detect if any pumps are currently running
  //


  // Animated display routine
  // Display the pump running animation for the relevant pump(s) and pump
  //   glyphs for 10 cycles


  for (int i=0; i <= 10; i++){
    display.setTextWrap(false);
    display.setTextSize(5);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.print(tempTank,0);
    display.write(9);
    display.println("C");
    display.setTextSize(2);
    if (tank == true){
      display.setCursor(20,48);
      display.print("/");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (solar == true){
      display.setCursor(64,48);
      display.print("-");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(112,48);
      display.print("\\");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
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
    if (tank == true){
      display.setCursor(20,48);
      display.print("-");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (solar == true){
      display.setCursor(64,48);
      display.print("\\");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(112,48);
      display.print("|");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
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
    if (tank == true){
      display.setCursor(20,48);
      display.print("\\");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (solar == true){
      display.setCursor(64,48);
      display.print("|");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(112,48);
      display.print("/");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
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
    if (tank == true){
      display.setCursor(20,48);
      display.print("|");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (solar == true){
      display.setCursor(64,48);
      display.print("/");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    if (rocket == true){
      display.setCursor(112,48);
      display.print("-");
    }
    else{
      display.setCursor(20,48);
      display.print(" "); 
    };
    display.drawBitmap(0, 46,  logo16_glcd_bmp, 16, 16, 1);
    display.drawBitmap(44, 46,  logo16_sol_bmp, 16, 16, 1);
    display.drawBitmap(92, 46,  logo16_rock_bmp, 16, 16, 1);
    display.display();

  };

  //
  // Hack to ensure smooth looping of the animation and no fixed frame while 
  //  restarting arduino main loop - set display to show main tank temperature
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

void printError(byte error)
// If there's an I2C error, this function will
// print out an explanation.
{
  //  Serial.print("I2C error: ");
  //  Serial.print(error,DEC);
  //  Serial.print(", ");

  switch(error)
  {
  case 0:
    //      Serial.println("success");
    break;
  case 1:
    //      Serial.println("data too long for transmit buffer");
    break;
  case 2:
    //      Serial.println("received NACK on address (disconnected?)");
    break;
  case 3:
    //      Serial.println("received NACK on data");
    break;
  case 4:
    //      Serial.println("other error");
    break;
  default:
    //      Serial.println("unknown error")
    ;
  }
}
