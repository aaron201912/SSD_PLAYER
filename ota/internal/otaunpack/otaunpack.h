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
}OTA_PROCESS;

typedef void (*OtaUnpackNotifyProcess)(const OTA_PROCESS *process, const char *message);


int OtaUnPack(const char* file, const char* bspatch_path, int is_compressed, OtaUnpackNotifyProcess process_func);

#ifdef __cplusplus
}
#endif	// __cplusplus
#endif //__OTAUNPACK__
