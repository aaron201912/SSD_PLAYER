#ifndef __AUDIODETECT_H__
#define __AUDIODETECT_H__
#include "base_types.h"
#include "DSpotterApi.h"

#ifdef __cplusplus
extern "C" {
#endif


#define COMMAND_LEN					40

typedef struct
{
	char cmd[COMMAND_LEN];
} TrainedWord_t;

typedef struct DSpotter_LibInfo_s
{
    void *pDSpotterLibHandle;
    DSPDLL_API HANDLE (*DSpotterInitMultiWithMod)(char *lpchCYBaseFile, char *lppchGroupFile[], INT nNumGroupFile, INT nMaxTime, BYTE *lpbyState, INT nStateSize, INT *lpnErr, char *lpchLicenseFile);
    DSPDLL_API INT (*DSpotterRelease)(HANDLE hDSpotter);
    DSPDLL_API INT (*DSpotterReset)(HANDLE hDSpotter);
    DSPDLL_API INT (*DSpotterAddSample)(HANDLE hDSpotter, SHORT *lpsSample, INT nNumSample);
    DSPDLL_API INT (*DSpotterGetResult)(HANDLE hDSpotter);
} DSpotter_LibInfo_t;

typedef void* (*VoiceAnalyzeCallback)(int);

int SSTAR_VoiceDetectGetWordList(TrainedWord_t *pWordList, int nWordCnt);
HANDLE SSTAR_VoiceDetectInit();
int SSTAR_VoiceDetectDeinit(HANDLE hDSpotter);
int SSTAR_VoiceDetectStart(HANDLE hDSpotter, VoiceAnalyzeCallback pfnCallback);
void SSTAR_VoiceDetectStop();

#ifdef __cplusplus
}
#endif

#endif
