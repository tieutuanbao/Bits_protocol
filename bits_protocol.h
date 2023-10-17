#ifndef __BITS_PROTOCOL_H__
#define __BITS_PROTOCOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define BITS_PROTOCOL_BUFFER_SIZE           20
#define BITS_PROTOCOL_BUFFER_LENGTH         10

#define BITS_PROTOCOL_FRAME_PREAMBLE        0xAA
#define BITS_PROTOCOL_FRAME_START           0x7E

#define BITS_PROTOCOL_OFFSET_SEND_TIMEOUT   5

#define BITS_PROTOCOL_ADDR_SIZE             1
#define BITS_PROTOCOL_BROADCAST_ADDR        (uint8_t *)"\x00\x00\x00\x00\x00\x00\x00\x00"

#define BITS_PROTOCOL_MAX_PAYLOADS          20


typedef enum {
    BITS_Protocol_Type_ACK = 0,
    BITS_Protocol_Type_DATA_ACK,
    BITS_Protocol_Type_DATA_NOACK,
    BITS_Protocol_Type_DATA_CHUNK_ACK,
    BITS_Protocol_Type_DATA_CHUNK_NOACK
} BITS_Protocol_Type_t;

/**
 * Frame truyền dữ liệu:
 * | PREAMBLE |  START  |  DEST ADDR  |  SRC ADDR  | FRAME_ID | FRAME_TYPE |  LEN  |  DATA[n]  | CHECKSUM |
 *   1 Byte      1 Byte    n Byte        n Byte      2 Byte      1 Byte      2 Byte    1 Byte     1 Byte
 */
#pragma pack(1)
typedef struct {
    uint8_t preamble;
    uint8_t start;
    uint8_t dest[BITS_PROTOCOL_ADDR_SIZE];
    uint8_t src[BITS_PROTOCOL_ADDR_SIZE];
    union {
        struct {
            uint8_t idL;
            uint8_t idH;
        };
        uint16_t id;
    };
    BITS_Protocol_Type_t type : 8;
    union {
        struct {
            uint8_t lenH;
            uint8_t lenL;
        };
        uint16_t len;
    };
    uint8_t data[];
} BITS_Protocol_Frame_t;
#pragma pack()

typedef struct {
    bool inUsed;
    uint8_t frame[BITS_PROTOCOL_BUFFER_SIZE];
    uint8_t length;
    uint8_t retryCount;
    uint32_t lastTick;
    uint32_t randTime;
    bool isWaitACK;
} BITS_Protocol_Payload_t;


typedef void (*BITS_Protocol_SendData_t)(void *userData, uint8_t *pData, uint16_t length);
typedef void (*BITS_Protocol_DataRecvCallback_t)(void *userData, uint8_t *srcMac, uint8_t *pData, uint16_t length);

typedef struct {
    uint8_t DEVaddr[BITS_PROTOCOL_ADDR_SIZE];
    uint8_t maxRetryNum;
    uint8_t ignoreFrameInterval;
    BITS_Protocol_SendData_t SendData;
    BITS_Protocol_DataRecvCallback_t DataRecvCallBack;

    struct {
        uint8_t tx[BITS_PROTOCOL_BUFFER_SIZE];
        uint8_t rx[BITS_PROTOCOL_BUFFER_SIZE];
    } tempBuf;

    BITS_Protocol_Payload_t txPayloads[BITS_PROTOCOL_MAX_PAYLOADS];

    struct {
        uint16_t indexByte;
        volatile uint32_t lastTick;        
    } frameState;

    uint16_t frameID;
    void *userData;
} BITS_Protocol_drv_t;

void BITS_Protocol_Parser(BITS_Protocol_drv_t *prtcl, uint8_t *dataRecv, uint16_t dataLen);
void BITS_Protocol_Exe(BITS_Protocol_drv_t *prtcl);
void BITS_Protocol_SendData(BITS_Protocol_drv_t *prtcl, uint8_t *destAddr, uint8_t *pData, uint8_t length, BITS_Protocol_Type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
