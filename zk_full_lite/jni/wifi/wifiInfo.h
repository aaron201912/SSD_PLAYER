/*
 * wifiInfo.h
 *
 *  Created on: 2019年8月15日
 *      Author: koda.xu
 */

#ifndef JNI_WIFIINFO_H_
#define JNI_WIFIINFO_H_

#ifdef SUPPORT_WLAN_MODULE
#include "mi_common_datatype.h"
#include "mi_wlan.h"

typedef struct
{
	int index;
	char passwd[40];
} Wifi_ConnectInfo_t;

bool getWifiInsmodStatus();
void setWifiInsmodStatus(bool enable);
bool getWifiSupportStatus();
void setWifiSupportStatus(bool enable);
bool getWifiEnableStatus();
void setWifiEnableStatus(bool enable);
bool getConnectionStatus();
void setConnectionStatus(bool enable);
WLAN_HANDLE getWlanHandle();
void setWlanHandle(WLAN_HANDLE handle);
MI_WLAN_ConnectParam_t * getConnectParam();
void saveConnectParam(MI_WLAN_ConnectParam_t *pConnParam);
int checkProfile();
int initWifiConfig();
int saveWifiConfig();
void checkSsidExist(char *ssid, Wifi_ConnectInfo_t *pConnInfo);
int addSsidToApList(char *ssid);
int addSsidInfoToConnList(char *ssid, Wifi_ConnectInfo_t *pConnInfo);
int updateCurSsid(char *ssid);
int updateConnPasswd(char *ssid, char *passwd);
int updateEnableStatus(bool bEnable);

#endif

#endif /* JNI_WIFIINFO_H_ */
