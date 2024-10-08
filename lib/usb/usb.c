#include <stdlib.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/stm32/st_usbfs.h>
#include "board.h"
#include "usb.h"
#include "stddef.h"
#include "slcan.h"

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

static char serial_no[9] = "killbill";

usbd_device *_usbd_dev = 0;

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
															   .bLength = USB_DT_ENDPOINT_SIZE,
															   .bDescriptorType = USB_DT_ENDPOINT,
															   .bEndpointAddress = 0x01,
															   .bmAttributes = USB_ENDPOINT_ATTR_BULK,
															   .wMaxPacketSize = 64,
															   .bInterval = 1,
														   },
														   {
															   .bLength = USB_DT_ENDPOINT_SIZE,
															   .bDescriptorType = USB_DT_ENDPOINT,
															   .bEndpointAddress = 0x82,
															   .bmAttributes = USB_ENDPOINT_ATTR_BULK,
															   .wMaxPacketSize = 64,
															   .bInterval = 1,
														   }};

static const struct
{
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors),
}};

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
												  .num_altsetting = 1,
												  .altsetting = comm_iface,
											  },
											  {
												  .num_altsetting = 1,
												  .altsetting = data_iface,
											  }};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"JeeLabs",
	"SerPlus",
	serial_no,
};

char *get_dev_unique_id(char *s);
void usb_preinit(void);

static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
															 uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest)
	{
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE:
	{
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		char local_buf[10];
		struct usb_cdc_notification *notif = (void *)local_buf;

		/* We echo signals back to host as notification. */
		notif->bmRequestType = 0xA1;
		notif->bNotification = USB_CDC_NOTIFY_SERIAL_STATE;
		notif->wValue = 0;
		notif->wIndex = 0;
		notif->wLength = 2;
		local_buf[8] = req->wValue & 3;
		local_buf[9] = 0;
		// usbd_ep_write_packet(0x83, buf, 10);
		return USBD_REQ_HANDLED;
	}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding))
			return USBD_REQ_NOTSUPP;
		return USBD_REQ_HANDLED;
	}
	return USBD_REQ_NOTSUPP;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	(void)usbd_dev;
#ifdef USE_RING_BUFFER
	// back pressure: don't read the packet if there's not enough room in ring
	if ((output_ring.begin - (output_ring.end + 1)) % BUFFER_SIZE <= 64)
		return;

	uint8_t buf[64];
	int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, sizeof buf);

	if (len)
	{
		// Retrieve the data from the peripheral.
		ring_write(&output_ring, buf, len);

		// Enable usart transmit interrupt so it sends out the data.
		USART_CR1(USART1) |= USART_CR1_TXEIE;
	}
#else
	uint8_t buf[64];
	uint8_t out[64];
	uint8_t outSize = 0;
	uint8_t len = (uint8_t)usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	if (len)
	{
		slcan_decode(buf, &len, out, &outSize);
		usbd_ep_write_packet(usbd_dev, 0x82, out, outSize);
	}
#endif
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;
	(void)usbd_dev;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
		usbd_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		cdcacm_control_request);
}

char *get_dev_unique_id(char *s)
{
#if defined(STM32F4) || defined(STM32F2)
#define UNIQUE_SERIAL_R 0x1FFF7A10
#define FLASH_SIZE_R 0x1fff7A22
#elif defined(STM32F3)
#define UNIQUE_SERIAL_R 0x1FFFF7AC
#define FLASH_SIZE_R 0x1fff77cc
#elif defined(STM32L1)
#define UNIQUE_SERIAL_R 0x1ff80050
#define FLASH_SIZE_R 0x1FF8004C
#elif defined(STM32F0)
#define UNIQUE_SERIAL_R 0x1FFFF7AC
#define FLASH_SIZE_R 0x1FFFF7CC
#else
#define UNIQUE_SERIAL_R 0x1FFFF7E8;
#define FLASH_SIZE_R 0x1ffff7e0
#endif
	volatile uint32_t *unique_id_p = (volatile uint32_t *)UNIQUE_SERIAL_R;
	uint32_t unique_id = *unique_id_p ^		  // was "+" in original BMP
						 *(unique_id_p + 1) ^ // was "+" in original BMP
						 *(unique_id_p + 2);
	int i;

	// Calculated the upper flash limit from the exported data
	//  in theparameter block
	// max_address = (*(uint32_t *) FLASH_SIZE_R) <<10;
	// Fetch serial number from chip's unique ID
	for (i = 0; i < 8; i++)
	{
		s[7 - i] = ((unique_id >> (4 * i)) & 0xF) + '0';
	}
	for (i = 0; i < 8; i++)
		if (s[i] > '9')
			s[i] += 'A' - '9' - 1;
	s[8] = 0;

	return s;
}

extern void delay_125ms(void);

void usb_preinit(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	*USB_BCDR_REG &= ~USB_BCDR_DPPU;

	delay_125ms();

	*USB_BCDR_REG |= USB_BCDR_DPPU;
	delay_125ms();
}

void usb_init(void)
{
	usb_preinit();

	get_dev_unique_id(serial_no);

	_usbd_dev = usbd_init(&st_usbfs_v2_usb_driver, &dev, &config,
						  usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
	usbd_register_set_config_callback(_usbd_dev, cdcacm_set_config);
}

void usb_loop(void)
{
	usbd_poll(_usbd_dev);
}

void usb_send(uint8_t *data, uint8_t size){
	usbd_ep_write_packet(_usbd_dev, 0x82, data, size);
}