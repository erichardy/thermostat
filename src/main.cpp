/*
multiple OLED displays withnthe same adress :
Multiple OLED SSD1306 Displays using 2IC : https://forum.arduino.cc/index.php?topic=248663.0
https://bitbanksoftware.blogspot.com/2019/01/controlling-lots-of-oled-displays-with.html
TCA9548A I2C Multiplexer : https://www.youtube.com/watch?v=vV42fCpmCFg
*/
/*
Alemiorations a apporter :
1- 3 temperatures prereglees : 14, 17, 19 et des boutons pour y acceder directement
2- un interrupteur marche/arret sur le PCB

*/

#include <Arduino.h>
// Include Wire Library for I2C
#include <Wire.h>

// Include Adafruit Graphics & OLED libraries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// see https://github.com/adafruit/Adafruit_SSD1306/blob/master/examples/ssd1306_128x32_i2c/ssd1306_128x32_i2c.ino
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET 4
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// related to DS18B20
#define DS18B20_PIN 9
#define TEMPERATURE_PRECISION 12
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;
float tempC; // Current temperature
float tempT = 15.0; // Target temperature
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

bool heatingActive = 0 ;

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

void setup() {
  Serial.begin(9600);
  //
  pinMode(PLUS_PIN, INPUT_PULLUP);
  pinMode(MINUS_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN1), btn1, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN2), btn2, FALLING);
  pinMode(HEATING_PIN, OUTPUT);
  digitalWrite(HEATING_PIN, LOW) ;
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
  //
  delay(500);
}

void loop() {
  bool on = 1, off = 0;
  // scanI2C();
  display.clearDisplay();
  display.setCursor(0,0);

  sensors.requestTemperatures();
  tempC = sensors.getTempC(insideThermometer);
  if (tempC < 0) tempC = 18.0 ;
  //
  display.print("T : ");
  display.println(tempC);
  display.print(" -> ");
  if (heatingActive) {
    display.print(tempT);
    display.println("*");
  } else {
    display.println(tempT);
  }
  display.display();

  if (tempC <= (tempT - HYSTERESIS)) {
    if (!heatingActive) {
      heating(on) ;
    }
  } else if (tempC >= (tempT + HYSTERESIS)) {
    if (heatingActive) {
      heating(off) ;
    }
  }
  delay(100);
}
