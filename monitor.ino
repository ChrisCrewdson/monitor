#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <SPI.h>
#include <string.h>

// CO2
#define __AVR__
#include <SC16IS750.h>
#include <NDIRZ16.h>

// Humidity
#include <DHT.h>

// Pressure
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Temperature
#include <Adafruit_MCP9808.h>

// OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// RGB LED
#include <Adafruit_NeoPixel.h>

/*
 Monitor

 * Sensors
 ** DHT11 (Humidity)
 ** Data - pin 12
 *
 ** BMP280 (Pressure)
 ** I2C - address ????
 * 
 ** MCP9808 (Temperature)
 ** I2C - address 0x18
 * 
 ** MH-Z16 (CO2)
 ** I2C - address 0x9A
  
 * Output
 ** OLED
 ** I2C - address ????
 * 
 ** RGB LED
 ** Data - pin 10
 * 
 * SD card
 ** SPI
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)
 *
 * SPI
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13

 */

// Humidity: DHT11
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pressure: BMP280
// I2C address 
Adafruit_BMP280 bmp;

// Temperature: MCP9808
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// CO2: MH-Z16
SC16IS750 i2cuart = SC16IS750(SC16IS750_PROTOCOL_I2C,SC16IS750_ADDRESS_BB);
NDIRZ16 mySensor = NDIRZ16(&i2cuart);

// SD card (datalogger)
const int cardChipSelect = 4;

// OLED display
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13

Adafruit_SSD1306 display = Adafruit_SSD1306();

// RGB LED (WS2812B)
// pin 10
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 10, NEO_GRB + NEO_KHZ800);


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  i2cuart.begin(9600);

  if (i2cuart.ping()) {
    Serial.println("SC16IS750 found.");
    Serial.println("Wait 10 seconds for sensor initialization...");
    delay(10000);
  } else {
      Serial.println("SC16IS750 not found.");
  }
  power(1);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay() ; // Clear the buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Monitor starting up");
  display.display();

  // Display buttons
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(cardChipSelect)) {
    Serial.println("SD card initialization failed or card not present");
    // don't do anything more:
    return;
  }
  Serial.println("SD card initialized");

  if(!bmp.begin()) {
    Serial.println("No BMP280 detected.");
  }

  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
  }

  pixels.begin();
  pixels.show();
}

void loop() {
  // make a string for assembling the data to log
  String dataString = "";

  // CO2 - MH-Z16
  // I2C address of 0x9A
  // ref: https://sandboxelectronics.com/?p=1126

  int ppm = 0;
  if (mySensor.measure()) {
    ppm = mySensor.ppm;
  } else {
//    Serial.println("CO2 sensor communication error");
  }
  dataString += String(ppm);
  dataString += ",";


  // Pressure - BMP280
  // connection: i2c
  // ref: https://learn.adafruit.com/adafruit-bmp183-spi-barometric-pressure-and-altitude-sensor

  float pressure = 0.0;
//  float temperature;
  
  pressure = bmp.readPressure();
//  temperature = bmp.readTemperature();

  dataString += String(pressure);
  dataString += ",";


  // Temperature - MCP9808
  // connection: I2C (address 0x18)
  // ref: https://learn.adafruit.com/adafruit-mcp9808-precision-i2c-temperature-sensor-guide

  float temperature = tempsensor.readTempC();
  dataString += String(temperature);
  dataString += ",";


  // Humidity - DHT11
  // connection: exclusive digital pin
  // ref: https://learn.adafruit.com/dht

  float humidity = 0.0;
  humidity = dht.readHumidity();
//  float temperature = dht.readTemperature();
  dataString += String(humidity);

//  pixels.setPixelColor(1, pixels.Color(255,0,0));
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
  if (humidity < 60.0) {
    // Green
    Serial.println("Humidity is green");
    pixels.setPixelColor(1, pixels.Color(0,255,0));
  } else if (humidity < 70.0) {
    // Yellow
    Serial.println("Humidity is yellow");
    pixels.setPixelColor(1, pixels.Color(255,255,0));
  } else {
    // Red
    Serial.println("Humidity is red");
    pixels.setPixelColor(1, pixels.Color(255,0,0));
  }
  pixels.show();
//  delay(1000);
//  pixels.setPixelColor(1, 0);
//  pixels.show();

  // open the file (only one file can be open at a time)
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    // file not available
    Serial.println("error opening datalog.txt");
  }

  Serial.print("CO2 (ppm):");
  Serial.print(ppm);
  Serial.print(" Pressure (Pa):");
  Serial.print(pressure);
  Serial.print(" Temperature (C):");
  Serial.print(temperature);
  Serial.print(" Humidity (%):");
  Serial.println(humidity);

  // OLED display
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("CO2:");
  display.println(ppm);
  display.print("Pressure:");
  display.println(pressure);
  display.print("Temperature:");
  display.println(temperature);
  display.print("Humidity:");
  display.println(humidity);
  display.display(); // actually display all of the above

  delay(1000);
  yield();
}

// Power control function for NDIR sensor. 1=ON, 0=OFF
void power (uint8_t state) {
  i2cuart.pinMode(0, INPUT); // set up the power control pin

  if (state) {
    i2cuart.pinMode(0, INPUT); // turn on the power of MH-Z16
  } else {
    i2cuart.pinMode(0, OUTPUT);
    i2cuart.digitalWrite(0, 0); // turn off the power of MH-Z16
  }
}









