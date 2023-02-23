/*
 * USB.c by Aidan Mocke (29/12/2020)
 */

#include "config.h"
#include "USB.h"
#include <xc.h>
#include <string.h>
#include <stdio.h>

// Language description
unsigned char string0[] = {4, 3, 0x09, 0x04}; // 0x0409 = English
// Vendor description
unsigned short string1[] = {STR_DESC(11), 'W', 'i', 'z', 'I', 'O', ' ', 'P', 'I', 'C', '3', '2'};
// Product description
unsigned short string2[] = {STR_DESC(12), 'U', 'S', 'B', ' ', 'M', 'S', 'D', ' ', 'B', 'o', 'o', 't'};
// Serial number description
unsigned short string3[] = {STR_DESC(8), '1', '2', '3', '4', '5', '6', '7', '8'};
// Configuration description
unsigned short string4[] = {STR_DESC(8), 'U', 'L', 'T', 'R', 'A', ' ', 'S', 'E'};
// Interface description
unsigned short string5[] = {STR_DESC(7), 'S', 't', 'o', 'r', 'a', 'g', 'e'};

volatile char USB_set_address = 0;
volatile char USB_line_control_state = 0;
unsigned char USB_address = 0;
volatile char USB_READY = 0;
volatile char USB_EP0_RECEIVED = 0;
volatile char USB_EP1_RECEIVED = 0;
volatile char USB_EP2_RECEIVED = 0;
volatile char USB_EJECTED = 0;
volatile char USB_POWERED = 0;
volatile char USB_EJECT_REQUEST = 0;
USB_TRANSACTION USB_transaction;

unsigned char __attribute__((aligned(8))) USB_EP0_buffer[64];
unsigned char __attribute__((aligned(8))) USB_Buffer[512]; 

void USB_init_endpoints()
{
    // PRINT("#\n");
    USBE1CSR0bits.MODE = 0; // EP1 is OUT (OUT from host = in to device = RX)

    // Clear all interrupt flags
    USBCSR0bits.EP0IF = 0;
    USBCSR0bits.EP1TXIF = 0;
    USBCSR1bits.EP1RXIF = 0;
    USBCSR0bits.EP2TXIF = 0;
    USBCSR1bits.EP2RXIF = 0;

    // Set the maximum transfer length for each endpoint
    // Configure endpoint 0 first.
    USBE0CSR0bits.TXMAXP = 64; // Set endpoint 0 buffer size to 16 bytes (can be a maximum of 64 for EP0)

    // And next my custom endpoints
    USBE1CSR0bits.TXMAXP = 512; // Endpoint 1 - Maximum TX payload / transaction - 512
    USBE1CSR1bits.RXMAXP = 512; // Endpoint 1 - Maximum TX payload / transaction - 512

    USBE2CSR0bits.TXMAXP = 512; // Endpoint 2 - Maximum TX payload / transaction - 512
    USBE2CSR1bits.RXMAXP = 512; // Endpoint 2 - Maximum TX payload / transaction - 512

    // Specify which kinds of endpoint we will be using
    USBE1CSR2bits.PROTOCOL = 2; // Endpoint 1 - Bulk mode
    USBE2CSR2bits.PROTOCOL = 2; // Endpoint 2 - Bulk mode

    // Enable DISNYET
    USBE1CSR1bits.PIDERR = 0; // Clear DISNYET to enable NYET replies
    USBE2CSR1bits.PIDERR = 0; // Clear DISNYET to enable NYET replies

    // Set up buffer locations for endpoint 1
    USBCSR3bits.ENDPOINT = 1;
    USBFIFOA = 0x00A000A;
    USBIENCSR0bits.CLRDT = 1;

    // Set up buffer locations for endpoint 2
    USBCSR3bits.ENDPOINT = 2;
    USBFIFOA = 0x04A004A;
    USBIENCSR0bits.CLRDT = 1;

    USBE1CSR3bits.TXFIFOSZ = 0x9; // Transmit FIFO Size bits - 512 bytes
    USBE1CSR3bits.RXFIFOSZ = 0x9; // Transmit FIFO Size bits - 512 bytes
    USBE2CSR3bits.TXFIFOSZ = 0x9; // Transmit FIFO Size bits - 512 bytes
    USBE2CSR3bits.RXFIFOSZ = 0x9; // Transmit FIFO Size bits - 512 bytes

    // Set maximum size for each packet before splitting occurs
    USBOTGbits.TXFIFOSZ = 0b0110; // 0b0110 = 512 bytes
    USBOTGbits.RXFIFOSZ = 0b0110; // 0b0110 = 512 bytes

    // Set receive endpoint 1 to High Speed
    USBE1CSR3bits.SPEED = 1;
    USBE2CSR3bits.SPEED = 1;

    // Disable Isochronous mode for endpoints 1 and 2
    USBE1CSR0bits.ISO = 0; // Isochronous TX Endpoint Disable bit (Device mode).
    USBE2CSR0bits.ISO = 0; // Isochronous TX Endpoint Disable bit (Device mode).

    USBCSR1 = 0;

    // Set current endpoint to EP2
    USBCSR3bits.ENDPOINT = 2;
}

void USB_init()
{
    USBCSR0 = 0;              //
    USB_EP0_RECEIVED = 0;     //
    USB_EP2_RECEIVED = 0;     //
    USB_init_endpoints();     //
    USBCSR0bits.SOFTCONN = 0; // Initially disable it while we go into setup
    USB_address = 0;          // We haven't been given an address yet
    USBCSR0bits.FUNC = 0;     // Initially set the USB address to 0 until later when the host assigns us an address
    USBCSR2bits.RESETIE = 1;  // Enable the reset interrupt
    USBCRCONbits.USBIE = 1;   // Enable USB module interrupt
    USBCSR0bits.HSEN = 1;     // Enable High Speed (480Mbps) USB mode
}

int USB_EP0_wait_TXRDY()
{
    int timeout;
    timeout = 0;
    while (USBE0CSR0bits.TXRDY)
    {
        timeout++;
        if (timeout > USB_EP0_WAIT_TIMEOUT)
            return 1;
    }
    return 0;
}

int USB_EP1_wait_TXRDY()
{
    int timeout;
    timeout = 0;
    while (USBE1CSR0bits.TXPKTRDY)
    {
        timeout++;
        if (timeout > USB_EP0_WAIT_TIMEOUT)
            return 1;
    }
    return 0;
}

void USB_EP0_send(unsigned char *buffer, uint32_t length)
{
    int cnt;
    unsigned char *FIFO_buffer;
    FIFO_buffer = (unsigned char *)&USBFIFO0;
    if (USB_EP0_wait_TXRDY())
        return;
    cnt = 0;
    while (cnt < length)
    {
        *FIFO_buffer = *buffer++;
        cnt++;
        if ((cnt > 0) && (cnt % 64 == 0))
        {
            USBE0CSR0bits.TXRDY = 1;
            if (USB_EP0_wait_TXRDY())
                return;
        }
    }
    USBE0CSR0bits.TXRDY = 1;
    USB_EP0_wait_TXRDY();
    if (length < 4)
        USBE0CSR0bits.RXRDYC = 1;
}

void USB_EP0_receive(unsigned char *buffer, uint32_t length)
{
    int cnt;
    unsigned char *FIFO_buffer;
    FIFO_buffer = (unsigned char *)&USBFIFO0;
    for (cnt = 0; cnt < length; cnt++)
        *buffer++ = *(FIFO_buffer + (cnt & 3));
    USBE0CSR0bits.RXRDYC = 1;
}

void USB_Write(unsigned char *buffer, int length)
{
    int cnt;
    unsigned char *FIFO_buffer;
    USBE1CSR0bits.MODE = 1; // EP1 is TX
    FIFO_buffer = (unsigned char *)&USBFIFO1;
    if (USB_EP1_wait_TXRDY())
        return;
    cnt = 0;
    while (cnt < length)
    {
        *FIFO_buffer = *buffer++;
        cnt++;
        if ((cnt > 0) && (cnt % 512 == 0))
        {
            USBE1CSR0bits.TXPKTRDY = 1;
            if (USB_EP1_wait_TXRDY())
                return;
        }
    }
    if ((cnt % 512) != 0)
    {
        USBE1CSR0bits.TXPKTRDY = 1;
        USB_EP1_wait_TXRDY();
    }
}

int USB_Read(unsigned char *buffer, uint32_t length) // 31 | 512
{
    int cnt = 0;
    unsigned char *FIFO_buffer = (unsigned char *)&USBFIFO2;
    while (length)
    {
        if (USBE2CSR1bits.RXPKTRDY)
        {
            cnt = USBE2CSR2bits.RXCNT; // 31 | 512
            for (int i = 0; i < cnt; i++)
                *buffer++ = *(FIFO_buffer + (i & 3));
            length -= cnt;
            USBE2CSR1bits.RXPKTRDY = 0;
        }
    }
    return cnt;
}

void USB_connect()
{
    PRINT_FUNC;
    USBCSR0bits.SUSPEN = 0;
    USBCSR0bits.SOFTCONN = 1;
}

void USB_disconnect()
{
    PRINT_FUNC;
    IEC4bits.USBIE = 0;
    USBCSR0bits.SUSPEN = 1;
    USBOTGbits.SESSION = 0;
    USBCSR0bits.SOFTCONN = 0;
    USB_EJECTED = 1;
}

void USB_EP0_tasks()
{
    // We're not going to bother with whether bmRequestType is IN or OUT for the most part
    switch (USB_transaction.bmRequest)
    {

    case 0xFF: // Mass storage reset
    {
        // Upon reset, we need to reset all endpoints
        USB_init_endpoints();
        break;
    }

    case 0xFE: // Get max LUN (Logical Unit)
    {
        // Send a 0
        USB_EP0_buffer[0] = 0;
        USB_EP0_send(USB_EP0_buffer, 1);
        break;
    }

    case 0xC: // Sync frame
    {
        USBE0CSR0bits.STALL = 1;
        break;
    }

    case 0x0:
    {
        if (USB_transaction.bmRequestType == 0x80) // Get status
            USBE0CSR0bits.RXRDYC = 1;
        if (USB_transaction.bmRequestType == 0x00) // Select function
            USBE0CSR0bits.RXRDYC = 1;
        break;
    }

    case 0x1: // Get report via control endpoint
    {
        // Required to support this, just send 4 0's to make it happy
        break;
    }

    case 0x2: // Get idle
    {
        break;
    }

    case 0x3: // Get protocol
    {
        break;
    }

    case 0xB: // Set protocol
    {
        break;
    }

    case 0xA: // Set idle
    {
        USB_READY = 1;
        break;
    }

    case 0x5: // Set USB address
    {
        USBE0CSR0bits.RXRDYC = 1;
        USB_address = USB_EP0_buffer[2];
        USB_set_address = 1;
        USB_READY = 1;
        break;
    }

    case 0x6: // Get descriptor
    {
        switch (USB_transaction.wValue >> 8)
        {
        case 0x1: // Get device descriptor
        {
            if (USB_transaction.wLength < USB_DEVICE_DESCRIPTION_LENGTH)
                USB_EP0_send(USB_device_description, USB_transaction.wLength);
            else
                USB_EP0_send(USB_device_description, USB_DEVICE_DESCRIPTION_LENGTH);
            break;
        }

        case 0x2: // Get configuration descriptor
        {
            if (USB_transaction.wLength < USB_CONFIG_DESCRIPTOR_LENGTH)
                USB_EP0_send(USB_config_descriptor, USB_transaction.wLength);
            else
                USB_EP0_send(USB_config_descriptor, USB_CONFIG_DESCRIPTOR_LENGTH);
            break;
        }

        case 0x3: // Get string descriptors
        {
            switch (USB_transaction.wValue & 0xFF)
            {
            case 0x0: // String 0 - Language ID
            {
                memcpy(USB_EP0_buffer, string0, sizeof(string0));
                USB_EP0_send(USB_EP0_buffer, sizeof(string1));
                break;
            }

            case 0x1: // String 1 - Vendor
            {
                memcpy(USB_EP0_buffer, string1, sizeof(string1));
                USB_EP0_send(USB_EP0_buffer, sizeof(string1));
                break;
            }

            case 0x2: // String 2 - Product name
            {
                memcpy(USB_EP0_buffer, string2, sizeof(string2));
                USB_EP0_send(USB_EP0_buffer, sizeof(string2));
                break;
            }

            case 0x3: // String 3 - Serial number
            {
                memcpy(USB_EP0_buffer, string3, sizeof(string3));
                USB_EP0_send(USB_EP0_buffer, sizeof(string3));
                break;
            }

            case 0x4: // String 4 - Configuration
            {
                memcpy(USB_EP0_buffer, string4, sizeof(string4));
                USB_EP0_send(USB_EP0_buffer, sizeof(string4));
                break;
            }

            case 0x5: // String 5 - Interface
            {
                memcpy(USB_EP0_buffer, string5, sizeof(string5));
                USB_EP0_send(USB_EP0_buffer, sizeof(string5));
                break;
            }

            default:
            {
                PRINT("MISSING string X\n");
                break;
            }

            } // string

            break;
        }

        default:
        {
            PRINT("MISSING descriptor X\n");
            break;
        }

        } // descriptor

        break;
    }

    case 0x9: // Set report
    {
        // Zegads we have enumeration!
        USBE0CSR0bits.RXRDYC = 1; // Respond with this on EP0
        break;
    }

    default:
    {
        USBE0CSR0bits.STALL = 1;
        PRINT("MISSING REQUEST\n");
        break;
    }

    } // request
}

void USB_handler()
{
    unsigned char RESETIF, EP0IF, EP1TXIF, EP1RXIF, EP2RXIF;
    unsigned int CSR0, CSR1, CSR2;
    CSR0 = USBCSR0;
    EP0IF = (CSR0 & (1 << 16)) ? 1 : 0;
    EP1TXIF = (CSR0 & (1 << 17)) ? 1 : 0;
    CSR1 = USBCSR1;
    EP2RXIF = (CSR1 & (1 << 2)) ? 1 : 0;
    CSR2 = USBCSR2;
    RESETIF = (CSR2 & (1 << 18)) ? 1 : 0;
    if (RESETIF)
    {
        USB_init_endpoints();
        USBCSR2bits.RESETIF = 0;
    }

    if (EP0IF)
    {
        USB_EP0_RECEIVED = 1;
        USBCSR0bits.EP0IF = 0;
    }

    if (EP1TXIF) // TX
    {
        USBCSR0bits.EP1TXIF = 0;
        USBCSR3bits.ENDPOINT = 1;
        USBIENCSR0bits.MODE = 0;
    }

    if (EP2RXIF) // RX
    {
        USBCSR1bits.EP2RXIF = 0;
        USBCSR3bits.ENDPOINT = 2;
        USBIENCSR0bits.MODE = 0;
        USB_EP2_RECEIVED = 1;
    }
}

void USB_tasks()
{
    if (USB_EP0_RECEIVED)
    {
        USB_EP0_RECEIVED = 0;
        if (USB_EJECT_REQUEST)
        {
            USB_EJECT_REQUEST = 0;
            USB_EJECTED = 1; // DIsconnect the hardware now
            return;
        }

        if (USB_set_address) // Do we need the set the USB address?
        {
            USBCSR0bits.FUNC = USB_address & 0x7F;
            USB_set_address = 0;
        }

        USB_EP0_receive(USB_EP0_buffer, USBE0CSR2bits.RXCNT);
        USB_transaction.bmRequestType = USB_EP0_buffer[0];
        USB_transaction.bmRequest = USB_EP0_buffer[1];
        USB_transaction.signature = ((int)USB_EP0_buffer[0] << 8) | USB_EP0_buffer[1];
        USB_transaction.wValue = (int)(USB_EP0_buffer[3] << 8) | USB_EP0_buffer[2];
        USB_transaction.wIndex = (int)(USB_EP0_buffer[5] << 8) | USB_EP0_buffer[4];
        USB_transaction.wLength = (int)(USB_EP0_buffer[7] << 8) | USB_EP0_buffer[6];
        USB_EP0_tasks();
        if (USB_transaction.wLength == 0)
            USBE0CSR0bits.DATAEND = 1;
        if (USBE0CSR0bits.SETEND)
            USBE0CSR0bits.SETENDC = 1;
    }

    if (USB_EP2_RECEIVED) // RX
    {
        USB_EP2_RECEIVED = 0;
        USB_EP2_tasks();
    }
}
