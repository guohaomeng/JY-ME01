#include <Arduino.h>
#include "./angle_sensor/angle_sensor.hpp"

SENSOR_MODE sensor_mode = MODBUS;
ANGLE_SENSOR angle_sensor(Serial2, sensor_mode);

/**
 * 示例代码
 * 串口2连接JY-ME01角度传感器，串口1连接上位机
 * 程序从串口2读取角度传感器数据并通过串口1发送给上位机
 */

void setup()
{
  Serial.begin(115200);
  delay(1000);
  // 务必先打开与角度传感器相接的串口，选对波特率，再执行初始化程序
  Serial2.begin(921600);
  angle_sensor.init();
}

void loop()
{
  // 读取一次角度传感器的值
  float angle = angle_sensor.modbus_getAngle(0x00); // ESP32下运行一次大概2077us
  Serial.println("a:" + Sring(angle, 4));
  delay(100);
}