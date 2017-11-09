//------------------------------------------------------------------------------
// Implement Windows Platform Services
//------------------------------------------------------------------------------

// PlatformMemory Win32PlatformAllocateMemoryCallback(u64 size, u8 alignment, u64 preferred)
PLATFORM_ALLOCATE_MEMORY(win32_allocate_memory)
{
	pt_Memory mem;
	u64 stride = (1ull << alignment);
	u64 aligned_size = size + stride - 1;
	mem.handle = VirtualAlloc((LPVOID) preferred, aligned_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	Assert(mem.handle && "Could not allocate enough memory");

	mem.memblock.begin = (void*) RALIGN((u64) mem.handle, stride);
	mem.memblock.end   = mem.memblock.begin + size;

	return mem;
};

// void Win32PlatformFreeMemoryCallback(PlatformMemory *pm)
PLATFORM_FREE_MEMORY(win32_free_memory)
{
	VirtualFree((LPVOID*) pm->handle, 0, MEM_RELEASE);
}

// PlatformFileHandle name(const char* file_begin, const char* file_end)
PLATFORM_OPEN_READ_FILE(win32_open_read_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = 0;
	fopen_s(&fp, name, "rb");

	free(name);

	pt_File pfh;
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		pfh.size = _ftelli64(fp);
		fseek(fp, 0L, SEEK_SET);
	}
	else {
		pfh.size = 0;
	}
	pfh.open = fp ? 1 : 0;
	pfh.eof  = 0;
	pfh.last_read = 0;
	pfh.last_seek_success = 0;
	pfh.handle = fp;
	pfh.read = 1;
	pfh.write = 0;
	return pfh;
}

// PlatformFileHandle name(const char* file)
PLATFORM_OPEN_WRITE_FILE(win32_open_write_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = 0;
	fopen_s(&fp, name, "wb");

	free(name);

	pt_File pfh;
	pfh.open = fp ? 1 : 0;
	pfh.eof  = 0;
	pfh.last_read = 0;
	pfh.last_seek_success = 0;
	pfh.handle = fp;
	pfh.read = 0;
	pfh.write = 1;
	pfh.size = 0;
	return pfh;
}

// void name(PlatformFileHandle *pfh, char *buffer_begin, char* buffer_end)
PLATFORM_READ_NEXT_FILE_CHUNK(win32_read_next_file_chunk)
{
	Assert(pfh->open && !pfh->eof && pfh->read);
	FILE* fp = (FILE*) pfh->handle;
	u64 buflen = buffer_end - buffer_begin;
	pfh->last_read = fread(buffer_begin, 1, buflen, fp);
	if (pfh->last_read < buflen) {
		// if read less than buffer size expect eof flag is on
		// and no error flag is set
		Assert(feof(fp) && !ferror(fp) && "FileTokenizer: error reading");
		pfh->eof = 1;
	}
}

// void name(PlatformFileHandle *pfh, u64 offset)
PLATFORM_SEEK_FILE(win32_seek_file)
{
	Assert(pfh->open && pfh->read);
	FILE* fp = (FILE*) pfh->handle;
	int err = _fseeki64(fp, (s64) offset, SEEK_SET);
	pfh->last_seek_success = !err;
}

// void name(PlatformFileHandle *pfh)
PLATFORM_CLOSE_FILE(win32_close_file)
{
	Assert(pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	fclose(fp);
	pfh->open = 0;
}

// void name(PlatformFileHandle *pfh, char *begin, char* end)
PLATFORM_WRITE_TO_FILE(win32_write_to_file)
{
	Assert(pfh->write && pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	size_t size = (size_t) (end-begin);
	size_t written = fwrite(begin, 1, size, fp);

	/* @TODO(llins): replace with windows functions */
// 	DWORD BytesWritten;
// 	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);

	Assert(written == size);
}


// void name(FilePath *fp)
PLATFORM_EXECUTABLE_PATH(win32_executable_path)
{
	DWORD exe_full_path_size = GetModuleFileNameA(0, fp->full_path,
						      MAX_FILE_PATH_SIZE);
	fp->end  = fp->full_path + exe_full_path_size;
	fp->name = fp->full_path;
	for(char *it=fp->full_path;*it;++it)
	{ if(*it == '\\')
		{
			fp->name = it + 1;
		}
	}
}

static f64 win32_frequency = 0.0;

PLATFORM_GET_TIME(win32_get_time)
{
	LARGE_INTEGER time_point;
	QueryPerformanceCounter(&time_point);
	return (f64) time_point.QuadPart / win32_frequency;
}


/* define NANOCUBE_HTTP to enable the mongoose server code */

#ifdef NANOCUBE_HTTP
#include "mongoose_nanocube_platform.c"
#endif

internal void
win32_init_platform(PlatformAPI* p)
{
	{
		LARGE_INTEGER t;
		QueryPerformanceFrequency(&t);
		win32_frequency = (f64) t.QuadPart;
	}

	p->allocate_memory      = win32_allocate_memory;
	p->free_memory          = win32_free_memory;
	p->open_write_file      = win32_open_write_file;
	p->open_read_file       = win32_open_read_file;
	p->read_next_file_chunk = win32_read_next_file_chunk;
	p->seek_file            = win32_seek_file;
	p->close_file           = win32_close_file;
	p->write_to_file        = win32_write_to_file;
	p->executable_path      = win32_executable_path;
	p->get_time             = win32_get_time;
#ifdef NANOCUBE_HTTP
	p->server_start         = mongoose_server_start;
	p->server_respond       = mongoose_server_respond;
#else
	p->server_start         = 0;
	p->server_respond       = 0;
#endif
}
