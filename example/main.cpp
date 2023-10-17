#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <Arduino.h>

#include "bits_protocol.h"

uint8_t dataRx[256];
BITS_Protocol_drv_t protocol;

/**
 * @brief Hàm gửi dữ liệu thô qua các chuẩn giao tiếp: UART, SPI, I2C, ...
 * 
 * @param userData Dữ liệu người dùng, đã được truyền vào con trỏ userData của protocol
 * @param pData Dữ liệu cần gửi
 * @param length Độ dài dữ liệu
 */
static void Protocol_SendRawData(void *userData, uint8_t *pData, uint16_t length) {
    Serial.write(pData, length);
}

/**
 * @brief Hàm nhận dữ liệu
 * 
 * @param userData Dữ liệu người dùng, đã được truyền vào con trỏ userData của protocol
 * @param MACAddr Địa chỉ MAC của thiết bị Gửi
 * @param dataRecv Dữ liệu nhận được
 * @param dataLen Độ dài dữ liệu
 */
static void Protocol_DataReceived(void *userData, uint8_t *MACAddr, uint8_t *dataRecv, uint16_t dataLen) {
    /* Xử lý dữ liệu nhận được ở đây */
}

void setup() {
    /* ~~~~~~~~~~~~~~~~~~~ UART Init ~~~~~~~~~~~~~~~~~~~ */
    Serial.begin();
    /* ~~~~~~~~~~~~~~~~~~~ Protocol ~~~~~~~~~~~~~~~~~~~ */
    protocol.SendData = Protocol_SendRawData;
    protocol.DataRecvCallBack = Protocol_DataReceived;
    protocol.maxRetryNum = 3;
    protocol.ignoreFrameInterval = 5;

    uint8_t txBuf[] = "hello";
    BITS_Protocol_SendData(&protocol, BITS_PROTOCOL_BROADCAST_ADDR, txBuf, strlen(txBuf), BITS_Protocol_Type_DATA_ACK);
    BITS_Protocol_SendData(&protocol, BITS_PROTOCOL_BROADCAST_ADDR, txBuf, strlen(txBuf), BITS_Protocol_Type_DATA_NOACK);
}

void loop() {
    uint32_t dataLen = Serial.available();
    if(dataLen > 0) {
        if(dataLen > sizeof(dataRx)) dataLen = sizeof(dataRx);
        Serial.readBytes(dataRx, dataLen);
        BITS_Protocol_Parser(&protocol, (uint8_t *), dataLen);
        UartRx_Read.cmplt = false;
    }
    BITS_Protocol_Exe(&protocol);

}