/*
 * This file is part of PIC18DeviceUSB
 * Copyright © 2015-2016 Rachel Mant (dx-mon@users.sourceforge.net)
 *
 * PIC18DeviceUSB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIC18DeviceUSB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "ADC.h" // TODO: Replace with proper VBus detection as this doesn't exist
#include "usb.h"
#include "usbTypes.h"
#include "usbRequests.h"
#include "usbCDC.h"

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/01/20
 */

#define NULL	((void *)0)

volatile usbDeviceState usbState;
volatile usbEP_t usbPacket;
volatile bool usbSuspended, usbAttachable;
volatile usbCtrlState_t usbCtrlState;
volatile uint8_t usbDeferalFlags;
volatile usbStallState_t usbStallState;

volatile uint8_t usbActiveConfig, usbStatusTimeout;
volatile bool usbStageLock1 = false, usbStageLock2 = false;

usbEPStatus_t usbStatusInEP[USB_ENDPOINTS];
usbEPStatus_t usbStatusOutEP[USB_ENDPOINTS];
/* Defines the buffer descriptor table and places it at it's fixed address in RAM */
volatile usbBDTEntry_t usbBDT[USB_BDT_ENTRIES] __at(USB_BDT_ADDR);
/* Define endpoint 0's buffers */
volatile usbSetupPacket_t usbEP0Setup __at(USB_EP0_SETUP_ADDR);
volatile uint8_t usbEP0Data[USB_EP0_DATA_LEN] __at(USB_EP0_DATA_ADDR);
/* And endpoint 2's */
volatile uint8_t usbEP2In[USB_EP2_IN_LEN] __at(USB_EP2_IN_ADDR);

void usbInit()
{
	UCON = 0x00;
	usbReset();
	usbCtrlState = USB_CTRL_STATE_WAIT;
	usbStallState = USB_STALL_STATE_STALL;
	usbDeferalFlags = 0;
	usbStatusTimeout = 0;

	/* Initialise the VBus detection pin */
	TRISA |= 0x02;
	ANSELA |= 0x02;
	usbAttachable = false;

	usbStatusInEP[0].epLen = USB_EP0_DATA_LEN;
}

void usbReset()
{
	uint8_t i;

	/* Reset all USB related interrupt enables */
	PIE3bits.USBIE = 0;
	UIE = 0;
	UEIE = 0;

	/* And flags */
	PIR3bits.USBIF = 0;
	UEIR = 0;
	UIR = 0;

	/*
	 * Configure EP0 as default control endpoint.
	 * By the defaults in the device, this leaves the others off,
	 * we just ensure that with the loop that follows.
	 */
	for (i = 0; i < USB_ENDPOINTS; i++)
		(&UEP0)[i] = 0;

	/*
	 * Configure for Full-Speed USB operation with
	 * all ping-pong buffers enabled for all endpoints
	 */
	UCFG = 0x16;

	/*
	 * Enable necessary interrupts
	 * UEIE: Listen to all error interrupts
	 * UIE: Listen to only:
	 *     USB Reset
	 *     Transaction Complete
	 *     Idle Detected
	 *     Stall Handshake
	 *     USB Error (enables UEIE)
	 *     Start-Of-Frame
	 */
	UEIE = 0x9F;
	UIE = 0x7F;

	for (i = 0; i < (USB_ENDPOINTS << 1); i++)
	{
		usbBDT[i].status.value = 0;
		usbBDT[i].count = 0;
		usbBDT[i].address = 0;
	}

	/* Reset the ping-pong buffers, bus address and transfer status */
	UCONbits.PPBRST = 1;
	UADDR = 0;
	UCONbits.PKTDIS = 0;
	UCONbits.PPBRST = 0;

	/* Clean out any pending crap */
	while (UIRbits.TRNIF == 1)
		UIRbits.TRNIF = 0;

	/* Reset status flags */
	usbStageLock1 = false;
	usbStageLock2 = false;
	usbSuspended = false;

	/* Reset driver endpoint states */
	for (i = 0; i < USB_ENDPOINTS; i++)
	{
		usbStatusInEP[i].value = 0;
		usbStatusInEP[i].xferCount = 0;
		usbStatusInEP[i].ep.value = 0;
		usbStatusInEP[i].ep.epNum = i;
		usbStatusInEP[i].ep.dir = USB_DIR_IN;
		usbStatusInEP[i].ep.buff = 0;
		usbStatusOutEP[i].value = 0;
		usbStatusOutEP[i].xferCount = 0;
		usbStatusOutEP[i].ep.value = 0;
		usbStatusOutEP[i].ep.epNum = i;
		usbStatusOutEP[i].ep.dir = USB_DIR_OUT;
		usbStatusOutEP[i].ep.buff = 0;
	}

	/* Prepare for an EP0 Setup packet */
	UEP0 = 0x16;
	usbBDT[0].count = USB_EP0_SETUP_LEN;
	usbBDT[0].address = USB_EP0_SETUP_ADDR;
	usbBDT[0].status.value = 0;
	usbBDT[0].status.dataToggleSync = 0;
	usbBDT[0].status.dataToggleSyncEn = 1;
	usbBDT[0].status.bufferStall = 1;
	usbBDT[0].status.usbOwned = 1;

	/* Finally, idle the peripheral */
	usbState = USB_STATE_DETACHED;
	usbActiveConfig = 0;
}

bool usbIsAttached()
{
	return usbState != USB_STATE_DETACHED;
}

bool usbCanAttach()
{
	if (adcGetChannel() == 1)
	{
		/*
		 * 1.24V should be seen on RA1 when VBus is present.
		 * We test for 1.0V to allow for it rising etc.
		 */
		if (adcRead() >= 410)
			usbAttachable = true;
		else
			usbAttachable = false;
	}
	return usbAttachable;
}

void usbAttach()
{
	/* It is an error to call this if we do not have VBus or are already attached */
	if (usbState != USB_STATE_DETACHED || !usbAttachable)
		return;

	/* Ensure we are currently detatched */
	UCON = 0x00;
	UIE = 0x00;

	/* Re-assert the configuration declared in usbInit() */
	UCFG = 0x16;
	UEIE = 0x9F;
	/* Except, ignore reset and idle conditions for the time being */
	UIE = 0x6E;

	/* And enable USB interrupts */
	RCONbits.IPEN = 1;
	IPR3bits.USBIP = 1;
	PIE3bits.USBIE = 1;

	/* Try to attach to bus */
	while (UCONbits.USBEN == 0)
		UCONbits.USBEN = 1;

	usbState = USB_STATE_ATTACHED;
}

void usbDetach()
{
	if (usbState == USB_STATE_DETACHED)
		return;

	UCON = 0x00;
	UIE = 0x00;
	usbState = USB_STATE_DETACHED;
}

void usbWakeup()
{
	usbSuspended = false;
	UCONbits.SUSPND = 0;
	UIEbits.ACTVIE = 0;
	while (UIRbits.ACTVIF == 1)
		UIRbits.ACTVIF = 0;
}

void usbSuspend()
{
	UIEbits.ACTVIE = 1;
	UIRbits.IDLEIF = 0;
	UCONbits.SUSPND = 1;
	usbSuspended = true;
}

void usbHandleStall()
{
	if (UEP0bits.EPSTALL == 1)
	{
		volatile usbBDTEntry_t *ep0 = &usbBDT[usbStatusInEP[0].ep.value];
		/* If EP0 in but not out is stalled */
		if (usbBDT[usbStatusOutEP[0].ep.value].status.usbOwned == 1 &&
			ep0->status.usbOwned == 1 && ep0->status.bufferStall == 1)
		{
			ep0 = &usbBDT[usbStatusOutEP[0].ep.value];
			/* Arm EP0 for a SETUP token */
			ep0->status.value = 0;
			ep0->status.dataToggleSync = 0;
			ep0->status.dataToggleSyncEn = 1;
			ep0->status.bufferStall = 1;
			ep0->status.usbOwned = 1;
		}
		/* Clear stall handshake condition */
		UEP0bits.EPSTALL = 0;
	}
	UIRbits.STALLIF = 0;
}

uint8_t usbServiceEPWrite(volatile usbBDTEntry_t *epBD, uint8_t ep)
{
	uint8_t ret, sendCount = usbStatusInEP[ep].epLen;
	volatile uint8_t *sendBuff;

	if (usbStatusInEP[ep].xferCount < usbStatusInEP[ep].epLen)
		sendCount = usbStatusInEP[ep].xferCount;
	/* Adjust the count of how much remains and prepare the transfer */
	usbStatusInEP[ep].xferCount -= sendCount;
	epBD->count = sendCount;
	ret = sendCount;
	sendBuff = (volatile uint8_t *)epBD->address;
	/* Copy the data to send this round from the user buffer */
	if (usbStatusInEP[ep].buffSrc == USB_BUFFER_SRC_MEM)
	{
		while (sendCount--)
			*sendBuff++ = *usbStatusInEP[ep].buffer.memBuff++;
	}
	else
	{
		if (usbStatusInEP[ep].multiPart)
		{
			while (usbStatusInEP[ep].partCount <= sendCount)
			{
				sendCount -= usbStatusInEP[ep].partCount;
				while (usbStatusInEP[ep].partCount--)
					*sendBuff++ = *usbStatusInEP[ep].buffer.flashBuff++;
				if (usbStatusInEP[ep].partDesc->numDesc != usbStatusInEP[ep].part)
				{
					const usbMultiPartDesc_t *nextPart = &usbStatusInEP[ep].partDesc->descriptors[usbStatusInEP[ep].part++];
					usbStatusInEP[ep].partCount = nextPart->length;
					usbStatusInEP[ep].buffer.flashPtr = nextPart->descriptor;
				}
			}
			if (usbStatusInEP[ep].xferCount == 0)
				usbStatusInEP[ep].multiPart = 0;
			usbStatusInEP[ep].partCount -= sendCount;
		}
		while (sendCount--)
			*sendBuff++ = *usbStatusInEP[ep].buffer.flashBuff++;
	}
	return ret;
}

void usbServiceCtrlEPWrite(volatile usbBDTEntry_t *ep0BD)
{
	if (usbStatusInEP[0].xferCount < USB_EP0_DATA_LEN)
	{
		if (usbStallState == USB_STALL_STATE_IDLE)
			usbStallState = USB_STALL_STATE_ARM;
		else
			usbStallState = USB_STALL_STATE_STALL;
	}
	usbServiceEPWrite(ep0BD, 0);
}

uint8_t usbServiceEPWriteArm(volatile usbBDTEntry_t *epBD, uint8_t ep)
{
	uint8_t ret;
	bool lastDTS;

	if (usbStatusInEP[ep].xferCount == 0)
		return 0;

	usbStatusInEP[ep].ep.buff ^= 1;
	lastDTS = usbBDT[usbStatusInEP[ep].ep.value].status.dataToggleSync;
	usbStatusInEP[ep].ep.buff ^= 1;
	ret = usbServiceEPWrite(epBD, ep);

	epBD->status.value = 0;
	if (lastDTS)
		epBD->status.dataToggleSync = 0;
	else
		epBD->status.dataToggleSync = 1;
	epBD->status.dataToggleSyncEn = 1;
	epBD->status.usbOwned = 1;

	return ret;
}

uint8_t usbServiceEPRead(volatile usbBDTEntry_t *epBD, uint8_t ep)
{
	uint8_t ret, readCount = epBD->count;
	volatile uint8_t *recvBuff = addrToPtr(epBD->address);
	/* Bounds sanity and then adjust how much is left to transfer */
	if (readCount > usbStatusOutEP[ep].xferCount)
		readCount = usbStatusOutEP[ep].xferCount;
	usbStatusOutEP[ep].xferCount -= readCount;
	ret = readCount;
	/* Copy the received data to the user buffer */
	while (readCount--)
		*usbStatusOutEP[ep].buffer.memBuff++ = *recvBuff++;
	return ret;
}

void usbServiceCtrlEPRead(volatile usbBDTEntry_t *ep0BD)
{
	usbServiceEPRead(ep0BD, 0);
	/* If there is still more to come, re-arm endpoint */
	if (usbStatusOutEP[0].xferCount > 0)
	{
		bool lastDTS = ep0BD->status.dataToggleSync;
		ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
		ep0BD->count = USB_EP0_DATA_LEN;
		ep0BD->address = USB_EP0_DATA_ADDR;
		ep0BD->status.value = 0;
		if (lastDTS)
			ep0BD->status.dataToggleSync = 0;
		else
			ep0BD->status.dataToggleSync = 1;
		ep0BD->status.dataToggleSyncEn = 1;
		ep0BD->status.usbOwned = 1;
	}
	else
	{
		/* Re-arm the endpoint for the next SETUP token */
		ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
		ep0BD->count = USB_EP0_DATA_LEN;
		ep0BD->address = USB_EP0_DATA_ADDR;
		ep0BD->status.value = 0;
		ep0BD->status.dataToggleSync = 0;
		ep0BD->status.dataToggleSyncEn = 1;
		ep0BD->status.bufferStall = 1;
		ep0BD->status.usbOwned = 1;

		/* Check for an execute function */
		if (usbStatusOutEP[0].func != NULL)
		{
			usbStatusOutEP[0].func();
			usbStatusOutEP[0].func = NULL;
		}
		usbStatusOutEP[0].needsArming = 0;

		if ((usbDeferalFlags & USB_DEFER_STATUS_PACKETS) == 0)
			usbHandleStatusCtrlEP();
	}
}

void usbHandleDataCtrlEP()
{
	volatile usbBDTEntry_t *ep0BD;
	usbDeferalFlags &= ~(USB_DEFER_IN_PACKETS | USB_DEFER_OUT_PACKETS);

	if (usbCtrlState == USB_CTRL_STATE_RX)
	{
		/* Setup to receive the first packet in the RX sequence */
		ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
		ep0BD->count = USB_EP0_DATA_LEN;
		ep0BD->address = USB_EP0_DATA_ADDR;
		ep0BD->status.value = 0;
		ep0BD->status.dataToggleSync = 1;
		ep0BD->status.dataToggleSyncEn = 1;
		ep0BD->status.usbOwned = 1;
	}
	else if (usbCtrlState == USB_CTRL_STATE_TX)
	{
		volatile usbSetupPacket_t *packet = addrToPtr(usbBDT[usbStatusOutEP[0].ep.value].address);
		/* Setup the data area */
		ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
		ep0BD->address = USB_EP0_DATA_ADDR;
		if (usbStatusInEP[0].xferCount > packet->length)
			usbStatusInEP[0].xferCount = packet->length;
		usbServiceCtrlEPWrite(ep0BD);

		/* Setup to transmit the first packet in the TX sequence */
		ep0BD->status.value = 0;
		ep0BD->status.dataToggleSync = 1;
		ep0BD->status.dataToggleSyncEn = 1;
		ep0BD->status.usbOwned = 1;
	}
}

void usbHandleStatusCtrlEP()
{
	if (usbStageLock1 == false)
	{
		usbStageLock1 = true;
		if (usbStageLock2 == false)
		{
			volatile usbBDTEntry_t *ep0BD;
			usbStageLock2 = true;
			if (usbCtrlState == USB_CTRL_STATE_RX)
			{
				/* Set up the 0 length IN transfer that terminates this RX sequence */
				ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
				ep0BD->count = 0;
				ep0BD->status.value = 0;
				ep0BD->status.dataToggleSync = 1;
				ep0BD->status.dataToggleSyncEn = 1;
				ep0BD->status.usbOwned = 1;
			}
			else if (usbCtrlState == USB_CTRL_STATE_TX)
			{
				/* Set up the 0 length OUT transfer that terminates this TX sequence */
				ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
				ep0BD->count = USB_EP0_SETUP_LEN;
				ep0BD->address = USB_EP0_SETUP_ADDR;
				ep0BD->status.value = 0;
				ep0BD->status.dataToggleSync = 1;
				ep0BD->status.dataToggleSyncEn = 1;
				ep0BD->status.usbOwned = 1;
				usbStatusOutEP[0].ep.buff ^= 1;

				/* Get ready to receive next SETUP token */
				ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
				ep0BD->count = USB_EP0_SETUP_LEN;
				ep0BD->address = USB_EP0_SETUP_ADDR;
				ep0BD->status.value = 0;
				ep0BD->status.dataToggleSync = 0;
				ep0BD->status.dataToggleSyncEn = 1;
				ep0BD->status.bufferStall = 1;
				ep0BD->status.usbOwned = 1;
				usbStatusOutEP[0].ep.buff ^= 1;
			}
		}
	}
}

void usbServiceCtrlEPComplete()
{
	volatile usbBDTEntry_t *ep0BD;
	/* Values in []'s indicate DTS bits values.

	/* Re-enable packet processing after a setup transaction */
	UCONbits.PKTDIS = 0;
	/* Grab the next slot (ping-pong buffers!) */
	usbPacket.buff ^= 1;
	ep0BD = &usbBDT[usbPacket.value];

	if (usbStatusInEP[0].needsArming == 0)
	{
		if (usbStatusOutEP[0].needsArming == 1)
		{
			/* <SETUP[0]><OUT[1]><OUT[0]>...<IN[1]> */
			usbCtrlState = USB_CTRL_STATE_RX;
			if ((usbDeferalFlags & USB_DEFER_OUT_PACKETS) == 0)
				usbHandleDataCtrlEP();
			usbStageLock1 = false;
			usbStageLock2 = false;
		}
		else
		{
			/* If nothing handled the request, get ready for the next SETUP token */
			ep0BD->count = USB_EP0_SETUP_LEN;
			ep0BD->address = USB_EP0_SETUP_ADDR;
			ep0BD->status.value = 0;
			ep0BD->status.dataToggleSync = 0;
			ep0BD->status.dataToggleSyncEn = 1;
			ep0BD->status.bufferStall = 1;
			ep0BD->status.usbOwned = 1;

			/* And stall the IN endpoint */
			ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
			ep0BD->status.value = 0;
			ep0BD->status.bufferStall = 1;
			ep0BD->status.usbOwned = 1;
		}
	}
	else
	{
		volatile usbSetupPacket_t *packet = addrToPtr(USB_EP0_SETUP_ADDR);
		if (packet->requestType.direction == USB_DIR_IN)
		{
			/* <SETUP[0]><IN[1]><IN[0]>...<OUT[1]> */
			usbCtrlState = USB_CTRL_STATE_TX;
			if ((usbDeferalFlags & USB_DEFER_IN_PACKETS) == 0)
				usbHandleDataCtrlEP();
			usbStageLock1 = false;
			usbStageLock2 = false;
			if ((usbDeferalFlags & USB_DEFER_STATUS_PACKETS) == 0)
				usbHandleStatusCtrlEP();
		}
		else
		{
			/* <SETUP[0] (OUT)><IN[1]> */
			usbCtrlState = USB_CTRL_STATE_RX;

			/* Get ready for the next SETUP token */
			ep0BD->count = USB_EP0_SETUP_LEN;
			ep0BD->address = USB_EP0_SETUP_ADDR;
			ep0BD->status.value = 0;
			ep0BD->status.dataToggleSync = 0;
			ep0BD->status.dataToggleSyncEn = 1;
			ep0BD->status.bufferStall = 1;
			ep0BD->status.usbOwned = 1;

			usbStageLock1 = false;
			usbStageLock2 = false;
			if ((usbDeferalFlags & USB_DEFER_STATUS_PACKETS) == 0)
				usbHandleStatusCtrlEP();
		}
	}
}

void usbHandleCtrlEPSetup()
{
	bool processed;
	volatile usbBDTEntry_t *ep0BD;

	/* Reset in buffers for EP0 (de-arm them) to get to a clean state for this call */
	ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
	ep0BD->status.usbOwned = 0;
	usbStatusInEP[0].ep.buff ^= 1;
	ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
	ep0BD->status.usbOwned = 0;
	usbStatusInEP[0].ep.buff ^= 1;

	ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
	ep0BD->status.usbOwned = 0;

	/* Set flags up*/
	usbStallState = USB_STALL_STATE_IDLE;
	usbDeferalFlags = 0;
	usbCtrlState = USB_CTRL_STATE_WAIT;
	usbStatusInEP[0].value = 0;
	usbStatusInEP[0].xferCount = 0;
	usbStatusOutEP[0].value = 0;
	usbStatusOutEP[0].xferCount = 0;

	/* Handle the request */
	processed = usbHandleStandardRequest(addrToPtr(usbBDT[usbPacket.value].address));
	if (!processed)
		usbHandleCDCRequest(&usbBDT[usbPacket.value]);

	usbServiceCtrlEPComplete();
}

void usbHandleCtrlEPOut()
{
	if (usbCtrlState == USB_CTRL_STATE_RX)
		usbServiceCtrlEPRead(&usbBDT[usbPacket.value]);
	else
	{
		volatile usbBDTEntry_t *ep0BD;
		usbCtrlState = USB_CTRL_STATE_WAIT;

		ep0BD = &usbBDT[usbStatusOutEP[0].ep.value];
		/* Get ready for the next SETUP token if we aren't already */
		if (ep0BD->status.usbOwned == 0)
		{
			ep0BD->count = USB_EP0_SETUP_LEN;
			ep0BD->address = USB_EP0_SETUP_ADDR;
			ep0BD->status.value = 0;
			ep0BD->status.dataToggleSync = 0;
			ep0BD->status.dataToggleSyncEn = 1;
			ep0BD->status.bufferStall = 1;
			ep0BD->status.usbOwned = 1;
		}
	}
}

void usbHandleCtrlEPIn()
{
	volatile usbBDTEntry_t *ep0BD = &usbBDT[usbStatusInEP[0].ep.value];
	bool lastDTS = usbBDT[usbPacket.value].status.dataToggleSync;

	if (usbState == USB_STATE_ADDRESSING)
	{
		/* Get the setup packet data area and check that the reason we're here is a set address request */
		volatile usbSetupPacket_t *packet = addrToPtr(usbBDT[usbStatusOutEP[0].ep.value].address);
		if (packet->requestType.type != USB_REQUEST_TYPE_STANDARD ||
			packet->request != USB_REQUEST_SET_ADDRESS ||
			packet->value.address.addrH != 0)
			UADDR = 0;
		else
			UADDR = packet->value.address.addrL;

		/* If the address payload was not 100% correct, enter the waiting state again */
		if (UADDR == 0)
			usbState = USB_STATE_WAITING;
		else
			usbState = USB_STATE_ADDRESSED;
	}

	if (usbCtrlState == USB_CTRL_STATE_TX)
	{
		ep0BD->address = USB_EP0_DATA_ADDR;
		usbServiceCtrlEPWrite(ep0BD);
		ep0BD->status.value = 0;
		if (usbStallState == USB_STALL_STATE_STALL)
		{
			ep0BD->status.bufferStall = 1;
			ep0BD->status.usbOwned = 1;
		}
		else
		{
			if (lastDTS)
				ep0BD->status.dataToggleSync = 0;
			else
				ep0BD->status.dataToggleSync = 1;
			ep0BD->status.dataToggleSyncEn = 1;
			ep0BD->status.usbOwned = 1;
		}
	}
	else
	{
		if (usbStatusOutEP[0].needsArming == 1)
		{
			/* Check for an execute function */
			if (usbStatusOutEP[0].func != NULL)
			{
				usbStatusOutEP[0].func();
				usbStatusOutEP[0].func = NULL;
			}
			usbStatusOutEP[0].needsArming = 0;
		}
		usbCtrlState = USB_CTRL_STATE_WAIT;
	}
}

void usbServiceCtrlEP()
{
	if (usbPacket.epNum != 0)
		return;
	usbStatusTimeout = USB_STATUS_TIMEOUT;
	if (usbPacket.dir == USB_DIR_OUT)
	{
		volatile usbBDTEntry_t *ep0BD = &usbBDT[usbPacket.value];

		if (ep0BD->status.pid == USB_PID_SETUP)
			usbHandleCtrlEPSetup();
		else
			usbHandleCtrlEPOut();
	}
	else
		usbHandleCtrlEPIn();
}

void usbIRQ()
{
	/*
	 * When Single-Ended 0 condition clears and we are in the freshly attached state,
	 * switch state processing to the "powered" state where we are unconfigured, but
	 * ready and awaiting address assignment and enumeration.
	 * SE0 is held on the bus during the process of the D+ and D-
	 * pair being bought up to operating voltage.
	 */
	if (usbState == USB_STATE_ATTACHED && UCONbits.SE0 == 0)
	{
		/* Clear interrupt condition */
		UIR &= 0x01;
		/* Enable Idle and Reset interrupts now it makes sense to have them */
		UIE |= 0x11;
		usbState = USB_STATE_POWERED;
	}

	/* If we detect activity, ensure we are in an awake state */
	if (UIRbits.ACTVIF == 1 && UIEbits.ACTVIE == 1)
	{
		UIRbits.ACTVIF = 0;
		usbWakeup();
	}

	/* If we are in a suspended state due to inactivity, ignore all further USB interrupt processors */
	if (usbSuspended)
		return;

	/* If we detect the USB reset condition then ready processing getting an address, etc */
	if (UIRbits.URSTIF == 1 && UIEbits.URSTIE == 1)
	{
		usbReset();
		PIE3bits.USBIE = 1;
		usbState = USB_STATE_WAITING;
		UIRbits.URSTIF = 0;
	}

	/* If we detect an idle condition, suspend activity */
	if (UIRbits.IDLEIF == 1 && UIEbits.IDLEIE == 1)
	{
		usbSuspend();
		UIRbits.IDLEIF = 0;
	}

	/* If we detect a start of frame, check the status stage timeout and dispatch as necessary */
	if (UIRbits.SOFIF == 1)
	{
		//if (UIEbits.SOFIE == 1);
		UIRbits.SOFIF = 0;

		if (usbStatusTimeout != 0)
			--usbStatusTimeout;
		else
			usbHandleStatusCtrlEP();
	}

	/* If we detect a Stall handshake condition, process it */
	if (UIRbits.STALLIF == 1 && UIEbits.STALLIE == 1)
		usbHandleStall();

	/* If we detect an USB error, process it */
	if (UIRbits.UERRIF == 1 && UIEbits.UERRIE == 1)
	{
		/* Clear the error condition */
		UEIR = 0;
		UIRbits.UERRIF = 0;
	}

	/* If we are not yet configured, process no further. */
	if (usbState < USB_STATE_WAITING)
		return;

	/* If we detect the SIE has completed IO for a transaction, handle that transaction */
	if (UIEbits.TRNIE == 1)
	{
		while (UIRbits.TRNIF == 1)
		{
			uint8_t endpointNum;

			/* Do something about the transaction data */
			usbPacket.value = (USTAT & 0x7E) >> 1;
			endpointNum = usbPacket.epNum;
			/* Mark the entry as processed */
			UIRbits.TRNIF = 0;

			/* Process the data */
			if (usbPacket.dir == USB_DIR_OUT)
				usbStatusOutEP[endpointNum].ep.buff ^= 1;
			else
				usbStatusInEP[endpointNum].ep.buff ^= 1;

			if (endpointNum == 0)
				usbServiceCtrlEP();
			else if (endpointNum == 1)
				usbServiceCDCDataEP();
		}
	}
}
