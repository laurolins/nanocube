/* platform services implemented in the same way in both linux and osx */

PLATFORM_ALLOCATE_MEMORY(osx_linux_allocate_memory)
{
	pt_Memory mem;
	u64 stride = (1ull << alignment);
	u64 aligned_size = size + stride - 1;
	mem.handle = malloc(aligned_size);

	Assert(mem.handle && "Could not allocate enough memory");

	mem.memblock.begin = (void*) RALIGN((u64) mem.handle, stride);
	mem.memblock.end   = mem.memblock.begin + size;

	return mem;
};

/* b8 name(pt_Memory* mem, u64 new_size, u8 alignment) */
PLATFORM_RESIZE_MEMORY(osx_linux_resize_memory)
{
	u64 size   = mem->memblock.end - mem->memblock.begin;
	s64 stride = (1ull << alignment);
	s64 offset = mem->memblock.begin - (char*) mem->handle;
	u64 aligned_new_size = new_size + stride - 1;
	void *new_handle = realloc(mem->handle, aligned_new_size);
	if (!new_handle) {
		return 0;
	}
	if (alignment) {
		char *it_src = (char*) new_handle + offset;
		char *it_dst = (char*) RALIGN((u64) new_handle, stride);
		s64 delta = it_dst - it_src;
		s64 bytes_to_copy = MIN(size,new_size);
		Assert(bytes_to_copy);
		if (delta < 0) {
			char *it_dst_end = it_dst + bytes_to_copy;
			while (it_dst != it_dst_end) {
				*it_dst = *it_src;
				++it_src;
				++it_dst;
			}
		} else if (delta > 0) {
			char *it_dst_rev = it_dst + bytes_to_copy - 1;
			char *it_dst_rev_end = it_dst - 1;
			char *it_src_rev = it_src + bytes_to_copy - 1;
			while (it_dst_rev != it_dst_rev_end) {
				*it_dst_rev = *it_src_rev;
				--it_dst_rev;
				--it_src_rev;
			}
		}
		mem->handle         = new_handle;
		mem->memblock.begin = it_dst;
		mem->memblock.end   = mem->memblock.begin + new_size;
		Assert(((u64) it_dst % stride) == 0);
	} else {
		/* replace block into mem struct */
		mem->handle = new_handle;
		mem->memblock.begin = (char*) new_handle + offset;
		mem->memblock.end   = mem->memblock.begin + new_size;
	}
	return 1;
};

PLATFORM_FREE_MEMORY(osx_linux_free_memory)
{
	/* should be robust to null pointer */
	free(pm->handle);
}

PLATFORM_COPY_MEMORY(osx_linux_copy_memory)
{
	memcpy(dest, src, count);
}

PLATFORM_OPEN_MMAP_FILE(osx_linux_open_mmap_file)
{
	struct stat   file_stat;

	char          file_name[1024];
	Assert(file_end - file_begin + 1 < 1024);
	u64 len = pt_copy_bytes(file_begin, file_end, file_name, file_name + 1023);
	file_name[len] = 0;

	pt_MappedFile mapped_file;
	pt_fill((char*) &mapped_file, (char*) &mapped_file + sizeof(pt_MappedFile), 0);


	if (read && !write) {

		s32 file_descriptor = open(file_name, O_RDONLY);

		if (file_descriptor == -1) {
			return mapped_file;
		}

		if (fstat(file_descriptor, &file_stat) == -1) {
			return mapped_file;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			return mapped_file;
		}

		mapped_file.size  = file_stat.st_size;

		/* map file in a shared fashion for reading */
		mapped_file.handle = mmap (0, file_stat.st_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
		if (mapped_file.handle == MAP_FAILED) {
			return mapped_file;
		}

		if (close(file_descriptor) == -1) {
			return mapped_file;
		}

		mapped_file.read   = 1;
		mapped_file.mapped = 1;
		mapped_file.begin  = (char*) mapped_file.handle;

	} else if (read && write) {

		s32 file_descriptor = open(file_name, O_RDWR);

		if (file_descriptor == -1) {
			return mapped_file;
		}

		if (fstat(file_descriptor, &file_stat) == -1) {
			return mapped_file;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			return mapped_file;
		}

		mapped_file.size  = file_stat.st_size;

		/* map file in a shared fashion for reading */
		mapped_file.handle = mmap (0, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
		if (mapped_file.handle == MAP_FAILED) {
			return mapped_file;
		}

		if (close(file_descriptor) == -1) {
			return mapped_file;
		}

		mapped_file.write  = 1;
		mapped_file.read   = 1;
		mapped_file.mapped = 1;
		mapped_file.begin  = (char*) mapped_file.handle;

		return mapped_file;

	}
	return mapped_file;
}

PLATFORM_RESIZE_FILE(osx_linux_resize_file)
{
	char file_name[1024];
	Assert(file_end - file_begin + 1 < 1024);
	u64 len = pt_copy_bytes(file_begin, file_end, file_name, file_name + 1023);
	file_name[len] = 0;
	s32 file_descriptor = open(file_name, O_RDWR);
	s32 result = !ftruncate(file_descriptor, new_size);
	close(file_descriptor);
	return result;
}

PLATFORM_CLOSE_MMAP_FILE(osx_linux_close_mmap_file)
{
	Assert(mapped_file->mapped);
	if (munmap(mapped_file->handle, mapped_file->size) == -1) {
		return;
	}
	mapped_file->mapped   = 0;
	mapped_file->unmapped = 1;
	mapped_file->handle   = 0;
	mapped_file->begin    = 0;
}

PLATFORM_OPEN_READ_FILE(osx_linux_open_read_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = fopen(name, "rb");

	free(name);

	pt_File pfh;
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		pfh.size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
	} else {
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

PLATFORM_OPEN_WRITE_FILE(osx_linux_open_write_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = fopen(name, "wb");

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

PLATFORM_READ_NEXT_FILE_CHUNK(osx_linux_read_next_file_chunk)
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
PLATFORM_SEEK_FILE(osx_linux_seek_file)
{
	Assert(pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	int err = fseek(fp, (s64) offset, SEEK_SET);
	pfh->last_seek_success = !err;
}

// void name(PlatformFileHandle *pfh)
PLATFORM_CLOSE_FILE(osx_linux_close_file)
{
	Assert(pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	fclose(fp);
	pfh->open = 0;
}

// b8 name(PlatformFileHandle *pfh, char *begin, char* end)
PLATFORM_WRITE_TO_FILE(osx_linux_write_to_file)
{
	Assert(pfh->write && pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	size_t size = (size_t) (end-begin);
	size_t written = fwrite(begin, 1, size, fp);
	fflush(fp);
	return written == size;
}

PLATFORM_GET_TIME(osx_linux_get_time)
{
	return (f64) time(0); // clock() / (f64) CLOCKS_PER_SEC;
}


PLATFORM_CREATE_MUTEX(osx_linux_create_mutex)
{
	pthread_mutex_t *raw_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	return (pt_Mutex) { .handle = raw_mutex };
}

PLATFORM_RELEASE_MUTEX(osx_linux_release_mutex)
{
	pthread_mutex_t *raw_mutex = (pthread_mutex_t*) mutex.handle;
	free(raw_mutex);
}

PLATFORM_LOCK_MUTEX(osx_linux_lock_mutex)
{
	pthread_mutex_lock((pthread_mutex_t*) mutex.handle);
}

PLATFORM_UNLOCK_MUTEX(osx_linux_unlock_mutex)
{
	pthread_mutex_unlock((pthread_mutex_t*) mutex.handle);
}


#include "x86intrin.h"

internal
PLATFORM_CYCLE_COUNT_CPU(osx_linux_cycle_count_cpu)
{
	return (u64) __rdtsc();
}

internal void
osx_linux_init_platform(PlatformAPI* p)
{
    p->allocate_memory          = osx_linux_allocate_memory;
    p->resize_memory            = osx_linux_resize_memory;
    p->free_memory              = osx_linux_free_memory;
    p->copy_memory              = osx_linux_copy_memory;
    p->open_write_file          = osx_linux_open_write_file;
    p->open_read_file           = osx_linux_open_read_file;
    p->read_next_file_chunk     = osx_linux_read_next_file_chunk;
    p->seek_file                = osx_linux_seek_file;
    p->close_file               = osx_linux_close_file;
    p->write_to_file            = osx_linux_write_to_file;
    p->get_time                 = osx_linux_get_time;
    p->open_mmap_file           = osx_linux_open_mmap_file;
    p->close_mmap_file          = osx_linux_close_mmap_file;
    p->resize_file              = osx_linux_resize_file;
    p->cycle_count_cpu          = osx_linux_cycle_count_cpu;

    // mutex
    p->create_mutex             = osx_linux_create_mutex;
    p->release_mutex            = osx_linux_release_mutex;
    p->lock_mutex               = osx_linux_lock_mutex;
    p->unlock_mutex             = osx_linux_unlock_mutex;
}


