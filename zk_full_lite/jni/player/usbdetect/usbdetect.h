/*
 * usb.h
 *
 *  Created on: 2019年8月26日
 *      Author: koda.xu
 */

#ifndef JNI_PLAYER_USB_H_
#define JNI_PLAYER_USB_H_


#ifdef  __cplusplus
extern "C"
{
#endif

#ifdef SUPPORT_PLAYER_MODULE

int SSTAR_InitUsbDev(char *pDirName, int nLen);
int SSTAR_DeinitUsbDev();
#endif

#ifdef  __cplusplus
}
#endif

#endif /* JNI_PLAYER_USB_H_ */
