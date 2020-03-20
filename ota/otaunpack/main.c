#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "st_fb.h"
#include "otaunpack.h"

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

static void _NotifyPrecentInfo(const OTA_PROCESS *process, const char *message)
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
            u32TextColor = 0xFFFF0000;
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Verify fail: %s", message);
        }
        break;
        case OTA_PROCESS_ERASE_FAIL:
        {
            u32TextColor = 0xFFFF0000;
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Erase fail: %s", message);
        }
        break;
        case OTA_PROCESS_UPDATE_FAIL:
        {
            u32TextColor = 0xFFFF0000;
            u32BarColor = 0xFFFF0000;
            snprintf(print1, 128, "Upgrade fail: %s", message);
        }
        break;
        default:
        {
            u32TextColor = 0xFFFF0000;
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
int main(int argc, char **argv)
{
    int s32Ret = 0;
    unsigned char u8UpdateChoice = 0;
    char as8BsPatchTmpPath[64];
    char as8PicturePath[64];
    char as8DstFile[64];
    stRect_t stRect;

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
                u8UpdateChoice = 0;
                strncpy(as8DstFile, optarg, 64);
            }
            break;
            case 'r':
            {
                u8UpdateChoice = 1;
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
    ST_Fb_Init();
    stRect.u16X = 0;
    stRect.u16Y = 0;
    ST_Fb_GetLayerSz(&stRect.u16Width, &stRect.u16Height);
    ST_Fb_DrawPicture(as8PicturePath);        
    ST_FB_SyncDirtyUp(&stRect);
    s32Ret = OtaUnPack(as8DstFile, as8BsPatchTmpPath, u8UpdateChoice, _NotifyPrecentInfo);
    ST_Fb_Deinit();

    return s32Ret;
}
