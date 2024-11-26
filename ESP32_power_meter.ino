#include "Secrets.h"
#include "Config.h"
#include <IntervalJob.h>
#include <SensorData.h>
#include <WiFi.h>
#include <MQTT.h>
#include <Adafruit_ADS1X15.h>
#include "Algorithms.h"

Adafruit_ADS1115 voltageSensor;
Adafruit_ADS1115 currentSensor;

WiFiClient wifiClient;
MQTTClient mqttClient;

SensorData<float> powerDataReadings(20, 6);


IntervalJob setConnectionJob(1000, []() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqttClient.connected()){
      mqttClient.connect(DEVICE_NAME, MQTT_USER, MQTT_PASS);
    }
  }
  mqttClient.loop();
});

IntervalJob collectPowerDataJob(3000, []() {
  powerDataReadings.addReadings(
    calculatePowerData(&voltageSensor, &currentSensor)
  );
});


bool publishPowerData() {
  if (!mqttClient.connected()){
    return false;
  }

  float* values = powerDataReadings.getAverage();
  if (values) {
    mqttClient.publish("home/" DEVICE_NAME "/active_power", String(values[0]));
    mqttClient.publish("home/" DEVICE_NAME "/apparent_power", String(values[1]));
    mqttClient.publish("home/" DEVICE_NAME "/reactive_power", String(values[2]));
    mqttClient.publish("home/" DEVICE_NAME "/voltage_rms", String(values[3]));
    mqttClient.publish("home/" DEVICE_NAME "/current_rms", String(values[4]));
    mqttClient.publish("home/" DEVICE_NAME "/frequency", String(values[5]));
  }

  short wifi_signal = WiFi.RSSI();
  mqttClient.publish("home/" DEVICE_NAME "/wifi_signal", String(wifi_signal));

  return true;
}

IntervalJob publishPowerDataJob(MQTT_POWER_DATA_PUBLISH_INTERVAL, publishPowerData, 1000);

IntervalJob publishCurrentDataJob(3000, []() {
  if (!mqttClient.connected()){
    return;
  }

  String sampleVoltageStr = "[";
  String sampleCurrentStr = "[";

  for (int i = SAMPLE_START; i < SAMPLE_END - 1; i++) {
    sampleVoltageStr += String(VOLTAGE_ARRAY[i]) + ", ";
    sampleCurrentStr += String(CURRENT_ARRAY[i]) + ", ";
  }

  sampleVoltageStr += String(VOLTAGE_ARRAY[SAMPLE_END - 1]) + "]";
  sampleCurrentStr += String(CURRENT_ARRAY[SAMPLE_END - 1]) + "]";

  float* values = powerDataReadings.getLastReadings();
  if (values) {
    mqttClient.publish("home/" DEVICE_NAME "/current/active_power", String(values[0]));
    mqttClient.publish("home/" DEVICE_NAME "/current/apparent_power", String(values[1]));
    mqttClient.publish("home/" DEVICE_NAME "/current/reactive_power", String(values[2]));
    mqttClient.publish("home/" DEVICE_NAME "/current/voltage_rms", String(values[3]));
    mqttClient.publish("home/" DEVICE_NAME "/current/current_rms", String(values[4]));
    mqttClient.publish("home/" DEVICE_NAME "/current/frequency", String(values[5]));
    mqttClient.publish("home/" DEVICE_NAME "/current/sample_voltage", sampleVoltageStr);
    mqttClient.publish("home/" DEVICE_NAME "/current/sample_current", sampleCurrentStr);
  }
});


void setup() {
  Serial.begin(9600);

  delay(1000);

  Wire.begin();
  Wire.setClock(400000);

  voltageSensor.setGain(GAIN_TWO);
  currentSensor.setGain(GAIN_TWO);
  voltageSensor.setDataRate(RATE_ADS1115_860SPS);
  currentSensor.setDataRate(RATE_ADS1115_860SPS);

  if (!voltageSensor.begin(0x49)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }
  if (!currentSensor.begin(0x48)) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  voltageSensor.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, true);
  currentSensor.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, true);

  WiFi.useStaticBuffers(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  mqttClient.begin(MQTT_BROKER, MQTT_PORT, wifiClient);
}

void loop() {
  IntervalJob::runAll();
}
