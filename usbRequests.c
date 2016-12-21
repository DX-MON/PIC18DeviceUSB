#include <xc.h>
#include <stdint.h>
#include "usb.h"
#include "usbTypes.h"
#include "usbRequests.h"
#include "usbCDC.h"

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/01/25
 */

/* This VID is specifically Atmel's */
#define USB_VID 0x03EB
#define USB_PID 0x2122

#define USB_NUM_CONFIG_DESC		1
#define USB_NUM_IFACE_DESC		2
#define USB_NUM_ENDPOINT_DESC	3
#define USB_NUM_CONFIG_SECS		11
#define USB_NUM_STRING_DESC		4

#define USB_EPDIR_IN			0x80
#define USB_EPDIR_OUT			0x00

const usbDeviceDescriptor_t usbDeviceDesc =
{
	sizeof(usbDeviceDescriptor_t),
	USB_DESCRIPTOR_DEVICE,
	0x0200, /* this is 2.00 in USB's BCD format */
	USB_CLASS_COMMS,
	USB_SUBCLASS_NONE,
	USB_PROTOCOL_NONE,
	USB_EP0_SETUP_LEN,
	USB_VID,
	USB_PID,
	0x0001, /* BCD encoded device version */
	0x01, /* Manufacturer string index */
	0x02, /* Product string index */
	0x00, /* Temporarily do not support a serial number string */
	USB_NUM_CONFIG_DESC /* One configuration only */
};

const usbConfigDescriptor_t usbConfigDesc[USB_NUM_CONFIG_DESC] =
{
	{
		sizeof(usbConfigDescriptor_t),
		USB_DESCRIPTOR_CONFIGURATION,
		sizeof(usbConfigDescriptor_t) + sizeof(usbInterfaceAssocDescriptor_t) +
		sizeof(usbInterfaceDescriptor_t) + sizeof(usbCDCHeader_t) +
		sizeof(usbCDCHeaderACM_t) + sizeof(usbCDCUnion2_t) + sizeof(usbCDCCallMgmt_t) +
		sizeof(usbEndpointDescriptor_t) + sizeof(usbInterfaceDescriptor_t) +
		sizeof(usbEndpointDescriptor_t) + sizeof(usbEndpointDescriptor_t),
		0x02, /* One interface */
		0x01, /* This is the first configuration */
		0x03, /* Configuration string index */
		USB_CONF_ATTR_DEFAULT | USB_CONF_ATTR_SELFPWR,
		50 /* 100mA max */
	}
};

const usbInterfaceDescriptor_t usbInterfaceDesc[USB_NUM_IFACE_DESC] =
{
	{
		sizeof(usbInterfaceDescriptor_t),
		USB_DESCRIPTOR_INTERFACE,
		0x00, /* 0th interface, by which we mean 1st */
		0x00, /* Alternate 0 */
		0x01, /* Two endpoints to the interface */
		USB_CLASS_COMMS,
		USB_SUBCLASS_ACM,
		USB_PROTOCOL_NONE,
		0x00 /* No string to describe this interface */
	},
	{
		sizeof(usbInterfaceDescriptor_t),
		USB_DESCRIPTOR_INTERFACE,
		0x01, /* 1st interface, by which we mean 2nd */
		0x00, /* Alternate 0 */
		0x02, /* One endpoint to the interface */
		USB_CLASS_DATA,
		USB_SUBCLASS_NONE,
		USB_PROTOCOL_NONE,
		0x00 /* No string to describe this interface */
	}
};

const usbEndpointDescriptor_t usbEndpointDesc[USB_NUM_ENDPOINT_DESC] =
{
	{
		sizeof(usbEndpointDescriptor_t),
		USB_DESCRIPTOR_ENDPOINT,
		USB_EPDIR_IN | 2,
		USB_EPTYPE_INTR,
		USB_EP2_IN_LEN,
		0x01 /* Poll once per frame */
	},
	{
		sizeof(usbEndpointDescriptor_t),
		USB_DESCRIPTOR_ENDPOINT,
		USB_EPDIR_IN | 1,
		USB_EPTYPE_BULK,
		USB_EP1_IN_LEN,
		0x01 /* Poll once per frame */
	},
	{
		sizeof(usbEndpointDescriptor_t),
		USB_DESCRIPTOR_ENDPOINT,
		USB_EPDIR_OUT | 1,
		USB_EPTYPE_BULK,
		USB_EP1_OUT_LEN,
		0x01 /* Poll once per frame */
	}
};

const usbInterfaceAssocDescriptor_t usbInterfaceAssocDesc =
{
	sizeof(usbInterfaceAssocDescriptor_t),
	USB_DESCRIPTOR_INTERFACE_ASSOCIATION,
	0, /* First interface is #1 */
	2, /* Two interfaces involved contiguously */
	USB_CLASS_COMMS,
	USB_SUBCLASS_ACM,
	USB_PROTOCOL_NONE,
	0x03 /* Configuration string index */
};

const usbCDCHeader_t usbCDCHeader =
{
	sizeof(usbCDCHeader_t),
	USB_DESCRIPTOR_CDC,
	USB_CDC_HEADER,
	0x0110
};

const usbCDCHeaderACM_t usbCDCHeaderACM =
{
	sizeof(usbCDCHeaderACM_t),
	USB_DESCRIPTOR_CDC,
	USB_CDC_ACM,
	USB_ACM_LINE_CODING /* Set break here does not make any sense */
};

const usbCDCUnion2_t usbCDCUnion =
{
	sizeof(usbCDCUnion2_t),
	USB_DESCRIPTOR_CDC,
	USB_CDC_UNION,
	0,
	1
};

const usbCDCCallMgmt_t usbCDCCallMgmt =
{
	sizeof(usbCDCCallMgmt_t),
	USB_DESCRIPTOR_CDC,
	USB_CDC_CM,
	USB_CDC_CM_SELF_MANAGE,
	2
};

const usbMultiPartDesc_t usbConfigSecs[USB_NUM_CONFIG_SECS] =
{
	{
		sizeof(usbConfigDescriptor_t),
		&usbConfigDesc[0]
	},
	{
		sizeof(usbInterfaceAssocDescriptor_t),
		&usbInterfaceAssocDesc
	},
	{
		sizeof(usbInterfaceDescriptor_t),
		&usbInterfaceDesc[0]
	},
	{
		sizeof(usbCDCHeader_t),
		&usbCDCHeader
	},
	{
		sizeof(usbCDCHeaderACM_t),
		&usbCDCHeaderACM
	},
	{
		sizeof(usbCDCUnion2_t),
		&usbCDCUnion
	},
	{
		sizeof(usbCDCCallMgmt_t),
		&usbCDCCallMgmt
	},
	{
		sizeof(usbEndpointDescriptor_t),
		&usbEndpointDesc[0]
	},
	{
		sizeof(usbInterfaceDescriptor_t),
		&usbInterfaceDesc[1]
	},
	{
		sizeof(usbEndpointDescriptor_t),
		&usbEndpointDesc[1]
	},
	{
		sizeof(usbEndpointDescriptor_t),
		&usbEndpointDesc[2]
	}
};

const usbMultiPartTable_t usbConfigDescs[USB_NUM_CONFIG_DESC] =
{
	{
		USB_NUM_CONFIG_SECS,
		usbConfigSecs
	}
};

const struct
{
	usbStringDescBase_t header;
	uint16_t ids[1];
} usbStringLangIDs =
{
	{
		sizeof(usbStringLangIDs),
		USB_DESCRIPTOR_STRING
	},
	{ 0x0409 } /* This encodes the ID code for US English so we work nicely with windows */
};

const struct
{
	usbStringDescBase_t header;
	uint16_t strMfr[15];
} usbStringMfr =
{
	{
		sizeof(usbStringMfr),
		USB_DESCRIPTOR_STRING
	},
	{
		/* Manufacturer string is "Ole Buhl Racing" */
		'O', 'l', 'e', ' ', 'B', 'u', 'h', 'l', ' ', 'R', 'a', 'c', 'i', 'n', 'g'
	}
};

const struct
{
	usbStringDescBase_t header;
	uint16_t strProduct[22];
} usbStringProduct =
{
	{
		sizeof(usbStringProduct),
		USB_DESCRIPTOR_STRING
	},
	{
		/* Product string is "Ride Height controller" */
		'R', 'i', 'd', 'e', ' ', 'H', 'e', 'i', 'g', 'h', 't', ' ',
		'c', 'o', 'n', 't', 'r', 'o', 'l', 'l', 'e', 'r'
	}
};

const struct
{
	usbStringDescBase_t header;
	uint16_t strVCP[16];
} usbStringVCP =
{
	{
		sizeof(usbStringVCP),
		USB_DESCRIPTOR_STRING
	},
	{
		/* Primary interface string is "Virtual COM Port" */
		'V', 'i', 'r', 't', 'u', 'a', 'l', ' ', 'C', 'O', 'M', ' ',
		'P', 'o', 'r', 't'
	}
};

const usbStringDescBase_t *usbStrings[USB_NUM_STRING_DESC] =
{
	&usbStringLangIDs.header,
	&usbStringMfr.header,
	&usbStringProduct.header,
	&usbStringVCP.header
};

void usbRequestGetDescriptor()
{
	volatile usbSetupPacket_t *packet = addrToPtr(USB_EP0_SETUP_ADDR);

	if (packet->requestType.direction == USB_DIR_IN)
	{
		usbStatusInEP[0].buffSrc = USB_BUFFER_SRC_FLASH;
		usbStatusInEP[0].needsArming = 1;

		switch (packet->value.descriptor.type)
		{
			case USB_DESCRIPTOR_DEVICE:
				usbStatusInEP[0].buffer.flashPtr = &usbDeviceDesc;
				usbStatusInEP[0].xferCount = usbDeviceDesc.length;
				break;
			case USB_DESCRIPTOR_CONFIGURATION:
				if (packet->value.descriptor.index < USB_NUM_CONFIG_DESC)
				{
					uint8_t i;
					const usbMultiPartTable_t *configDesc = &usbConfigDescs[packet->value.descriptor.index];
					usbStatusInEP[0].buffer.flashPtr = configDesc->descriptors[0].descriptor;
					usbStatusInEP[0].xferCount = 0;
					for (i = 0; i < configDesc->numDesc; i++)
						usbStatusInEP[0].xferCount += configDesc->descriptors[i].length;
					usbStatusInEP[0].multiPart = 1;
					usbStatusInEP[0].part = 1;
					usbStatusInEP[0].partDesc = configDesc;
					usbStatusInEP[0].partCount = configDesc->descriptors[0].length;
				}
				else
					usbStatusInEP[0].value = 0;
				break;
			case USB_DESCRIPTOR_INTERFACE:
				if (packet->value.descriptor.index < USB_NUM_IFACE_DESC)
				{
					const usbInterfaceDescriptor_t *ifaceDesc = &usbInterfaceDesc[packet->value.descriptor.index];
					usbStatusInEP[0].buffer.flashPtr = ifaceDesc;
					usbStatusInEP[0].xferCount = ifaceDesc->length;
				}
				else
					usbStatusInEP[0].value = 0;
				break;
			case USB_DESCRIPTOR_ENDPOINT:
				if (packet->value.descriptor.index < USB_NUM_ENDPOINT_DESC)
				{
					const usbEndpointDescriptor_t *epDesc = &usbEndpointDesc[packet->value.descriptor.index];
					usbStatusInEP[0].buffer.flashPtr = epDesc;
					usbStatusInEP[0].xferCount = epDesc->length;
				}
				else
					usbStatusInEP[0].value = 0;
				break;
			case USB_DESCRIPTOR_STRING:
				if (packet->value.descriptor.index < USB_NUM_STRING_DESC)
				{
					const usbStringDescBase_t *strDesc = usbStrings[packet->value.descriptor.index];
					usbStatusInEP[0].buffer.flashPtr = strDesc;
					usbStatusInEP[0].xferCount = strDesc->length;
				}
				else
					usbStatusInEP[0].value = 0;
				break;
			default:
				usbStatusInEP[0].value = 0;
		}
	}
}

void usbRequestSetConfiguration()
{
	uint8_t i;
	volatile usbSetupPacket_t *packet = addrToPtr(USB_EP0_SETUP_ADDR);

	/* Generate a 0 length ack for this */
	usbStatusInEP[0].needsArming = 1;
	/* And reset all non-EP0 endpoints */
	for (i = 1; i < USB_ENDPOINTS; i++)
		(&UEP0)[i] = 0;
	/* Reset all BDT entiries to pristine state */
	for (i = 0; i < USB_BDT_ENTRIES; i++)
	{
		usbBDT[i].status.value = 0;
		usbBDT[i].count = 0;
		usbBDT[i].address = 0;
	}

	/* Reset the ping-pong buffers, and their states */
	UCONbits.PPBRST = 1;
	for (i = 0; i < USB_ENDPOINTS; i++)
	{
		usbStatusInEP[i].ep.buff = 0;
		usbStatusOutEP[i].ep.buff = 0;
	}
	UCONbits.PPBRST = 0;

	/* Reset alternate interface setting and set active config */
	usbActiveConfig = packet->value.config.value;

	if (usbActiveConfig == 0)
		usbState = USB_STATE_ADDRESSED;
	else if (usbActiveConfig <= USB_NUM_CONFIG_DESC)
	{
		uint8_t configIdx = usbActiveConfig - 1;
		uint8_t j, ifaceIdx = 0, endpointIdx = 0;

		usbState = USB_STATE_CONFIGURED;
		/* Count interfaces to arrive at the first to deref */
		for (i = 0; i < configIdx; i++)
		{
			j = ifaceIdx;
			ifaceIdx += usbConfigDesc[i].numInterfaces;
			for (; j < ifaceIdx; j++)
				endpointIdx += usbInterfaceDesc[j].numEndpoints;
		}

		for (i = 0; i < usbConfigDesc[configIdx].numInterfaces; i++)
		{
			for (j = 0; j < usbInterfaceDesc[i].numEndpoints; j++)
			{
				const usbEndpointDescriptor_t *endpoint = &usbEndpointDesc[endpointIdx + j];
				volatile uint8_t *ep = &UEP0 + (endpoint->endpointAddress & 0x7F);
				uint8_t epType = endpoint->attributes & 0x03;

				/* Disable control transfers on the endpoint */
				*ep |= 0x08;
				/* And enable the direction requested*/
				if ((endpoint->endpointAddress & 0x80) == USB_EPDIR_IN)
					*ep |= 0x02;
				else
					*ep |= 0x04;

				/* Bulk and interrupt endpoints don't want ISO enabled */
				if (epType == USB_EPTYPE_BULK || epType == USB_EPTYPE_INTR)
					*ep |= 0x10;
			}
			endpointIdx += j;
		}

		usbCDCInit();
	}
}

void usbRequestGetStatus()
{
	volatile usbSetupPacket_t *packet = addrToPtr(USB_EP0_SETUP_ADDR);
	volatile uint8_t *dataBuff = addrToPtr(USB_EP0_DATA_ADDR);
	dataBuff[0] = dataBuff[1] = 0;

	switch (packet->requestType.recipient)
	{
		case USB_RECIPIENT_DEVICE:
			usbStatusInEP[0].needsArming = 1;
			/* We are never remotely woken up, but we are self-powered. */
			dataBuff[0] |= 0x01;
			break;
		case USB_RECIPIENT_INTERFACE:
			usbStatusInEP[0].needsArming = 1;
			/* The spec mandates this has to return 0 for both bytes */
			break;
		case USB_RECIPIENT_ENDPOINT:
		{
			usbEP_t endpoint;
			volatile usbBDTEntry_t *epBD;
			usbStatusInEP[0].needsArming = 1;
			/* Work out which endpoint and which buffer of which endpoint we're checking */
			endpoint.value = 0;
			if (packet->index.epDir == USB_DIR_OUT)
				endpoint.buff = usbStatusOutEP[endpoint.epNum].ep.buff;
			else
				endpoint.buff = usbStatusInEP[endpoint.epNum].ep.buff;
			endpoint.dir = packet->index.epDir;
			endpoint.epNum = packet->index.epNum;
			/* Look the endpoint up */
			epBD = &usbBDT[endpoint.value];
			/* Check if the endpoint is halted */
			if (epBD->status.usbOwned == 1 && epBD->status.bufferStall == 1)
				dataBuff[0] = 0x01;
			break;
		}
	}

	/* If something generated something to report, set up the endpoint state for it */
	if (usbStatusInEP[0].needsArming)
	{
		usbStatusInEP[0].buffer.memPtr = dataBuff;
		usbStatusInEP[0].buffSrc = USB_BUFFER_SRC_MEM;
		usbStatusInEP[0].xferCount = 2;
	}
}

void usbRequestDoFeature()
{
	volatile usbSetupPacket_t *packet = addrToPtr(USB_EP0_SETUP_ADDR);
	/* Process any eventual Remote Wakeup requests to apease Windows. */
	if (packet->value.feature.value == USB_FEATURE_REMOTE_WAKEUP &&
		packet->requestType.recipient == USB_RECIPIENT_DEVICE)
	{
		usbStatusInEP[0].needsArming = 1;
		return;
	}
}

bool usbHandleStandardRequest(volatile usbSetupPacket_t *packet)
{
	if (packet->requestType.type != USB_REQUEST_TYPE_STANDARD)
		return false;

	switch (packet->request)
	{
		case USB_REQUEST_SET_ADDRESS:
			/* Generate a reply that is 0 bytes long to acknowledge */
			usbStatusInEP[0].needsArming = 1;
			usbState = USB_STATE_ADDRESSING;
			return true;
		case USB_REQUEST_GET_DESCRIPTOR:
			/* Transmits the requested descriptor of the requested type to the host */
			usbRequestGetDescriptor();
			return true;
		case USB_REQUEST_SET_CONFIGURATION:
			/* Reconfigures the device endpoints for the indicated configuration index */
			usbRequestSetConfiguration();
			usbPacket.buff = 1;
			return true;
		case USB_REQUEST_GET_CONFIGURATION:
			/* Returns the index of the active configuration */
			usbStatusInEP[0].buffSrc = USB_BUFFER_SRC_MEM;
			usbStatusInEP[0].buffer.memPtr = &usbActiveConfig;
			usbStatusInEP[0].xferCount = 1;
			usbStatusInEP[0].needsArming = 1;
			return true;
		case USB_REQUEST_GET_STATUS:
			/* Returns the device status of the requested sub-entity */
			usbRequestGetStatus();
			return true;
		case USB_REQUEST_CLEAR_FEATURE:
		case USB_REQUEST_SET_FEATURE:
			/* Manipulates a device feature */
			usbRequestDoFeature();
			return true;
		case USB_REQUEST_GET_INTERFACE:
			/* Set source and types */
			usbStatusInEP[0].buffSrc = USB_BUFFER_SRC_MEM;
			usbStatusInEP[0].xferCount = 1;
			usbStatusInEP[0].needsArming = 1;
			return true;
		case USB_REQUEST_SET_INTERFACE:
			/* Generate a reply that is 0 bytes long to acknowledge */
			usbStatusInEP[0].needsArming = 1;
			/* AltID */
			return true;
		case USB_REQUEST_SET_DESCRIPTOR:
			/* Set descriptor handler */
			return true;
		case USB_REQUEST_SYNC_FRAME:
			return true;
	}
	return false;
}
