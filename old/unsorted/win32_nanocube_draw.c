#include "nanocube_platform.h"
#include "nanocube_alloc.h"
#include "nanocube_index.h"
#include "nanocube_draw.h"

#include "nanocube_platform.c"

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "win32_nanocube_platform.c"

//------------------------------------------------------------------------------
// Application Code
//------------------------------------------------------------------------------

typedef struct Win32ApplicationCode {

    HMODULE                    application_code_dll;
    ApplicationProcessRequest* application_process_request;
    b8                         is_valid;

} Win32ApplicationCode; 


internal Win32ApplicationCode 
win32_load_application_code(char *application_dll_path)
{

    Win32ApplicationCode result;
    result.application_code_dll = LoadLibraryA(application_dll_path);
   
    Assert(result.application_code_dll);

    result.application_process_request = (ApplicationProcessRequest*)
        GetProcAddress(result.application_code_dll, "application_process_request");
    
    result.is_valid = result.application_process_request != 0;

    return result;
}


//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

int 
main(int num_args, char** args) 
{
    ApplicationState app_state;
    win32_init_platform(&app_state.platform);
   
    // get .dll full path 
    FilePath executable_path, dll_path;
    app_state.platform.executable_path(&executable_path);
    FilePath_copy(&dll_path, &executable_path);
    FilePath_set_name_cstr(&dll_path, "nanocube_draw.dll");

    Win32ApplicationCode app_code = win32_load_application_code(dll_path.full_path);

    Assert(app_code.is_valid);

    if (num_args < 2) {
        fprintf(stderr,"usage: nanocube_draw <txt>\n");
        return(-1);
    }

    PlatformFileHandle platform_stdout;
    platform_stdout.open = 1;
    platform_stdout.write = 1;
    platform_stdout.read = 0;
    platform_stdout.last_seek_success = 0;
    platform_stdout.last_read = 0;
    platform_stdout.handle = stderr;

    app_code.application_process_request(&app_state, args[1], 0, &platform_stdout);

}

