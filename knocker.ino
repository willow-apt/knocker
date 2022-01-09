// Willow Bot "Knocker"

#include <PeakDetection.h> // import lib
#include <ESP8266WiFi.h>
#include <ArduinoHttpClient.h>

#define WIFI_SSID ""
#define WIFI_PW ""
#define WILLOWBOT_ADDRESS ""
#define WILLOWBOT_PORT 0

PeakDetection peakDetection; // create PeakDetection object

char willowbot_address[] = WILLOWBOT_ADDRESS;
int willowbot_port = WILLOWBOT_PORT;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, willowbot_address, willowbot_port);
int status = WL_IDLE_STATUS;

int lag =  64;
int threshold = 5;
double influence = 0.05;

unsigned long debounce_ms = 200;

unsigned long max_delay_between_knock_ms = 3000; // 3 seconds
unsigned long max_delay_between_knock_same_bin_ms = 750; // .75 second
unsigned long previous_knock_timestamp = 0;

String pattern = "";
int current_bin = 0;
bool knocking = false;

void setup() {
  Serial.begin(9600); // set the data rate for the Serial communication
  pinMode(A0, INPUT); // analog pin used to connect the sensor
  peakDetection.begin(lag, threshold, influence);

  // Init Wifi
  WiFi.begin(WIFI_SSID, WIFI_PW);
  Serial.print("Connecting to WiFi Network...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Init value
  previous_knock_timestamp = millis();
}

void loop() {
  double data = (double) analogRead(A0) / 512 - 1; // reads the value of the sensor and converts to a range between -1 and 1
  peakDetection.add(data); // adds a new data point
  int peak = peakDetection.getPeak(); // returns 0, 1 or -1
  double filtered = peakDetection.getFilt(); // moving average

  unsigned long current_time = millis();
  unsigned long elapsed = current_time - previous_knock_timestamp;
  if (elapsed > max_delay_between_knock_ms && knocking)
  {
    Serial.println("Resetting pattern.");
    pattern = "";
    current_bin = 0;
    knocking = false;
  }
  else if (elapsed > max_delay_between_knock_same_bin_ms && current_bin != 0)
  {
    pattern.concat(current_bin);
    current_bin = 0;
  }

  if (peak == 1)
  {
    if (elapsed < debounce_ms) {
      Serial.print("o");
      return;
    }

    Serial.print("\nDetected: ");
    ++current_bin;
    Serial.println(pattern + "" + current_bin);
    // Store this current peak as the previous knock
    previous_knock_timestamp = current_time;
    knocking = true;
  }

  if (pattern.length() == 3)
  {
    tellWillowbot(pattern);
    pattern = "";
    current_bin = 0;
    knocking = false;
  }
}

void tellWillowbot(String knockPattern)
{
  client.get("/secretknock/" + knockPattern);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
}
