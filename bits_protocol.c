#include "bits_protocol.h"
#include <string.h>
#include "millis.h"

#include <stdlib.h>

static inline void BITS_Protocol_PushPayload(BITS_Protocol_drv_t *prtcl, uint8_t *frame, uint16_t len) {
    for(uint8_t indexPl = 0; indexPl < BITS_PROTOCOL_MAX_PAYLOADS; indexPl++) {
        if(prtcl->txPayloads[indexPl].inUsed == false) {
            if(len >= BITS_PROTOCOL_BUFFER_SIZE) return;
            prtcl->txPayloads[indexPl].inUsed = true;
            memcpy(prtcl->txPayloads[indexPl].frame, frame, len);
            prtcl->txPayloads[indexPl].length = len;
            prtcl->txPayloads[indexPl].retryCount = 0;
            prtcl->txPayloads[indexPl].isWaitACK = false;
            return;
        }
    }
}

static inline void BITS_Protocol_RemovePayload(BITS_Protocol_drv_t *prtcl, uint8_t indexPayload) {
    prtcl->txPayloads[indexPayload].inUsed = false;
}

/**
 * @brief ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~> Frame
 */
static uint8_t BITS_Protocol_Frame_CheckSumCal(uint8_t *frame, uint16_t dataLen) {
    uint8_t ret = 0;
    frame += 2;
    dataLen += (BITS_PROTOCOL_ADDR_SIZE * 2) + 2 + 1 + 2;
    for(uint16_t indexByte = 0; indexByte < dataLen; indexByte++) {
        ret ^= frame[0];
        ret = (ret << 4) | (ret & 0xF0);
        frame++;
    }
    return ret;
}
/**
 * @brief Hàm tạo frame dữ liệu gửi
 * 
 * @param pBuf Con trỏ đến Buffer lưu frame sau khởi tạo, kích thước tối thiểu [17 + length] Byte
 * @param pData Con trỏ đến dữ liệu
 * @param length Độ dài dữ liệu
 * @param frameID payload ID
 */
static uint8_t BITS_Protocol_GenFrame(uint8_t *pBuf,
                                        BITS_Protocol_Type_t frameType,
                                        uint16_t frameID,
                                        uint8_t *srcAddr, uint8_t *destAddr,
                                        uint8_t *pData, uint8_t length) {
    uint8_t frameLength = 0;
    /* Byte Preamble */
    pBuf[frameLength] = BITS_PROTOCOL_FRAME_PREAMBLE;
    frameLength++;
    /* Byte Start */
    pBuf[frameLength] = BITS_PROTOCOL_FRAME_START;
    frameLength++;
    /* Địa chỉ nhận */
    for(uint8_t indexByte = 0; indexByte < BITS_PROTOCOL_ADDR_SIZE; indexByte++) {
        pBuf[frameLength] = destAddr[indexByte];
        frameLength++;
    }
    /* Địa chỉ gửi */
    for(uint8_t indexByte = 0; indexByte < BITS_PROTOCOL_ADDR_SIZE; indexByte++) {
        pBuf[frameLength] = srcAddr[indexByte];
        frameLength++;
    }
    /* Byte Payload ID L */
    pBuf[frameLength] = frameID & 0xFF;
    frameLength++;
    /* Byte Payload ID H */
    pBuf[frameLength] = frameID >> 8;
    frameLength++;
    /* Byte Frame type */
    pBuf[frameLength] = frameType;
    frameLength++;
    /* Data length L */
    pBuf[frameLength] = length & 0xFF;
    frameLength++;
    /* Data length H */
    pBuf[frameLength] = length >> 8;
    frameLength++;
    /* Data */
    for(uint8_t indexByte = 0; indexByte < length; indexByte++) {
        pBuf[frameLength] = pData[indexByte];
        frameLength++;
    }
    /* Checksum */
    pBuf[frameLength++] = BITS_Protocol_Frame_CheckSumCal(pBuf, length);
    return frameLength;
}

/**
 * @brief Hàm gửi dữ liệu theo protocol
 * 
 * @param prtcl 
 * @param pData Con trỏ dữ liệu cần gửi
 * @param length Độ dài dữ liệu cần gửi
 * @return true 
 * @return false 
 */
void BITS_Protocol_SendData(BITS_Protocol_drv_t *prtcl, uint8_t *destAddr, uint8_t *pData, uint8_t length, BITS_Protocol_Type_t type) {
    uint8_t frameLen = BITS_Protocol_GenFrame(prtcl->tempBuf.tx, type, prtcl->frameID, prtcl->DEVaddr, destAddr, pData, length);
    prtcl->frameID++;
    BITS_Protocol_PushPayload(prtcl, prtcl->tempBuf.tx, frameLen);
}

void BITS_Protocol_SendACK(BITS_Protocol_drv_t *prtcl, uint8_t *destMac, uint16_t frameID) {
    uint8_t frameLen = BITS_Protocol_GenFrame(prtcl->tempBuf.tx, BITS_Protocol_Type_ACK, frameID, prtcl->DEVaddr, destMac, 0, 0);
    prtcl->SendData(prtcl->userData, prtcl->tempBuf.tx, frameLen);
}

void BITS_Protocol_RemoveWaitFrameWidthID(BITS_Protocol_drv_t *prtcl, uint16_t ID) {
    BITS_Protocol_Frame_t *frame;
    for(uint8_t indexPl = 0; indexPl < BITS_PROTOCOL_BUFFER_SIZE; indexPl++) {
        frame = ((BITS_Protocol_Frame_t *)(prtcl->txPayloads[indexPl].frame));
        switch(frame->type) {
            case BITS_Protocol_Type_DATA_ACK : case BITS_Protocol_Type_DATA_CHUNK_ACK : {
                if(frame->id == ID) {
                    BITS_Protocol_RemovePayload(prtcl, indexPl);
                }
                break;
            }
            default: {
            }
        }
    }
}

/**
 * @brief Hàm xử lý dữ liệu nhận được
 * 
 * @param prtcl 
 * @param pData Con trỏ dữ liệu nhận được
 * @param dataRecv Byte nhận
 */
void BITS_Protocol_Parser(BITS_Protocol_drv_t *prtcl, uint8_t *dataRecv, uint16_t dataLen) {
    BITS_Protocol_Frame_t *frame = (BITS_Protocol_Frame_t *)prtcl->tempBuf.rx;
    while(dataLen--) {
        if(prtcl->frameState.indexByte >= BITS_PROTOCOL_BUFFER_SIZE) {
            prtcl->frameState.indexByte = 0;
        }
        prtcl->tempBuf.rx[prtcl->frameState.indexByte] = dataRecv[0];
        if(prtcl->frameState.indexByte == 0) {
            if (prtcl->tempBuf.rx[0] != BITS_PROTOCOL_FRAME_PREAMBLE) {
                prtcl->frameState.indexByte = 0;
                prtcl->frameState.lastTick = millis();
            }
            else {
                prtcl->frameState.indexByte++;
            }
        }
        else if(prtcl->frameState.indexByte == 1) {
            if(prtcl->tempBuf.rx[1] != BITS_PROTOCOL_FRAME_START) {
                prtcl->frameState.indexByte = 0;
            }
            else {
                prtcl->frameState.indexByte++;
            }
        }
        else if(prtcl->frameState.indexByte == ((2 + (BITS_PROTOCOL_ADDR_SIZE * 2) + 2 + 1 + 2) + frame->len)) { // Checksum
            if (frame->type != BITS_Protocol_Type_ACK) {
                if (dataRecv[0] == BITS_Protocol_Frame_CheckSumCal(prtcl->tempBuf.rx, frame->len)) {
                    if (memcmp(frame->dest, prtcl->DEVaddr, BITS_PROTOCOL_ADDR_SIZE) == 0) {
                        if (frame->type == BITS_Protocol_Type_DATA_ACK) {
                            BITS_Protocol_SendACK(prtcl, frame->src, frame->id);
                        }
                        if (prtcl->DataRecvCallBack) {
                            prtcl->DataRecvCallBack(prtcl->userData,
                                                        frame->src,
                                                        frame->data,
                                                        frame->len);
                        }
                    }
                }
            }
            else {
                BITS_Protocol_RemoveWaitFrameWidthID(prtcl, frame->id);
            }
            prtcl->frameState.indexByte = 0;
        }
        else {
            prtcl->frameState.indexByte++;
        }
        dataRecv++;
    }
}

/**
 * @brief Hàm thực thi protocol
 * Xử lý các frame chờ gửi
 * @param prtcl 
 */
void BITS_Protocol_Exe(BITS_Protocol_drv_t *prtcl) {
    for(uint8_t indexPl = 0; indexPl < BITS_PROTOCOL_MAX_PAYLOADS; indexPl++) {
        if(prtcl->txPayloads[indexPl].inUsed == true) { // Dữ liệu chờ gửi
            if(prtcl->txPayloads[indexPl].isWaitACK == true) { // Dữ liệu chờ ACK
                if(prtcl->txPayloads[indexPl].retryCount < prtcl->maxRetryNum) {
                    if((millis() - prtcl->txPayloads[indexPl].lastTick) > prtcl->txPayloads[indexPl].randTime + BITS_PROTOCOL_OFFSET_SEND_TIMEOUT) { // Đã hết thời gian chờ ACK
                        prtcl->SendData(prtcl->userData, prtcl->txPayloads[indexPl].frame, prtcl->txPayloads[indexPl].length);
                        prtcl->txPayloads[indexPl].retryCount++;
                        prtcl->txPayloads[indexPl].randTime = prtcl->txPayloads[indexPl].lastTick % 100;
                    }
                }
                else { // Xóa payload nếu hết số lần retry
                    BITS_Protocol_RemovePayload(prtcl, indexPl);
                }
            }
            else {
                prtcl->SendData(prtcl->userData, prtcl->txPayloads[indexPl].frame, prtcl->txPayloads[indexPl].length);
                if(((BITS_Protocol_Frame_t *)prtcl->txPayloads[indexPl].frame)->type == BITS_Protocol_Type_DATA_NOACK) {
                    BITS_Protocol_RemovePayload(prtcl, indexPl);
                }
                else {
                    prtcl->txPayloads[indexPl].lastTick = millis();
                    prtcl->txPayloads[indexPl].randTime = prtcl->txPayloads[indexPl].lastTick % 100;
                    prtcl->txPayloads[indexPl].retryCount = 0;
                    prtcl->txPayloads[indexPl].isWaitACK = true;
                }
            }
        }
    }
    if((millis() - prtcl->frameState.lastTick) >= prtcl->ignoreFrameInterval) {
        prtcl->frameState.indexByte = 0;
    }
}
