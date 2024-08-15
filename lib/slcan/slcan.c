/*
 * SLCAN.c
 *
 *  Created on: Aug 11, 2024
 *      Author: Berran
 */
#include "slcan.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "can.h"
#include <libopencm3/stm32/can.h>
#include "led.h"
// #include "usbd_cdc_if.h"

const uint8_t version[6] = "V1013\r";
const uint8_t serial[6] = "NA123\r";

/*  -----------  defines  ------------------------------------------------
 */

#define BCD2CHR(x) ((((x) & 0xF) < 0xA) ? ('0' + ((x) & 0xF)) : ('7' + ((x) & 0xF)))

#ifdef USE_MACRO
#define CHR2BCD(x) ((('0' <= (x)) && ((x) <= '9')) ? ((x) - '0') : ((('A' <= (x)) && ((x) <= 'F')) ? (10 + (x) - 'A') : ((('a' <= (x)) && ((x) <= 'f')) ? (10 + (x) - 'a') : 0xFF)))
#else
uint8_t CHR2BCD(char ch);

uint8_t CHR2BCD(char ch) {
    uint8_t bcd = 0;

    // Handle digits 0-9
    if (ch >= '0' && ch <= '9') {
        bcd = (ch - '0') & 0x0F; // Extract lower 4 bits
    }
    // Handle letters A-F
    else if (ch >= 'A' && ch <= 'F') {
        bcd = (ch - 'A' + 10) & 0x0F; // Extract lower 4 bits
    }
    // Handle letters a-f
    else if (ch >= 'a' && ch <= 'f') {
        bcd = (ch - 'a' + 10) & 0x0F; // Extract lower 4 bits
    }
    // Handle invalid input (return 0 or other error code)
    else {
        // You can choose to return 0 for invalid input or handle it differently
        bcd = 0;
    }

    return bcd;
}
#endif /* USE_MACRO */

#define MAX_DLC(l) (((l) < CAN_LEN_MAX) ? (l) : (CAN_DLC_MAX))

#define BUFFER_SIZE 128U
#define RESPONSE_TIMEOUT 100U

/** @brief  CAN message (SocketCAN compatible)
 */
typedef struct slcan_message_t_
{                              /* SLCAN message: */
    uint32_t can_id;           /**< message identifier */
    uint8_t can_dlc;           /**< data length code (0..8) */
    uint8_t __pad;             /**< (padding) */
    uint8_t __res1;            /**< (resvered for CAN FD) */
    uint8_t __res2;            /**< (resvered for CAN FD) */
    uint8_t data[CAN_LEN_MAX]; /**< payload (max. 8 data bytes) */
} slcan_message_t;

// Function pointer type for command handlers
typedef uint8_t (*CmdHandler)(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);

// Define the struct for the command lookup table
typedef struct
{
    char cmd;
    CmdHandler handler;
} CmdLookupEntry;

static bool encode_message(const slcan_message_t *message, uint8_t *buffer, uint8_t *nbytes);

uint8_t handleSn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handlesxxyy(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleO(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleL(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleC(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handletiiiildd(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleTiiiiiiiildd(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleriiil(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleRiiiiiiiil(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleP(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleA(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleF(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleXn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleWn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleMxxxxxxxx(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handlemxxxxxxxx(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleUn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleV(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleN(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleZn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);
uint8_t handleQn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);

static bool encode_message(const slcan_message_t *message, uint8_t *buffer, uint8_t *nbytes)
{
    uint8_t index = 0;

    // assert(message);
    // assert(buffer);
    // assert(nbytes);

    if (!(message->can_id & CAN_XTD_FRAME))
    {
        if (!(message->can_id & CAN_RTR_FRAME))
            buffer[index++] = (uint8_t)'t';
        else
            buffer[index++] = (uint8_t)'r';
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_STD_MASK) >> 8);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_STD_MASK) >> 4);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_STD_MASK) >> 0);
    }
    else
    {
        if (!(message->can_id & CAN_RTR_FRAME))
            buffer[index++] = (uint8_t)'T';
        else
            buffer[index++] = (uint8_t)'R';
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 28);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 24);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 20);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 16);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 12);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 8);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 4);
        buffer[index++] = (uint8_t)BCD2CHR((message->can_id & CAN_XTD_MASK) >> 0);
    }
    if (!(message->can_id & CAN_RTR_FRAME))
    {
        buffer[index++] = (uint8_t)BCD2CHR(MAX_DLC(message->can_dlc));
        for (uint8_t i = 0; i < (uint8_t)MAX_DLC(message->can_dlc); i++)
        {
            buffer[index++] = (uint8_t)BCD2CHR(message->data[i] >> 4);
            buffer[index++] = (uint8_t)BCD2CHR(message->data[i] >> 0);
        }
    }
    else
    {
        buffer[index++] = (uint8_t)BCD2CHR(MAX_DLC(message->can_dlc));
    }
    buffer[index++] = (uint8_t)'\r';
    *nbytes = index;
    return true;
}

static bool decode_message(slcan_message_t *message, const uint8_t *buffer, uint8_t nbytes)
{
    int i = 0;
    uint8_t index = 0;
    uint8_t offset;
    uint8_t digit;
    uint32_t flags;

    // assert(message);
    // assert(buffer);
    // assert(nbytes);

    //(void)memset(message, 0x00, sizeof(slcan_message_t));

    /* (1) message flags: XTD and RTR */
    switch (buffer[index++])
    {
    case 't':
        flags = CAN_STD_FRAME;
        offset = index + 3;
        break;
    case 'T':
        flags = CAN_XTD_FRAME;
        offset = index + 8;
        break;
    case 'r':
        flags = CAN_RTR_FRAME;
        offset = index + 3;
        break;
    case 'R':
        flags = CAN_RTR_FRAME | CAN_XTD_FRAME;
        offset = index + 8;
        break;
    default:
        return false;
    }
    if (index >= nbytes)
        return false;
    /* (2) CAN identifier: 11-bit or 29-bit */
    while ((index < offset) && (index < nbytes))
    {
        digit = CHR2BCD(buffer[index++]);
        if (digit != 0xFF)
            message->can_id = (message->can_id << 4) | (uint32_t)digit;
        else
            return false;
    }
    if (index >= nbytes)
        return false;
    /* (!) ORing message flags (Linux-CAN compatible) */
    message->can_id |= flags;
    /* (3) Data Length Code: 0..8 */
    digit = CHR2BCD(buffer[index++]);
    if (digit <= CAN_DLC_MAX)
        message->can_dlc = (uint8_t)digit;
    else
        return false;
    if (index >= nbytes)
        return false;
    /* (4) message data: up to 8 bytes */
    if (!(flags & CAN_RTR_FRAME))
        offset = index + (uint8_t)(message->can_dlc * 2);
    else /* note: no data in RTR frames! */
        offset = index;
    while ((index < offset) && (index < nbytes))
    {
        digit = CHR2BCD(buffer[index++]);
        if ((digit != 0xFF) && (index < nbytes))
            message->data[i] = (uint8_t)digit;
        else{
            return false;}
        digit = CHR2BCD(buffer[index++]);
        if (digit != 0xFF)
            message->data[i] = (message->data[i] << 4) | (uint8_t)digit;
        else{
            return false;}
        i++;
    }
    
    if (index >= nbytes)
        return false;
    /* (5) ignore the rest: CR or time-stamp + CR */
    return true;
}

// Command handler function implementations
uint8_t handleSn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    (void)inSize;
    (void)outData;
    (void)outSize;
    // Handle the 'Sn' command (Setup with standard CAN bit-rates where n is 0-8)
    uint8_t digit = CHR2BCD(inData[1]);
    can_setup(digit);
    return true;
}

uint8_t handlesxxyy(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'sxxyy' command
    return true;
}

uint8_t handleO(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'O' command (Open CAN)
    return true;
}

uint8_t handleL(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'L' command (Open CAN in listen-only mode)
    // CAN_Open(CAN_MODE_LOOPBACK);
    return true;
}

uint8_t handleC(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'C' command (Close CAN)
    return true;
}

uint8_t handletiiiildd(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'tiiildd...' command (Transmit standard CAN frame)

    slcan_message_t message = { 0 };
    /* new message received (indication) */
    if (decode_message(&message, inData, *inSize))
    {
        // CAN_SendMessage(message.can_id, message.can_dlc, message.data);
        can_transmit(CAN1, message.can_id, 0/*ext*/, 0/*rtr*/, message.can_dlc, message.data);
    } else {
        //led_toggle(LED_ACT);
    }
    return true;
}

uint8_t handleTiiiiiiiildd(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Tiiiiiiiildd...' command (Transmit extended CAN frame)
    return true;
}

uint8_t handleriiil(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'riiil' command (Request standard CAN frame)
    return true;
}

uint8_t handleRiiiiiiiil(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Riiiiiiiil' command (Request extended CAN frame)
    return true;
}

uint8_t handleP(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'P' command (Switch to polling mode)
    return true;
}

uint8_t handleA(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'A' command (Switch to auto-send mode)
    return true;
}

uint8_t handleF(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'F' command (Read status flags)
    return true;
}

uint8_t handleXn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Xn' command (Set time-stamp mode)
    return true;
}

uint8_t handleWn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Wn' command (Set acceptance mask)
    return true;
}

uint8_t handleMxxxxxxxx(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Mxxxxxxxx' command (Set acceptance code)
    return true;
}

uint8_t handlemxxxxxxxx(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'mxxxxxxxx' command (Set acceptance mask)
    return true;
}

uint8_t handleUn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Un' command (Set baud rate)
    return true;
}

uint8_t handleV(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'V' command (Get version number)
    // CDC_Transmit_FS((uint8_t*)version, sizeof(version));
    return true;
}

uint8_t handleN(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'N' command (Get serial number)
    // CDC_Transmit_FS((uint8_t*)serial, sizeof(serial));
    get_dev_unique_id(outData);
    *outSize = 8;
    return true;
}

uint8_t handleZn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    (void)inData;
    (void)inSize;
    (void)outData;
    (void)outSize;
    // Handle the 'Zn' command (Set auto-reply mode)
    return true;
}

uint8_t handleQn(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    // Handle the 'Qn' command (Set flow-control mode)
    return true;
}

CmdLookupEntry cmdLookupTable[] = {
    {'S', handleSn},           // Sn[CR] command handler
    {'s', handlesxxyy},        // sxxyy[CR] command handler
    {'O', handleO},            // O[CR] command handler
    {'L', handleL},            // L[CR] command handler
    {'C', handleC},            // C[CR] command handler
    {'t', handletiiiildd},     // tiiildd...[CR] command handler
    {'T', handleTiiiiiiiildd}, // Tiiiiiiiildd...[CR] command handler
    {'r', handleriiil},        // riiil[CR] command handler
    {'R', handleRiiiiiiiil},   // Riiiiiiiil[CR] command handler
    {'P', handleP},            // P[CR] command handler
    {'A', handleA},            // A[CR] command handler
    {'F', handleF},            // F[CR] command handler
    {'X', handleXn},           // Xn[CR] command handler
    {'W', handleWn},           // Wn[CR] command handler
    {'M', handleMxxxxxxxx},    // Mxxxxxxxx[CR] command handler
    {'m', handlemxxxxxxxx},    // mxxxxxxxx[CR] command handler
    {'U', handleUn},           // Un[CR] command handler
    {'V', handleV},            // V[CR] command handler
    {'N', handleN},            // N[CR] command handler
    {'Z', handleZn},           // Zn[CR] command handler
    {'Q', handleQn},           // Qn[CR] command handler
};

void slcan_parse(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize)
{
    if (inData[*inSize - 1u] != '\r')
    {
        outData[*outSize] = '\r';
        (*outSize)++;
        return;
    }
    char command = inData[0];
    int numCommands = sizeof(cmdLookupTable) / sizeof(CmdLookupEntry);

    for (int i = 0; i < numCommands; i++)
    {
        if (cmdLookupTable[i].cmd == command)
        {
            cmdLookupTable[i].handler(inData, inSize, outData, outSize);
            outData[*outSize] = '\r';
            (*outSize)++;
            return;
        }
    }
}
