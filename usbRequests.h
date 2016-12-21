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
