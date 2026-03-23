#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

const int LED_PIN = 2;
const bool SERIAL_DEBUG = false;
const int LED_FREQUENCY = 5000;
const int LED_RESOLUTION = 8;

const char *DEVICE_NAME = "NeuroFit-ESP32";
const char *SERVICE_UUID = "12345678-1234-1234-1234-1234567890ab";
const char *CHARACTERISTIC_UUID = "abcdefab-1234-1234-1234-abcdefabcdef";

BLECharacteristic *commandCharacteristic = nullptr;
bool deviceConnected = false;
uint8_t signalValue = 0;

void logDebug(const char *message) {
  if (SERIAL_DEBUG) {
    Serial.println(message);
  }
}

void publishSignalValue() {
  Serial.println(signalValue);
}

void setSignalValue(uint8_t newValue) {
  if (signalValue == newValue) {
    return;
  }

  signalValue = newValue;
  ledcWrite(LED_PIN, signalValue);
  publishSignalValue();
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

  publishSignalValue();
}

void loop() {
  delay(100);
}
