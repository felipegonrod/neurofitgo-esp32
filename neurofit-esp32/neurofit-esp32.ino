#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

const int LED_PIN = 2;
const bool SERIAL_DEBUG = false;
const int LED_FREQUENCY = 5000;
const int LED_RESOLUTION = 8;
const float TWO_PI_VALUE = 6.28318530718f;
const float OMEGA_MIN = 0.0f;
const float OMEGA_MAX = 6.0f;
const float PEDAL_GROUND_OFFSET = 0.0f;
const float PEDAL_SWING_AMPLITUDE = 15.0f * PI / 180.0f;
const float ELECTRODE_RAMP_DEGREES = 15.0f;
const unsigned long MOTOR_UPDATE_INTERVAL_MS = 50;

const char *DEVICE_NAME = "NeuroFit-ESP32";
const char *SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
const char *CHARACTERISTIC_UUID = "abcdefab-1234-1234-1234-abcdefabcdef";

BLECharacteristic *commandCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t signalValue = 0;
float omegaTarget = 0.0f;
float leftCrankAngle = 0.0f;
float rightCrankAngle = TWO_PI_VALUE * 0.5f;
float leftPedalAngle = PEDAL_GROUND_OFFSET;
float rightPedalAngle = PEDAL_GROUND_OFFSET;
uint8_t leftSuperiorActivation = 0;
uint8_t leftInferiorActivation = 0;
uint8_t rightSuperiorActivation = 0;
uint8_t rightInferiorActivation = 0;
unsigned long lastMotorUpdateMs = 0;
bool motorStreamActive = false;

void logDebug(const char *message) {
  if (SERIAL_DEBUG) {
    Serial.println(message);
  }
}

float wrapAngle(float angle) {
  float wrapped = fmodf(angle, TWO_PI_VALUE);

  if (wrapped < 0.0f) {
    wrapped += TWO_PI_VALUE;
  }

  return wrapped;
}

float getTargetVelocity(uint8_t sourceSignal) {
  return OMEGA_MIN + (static_cast<float>(sourceSignal) / 255.0f) * (OMEGA_MAX - OMEGA_MIN);
}

float toDegrees(float angle) {
  return wrapAngle(angle) * 180.0f / PI;
}

float getPlateauEnvelope(float angleDegrees, float startDegrees, float endDegrees) {
  if (angleDegrees < startDegrees || angleDegrees > endDegrees) {
    return 0.0f;
  }

  const float plateauStart = startDegrees + ELECTRODE_RAMP_DEGREES;
  const float plateauEnd = endDegrees - ELECTRODE_RAMP_DEGREES;

  if (angleDegrees <= plateauStart) {
    const float rampProgress = (angleDegrees - startDegrees) / ELECTRODE_RAMP_DEGREES;
    return sinf(constrain(rampProgress, 0.0f, 1.0f) * PI * 0.5f);
  }

  if (angleDegrees >= plateauEnd) {
    const float rampProgress = (endDegrees - angleDegrees) / ELECTRODE_RAMP_DEGREES;
    return sinf(constrain(rampProgress, 0.0f, 1.0f) * PI * 0.5f);
  }

  return 1.0f;
}

uint8_t getActivationForRange(float angleRadians, float startDegrees, float endDegrees) {
  const float angleDegrees = toDegrees(angleRadians);
  const float envelope = getPlateauEnvelope(angleDegrees, startDegrees, endDegrees);
  return static_cast<uint8_t>(roundf(signalValue * envelope));
}

void updateElectrodeActivations() {
  leftInferiorActivation = getActivationForRange(leftCrankAngle, 0.0f, 180.0f);
  leftSuperiorActivation = getActivationForRange(leftCrankAngle, 180.0f, 360.0f);
  rightInferiorActivation = getActivationForRange(rightCrankAngle, 0.0f, 180.0f);
  rightSuperiorActivation = getActivationForRange(rightCrankAngle, 180.0f, 360.0f);
}

void publishMotorState() {
  Serial.print("{\"signal\":");
  Serial.print(signalValue);
  Serial.print(",\"motors\":{\"omegaTarget\":");
  Serial.print(omegaTarget, 4);
  Serial.print(",\"left\":{\"crankAngle\":");
  Serial.print(leftCrankAngle, 4);
  Serial.print(",\"pedalAngle\":");
  Serial.print(leftPedalAngle, 4);
  Serial.print("},\"right\":{\"crankAngle\":");
  Serial.print(rightCrankAngle, 4);
  Serial.print(",\"pedalAngle\":");
  Serial.print(rightPedalAngle, 4);
  Serial.print("}},\"electrodes\":{\"left\":{\"superiores\":");
  Serial.print(leftSuperiorActivation);
  Serial.print(",\"inferiores\":");
  Serial.print(leftInferiorActivation);
  Serial.print("},\"right\":{\"superiores\":");
  Serial.print(rightSuperiorActivation);
  Serial.print(",\"inferiores\":");
  Serial.print(rightInferiorActivation);
  Serial.println("}}}");
}

void updateMotorState(float deltaSeconds) {
  omegaTarget = getTargetVelocity(signalValue);
  leftCrankAngle = wrapAngle(leftCrankAngle + omegaTarget * deltaSeconds);
  rightCrankAngle = wrapAngle(leftCrankAngle + PI);
  leftPedalAngle = -leftCrankAngle + PEDAL_GROUND_OFFSET + PEDAL_SWING_AMPLITUDE * sinf(leftCrankAngle);
  rightPedalAngle = -rightCrankAngle + PEDAL_GROUND_OFFSET + PEDAL_SWING_AMPLITUDE * sinf(rightCrankAngle);
  updateElectrodeActivations();
}

void setSignalValue(uint8_t newValue) {
  if (signalValue == newValue) {
    return;
  }

  signalValue = newValue;
  ledcWrite(LED_PIN, signalValue);
  omegaTarget = getTargetVelocity(signalValue);
  updateElectrodeActivations();

  if (signalValue == 0) {
    motorStreamActive = false;
  } else {
    motorStreamActive = true;
    lastMotorUpdateMs = millis();
  }

  publishMotorState();
}

void normalizeCommand(char *command) {
  size_t start = 0;
  size_t end = strlen(command);

  while (start < end && isspace(static_cast<unsigned char>(command[start]))) {
    start++;
  }

  while (end > start && isspace(static_cast<unsigned char>(command[end - 1]))) {
    end--;
  }

  size_t length = end - start;
  memmove(command, command + start, length);
  command[length] = '\0';

  for (size_t i = 0; i < length; i++) {
    command[i] = static_cast<char>(toupper(static_cast<unsigned char>(command[i])));
  }
}

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    logDebug("iPhone connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    logDebug("iPhone disconnected");
    BLEDevice::startAdvertising();
    logDebug("Advertising restarted");
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rawValue = pCharacteristic->getValue();
    if (rawValue.length() == 0) {
      return;
    }

    char command[32];
    rawValue.toCharArray(command, sizeof(command));
    normalizeCommand(command);

    if (command[0] == '\0') {
      return;
    }

    if (strcmp(command, "HIGH") == 0 || strcmp(command, "ON") == 0 || strcmp(command, "1") == 0) {
      setSignalValue(255);
    } else if (strcmp(command, "LOW") == 0 || strcmp(command, "OFF") == 0 || strcmp(command, "0") == 0) {
      setSignalValue(0);
    } else {
      char *endPointer = nullptr;
      long parsedValue = strtol(command, &endPointer, 10);

      if (endPointer != command && *endPointer == '\0') {
        if (parsedValue < 0) {
          parsedValue = 0;
        } else if (parsedValue > 255) {
          parsedValue = 255;
        }

        setSignalValue(static_cast<uint8_t>(parsedValue));
      } else if (SERIAL_DEBUG) {
        Serial.print("Unknown command: ");
        Serial.println(command);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  ledcAttach(LED_PIN, LED_FREQUENCY, LED_RESOLUTION);
  ledcWrite(LED_PIN, signalValue);
  lastMotorUpdateMs = millis();

  BLEDevice::init(DEVICE_NAME);

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService *service = server->createService(SERVICE_UUID);

  commandCharacteristic = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  commandCharacteristic->setCallbacks(new CommandCallbacks());
  commandCharacteristic->addDescriptor(new BLE2902());

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  publishMotorState();
}

void loop() {
  const unsigned long now = millis();

  if (!motorStreamActive) {
    return;
  }

  if (now - lastMotorUpdateMs >= MOTOR_UPDATE_INTERVAL_MS) {
    const float deltaSeconds = (now - lastMotorUpdateMs) / 1000.0f;
    lastMotorUpdateMs = now;
    updateMotorState(deltaSeconds);
    publishMotorState();
  }
}
