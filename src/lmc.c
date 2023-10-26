#include "lmc.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }

#if defined(DEBUG) && defined(__GNUC__)
#define assert(e) do { if (!(e)) __builtin_trap(); } while (0)
// Macro only works for "GNUC" compilers. I will decide if I care about visual studio later
#else
#define assert(e)
#endif


#include <stdio.h>
#include <string.h>
#include <limits.h>

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

bool IsDigit(unsigned char c)
{
	return ((c >= '0') && (c <= '9'));
}

typedef enum
{
	NOT_A_NUMBER = 1,
	NOT_IN_RANGE = 2,
} IntegerInputError;

IntegerInputError s8ToInteger(s8 s, int* result)
{
	int i = 0;
	bool negate = false;
	unsigned int value = 0;
	unsigned int limit = INT_MAX;

	switch (*s.str) {
		case '-':
			i = 1;
			negate = true;
			limit = UINT_MAX;
			break;
		case '+':
			i = 1;
			break;
	}

	for (; i < s.len; ++i)
	{
		if (!IsDigit(s.str[i]))
		{
			return NOT_A_NUMBER;
		}
		int d = s.str[i] - '0';
		if (value > (limit - d)/10)
		{
			return NOT_IN_RANGE;
		}
		value = value*10 + d;
	}
	if (value > 999)
	{
		return NOT_IN_RANGE;
	}
	if (negate) value *= -1;

	*result = value;
	return 0;
}

bool IsLower(unsigned char c)
{
	return (c >= 'a' && c <= 'z');
}

unsigned char ToUpper(unsigned char c)
{
	if (IsLower(c))
		c += 'A' - 'a';
	return c;
}

static const s8 mnemonics[] = {
	S("HLT"), // 000
	S("ADD"), // 1xx
	S("SUB"), // 2xx
	S("STA"), // 3xx
	          // 4xx (unused)
	S("LDA"), // 5xx
	S("BRA"), // 6xx
	S("BRZ"), // 7xx
	S("BRP"), // 8xx
	S("INP"), // 901
	S("OUT"), // 902
	S("OTC"), // 922
	S("DAT")
};

// Give proper forward declarations later
bool s8iEqual(s8 a, s8 b);

int GetMnemonicValue(s8 mnemonic)
{
	if (false) {}

	else if (s8iEqual(mnemonic, mnemonics[0 ])) // HLT
		return 000;
	else if (s8iEqual(mnemonic, mnemonics[1 ])) // ADD
		return 100;
	else if (s8iEqual(mnemonic, mnemonics[2 ])) // SUB
		return 200;
	else if (s8iEqual(mnemonic, mnemonics[3 ])) // STA
		return 300;
	else if (s8iEqual(mnemonic, mnemonics[4 ])) // LDA
		return 500;
	else if (s8iEqual(mnemonic, mnemonics[5 ])) // BRA
		return 600;
	else if (s8iEqual(mnemonic, mnemonics[6 ])) // BRZ
		return 700;
	else if (s8iEqual(mnemonic, mnemonics[7 ])) // BRP
		return 800;
	else if (s8iEqual(mnemonic, mnemonics[8 ])) // INP
		return 901;
	else if (s8iEqual(mnemonic, mnemonics[9 ])) // OUT
		return 902;
	else if (s8iEqual(mnemonic, mnemonics[10])) // OTC
		return 922;
	else
		return -1;
}

bool IsWhitespace(char c)
{
	return ((c == ' ') || (c == '\t'));
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

// Strips comments from the end of the line
// 3 tokens are supported: // ; #
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

bool IsNewline(char c)
{
	return ((c == '\n') || (c == '\r'));
}

// Extract next line of "buf". Modifies buf to point to the next line too, so it can be called repeatedly until it returns an empty string
s8 GetLine(s8* buf)
{
	assert(buf);
	// skip over any newlines, if they are at the beginning
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

s8 GetWord(s8* line)
{
	assert(line);

	// skip over any spaces, if they are at the beginning
	while (line->len > 0 && *line->str == ' ')
	{
		++line->str;
		--line->len;
	}


	// Find the next space, and end the string before it
	for (int i = 0; i < line->len; ++i)
	{
		if (line->str[i] == ' ')
		{
			s8 ret;
			ret.str = line->str;
			ret.len = i;
			// Remove returned word from line
			line->len -= ret.len;
			line->str += ret.len;
			return ret;
		}
	}

	// No space found - last word or end of line
	s8 ret = *line;
	line->len -= ret.len;
	line->str += ret.len;
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

// Case insensitive comparison
bool s8iEqual(s8 a, s8 b)
{
	if (a.len != b.len) return false;

	for (int i = 0; i < a.len; ++i)
	{
		if (ToUpper(a.str[i]) != ToUpper(b.str[i])) return false;
	}

	return true;
}

AssemblerError Assemble(s8 assembly, LMCContext* code)
{
	assert(code);
	int lineNumber = 1;
	do {
		printf("%d: ", lineNumber++);
		s8 line = GetLine(&assembly);
		line = StripComment(line);
		line = StripWhitespace(line);
		s8 word = GetWord(&line);
		if (!s8Equal(word, S("")))
		{
			int value = GetMnemonicValue(word);
			bool takesOperand = ((value != 0) && (value % 100 == 0));
			if (takesOperand)
			{
				int result;
				word = GetWord(&line);
				// TODO handle integer parsing errors
				s8ToInteger(word, &result);
				value += result;
			}
			printf("%d\n", value);
		} else continue;
	} while(!s8Equal(S(""), assembly));
	return 0;
}


#ifdef TEST

#define testcase(s1, s2) assert(s8Equal(s1, s2))

int main(void)
{
	// Tests for GetLine
	{
		s8 test = S("\n \ntest  \r testtt\r\n  test");
		testcase(GetLine(&test), S(" "));
		testcase(GetLine(&test), S("test  "));
		testcase(GetLine(&test), S(" testtt"));
		testcase(GetLine(&test), S("  test"));
		testcase(GetLine(&test), S(""));
		testcase(GetLine(&test), S(""));
	}
	// Tests for GetWord
	{
		s8 test = S("this is a     line");
		testcase(GetWord(&test), S("this"));
		testcase(GetWord(&test), S("is"));
		testcase(GetWord(&test), S("a"));
		testcase(GetWord(&test), S("line"));
		testcase(GetWord(&test), S(""));
		testcase(GetWord(&test), S(""));
	}
	// Tests for StripWhitespace
	{
		testcase(StripWhitespace(S("")), S(""));
		testcase(StripWhitespace(S("  \t   \t ")), S(""));
		testcase(StripWhitespace(S("    hi   ")), S("hi"));
		testcase(StripWhitespace(S("    hi")), S("hi"));
		testcase(StripWhitespace(S("hi   ")), S("hi"));
		testcase(StripWhitespace(S("\t hello world hi   ")), S("hello world hi"));
	}
	// Tests for StripComment
	{
		testcase(StripComment(S("")), S(""));
		testcase(StripComment(S("     ")), S("     "));
		testcase(StripComment(S("INP")), S("INP"));
		testcase(StripComment(S("INP  ")), S("INP  "));
		testcase(StripComment(S("INP  # comment")), S("INP  "));
		testcase(StripComment(S("INP  // comment")), S("INP  "));
		testcase(StripComment(S("INP  ; comment")), S("INP  "));
		testcase(StripComment(S("INP  / / comment")), S("INP  / / comment"));
		testcase(StripComment(S("INP  comment")), S("INP  comment"));
		testcase(StripComment(S("INP  ; comment // othercomment")), S("INP  "));
		testcase(StripComment(S("INP  // ; comment # othercomment")), S("INP  "));
		testcase(StripComment(S("// ; comment # othercomment")), S(""));
	}

	// Tests for s8iEqual
	{
		assert(s8iEqual(S("Hi"), S("hi")));
		assert(!s8iEqual(S("hi"), S("no")));
		assert(s8iEqual(S("hi"), S("hi")));
	}

	// Tests for s8ToInteger
	{
		for (int i = -999; i < 1000; ++i)
		{
			unsigned char buf[5] = {0};
			sprintf((char*)buf, "%d", i);
			s8 s;
			s.str = buf;
			s.len = strlen((char*)buf);
			int res;
			IntegerInputError error = s8ToInteger(s, &res);
			assert(error == 0);
			assert(res == i);
		}

		{
			s8 num = S("432075432057932475904203598347532048");
			IntegerInputError error = s8ToInteger(num, 0);
			assert(error == NOT_IN_RANGE);
			num = S("-10...");
			error = s8ToInteger(num, 0);
			assert(error == NOT_A_NUMBER);
		}
	}
}

#else

int main(void)
{
	s8 program = S("inp\nsta 99\n    \nadd 99\nout\nhlt // output the sum of two numbers");
	LMCContext x;

	Assemble(program, &x);
}
#endif
