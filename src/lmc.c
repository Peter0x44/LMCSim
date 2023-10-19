#include "lmc.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }

#if defined(DEBUG) && defined(__GNUC__)
#define assert(e) do { if (!(e)) __builtin_trap(); } while (0)
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

bool IsWhitespace(char c)
{
	return ((c == ' ') || (c == '\t'));
}

bool IsNewline(char c)
{
	return ((c == '\n') || (c == '\r'));
}

s8 StripWhitespace(s8 line)
{
	while (line.len > 0 && IsWhitespace(*line.str))
	{
		++line.str;
		--line.len;
	}

	while (line.len > 0 && IsWhitespace(line.str[line.len-1]))
	{
		--line.len;
	}

	return line;
}

bool IsCommentToken(char c)
{
	return ((c == '#') || (c == '/') || (c == ';'));
}

s8 StripComment(s8 line)
{
	unsigned char* commentPos = 0;
	bool wasSlash = false;

	for (int i = line.len-1; i >= 0; --i)
	{
		if (IsCommentToken(line.str[i]))
		{
			unsigned char* end = line.str + i;
			if (line.str[i] == '/')
			{
				if (wasSlash)
					commentPos = end;
				else
					wasSlash = true;
			}
			else
			{
				commentPos = end;
				wasSlash = false;
			}
		}
		else wasSlash = false;
	}
	if (commentPos)
		line.len -= (line.str + line.len) - commentPos;

	return line;
}

// Extract next line of "buf". Modifies buf to point to the next line too, so it can be called repeatedly until it returns an empty string
s8 GetLine(s8* buf)
{
	assert(buf);
	// Skip over any newlines, if they are at the beginning
	while (buf->len > 0 && IsNewline(*buf->str))
	{
		++buf->str;
		--buf->len;
	}

	// Find the next newline, and end the string before it
	for (int i = 0; i < buf->len; ++i)
	{
		if (IsNewline(buf->str[i]))
		{
			s8 ret;
			ret.str = buf->str;
			ret.len = i;
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

	for (int i = 0; i < a.len; ++i)
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
	// Tests for GetLine
	{
		s8 test = S("\n \n test  \r testtt\r\n  test");
		assert(s8Equal(GetLine(&test), S(" ")));
		assert(s8Equal(GetLine(&test), S(" test  ")));
		assert(s8Equal(GetLine(&test), S(" testtt")));
		assert(s8Equal(GetLine(&test), S("  test")));
		assert(s8Equal(GetLine(&test), S("")));
		assert(s8Equal(GetLine(&test), S("")));
	}
	// Tests for StripWhitespace
	{
		assert(s8Equal(StripWhitespace(S("    hi   ")), S("hi")));
		assert(s8Equal(StripWhitespace(S("    hi")), S("hi")));
		assert(s8Equal(StripWhitespace(S("hi   ")), S("hi")));
		assert(s8Equal(StripWhitespace(S("\t hello world hi   ")), S("hello world hi")));
		assert(s8Equal(StripWhitespace(S("  \t   \t ")), S("")));
	}
	// Tests for StripComment
	{
		assert(s8Equal(StripComment(S("")), S("")));
		assert(s8Equal(StripComment(S("INP")), S("INP")));
		assert(s8Equal(StripComment(S("INP  ")), S("INP  ")));
		assert(s8Equal(StripComment(S("INP  # comment")), S("INP  ")));
		assert(s8Equal(StripComment(S("INP  // comment")), S("INP  ")));
		assert(s8Equal(StripComment(S("INP  ; comment")), S("INP  ")));
		assert(s8Equal(StripComment(S("INP  / / comment")), S("INP  / / comment")));
		assert(s8Equal(StripComment(S("INP  comment")), S("INP  comment")));
		assert(s8Equal(StripComment(S("INP  ; comment // othercomment")), S("INP  ")));
		assert(s8Equal(StripComment(S("INP  // ; comment # othercomment")), S("INP  ")));
		assert(s8Equal(StripComment(S("// ; comment # othercomment")), S("")));
	}
}

#else

int main(void)
{
	s8 program = S("INP\nSTA 99\nADD 99\nOUT\nHLT");
	LMCContext x;

	Assemble(program, &x);
}
#endif
