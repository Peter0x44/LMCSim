#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>

void OutCallbackDefault(unsigned char* str, ptrdiff_t len, void* ctx)
{
	// Just writing to stdout, don't need ctx
	(void) ctx;

	for (int offset = 0; offset < len;)
	{
		int ret = write(STDOUT_FILENO, str, len);
		if (ret < 0)
		{
			exit(2);
		}
		offset += ret;
	}
}

