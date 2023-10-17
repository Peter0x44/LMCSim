#include "lmc.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }

#if defined(DEBUG) && defined(__GNUC__)
#define assert(e) do { if (!(e)) __builtin_trap(); } while (0);
// Macro only works for "GNUC" compilers. I will decide if I care about visual studio later
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

	// Find the next newline, and end the string before it
	for (unsigned char* s = buf->str; s < buf->str + buf->len; ++s)
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

bool s8Equal(s8 a, s8 b)
{
	if (a.len != b.len) return false;

	for (ptrdiff_t i = 0; i < a.len; ++i)
	{
		if (a.str[i] != b.str[i])
			return false;
	}

	return true;
}

AssemblerError Assemble(s8 assembly, LMCContext* code)
{
	assert(code);
	int lineNumber = 1;
	do {
		printf("%d: ", lineNumber++);
		s8Print(GetLine(&assembly));
	} while(!s8Equal(S(""), assembly));
	return 0;
}


#ifdef TEST

int main(void)
{
	s8 test = S("\n \n test  \r testtt\r\n  test");

	assert(s8Equal(GetLine(&test), S(" ")));
	assert(s8Equal(GetLine(&test), S(" test  ")));
	assert(s8Equal(GetLine(&test), S(" testtt")));
	assert(s8Equal(GetLine(&test), S("  test")));
	assert(s8Equal(GetLine(&test), S("")));
}

#else

int main(void)
{
	s8 bruh = S("\n \n bruh  \n bruhhh\n  bruh");
	LMCContext x;

	Assemble(bruh, &x);

}
#endif
