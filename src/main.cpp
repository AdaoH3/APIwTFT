/***************************************************************************
  This is a library for several Adafruit displays based on ST77* drivers.

  Works with the Adafruit ESP32-S2 TFT Feather
    ----> http://www.adafruit.com/products/5300

  Check out the links above for our tutorials and wiring diagrams.

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 **************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Define the server and path for the API
#define SERVER "api.quavergame.com"
#define PATH "/v1/users/609092/achievements"

// Enter your WiFi SSID and password
char ssid[] = "Hotspot1";    // your network SSID (name)
char pass[] = "NotYours"; 

// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void printWifiStatus();
int updateAchievements();
int unlockedCount = 0;
int tempCount;
int achieveCountFinal = 0;

void setup(void) 
{
  Serial.begin(9600);
  
  Serial.print("Attempting to connect to Wifi");

  // Attempt to connect to Wifi network:
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to WiFi");
  printWifiStatus();
  
  Serial.print(F("Hello! Feather TFT Test"));

  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  tft.init(135, 240);
  tft.setRotation(3);

  // Turn on backlight
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // Make the screen black
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(120,67.5);
  tft.setTextWrap(true);
  
  Serial.println("\nStarting connection to server...");
  // If you get a connection, report back via serial:
  tft.setCursor(0,0);
  tft.setTextSize(3);
  tft.print("Achievements");
}

uint32_t bytes = 0;

void loop() 
{
  tft.fillRect(105, 50, 50, 50, ST77XX_BLACK);
  tft.setCursor(120,67.5); //Center of the Screen

  achieveCountFinal = updateAchievements();

  if (achieveCountFinal > 0)
  {
    tft.print(achieveCountFinal);
  }
  
  delay(2000);
}

void printWifiStatus() {
  // Print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

int updateAchievements()
{
  WiFiClientSecure client;
  client.setInsecure(); 

  if (client.connect(SERVER, 443)) 
  {
    //Serial.println("Connected to server"); This is for Debugging
    // Make an HTTP request:
    client.println("GET " PATH " HTTP/1.1");
    client.println("Host: " SERVER);
    client.println("Connection: close"); 
    client.println();
  }
  else
  {
    //return 2; // Connection failed
  }

  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
   //return 3; // Unexpected HTTP response
  }

  // Wait until we get a double blank line
  client.find("\r\n\r\n", 4);
  client.println();

  DynamicJsonDocument filter(48);
  filter["achievements"][0]["unlocked"] = true;

  DynamicJsonDocument doc(1536);

  DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return NULL; // JSON deserialization failed
  }

  for (JsonObject achievement : doc["achievements"].as<JsonArray>()) {

    bool achievement_unlocked = achievement["unlocked"];

    if (achievement_unlocked) {
      unlockedCount++;
    }
  }

  tempCount = unlockedCount;
  unlockedCount = 0;

  return tempCount; // Return the count of unlocked achievements
}