#ifndef USB_H
#define USB_H
#include <stdint.h>

void usb_init(void);
void usb_loop(void);
void usb_send(uint8_t *data, uint8_t size);

#endif