#ifndef USBVCP_H
#define	USBVCP_H

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/02/18
 */

#ifdef	__cplusplus
extern "C"
{
#endif

#define USB_DESCRIPTOR_CDC		0x24

#define USB_CDC_HEADER			0x00
#define USB_CDC_CM				0x01
#define USB_CDC_ACM				0x02
#define USB_CDC_DLM 			0x03
#define USB_CDC_TR				0x04
#define USB_CDC_TCLSR			0x05
#define USB_CDC_UNION			0x06
#define USB_CDC_CS				0x07
#define USB_CDC_TOM				0x08
#define USB_CDC_TERM			0x09
#define USB_CDC_NCT				0x0A
#define USB_CDC_PU				0x0B
#define USB_CDC_EU				0x0C
#define USB_CDC_MCM				0x0D
#define USB_CDC_CAPI			0x0E
#define USB_CDC_ETH				0x0F
#define USB_CDC_ATM				0x10

#define USB_CDC_CM_DATA_CHANNEL	0x02
#define USB_CDC_CM_SELF_MANAGE	0x01

#define USB_ACM_NETWORK_CON		0x08
#define USB_ACM_SEND_BREAK		0x04
#define USB_ACM_LINE_CODING		0x02
#define USB_ACM_COMM_FEAT		0x01

typedef struct
{
	uint8_t length;
	uint8_t descriptorType;
	uint8_t descriptorSubtype;
	uint16_t cdcVersion;
} usbCDCHeader_t;

typedef struct
{
	uint8_t length;
	uint8_t descriptorType;
	uint8_t descriptorSubtype;
	uint8_t capabilities;
	uint8_t dataInterface;
} usbCDCCallMgmt_t;

typedef struct
{
	uint8_t length;
	uint8_t descriptorType;
	uint8_t descriptorSubtype;
	uint8_t capabilities;
} usbCDCHeaderACM_t;

typedef struct
{
	uint8_t length;
	uint8_t descriptorType;
	uint8_t descriptorSubtype;
	uint8_t masterInterface;
	uint8_t slaveInterface0;
} usbCDCUnion2_t;

typedef enum
{
	USB_REQUEST_SET_COMM_FEATURE = 0x02,
	USB_REQUEST_GET_COMM_FEATURE = 0x03,
	USB_REQUEST_CLEAR_COMM_FEATURE = 0x04,
	USB_REQUEST_SET_LINE_CODING = 0x20,
	USB_REQUEST_GET_LINE_CODING = 0x21,
	USB_REQUEST_SET_CONTROL_LINE = 0x22,
	USB_REQUEST_SEND_BREAK = 0x23
} usbCDCRequest_t;

typedef struct
{
	uint32_t baudRate;
	uint8_t format;
	uint8_t parityType;
	uint8_t dataBits;
} usbLineCoding_t;

extern void usbCDCInit();
extern void usbHandleCDCRequest(volatile usbBDTEntry_t *BD);
extern void usbServiceCDCDataEP();

#ifdef	__cplusplus
}
#endif

#endif	/* USBVCP_H */
