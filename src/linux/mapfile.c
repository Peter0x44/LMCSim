#include "../mapfile.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

// returns the contents of file
// No unmap function because I really don't care if it leaks
// TODO port to Windows
s8 s8FileMap(const char* fileName)
{
	s8 contents = (s8) { 0, 0 };
	if (!fileName) return contents;

	int fd = open(fileName, O_RDONLY);

	if (fd == -1) return contents;

	struct stat stbuf;
	if (fstat(fd, &stbuf) == -1)
	{
		close(fd);
		return contents;
	}
	off_t fileLen = stbuf.st_size;

	void* fileContents = mmap(0, fileLen, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);

	if (fileContents == MAP_FAILED)
	{
		return contents;
	}
	contents.len = fileLen;
	contents.str = fileContents;

	return contents;
}
