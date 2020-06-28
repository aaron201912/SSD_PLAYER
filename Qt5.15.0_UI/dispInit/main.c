#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "sstardisp.h"


#define MAKE_YUYV_VALUE(y,u,v)  ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK              MAKE_YUYV_VALUE(0,128,128)


static MI_DISP_PubAttr_t stDispPubAttr;

static void server_on_exit() {
    sstar_disp_Deinit(&stDispPubAttr);
}

static void signal_crash_handler(int sig) {
    exit(-1);
}

static void signal_exit_handler(int sig) {
    exit(0);
}

static void installHandler() {
    atexit(server_on_exit);

    signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);

    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    signal(SIGBUS, signal_crash_handler);
    signal(SIGSEGV, signal_crash_handler);
}

void comandInfo()
{
    printf("input 'q' to exit app\n");
}

int main(int argc, const char *argv[]) 
{
	int isExit = 0;
	
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stDispPubAttr.u32BgColor = YUYV_BLACK;

    sstar_disp_init(&stDispPubAttr);

    installHandler();

    while (!isExit)
    {
        char ch = 0;
//        comandInfo();
        ch = getchar();
//        rewind(stdin);

        switch (ch)
        {
            case 'q':
                isExit = 1;
                break;
            default:
                break;
        }
    }

    sstar_disp_Deinit(&stDispPubAttr);

    return 0;
}
#if 0
int main(int argc, const char *argv[])
{
    int isExit = 0;

    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stDispPubAttr.u32BgColor = YUYV_BLACK;
    sstar_disp_init(&stDispPubAttr);

    installHandler();

    printf("do play video stuff\n");

    comandInfo();

    while (!isExit)
    {
        char ch = getchar();

        switch (ch)
        {
            case 'q':
                isExit = 1;
                break;
            default:
                break;
        }

        if (!isExit)
            comandInfo();
    }

    sstar_disp_Deinit(&stDispPubAttr);

    return 0;
}
#endif
