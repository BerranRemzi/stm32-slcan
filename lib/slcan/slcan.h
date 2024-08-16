/*
 * SLCAN.h
 *
 *  Created on: Aug 11, 2024
 *      Author: Berran
 */

#ifndef SLCAN_SLCAN_H_
#define SLCAN_SLCAN_H_
#include "stdint.h"

/*  -----------  defines  ------------------------------------------------
 */

/** @name  CAN Message flags
 *  @brief CAN frame types (encoded in the CAN identifier)
 *  @{
 */
#define CAN_STD_FRAME   0x00000000U     /**< standard frame format (11-bit) */
#define CAN_XTD_FRAME   0x80000000U     /**< extended frame format (29-bit) */
#define CAN_ERR_FRAME   0x40000000U     /**< error frame (not supported) */
#define CAN_RTR_FRAME   0x20000000U     /**< remote frame */
/** @} */

/** @name  CAN Identifier
 *  @brief CAN identifier masks
 *  @{ */
#define CAN_STD_MASK    0x000007FFU     /**< highest 11-bit identifier */
#define CAN_XTD_MASK    0x1FFFFFFFU     /**< highest 29-bit identifier */
/** @} */

/** @name  CAN Data Length
 *  @brief CAN payload length and DLC definition
 *  @{ */
#define CAN_DLC_MAX     8U              /**< max. data lenth code (CAN 2.0) */
#define CAN_LEN_MAX     8U              /**< max. payload length (CAN 2.0) */
/** @} */

/** @name  CAN Baud Rate Indexes
 *  @brief CAN baud rate indexes defined by SLCAN protocol
 *  @{ */
#define CAN_10K         0U              /**< bit-rate:   10 kbit/s */
#define CAN_20K         1U              /**< bit-rate:   20 kbit/s */
#define CAN_50K         2U              /**< bit-rate:   50 kbit/s */
#define CAN_100K        3U              /**< bit-rate:  100 kbit/s */
#define CAN_125K        4U              /**< bit-rate:  120 kbit/s */
#define CAN_250K        5U              /**< bit-rate:  250 kbit/s */
#define CAN_500K        6U              /**< bit-rate:  500 kbit/s */
#define CAN_800K        7U              /**< bit-rate:  800 kbit/s */
#define CAN_1000K       8U              /**< bit-rate: 1000 kbit/s */
#define CAN_1M          CAN_1000K
/** @} */

#define CAN_OK (uint8_t)'\r'
#define CAN_ERROR (uint8_t)'\a'
#define CAN_AUTOPOLL (uint8_t)'z'

void slcan_decode(uint8_t *inData, uint8_t *inSize, uint8_t *outData, uint8_t *outSize);

#endif /* SLCAN_SLCAN_H_ */
