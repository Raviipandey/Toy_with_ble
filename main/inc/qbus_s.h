#ifndef __QBUS_S_H
#define __QBUS_S_H

#include <stdint.h>
#include "crc16.h"

#define QB_REG_READ_CMD        0x01
#define QB_REG_WRITE_CMD       0x02

#define QB_FRAME_FUNC_INDEX         0
#define QB_FRAME_ADDR_HB_INDEX      1
#define QB_FRAME_ADDR_LB_INDEX      2
#define QB_FRAME_REG_LEN_HB_INDEX   3
#define QB_FRAME_REG_LEN_LB_INDEX   4
#define QB_FRAME_REG_DATA_INDEX     5

#define QB_FRAME_MIN_LEN            7

// set the frame completion check timeout
#define QB_FRAME_TIMEOUT_MS         3

typedef enum
{
    QBS_INIT,
    QBS_IDLE,
    QBS_FRAME_RCV,
    QBS_SEND_FRAME,
    QBS_STOP
}qBSlaveState_et;

#define QB_ERR_CODE                 0x80
typedef enum
{
    QB_REG_SUCCESS,
    QB_REG_ERR_FUNC_CODE,
    QB_REG_ERR_ADDR
}qBRegRetCode_et;

typedef qBRegRetCode_et (*RegCbFnPtr_t)(uint16_t address, uint16_t len, uint8_t* regData);
typedef void (*TransmitCharFnPtr_t)(uint8_t txData);
typedef void (*EnableTxRxFnPtr_t)(uint8_t txState,uint8_t rxState);

typedef enum
{
    QB_DISABLE,
    QB_ENABLE
}qBEnable_et;

typedef struct qBSlave
{
    qBSlaveState_et qBState;
    uint32_t timeOutStartTick;
    uint8_t *frameBuffer;
    uint16_t framBuffLen;
    uint16_t recvByteCount;
    uint16_t sentByteCount;
    uint16_t totalByteToSend;
    TransmitCharFnPtr_t TransmitChar;
    EnableTxRxFnPtr_t EnableTxRx;
    RegCbFnPtr_t ReadRegCb;
    RegCbFnPtr_t WriteRegCb;
}qBSlave_st;

void QBSlaveInit(qBSlave_st *q, uint8_t *frameBuff,uint16_t buffLen,TransmitCharFnPtr_t transmitCharFnPtr,
                    EnableTxRxFnPtr_t EnableTxRx,RegCbFnPtr_t ReadRegCb,RegCbFnPtr_t WriteRegCb);
void QBSlavePoll(qBSlave_st *q);
void QBSlaveStop(qBSlave_st *q);
void QBSlavePortByteReceived(qBSlave_st *q,uint8_t byte);
void QBSlavePortTxEmptyIsr(qBSlave_st *q);

void QBSlaveStart(qBSlave_st *q);
void QBSlaveStop(qBSlave_st *q);

void writeRfidToFolder();


extern char tagUid_qbus[20];



#endif
