#ifndef __IOT_QBUS_H
#define __IOT_QBUS_H

#include "main.h"


//read registers
#define         READ_DATA_REG_START                                 0x1000
#define         READ_DATA_NUM_REGS                                  11              //1 reserved

#define         WRITE_TOY_NAME_REG_START                            0x2000
#define         WRITE_TOY_NAME_NUM_REGS                             60

#define         WRITE_VOLUME_CONTROL_REG_START                      0x3000
#define         WRITE_VOLUME_CONTROL_NUM_REGS_MAX                   1

#define         WRITE_NEXT_PREV_REG_START                           0x5000
#define         WRITE_NEXT_PREV_NUM_REGS_MAX                        1

#define         WRITE_PLAY_PAUSE_REG_START                          0x4000
#define         WRITE_PLAY_PAUSE_NUM_REGS_MAX                       1

#define         WRITE_HALL_SENSOR_REG_START                         0x6000
#define         WRITE_HALL_SENSOR_NUM_REGS_MAX                      1

#define         WRITE_ANSWER_OPTION_REG_START                       0x7000
#define         WRITE_ANSWER_OPTION_NUM_REGS_MAX                    1

#define         WRITE_CHARGING_STATUS_REG_START                     0x8000
#define         WRITE_CHARGING_STATUS_NUM_REGS_MAX                  1

#define         WRITE_BATTERY_PERCENT_REG_START                     0x9000
#define         WRITE_BATTERY_PERCENT_NUM_REGS_MAX                  1

#define         WRITE_TURN_TABLE_POSITION_REG_START                 0xA000
#define         WRITE_TURN_TABLE_POSITION_NUM_REGS_MAX              1

typedef enum
{
	QBUS_DATA_ESP_STATE_CMD_BIT = 0,
	QBUS_DATA_LEFT_EAR_LED_R_VAL_BIT,
	QBUS_DATA_LEFT_EAR_LED_G_VAL_BIT,
	QBUS_DATA_LEFT_EAR_LED_B_VAL_BIT,
	QBUS_DATA_RIGHT_EAR_LED_R_VAL_BIT,
	QBUS_DATA_RIGHT_EAR_LED_G_VAL_BIT,
	QBUS_DATA_RIGHT_EAR_LED_B_VAL_BIT,

	QBUS_DATA_LP_LED_R_VAL_BIT,
	QBUS_DATA_LP_LED_G_VAL_BIT,
	QBUS_DATA_LP_LED_B_VAL_BIT,
	QBUS_DATA_ESP_POWER_OFF_BIT,
	QBUS_DATA_LAMP_DURATION_BIT
	
}qbus_data_buff_bits_et;


void QbusInit(void);
void QbusTask(void *params);
void QbusHandler(void);
void UpdateDataBuffer(uint8_t *dataPacket);
qBRegRetCode_et QbusRegWriteCb(uint16_t address, uint16_t len, uint8_t* regData);
qBRegRetCode_et QbusRegReadCb(uint16_t address, uint16_t len, uint8_t* regData);


#endif