#ifndef _USB_H 
#define _USB_H

#include <stdint.h>

#define CONCAT_EX(X, Y, Z) X##Y##Z
#define CONCAT(X, Y, Z) CONCAT_EX(X, Y, Z)

//#define USB_USE_ISR

#define STR_DESC(l) (((((l)) << 1) & 0xFF) | 0x0300)

#define USB_ENDPOINT_BUFFER_SIZE 512
#define USB_EP0_WAIT_TIMEOUT 4000

typedef struct
{
    int baud;
    short stop_bits;
    short parity_bits;
    short data_bits;
} USB_LINE_CODING;

typedef struct
{
    volatile unsigned char bmRequestType;
    volatile unsigned char bmRequest;
    volatile unsigned short wValue;
    volatile unsigned short wIndex;
    volatile unsigned short wLength;
    unsigned char rx_buffer[8];
    volatile unsigned short signature;
} USB_TRANSACTION;

typedef struct __attribute__((packed))
{
    unsigned int dCBWSignature;
    unsigned int dCBWTag;
    unsigned int dCBWDataTransferLength;
    unsigned char bmCBWFlags;
    unsigned char bCBWLUN;
    unsigned char bCBWCBLength;
    unsigned char CBWCB[16];

} USB_CBW; // Command Block Wrapper - The actual command block

typedef struct __attribute__((packed))
{
    unsigned int dCSWSignature;
    unsigned int dCSWTag;
    unsigned int dCSWDataResidue;
    unsigned char bCSWStatus;
} USB_CSW; // Command Status Wrapper - Status of a command block

extern unsigned char USB_device_description[];
extern unsigned char USB_config_descriptor[];

// String descriptors. All have a common format: [byte] string_length, [byte] data_type (3 = string), UTF-16 encoded string

extern unsigned char  string0[]; // Language description
extern unsigned short string1[]; // Vendor description
extern unsigned short string2[]; // Product description
extern unsigned short string3[]; // Serial number description
extern unsigned short string4[]; // Configuration description
extern unsigned short string5[]; // Interface description

extern unsigned char USB_address; // Address given to us by the host

extern volatile char USB_READY;
extern volatile char USB_EP0_RECEIVED;
extern volatile char USB_EP2_RECEIVED;
extern unsigned char MSD_ready;
extern volatile char USB_EJECTED;
extern volatile char USB_POWERED;
extern volatile char USB_EJECT_REQUEST;
extern volatile int USB_bytes_sent;

extern unsigned char USB_EP0_buffer[64];
extern unsigned char USB_Buffer[512];

extern char USB_DEVICE_DESCRIPTION_LENGTH;
extern char USB_CONFIG_DESCRIPTOR_LENGTH;

void USB_init_endpoints();
void USB_init();
void USB_connect();
void USB_disconnect();
void USB_EP0_receive(unsigned char *buffer, uint32_t length);
void USB_EP0_send(unsigned char *buffer, uint32_t length);
void USB_Write(unsigned char *buffer, int length);
int USB_Read(unsigned char *buffer, uint32_t length);
int USB_EP1_wait_TXRDY();
void USB_EP1_tasks();
void USB_EP2_tasks();
void USB_tasks();
void USB_handler();

#define USB_IPL ipl7srs

#endif /* _USB_H */