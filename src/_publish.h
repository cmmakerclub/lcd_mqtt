#include <MqttConnector.h>

#include <Arduino.h>
#include <SPI.h>

extern int relayPinState;
extern MqttConnector* mqtt;

extern int relayPin;
extern int LED_PIN;

extern char myName[];

static void readSensor();

extern String DEVICE_NAME;
extern int PUBLISH_EVERY;

double readThermocouple(int MAX6675_SCK,int MAX6675_CS,int MAX6675_SO);

void register_publish_hooks() {
  strcpy(myName, DEVICE_NAME.c_str());
  mqtt->on_prepare_data_once([&](void) {
    Serial.println("initializing sensor...");
  });

  mqtt->on_before_prepare_data([&](void) {
    readSensor();
  });

  mqtt->on_prepare_data([&](JsonObject *root) {
    JsonObject& data = (*root)["d"];
    JsonObject& info = (*root)["info"];
    data["myName"] = myName;
    data["millis"] = millis();
    data["relayState"] = relayPinState;
    data["temp1"] = readThermocouple(12,14,27);
    data["temp2"] = readThermocouple(12,26,27);

    data["updateInterval"] = PUBLISH_EVERY;
  }, PUBLISH_EVERY);

  mqtt->on_after_prepare_data([&](JsonObject * root) {
    /**************
      JsonObject& data = (*root)["d"];
      data.remove("version");
      data.remove("subscription");
    **************/
  });

  mqtt->on_published([&](const MQTT::Publish & pub) {
      Serial.println("Published.");
  });
}

static void readSensor() {
  // perform reading sensor
  Serial.println("Perform reading sensor...");
  Serial.print(readThermocouple(12,14,27));
  Serial.print(" C1 ");
  Serial.print(readThermocouple(12,26,27));
  Serial.println(" C2 ");
}

double readThermocouple(int MAX6675_SCK,int MAX6675_CS,int MAX6675_SO) {

  uint16_t v;
  pinMode(MAX6675_CS, OUTPUT);
  pinMode(MAX6675_SO, INPUT);
  pinMode(MAX6675_SCK, OUTPUT);

  digitalWrite(MAX6675_CS, LOW);
  delay(1);

  // Read in 16 bits,
  //  15    = 0 always
  //  14..2 = 0.25 degree counts MSB First
  //  2     = 1 if thermocouple is open circuit
  //  1..0  = uninteresting status

  v = shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);
  v <<= 8;
  v |= shiftIn(MAX6675_SO, MAX6675_SCK, MSBFIRST);

  digitalWrite(MAX6675_CS, HIGH);
  if (v & 0x4)
  {
    // Bit 2 indicates if the thermocouple is disconnected
    return NAN;
  }

  // The lower three bits (0,1,2) are discarded status bits
  v >>= 3;

  // The remaining bits are the number of 0.25 degree (C) counts
  return v*0.25;
}
