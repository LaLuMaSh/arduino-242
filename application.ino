// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <Wire.h>

#include "rgb_lcd.h"
#include "Zanshin_BME680.h"  // Include the BME680 Sensor library

#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define PIN        6 // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 10 // Popular NeoPixel ring size
#define DELAYVAL 10*1000 // Time (in milliseconds) to pause between pixels

char ssid[] = "evg-15926";        // your network SSID (name)
char pass[] = "bnhs-2xn2-yzj5-uzlf";    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;
char serverAddress[] = "65.21.62.120";  // server address
int port = 8080;
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
WiFiClient client;
HttpClient httpClient = HttpClient(client, serverAddress, port);
BME680_Class BME680;  ///< Create an instance of the BME680 class
rgb_lcd lcd;

//if a specific component is connected
#define LEDs true
#define BME true
#define LCD true 
int abc = 10;
String serverName = "localhost";

void setup() {
  setupWifi();
  
  setupLCD();
  setupBME680();
  setupLEDs();
}


void loop() {
  saveDataToDb();
  displayDataLCD();
  
  displayDataLED();
  delay(DELAYVAL); 
}

void setupLCD() {
  if (!LCD) {
    return;
  }
  lcd.begin(16, 2);   
  lcd.setRGB(colorR, colorG, colorB);
}

void setupBME680() {
  if (!BME) {
    return;
  }
  
  Serial.print(F("Starting I2CDemo example program for BME680\n"));
  Serial.print(F("- Initializing BME680 sensor\n"));
  while (!BME680.begin(I2C_STANDARD_MODE)) {  // Start BME680 using I2C, use first device found
    Serial.print(F("-  Unable to find BME680. Trying again in 5 seconds.\n"));
    delay(5000);
  }  // of loop until device is located
  Serial.print(F("- Setting 16x oversampling for all sensors\n"));
  BME680.setOversampling(TemperatureSensor, Oversample16);  // Use enumerated type values
  BME680.setOversampling(HumiditySensor, Oversample16);     // Use enumerated type values
  BME680.setOversampling(PressureSensor, Oversample16);     // Use enumerated type values
  Serial.print(F("- Setting IIR filter to a value of 4 samples\n"));
  BME680.setIIRFilter(IIR4);  // Use enumerated type values
  Serial.print(F("- Setting gas measurement to 320\xC2\xB0\x43 for 150ms\n"));  // "�C" symbols
  BME680.setGas(320, 150);  // 320�c for 150 milliseconds
}
float altitude(const int32_t press, const float seaLevel = 1013.25);
float altitude(const int32_t press, const float seaLevel) {
  static float Altitude;
  Altitude =
      44330.0 * (1.0 - pow(((float)press / 100.0) / seaLevel, 0.1903));
  return (Altitude);
}



void setupWifi() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();
}

void setupLEDs() {
  if (!LEDs) {
    return;
  }
  pixels.begin();
}

void saveDataToDb() {
  if (!BME) {
    return;
  }
  static int32_t  temp, humidity, pressure, gas;  // BME readings
  
  static char     buf[16];                        // sprintf text buffer
  BME680.getSensorData(temp, humidity, pressure, gas);

  String tem= String((int8_t)(temp / 100)) + "." + (uint8_t)(temp % 100);
  String hum = String((int8_t)(humidity / 1000)) + "." + String((uint16_t)(humidity % 1000));
  String pres = String((int16_t)(pressure / 100)) + "." + String((uint8_t)(pressure % 100));

  Serial.println(pres + " " + hum + " " + tem);
  Serial.print("saved pressure: ");
  Serial.println(post("/api/v1/weather/air_pressure/", "{\"pressure\":" + pres + "}"));
  Serial.print("saved humidity: ");
  Serial.println(post("/api/v1/weather/humidity/", "{\"humidity\":" + hum + "}"));
  Serial.print("saved temperature: ");
  Serial.println(post("/api/v1/weather/temperature/", "{\"temperature\":" + tem + "}"));
}

void displayDataLCD() {
  if (!LCD) {
    return;
  }

  Serial.println("loading data for display...");
  String pressure = get("/api/v1/weather/air_pressure/latest");
  String humidity = get("/api/v1/weather/humidity/latest");
  String temperature = get("/api/v1/weather/temperature/latest");
  Serial.println("done loading data display: " + pressure + " " + humidity + " " +temperature );

  lcd.setCursor(0, 0);
  lcd.print("temp:" + temperature);
  lcd.setCursor(0, 1);
  lcd.print("p:" + pressure +" ");
  lcd.print("h:" + humidity);
}

void displayDataLED() {
  if (!LEDs) {
    return;
  }
  
  int indi = get("/api/v1/weather/indicator/").toInt();
  if (indi > 5) {
    Serial.println("number to big");
    return;
  }
  int amount = NUMPIXELS / 5 * indi;
  Serial.println("displaying LEDs: " + String(amount));
  pixels.clear();
  
  for(int i=0; i<amount; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 150, 0));
    pixels.show();
  }
}



// this method makes a HTTP connection to the server:
boolean httpRequest() {
  Serial.println(get("/api/v1/weather/air_pressure/"));
  Serial.println(post("/api/v1/weather/air_pressure/", "{\"pressure\":35.0}"));
  Serial.println(post("/api/v1/weather/humidity/", "{\"humidity\":20}"));
  Serial.println(post("/api/v1/weather/temperature/", "{\"temperature\":22.0}"));
}

String get(String url) {
  httpClient.get(url);

  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  if (statusCode != 200) {
    return "";
  }
  return response;
}

String post(String url, String content) {
  String contentType = "application/json";

  httpClient.post(url, contentType, content);
  
  // read the status code and body of the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  if (statusCode != 200) {
    return "";
  }
  return response;
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
