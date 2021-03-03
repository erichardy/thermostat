/*
multiple OLED displays withnthe same adress :
Multiple OLED SSD1306 Displays using 2IC : https://forum.arduino.cc/index.php?topic=248663.0
https://bitbanksoftware.blogspot.com/2019/01/controlling-lots-of-oled-displays-with.html
TCA9548A I2C Multiplexer : https://www.youtube.com/watch?v=vV42fCpmCFg
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
float tempT = 19.5; // Target temperature

#define PLUS_PIN 3
#define MINUS_PIN 2
#define HEATING_PIN 7 // relay pin

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

void heating(bool OnOff) {
  // turn ON or OFF the heating
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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //
  pinMode(PLUS_PIN, INPUT_PULLUP);
  pinMode(MINUS_PIN, INPUT_PULLUP);
  pinMode(HEATING_PIN, OUTPUT);
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
  // put your main code here, to run repeatedly:
  // scanI2C();
  display.clearDisplay();
  display.setCursor(0,0);
  
  sensors.requestTemperatures();
  tempC = sensors.getTempC(insideThermometer);
  //
  display.print("T : ");
  display.println(tempC);
  display.print(" -> ");
  display.println(tempT);
  display.display();
  delay(1500);
}