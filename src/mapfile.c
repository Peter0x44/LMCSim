#include "mapfile.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

// contents will be the the contents of the file
// returns the file descriptor
int s8FileMap(const char* fileName, s8* contents)
{
	if (!fileName) return -1;
	int fd = open(fileName, O_RDONLY);

	if (fd == -1) return -1;

	struct stat stbuf;
	if (fstat(fd, &stbuf) == -1)
	{
		close(fd);
		return -1;
	}
	off_t fileLen = stbuf.st_size;

	void* fileContents = mmap(0, fileLen, PROT_READ, MAP_PRIVATE, fd, 0);
	if (fileContents == MAP_FAILED)
	{
		close(fd);
		return -1;
	}
	contents->len = fileLen;
	contents->str = fileContents;

	return fd;
}

void s8FileUnmap(int fd, s8 fileContents)
{
	munmap(fileContents.str, fileContents.len);
	close(fd);
}
