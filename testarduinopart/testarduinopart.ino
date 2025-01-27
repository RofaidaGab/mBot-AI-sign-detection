#include <MeMCore.h>

MeDCMotor leftMotor(M1);
MeDCMotor rightMotor(M2);
MeLineFollower lineSensor(PORT_2);

int baseSpeed = 100;
int maxSpeed = 160;             // New speed after a turn
int turnSpeed30 = 120;          // Speed for 30 cm turn
int turnSpeed100 = 90;          // Speed for 100 cm turn
int lastKnownDirection = 0;     // 1 = Left, 2 = Right
bool firstTurnDone = false;     // Tracks if the first sharp turn is complete
bool carStarted = false;        // Car starts only when "start" command is sent

void setup() {
  Serial.begin(9600);
  Serial.println("Waiting for 'start' command to begin...");
}

void loop() {
  // Check for serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command == "start") {
      carStarted = true;
      Serial.println("Car started. Following the line...");
    } else if (command == "stop") {
      carStarted = false;
      stopCar();
      Serial.println("Car stopped.");
    }
  }

  // Proceed only if the car is started
  if (!carStarted) {
    delay(50);  // Small delay to reduce idle loop spinning
    return;
  }

  int sensorValue = lineSensor.readSensors();

  if ((sensorValue & 0b11) == 0b11) {  // Both sensors detect white
    Serial.println("Both sensors detect white. Performing a sharp turn...");

    // Perform sharp turn based on the last known direction
    if (!firstTurnDone) {
      if (lastKnownDirection == 1) {
        sharpTurnLeft(30);  // 30 cm turn
      } else {
        sharpTurnRight(30); // 30 cm turn
      }
      firstTurnDone = true;  // Mark first turn as complete
    } else {
      if (lastKnownDirection == 1) {
        sharpTurnLeft(100);  // 100 cm turn
      } else {
        sharpTurnRight(100); // 100 cm turn
      }
    }

    // Increase speed after turn
    baseSpeed = maxSpeed;
    Serial.print("Speed increased to: ");
    Serial.println(baseSpeed);
  } else {
    // Update last known direction and move accordingly
    if ((sensorValue & 0b01) == 0b01) {  // Left sensor on black
      turnSlightLeft();
      lastKnownDirection = 1;
    } else if ((sensorValue & 0b10) == 0b10) {  // Right sensor on black
      turnSlightRight();
      lastKnownDirection = 2;
    } else {
      moveForward();  // Both sensors on black
    }
  }

  delay(20);  // Small delay for stability
}

// Function to move forward
void moveForward() {
  leftMotor.run(-baseSpeed);
  rightMotor.run(baseSpeed);
  Serial.println("Moving forward...");
}

// Function to turn slightly left
void turnSlightLeft() {
  leftMotor.run(-(baseSpeed / 2)); // Slow down left motor
  rightMotor.run(baseSpeed);       // Keep right motor at base speed
  Serial.println("Turning slightly left...");
}

// Function to turn slightly right
void turnSlightRight() {
  leftMotor.run(-baseSpeed);       // Keep left motor at base speed
  rightMotor.run(baseSpeed / 2);   // Slow down right motor
  Serial.println("Turning slightly right...");
}

// Function for sharper left turn
void sharpTurnLeft(int diameter) {
  int speed = (diameter == 30) ? turnSpeed30 : turnSpeed100;
  int duration = (diameter == 30) ? 800 : 1200; // Adjusted durations for 30 cm and 100 cm turns

  Serial.print("Making a sharp left turn with diameter ");
  Serial.print(diameter);
  Serial.println(" cm...");

  leftMotor.run(0);              // Stop left motor briefly
  rightMotor.run(speed);         // Right motor moves forward
  delay(duration);               // Duration depends on diameter
  stopCar();
}

// Function for sharper right turn
void sharpTurnRight(int diameter) {
  int speed = (diameter == 30) ? turnSpeed30 : turnSpeed100;
  int duration = (diameter == 30) ? 800 : 1200; // Adjusted durations for 30 cm and 100 cm turns

  Serial.print("Making a sharp right turn with diameter ");
  Serial.print(diameter);
  Serial.println(" cm...");

  leftMotor.run(-speed);         // Left motor moves forward
  rightMotor.run(0);             // Stop right motor briefly
  delay(duration);               // Duration depends on diameter
  stopCar();
}

// Function to stop the car
void stopCar() {
  leftMotor.run(0);
  rightMotor.run(0);
  Serial.println("Car stopped.");
}
