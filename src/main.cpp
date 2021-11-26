/*! @file main.cpp */

/**
 * @brief Pinout summary :
Digital pins
2 : Btn1, minus button for settings
3 : Btn2, plus button for settings
4 : 3 positions switch, temperatures presets
5 : 3 positions switch, time settings
6 : yellow LED
7 : blue LED
8 : Relay (HEATING)
9 : DS18B20 (One Wire)

Analog pins
A2 : presets, 6 positions switch
A4 : SDA (I2C)
A5 : SCL (I2C)

*/
#define X_RTC
#include <Arduino.h>
// Include Wire Library for I2C
#include <Wire.h>
#include <RTClib.h>
RTC_DS1307 rtc;
/*
DateTime now;
*/
// Include Adafruit Graphics & OLED libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// see https://github.com/adafruit/Adafruit_SSD1306/blob/master/examples/ssd1306_128x32_i2c/ssd1306_128x32_i2c.ino
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// related to DS18B20
#define DS18B20_PIN 9
#define TEMPERATURE_PRECISION 12
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

// for "volatile", see https://www.youtube.com/watch?v=55YEZppz7p4
volatile float tempT = 15.0; /*!< Target temperature */
volatile float tempStep = 0.2;

volatile char mode;
volatile bool timeUpdatedPlus, timeUpdatedMinus;

// PLUS_PIN and MINUS_Pin are used for setting target temperature via buttons BTN1 and BTN2 buttons
#define PLUS_PIN 3
#define MINUS_PIN 2
#define BTN1 3
#define BTN2 2

#define BTN_DELAY 300
#define HEATING_PIN 8 /*!< relay pin */
#define HEATING_DELAY 40000000 /*!< 40000000 uSec = 40sec. */
#define HYSTERESIS .2
// LEDs
#define YELLOW 6
#define BLUE 7

bool heatingActive = 0 ;

// int sw1, sw2;
// bool on = 1, off = 0;
int i = 0;
int d = 0;

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void heating(bool On) {
  // turn ON or OFF the heating
  unsigned long _micros = micros() ;
  static unsigned long lastChange = 0 ;
  // if delay too short beetween 2 transitions, we do nothing
  // this is to prevent switches too fast of the thermostat
  if ((_micros - lastChange) < HEATING_DELAY) {
    return ;
  }
  // the delay is ok
  if (On) {
    digitalWrite(HEATING_PIN, HIGH) ;
    heatingActive = 1 ;
  } else {
    digitalWrite(HEATING_PIN, LOW) ;
    heatingActive = 0 ;
  }
  lastChange = _micros ;
}

void led(int LEDcolor, bool onOff) {
  if (onOff) {
    digitalWrite(LEDcolor, HIGH);
  } else {
    digitalWrite(LEDcolor, LOW);
  }
}

void blinkLeds() {
  led(BLUE, 1);
  led(YELLOW, 1);
  delay(100);
  led(BLUE, 0);
  led(YELLOW, 0);
}

/**
 * @brief Gestion du bouton poussoir connecté à MINUS_PIN == 2
 *
 * @author Eric Hardy
 * 
 * see : https://www.youtube.com/watch?v=55YEZppz7p4
 */
void btn1() {
  static unsigned long lastPressBTN1 = 0;
  unsigned long _millisBTN1 = millis();
  if ((_millisBTN1 - lastPressBTN1) >= BTN_DELAY) {
    if (mode < 4) {
        tempT -= tempStep ;
    }
    if (mode == 5) {
      timeUpdatedMinus = 1;
    }
    lastPressBTN1 = _millisBTN1;
  }
}

void btn2() {
  static unsigned long lastPressBTN2 = 0;
  unsigned long _millisBTN2 = millis();
  DateTime now;
  if ((_millisBTN2 - lastPressBTN2) >= BTN_DELAY) {
    if (mode < 4) {
      tempT += tempStep ;
      }
    if (mode == 5) {
      timeUpdatedPlus = 1;
    }
    lastPressBTN2 = _millisBTN2;
  }
}

/**
 * @brief Mise à jour des heures et minutes
 * 
 * @param PlusOrMinus 
 */
void timeUpdate(bool PlusOrMinus) {
  uint8_t oldSREG = SREG;
  DateTime now;
  uint8_t sw1, sw2, h, m, s;
  sw1 = digitalRead(4);
  sw2 = digitalRead(5);
  //
  now = rtc.now();
  h = now.hour();
  m = now.minute();
  s = now.second();
  if (!sw2) { // Hours  
    if (PlusOrMinus) {h++; led(BLUE, 1);} else {h--; led(YELLOW, 1);}
  }
  if (!sw1) { // minutes
    if (PlusOrMinus) {m++; led(BLUE, 1);} else {m--; led(YELLOW, 1);}
  }
  //
  rtc.adjust(DateTime(2021, 12, 31, h, m, s));
  cli();
  timeUpdatedPlus = 0;
  timeUpdatedMinus = 0;
  SREG = oldSREG;
  led(BLUE, 0);
  led(YELLOW, 0);
}

void prog1() {
  static unsigned long lastChange = 0;
  unsigned long _millis = millis();
  const uint8_t nb_values = 2;
  /*
  Serial.print(lastChange);
  Serial.print(" ");
  Serial.println(_millis);
  */
  if ((_millis - lastChange) < 60000) return ;

  const float prog1Times[]   = {5.10, 6.50, 11.0, 14.0, 19.0, 22.0};
  const float prog1Degrees[] = {19.0, 17.0, 19.0, 17.0, 19.0, 16.5};
  // below, for tests
  // const float prog1Times[]   = {0.29, 0.31};
  // const float prog1Degrees[] = {19.0, 17.0};
  float xTime;
  DateTime now;
  now = rtc.now();
  xTime = now.hour() + (now.minute() / 100.0);
  // Serial.println(xTime);
  for (uint8_t i = 0; i < nb_values; i++) {
    if (xTime == prog1Times[i]) {
      tempT = prog1Degrees[i];
      lastChange = _millis;
    }
  }
}

/**
 * @brief We use the 6 positions switch for the 6 different modes
 * 
 */
char getMode() {
  int16_t mode_position;
  int16_t pos;
  int16_t modes[] = {77, 184, 313, 571, 797, 965};
  // char mode;  short integer, 0 -> 5, for the six modes (volatile & global)
  mode_position = analogRead(A2);
  for (i = 0; i < 6 ; i++) {
    pos = modes[i] - mode_position;
    if (abs(pos) < 30) {
      mode = i;
      break;
    }
  }
  if (i == 6) {
    Serial.print(pos);
    Serial.print(" ");
    Serial.println(mode_position);
    }
  return(i);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(PLUS_PIN, INPUT_PULLUP);
  pinMode(MINUS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN1), btn1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN2), btn2, FALLING);

  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, LOW) ; // SSR Relay
  //
  pinMode(A2, INPUT); // 6 positions switch with resistor dividers
  //
  pinMode(4, INPUT_PULLUP); // btn1
  pinMode(5, INPUT_PULLUP); // btn2
  pinMode(6, OUTPUT); // Yellow LED
  pinMode(7, OUTPUT); // Blue LED
  digitalWrite(YELLOW, LOW);
  digitalWrite(BLUE, LOW);
  //
  Serial.println("ok, paré...");
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  //Set the color - always use white despite actual display color
  display.setTextColor(WHITE);
  //Set the font size
  display.setTextSize(2);
  //Set the cursor coordinates
  display.setCursor(0,0);
  display.println("...init...");
  display.display();

  // Temp sensor
  sensors.begin();
  delay(500);
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  if (!sensors.getAddress(insideThermometer, 0)){
    Serial.println("Unable to find address for Device 0");
  }
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();
  sensors.setResolution(insideThermometer, TEMPERATURE_PRECISION);

  // RTC
  rtc.begin();
  timeUpdatedPlus = 0;
  timeUpdatedMinus = 0;
  // ex. rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  // set date and time at compilation time
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  /* */
  delay(200);
  blinkLeds();
}

void loop() {
  float tempC; // Current temperature
  uint8_t sw1, sw2;
  bool on = 1, off = 0;
  DateTime now;
  float _tempT;
  uint8_t oldSREG = SREG;
  //
  sw1 = digitalRead(4);
  sw2 = digitalRead(5);
  /*
  Serial.print("sw1 : ");
  Serial.print(sw1);
  Serial.print("   sw2 : ");
  Serial.println(sw2);
  */
  mode = getMode();
  switch(mode) {
    // mode 0 is the more basic mode
    case 0:
      if (!sw2) {tempStep = 1.0; led(BLUE, 1); led(YELLOW, 0);}
      if (!sw1) {tempStep = 0.2; led(YELLOW, 1);led(BLUE, 0);}
      if (sw1 && sw2) {tempStep = 0.5; led(YELLOW, 0);led(BLUE, 0);}
      break;
    case 1:
      // 14 Deg
      tempT = 14.0;
      break;
    case 2:
      // 19 Deg
      tempT = 19.0;
      break;
    case 3:
      // prog 1
      prog1();
      break;
    case 4:
      // prog 2
      break;
    case 5:
      // clock setting
      break;
    default:
      {tempStep = 0.2; led(YELLOW, 0);led(BLUE, 0);}
      break;
  }

  /* */
  sensors.requestTemperatures();
  tempC = sensors.getTempC(insideThermometer);
  if (tempC < 0) tempC = 18.0 ;
  /* see at 13:45 https://www.youtube.com/watch?v=55YEZppz7p4 */
  cli();
  _tempT = tempT;
  SREG = oldSREG;

  if (tempC <= (_tempT - HYSTERESIS)) {
    if (!heatingActive) {
      heating(on) ;
    }
  } else if (tempC >= (_tempT + HYSTERESIS)) {
    if (heatingActive) {
      heating(off) ;
    }
  }
  //
  if (timeUpdatedPlus) {timeUpdate(1); }
  if (timeUpdatedMinus) {timeUpdate(0); }
  //
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.print("T : ");
  display.println(tempC);

  display.print(" -> ");
  if (heatingActive) {
    display.print(_tempT);
    display.println("*");
  } else {
    display.println(_tempT);
  }
  /* */
  #ifdef X_RTC
  display.setTextSize(2);
  display.setCursor(10, 33);
  now = rtc.now();
  display.print(now.hour(), DEC);
  display.print(':');
  display.print(now.minute(), DEC);
  display.print(':');
  display.print(now.second(), DEC);
  display.println();
  #endif
  /* */
  // display.setCursor(0, 48);
  display.print("mode : ");
  display.print((int)mode);
  display.display();
  delay(200);
}


/* informations annexes :
multiple OLED displays withnthe same adress :
Multiple OLED SSD1306 Displays using 2IC : https://forum.arduino.cc/index.php?topic=248663.0
https://bitbanksoftware.blogspot.com/2019/01/controlling-lots-of-oled-displays-with.html
https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
TCA9548A I2C Multiplexer : https://www.youtube.com/watch?v=vV42fCpmCFg
*/