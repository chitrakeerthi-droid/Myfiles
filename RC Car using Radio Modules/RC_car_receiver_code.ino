#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// nRF24L01 pins
#define CE_PIN   3 // Arduino Pin 3
#define CSN_PIN  8 // Arduino Pin 8

// L298N Motor Driver pins
#define ENA      9  // Enable/Speed for Motor A (PWM) - Left Motor
#define IN1      7  // Direction pin 1 for Motor A (Left)
#define IN2      6  // Direction pin 2 for Motor A (Left)

#define ENB      10 // Enable/Speed for Motor B (PWM) - Right Motor
#define IN3      5  // Direction pin 1 for Motor B (Right)
#define IN4      4  // Direction pin 2 for Motor B (Right)

RF24 radio(CE_PIN, CSN_PIN); // Create an RF24 object

const byte addresses[][6] = {"00001", "00002"}; // Addresses for communication

// Structure to hold joystick data
struct JoyData {
  int xValue; // X-axis of joystick 1 (steering)
  int yValue; // Y-axis of joystick 2 (throttle)
};

void setup() {
  Serial.begin(9600);
  
  // Initialize L298N pins as outputs
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Set initial motor state to stopped
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  // Initialize nRF24L01
  if (!radio.begin()) {
    Serial.println("radio.begin() failed");
    while (1); // Halt if radio fails to initialize
  }
  radio.openReadingPipe(1, addresses[0]); // Open a pipe for reading (address must match transmitter's writing pipe)
  radio.setPALevel(RF24_PA_LOW); // Set power amplifier level
  radio.startListening(); // Start listening for incoming data
  Serial.println("Receiver ready...");
}

void loop() {
  if (radio.available()) {
    JoyData data;
    radio.read(&data, sizeof(data));

    Serial.print("Received X: ");
    Serial.print(data.xValue);
    Serial.print(", Y: ");
    Serial.println(data.yValue);

    // Map joystick values (0-1023)
    // Joystick 1 X-axis (Steering): Left (0) to Right (1023), Center ~512
    // Joystick 2 Y-axis (Throttle): Forward (0) to Backward (1023), Center ~512
    // Inverted throttle: 0 (forward) maps to 255, 1023 (backward) maps to -255

    int throttle = map(data.yValue, 0, 1023, 255, -255); // Main speed (forward/backward)
    int steering = map(data.xValue, 0, 1023, -255, 255); // Steering amount (left/right)

    int leftMotorSpeed;
    int rightMotorSpeed;

    // Define a small dead zone for the joystick
    int joystickDeadZone = 50; // Adjust this value if your joystick is too sensitive around center

    // Check if throttle is within dead zone
    if (abs(throttle) < joystickDeadZone) {
      throttle = 0; // Stop the car if throttle is near center
    }

    // Check if steering is within dead zone
    if (abs(steering) < joystickDeadZone) {
      steering = 0; // No steering if joystick is near center
    }

    // --- Differential Steering Logic ---
    // This logic ensures that both motors are contributing to movement,
    // and one slows down or reverses relative to the other for turning.

    if (throttle >= 0) { // Moving Forward or Stopped
      if (steering >= 0) { // Turning Right (or going straight)
        rightMotorSpeed = throttle;
        leftMotorSpeed = throttle - steering; // Right motor slows down
      } else { // Turning Left
        rightMotorSpeed = throttle + steering; // Left motor slows down (since steering is negative)
        leftMotorSpeed = throttle;
      }
    } else { // Moving Backward
      if (steering >= 0) { // Turning Right (while reversing)
        rightMotorSpeed = throttle + steering; // Left motor slows down (less negative)
        leftMotorSpeed = throttle;
      } else { // Turning Left (while reversing)
        rightMotorSpeed = throttle;
        leftMotorSpeed = throttle - steering; // Right motor slows down (less negative, since steering is negative)
      }
    }
    
    // Ensure speeds are within valid PWM range (-255 to 255)
    leftMotorSpeed = constrain(leftMotorSpeed, -255, 255);
    rightMotorSpeed = constrain(rightMotorSpeed, -255, 255);

    Serial.print("Left Speed: ");
    Serial.print(leftMotorSpeed);
    Serial.print(", Right Speed: ");
    Serial.println(rightMotorSpeed);

    // Apply motor control
    setMotorSpeed(IN1, IN2, ENA, leftMotorSpeed); // Left Motor
    setMotorSpeed(IN3, IN4, ENB, rightMotorSpeed); // Right Motor
  }
}

// Helper function to control motor speed and direction
void setMotorSpeed(int in1Pin, int in2Pin, int enablePin, int speed) {
  if (speed > 0) { // Forward
    digitalWrite(in1Pin, HIGH);
    digitalWrite(in2Pin, LOW);
    analogWrite(enablePin, speed); // Speed is already constrained to 0-255
  } else if (speed < 0) { // Backward
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, HIGH);
    analogWrite(enablePin, abs(speed)); // Use absolute speed, already constrained to 0-255
  } else { // Stop
    digitalWrite(in1Pin, LOW);
    digitalWrite(in2Pin, LOW);
    analogWrite(enablePin, 0);
  }
}