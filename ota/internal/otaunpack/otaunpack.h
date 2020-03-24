/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef __OTAUNPACK__
#define __OTAUNPACK__

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

typedef enum
{
    OTA_PROCESS_START,
    OTA_PROCESS_VERIFY,
    OTA_PROCESS_ERASE,
    OTA_PROCESS_UPDATE,
    OTA_PROCESS_VERIFY_FAIL,
    OTA_PROCESS_ERASE_FAIL,
    OTA_PROCESS_UPDATE_FAIL,
    OTA_PROCESS_ALL_DONE
}OTA_PROCESS_STATE;
typedef struct
{
    unsigned int total_step;
    unsigned int current_step;
    unsigned int precent_main;
    OTA_PROCESS_STATE state_sub;
    unsigned int precent_sub;
}OTA_Process_e;
typedef int (*OtaFileOpen)(const char *pFilePath);
typedef int (*OtaUbiVolOpen)(const char *pFilePath, unsigned int u32FileSize);
typedef int (*OtaFileCreate)(const char *pFilePath, int s32Mode);
typedef int (*OtaFileDelete)(const char *pFilePath);
typedef int (*OtaFileClose)(int fd);
typedef int (*OtaFileRead)(int fd, char *pBuf, int size);
typedef int (*OtaFileWrite)(int fd, char *pBuf, int size);
typedef int (*OtaGetBufSize)(void);
typedef int (*OtaRunDiffPatch)(const char *pOldFile, const char *pNewFile, const char *pPatchFile);
typedef int (*OtaRunScripts)(const char *pScripts, int s32DataSize);

typedef struct
{
    OtaFileOpen fpFileOpen;
    OtaFileClose fpFileClose;
    OtaFileRead fpFileRead;
}OTA_SrcFileOperation_t;
typedef struct
{
    OtaFileOpen fpFileOpen;
    OtaFileCreate fpFileCreate;
    OtaFileDelete fpFileDelete;
    OtaUbiVolOpen fpUbiFileOpen;
    OtaFileClose fpFileClose;
    OtaFileWrite fpFileWrite;
}OTA_DstFileOperation_t;

typedef void (*OtaNotifyProcess)(const OTA_Process_e *process, const char *message);
typedef struct
{
    const char *pSrcFile;
    const char *pDiffPatchPath;
    int isCompress;
    union
    {
        OTA_SrcFileOperation_t stInputNormalFile;
        OTA_SrcFileOperation_t stInputCompressedFile;
    };
    OTA_DstFileOperation_t stBlockUpgrade;
    OTA_DstFileOperation_t stUbiUpgrade;
    OTA_DstFileOperation_t stFileUpgrade;
    OtaRunDiffPatch fpRunDiffPatch;
    OtaRunScripts fpRunScripts;
    OtaGetBufSize fpGetBufSize;
    OtaNotifyProcess fpProcess;
}OTA_UserInterface_t;

int OtaUnPack(OTA_UserInterface_t *user);

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif //__OTAUNPACK__
