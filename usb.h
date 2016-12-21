#ifndef USB_H
#define	USB_H

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/01/20
 */

#ifdef	__cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "USBTypes.h"

extern void usbInit();
extern void usbReset();
extern bool usbIsAttached();
extern bool usbCanAttach();
extern void usbAttach();
extern void usbDetach();
extern void usbIRQ();

extern void usbHandleDataCtrlEP();
extern void usbHandleStatusCtrlEP();
extern uint8_t usbServiceEPWrite(volatile usbBDTEntry_t *epBD, uint8_t ep);
extern uint8_t usbServiceEPWriteArm(volatile usbBDTEntry_t *epBD, uint8_t ep);
extern uint8_t usbServiceEPRead(volatile usbBDTEntry_t *epBD, uint8_t ep);

extern volatile usbEP_t usbPacket;

#ifdef	__cplusplus
}
#endif

#endif	/* USB_H */

