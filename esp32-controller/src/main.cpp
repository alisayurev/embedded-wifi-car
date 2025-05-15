/*
* main.cpp
* 
* ESP32 WiFi access point for remote control of STM32 
*   - creates WiFi soft AP named "ESP32-Car"
*   - listens on port 80 for command from client
*   - sends recieved commands to STM32 over I2C
* Alisa Yurevich, Tufts University, Spring 2025
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h> 

// wifi credentials for ESP access point
const char* ssid = "ESP32-Car"; 
const char* password = "12345678"; 

// i2c address of STM device
#define I2C_DEV_ADDR 0x0F 

// WiFi server listening on port 80 for incoming connections
// note: on wifi, esp32 draws ~200mA and needs a high impedance external battery
WiFiServer server(80); 

/*
* name:       i2c_setup
* purpose:    intializes ESP I2C Pins (SDA - 18, SCL - 19) with 400khz clock. 
*             sends byte to check STM prescense on the bus
*/
void i2c_setup() {
  Wire.begin(18, 19, 400000); // set up esp 32 i2c
  Wire.beginTransmission(I2C_DEV_ADDR); // address match check
  Wire.write(1); 
  Wire.endTransmission(true);
}

/*
* name:       setup
* purpose:    arduino set up function. initializes serial, i2c and wifi
*/
void setup() {
  Serial.begin(115200); // set up baudrate -> standard 115200
  delay(2000);
  i2c_setup(); 
  Wifi_setup();
}

/*
* name:      Wifi_setup
* purpose:   sets up ESP32 as soft AP. starts the TCP server and prints the AP
*            IP to serial.
*/
void Wifi_setup() {
  WiFi.softAP(ssid, password); 
  Serial.println(WiFi.softAPIP()); 
  server.begin(); 
}

/*
* name:       sendToSTM32
* purpose:    sends a null-terminated command string to STM32 over I2C.
*             prints success or error code to serial.
*/
void sendToSTM32(const char* command) {
  Wire.beginTransmission(I2C_DEV_ADDR); 
  Wire.write(command);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.print("successfully sent: ");
    Serial.println(command);
  } else {
    Serial.print("I2C error: ");
    Serial.println(error);
  }
}

/*
 * main loop:
 * - checks for incoming TCP clients
 * - reads command strings from connected clients until newline
 * - sends commands to STM32 over I2C
 * - handles client disconnects
 */
void loop() {
  WiFiClient client = server.available(); 

  if (client) { 
    while (client.connected()) {
      if (client.available()) { 
        String command = client.readStringUntil('\n');
        Serial.println("received: " + command); 
        sendToSTM32(command.c_str());
      } 
    }
    Serial.println("client disconnected");
    client.stop(); 
   }
}