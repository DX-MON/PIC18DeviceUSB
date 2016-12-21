#include <stdint.h>
#include <stdbool.h>
#include "usbTypes.h"
#include "usb.h"
#include "usbCDC.h"
#include "usbUART.h"
#include "uart.h"

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/02/18
 */

#define USB_BUFFER_SRC_CHAR		2

typedef struct
{
	uint8_t source;
	union
	{
		void *mem;
		const void *flash;
		char ch;
	} data;
	uint16_t len;
} sendFIFOEntry_t;

usbLineCoding_t usbCDCLineCoding;
char usbCDCCtrlBuffer[64] __at(0x5D0);
uint8_t dataFullness, readCounter;

sendFIFOEntry_t sendFIFO[5];
uint8_t sendFIFOCount, sendChar;

/* Define our endpoint 1 data buffers */
volatile uint8_t usbEP1In[USB_EP1_IN_LEN] __at(USB_EP1_IN_ADDR);
volatile uint8_t usbEP1Out[USB_EP1_OUT_LEN] __at(USB_EP1_OUT_ADDR);

void usbCDCInit()
{
	volatile usbBDTEntry_t *ep1BD;

	usbCDCLineCoding.baudRate = 11250;
	usbCDCLineCoding.format = 0;
	usbCDCLineCoding.parityType = 0;
	usbCDCLineCoding.dataBits = 8;

	readCounter = 0;
	dataFullness = 0;
	sendFIFOCount = 0;
	usbStatusInEP[1].xferCount = 0;
	usbStatusInEP[1].epLen = USB_EP1_IN_LEN;
	usbStatusOutEP[1].xferCount = 64 - dataFullness;
	usbStatusOutEP[1].epLen = USB_EP1_OUT_LEN;

	ep1BD = &usbBDT[usbStatusOutEP[1].ep.value];
	ep1BD->count = USB_EP1_OUT_LEN;
	ep1BD->address = USB_EP1_OUT_ADDR;
	ep1BD->status.value = 0;
	ep1BD->status.dataToggleSync = 0;
	ep1BD->status.dataToggleSyncEn = 1;
	ep1BD->status.usbOwned = 1;

	usbStatusInEP[1].ep.buff = 1;
	ep1BD = &usbBDT[usbStatusInEP[1].ep.value];
	ep1BD->status.value = 0;
	ep1BD->status.dataToggleSync = 1;
	ep1BD->status.dataToggleSyncEn = 1;
	usbStatusInEP[1].ep.buff = 0;

	//uartInit();
}

void usbRequestSetLineCoding()
{
	usbLineCoding_t *lineCoding = (usbLineCoding_t *)usbCDCCtrlBuffer;
	usbCDCLineCoding.baudRate = lineCoding->baudRate;
}

void usbHandleCDCRequest(volatile usbBDTEntry_t *BD)
{
	volatile usbSetupPacket_t *packet = addrToPtr(BD->address);
	if (packet->requestType.type != USB_REQUEST_TYPE_CLASS)
		return;

	switch (packet->request)
	{
		/*
		 * 	USB_REQUEST_SET_COMM_FEATURE = 0x02,
	USB_REQUEST_GET_COMM_FEATURE = 0x03,
	USB_REQUEST_CLEAR_COMM_FEATURE = 0x04
		 */
		case USB_REQUEST_SET_LINE_CODING:
			usbStatusOutEP[0].buffSrc = USB_BUFFER_SRC_MEM;
			usbStatusOutEP[0].buffer.memPtr = usbCDCCtrlBuffer;
			usbStatusOutEP[0].xferCount = sizeof(usbLineCoding_t);
			usbStatusOutEP[0].func = usbRequestSetLineCoding;
			usbStatusOutEP[0].needsArming = 1;
			break;
		case USB_REQUEST_GET_LINE_CODING:
			/* Returns the current Line Coding configuration */
			usbStatusInEP[0].buffSrc = USB_BUFFER_SRC_MEM;
			usbStatusInEP[0].buffer.memPtr = &usbCDCLineCoding;
			usbStatusInEP[0].xferCount = sizeof(usbLineCoding_t);
			usbStatusInEP[0].needsArming = 1;
			break;
		case USB_REQUEST_SET_CONTROL_LINE:
			/* Generate a reply that is 0 bytes long to acknowledge */
			usbStatusInEP[0].needsArming = 1;
			break;
		case USB_REQUEST_SEND_BREAK:
			/* Just acknowledge, don't actually do anything */
			usbStatusInEP[0].needsArming = 1;
			break;
	}
}

void usbHandleDataEPIn()
{
	volatile usbBDTEntry_t *ep1BD;

	// If there is nothing to transfer or a transfer is already in progress, don't do anything.
	if (usbStatusInEP[1].xferCount == 0)
	{
		--sendFIFOCount;
		if (usbUARTDataSent())
			return;
	}

	ep1BD = &usbBDT[usbStatusInEP[1].ep.value];
	ep1BD->address = USB_EP1_IN_ADDR;
	usbServiceEPWriteArm(ep1BD, 1);
}

void usbHandleDataEPOut()
{
	volatile usbBDTEntry_t *ep1BD = &usbBDT[usbPacket.value];
	bool lastDTS = usbBDT[usbPacket.value].status.dataToggleSync;
	uint8_t readCount = ep1BD->count;
	if (readCount > usbStatusOutEP[1].xferCount)
		readCount = usbStatusOutEP[1].xferCount;
	dataFullness += readCount;

	//uartIRQ();
	if (dataFullness == readCounter)
	{
		readCounter = 0;
		dataFullness = 0;
	}

	ep1BD = &usbBDT[usbStatusOutEP[1].ep.value];
	ep1BD->count = USB_EP1_OUT_LEN - dataFullness;
	ep1BD->address = USB_EP1_OUT_ADDR + dataFullness;
	ep1BD->status.value = 0;
	if (lastDTS)
		ep1BD->status.dataToggleSync = 0;
	else
		ep1BD->status.dataToggleSync = 1;
	ep1BD->status.dataToggleSyncEn = 0;
	ep1BD->status.usbOwned = 1;
}

void usbServiceCDCDataEP()
{
	if (usbPacket.epNum != 1)
		return;
	if (usbPacket.dir == USB_DIR_OUT)
		usbHandleDataEPOut();
	else
		usbHandleDataEPIn();
}

void usbUARTSendStringF(const char *str)
{
	uint16_t i = 0;
	while (str[i] != 0)
		++i;
	if (sendFIFOCount == 0)
	{
		volatile usbBDTEntry_t *ep1BD = &usbBDT[usbStatusInEP[1].ep.value];
		usbStatusInEP[1].buffSrc = USB_BUFFER_SRC_FLASH;
		usbStatusInEP[1].buffer.flashPtr = str;
		usbStatusInEP[1].xferCount = i;
		ep1BD->address = USB_EP1_IN_ADDR;
		usbServiceEPWriteArm(ep1BD, 1);
	}
	else
	{
		uint8_t idx = sendFIFOCount - 1;
		sendFIFO[idx].source = USB_BUFFER_SRC_FLASH;
		sendFIFO[idx].data.flash = str;
		sendFIFO[idx].len = i;
	}
	++sendFIFOCount;
}

void usbUARTSendStringM(char *str)
{
	uint16_t i = 0;
	while (str[i] != 0)
		++i;
	if (sendFIFOCount == 0)
	{
		volatile usbBDTEntry_t *ep1BD = &usbBDT[usbStatusInEP[1].ep.value];
		usbStatusInEP[1].buffSrc = USB_BUFFER_SRC_MEM;
		usbStatusInEP[1].buffer.memPtr = str;
		usbStatusInEP[1].xferCount = i;
		ep1BD->address = USB_EP1_IN_ADDR;
		usbServiceEPWriteArm(ep1BD, 1);
	}
	else
	{
		uint8_t idx = sendFIFOCount - 1;
		sendFIFO[idx].source = USB_BUFFER_SRC_MEM;
		sendFIFO[idx].data.mem = str;
		sendFIFO[idx].len = i;
	}
	++sendFIFOCount;
}

void usbUARTSendChar(const char c)
{
	if (sendFIFOCount == 0)
	{
		volatile usbBDTEntry_t *ep1BD = &usbBDT[usbStatusInEP[1].ep.value];
		usbStatusInEP[1].buffSrc = USB_BUFFER_SRC_MEM;
		usbStatusInEP[1].buffer.memPtr = &c;
		usbStatusInEP[1].xferCount = 1;
		ep1BD->address = USB_EP1_IN_ADDR;
		usbServiceEPWriteArm(ep1BD, 1);
	}
	else
	{
		uint8_t idx = sendFIFOCount - 1;
		sendFIFO[idx].source = USB_BUFFER_SRC_CHAR;
		sendFIFO[idx].data.ch = c;
		sendFIFO[idx].len = 1;
	}
	++sendFIFOCount;
}

bool usbUARTDataSent()
{
	if (sendFIFOCount != 0)
	{
		uint8_t i, fifoCount;
		usbStatusInEP[1].buffSrc = sendFIFO[0].source;
		if (sendFIFO[0].source == USB_BUFFER_SRC_MEM)
			usbStatusInEP[1].buffer.memPtr = sendFIFO[0].data.mem;
		else if (sendFIFO[0].source == USB_BUFFER_SRC_FLASH)
			usbStatusInEP[1].buffer.flashPtr = sendFIFO[0].data.flash;
		else
		{
			usbStatusInEP[1].buffSrc = USB_BUFFER_SRC_MEM;
			usbStatusInEP[1].buffer.memPtr = &sendChar;
			sendChar = sendFIFO[0].data.ch;
		}
		usbStatusInEP[1].xferCount = sendFIFO[0].len;

		fifoCount = sendFIFOCount - 1;
		for (i = 0; i < fifoCount; i++)
		{
			sendFIFO[i].source = sendFIFO[i + 1].source;
			if (sendFIFO[i].source == USB_BUFFER_SRC_MEM)
				sendFIFO[i].data.mem = sendFIFO[i + 1].data.mem;
			else if (sendFIFO[0].source == USB_BUFFER_SRC_FLASH)
				sendFIFO[i].data.flash = sendFIFO[i + 1].data.flash;
			else
				sendFIFO[i].data.ch = sendFIFO[i + 1].data.ch;
			sendFIFO[i].len = sendFIFO[i + 1].len;
		}
	}
	return sendFIFOCount == 0;
}

bool usbUARTHaveData()
{
	return dataFullness != readCounter;
}

char usbUARTRecvChar()
{
	if (readCounter >= dataFullness)
		return 0;
	return usbEP1Out[readCounter++];
}
