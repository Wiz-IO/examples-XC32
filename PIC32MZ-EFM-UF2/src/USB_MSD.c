/*
 * USB_MSD.c by Aidan Mocke (29/12/2020)
 * Reference: http://www.usb.org/developers/docs/devclass_docs/usb_msc_boot_1.0.pdf
 */

#include "USB.h"
#include <xc.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "uf2.h"

char USB_DEVICE_DESCRIPTION_LENGTH = 18;
char USB_CONFIG_DESCRIPTOR_LENGTH = 32;
unsigned char MSD_ready = 1;
int read_capacity[8];
USB_CBW usb_cbw;
USB_CSW usb_csw;
static WriteState state = {0};

unsigned char USB_device_description[] = {
    /* Descriptor Length						*/ 0x12, /* Always 0x12 */
    /* DescriptorType: DEVICE					*/ 0x01,
    /* bcdUSB (ver 2.0)							*/ 0x00, 0x02,
    /* bDeviceClass								*/ 0x00,
    /* bDeviceSubClass							*/ 0x00,
    /* bDeviceProtocol							*/ 0x00,
    /* bMaxPacketSize0							*/ 0x40,       /* Always 0x40 for High Speed USB */
    /* idVendor									*/ 0xD8, 0x04, /* e.g. - 0x04D8 - Microchip VID */
    /* idProduct								*/ 0x09, 0x00, /* e.g. - 0x0000 */
    /* bcdDevice								*/ 0x00, 0x01, /* e.g. - 01.00 */
    /* iManufacturer							*/ 0x01,
    /* iProduct									*/ 0x02,
    /* iSerialNumber							*/ 0x03,
    /* bNumConfigurations						*/ 0x01};

unsigned char USB_config_descriptor[] = {
    /*********************************************************************
     Configuration Descriptors
     *********************************************************************/
    /*  bLength (Descriptor Length)             */ 9,
    /*  bDescriptorType: CONFIG					*/ 0x02,
    /*  wTotalLength							*/ 0x20,
    0x00, /**/
    /*  bNumInterfaces							*/ 1,
    /*  bConfigurationValue						*/ 1,
    /*  iConfiguration							*/ 0,
    /*  bmAttributes							*/ 0x80, /* bit 6 set = bus powered = 0x80, 0xC0 is for self powered */
    /*  bMaxPower								*/ 0x32, /* Value x 2mA, set to 0 for self powered */

    /*********************************************************************
      Interface 0 - HID Class
     *********************************************************************/
    /* bLength                                  */ 0x09,
    /* bDescriptorType: INTERFACE               */ 0x04,
    /* bInterfaceNumber                         */ 0x00,
    /* bAlternateSetting                        */ 0x00,
    /* bNumEndpoints: 2 endpoint(s)             */ 0x02,
    /* bInterfaceClass: Mass Storage            */ 0x08,
    /* bInterfaceSubclass:SCSI transparent      */ 0x06,
    /* bInterfaceProtocol:Bulk only             */ 0x50,
    /* iInterface                               */ 0x00,
    /*********************************************************************
    Endpoint 1 (Bulk IN)
     *********************************************************************/
    /* bLength                                  */ 0x07,
    /* bDescriptorType: ENDPOINT                */ 0x05,
    /* bEndpointAddress: IN Endpoint 1          */ 0x81,
    /* bmAttributes: BULK                       */ 0x02,
    /* max packet size (LSB)                    */ 0x00,
    /* max packet size (MSB)                    */ 0x02,
    /* polling interval                         */ 0x00,
    /*********************************************************************
     Endpoint 1 (Bulk OUT)
     *********************************************************************/
    /* bLength                                  */ 0x07,
    /* bDescriptorType: ENDPOINT                */ 0x05,
    /* bEndpointAddress: OUT Endpoint 2         */ 0x02,
    /* bmAttributes: BULK                       */ 0x02,
    /* max packet size (LSB)                    */ 0x00,
    /* max packet size (MSB)                    */ 0x02,
    /* bInterval: None for BULK                 */ 0x00,
};

static unsigned char inquiryAnswer[36] = {
    0x00,                                                                           // Peripheral Device Type (PDT) - SBC Direct-access device
    0x80,                                                                           // Removable Medium Bit is Set
    0x04,                                                                           // Version
    0x02,                                                                           // Obsolete[7:6],NORMACA[5],HISUP[4],Response Data Format[3:0]
    0x1f,                                                                           // Additional Length
    0x00,                                                                           // SCCS[7],ACC[6],TPGS[5:4],3PC[3],Reserved[2:1],PROTECT[0]
    0x00,                                                                           // BQUE[7],ENCSERV[6],VS[5],MULTIP[4],MCHNGR[3],Obsolete[2:1],ADDR16[0]
    0x00,                                                                           // Obsolete[7:6],WBUS116[5],SYNC[4],LINKED[3],Obsolete[2],CMDQUE[1],VS[0]
    'W', 'i', 'z', 'I', 'O', ' ', ' ', ' ',                                         // Vendor Identification
    'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e', ' ', ' ', ' ', ' ', // Product Identification
    '0', '.', '0', '1'                                                              // Product Revision Level
};

static unsigned char modeSenseAnswer[4] = {
    0x03, 0x00, 0x00, 0x00 //
};

static unsigned char requestSenseAnswer[18] = {
    0x70,                   // Error Code
    0x00,                   // Segment Number (Reserved)
    0x02,                   // ILI, Sense Key
    0x00, 0x00, 0x00, 0x00, // Information
    0x0A,                   // Additional Sense Length (n-7), i.e. 17-7
    0x00, 0x00, 0x00, 0x00, // Command Specific Information
    0x3A,                   // Additional Sense Code
    0x00,                   // Additional Sense Qualifier (optional)
    0x00, 0x00, 0x00, 0x00  // Reserved
};

unsigned long int disk_size = -1;

void USB_EP2_tasks()
{
    int cnt;
    USB_CBW cbw;
    USB_CSW csw;
    int block_len, block;
    unsigned int capacity_reversed;
    int Error = 0;

    if (USB_Read((unsigned char *)&cbw, 31) != 31)
        return;

    cbw.bCBWLUN &= 0x0F;            // Only the lower 4 bits matter
    csw.bCSWStatus = 0;             // Good
    csw.dCSWDataResidue = 0;        // No residue - what a word
    csw.dCSWSignature = 0x53425355; //
    csw.dCSWTag = cbw.dCBWTag;      //

    if (cbw.dCBWSignature != 0x43425355) // Signature must always be 0x43425355 for CBWs
        return;

    switch (cbw.CBWCB[0]) // Actual command number
    {
    case 0x0: // Test unit ready
    {
        if (!MSD_ready)
            Error = 2;
        break;
    }

    case 0x3: // Request sense CMD
    {
        USB_Write(requestSenseAnswer, sizeof(requestSenseAnswer));
        // Reset the sense data now
        if (MSD_ready)
            requestSenseAnswer[2] = 0;
        else
            requestSenseAnswer[2] = 2;
        break;
    }

    case 0x12: // Inquiry
    {
        // Send off inquiryAnswer
        USB_Write(inquiryAnswer, sizeof(inquiryAnswer));
        break;
    }

    case 0x1A: // Mode sense (6)
    {
        // For write protect, set modeSenseAnswer[2] |= 0x80;
        USB_Write(modeSenseAnswer, sizeof(modeSenseAnswer));
        break;
    }

    case 0x1B: // Start/stop unit
    {
        MSD_ready = ((cbw.CBWCB[4] >> 1) == 0);
        USB_EJECT_REQUEST = (MSD_ready == 0);
        USBE0CSR0bits.RXRDYC = 1;
        break;
    }

    case 0x1E: // Prevent allow removal
    {
        // NO, we DO NOT want this otherwise Windows enables write caching!
        Error = 5; // Illegal command
        break;
    }

    case 0x23: // Read format capacity
    {
        if (MSD_ready)
        {
            capacity_reversed = disk_size >> 24;                 // High byte now low
            capacity_reversed |= (disk_size & 0x00FF0000) >> 8;  // Next byte
            capacity_reversed |= (disk_size & 0x0000FF00) << 8;  // Next byte
            capacity_reversed |= (disk_size & 0x000000FF) << 25; // Low byte now high
            read_capacity[0] = 0x08000000;                       // Capacity list length - Number of descriptors = 1 x 8 = 8
            read_capacity[1] = capacity_reversed;                // Number of sectors
            read_capacity[2] = 0x02020000;                       // Size of sector, OR 2
            USB_Write((unsigned char *)&read_capacity, 12);
        }
        else
        {
            Error = 2;
        }

        break;
    }

    case 0x25: // Read capacity (10))
    {
        if (MSD_ready)
        {
            disk_size -= 1;
            capacity_reversed = disk_size >> 24;                 // High byte now low
            capacity_reversed |= (disk_size & 0x00FF0000) >> 8;  // Next byte
            capacity_reversed |= (disk_size & 0x0000FF00) << 8;  // Next byte
            capacity_reversed |= (disk_size & 0x000000FF) << 25; // Low byte now high
            disk_size += 1;
            read_capacity[0] = capacity_reversed; // Number of blocks Backwards, yay
            read_capacity[1] = 0x00020000;        // Block size in bytes (512) Backwards, yay
            USB_Write((unsigned char *)&read_capacity, 8);
        }
        else
        {
            csw.bCSWStatus |= 1;
        }
        break;
    }

    case 0x28: // READ-10
    {
        if (MSD_ready)
        {
            block_len = (int)(cbw.CBWCB[7] << 8) | cbw.CBWCB[8];
            block = (int)(cbw.CBWCB[2] << 24) | (int)(cbw.CBWCB[3] << 16) | (cbw.CBWCB[4] << 8) | (cbw.CBWCB[5]);
            for (int i = block; i < block_len + block; i++)
            {
                uf2_read_block(i, USB_Buffer);
                USB_Write(USB_Buffer, 512);
            }
        }
        else
        {
            Error = 2;
        }
        break;
    }

    case 0x2A: // WRITE-10
    {
        if (MSD_ready)
        {
            block_len = (int)(cbw.CBWCB[7] << 8) | cbw.CBWCB[8];
            block = (int)(cbw.CBWCB[2] << 24) | (int)(cbw.CBWCB[3] << 16) | (cbw.CBWCB[4] << 8) | (cbw.CBWCB[5]);
            USBCSR3bits.ENDPOINT = 2;
            for (int i = block; i < block_len + block; i++)
            {
                if (USB_Read(USB_Buffer, 512) != 512)
                {
                    Error = 2;
                }
                else
                {
                    uf2_write_block(i, USB_Buffer, &state);
                }
            }
        }
        else
        {
            Error = 2;
        }
        break;
    }

    case 0x2F: // Verify
    {
        break;
    }

    default:
    {
        Error = 5; // Illegal request (aka not supported request)
        requestSenseAnswer[12] = 0x20;
        break;
    }
    }

    if (Error)
    {
        csw.bCSWStatus = 1;
        requestSenseAnswer[2] = Error;
    }

    // Send the CSW
    memcpy(USB_Buffer, &csw, 13);
    USB_Write(USB_Buffer, 13);
}
