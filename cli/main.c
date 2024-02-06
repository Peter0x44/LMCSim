#include "lmc.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }


// TODO figure out wtf to do with these - duplication probably not ideal

typedef struct {
	unsigned char* buf;
	int capacity;
	int len;
	bool error;
} buf;

static s8 bufTos8(buf* buffer)
{
	return (s8) { buffer->buf, buffer->len };
}

static void append(buf* buffer, unsigned char* src, int len)
{
	int available = buffer->capacity - buffer->len;
	// copy  over as much as possible
	int amount = available < len ? available : len;
	for (int i = 0; i < amount; ++i)
	{
		buffer->buf[buffer->len+i] = src[i];
	}
	buffer->len += amount;
	// tried to append too much?
	buffer->error |= amount < len;
}

static void appends8(buf* buffer, s8 string)
{
	append(buffer, string.str, string.len);
}

static void appendInteger(buf* buffer, int x)
{
	unsigned char tmp[64];
	unsigned char* end = &tmp[sizeof(tmp)];
	unsigned char* beginning = end;
	int t = x>0 ? -x : x;
	do
	{
		// append to the buffer "backwards"
		--beginning;
		*beginning = '0' - t%10;
	} while (t /= 10);
	if  (x < 0)
	{
		--beginning;
		*beginning = '-';
	}
	append(buffer, beginning, end-beginning);
}

static void appendChar(buf* buffer, unsigned char c)
{
	append(buffer, &c, 1);
}


///////////////////////////////////////////////////////


#ifdef __linux__

#include "linux/inpcallback.c"
#include "linux/mapfile.c"
#include "linux/outcallback.c"

#elifdef __WIN32__

#include "windows/inpcallback.c"

// TODO implement these on windows
//#include "windows/mapfile.c"
//#include "windows/outcallback.c"

#endif

int main(int argc, char* argv[])
{
	if (argc <= 1) return 0;
	s8 program = s8FileMap(argv[1]);

	// for a nonexistent file, gcc returns 1
	// so, I assume it's the correct thing to do here
	// I tested a few other examples, and the only exception I could find is python, which returns 2
	if (!program.str) return 1;

	LMCContext x = {0} ;
	x.inpFunction = InpCallbackDefault;
	x.outFunction = OutCallbackDefault;

	AssemblerError ret = Assemble(program, &x, true);

	if (ret.lineNumber != -1)
	{
		unsigned char mem[12];
		buf buffer;
		buffer.buf = &mem[0];
		buffer.capacity = sizeof(mem);
		buffer.len = 0;
		buffer.error = 0;

		appendInteger(&buffer, ret.lineNumber);
		appends8(&buffer, S(": "));
		s8 s = bufTos8(&buffer);
		OutCallbackDefault(s.str, s.len, 0);
		OutCallbackDefault(ret.message.str, ret.message.len, 0);
		unsigned char n = '\n';
		OutCallbackDefault(&n, 1, 0);
		return 1;
	}

	RuntimeError termination;
	while (true)
	{
		termination = Step(&x);

		if (termination == ERROR_BAD_INPUT)
		{
			s8 s = RuntimeError_StrError(&x, termination);
			OutCallbackDefault(s.str, s.len, 0);
		}
		else if (termination != ERROR_OK) break;
	}
	s8 s = RuntimeError_StrError(&x, termination);

	OutCallbackDefault(s.str, s.len, 0);

	unsigned char n = '\n';
	OutCallbackDefault(&n, 1, 0);
}
