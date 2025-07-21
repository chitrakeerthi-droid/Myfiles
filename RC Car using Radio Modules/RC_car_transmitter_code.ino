#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nRF24L01 pins
#define CE_PIN   7 // Arduino Nano Pin 7
#define CSN_PIN  8 // Arduino Nano Pin 8

// Joystick pins
#define JOY1_X_PIN A0 // Steering (Left/Right)
#define JOY2_Y_PIN A3 // Throttle (Forward/Backward)

RF24 radio(CE_PIN, CSN_PIN); // Create an RF24 object

const byte addresses[][6] = {"00001", "00002"}; // Addresses for communication (match receiver's reading pipe)

// Structure to hold joystick data
struct JoyData {
  int xValue;
  int yValue;
};

void setup() {
  Serial.begin(9600);

  // Initialize nRF24L01
  if (!radio.begin()) {
    Serial.println("radio.begin() failed");
    while (1); // Halt if radio fails to initialize
  }
  radio.openWritingPipe(addresses[0]); // Open a pipe for writing (address must match receiver's reading pipe)
  radio.setPALevel(RF24_PA_LOW); // Set power amplifier level
  radio.stopListening(); // Stop listening as this is a transmitter
  Serial.println("Transmitter ready...");
}

void loop() {
  JoyData data;
  data.xValue = analogRead(JOY1_X_PIN); // Read X-axis of joystick 1 (steering)
  data.yValue = analogRead(JOY2_Y_PIN); // Read Y-axis of joystick 2 (throttle)

  bool success = radio.write(&data, sizeof(data));

  if (success) {
    Serial.print("Sent X: ");
    Serial.print(data.xValue);
    Serial.print(", Y: ");
    Serial.println(data.yValue);
  } else {
    Serial.println("Failed to send data");
  }

  delay(50); // Small delay between transmissions
}