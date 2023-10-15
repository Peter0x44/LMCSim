#include "lmc.h"

#define S(s) (s8) { (char*)s, (ptrdiff_t)(sizeof(s)-1) }

#ifdef DEBUG
#define assert(e) if (!(e)) __builtin_trap()
#else
#define assert(e)
#endif

#include <stdio.h>

// function to print s8s for debugging
// also escapes newlines and writes them verbatim like a debugger would
void s8Print(s8 s)
{
//        printf("\"%.*s\"\n", (int)s.len, s.str);
	putchar('\"');
	for (int i = 0; i < s.len; ++i)
	{
		if (s.str[i] == '\n')
		{
			putchar('\\');
			putchar('n');
		}
		else
			putchar(s.str[i]);
	}
	putchar('\"');
	putchar('\n');
}

// Extract next line of "buf". Modifies buf to point to the next line too, so it can be called repeatedly until it returns an empty string
s8 GetLine(s8* buf)
{
	assert(buf);
	// Skip over any newlines, if they are at the beginning
	s8 tmp = *buf;
	for (ptrdiff_t i = 0; i < buf->len; ++i)
	{
		if (tmp.str[i] == '\n' || tmp.str[i] == '\r')
		{
			++buf->str;
			--buf->len;
		}
		else
			break;
	}

	// find the next newline, and end the string before it
	for (char* s = buf->str; s < buf->str + buf->len; ++s)
	{
		if (*s == '\n' || *s == '\r')
		{
			s8 ret;
			ret.str = buf->str;
			ret.len = s - buf->str;
			// Remove returned line from buf
			buf->len -= ret.len;
			buf->str += ret.len;
			return ret;
		}
	}

	// No newline found - last line or EOF
	s8 ret = *buf;
	buf->len -= ret.len;
	buf->str += ret.len;
	return ret;
}

AssemblerError Assemble(s8 assembly, LMCContext* code)
{
	assert(code);
	return 0;
}


int main(void)
{
	s8 bruh = S("\n \n bruh  \n bruhhh\n  bruh");
	s8 tmp;

	for (int i = 0; i < 5; ++i)
	{
		tmp = GetLine(&bruh);
		s8Print(tmp);
	}
}
