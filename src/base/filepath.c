//------------------------------------------------------------------------------
// platform independent file path struct
//------------------------------------------------------------------------------

#define MAX_FILE_PATH_SIZE 1023

typedef struct {
	char  full_path[MAX_FILE_PATH_SIZE+1];
	char* name;
	char* extension; // extension start last . in name
	char* end;
} FilePath;

//------------------------------------------------------------------------------
// platform independent file path services
//------------------------------------------------------------------------------

static void
FilePath_init(FilePath *self, char *begin, char *end)
{
	s64 n = end - begin;
	Assert(n <= MAX_FILE_PATH_SIZE);
	self->end = self->full_path + n;
	*self->end = 0;
	self->full_path[n+1] = 0; // make sure it is a cstr
	pt_copy_bytes(begin, end, self->full_path, self->end);
	self->end  = self->full_path + n;
	self->name = self->full_path;

	// find last /
	char *it = self->full_path;
	while (it != self->end) {
		if (*it == '/' || *it == '\\') {
			self->name = it + 1;
			self->extension = self->end;
		} else if (*it == '.') {
			self->extension = it;
		}
		++it;
	}
}

static void
FilePath_copy(FilePath* self, FilePath *other)
{
	char *dst = self->full_path;
	for (char *it=other->full_path;it<other->end;++it) {
		*dst++ = *it;
	}
	self->end  = dst;
	*dst = 0; // make it cstr
	self->name = self->full_path + (other->name - other->full_path);
	self->extension = self->full_path
		+ (other->extension - other->full_path);
}

static void
FilePath_set_name(FilePath* self, char *begin, char *end)
{
	s64 n = end - begin;

	Assert((self->name - self->full_path) + n <= MAX_FILE_PATH_SIZE);

	self->end = self->name + n;
	*self->end = 0;
	pt_copy_bytes(begin, end, self->name, self->end);
	self->extension = self->end;

	// find last /
	self->extension = self->end;
	char *it = self->full_path;
	while (it != self->end)
	{
		if (*it == '.')
		{
			self->extension = it;
		}
		++it;
	}
}

static inline void
FilePath_set_name_cstr(FilePath* fp, char *cstr)
{
	FilePath_set_name(fp, cstr, cstr_end(cstr));
}


