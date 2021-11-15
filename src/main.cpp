/*
multiple OLED displays withnthe same adress :
Multiple OLED SSD1306 Displays using 2IC : https://forum.arduino.cc/index.php?topic=248663.0
https://bitbanksoftware.blogspot.com/2019/01/controlling-lots-of-oled-displays-with.html
https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/
TCA9548A I2C Multiplexer : https://www.youtube.com/watch?v=vV42fCpmCFg
*/
/*
Alemiorations a apporter :
1- 3 temperatures prereglees : 14, 17, 19 et des boutons pour y acceder directement
2- un interrupteur marche/arret sur le PCB

*/
/*
Pinout summary :
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
volatile float tempT = 15.0; // Target temperature
#define tempStep 0.2

// PLUS_PIN and MINUS_Pin are used for setting target temperature via buttons BTN1 and BTN2 buttons
#define PLUS_PIN 3
#define MINUS_PIN 2
#define BTN1 3
#define BTN2 2

#define BTN_DELAY 300
#define HEATING_PIN 8 // relay pin
#define HEATING_DELAY 40000000 // 40000000 uSec = 40sec.
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

void led(int color, bool onOff) {
  if (onOff) {
    digitalWrite(color, HIGH);
  } else {
    digitalWrite(color, LOW);
  }
}

void scanI2C() {
  byte error, address; //variable for error and I2C address
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);
}

// see : https://www.youtube.com/watch?v=55YEZppz7p4
void btn1() {
  static unsigned long lastPressBTN1 = 0;
  unsigned long _millisBTN1 = millis();
  if ((_millisBTN1 - lastPressBTN1) >= BTN_DELAY) {
      tempT -= tempStep ;
      }
  lastPressBTN1 = _millisBTN1;
}

void btn2() {
  static unsigned long lastPressBTN2 = 0;
  unsigned long _millisBTN2 = millis();
  if ((_millisBTN2 - lastPressBTN2) >= BTN_DELAY) {
      tempT += tempStep ;
      }
  lastPressBTN2 = _millisBTN2;
}


void testRTC() {
  // example from https://github.com/adafruit/RTClib/blob/master/examples/ds1307/ds1307.ino
  DateTime now;
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.println(now.day(), DEC);
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void presetTempT() {
  if (digitalRead(4)) {
    return;
  }
  int16_t preset_position;
  int16_t pos;
  int16_t presets[] = {77, 184, 313, 571, 797, 965};
  uint8_t temps[] = {12, 14, 16, 17, 18, 19};
  uint8_t newTempT, i;
  uint8_t oldSREG = SREG;
  // digitalRead(4) = sw1 = 0 => set tempT to one of the preset values
  preset_position = analogRead(A2);
  
  // volts = map(preset_position, 0, 1024, 0, 255);
  /* Values for volts :           19  45  77 142 198 240
     Values for preset_position : 77 184 313 571 797 965
     Corresponding tempertures :  12  14  16  17  18  19 degrees
  */
  for (i = 0; i < 6 ; i++) {
    pos = presets[i] - preset_position;
    if (abs(pos) < 10) {
      newTempT = temps[i];
      break;
    }
  }
  cli();
  tempT = float(newTempT);
  SREG = oldSREG;
  /* Serial.print(i);
  Serial.print(" ");
  // Serial.print(presets[i]);
  Serial.print(" ");
  Serial.print(preset_position);
  Serial.print(" ");
  Serial.println(pos); */
  /*
  display.setCursor(0, 51);
  display.print(preset_position);
  display.print(':');
  display.println(volts);
  */
}

void setup() {
  Serial.begin(9600);
  
  pinMode(PLUS_PIN, INPUT_PULLUP);
  pinMode(MINUS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN1), btn1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN2), btn2, FALLING);
  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, LOW) ;
  //
  pinMode(A2, INPUT);
  //
  Serial.print("UINT16_MAX ");
  Serial.println(UINT16_MAX);
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
  //
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
  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  // set date and time at compilation time
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  /* */
  DateTime now;
  now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.println(now.day(), DEC);
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  /* */

  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(YELLOW, LOW);
  digitalWrite(BLUE, LOW);
  delay(200);
}

void loop() {
  float tempC; // Current temperature
  int sw1, sw2;
  bool on = 1, off = 0;
  // int volts;
  DateTime now;
  // static bool LEDon = 0;
  float _tempT;
  uint8_t oldSREG = SREG;
  /*
  if (LEDon) {
    led(YELLOW, on);
  } else {
    led(YELLOW, off);
  }
  LEDon = !LEDon;
  */
  presetTempT();
  cli();
  _tempT = tempT;
  SREG = oldSREG;
  /* */
  sw1 = digitalRead(4); // we use preset value for tempT
  sw2 = digitalRead(5); // we enter in clock set mode
  Serial.print("sw1 : ");
  Serial.print(sw1);
  Serial.print("   sw2 : ");
  Serial.println(sw2);
  /* */
  /* */
  sensors.requestTemperatures();
  tempC = sensors.getTempC(insideThermometer);
  if (tempC < 0) tempC = 18.0 ;
  /* see at 13:45 https://www.youtube.com/watch?v=55YEZppz7p4 */
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
  
  
  display.display();
  delay(200);
}
