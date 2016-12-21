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

#ifndef USBREQUESTS_H
#define	USBREQUESTS_H

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/01/25
 */

#ifdef	__cplusplus
extern "C"
{
#endif

extern bool usbHandleStandardRequest(volatile usbSetupPacket_t *packet);

extern volatile usbDeviceState usbState;
extern volatile uint8_t usbActiveConfig;

#ifdef	__cplusplus
}
#endif

#endif	/* USBREQUESTS_H */
