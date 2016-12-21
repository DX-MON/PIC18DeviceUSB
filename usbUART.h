#ifndef USBUART_H
#define	USBUART_H

/*
 * @file
 * @author Rachel Mant
 *
 * @date 2015/02/19
 */

#ifdef	__cplusplus
extern "C"
{
#endif

#include <stdbool.h>

extern void usbUARTSendStringF(const char *str);
extern void usbUARTSendStringM(char *str);
extern void usbUARTSendChar(const char c);

extern bool usbUARTDataSent();
extern bool usbUARTHaveData();
extern char usbUARTRecvChar();

#ifdef	__cplusplus
}
#endif

#endif	/* USBUART_H */
