#ifndef _TP_PLAYER_H_
#define _TP_PLAYER_H_

#ifdef __cplusplus
extern "C" {               // 告诉编译器下列代码要以C链接约定的模式进行链接
#endif

#if defined(SUPPORT_CLOUD_PLAY_MODULE) || defined(SUPPORT_PLAYER_MODULE)
#include "player.h"
#include "mi_divp.h"
#include "mi_divp_datatype.h"

#define MAINWND_W       1024
#define MAINWND_H       600

int tp_player_open(char *fp, uint16_t x, uint16_t y, uint16_t width, uint16_t height, void *parg);
int tp_player_close(void);
int tp_player_loop(player_stat_t *parg);

#endif

#ifdef __cplusplus
}
#endif

#endif

