/*
 * usb.c
 *
 *  Created on: 2019年8月26日
 *      Author: koda.xu
 *
 *  该部分为自动mount usb的实现，自动mount usb分区名称为：/tmp/udiskX (X为分区索引，0，1，2...),暂不考虑在下列场景下出现mount分区重名的问题：
 *  手动mount usb -> 进入播放列表页面 -> 自动mount usb
 */

#ifdef SUPPORT_PLAYER_MODULE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/bitypes.h>
#include "usbdetect.h"
#include "list.h"


#define USB_PARTTITION_CHECK                "/proc/partitions"
#define USB_MOUNT_CHECK                     "/proc/mounts"
#define USB_FAT32_FORMAT                    "vfat"
#define USB_NTFS_FORMAT                     "ntfs"
#define USB_MOUNT_DEFAULT_DIR               "/vendor"

#define MAX_LINE_LEN        512
#define MAX_DEV_NAMELEN     256

typedef struct
{
	list_t partitionList;
	char partitionName[MAX_DEV_NAMELEN];
	char mountName[MAX_DEV_NAMELEN];
} UsbPartitionInfo_t;

static LIST_HEAD(g_usbPartitionListHead);
static char g_line[MAX_LINE_LEN];

static char *freadline(FILE *stream)
{
    int count = 0;

    while(!feof(stream) && (count < MAX_LINE_LEN) && ((g_line[count++] = getc(stream)) != '\n'));
    if (!count)
        return NULL;

    g_line[count - 1] = '\0';

    return g_line;
}

static int DetectUsbDev(void)
{
    FILE *pFile = fopen(USB_PARTTITION_CHECK, "r");
    char *pCurLine = NULL;
    char *pSeek = NULL;
    int nRet = -1;

    // refresh disk partition list
    if (!list_empty(&g_usbPartitionListHead))
    {
    	UsbPartitionInfo_t *pos = NULL;
    	UsbPartitionInfo_t *posN = NULL;
		list_for_each_entry_safe(pos, posN, &g_usbPartitionListHead, partitionList)
		{
			if (strlen(pos->mountName))
			{
				char umountCmd[128] = {0};
				char rmCmd[128] = {0};

				sprintf(umountCmd, "umount %s", pos->mountName);
				system(umountCmd);

				sprintf(rmCmd, "rm -rf %s", pos->mountName);
				system(rmCmd);
			}

			list_del(&pos->partitionList);
			free(pos);
		}
    }

    if (pFile)
    {
        while((pCurLine = freadline(pFile)) != NULL)
        {
            pSeek = strstr(pCurLine, "sd");
            if (pSeek)
            {
                if ((pSeek[2] >= 'a' && pSeek[2] <= 'z') && (pSeek[3] >= '1' && pSeek[3] <= '9'))
                {
                	UsbPartitionInfo_t *pUsbPartitionInfo = (UsbPartitionInfo_t*)malloc(sizeof(UsbPartitionInfo_t));
					memset(pUsbPartitionInfo, 0, sizeof(UsbPartitionInfo_t));
					INIT_LIST_HEAD(&pUsbPartitionInfo->partitionList);
					memcpy(pUsbPartitionInfo->partitionName, pSeek, 4);
					list_add_tail(&pUsbPartitionInfo->partitionList, &g_usbPartitionListHead);
					nRet = 0;
                }
            }
        }

        fclose(pFile);
        pFile = NULL;
    }

    if (nRet)
    	printf("Can't find usb device\n");

    return nRet;
}

static int GetUsbDevMountStatus()
{
    FILE *pFile = fopen(USB_MOUNT_CHECK, "r");
    char *pCurLine = NULL;
    char *pSeek = NULL;
    int nRet = 0;

    if (pFile)
    {
        while((pCurLine = freadline(pFile)) != NULL)
        {
        	UsbPartitionInfo_t *pos = NULL;
			list_for_each_entry(pos, &g_usbPartitionListHead, partitionList)
			{
				pSeek = strstr(pCurLine, pos->partitionName);
				if (pSeek)
				{
					char *pMount = NULL;
					pSeek += strlen(pos->partitionName);
					while(*(pSeek) == ' ')
						pSeek++;

					if (pSeek)
					{
						pMount = pSeek;
						while (*pSeek != ' ')
						{
							pSeek++;
						}

						memcpy(pos->mountName, pMount, (pSeek - pMount));
						printf("/dev/%s has been mounted on %s\n", pos->partitionName, pos->mountName);
						break;
					}
				}
			}
        }

        fclose(pFile);
        pFile = NULL;
        nRet = 0;
    }
    else
    {
        printf("open %s failed\n", USB_MOUNT_CHECK);
        nRet = -1;
    }

    return nRet;
}

static int AutoMountUsbDev()
{
	int mountState = -1;
	
	if (!GetUsbDevMountStatus())
    {
		UsbPartitionInfo_t *pos = NULL;
		int mountIdx = 0;

		list_for_each_entry(pos, &g_usbPartitionListHead, partitionList)
		{
			if (!strlen(pos->mountName))
			{
				int nRet = 0;
				char dirCmd[64] = {0};
				char mountCmd[128] = {0};
				char rmCmd[128] = {0};

				sprintf(dirCmd, "mkdir -p %s/udisk%d", USB_MOUNT_DEFAULT_DIR, mountIdx);
				system(dirCmd);

				sprintf(mountCmd, "mount -t %s -o iocharset=utf8 /dev/%s %s/udisk%d", USB_FAT32_FORMAT, pos->partitionName, USB_MOUNT_DEFAULT_DIR, mountIdx);
				nRet = system(mountCmd);
				if (!nRet)
				{
					sprintf(pos->mountName, "%s/udisk%d", USB_MOUNT_DEFAULT_DIR, mountIdx);
					printf("mount /dev/%s on %s success, usb format is FAT32.\n", pos->partitionName, pos->mountName);
					mountState = 0;
					mountIdx++;
					continue;
				}

				memset(mountCmd, 0, sizeof(mountCmd));
				sprintf(mountCmd, "mount -t %s -o iocharset=utf8 /dev/%s %s/udisk%d", USB_NTFS_FORMAT, pos->partitionName, USB_MOUNT_DEFAULT_DIR, mountIdx);
				nRet = system(mountCmd);
				if (!nRet)
				{
					sprintf(pos->mountName, "%s/udisk%d", USB_MOUNT_DEFAULT_DIR, mountIdx);
					printf("mount /dev/%s on %s success, usb format is NTFS.\n", pos->partitionName, pos->mountName);
					mountState = 0;
					mountIdx++;
					continue;
				}

				printf("mount /dev/%s on %s/udisk%d failed\n", pos->partitionName, USB_MOUNT_DEFAULT_DIR, mountIdx);
				sprintf(rmCmd, "rm -rf %s/udisk%d", USB_MOUNT_DEFAULT_DIR, mountIdx);
				system(rmCmd);
				//return -1;
			}
			else
				mountState = 0;
		}
    }
	else
	{
		printf("can't get disk partition info\n");
		mountState = -1;

	}

	return mountState;
}

static int AutoUmountUsbDev()
{
	if (!list_empty(&g_usbPartitionListHead))
	{
		UsbPartitionInfo_t *pos = NULL;
		UsbPartitionInfo_t *posN = NULL;
		char umountCmd[128];
		char rmCmd[128];
		int nRet = 0;

		list_for_each_entry_safe(pos, posN, &g_usbPartitionListHead, partitionList)
		{
			if (strlen(pos->mountName))
			{
				printf("mount name is %s\n", pos->mountName);
				memset(umountCmd, 0, sizeof(umountCmd));
				sprintf(umountCmd, "umount %s", pos->mountName);
				nRet = system(umountCmd);
				if (nRet)
				{
					printf("umount /dev/%s from %s failed\n", pos->partitionName, pos->mountName);
				}

				memset(rmCmd, 0, sizeof(rmCmd));
				sprintf(rmCmd, "rm -rf %s", pos->mountName);
				system(rmCmd);
			}

			list_del(&pos->partitionList);
			free(pos);
		}
	}

    return 0;
}

int SSTAR_InitUsbDev(char *pDirName, int nLen)
{
    int s32Ret = -1;
    s32Ret = DetectUsbDev();
    if (s32Ret)
        goto exit;

    s32Ret = AutoMountUsbDev();
    if (s32Ret)
        goto exit;

    strncpy(pDirName, USB_MOUNT_DEFAULT_DIR, nLen);

exit:
    return s32Ret;
}

int SSTAR_DeinitUsbDev()
{
    return AutoUmountUsbDev();
}
#endif
