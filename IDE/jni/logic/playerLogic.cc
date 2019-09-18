#pragma once
#include "uart/ProtocolSender.h"
/*
*此文件由GUI工具生成
*文件功能：用于处理用户的逻辑相应代码
*功能说明：
*========================onButtonClick_XXXX
当页面中的按键按下后系统会调用对应的函数，XXX代表GUI工具里面的[ID值]名称，
如Button1,当返回值为false的时候系统将不再处理这个按键，返回true的时候系统将会继续处理此按键。比如SYS_BACK.
*========================onSlideWindowItemClick_XXXX(int index) 
当页面中存在滑动窗口并且用户点击了滑动窗口的图标后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如slideWindow1;index 代表按下图标的偏移值
*========================onSeekBarChange_XXXX(int progress) 
当页面中存在滑动条并且用户改变了进度后系统会调用此函数,XXX代表GUI工具里面的[ID值]名称，
如SeekBar1;progress 代表当前的进度值
*========================ogetListItemCount_XXXX() 
当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表的总数目,XXX代表GUI工具里面的[ID值]名称，
如List1;返回值为当前列表的总条数
*========================oobtainListItemData_XXXX(ZKListView::ZKListItem *pListItem, int index)
 当页面中存在滑动列表的时候，更新的时候系统会调用此接口获取列表当前条目下的内容信息,XXX代表GUI工具里面的[ID值]名称，
如List1;pListItem 是贴图中的单条目对象，index是列表总目的偏移量。具体见函数说明
*========================常用接口===============
*LOGD(...)  打印调试信息的接口
*mTextXXXPtr->setText("****") 在控件TextXXX上显示文字****
*mButton1Ptr->setSelected(true); 将控件mButton1设置为选中模式，图片会切换成选中图片，按钮文字会切换为选中后的颜色
*mSeekBarPtr->setProgress(12) 在控件mSeekBar上将进度调整到12
*mListView1Ptr->refreshListView() 让mListView1 重新刷新，当列表数据变化后调用
*mDashbroadView1Ptr->setTargetAngle(120) 在控件mDashbroadView1上指针显示角度调整到120度
*
* 在Eclipse编辑器中  使用 “alt + /”  快捷键可以打开智能提示
*/
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include "mi_sys.h"
#include "mi_divp.h"
#include "mi_disp.h"
#include "mi_panel.h"
#include "mi_gfx.h"

#include "usbdetect.h"
#include "frame.h"
#include "demux.h"
#include "videostream.h"
#include "audiostream.h"
#include "player.h"


#define USE_PANEL_1024_600		1
#define USE_MIPI_PANEL			0

#if USE_PANEL_1024_600
#include "SAT070CP50_1024x600.h"
#else
#include "SAT070AT50_800x480.h"
#endif

#define UI_MAX_WIDTH			800
#define UI_MAX_HEIGHT			480

#if USE_PANEL_1024_600
#define PANEL_MAX_WIDTH			1024
#define PANEL_MAX_HEIGHT		600
#else
#define PANEL_MAX_WIDTH			800
#define PANEL_MAX_HEIGHT		480
#endif

#define ALIGN_DOWN(x, n)		(x / n * n)
#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE              MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED                MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN              MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE               MAKE_YUYV_VALUE(29,225,107)

#define DIVP_CHN        		0
#define DISP_DEV        		0
#define DISP_LAYER      		0
#define DISP_INPUTPORT  		0
#define AUDIO_DEV       		0
#define AUDIO_CHN       		0
#define AUDIO_SAMPLE_PER_FRAME  1024
#define MIN_AO_VOLUME           -60
#define MAX_AO_VOLUME           30
#define MIN_ADJUST_AO_VOLUME    -10
#define MAX_ADJUST_AO_VOLUME    20

typedef enum
{
    E_PLAY_FORWARD,
    E_PLAY_BACKWARD
}PlayDirection_e;

typedef enum
{
    E_PLAY_NORMAL_MODE,
    E_PLAY_FAST_MODE,
    E_PLAY_SLOW_MODE
}PlayMode_e;

typedef enum
{
    E_NORMAL_SPEED = 0,
    E_2X_SPEED,
    E_4X_SPEED,
    E_8X_SPEED,
    E_16X_SPEED,
    E_32X_SPEED
}PlaySpeedMode_e;

// playing page
static bool g_bShowPlayToolBar = FALSE;          // select file list page or playing page
static bool g_bPlaying = FALSE;
static bool g_bPause = FALSE;
static bool g_bMute = FALSE;
static int g_s32VolValue = 0;
static bool g_ePlayDirection = E_PLAY_FORWARD;
static PlayMode_e g_ePlayMode = E_PLAY_NORMAL_MODE;
static PlaySpeedMode_e g_eSpeedMode = E_NORMAL_SPEED;
static unsigned int g_u32SpeedNumerator = 1;
static unsigned int g_u32SpeedDenomonator = 1;

// playViewer size
static int g_playViewWidth = PANEL_MAX_WIDTH;
static int g_playViewHeight = PANEL_MAX_HEIGHT;

// media file video size
static int g_videoStreamWidth = 1280;
static int g_videoStreamHeight = 720;

// play circle, play list
static bool g_bPlayCircle = false;
static bool g_bPlayInList = false;
// file list, get from playlist.txt

static std::string fileName;
static player_stat_t *g_pstPlayStat = NULL;

void SetPlayingStatus(bool bPlaying)
{
//	if (bPlaying)
//		mButton_playPtr->setBackgroundPic("player/pause.png");
//	else
//		mButton_playPtr->setBackgroundPic("player/play.png");

	mButton_playPtr->setSelected(bPlaying);
}

void SetMuteStatus(bool bMute)
{
//	if (bMute)
//		mButton_voicePtr->setBackgroundPic("player/silence.png");
//	else
//		mButton_voicePtr->setBackgroundPic("player/volumn.png");

	mButton_voicePtr->setSelected(bMute);
}

int SetPlayerVolumn(int vol)
{
	mSeekbar_volumnPtr->setProgress(vol);
	return 0;
}

int GetPlayerVolumn()
{
	return mSeekbar_volumnPtr->getProgress();
}

MI_S32 CreatePlayerDev()
{
    MI_SYS_Version_t stVersion;
    MI_U64 u64Pts = 0;
    MI_DISP_PubAttr_t stPubAttr;
    MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_SYS_ChnPort_t stDivpChnPort;
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DISP_DEV dispDev = DISP_DEV;
    MI_DISP_LAYER dispLayer = DISP_LAYER;
    MI_U32 u32InputPort = DISP_INPUTPORT;
    MI_SYS_ChnPort_t stDispChnPort;
    MI_DISP_InputPortAttr_t stInputPortAttr;
    MI_PANEL_LinkType_e eLinkType;
#if 0
    MI_SYS_Init();
    memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
    MI_SYS_GetVersion(&stVersion);
    MI_SYS_GetCurPts(&u64Pts);
    u64Pts = 0xF1237890F1237890;
    MI_SYS_InitPtsBase(u64Pts);
    u64Pts = 0xE1237890E1237890;
    MI_SYS_SyncPts(u64Pts);
#endif
    memset(&stDivpChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stDivpChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnPort.u32DevId = 0;
    stDivpChnPort.u32ChnId = DIVP_CHN;
    stDivpChnPort.u32PortId = 0;
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = 0;
    stDivpChnAttr.stCropRect.u16Y       = 0;
    stDivpChnAttr.stCropRect.u16Width   = 0;
    stDivpChnAttr.stCropRect.u16Height  = 0;
    stDivpChnAttr.u32MaxWidth           = 1920; // g_playViewWidth;
    stDivpChnAttr.u32MaxHeight			= 1080;	// g_playViewHeight;
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stOutputPortAttr.u32Width           = g_playViewWidth;
    stOutputPortAttr.u32Height          = g_playViewHeight;

    printf("divp maxW=%d, maxH=%d, w=%d, h=%d\n", stDivpChnAttr.u32MaxWidth, stDivpChnAttr.u32MaxHeight,
    		stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);

    MI_DIVP_CreateChn(DIVP_CHN, &stDivpChnAttr);
    MI_DIVP_StartChn(DIVP_CHN);
    MI_DIVP_SetOutputPortAttr(DIVP_CHN, &stOutputPortAttr);
    MI_SYS_SetChnOutputPortDepth(&stDivpChnPort, 0, 3);
#if 0
    stPubAttr.stSyncInfo.u16Vact = stPanelParam.u16Height;
    stPubAttr.stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
    stPubAttr.stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth + stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
    stPubAttr.stSyncInfo.u16Hact = stPanelParam.u16Width;
    stPubAttr.stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
    stPubAttr.stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth + stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
    stPubAttr.stSyncInfo.u16Bvact = 0;
    stPubAttr.stSyncInfo.u16Bvbb = 0;
    stPubAttr.stSyncInfo.u16Bvfb = 0;
    stPubAttr.stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
    stPubAttr.stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
    stPubAttr.stSyncInfo.u32FrameRate = stPanelParam.u16DCLK*1000000/(stPanelParam.u16HTotal*stPanelParam.u16VTotal);
    stPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stPubAttr.u32BgColor = YUYV_BLACK;
    MI_DISP_SetPubAttr(dispDev, &stPubAttr);
    MI_DISP_Enable(dispDev);

    stLayerAttr.stVidLayerSize.u16Width  = stPanelParam.u16Width;
    stLayerAttr.stVidLayerSize.u16Height = stPanelParam.u16Height;
    stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stLayerAttr.stVidLayerDispWin.u16X      = 0;
    stLayerAttr.stVidLayerDispWin.u16Y      = 0;
    stLayerAttr.stVidLayerDispWin.u16Width  = stPanelParam.u16Width;
    stLayerAttr.stVidLayerDispWin.u16Height = stPanelParam.u16Height;
    MI_DISP_SetVideoLayerAttr(dispLayer, &stLayerAttr);
    MI_DISP_BindVideoLayer(dispLayer, dispDev);
    MI_DISP_EnableVideoLayer(dispLayer);
#endif
    memset(&stDispChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stInputPortAttr, 0, sizeof(MI_DISP_InputPortAttr_t));
    stDispChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDispChnPort.u32DevId = DISP_DEV;
    stDispChnPort.u32ChnId = 0;
    stDispChnPort.u32PortId = DISP_INPUTPORT;
    MI_DISP_GetInputPortAttr(dispLayer, u32InputPort, &stInputPortAttr);
    stInputPortAttr.stDispWin.u16X      = 0;
    stInputPortAttr.stDispWin.u16Y      = 0;
    stInputPortAttr.stDispWin.u16Width  = g_playViewWidth;
    stInputPortAttr.stDispWin.u16Height = g_playViewHeight;
    stInputPortAttr.u16SrcWidth = g_playViewWidth;
    stInputPortAttr.u16SrcHeight = g_playViewHeight;

    printf("disp input: w=%d, h=%d\n", stInputPortAttr.u16SrcWidth, stInputPortAttr.u16SrcHeight);
    MI_DISP_SetInputPortAttr(dispLayer, u32InputPort, &stInputPortAttr);
    MI_DISP_GetInputPortAttr(dispLayer, u32InputPort, &stInputPortAttr);
    MI_DISP_EnableInputPort(dispLayer, u32InputPort);
    MI_DISP_SetInputPortSyncMode(dispLayer, u32InputPort, E_MI_DISP_SYNC_MODE_FREE_RUN);
#if 0
#if USE_MIPI_PANEL
    eLinkType = E_MI_PNL_LINK_MIPI_DSI;
#else
    eLinkType = E_MI_PNL_LINK_TTL;
#endif

    MI_PANEL_Init(eLinkType);
    MI_PANEL_SetPanelParam(&stPanelParam);
    if (eLinkType == E_MI_PNL_LINK_MIPI_DSI)
    {
    	MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
    }
#endif
    MI_GFX_Open();
    MI_SYS_BindChnPort(&stDivpChnPort, &stDispChnPort, 30, 30);

    return MI_SUCCESS;
}


void DestroyPlayerDev()
{
    MI_DISP_DEV dispDev = DISP_DEV;
    MI_DISP_LAYER dispLayer = DISP_LAYER;
    MI_U32 u32InputPort = DISP_INPUTPORT;
    MI_SYS_ChnPort_t stDivpChnPort;
    MI_SYS_ChnPort_t stDispChnPort;

    memset(&stDivpChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    memset(&stDispChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stDivpChnPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnPort.u32DevId = 0;
    stDivpChnPort.u32ChnId = DIVP_CHN;
    stDivpChnPort.u32PortId = 0;
    stDispChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDispChnPort.u32DevId = DISP_DEV;
    stDispChnPort.u32ChnId = 0;
    stDispChnPort.u32PortId = DISP_INPUTPORT;
    MI_SYS_UnBindChnPort(&stDivpChnPort, &stDispChnPort);

    MI_GFX_Close();
#if 0
    MI_PANEL_DeInit();
    MI_DISP_DisableInputPort(dispLayer, u32InputPort);
    MI_DISP_DisableVideoLayer(dispLayer);
    MI_DISP_UnBindVideoLayer(dispLayer, dispDev);
    MI_DISP_Disable(dispDev);
#endif
    MI_DIVP_StopChn(DIVP_CHN);
    MI_DIVP_DestroyChn(DIVP_CHN);

    //MI_SYS_Exit();
}

MI_S32 CreatePlayerDev2(MI_PANEL_ParamConfig_t *stPanelParam, MI_U16 width, MI_U16 height)
{
    MI_DISP_DEV dispDev = DISP_DEV;
	MI_DISP_LAYER dispLayer = DISP_LAYER;
    MI_U32 u32InputPort = DISP_INPUTPORT;
	MI_DISP_PubAttr_t stPubAttr;
	MI_DISP_VideoLayerAttr_t stLayerAttr;
    MI_DISP_InputPortAttr_t stInputPortAttr;

	//sstar_sys_init();
	MI_SYS_Version_t stVersion;
	MI_U64 u64Pts = 0;

	MI_SYS_Init();
	memset(&stVersion, 0x0, sizeof(MI_SYS_Version_t));
	MI_SYS_GetVersion(&stVersion);
	MI_SYS_GetCurPts(&u64Pts);
	u64Pts = 0xF1237890F1237890;
	MI_SYS_InitPtsBase(u64Pts);
	u64Pts = 0xE1237890E1237890;
	MI_SYS_SyncPts(u64Pts);


	memset(&stPubAttr, 0, sizeof(MI_DISP_PubAttr_t));
    stPubAttr.stSyncInfo.u16Vact = stPanelParam->u16Height;
    stPubAttr.stSyncInfo.u16Vbb  = stPanelParam->u16VSyncBackPorch;
    stPubAttr.stSyncInfo.u16Vfb  = stPanelParam->u16VTotal - (stPanelParam->u16VSyncWidth + stPanelParam->u16Height + stPanelParam->u16VSyncBackPorch);
    stPubAttr.stSyncInfo.u16Hact = stPanelParam->u16Width;
    stPubAttr.stSyncInfo.u16Hbb  = stPanelParam->u16HSyncBackPorch;
    stPubAttr.stSyncInfo.u16Hfb  = stPanelParam->u16HTotal - (stPanelParam->u16HSyncWidth + stPanelParam->u16Width + stPanelParam->u16HSyncBackPorch);
    stPubAttr.stSyncInfo.u16Bvact= 0;
    stPubAttr.stSyncInfo.u16Bvbb = 0;
    stPubAttr.stSyncInfo.u16Bvfb = 0;
    stPubAttr.stSyncInfo.u16Hpw  = stPanelParam->u16HSyncWidth;
    stPubAttr.stSyncInfo.u16Vpw  = stPanelParam->u16VSyncWidth;
    stPubAttr.stSyncInfo.u32FrameRate = stPanelParam->u16DCLK*1000000/(stPanelParam->u16HTotal*stPanelParam->u16VTotal);
    stPubAttr.eIntfSync  = E_MI_DISP_OUTPUT_USER;
    stPubAttr.eIntfType  = E_MI_DISP_INTF_LCD;
    stPubAttr.u32BgColor = YUYV_BLACK;
    MI_DISP_SetPubAttr(dispDev, &stPubAttr);
    MI_DISP_Enable(dispDev);

	stLayerAttr.stVidLayerSize.u16Width     = width;
	stLayerAttr.stVidLayerSize.u16Height    = height;
	stLayerAttr.ePixFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
	stLayerAttr.stVidLayerDispWin.u16X      = 0;
	stLayerAttr.stVidLayerDispWin.u16Y      = 0;
	stLayerAttr.stVidLayerDispWin.u16Width  = width;
	stLayerAttr.stVidLayerDispWin.u16Height = height;
	MI_DISP_SetVideoLayerAttr(dispLayer, &stLayerAttr);
	MI_DISP_BindVideoLayer(dispLayer, dispDev);
	MI_DISP_EnableVideoLayer(dispLayer);

	memset(&stInputPortAttr, 0, sizeof(MI_DISP_InputPortAttr_t));
	MI_DISP_GetInputPortAttr(dispLayer, u32InputPort, &stInputPortAttr);
    stInputPortAttr.stDispWin.u16X      = 0;
    stInputPortAttr.stDispWin.u16Y      = 0;
    stInputPortAttr.stDispWin.u16Width  = width;
    stInputPortAttr.stDispWin.u16Height = height;
	stInputPortAttr.u16SrcWidth         = width;
    stInputPortAttr.u16SrcHeight        = height;

    MI_DISP_SetInputPortAttr(dispLayer, u32InputPort, &stInputPortAttr);
    MI_DISP_EnableInputPort(dispLayer, u32InputPort);
    MI_DISP_SetInputPortSyncMode(dispLayer, u32InputPort, E_MI_DISP_SYNC_MODE_FREE_RUN);

	MI_PANEL_Init(stPanelParam->eLinkType);
    MI_PANEL_SetPanelParam(stPanelParam);
	MI_GFX_Open();

	return 0;
}

void DestroyPlayerDev2(void)
{
	MI_DISP_DEV dispDev = DISP_DEV;
    MI_DISP_LAYER dispLayer = DISP_LAYER;
    MI_U32 u32InputPort = DISP_INPUTPORT;

	MI_GFX_Close();
    MI_PANEL_DeInit();

	MI_DISP_UnBindVideoLayer(dispLayer, dispDev);
	MI_DISP_DisableVideoLayer(dispLayer);
	MI_DISP_DisableInputPort(dispLayer, u32InputPort);
	MI_DISP_Disable(dispDev);

	//sstar_sys_deinit();
	MI_SYS_Exit();
}



MI_S32 StartPlayVideo()
{
    MI_DISP_ShowInputPort(DISP_LAYER, DISP_INPUTPORT);
	return 0;
}

void StopPlayVideo()
{
    MI_DISP_HideInputPort(DISP_LAYER, DISP_INPUTPORT);
}

MI_S32 StartPlayAudio()
{
    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_DEV AoDevId = AUDIO_DEV;
    MI_AO_CHN AoChn = AUDIO_CHN;

    MI_S32 s32SetVolumeDb;
    MI_S32 s32GetVolumeDb;

    //set Ao Attr struct
    memset(&stSetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = AUDIO_SAMPLE_PER_FRAME;
    stSetAttr.u32ChnCnt = 1;

    if(stSetAttr.u32ChnCnt == 2)
    {
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;
    }
    else if(stSetAttr.u32ChnCnt == 1)
    {
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;
    }

    stSetAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_48000;

    /* set ao public attr*/
    MI_AO_SetPubAttr(AoDevId, &stSetAttr);

    /* get ao device*/
    MI_AO_GetPubAttr(AoDevId, &stGetAttr);

    /* enable ao device */
    MI_AO_Enable(AoDevId);

    /* enable ao channel of device*/
    MI_AO_EnableChn(AoDevId, AoChn);


    /* if test AO Volume */
    s32SetVolumeDb = g_s32VolValue * (MAX_ADJUST_AO_VOLUME-MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
    MI_AO_SetVolume(AoDevId, s32SetVolumeDb);
    MI_AO_SetMute(AoDevId, g_bMute);
    /* get AO volume */
    MI_AO_GetVolume(AoDevId, &s32GetVolumeDb);

	return 0;
}

void StopPlayAudio()
{
    MI_AUDIO_DEV AoDevId = AUDIO_DEV;
    MI_AO_CHN AoChn = AUDIO_CHN;

    /* disable ao channel of */
    MI_AO_DisableChn(AoDevId, AoChn);

    /* disable ao device */
    MI_AO_Disable(AoDevId);
}

static void ResetSpeedMode()
{
    g_ePlayDirection = E_PLAY_FORWARD;
    g_ePlayMode = E_PLAY_NORMAL_MODE;
    g_eSpeedMode = E_NORMAL_SPEED;
    g_u32SpeedNumerator = 1;
    g_u32SpeedDenomonator = 1;
}

// reset divp input buffer size
MI_S32 ResetDivpInputSize(MI_S32 s32Width, MI_S32 s32Height)
{
	MI_DIVP_ChnAttr_t stChnAttr;

	g_videoStreamWidth = s32Width;
	g_videoStreamHeight = s32Height;
//	memset(&stChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
//	MI_DIVP_StopChn(DIVP_CHN);
//	MI_DIVP_GetChnAttr(DIVP_CHN, &stChnAttr);
//	stChnAttr.u32MaxWidth = s32Width;
//	stChnAttr.u32MaxHeight = s32Height;
//	MI_DIVP_SetChnAttr(DIVP_CHN, &stChnAttr);
//	MI_DIVP_StartChn(DIVP_CHN);
	return 0;
}

// duration, format, width, height, I-frame/P-frame, etc.
MI_S32 GetMediaInfo()
{
    return 0;
}

// current pos(play time) s
MI_S32 GetCurrentPlayPos(MI_S32 s32Duration, MI_S32 s32CurrentPos)
{
    char curTime[32];
    char totalTime[32];
    int trackPos = 0;

    // update playtime static
    memset(curTime, 0, sizeof(curTime));
    memset(totalTime, 0, sizeof(totalTime));
    sprintf(curTime, "%02d:%02d:%02d", s32CurrentPos/3600, (s32CurrentPos%3600)/60, s32CurrentPos%60);
    sprintf(totalTime, "%02d:%02d:%02d", s32Duration/3600, (s32Duration%3600)/60, s32Duration%60);

    mTextview_curtimePtr->setText(curTime);
	mTextview_durationPtr->setText(totalTime);

    // update progress bar
    trackPos = (s32CurrentPos * mSeekbar_progressPtr->getMax()) / s32Duration;
    mSeekbar_progressPtr->setProgress(trackPos);

    return 0;
}

// MI display video
MI_S32 DisplayVideo(MI_S32 s32DispWidth, MI_S32 s32DispHeight, void *pYData, void *pUVData)
{
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_ChnPort_t pstSysChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;

    pstSysChnPort.eModId = E_MI_MODULE_ID_DIVP;
    pstSysChnPort.u32ChnId = 0;
    pstSysChnPort.u32DevId = 0;
    pstSysChnPort.u32PortId = 0;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u64TargetPts = 0;
    stBufConf.stFrameCfg.u16Width = s32DispWidth;
    stBufConf.stFrameCfg.u16Height = s32DispHeight;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;

    if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&pstSysChnPort,&stBufConf,&stBufInfo,&hHandle, -1))
    {
        stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
        stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
        stBufInfo.bEndOfStream = FALSE;

        memcpy(stBufInfo.stFrameData.pVirAddr[0], pYData, s32DispWidth*s32DispHeight);
        memcpy(stBufInfo.stFrameData.pVirAddr[1], pUVData, s32DispWidth*s32DispHeight/2);

        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
    }

    return 0;
}

MI_S32 DisplayVideo2(MI_S32 s32DispWidth, MI_S32 s32DispHeight, void *pYData, void *pUVData)
{
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_ChnPort_t pstSysChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;

    pstSysChnPort.eModId = E_MI_MODULE_ID_DISP;
    pstSysChnPort.u32ChnId = 0;
    pstSysChnPort.u32DevId = 0;
    pstSysChnPort.u32PortId = 0;

    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u64TargetPts = 0;
    stBufConf.stFrameCfg.u16Width = s32DispWidth;
    stBufConf.stFrameCfg.u16Height = s32DispHeight;
    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;

    if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&pstSysChnPort,&stBufConf,&stBufInfo,&hHandle, -1))
    {
        stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
        stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
        stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
        stBufInfo.bEndOfStream = FALSE;

        memcpy(stBufInfo.stFrameData.pVirAddr[0], pYData, s32DispWidth*s32DispHeight);
        memcpy(stBufInfo.stFrameData.pVirAddr[1], pUVData, s32DispWidth*s32DispHeight/2);

        MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
    }

    return 0;
}

// MI play audio
MI_S32 PlayAudio(MI_U8 *pu8AudioData, MI_U32 u32DataLen)
{
    MI_AUDIO_Frame_t stAoSendFrame;
    MI_S32 s32RetSendStatus = 0;
    MI_AUDIO_DEV AoDevId = AUDIO_DEV;
    MI_AO_CHN AoChn = AUDIO_CHN;

    //read data and send to AO module
    stAoSendFrame.u32Len = u32DataLen;
    stAoSendFrame.apVirAddr[0] = pu8AudioData;
    stAoSendFrame.apVirAddr[1] = NULL;

    //printf("PlayAudio\n");
    do{
        s32RetSendStatus = MI_AO_SendFrame(AoDevId, AoChn, &stAoSendFrame, 128);
    }while(s32RetSendStatus == MI_AO_ERR_NOBUF);

    if(s32RetSendStatus != MI_SUCCESS)
    {
        printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n",s32RetSendStatus);
    }

    return 0;
}

// pause audio
MI_S32 PauseAudio()
{
    MI_AO_PauseChn(AUDIO_DEV, AUDIO_CHN);
    return 0;
}

// resume audio
MI_S32 ResumeAudio()
{
    MI_AO_ResumeChn(AUDIO_DEV, AUDIO_CHN);
    return 0;
}

// stay in playing page, clear play status
MI_S32 PlayComplete()
{
    g_bPlaying = false;
    g_bPause = false;
    player_deinit(g_pstPlayStat);
    StopPlayAudio();
    StopPlayVideo();
    ResetSpeedMode();

    SetPlayingStatus(false);
	mTextview_speedPtr->setText("");
	g_bShowPlayToolBar = FALSE;

	EASYUICONTEXT->goBack();
    return 0;
}

// stay in playing page , clear play status,
MI_S32 PlayError()
{
    // stop play video & audio
    g_bPlaying = FALSE;
    g_bPause = FALSE;
	printf("error in playing!\n");
    // sendmessage to stop playing
    player_deinit(g_pstPlayStat);
    StopPlayAudio();
    StopPlayVideo();
    ResetSpeedMode();

    SetPlayingStatus(false);
	mTextview_speedPtr->setText("");
	g_bShowPlayToolBar = FALSE;

	EASYUICONTEXT->goBack();

    return 0;
}

static void SetPlayerControlCallBack(player_stat_t *is)
{
	is->playerController.fpGetMediaInfo = GetMediaInfo;
	is->playerController.fpGetCurrentPlayPos = GetCurrentPlayPos;
	is->playerController.fpDisplayVideo = DisplayVideo;
	is->playerController.fpPlayAudio = PlayAudio;
	is->playerController.fpPauseAudio = PauseAudio;
	is->playerController.fpResumeAudio = ResumeAudio;
	is->playerController.fpPlayComplete = PlayComplete;
	is->playerController.fpPlayError = PlayError;
}



/**
 * 注册定时器
 * 填充数组用于注册定时器
 * 注意：id不能重复
 */
static S_ACTIVITY_TIMEER REGISTER_ACTIVITY_TIMER_TAB[] = {
	//{0,  6000}, //定时器id=0, 时间间隔6秒
	//{1,  1000},
};

/**
 * 当界面构造时触发
 */
static void onUI_init(){
    //Tips :添加 UI初始化的显示代码到这里,如:mText1Ptr->setText("123");
	printf("create player dev\n");

	// init play view real size
	LayoutPosition layoutPos = mVideoview_videoPtr->getPosition();
	g_playViewWidth = layoutPos.mWidth * PANEL_MAX_WIDTH / UI_MAX_WIDTH;
	g_playViewHeight = ALIGN_DOWN(layoutPos.mHeight * PANEL_MAX_HEIGHT / UI_MAX_HEIGHT, 2);
	printf("play view size: w=%d, h=%d\n", g_playViewWidth, g_playViewHeight);

	// divp use window max width & height default, when play file, the inputAttr of divp will be set refer to file size.
	CreatePlayerDev();
	//CreatePlayerDev2(&stPanelParam, g_playViewWidth, g_playViewHeight);
}

/**
 * 当切换到该界面时触发
 */
static void onUI_intent(const Intent *intentPtr) {
    if (intentPtr != NULL) {
//#ifdef SUPPORT_PLAYER_MODULE
    	char curTime[] = "--:--:--";
    	char totalTime[] = "--:--:--";
    	fileName = intentPtr->getExtra("filepath");
    	// init player
    	ResetSpeedMode();
    	StartPlayVideo();
    	StartPlayAudio();

    	g_pstPlayStat = player_init(fileName.c_str());
		if (!g_pstPlayStat)
		{
			StopPlayAudio();
			StopPlayVideo();
			printf("Initilize player failed!\n");
			return;
		}

		printf("video file name is : %s\n", g_pstPlayStat->filename);

		// sendmessage to play file
		g_bPlaying = TRUE;
		g_bPause   = FALSE;

		SetPlayerControlCallBack(g_pstPlayStat);
		printf("open_demux\n");
		open_demux(g_pstPlayStat);
		printf("open_video\n");
		open_video(g_pstPlayStat);
		printf("open_audio\n");
		open_audio(g_pstPlayStat);
		SetPlayingStatus(true);
		SetPlayerVolumn(20);

		if (g_pstPlayStat->p_fmt_ctx->duration > 0)
		{
			unsigned int second = g_pstPlayStat->p_fmt_ctx->duration / AV_TIME_BASE;
			if (second / 3600 > 99)
			{
				printf("file size is limited\n");
				return;
			}

			sprintf(totalTime, "%02d:%02d:%02d", (second/3600), ((second%3600) / 60), (second % 60));
		}

		mTextview_curtimePtr->setText(curTime);
		mTextview_durationPtr->setText(totalTime);

//#endif
    }
}

/*
 * 当界面显示时触发
 */
static void onUI_show() {

}

/*
 * 当界面隐藏时触发
 */
static void onUI_hide() {

}

/*
 * 当界面完全退出时触发
 */
static void onUI_quit() {
	printf("destroy player dev\n");
	DestroyPlayerDev();
	//DestroyPlayerDev2();
}

/**
 * 串口数据回调接口
 */
static void onProtocolDataUpdate(const SProtocolData &data) {

}

/**
 * 定时器触发函数
 * 不建议在此函数中写耗时操作，否则将影响UI刷新
 * 参数： id
 *         当前所触发定时器的id，与注册时的id相同
 * 返回值: true
 *             继续运行当前定时器
 *         false
 *             停止运行当前定时器
 */
static bool onUI_Timer(int id){
	switch (id) {

		default:
			break;
	}
    return true;
}

/**
 * 有新的触摸事件时触发
 * 参数：ev
 *         新的触摸事件
 * 返回值：true
 *            表示该触摸事件在此被拦截，系统不再将此触摸事件传递到控件上
 *         false
 *            触摸事件将继续传递到控件上
 */
static bool onplayerActivityTouchEvent(const MotionEvent &ev) {
    switch (ev.mActionStatus) {
		case MotionEvent::E_ACTION_DOWN://触摸按下
			//LOGD("时刻 = %ld 坐标  x = %d, y = %d", ev.mEventTime, ev.mX, ev.mY);
			break;
		case MotionEvent::E_ACTION_MOVE://触摸滑动
			break;
		case MotionEvent::E_ACTION_UP:  //触摸抬起
			break;
		default:
			break;
	}
	return false;
}
static void onProgressChanged_Seekbar_progress(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged Seekbar_progress %d !!!\n", progress);

	// do seek stuff
}

static bool onButtonClick_sys_back(ZKButton *pButton) {
    //LOGD(" ButtonClick sys_back !!!\n");
    return false;
}
static bool onButtonClick_Button_play(ZKButton *pButton) {
    //LOGD(" ButtonClick Button_play !!!\n");
//	if (g_bPlaying)
//	{
//		g_bPause = !g_bPause;
//		// sendmessage to pause/resume playing
//		toggle_pause(g_pstPlayStat);
//	}
//	else
//	{
//		g_bPlaying = TRUE;
//		g_bPause = FALSE;
//
//		// sendmessage to start playing
//		StartPlayVideo();
//		StartPlayAudio();
//
//		printf("init player, file is %s\n", fileName.c_str());
//
//		g_pstPlayStat = player_init(fileName.c_str());
//		if (!g_pstPlayStat)
//		{
//			StopPlayAudio();
//			StopPlayVideo();
//			return;
//		}
//
//		printf("open_demux\n");
//		open_demux(g_pstPlayStat);
//		printf("open_video\n");
//		open_video(g_pstPlayStat);
//		printf("open_audio\n");
//		open_audio(g_pstPlayStat);
//	}

	g_bPause = !g_bPause;
	toggle_pause(g_pstPlayStat);
	SetPlayingStatus(!g_bPause);

    return false;
}

static bool onButtonClick_Button_stop(ZKButton *pButton) {
    //LOGD(" ButtonClick Button_stop !!!\n");
	g_bPlaying = FALSE;
	g_bPause = FALSE;

	// sendmessage to stop playing
	player_deinit(g_pstPlayStat);
	StopPlayAudio();
	StopPlayVideo();

	SetPlayingStatus(false);
	ResetSpeedMode();
	mTextview_speedPtr->setText("");
	g_bShowPlayToolBar = FALSE;

	EASYUICONTEXT->goBack();
    return false;
}

static bool onButtonClick_Button_slow(ZKButton *pButton) {
    //LOGD(" ButtonClick Button_slow !!!\n");
	char speedMode[16] = {0};

	if (g_bPlaying)
	{
		if (g_ePlayDirection == E_PLAY_FORWARD)
		{
			// slow down
			if (g_ePlayMode == E_PLAY_FAST_MODE)
			{
				g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
				g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
				g_u32SpeedDenomonator = 1;

				if (g_eSpeedMode == E_NORMAL_SPEED)
					g_ePlayMode = E_PLAY_NORMAL_MODE;
			}
			else
			{
				if (g_eSpeedMode < E_32X_SPEED)
				{
					g_ePlayMode = E_PLAY_SLOW_MODE;
					g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
				}
				else    // turn to play backward
				{
					g_ePlayDirection = E_PLAY_BACKWARD;
					g_ePlayMode = E_PLAY_NORMAL_MODE;
					g_eSpeedMode = E_NORMAL_SPEED;
				}

				g_u32SpeedNumerator = 1;
				g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;
			}
		}
		else
		{
			// speed up
			if (g_ePlayMode == E_PLAY_SLOW_MODE)
			{
				g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
				g_u32SpeedNumerator = 1;
				g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;

				if (g_eSpeedMode == E_NORMAL_SPEED)
				{
					g_ePlayMode = E_PLAY_NORMAL_MODE;
				}
			}
			else
			{
				if (g_eSpeedMode < E_32X_SPEED)
				{
					g_ePlayMode = E_PLAY_FAST_MODE;
					g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
					g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
					g_u32SpeedDenomonator = 1;
				}
			}
		}

		memset(speedMode, 0, sizeof(speedMode));
		if (g_u32SpeedNumerator == g_u32SpeedDenomonator)
			sprintf(speedMode, "", g_u32SpeedNumerator);
		else if (g_u32SpeedNumerator > g_u32SpeedDenomonator)
			sprintf(speedMode, "%s %dX", ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"), g_u32SpeedNumerator);
		else
			sprintf(speedMode, "%s 1/%dX", ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"), g_u32SpeedDenomonator);

		mTextview_speedPtr->setText(speedMode);

		// sendmessage to adjust speed
	}

    return false;
}

static bool onButtonClick_Button_fast(ZKButton *pButton) {
    //LOGD(" ButtonClick Button_fast !!!\n");
	char speedMode[16] = {0};

	if (g_bPlaying)
	{
		if (g_ePlayDirection == E_PLAY_FORWARD)
		{
			// speed up
			if (g_ePlayMode == E_PLAY_SLOW_MODE)
			{
				g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
				g_u32SpeedNumerator = 1;
				g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;

				if (g_eSpeedMode == E_NORMAL_SPEED)
					g_ePlayMode = E_PLAY_NORMAL_MODE;
			}
			else
			{
				if (g_eSpeedMode < E_32X_SPEED)
				{
					g_ePlayMode = E_PLAY_FAST_MODE;
					g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
					g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
					g_u32SpeedDenomonator = 1;
				}
			}
		}
		else
		{
			// slow down
			if (g_ePlayMode == E_PLAY_FAST_MODE)
			{
				g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode - 1);
				g_u32SpeedNumerator = 1 << (int)g_eSpeedMode;
				g_u32SpeedDenomonator = 1;

				if (g_eSpeedMode == E_NORMAL_SPEED)
					g_ePlayMode = E_PLAY_NORMAL_MODE;
			}
			else
			{
				// 1/32X speed backward to normal speed forward
				if (g_eSpeedMode == E_32X_SPEED)
				{
					g_eSpeedMode = E_NORMAL_SPEED;
					g_ePlayMode = E_PLAY_NORMAL_MODE;
					g_ePlayDirection = E_PLAY_FORWARD;
				}
				else
				{
					g_ePlayMode = E_PLAY_SLOW_MODE;
					g_eSpeedMode = (PlaySpeedMode_e)((int)g_eSpeedMode + 1);
				}

				g_u32SpeedNumerator = 1;
				g_u32SpeedDenomonator = 1 << (int)g_eSpeedMode;
			}
		}

		memset(speedMode, 0, sizeof(speedMode));
		if (g_u32SpeedNumerator == g_u32SpeedDenomonator)
			memset(speedMode, 0, sizeof(speedMode));
		else if (g_u32SpeedNumerator > g_u32SpeedDenomonator)
			sprintf(speedMode, "%dX %s", g_u32SpeedNumerator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));
		else
			sprintf(speedMode, "1/%dX %s", g_u32SpeedDenomonator, ((g_ePlayDirection == E_PLAY_FORWARD) ? ">>" : "<<"));

		mTextview_speedPtr->setText(speedMode);

		// sendmessage to adjust speed
	}

    return false;
}
static bool onButtonClick_Button_voice(ZKButton *pButton) {
    //LOGD(" ButtonClick Button_voice !!!\n");
//	MI_S32 vol = 0;
//
//	if (g_s32VolValue)
//		vol = g_s32VolValue * (MAX_ADJUST_AO_VOLUME - MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
//	else
//		vol = MIN_AO_VOLUME;
//	g_bMute = !g_bMute;
//
//	MI_AO_SetVolume(AUDIO_DEV, vol);
//	MI_AO_SetMute(AUDIO_DEV, g_bMute);
//
//	SetMuteStatus(g_bMute);
//	SetPlayerVolumn(g_s32VolValue);

	g_bMute = !g_bMute;
	MI_AO_SetMute(AUDIO_DEV, g_bMute);
	SetMuteStatus(g_bMute);
	printf("set mute to %d\n", g_bMute);

    return false;
}

static void onProgressChanged_Seekbar_volumn(ZKSeekBar *pSeekBar, int progress) {
    //LOGD(" ProgressChanged Seekbar_volumn %d !!!\n", progress);
	MI_S32 vol = 0;
	MI_AO_ChnState_t stAoState;

	printf("voice changed!!!!!!!!\n");
	g_s32VolValue = GetPlayerVolumn();
	if (g_s32VolValue)
		vol = g_s32VolValue * (MAX_ADJUST_AO_VOLUME - MIN_ADJUST_AO_VOLUME) / 100 + MIN_ADJUST_AO_VOLUME;
	else
		vol = MIN_AO_VOLUME;

	memset(&stAoState, 0, sizeof(MI_AO_ChnState_t));
	if (MI_SUCCESS == MI_AO_QueryChnStat(AUDIO_DEV, AUDIO_CHN, &stAoState))
	{
		MI_AO_SetVolume(AUDIO_DEV, vol);
		MI_AO_SetMute(AUDIO_DEV, g_bMute);
	}
}
