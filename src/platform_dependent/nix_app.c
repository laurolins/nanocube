#include "../base/platform.c"

#include "../base/cstr.c"
#include "../base/arena.c"
#include "../base/print.c"

#ifdef POLYCOVER
#include "../polycover/polycover.h"
#endif

#include "../app.h"

#include "nix_platform.c"
#include <signal.h>

#if defined(OS_MAC)
#include <mach-o/dyld.h>
#endif

//
// @revise: the platform layer is outdated, should bring in the newer version
//
// static Print*
// print_new(u64 request_size)
// {
// 	u64 size = RALIGN(request_size, 4096);
// 	char *buffer = (char*) malloc(size);
// 	Assert(buffer);
// 	Print *print = (Print*) buffer;
// 	print_init(print, buffer + sizeof(Print), buffer + size);
// 	return print;
// }
// 
// static void
// print_delete(Print *print)
// {
// 	free(print);
// }

//
// include the whole infra-structure (whichi is more or less platform independent)
//
#include "../app.c"

global_variable ApplicationState app_state;

void exit_callback(void)
{
	//gs_nanodb.deregister_nanocube(exit_status);
}

void signal_handler(int signum)
{
	if (signum == SIGINT) {
		app_state.interrupted = 1;
		nix_app_state_interrupted = 1;
	} else {
		exit(signum);
	}
}

void setup_signal_handlers()
{
	// If SIGHUP was being ignored, keep it that way...
	void (*previous_signal_handler)(int);

	previous_signal_handler = signal(SIGHUP, signal_handler);
	if (previous_signal_handler == SIG_IGN) {
		signal(SIGHUP, SIG_IGN);
	}
	signal(SIGINT,  signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGILL,  signal_handler);
	signal(SIGTRAP, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGFPE,  signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGTERM, signal_handler);
	atexit(exit_callback);
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

#ifdef PROFILE
global_variable pf_Table global_profile_table_storage = { .slot_event_index = 0 };
pf_Table *global_profile_table = &global_profile_table_storage;
#endif

#define main_BUFFER_SIZE Megabytes(1)


int
main(int num_args, char** args)
{
	app_state.data        = 0;
	app_state.interrupted = 0;

#ifdef PROFILE
	/* initialize profile table */
	u64 profile_memory_size = Megabytes(128);
	pt_Memory profile_memory = nix_allocate_memory(profile_memory_size, 3, 0);
	pf_Table_init(global_profile_table, profile_memory.memblock.begin, profile_memory.memblock.end);
	app_state.global_profile_table = (void*) global_profile_table;
#endif

	/* signal handlers */
	setup_signal_handlers();

	/* make sure any initial state needed by the linux platform is initializes */
	nix_init();

	/* initialize a PlatformAPI object by lining specific functions */
	nix_init_platform(&app_state.platform);
	platform = app_state.platform;

	// prepare platform file handle
	pt_File platform_stdout = {
		.open = 1,
		.eof = 0,
		.write = 1,
		.read = 0,
		.last_seek_success = 0,
		.last_read = 0,
		.handle = stdout
	};

	// prepare platform file handle
	pt_File platform_stdin = {
		.open = 1,
		.eof = 0,
		.write = 0,
		.read = 1,
		.last_seek_success = 0,
		.last_read = 0,
		.handle = stdin
	};

	pt_File platform_stderr = {
		.open = 1,
		.eof = 0,
		.write = 1,
		.read = 0,
		.last_seek_success = 0,
		.last_read = 0,
		.handle = stderr
	};

	// buffer
	Print *print = print_new_raw(main_BUFFER_SIZE);

#ifdef POLYCOVER

	// @revise do we really need this indirection. maybe keep it simple
	app_state.polycover = (PolycoverAPI) {
		.get_code = polycover_get_code,
		.new_shape = polycover_new_shape,
		.free_shape = polycover_free_shape,
		.get_intersection = polycover_get_intersection,
		.get_symmetric_difference = polycover_get_symmetric_difference,
		.get_difference = polycover_get_difference,
		.get_union = polycover_get_union,
		.get_complement = polycover_get_complement
	};

#endif

	//
	// treat request as a memory block by copying all
	// the arguments into a single buffer space separated
	//
	print_clear(print);
	for (s64 i=1;i<num_args;++i) {
		if (i > 1) print_char(print, ' ');
		print_cstr(print, args[i]);
	}
	// cannot append zero it messes up the options
	// print_char(print,0);

	// application process request is implemented
	application_process_request(&app_state, print->begin, print->end, &platform_stdin, &platform_stdout, &platform_stderr);

#ifdef PROFILE
	nix_free_memory(&profile_memory);
#endif

	print_free_raw(print);

	return 0;

}

