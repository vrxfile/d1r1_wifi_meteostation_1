#include "DHT.h"
#include <BH1750FVI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LedControl.h"
#include <ESP8266WiFi.h>

// Wi-Fi point
const char* ssid     = "MGBot";
const char* password = "Terminator812";

// For ThingSpeak IoT
const String CHANNELID_0 = "104514";
const String WRITEAPIKEY_0 = "JE9ZA95KD73VR0IJ";

IPAddress thingspeak_server(184, 106, 153, 149);
const int httpPort = 80;

WiFiClient client;

#define SERVER_UPDATE_TIME 5000  // Update IoT data server
#define DS18B20_UPDATE_TIME 5000  // Update time for DS18B20 sensor
#define DHT11_UPDATE_TIME 5000    // Update time for DHT11 sensor
#define BH1750_UPDATE_TIME 5000   // Update time for BH1750 sensor
#define MOISTURE_UPDATE_TIME 5000 // Update time for moisture sensor
#define CONTROL_UPDATE_TIME 5000  // Update time for devices control

// DHT11 sensor
#define DHT11_PIN 0
DHT dht11(DHT11_PIN, DHT11, 15);

// DS18B20 sensor
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// BH1750 sensor
BH1750FVI LightSensor_1;

// Moisture sensor
#define MOISTURE_PIN A0

// LED matrix 8x8
LedControl LED_MATRIX = LedControl(13, 14, 12, 1);

// Data from sensors
float h1 = 0;
float t1 = 0;
float t2 = 0;
float m1 = 0;
float l1 = 0;

// Control paramters
int pump1 = 0;

// Timer counters
unsigned long timer_server = 0;
unsigned long timer_thingspeak = 0;
unsigned long timer_ds18b20 = 0;
unsigned long timer_dht11 = 0;
unsigned long timer_bh1750 = 0;
unsigned long timer_moisture = 0;
unsigned long timer_control = 0;

#define TIMEOUT 1000 // 1 second timout

// Smiles
const byte PROGMEM smile_sad[8] =
{
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10011001,
  B10100101,
  B01000010,
  B00111100
};
const byte PROGMEM smile_neutral[8] =
{
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10000001,
  B10111101,
  B01000010,
  B00111100
};
const byte PROGMEM smile_happy[8] =
{
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10100101,
  B10011001,
  B01000010,
  B00111100
};
const byte PROGMEM smile_warning[8] =
{
  B00011000,
  B00011000,
  B00111100,
  B00111100,
  B00011000,
  B00011000,
  B00000000,
  B00011000
};

// Moisture constants
#define MIN_MOISTURE 10
#define AVG_MOISTURE 25
#define MAX_MOISTURE 50

// Send IoT packet to ThingSpeak
void sendThingSpeakStream()
{
  Serial.print("Connecting to ");
  Serial.print(thingspeak_server);
  Serial.println("...");
  if (client.connect(thingspeak_server, httpPort))
  {
    if (client.connected())
    {
      Serial.println("Sending data to ThingSpeak server...\n");
      String post_data = "field1=";
      post_data = post_data + String(t1, 1);
      post_data = post_data + "&field2=";
      post_data = post_data + String(h1, 1);
      post_data = post_data + "&field3=";
      post_data = post_data + String(t2, 1);
      post_data = post_data + "&field4=";
      post_data = post_data + String(m1, 1);
      post_data = post_data + "&field5=";
      post_data = post_data + String(l1, 1);
      Serial.println("Data to be send:");
      Serial.println(post_data);
      client.println("POST /update HTTP/1.1");
      client.println("Host: api.thingspeak.com");
      client.println("Connection: close");
      client.println("X-THINGSPEAKAPIKEY: " + WRITEAPIKEY_0);
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.print("Content-Length: ");
      int thisLength = post_data.length();
      client.println(thisLength);
      client.println();
      client.println(post_data);
      client.println();
      delay(1000);
      timer_thingspeak = millis();
      while ((client.available() == 0) && (millis() < timer_thingspeak + TIMEOUT));
      while (client.available() > 0)
      {
        char inData = client.read();
        Serial.print(inData);
      }
      Serial.println();
      client.stop();
      Serial.println("Data sent OK!");
      Serial.println();
    }
  }
}

// Print sensors data to terminal
void printAllSensors()
{
  Serial.print("Air temperature: ");
  Serial.print(t1);
  Serial.println(" *C");
  Serial.print("Air humidity: ");
  Serial.print(h1);
  Serial.println(" %");
  Serial.print("Soil temperature: ");
  Serial.print(t2);
  Serial.println(" *C");
  Serial.print("Soil moisture: ");
  Serial.print(m1);
  Serial.println(" %");
  Serial.print("Ambient light intensity: ");
  Serial.print(l1);
  Serial.println(" lx");
  Serial.println("");
}

// Read DHT11 sensor
void readDHT11()
{
  h1 = dht11.readHumidity();
  t1 = dht11.readTemperature();
  if (isnan(h1) || isnan(t1))
  {
    Serial.println("Failed to read from DHT11 sensor!");
  }
}

// Read DS18B20 sensor
void readDS18B20()
{
  ds18b20.requestTemperatures();
  t2 = ds18b20.getTempCByIndex(0);
  if (isnan(t2))
  {
    Serial.println("Failed to read from DS18B20 sensor!");
  }
}

// Read BH1750 sensor
void readBH1750()
{
  l1 = LightSensor_1.getAmbientLight();
}

// Read MOISTURE sensor
void readMOISTURE()
{
  m1 = analogRead(MOISTURE_PIN);
}

// Control devices
void controlDEVICES()
{
  m1 = 15;
  // Smiles
  if (m1 < MIN_MOISTURE)
  {
    LED_MATRIX.setRow(0, 0, smile_sad[0]);
    LED_MATRIX.setRow(0, 1, smile_sad[1]);
    LED_MATRIX.setRow(0, 2, smile_sad[2]);
    LED_MATRIX.setRow(0, 3, smile_sad[3]);
    LED_MATRIX.setRow(0, 4, smile_sad[4]);
    LED_MATRIX.setRow(0, 5, smile_sad[5]);
    LED_MATRIX.setRow(0, 6, smile_sad[6]);
    LED_MATRIX.setRow(0, 7, smile_sad[7]);
  } else if ((m1 >= MIN_MOISTURE) && (m1 < AVG_MOISTURE))
  {
    LED_MATRIX.setRow(0, 0, smile_neutral[0]);
    LED_MATRIX.setRow(0, 1, smile_neutral[1]);
    LED_MATRIX.setRow(0, 2, smile_neutral[2]);
    LED_MATRIX.setRow(0, 3, smile_neutral[3]);
    LED_MATRIX.setRow(0, 4, smile_neutral[4]);
    LED_MATRIX.setRow(0, 5, smile_neutral[5]);
    LED_MATRIX.setRow(0, 6, smile_neutral[6]);
    LED_MATRIX.setRow(0, 7, smile_neutral[7]);
  } else if ((m1 >= AVG_MOISTURE) && (m1 < MAX_MOISTURE))
  {
    LED_MATRIX.setRow(0, 0, smile_happy[0]);
    LED_MATRIX.setRow(0, 1, smile_happy[1]);
    LED_MATRIX.setRow(0, 2, smile_happy[2]);
    LED_MATRIX.setRow(0, 3, smile_happy[3]);
    LED_MATRIX.setRow(0, 4, smile_happy[4]);
    LED_MATRIX.setRow(0, 5, smile_happy[5]);
    LED_MATRIX.setRow(0, 6, smile_happy[6]);
    LED_MATRIX.setRow(0, 7, smile_happy[7]);
  } else if (m1 >= MAX_MOISTURE)
  {
    LED_MATRIX.setRow(0, 0, smile_warning[0]);
    LED_MATRIX.setRow(0, 1, smile_warning[1]);
    LED_MATRIX.setRow(0, 2, smile_warning[2]);
    LED_MATRIX.setRow(0, 3, smile_warning[3]);
    LED_MATRIX.setRow(0, 4, smile_warning[4]);
    LED_MATRIX.setRow(0, 5, smile_warning[5]);
    LED_MATRIX.setRow(0, 6, smile_warning[6]);
    LED_MATRIX.setRow(0, 7, smile_warning[7]);
  }
  // Pump
}

// Main setup
void setup()
{
  // Init serial port
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Init Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Init DHT11
  dht11.begin();

  // Init DS18B20
  ds18b20.begin();

  // Init BH1750
  LightSensor_1.begin();
  LightSensor_1.setMode(Continuously_High_Resolution_Mode);

  // Init moisture
  pinMode(MOISTURE_PIN, INPUT);

  // Init LED matrix
  LED_MATRIX.shutdown(0, false);
  LED_MATRIX.setIntensity(0, 8);
  LED_MATRIX.clearDisplay(0);

  // First measurement and print data from sensors
  readDHT11();
  readDS18B20();
  readBH1750();
  readMOISTURE();
  printAllSensors();
}

// Main loop cycle
void loop()
{
  // Send data to IoT server timeout
  if (millis() > timer_server + SERVER_UPDATE_TIME)
  {
    printAllSensors();
    //sendThingSpeakStream();
    timer_server = millis();
  }

  // DHT11 sensor timeout
  if (millis() > timer_dht11 + DHT11_UPDATE_TIME)
  {
    readDHT11();
    timer_dht11 = millis();
  }

  // DS18B20 sensor timeout
  if (millis() > timer_ds18b20 + DS18B20_UPDATE_TIME)
  {
    readDS18B20();
    timer_ds18b20 = millis();
  }

  // BH1750 sensor timeout
  if (millis() > timer_bh1750 + BH1750_UPDATE_TIME)
  {
    readBH1750();
    timer_bh1750 = millis();
  }

  // Moisture sensor timeout
  if (millis() > timer_moisture + MOISTURE_UPDATE_TIME)
  {
    readMOISTURE();
    timer_moisture = millis();
  }

  // Control devices timeout
  if (millis() > timer_control + CONTROL_UPDATE_TIME)
  {
    controlDEVICES();
    timer_control = millis();
  }
}


