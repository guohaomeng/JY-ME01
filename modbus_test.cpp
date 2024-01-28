#include <Arduino.h>
#include "./angle_sensor/angle_sensor.hpp"

SENSOR_MODE sensor_mode = MODBUS;
ANGLE_SENSOR angle_sensor(Serial2, sensor_mode);

/**
 * Example code
 * Serial port 2 is connected to the JY-ME01 angle sensor and serial port 1 is connected to the host computer.
 * The programme reads the angle sensor data from serial port 2 and sends it to the host computer via serial port 1
 */

void setup()
{
  Serial.begin(115200);
  delay(1000);
  // Always open the serial port connected to the angle sensor and select the correct baud rate before executing the initialisation procedure.
  Serial2.begin(921600);
  angle_sensor.init();
}

void loop()
{
// Read the angle sensor value once
  float angle = angle_sensor.modbus_getAngle(0x00); // One run on ESP32 is about 2077 us.
  Serial.printf("%.3f\n",angle);
  delay(100);
}
