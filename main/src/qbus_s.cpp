#include "qbus_s.h"
#include "time_spent.h"
#include "esp_log.h"
#include "cstring"
#include "main.h"
#include "system.h"
#include "player.h"

extern System *sys;
extern Player *player;

// before calling the function the serial uart port  should be initialised
void QBSlaveInit(qBSlave_st *q, uint8_t *frameBuff, uint16_t buffLen, TransmitCharFnPtr_t transmitCharFnPtr,
                 EnableTxRxFnPtr_t EnableTxRx, RegCbFnPtr_t ReadRegCb, RegCbFnPtr_t WriteRegCb)
{
    // intialiase the structure with the value sent
    q->qBState = QBS_INIT;
    q->timeOutStartTick = GetStartTime();
    q->frameBuffer = frameBuff;
    q->framBuffLen = buffLen;
    q->TransmitChar = transmitCharFnPtr;
    q->EnableTxRx = EnableTxRx;
    q->ReadRegCb = ReadRegCb;
    q->WriteRegCb = WriteRegCb;
    q->recvByteCount = 0;
    q->sentByteCount = 0;
    q->totalByteToSend = 0;

    // enable the receive interrupt and disable the tx interrupt
    q->EnableTxRx(QB_DISABLE, QB_DISABLE);
}

uint8_t debugCount = 0;

void QBSlaveSetState(qBSlave_st *q, qBSlaveState_et state)
{

    q->qBState = state;

    switch (q->qBState)
    {
        // enable the receive interrupt
    case QBS_IDLE:
        q->recvByteCount = 0;
        q->EnableTxRx(QB_DISABLE, QB_ENABLE);

        // added for debug
        debugCount++;
        if (debugCount == 10)
        {
            debugCount = 0;
            // LOG_DBGS(CH_DISP,"Reply");
        }
        break;

    case QBS_FRAME_RCV:

        break;

    case QBS_SEND_FRAME:
        q->sentByteCount = 0;
        q->EnableTxRx(QB_ENABLE, QB_DISABLE);
        break;

    case QBS_STOP:
        q->EnableTxRx(QB_DISABLE, QB_DISABLE);
        q->sentByteCount = 0;
        q->recvByteCount = 0;
        break;

    default:
        break;
    }
}

uint8_t QBSlaveValidateFrame(qBSlave_st *q)
{
    // check the length of the frame
    if (q->recvByteCount < QB_FRAME_MIN_LEN)
    {
        return 0;
    }

    // Print the entire received data before the CRC check
    // printf("Received Data: ");
    // for (uint16_t i = 0; i < q->recvByteCount; i++)
    // {
    //     printf("%02X ", q->frameBuffer[i]);
    // }
    // printf("\n");

    // TODO: check CRC of the received frame
    if (Crc16Get(q->frameBuffer, q->recvByteCount) == 0)
    {
        return 1;
    }
    else
    {
        ESP_LOGW("log", "qbus crc err");
        return 0;
    }
}

char tagUid_qbus[20];
char previousTagUid[20] = {0};

uint8_t tagPresentFlag_qbus = 0;
uint32_t tagCount = 0;
void QBSlaveProcessFrame(qBSlave_st *q)
{
    uint8_t functionCode;
    uint16_t address;
    uint16_t regLen;

    qBRegRetCode_et retCode;

    // get the function code
    functionCode = q->frameBuffer[QB_FRAME_FUNC_INDEX];

    // get the address
    address = q->frameBuffer[QB_FRAME_ADDR_HB_INDEX] << 8;
    address |= q->frameBuffer[QB_FRAME_ADDR_LB_INDEX];

    // get the register len to read or write
    regLen = q->frameBuffer[QB_FRAME_REG_LEN_HB_INDEX] << 8;
    regLen |= q->frameBuffer[QB_FRAME_REG_LEN_LB_INDEX];

    // check the first byte
    if (functionCode == QB_REG_READ_CMD)
    {
        // uint8_t uid[8];

        char *uidPtr = tagUid_qbus;
        uint8_t i;
        for (i = 0; i < 8; i++)
        {
            // uid[i] = (char)q->frameBuffer[QB_FRAME_REG_DATA_INDEX+i];
            // uidPtr += sprintf(uidPtr, "%02", uid[i]);

            uidPtr += sprintf(uidPtr, "%02X", (char)q->frameBuffer[QB_FRAME_REG_DATA_INDEX + i]);
        }

        uint8_t batteryVolts = q->frameBuffer[QB_FRAME_REG_DATA_INDEX + i];
        // ESP_LOGW("TAG", "Battery remaining: %d", batteryVolts); // Uncomment this to see in logs how much battery is remaining
        sys->setBatteryLevel(batteryVolts);
        i++;
        uint8_t batterchargingState = q->frameBuffer[QB_FRAME_REG_DATA_INDEX + i];
        sys->setChargingStatus(batterchargingState);
        // ESP_LOGW("TAG", "Battery charging state: %d", batterchargingState); // Uncomment this to see in logs if battery is charging or not
        // sys->setChargingStatus(batterchargingState);

        if (strcmp(tagUid_qbus, "0000000000000000") == 0) // returns 0 if strings are equal
        {
            // ESP_LOGW("log", "no tag found");
            if (tagPresentFlag_qbus == 1) // if toy was removed just now
            {
                sys->resetToy();
                ESP_LOGW("qbus", "tag removed");
            }
            tagPresentFlag_qbus = 0;
        }
        else
        {

            if ((tagPresentFlag_qbus == 0) || ((strncmp(previousTagUid, tagUid_qbus, 16) != 0))) // if toy was freshly placed or toyChanged
            {

                ESP_LOGW("qbus", "NEW TAG : %s", tagUid_qbus);
                strncpy(previousTagUid, tagUid_qbus, 16);

                // writeRfidToFolder();
                // tagCount++;
                // ESP_LOGW("tagCount", "%d", tagCount);

                sys->setToy(tagUid_qbus);
                // ESP_LOGW("qbus", "tag");
            }
            tagPresentFlag_qbus = 1;
        }
        // register read function
        retCode = q->ReadRegCb(address, regLen, &q->frameBuffer[QB_FRAME_REG_DATA_INDEX]);
        if (retCode == QB_REG_SUCCESS)
        {
            q->totalByteToSend = (QB_FRAME_REG_LEN_LB_INDEX + 1) + regLen;
        }
        else
        {
            q->frameBuffer[QB_FRAME_FUNC_INDEX] = QB_ERR_CODE | functionCode;
            q->totalByteToSend = (QB_FRAME_REG_LEN_LB_INDEX + 1);
            q->frameBuffer[q->totalByteToSend] = retCode;
            q->totalByteToSend++;
        }
    }
    else if (functionCode == QB_REG_WRITE_CMD)
    {
        // register write function
        retCode = q->WriteRegCb(address, regLen, &q->frameBuffer[QB_FRAME_REG_DATA_INDEX]);
        if (retCode != QB_REG_SUCCESS)
        {
            q->frameBuffer[QB_FRAME_FUNC_INDEX] = QB_ERR_CODE | functionCode;
        }
        q->totalByteToSend = (QB_FRAME_REG_LEN_LB_INDEX + 1);
        q->frameBuffer[q->totalByteToSend] = retCode;
        q->totalByteToSend++;
    }
    else
    {
        // invalid function code
        q->frameBuffer[QB_FRAME_FUNC_INDEX] = QB_ERR_CODE;
        q->totalByteToSend = (QB_FRAME_REG_LEN_LB_INDEX + 1);
        q->frameBuffer[q->totalByteToSend] = QB_REG_ERR_FUNC_CODE;
        q->totalByteToSend++;
    }

    // TODO add checksum
    uint16_t checksum = Crc16Get(q->frameBuffer, q->totalByteToSend);

    q->frameBuffer[q->totalByteToSend] = checksum & 0x00ff;
    q->totalByteToSend++;
    q->frameBuffer[q->totalByteToSend] = checksum >> 8;
    q->totalByteToSend++;
}

// FILE* rfidFile = NULL;
// extern SdCard *sdcard;
// void writeRfidToFolder()
// {

//         char folderName[150];
//         sprintf(folderName, "/sdcard/rfidList");
//         mkdir(folderName, 0775);

//     char url[200];
//     sprintf(url, "/sdcard/rfidList/rfidTable.csv");
//     rfidFile = fopen(url, "a");

//     char data[40] = {0};
//     uint8_t data2[40] = {0};
//     sprintf(data, "\n %s,productCode",tagUid_qbus);
//     memcpy(data2, data, 30);
//     sdcard->write(data2, 30, rfidFile);
//     fclose(rfidFile);
// }
void QBSlavePoll(qBSlave_st *q)
{
    switch (q->qBState)
    {
    case QBS_IDLE:
        // check first byte received
        if (q->recvByteCount >= 1)
        {
            if ((q->frameBuffer[0] == 0x01) || (q->frameBuffer[0] == 0x02)) // only proceed if avalid start byte is received
            {
                // set the state to qbs frame rcv
                QBSlaveSetState(q, QBS_FRAME_RCV);
            }
            else
            {
                QBSlaveSetState(q, QBS_IDLE);
            }
        }
        break;

    case QBS_FRAME_RCV:
        // check whether frame timeout complete
        if (TimeSpent(q->timeOutStartTick, QB_FRAME_TIMEOUT_MS))
        {
            // stop the reception interrupt during this time
            q->EnableTxRx(QB_DISABLE, QB_DISABLE);

            // check frame CRC
            if (QBSlaveValidateFrame(q) == 1)
            {
                // process the receive frame
                QBSlaveProcessFrame(q);
                QBSlaveSetState(q, QBS_SEND_FRAME);
            }
            else
            {
                // no reply should be sent as the frame itself is wrong
                QBSlaveSetState(q, QBS_IDLE);
            }
        }
        break;

    case QBS_SEND_FRAME:
        // in the ISR  when the frame is completely sent then the state is changed to IDLE
        QBSlavePortTxEmptyIsr(q);
        break;

    default:
        break;
    }
}

void QBSlavePortByteReceived(qBSlave_st *q, uint8_t byte)
{
    if (q->recvByteCount < q->framBuffLen)
    {
        q->frameBuffer[q->recvByteCount] = byte;
    }
    else
    {
        q->recvByteCount = q->framBuffLen - 1;
        q->frameBuffer[q->recvByteCount] = byte;
    }
    q->recvByteCount++;
    q->timeOutStartTick = GetStartTime();
}

void QBSlavePortTxEmptyIsr(qBSlave_st *q)
{
    if (q->totalByteToSend > 0)
    {
        // newly added while loop
        while (q->totalByteToSend > 0)
        {
            if (q->sentByteCount < q->framBuffLen)
            {
                q->TransmitChar(q->frameBuffer[q->sentByteCount++]);
            }
            q->totalByteToSend--;
        }
    }
    else
    {
        QBSlaveSetState(q, QBS_IDLE);
    }
}

void QBSlaveStart(qBSlave_st *q)
{
    QBSlaveSetState(q, QBS_IDLE);
}

void QBSlaveStop(qBSlave_st *q)
{
    QBSlaveSetState(q, QBS_STOP);
}