#include "iot_qbus.h"
#include "esp_log.h"
#include "main.h"
#include "system.h"
#include "player.h"

#define FREERTOS_TASK_DELAY 200 / portTICK_PERIOD_MS

extern System *sys;
extern LED *led;
extern Player *player;
extern uint8_t tagPresentFlag_qbus;

qBSlave_st qbusSlave;

uint8_t qbusBuff[100];
uint8_t qbusDataBuff[20];
uint8_t qbusToyBuff[20];

void QbusTask(void *params)
{

  ESP_LOGW("Task", "[QBus Task Added]");
  QbusInit();
  while (1)
  {

    QbusHandler();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void QbusInit(void)
{
  IotQbusUartPortInit();
  QBSlaveInit(&qbusSlave, qbusBuff, sizeof(qbusBuff), IotQbusSendByte, IotQbusInterruptEnable, QbusRegReadCb, QbusRegWriteCb);
  QBSlaveStart(&qbusSlave);
  // ESP_LOGI("QbusInit", "Qbus initialized successfully");
}

void QbusHandler(void)
{
  QBSlavePoll(&qbusSlave);
}

qBRegRetCode_et QbusRegWriteCb(uint16_t address, uint16_t len, uint8_t *regData)
{
  /*if((address >= WRITE_TOY_NAME_REG_START) && ((address + len) <= (WRITE_TOY_NAME_REG_START + WRITE_TOY_NAME_NUM_REGS)))
  {
    if(regData[0] == 0) {
      sys->resetToy();
    }
    sys->setToy((char*)regData);
    return QB_REG_SUCCESS;
  }
  else */
  // ESP_LOGI("QbusRegWriteCb", "Write callback triggered for address: %d", address);
  if ((address >= WRITE_VOLUME_CONTROL_REG_START) && ((address + len) <= (WRITE_VOLUME_CONTROL_REG_START + WRITE_VOLUME_CONTROL_NUM_REGS_MAX)))
  {
    if (regData[0] == 2)
    {

      player->decVolume();
      // sys->setPowerStatus(true);
      // ESP_LOGW("TAG", "Power %d", sys->getPowerStatus());
    }
    else if (regData[0] == 1)
    {
      player->incVolume();
      // sys->setPowerStatus(false);
      // ESP_LOGW("TAG", "Power %d", sys->getPowerStatus());
    }
    return QB_REG_SUCCESS;
  }
  else if ((address >= WRITE_PLAY_PAUSE_REG_START) && ((address + len) <= (WRITE_PLAY_PAUSE_REG_START + WRITE_PLAY_PAUSE_NUM_REGS_MAX)))
  {
    if (tagPresentFlag_qbus)
    {
      player->playPauseToggle();
    }
    return QB_REG_SUCCESS;
  }
    else if ((address >= WRITE_NEXT_PREV_REG_START) && ((address + len) <= (WRITE_NEXT_PREV_REG_START + WRITE_NEXT_PREV_NUM_REGS_MAX)))
    {
      if (regData[0] == 2)
      {
        player->previous();
        ESP_LOGW("button", "prev");
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      else if (regData[0] == 1)
      {
        player->next();
        ESP_LOGW("button", "next");
        vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      return QB_REG_SUCCESS;
    }
    else if ((address >= WRITE_ANSWER_OPTION_REG_START) && ((address + len) <= (WRITE_ANSWER_OPTION_REG_START + WRITE_ANSWER_OPTION_NUM_REGS_MAX)))
    {
      if (regData[0] == 1)
      {
        sys->setLeftEarButton(true);
        player->skipQuestion();
        ESP_LOGW("button", "leftEar");
      }
      else if (regData[0] == 2)
      {
        sys->setRightEarButton(true);
        player->skipQuestion();
        ESP_LOGW("button", "rightEar");
      }

      return QB_REG_SUCCESS;
    }
    else if ((address >= WRITE_CHARGING_STATUS_REG_START) && ((address + len) <= (WRITE_CHARGING_STATUS_REG_START + WRITE_CHARGING_STATUS_NUM_REGS_MAX)))
    {
      if (!regData[0])
      {
        sys->setChargingStatus(false);
        ESP_LOGW("battery", "Not charging");
      }
      else
      {
        sys->setChargingStatus(true);
        ESP_LOGW("battery", "Charging");
      }
      return QB_REG_SUCCESS;
    }
    else if ((address >= WRITE_BATTERY_PERCENT_REG_START) && ((address + len) <= (WRITE_BATTERY_PERCENT_REG_START + WRITE_BATTERY_PERCENT_NUM_REGS_MAX)))
    {
      sys->setBatteryLevel(regData[0]);
      return QB_REG_SUCCESS;
    }
    else if ((address >= WRITE_TURN_TABLE_POSITION_REG_START) && ((address + len) <= (WRITE_TURN_TABLE_POSITION_REG_START + WRITE_TURN_TABLE_POSITION_NUM_REGS_MAX)))
    {
      sys->setTurnTable((turn_table_direction_et)regData[0]);

      return QB_REG_SUCCESS;
    }
    else
    {
      return QB_REG_ERR_ADDR;
    }
  }

  qBRegRetCode_et QbusRegReadCb(uint16_t address, uint16_t len, uint8_t * regData)
  {
    // ESP_LOGI("QbusRegReadCb", "Read callback triggered for address: %d", address);
    uint16_t index = 0;
    if ((address >= READ_DATA_REG_START) && ((address + len) <= (READ_DATA_REG_START + READ_DATA_NUM_REGS)))
    {
      index = (address - READ_DATA_REG_START);

      // Create a buffer to hold the hexadecimal string
      char hexString[3 * len + 1]; // Each byte takes 2 hex digits + 1 space, and 1 for null terminator
      char *hexPtr = hexString;

      for (int i = 0; i < len; i++)
      {
        hexPtr += sprintf(hexPtr, "%02X ", qbusDataBuff[index + i]);
      }

      // ESP_LOGI("QbusSend", "Sending data: %s", hexString);

      UpdateDataBuffer(qbusDataBuff);

      while (len > 0)
      {
        *regData++ = qbusDataBuff[index];
        index++;
        len--;
      }
      return QB_REG_SUCCESS;
    }
    else
    {
      ESP_LOGW("log", "qbus read cb err");
      return QB_REG_ERR_ADDR;
    }
  }

  int testi = 0, testb = 0;
  void UpdateDataBuffer(uint8_t * dataPacket)
  {
    /*if (testi >= 10)
    {
      testi=0;
      if (testb >= 10)
      {
        testb=0;

      }
      else
      {
        testb++;
      }
    }
    else
    {
      testi++;
    }
    */

    // led->setLandingPadColor(0, 0, 255); // Blue

    // led->setPadState(ESP_DOWNLOADING_AUDIO);

    dataPacket[QBUS_DATA_ESP_STATE_CMD_BIT] = led->getPadState();
    dataPacket[QBUS_DATA_LEFT_EAR_LED_R_VAL_BIT] = led->getLeftEarColor(0);
    dataPacket[QBUS_DATA_LEFT_EAR_LED_G_VAL_BIT] = led->getLeftEarColor(1);
    dataPacket[QBUS_DATA_LEFT_EAR_LED_B_VAL_BIT] = led->getLeftEarColor(2);
    dataPacket[QBUS_DATA_RIGHT_EAR_LED_R_VAL_BIT] = led->getRightEarColor(0);
    dataPacket[QBUS_DATA_RIGHT_EAR_LED_G_VAL_BIT] = led->getRightEarColor(1);
    dataPacket[QBUS_DATA_RIGHT_EAR_LED_B_VAL_BIT] = led->getRightEarColor(2);
    dataPacket[QBUS_DATA_LP_LED_R_VAL_BIT] = led->getLandingPadColor(0);
    dataPacket[QBUS_DATA_LP_LED_G_VAL_BIT] = led->getLandingPadColor(1);
    dataPacket[QBUS_DATA_LP_LED_B_VAL_BIT] = led->getLandingPadColor(2);
    dataPacket[QBUS_DATA_ESP_POWER_OFF_BIT] = sys->getPowerStatus();
    // dataPacket[QBUS_DATA_ESP_POWER_OFF_BIT]          = 255;
    // dataPacket[QBUS_DATA_ESP_POWER_OFF_BIT]          = sys->getPowerStatus();
  }