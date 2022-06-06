#ifndef ANGLE_SENSOR_H
#define ANGLE_SENSOR_H
#include <Arduino.h>
#include "crc16.hpp"

// 角度传感器模式
enum SENSOR_MODE
{
  ASCII,        // 0：ASCII模式，没测试
  MODBUS,       // 1：modbus模式，就用这个吧
  MODBUS_ACTIVE // 2：modbus主动回传模式，没有做
};

class ANGLE_SENSOR
{
public:
  float angle;                  // 角度[°]
  uint8_t prate, mrate, id = 0; // 回传速度
  SENSOR_MODE sensor_mode;
  ANGLE_SENSOR(Stream &serial, SENSOR_MODE init_mode);
  ~ANGLE_SENSOR();
  Stream *com_port = nullptr; // 串行终端变量（如果提供）
  char eol = '\n';            // 结束符
  bool echo = false;          // 回显最后键入的字符（用于命令行反馈）
  bool init();
  SENSOR_MODE set_mode(SENSOR_MODE sensor_mode); // 先实现最基础的角度读取功能，剩下的有需要再加
  int set_id(int id);                            // 设置传感器ID 待做
  int set_prate(int prate);                      // 设置ASCII模式下回传速度 还没做
  int set_mrate(int mrate);                      // 设置MODBUS模式下回传速度 还没做，传感器就坏了
  void send_string(char *rec_data, const char *str_command);
  bool send_hex(uint8_t *rec_hex, uint8_t *hex_command); // 返回响应数据rec_hex的CRC校验结果
  uint8_t rec_hex_main[20] = {0};
  uint8_t hex_command_crc[8] = {0};                                      // hex命令长8字节，结构：|ID|CMD|RegH|RegL|LenH|LenL|CRCH|CRCL|，不需要额外的换行与回车
  bool RW_reg(uint8_t id, uint8_t cmd, uint16_t reg_addr, uint16_t len); // 读写寄存器,返回响应数据的CRC校验结果
  float modbus_getAngle(uint8_t modbus_id);                              // modbus模式下通过hex命令读取寄存器获得角度值
  float ascii_getAngle(uint8_t modbus_id);                               // ascii模式下通过AT指令获得角度值
};

// 构造函数，实例化一个角度传感器对象
ANGLE_SENSOR::ANGLE_SENSOR(Stream &serial, SENSOR_MODE init_mode)
{
  com_port = &serial;
  this->sensor_mode = init_mode;
}
// 析构函数
ANGLE_SENSOR::~ANGLE_SENSOR() {}

// 初始化函数，初始化前应先打开串口并连接到传感器
bool ANGLE_SENSOR::init()
{
  if (!com_port)
    return false;
  char cmd_res[20] = {0};
  send_string(cmd_res, "AT"); // 发送测试语句，测试连接
  if (cmd_res[0] == 'O' && cmd_res[1] == 'K')
  {
    Serial.println("连接角度传感器成功");
  }
  switch (sensor_mode)
  {
  case ASCII:
    send_string(cmd_res, "AT+MODE=0"); //设置为ASCII模式
    Serial.println("ASCII模式");
    break;
  case MODBUS:
    send_string(cmd_res, "AT+MODE=1"); //设置为MODBUS模式
    Serial.println("MODBUS模式");
    break;
  case MODBUS_ACTIVE: // MODBUS主动回传模式，用不到，先不写
    break;
  default:
    Serial.println("angle sensor init fault");
    return false;
  }
  return true;
}

// 发送字符串命令 str_command,并返回响应数据到rec_data
void ANGLE_SENSOR::send_string(char *rec_data, const char *str_command)
{
  // 首先清空串口
  while (com_port->read() >= 0)
    ;
  // 发送命令
  com_port->write(str_command);
  com_port->write("\r\n");
  // 先清空返回数组rec_data
  memset(rec_data, '\0', 20);
  // 从串口读取返回的数据，读取20个字符
  int i = 0;
  vTaskDelay(1 / portTICK_PERIOD_MS); //等待一下传感器回应
  while (com_port->available())
  {
    if (i >= 20)
    {
      printf("rec_data overflow!\n");
      break;
    }
    int ch = com_port->read();
    rec_data[i] = (char)ch;
    i++;
  }
}

// 发送16进制命令 hex_command,并返回响应数据到rec_hex
bool ANGLE_SENSOR::send_hex(uint8_t *rec_hex, uint8_t *hex_command) // 数组名作为参数时，会变成指针（4字节），无法用sizeof获取数组长度
{
  // 首先清空串口
  while (com_port->read() >= 0)
    ;

  // printf("command:\n");
  // for (int i = 0; i < 8; i++)
  // {
  //   printf("%02X ", hex_command[i]);
  // }
  // printf("\n");
  // 发送命令
  com_port->write(hex_command, 8);
  // 先清空返回数组rec_hex
  memset(rec_hex, '\0', 8);
  // 从串口读取返回的数据，读取20个字符
  int i = 0;
  vTaskDelay(1 / portTICK_PERIOD_MS); //等待一下传感器回应
  while (com_port->available())
  {
    if (i >= 20)
    {
      printf("rec_data overflow!\n");
      break;
    }
    int ch = com_port->read();
    rec_hex[i] = (uint8_t)ch;
    i++;
  }
  // com_port->readBytes(rec_hex, 20); // 不知道为啥，这个函数很耗时 块0.5s了
  // 下面对接收到的数据进行CRC校验
  uint8_t len = rec_hex[2];
  uint8_t crc_tmp[20];
  memset(crc_tmp, '\0', 20);
  for (int i = 0; i < len + 5; i++)
    crc_tmp[i] = rec_hex[i];
  uint16_t crc16 = Crc16(crc_tmp, len + 3);
  if ((HIG_UINT16(crc16) == rec_hex[len + 4]) && (LOW_UINT16(crc16) == rec_hex[len + 3]))
  {
    // Serial.println("CRC校验成功");
    return true;
  }
  else
  {
    Serial.println("CRC校验失败,数据可能存在丢失");
    // printf("received:\n");
    // for (int i = 0; i < sizeof(rec_hex_main); i++)
    // {
    //   printf("%02X ", rec_hex_main[i]);
    // }
    // printf("\n");
    return false;
  }
  // printf("%s\n", rec_hex);
  // printf("received:\n");
  // for (int i = 0; i < sizeof(rec_hex_main); i++)
  // {
  //   printf("%02X ", rec_hex_main[i]);
  // }
  // printf("\n");
}

// 角度传感器modbus模式下寄存器读写函数,返回传感器响应数据rec_hex_main的CRC校验结果
bool ANGLE_SENSOR::RW_reg(uint8_t id, uint8_t cmd, uint16_t reg_addr, uint16_t len)
{
  uint8_t cmd_hex[6];
  uint8_t cmd_hex_full[8];
  cmd_hex[0] = id;
  cmd_hex[1] = cmd;
  cmd_hex[2] = (uint8_t)(HIG_UINT16(reg_addr)); // 取reg_addr高8位
  cmd_hex[3] = (uint8_t)(LOW_UINT16(reg_addr)); // 取reg_addr低8位
  cmd_hex[4] = (uint8_t)(HIG_UINT16(len));      // 取len高8位
  cmd_hex[5] = (uint8_t)(LOW_UINT16(len));      // 取len低8位
  for (int i = 0; i < 6; i++)
    cmd_hex_full[i] = cmd_hex[i];
  uint16_t CRC = Crc16(cmd_hex, 6);            // 最后两字节添加CRC校验
  cmd_hex_full[6] = LOW_UINT16(CRC);           // 取CRC高8位
  cmd_hex_full[7] = HIG_UINT16(CRC);           // 取CRC低8位
  return send_hex(rec_hex_main, cmd_hex_full); // 发送读取寄存器命令，并将返回值存到rec_hex_main中
}

// 封装好的函数，MODBUS模式下调用即可返回角度值
float ANGLE_SENSOR::modbus_getAngle(uint8_t modbus_id)
{
  memset(rec_hex_main, '\0', 20);
  bool flag = RW_reg(modbus_id, 0x03, 0x00D5, 0x0001); // 读取角度值寄存器高8位
  if (!flag)                                           // 如果传感器返回值CRC校验失败，则返回-1
    return -1;
  uint16_t ANGH = (rec_hex_main[3] << 8) | rec_hex_main[4];
  flag = RW_reg(modbus_id, 0x03, 0x00D6, 0x0001); // 读取角度值寄存器低8位
  if (!flag)                                      // 如果传感器返回值CRC校验失败，则返回-1
    return -1;
  uint16_t ANGL = (rec_hex_main[3] << 8) | rec_hex_main[4];
  uint32_t ANGLE = (ANGH << 16) | ANGL; // 32位角度值
  angle = (float)ANGLE / 262144 * 360;
  // printf("a:%.3f\n", angle);
  return angle;
}

// 封装好的函数，ASCII模式下调用即可返回角度值
float ANGLE_SENSOR::ascii_getAngle(uint8_t modbus_id)
{
  char rec_str_main[20];
  memset(rec_str_main, '\0', 20);
  send_string(rec_str_main, "AT+PRATE=0");
  angle = atof(rec_str_main + 10);
  printf("a:%.3f\n", angle);
  return angle;
}

#endif