#include <WiFi.h>
#include <HTTPClient.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// Cloud endpoint (ThingSpeak example)
const char* server = "http://api.thingspeak.com/update";
const String apiKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

// MH-Z19 CO2 sensor on UART pins
HardwareSerial MySerial(2);  // Use UART2 on ESP32

// PMS5003 pins (example with SoftwareSerial)
SoftwareSerial pmsSerial(16, 17); // RX, TX

int co2 = 0;
int pm25 = 0;

void setup() {
  Serial.begin(115200);
  
  // Start UART for MH-Z19 sensor
  MySerial.begin(9600, SERIAL_8N1, 16, 17); // Adjust pins if needed

  // Start PMS5003 sensor
  pmsSerial.begin(9600);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  co2 = readCO2();
  pm25 = readPM25();
  
  Serial.printf("CO2: %d ppm, PM2.5: %d ug/m3\n", co2, pm25);
  
  if(WiFi.status() == WL_CONNECTED) {
    sendToCloud(co2, pm25);
  }
  
  delay(15000);  // send every 15 seconds
}

// Function to read CO2 from MH-Z19 (simple example)
int readCO2() {
  byte cmd[9] = {0xFF, 0x01, 0x86, 0, 0, 0, 0, 0, 0x79};
  byte response[9];
  MySerial.write(cmd, 9);
  delay(100);
  if (MySerial.available()) {
    MySerial.readBytes(response, 9);
    if (response[0] == 0xFF && response[1] == 0x86) {
      int co2Value = (int)response[2] * 256 + (int)response[3];
      return co2Value;
    }
  }
  return 0;
}

// Function to read PM2.5 from PMS5003 sensor (simplified)
int readPM25() {
  if (pmsSerial.available() >= 32) {
    uint8_t buffer[32];
    pmsSerial.readBytes(buffer, 32);
    if (buffer[0] == 0x42 && buffer[1] == 0x4D) {
      int pm25Value = buffer[12] * 256 + buffer[13];
      return pm25Value;
    }
  }
  return 0;
}

// Send data to ThingSpeak
void sendToCloud(int co2Val, int pm25Val) {
  HTTPClient http;
  String url = String(server) + "?api_key=" + apiKey + "&field1=" + co2Val + "&field2=" + pm25Val;
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.println("Data sent to cloud.");
  } else {
    Serial.println("Error sending data.");
  }
  http.end();
}
