#include "nanocube_platform.c"
#include "nanocube_app.h"

#include <stdlib.h>
#include <stdio.h>

/* winsock2.h needs to be included before windows.h */
#include <winsock2.h>
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

	if (!result.application_code_dll) {
		result.is_valid = 0;
		return result;
	}

	result.application_process_request = (ApplicationProcessRequest*)
		GetProcAddress(result.application_code_dll, "application_process_request");

	if (!result.application_process_request) {
		result.is_valid = 0;
		return result;
	} else {
		result.is_valid = 1;
	}

	return result;
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

int
main(int num_args, char** args)
{
	ApplicationState app_state;
	app_state.data = 0;
	win32_init_platform(&app_state.platform);

	pt_File platform_stdout;
	platform_stdout.open = 1;
	platform_stdout.write = 1;
	platform_stdout.read = 0;
	platform_stdout.last_seek_success = 0;
	platform_stdout.last_read = 0;
	platform_stdout.handle = stderr;

	// buffer
	char buffer[Kilobytes(4)];
	Print print;
	Print_init(&print, buffer, buffer + sizeof(buffer));

	// get .dll full path
	FilePath executable_path, dll_path;
	app_state.platform.executable_path(&executable_path);
	FilePath_copy(&dll_path, &executable_path);
	FilePath_set_name_cstr(&dll_path, "nanocube_app.dll");

	Win32ApplicationCode app_code = win32_load_application_code(dll_path.full_path);

	if (!app_code.is_valid) {
		app_state.platform.write_to_file(&platform_stdout, print.begin, print.end);
		Print_cstr(&print, "\n[Problem] Could not find library: ");
		Print_str(&print, dll_path.full_path, cstr_end(dll_path.full_path));
		Print_cstr(&print, "\n");
		app_state.platform.write_to_file(&platform_stdout, print.begin, print.end);
		return -1;
	}

	//
	// treat request as a memory block by copying all
	// the arguments into a single buffer space separated
	//
	char request[Kilobytes(4)];
	Print p;
	Print_init(&p, request, request + sizeof(request));
	for (s64 i=1;i<num_args;++i) {
		if (i > 1)
			Print_char(&p, ' ');
		Print_cstr(&p, args[i]);
	}

	app_code.application_process_request(&app_state, p.begin, p.end, &platform_stdout);
}



