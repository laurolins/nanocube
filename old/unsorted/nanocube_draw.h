typedef struct ApplicationState 
{
    PlatformAPI    platform;

} ApplicationState;


#define APPLICATION_PROCESS_REQUEST(name) void name(ApplicationState* app_state, char* request_begin, char* request_end, PlatformFileHandle *pfh_stdout)
typedef APPLICATION_PROCESS_REQUEST(ApplicationProcessRequest);


