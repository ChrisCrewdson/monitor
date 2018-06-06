#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <SPI.h>
#include <string.h>

// CO2 sensor
#define __AVR__
#include <SC16IS750.h>
#include <NDIRZ16.h>

// Humidity sensor
#include <DHT.h>

// Pressure sensor
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Temperature sensor
#include <Adafruit_MCP9808.h>

// OLED display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// RGB LED display
#include <Adafruit_NeoPixel.h>

/*
 Monitor

 * Sensors
 ** CO2: MH-Z16 - I2C - address 0x9A
 ** Humidity: DHT11 - pin 12
 ** Pressure: BMP280 - I2C - address 0x77
 ** Temperature: MCP9808 - I2C - address 0x18
 * 
 * Output
 ** SD card - SPI - CS pin 4
 ** OLED - I2C - address ???
 ** RGB LEDs - pin 10
 *
 * SPI bus pinout
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 * 
 * I2C bus pinout
 ** SCL - pin 21
 ** SDA - pin 20
 */

// CO2: MH-Z16
SC16IS750 co2i2cUart = SC16IS750(SC16IS750_PROTOCOL_I2C,SC16IS750_ADDRESS_BB);
NDIRZ16 co2Sensor = NDIRZ16(&co2i2cUart);

// Humidity: DHT11
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Pressure: BMP280
Adafruit_BMP280 bmp;

// Temperature: MCP9808
Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();

// SD card data log
const int cardChipSelect = 4;

// OLED display
#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13
Adafruit_SSD1306 display = Adafruit_SSD1306();

// RGB LEDs (WS2812B)
#define RGB_LED_PIN 10
#define RGB_LED_PIXELS 4
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(RGB_LED_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t green = pixels.Color(0, 64, 0);
uint32_t yellow = pixels.Color(32, 32, 0);
uint32_t red = pixels.Color(64, 0, 0);


void setup() {
  // Open serial communications
  Serial.begin(9600);

  // CO2 sensor setup
  co2i2cUart.begin(9600);
  if (co2i2cUart.ping()) {
    Serial.println("SC16IS750 found.");
    Serial.println("Wait 10 seconds for sensor initialization...");
    delay(10000);
  } else {
      Serial.println("SC16IS750 not found.");
  }
  power(1);

  // Pressure sensor setup
  if(!bmp.begin()) {
    Serial.println("No BMP280 detected.");
  }

  // Temperature sensor setup
  if (!tempsensor.begin()) {
    Serial.println("Couldn't find MCP9808!");
  }

  // SD card setup
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(cardChipSelect)) {
    Serial.println("SD card initialization failed or card not present");
    // don't do anything more:
    return;
  }
  Serial.println("SD card initialized");

  // OLED display setup
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay() ; // Clear the buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Monitor starting up");
  display.display();

  // OLED display buttons
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // RGB LED setup
  pixels.begin();
  pixels.show(); // turn all the pixels off
}

void loop() {
  // CO2 - MH-Z16
  // I2C address of 0x9A
  // ref: https://sandboxelectronics.com/?p=1126

  int ppm = 0;
  if (co2Sensor.measure()) {
    ppm = co2Sensor.ppm;
  } else {
    Serial.println("CO2 sensor communication error");
  }

  Serial.print("CO2:");
  if (ppm < 2000.0) {
    // Green
    Serial.print("green");
    pixels.setPixelColor(0, green);
  } else if (ppm < 8000.0) {
    // Yellow
    Serial.print("yellow");
    pixels.setPixelColor(0, yellow);
  } else {
    // Red
    Serial.print("red");
    pixels.setPixelColor(0, red);
  }

  // Humidity - DHT11
  // connection: exclusive digital pin
  // ref: https://learn.adafruit.com/dht

  float humidity = 0.0;
  humidity = dht.readHumidity();
//  float temperature = dht.readTemperature();

  Serial.print(", Humidity:");
  if (humidity < 60.0) {
    // Green
    Serial.print("green");
    pixels.setPixelColor(1, green);
  } else if (humidity < 70.0) {
    // Yellow
    Serial.print("yellow");
    pixels.setPixelColor(1, yellow);
  } else {
    // Red
    Serial.print("red");
    pixels.setPixelColor(1, red);
  }

  // Pressure - BMP280
  // connection: i2c
  // ref: https://learn.adafruit.com/adafruit-bmp183-spi-barometric-pressure-and-altitude-sensor

  float pressure = bmp.readPressure();
//  float temperature = bmp.readTemperature();

  Serial.print(", Pressure:");
  if (pressure < 250000.0) {
    // Green
    Serial.print("green");
    pixels.setPixelColor(2, green);
  } else if (pressure < 350000.0) {
    // Yellow
    Serial.print("yellow");
    pixels.setPixelColor(2, yellow);
  } else {
    // Red
    Serial.print("red");
    pixels.setPixelColor(2, red);
  }

  // Temperature - MCP9808
  // ref: https://learn.adafruit.com/adafruit-mcp9808-precision-i2c-temperature-sensor-guide
  float temperature = tempsensor.readTempC();

  Serial.print(", Temperature:");
  if (temperature < 75.0) {
    // Green
    Serial.println("green");
    pixels.setPixelColor(3, green);
  } else if (temperature < 80.0) {
    // Yellow
    Serial.println("yellow");
    pixels.setPixelColor(3, yellow);
  } else {
    // Red
    Serial.println("red");
    pixels.setPixelColor(3, red);
  }
  
  // SD card writing
  // open the file (only one file can be open at a time)
  File dataFile = SD.open("datalog.csv", FILE_WRITE);

  // dataString is co2,humidity,pressure,temperature
  String dataString = "";
  dataString += String(ppm);
  dataString += ",";
  dataString += String(humidity);
  dataString += ",";
  dataString += String(pressure);
  dataString += ",";
  dataString += String(temperature);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  } else {
    // file not available
    Serial.println("error opening datalog file");
  }

  // OLED display
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("CO2:");
  display.println(ppm);
  display.print("Humidity:");
  display.println(humidity);
  display.print("Pressure:");
  display.println(pressure);
  display.print("Temperature:");
  display.println(temperature);
  display.display(); // actually display all of the above

  // show all the pixel colors that have been set
  pixels.show();

  Serial.print("CO2 (ppm):");
  Serial.print(ppm);
  Serial.print(", Humidity (%):");
  Serial.print(humidity);
  Serial.print(", Pressure (Pa):");
  Serial.print(pressure);
  Serial.print(", Temperature (C):");
  Serial.println(temperature);
  
  delay(1000);
  yield();
}

// Power control function for NDIR CO2 sensor. 1=ON, 0=OFF
void power (uint8_t state) {
  co2i2cUart.pinMode(0, INPUT); // set up the power control pin

  if (state) {
    co2i2cUart.pinMode(0, INPUT); // turn on the power of MH-Z16
  } else {
    co2i2cUart.pinMode(0, OUTPUT);
    co2i2cUart.digitalWrite(0, 0); // turn off the power of MH-Z16
  }
}

