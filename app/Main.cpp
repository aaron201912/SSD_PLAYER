#include "entry/EasyUIContext.h"

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <sys/wait.h>
#include <sys/resource.h>




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
 
    signal(SIGBUS, signal_crash_handler);	// 总线错误
    signal(SIGSEGV, signal_crash_handler);	// SIGSEGV，非法内存访�?    signal(SIGFPE, signal_crash_handler);	// SIGFPE，数学相关的异常，如�?除，浮点溢出，等�?    signal(SIGABRT, signal_crash_handler);	// SIGABRT，由调用abort函数产生，进程非正常退�?}
}

void child_catch(int signalNumber) 
{

    //子进程状态发生改变时，内核对信号作处理的回调函数
    int w_status;
    pid_t w_pid;
    printf("child_catch: %d\n",signalNumber);
    while ((w_pid = waitpid(-1, &w_status, WNOHANG)) != -1 && w_pid != 0) 
    {
        printf("childpid: %d\n",w_pid);
        if (WIFEXITED(w_status)) //判断子进程是否正常退出
        {
            //打印子进程PID和子进程返回值
            printf("---------------------normal catch pid %d,return value %d\n", w_pid,WEXITSTATUS(w_status));
			system("./customer/browser/run.sh");
        }

        if(WIFSIGNALED(w_status))
        {
            printf("---------------------exception catch pid %d,return value %d\n", w_pid,WTERMSIG(w_status));
            //sstar_disp_Deinit(&stDispPubAttr);
            //sstar_disp_init(&stDispPubAttr);
            //system("./customer/browser/run.sh");
            
        }

        if(WIFSTOPPED(w_status))
        {
            printf("---------------------catch pid %d,return value %d\n", w_pid,WSTOPSIG(w_status));
        }

    }

}


int main(int argc, const char *argv[]) 
{
    struct rlimit limit;
    int resource;
    
    resource = RLIMIT_CORE;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(resource, &limit);

    umask(0);

    pid_t pid;
    
    //在此处阻塞SIGCHLD信号，防止信号处理函数尚未注册成功就有子进程结束
    
    sigset_t child_sigset;
    
    sigemptyset(&child_sigset); //将child_sigset每一位都设置为0
    
    sigaddset(&child_sigset, SIGCHLD); //添加SIGCHLD位
    
    sigprocmask(SIG_BLOCK, &child_sigset, NULL); //完成父进程阻塞SIGCHLD的设置

    //init sdk
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    stDispPubAttr.u32BgColor = YUYV_BLACK;

    sstar_disp_init(&stDispPubAttr);

    int retChdir;
    retChdir = chdir("/");
    if (retChdir)
        printf("change directory to /applications/bin  fail:%s",strerror(errno));
    
    pid = fork();
    if (pid < 0)
    {
        printf("<<%s>> <<%d>> fork failed! pid=%d", __PRETTY_FUNCTION__, __LINE__, pid);
    }
    else if (!pid)
    {
        printf("im child ,my pid is %d\n", getpid());
        //start easy ui
        if (EASYUICONTEXT->initEasyUI()) 
        {
            EASYUICONTEXT->runEasyUI();
            EASYUICONTEXT->deinitEasyUI();
        }
    }
    else
    {
        struct sigaction act; //信号回调函数使用的结构体
        
        act.sa_handler = child_catch;
        
        sigemptyset(&(act.sa_mask)); //设置执行信号回调函数时父进程的的信号屏蔽字
        
        act.sa_flags = 0;
        
        sigaction(SIGCHLD, &act, NULL); //给SIGCHLD注册信号处理函数
        
        //解除SIGCHLD信号的阻塞
        
        sigprocmask(SIG_UNBLOCK, &child_sigset, NULL);
        
        printf("im PARENT ,my pid is %d\n", getpid());
        
        while (1); //父进程堵塞，回收子进程

    }

    //installHandler();

    return 0;
}
