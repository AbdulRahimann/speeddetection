
#include <Arduino.h>

// ========================================
// PIN CONFIGURATION
// ========================================

const uint8_t SENSOR1_PIN = 18;
const uint8_t SENSOR2_PIN = 19;

// Measured distance between the sensor trigger points.
const float SENSOR_DISTANCE_M = 20.0f;

// Typical LM393 reflective sensor:
// HIGH = clear
// LOW  = object detected

const int TRIGGER_EDGE = FALLING;

// ========================================
// MEASUREMENT SETTINGS
// ========================================

const uint32_t TIMEOUT_US = 2000000UL;
const uint32_t DEBOUNCE_US = 3000UL;
const uint32_t CLEAR_STABLE_MS = 100;

const float MAX_EXPECTED_SPEED_KMH = 5.0f;

// ========================================
// INTERRUPT VARIABLES
// ========================================

// 0 = no sensor triggered
// 1 = sensor 1 triggered first
// 2 = sensor 2 triggered first

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile uint8_t firstSensor = 0;

volatile bool measurementComplete = false;

volatile uint32_t firstTime = 0;
volatile uint32_t secondTime = 0;

volatile uint32_t lastEdge1 = 0;
volatile uint32_t lastEdge2 = 0;

// ========================================
// SENSOR 1 INTERRUPT
// ========================================

void IRAM_ATTR sensor1ISR() {

  uint32_t now = micros();

  portENTER_CRITICAL_ISR(&mux);

  if ((uint32_t)(now - lastEdge1) >= DEBOUNCE_US) {

    lastEdge1 = now;

    if (firstSensor == 0) {

      firstSensor = 1;
      firstTime = now;

    }
    else if (firstSensor == 2 && !measurementComplete) {

      secondTime = now;
      measurementComplete = true;

    }
  }

  portEXIT_CRITICAL_ISR(&mux);
}

// ========================================
// SENSOR 2 INTERRUPT
// ========================================

void IRAM_ATTR sensor2ISR() {

  uint32_t now = micros();

  portENTER_CRITICAL_ISR(&mux);

  if ((uint32_t)(now - lastEdge2) >= DEBOUNCE_US) {

    lastEdge2 = now;

    if (firstSensor == 0) {

      firstSensor = 2;
      firstTime = now;

    }
    else if (firstSensor == 1 && !measurementComplete) {

      secondTime = now;
      measurementComplete = true;

    }
  }

  portEXIT_CRITICAL_ISR(&mux);
}

// ========================================
// RESET
// ========================================

void resetMeasurement() {

  portENTER_CRITICAL(&mux);

  firstSensor = 0;
  measurementComplete = false;

  firstTime = 0;
  secondTime = 0;

  portEXIT_CRITICAL(&mux);
}

// ========================================
// SETUP
// ========================================

void setup() {

  Serial.begin(115200);

  delay(1000);

  pinMode(SENSOR1_PIN, INPUT_PULLUP);
  pinMode(SENSOR2_PIN, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(SENSOR1_PIN),
    sensor1ISR,
    TRIGGER_EDGE
  );

  attachInterrupt(
    digitalPinToInterrupt(SENSOR2_PIN),
    sensor2ISR,
    TRIGGER_EDGE
  );

  Serial.println("SYSTEM_READY");
  Serial.println("ESP32 speed detector started");
}

// ========================================
// MAIN LOOP
// ========================================

void loop() {

  static bool waitingForClear = false;
  static uint32_t clearStart = 0;

  uint8_t first;
  bool complete;

  uint32_t tFirst;
  uint32_t tSecond;

  // Safely copy interrupt variables

  portENTER_CRITICAL(&mux);

  first = firstSensor;
  complete = measurementComplete;

  tFirst = firstTime;
  tSecond = secondTime;

  portEXIT_CRITICAL(&mux);

  // ------------------------------------
  // CALCULATE SPEED
  // ------------------------------------

  if (!waitingForClear && complete) {

    uint32_t elapsed = tSecond - tFirst;

    if (elapsed > 0) {

      float timeSeconds = elapsed / 1000000.0f;

      float speedMs =
        SENSOR_DISTANCE_M / timeSeconds;

      float speedKmh = speedMs * 3.6f;

      const char* direction =
        (first == 1) ? "1_TO_2" : "2_TO_1";

      if (speedKmh <= MAX_EXPECTED_SPEED_KMH) {

        Serial.printf(
          "SPEED,%s,%.3f,%.2f,%.2f\n",
          direction,
          elapsed / 1000.0f,
          speedMs,
          speedKmh
        );

      } else {

        Serial.printf(
          "OUT_OF_RANGE,%s,%.2f\n",
          direction,
          speedKmh
        );
      }

    } else {

      Serial.println("INVALID_MEASUREMENT");
    }

    waitingForClear = true;
    clearStart = 0;
  }

  // ------------------------------------
  // TIMEOUT
  // ------------------------------------

  if (!waitingForClear && first != 0 && !complete) {

    if ((uint32_t)(micros() - tFirst) > TIMEOUT_US) {

      Serial.println("TIMEOUT");

      waitingForClear = true;
      clearStart = 0;
    }
  }

  // ------------------------------------
  // WAIT FOR BOTH SENSORS TO CLEAR
  // ------------------------------------

  if (waitingForClear) {

    bool sensor1Clear =
      digitalRead(SENSOR1_PIN) == HIGH;

    bool sensor2Clear =
      digitalRead(SENSOR2_PIN) == HIGH;

    if (sensor1Clear && sensor2Clear) {

      if (clearStart == 0) {
        clearStart = millis();
      }

      if ((uint32_t)(millis() - clearStart) >= CLEAR_STABLE_MS) {

        resetMeasurement();

        waitingForClear = false;
        clearStart = 0;

        Serial.println("READY");
      }

    } else {

      clearStart = 0;
    }
  }
}
