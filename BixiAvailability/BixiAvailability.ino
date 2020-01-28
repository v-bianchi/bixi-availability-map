#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <LedControl.h>

#ifndef STASSID
#define STASSID "" // Enter network name here
#define STAPSK  "" // Enter password here
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

const char* host = "bixi-station-status-api.herokuapp.com";
const int httpPort = 80;

// Use WiFiClient class to create connection
WiFiClient client;

// LedControl(DataIn, CLK, LOAD, quantity_of_max72xx)
LedControl lc = LedControl(D0,D5,D6,1);

void ledControlReset() {
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set brightness*/
  lc.setIntensity(0,9);
  /* and clear the display */
  lc.clearDisplay(0);
}

void initializePins() {
  // initialize digital pins as output
  pinMode(D0, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
}

void connectToWifi() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  digitalWrite(D3, HIGH); // turn on LED to show device is connected to wifi
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void displayQuantity(unsigned short int digit, unsigned short int qty) {
  if (qty > 9) {
      lc.setChar(0, digit, '9', true); // If there are more than 9 bikes, display '9.'
    } else {
      lc.setChar(0, digit, qty, false);
    }
}

void setup() {
  initializePins();
  digitalWrite(D2, HIGH); // turn on LED to show device is on
  ledControlReset();
  connectToWifi();
}

void loop() {
  Serial.print("connecting to ");
  Serial.println(host);

  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  String url = "/stations/6138-6129-6137-6140/bikes"; // Change the URL to desired stations
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');

  // Serial.println("reply was:");
  Serial.println(line);
  // Serial.println("closing connection");

  /*
    Use this assistant
    https://arduinojson.org/v6/assistant/
    to adjust the memory allocation for the desired JSON response
  */
  const size_t capacity = JSON_ARRAY_SIZE(4) + JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + 223;
  DynamicJsonDocument doc(capacity);
  
  deserializeJson(doc, line);

  unsigned short int i;
  for(i = 0; i < 4; i++) {
    unsigned short int qty = doc["available_bikes_array"][i];
    displayQuantity(i, qty);
    // Serial.println("Parsed bike quantity: ");
    // Serial.println(qty);
  }
  
  delay(30000);
}
