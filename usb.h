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

