#include <BH1750.h>
#include <Wire.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(9600);
  delay(100);
  // Initialize the I2C bus (BH1750 library doesn't do this automatically)
  Wire.begin(22, 21);
  lightMeter.begin();
  delay(100);
}

void loop() {
  float lux = lightMeter.readLightLevel();
  delay(1000);
  Serial.println(""); // Sacrificial Data since Arduino (or esp32) sends the data so badly, that it very often just sends the very first character, pauses for no reason and then sends the rest. This causes a receiver to receive the data in at least 2 lines
  Serial.print("lux: ");
  Serial.println(lux);
}
