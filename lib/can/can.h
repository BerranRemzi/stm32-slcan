#ifndef CAN_H
#define CAN_H
#include "stdint.h"

void can_setup(uint8_t i);
void can_send_message(uint32_t id, uint8_t *data, uint8_t len);

#endif /* CAN_H */