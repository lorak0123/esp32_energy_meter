#define NEOPIXEL_PIN 10
#define NEOPIXEL_BRIGHTNESS 5

#define VOLTAGE_SENSOR_PIN A0
#define VOLTAGE_SENSOR_SCALE 0.01225490196078431372549019607843  // resistor divider 1M/5.1k * precision voltage reference 0.0625mV
#define CURRENT_SENSOR_PIN A1
#define CURRENT_SENSOR_SCALE 5.6818181818181818181818181818182e-4  // current driver 100A/50mA (2000:1) / resistor 220 Ohm * precision voltage reference 0.0625mV

#define MQTT_POWER_DATA_PUBLISH_INTERVAL 60000