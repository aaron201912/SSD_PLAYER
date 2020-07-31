/*
* main.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: malloc.peng <malloc.peng@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>

#ifndef __packed
#define __packed __attribute((packed))
#endif
#include <mtd/ubi-user.h>

#if USE_FB
#include "st_fb.h"
#endif
#include "otaunpack.h"

#define BUF_SIZE_BYTE 4096
#define OTA_FILE_NAME_LENGTH 256


static void _PrintHelp(void)
{
    printf("This tool can unpackage the file you give to do upgrade.\n");
    printf("Unpackage support compressed or normal file by using different option.\n");
    printf("'bspatch' is inside to do differential upgrade.");
    printf("Usage:\n");
    printf("To unpackage compressed file.\n");
    printf("    otaunpack -x SStarOta.bin.gz\n");
    printf("To unpackage a normal file.\n");
    printf("    otaunpack -r SStarOta.bin\n");
    printf("If the package has data to do differential upgrade, you should provide a writable path to store temporary file.\n");
    printf("    otaunpack -r SStarOta.bin -t /tmp\n");
    printf("    otaunpack -x SStarOta.bin.gz -t /tmp\n");
    printf("Option -p [file] is to show the pictures(jpeg, png) while upgrading\n");
    printf("    otaunpack -x SStarOta.bin.gz -t /tmp -p logo.jpg\n");
}
static int _OpenFile(const char *pFilePath)
{
    int s32Fd = open(pFilePath, O_RDONLY);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }

    return s32Fd;
}
static int _CloseFile(int s32Fd)
{
    fsync(s32Fd);
    return close(s32Fd);
}
static int _FileRead(int fd, char *pBuf, int size)
{
    int s32ReadCnt = 0;
    int s32Ret = 0;
    do
    {
        s32Ret = read(fd, pBuf, size - s32ReadCnt);
        if (s32Ret < 0)
        {
            return -1;
        }
        if (s32Ret == 0)
        {
            break;
        }
        s32ReadCnt += s32Ret;
        pBuf += s32Ret;
    }while(s32ReadCnt < size);

    return s32ReadCnt;
}
static int _FileWrite(int fd, char *pBuf, int size)
{
    int s32WriteCnt = 0;
    int s32Ret = 0;
    do
    {
        s32Ret = write(fd, pBuf, size - s32WriteCnt);
        if (s32Ret < 0)
        {
            perror("write");
            return -1;
        }
        if (s32Ret == 0)
        {
            break;
        }
        s32WriteCnt += s32Ret;
        pBuf += s32Ret;
    }while(s32WriteCnt < size);

    return s32WriteCnt;
}

static int gs32FdTmp = 0;
static pid_t fPid;
static int _FileLockSet(int fd, int type)
{
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = type;
    lock.l_pid = -1;

    fcntl(fd, F_GETLK, &lock);
    if (lock.l_type != F_UNLCK)
    {
        if (lock.l_type == F_RDLCK)
        {
            fprintf(stderr, "Read lock already set by %d\n", lock.l_pid);
            return -1;
        }
        else if (lock.l_type == F_WRLCK)
        {
            fprintf(stderr, "Write lock already set by %d\n", lock.l_pid);
            return -1;
        }
    }

    lock.l_type = type;
    if ((fcntl(fd, F_SETLKW, &lock)) < 0)
    {
        fprintf(stderr, "Lock failed:type = %d\n", lock.l_type);
        return 1;
    }

    switch(lock.l_type)
    {
        case F_RDLCK:
        {
            fprintf(stderr, "Read lock set by %d\n", getpid());
        }
        break;
        case F_WRLCK:
        {
            fprintf(stderr, "Write lock set by %d\n", getpid());
        }
        break;
        case F_UNLCK:
        {
            fprintf(stderr, "Release lock by %d\n", getpid());
            return 1;
        }
        break;
        default:
        break;
    }/* end of switch */
    return 0;
}

static int _OpenCompressedFile(const char *pOpenData)
{
    int s32Fd[2];

    if (pipe(s32Fd) < 0)
    {
        perror("pipe");

        return -1;
    }
    gs32FdTmp = dup(STDOUT_FILENO);
    if (gs32FdTmp < 0)
    {
        perror("dup");

        return -1;
    }
    if (dup2(s32Fd[1], STDOUT_FILENO) < 0)
    {
        perror("dup2");
        close(s32Fd[0]);
        close(s32Fd[1]);
        close(gs32FdTmp);

        return -1;
    }
    _FileLockSet(STDOUT_FILENO, F_WRLCK);
    fPid = fork();
    switch (fPid)
    {
        case -1:
        {
            perror("error in fork!");
            close(s32Fd[0]);
            close(s32Fd[1]);
            close(gs32FdTmp);

            return -1;
        }
        break;
        case 0:
        {
            close(s32Fd[0]);
            if(execlp("gunzip", "gunzip", "-c", pOpenData, (char *)NULL) < 0)
            {
                perror("execlp error!");
                close(s32Fd[1]);
                close(gs32FdTmp);

                exit(-1);
            }
            close(s32Fd[1]);
            close(gs32FdTmp);
            exit(0);
        }
        break;
        default:
            break;
    }
    close(s32Fd[1]);

    return s32Fd[0];
}
static int _CloseCompressedFile(int s32Fd)
{

    kill(fPid, SIGKILL);
    waitpid(fPid, NULL, 0);
    _FileLockSet(STDOUT_FILENO, F_UNLCK);
    if (dup2(gs32FdTmp, STDOUT_FILENO) < 0)
    {
        perror("dup2");
        close(s32Fd);
        close(gs32FdTmp);

        return -1;
    }
    close(s32Fd);
    close(gs32FdTmp);
    return 0;
}
static int _OpenBlockFile(const char *pOpenData)
{
    int s32Fd = open(pOpenData, O_WRONLY);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }

    return s32Fd;
}
int _GetBlockSize(const char *pBlockPath)
{
    int intSize = 0;
    int s32Fd = open(pBlockPath, O_RDONLY);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }
    intSize = lseek(s32Fd, 0, SEEK_END);
    close(s32Fd);

    return intSize;
}

static int _OpenUbiDev(const char *pOpenData, unsigned int size)
{
    unsigned long long u64UbiVolSize = (unsigned long long)size;

    int s32Fd = open(pOpenData, O_WRONLY);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }
    ioctl(s32Fd, UBI_IOCVOLUP, &u64UbiVolSize);

    return s32Fd;
}
static int _OpenNormalFile(const char *pOpenData)
{
    int s32Fd = open(pOpenData, O_WRONLY | O_TRUNC);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }

    return s32Fd;
}
static int _CreateNormalFile(const char *pFile, int s32Mode)
{
    char s8MkdirCmd[OTA_FILE_NAME_LENGTH * 2];
    char s8Dir[OTA_FILE_NAME_LENGTH];
    char *pFileNameLocate = NULL;
    char *pTmp = NULL;

    memset(s8MkdirCmd, 0, OTA_FILE_NAME_LENGTH);
    memset(s8Dir, 0, OTA_FILE_NAME_LENGTH);
    strcpy(s8Dir, pFile);
    pTmp = strstr(s8Dir, "/");
    while (pTmp)
    {
        pFileNameLocate = pTmp + 1;
        pTmp = strstr(pFileNameLocate, "/");
    }
    if (pFileNameLocate)
    {
        *pFileNameLocate = 0;
        snprintf(s8MkdirCmd, OTA_FILE_NAME_LENGTH * 2, "mkdir -p %s", s8Dir);
        system(s8MkdirCmd);
    }

    int s32Fd = open(pFile, O_WRONLY | O_TRUNC | O_CREAT, s32Mode);
    if (s32Fd < 0)
    {
        perror("open");

        return -1;
    }

    return s32Fd;
}
static int _GetBufSizeByte(void)
{
    return BUF_SIZE_BYTE;
}
static int _RunBsPatch(const char *s8OldFile, const char *s8NewFile, const char *s8PatchFile)
{
    pid_t pid;
    int s32Ret;

    pid = fork();
    switch (pid)
    {
        case -1:
        {
            return -1;
        }
        case 0:
        {
            s32Ret = execlp("bspatch", "bspatch", s8OldFile, s8NewFile, s8PatchFile, (char *)NULL);
            if (s32Ret < 0)
            {
                perror("execlp error!");
                exit(-1);
            }
            exit(s32Ret);
        }
        default:
        {
            waitpid(pid, &s32Ret, 0);
            if (s32Ret < 0)
            {
                fprintf(stderr, "bspatch exec error\n");

                return -1;
            }
        }
        break;
    }

    return 0;
}
static int _RunScripts(const char *pScripts, int s32DataSize)
{
    FILE *pFile = NULL;
    int s32Ret = 0;
    const char *pDataTmp = NULL;
    int s32WrCnt = 0;

    pFile = popen("sh", "w");
    if (!pFile)
    {
        s32Ret = -1;

        return s32Ret;
    }
    do
    {
        s32Ret = fwrite(pScripts, 1, s32DataSize, pFile);
        if (s32Ret < 0)
        {
            perror("fwrite");
            s32Ret = -1;

            goto ERR;
        }
        if (s32Ret == 0)
        {
            break;
        }
        s32WrCnt += s32Ret;
        pDataTmp += s32Ret;
    }while(s32WrCnt < s32DataSize);

ERR:
    pclose(pFile);

    return s32Ret;
}
#if USE_FB
static void _DrawMainBar(unsigned int barHeight, unsigned int u32Precent, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16Width = layerW * 618 / 1000 * u32Precent / 100;
    pRect->u16Height = barHeight;
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000;
    ST_Fb_FillRect(pRect, u32Color);

    return;
}
static void _DrawSubBar(unsigned int barHeight, unsigned int u32Precent, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16Width = layerW * 618 / 1000 * u32Precent / 100;
    pRect->u16Height = barHeight;
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000 - pRect->u16Height - 4;
    ST_Fb_FillRect(pRect, u32Color);

    return;
}
static void _DrawStaticBarMain(unsigned int barHeight, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16Width = layerW * 618 / 1000;
    pRect->u16Height = barHeight;
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000;
    ST_Fb_FillRect(pRect, u32Color);

    return;
}
static void _DrawMainText(const char *pText, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    ST_Fb_GetFontSz(&pRect->u16Width, &pRect->u16Height);
    pRect->u16Width *= strlen(pText);
    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000 + 4;
    ST_Fb_DrawText(pRect->u16X, pRect->u16Y, u32Color, pText);
}
static void _DrawStaticBarSub(unsigned int barHeight, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16Width = layerW * 618 / 1000;
    pRect->u16Height = barHeight;
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000 - pRect->u16Height - 4;
    ST_Fb_FillRect(pRect, u32Color);

    return;
}
static void _DrawSubText(const char *pText, unsigned int u32Color, stRect_t *pRect)
{
    unsigned short layerW = 0;
    unsigned short layerH = 0;

    ST_Fb_GetLayerSz(&layerW, &layerH);
    ST_Fb_GetFontSz(&pRect->u16Width, &pRect->u16Height);
    pRect->u16Width *= strlen(pText);
    ST_Fb_GetLayerSz(&layerW, &layerH);
    pRect->u16X = (layerW - layerW * 618 / 1000) / 2;
    pRect->u16Y = layerH * 750 / 1000 - pRect->u16Height - 4;
    ST_Fb_DrawText(pRect->u16X, pRect->u16Y, u32Color, pText);
}

static void _NotifyPrecentInfo(const OTA_Process_e *process, const char *message)
{
    char print0[128];
    char print1[128];
    unsigned int u32TextColor = 0xFF000000;
    unsigned int u32SubTextColor = 0xFF000000;
    unsigned int u32BarColor = 0xFF00FF00;
    unsigned int u32StaticBarColor = 0xFF00AEEF;
    static stRect_t stOldRect0;
    static stRect_t stOldRect1;
    static stRect_t stOldMainBarRect;
    static stRect_t stOldStaticMainBarRect;
    static stRect_t stOldSubBarRect;
    static stRect_t stOldStaticSubBarRect;
    static int bFirst = 1;
    stRect_t stRect0;
    stRect_t stRect1;
    stRect_t stMainBarRect;
    stRect_t stStaticMainBarRect;
    stRect_t stSubBarRect;
    stRect_t stStaticSubBarRect;
    stRect_t stRectCom;

    memset(print0, 0, 128);
    memset(print1, 0, 128);
    memset(&stRectCom, 0, sizeof(stRect_t));

    snprintf(print0, 128, "(%d%%)(%d/%d)", process->precent_main, process->current_step, process->total_step);
    switch (process->state_sub)
    {
        case OTA_PROCESS_START:
        {
            snprintf(print1, 128, "(%d%%)Start", process->precent_sub);
        }
        break;
        case OTA_PROCESS_VERIFY:
        {
            snprintf(print1, 128, "(%d%%)Verifying(%s)", process->precent_sub, message);
        }
        break;
        case OTA_PROCESS_ERASE:
        {
            snprintf(print1, 128, "(%d%%)Erasing(%s)", process->precent_sub, message);
        }
        break;
        case OTA_PROCESS_UPDATE:
        {
            snprintf(print1, 128, "(%d%%)Updating(%s)", process->precent_sub, message);
        }
        break;
        case OTA_PROCESS_ALL_DONE:
        {
            snprintf(print1, 128, "(100%%)Success");
        }
        break;
        case OTA_PROCESS_VERIFY_FAIL:
        {
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Verify fail: %s", message);
        }
        break;
        case OTA_PROCESS_ERASE_FAIL:
        {
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Erase fail: %s", message);
        }
        break;
        case OTA_PROCESS_UPDATE_FAIL:
        {
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Upgrade fail: %s", message);
        }
        break;
        default:
        {
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Unknown fail");
        }
        break;
    }
    ST_FB_SyncDirtyDown();
    if (bFirst)
    {
        bFirst = 0;
    }
    else
    {
        ST_Fb_ClearRect(&stOldMainBarRect);
        ST_Fb_ClearRect(&stOldStaticMainBarRect);
        ST_Fb_ClearRect(&stOldSubBarRect);
        ST_Fb_ClearRect(&stOldStaticSubBarRect);
        ST_Fb_ClearRect(&stOldRect0);
        ST_Fb_ClearRect(&stOldRect1);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldRect0);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldRect1);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldMainBarRect);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldStaticMainBarRect);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldSubBarRect);
        ST_FB_CombineRect(&stRectCom, &stRectCom, &stOldStaticSubBarRect);

    }
    _DrawStaticBarMain(32, u32StaticBarColor, &stStaticMainBarRect);
    _DrawMainBar(32, process->precent_main, u32BarColor, &stMainBarRect);
    _DrawStaticBarSub(32, u32StaticBarColor, &stStaticSubBarRect);
    _DrawSubBar(32, process->precent_sub, u32BarColor, &stSubBarRect);
    _DrawMainText(print0, u32TextColor, &stRect0);
    _DrawSubText(print1, u32SubTextColor, &stRect1);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stMainBarRect);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stStaticMainBarRect);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stSubBarRect);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stStaticSubBarRect);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stRect0);
    ST_FB_CombineRect(&stRectCom, &stRectCom, &stRect1);
    ST_FB_SyncDirtyUp(&stRectCom);
    stOldRect0 = stRect0;
    stOldRect1 = stRect1;
    stOldMainBarRect= stMainBarRect;
    stOldStaticMainBarRect = stStaticMainBarRect;
    stOldSubBarRect= stSubBarRect;
    stOldStaticSubBarRect = stStaticSubBarRect;
}
#else
static void _NotifyPrecentInfo(const OTA_Process_e *process, const char *message)
{
    fprintf(stderr, "(%d%%)(%d/%d)(state sub %d)\n", process->precent_main, process->current_step, process->total_step, process->state_sub);
}
#endif
int main(int argc, char **argv)
{
    int s32Ret = 0;
    unsigned char u8UpdateChoice = 0;
    char as8BsPatchTmpPath[64];
    char as8PicturePath[64];
    char as8DstFile[64];
    OTA_UserInterface_t stOtaApi;

    if (argc == 1)
    {
        _PrintHelp();
        return -1;
    }
    memset(as8BsPatchTmpPath, 0, 64);
    memset(as8PicturePath, 0, 64);

    while ((s32Ret = getopt(argc, argv, "x:r:t:p:")) != -1)
    {
        switch (s32Ret)
        {
            case 'x':
            {
                u8UpdateChoice = 1;
                strncpy(as8DstFile, optarg, 64);
            }
            break;
            case 'r':
            {
                u8UpdateChoice = 0;
                strncpy(as8DstFile, optarg, 64);
            }
            break;
            case 't':
            {
                strncpy(as8BsPatchTmpPath, optarg, 64);
            }
            break;
            case 'p':
            {
                strncpy(as8PicturePath, optarg, 64);
            }
            break;
            default:
                _PrintHelp();
                return -1;
        }
    }
    if (strlen(as8DstFile) == 0)
    {
        _PrintHelp();
        return -1;
    }
#if USE_FB
    stRect_t stRect;

    ST_Fb_Init();
    if (strlen(as8PicturePath))
    {
        stRect.u16X = 0;
        stRect.u16Y = 0;
        ST_Fb_GetLayerSz(&stRect.u16Width, &stRect.u16Height);
        ST_FB_SyncDirtyDown();
        ST_Fb_DrawPicture(as8PicturePath);
        ST_FB_SyncDirtyUp(&stRect);
    }
#endif
    memset(&stOtaApi, 0, sizeof(OTA_UserInterface_t));
    if (u8UpdateChoice)
    {
        stOtaApi.stInputFile.fpFileOpen = _OpenCompressedFile;
        stOtaApi.stInputFile.fpFileClose = _CloseCompressedFile;
        stOtaApi.stInputFile.fpFileRead = _FileRead;
    }
    else
    {
        stOtaApi.stInputFile.fpFileOpen = _OpenFile;
        stOtaApi.stInputFile.fpFileClose = close;
        stOtaApi.stInputFile.fpFileRead = _FileRead;
    }
    stOtaApi.stBlockUpgrade.fpFileOpen = _OpenBlockFile;
    stOtaApi.stBlockUpgrade.fpFileClose = _CloseFile;
    stOtaApi.stBlockUpgrade.fpFileWrite= _FileWrite;
    stOtaApi.stBlockUpgrade.fpGetSize = _GetBlockSize;
    stOtaApi.stUbiUpgrade.fpFileOpen = _OpenUbiDev;
    stOtaApi.stUbiUpgrade.fpFileClose = close;
    stOtaApi.stUbiUpgrade.fpFileWrite= _FileWrite;
    stOtaApi.stFileUpgrade.fpFileOpen = _OpenNormalFile;
    stOtaApi.stFileUpgrade.fpFileCreate = _CreateNormalFile;
    stOtaApi.stFileUpgrade.fpFileDelete = remove;
    stOtaApi.stFileUpgrade.fpFileClose = _CloseFile;
    stOtaApi.stFileUpgrade.fpFileWrite= _FileWrite;
    stOtaApi.fpProcess = _NotifyPrecentInfo;
    stOtaApi.fpGetBufSize = _GetBufSizeByte;
    stOtaApi.fpRunDiffPatch = _RunBsPatch;
    stOtaApi.fpRunScripts = _RunScripts;
    stOtaApi.pSrcFile = as8DstFile;
    stOtaApi.pDiffPatchPath = as8BsPatchTmpPath;
    s32Ret = OtaUnPack(&stOtaApi);
#if USE_FB
    ST_Fb_Deinit();
#endif

    return s32Ret;
}
