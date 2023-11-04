#include "lmc.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }

// Macro only works for "GNUC" compilers. I will decide if I care about visual studio later
#if defined(__GNUC__)
#define assert(e) do { if (!(e)) __builtin_unreachable(); } while (0)
#endif


#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

typedef struct
{
	char* beginning;
	char* end;
} arena;

void* alloc(arena* a, ptrdiff_t size, ptrdiff_t count)
{
	ptrdiff_t available = a->end - a->beginning;
	if (count > (available/size))
	{
		return 0;
	}
	ptrdiff_t total = size * count;
	char* p = a->beginning;
	a->beginning += total;
	memset(p, 0, total);
	return p;
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
	else if (s8iEqual(mnemonic, mnemonics[11])) // DAT
		return 1000;
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

typedef struct
{
	s8 label;
	int value;
} LabelInfo;

AssemblerError Assemble(s8 assembly, LMCContext* code, bool strict)
{
	assert(code);
	memset(code->mailBoxes, 0, 100*sizeof(int));

	// Memory for arena allocator
	/*
	char mem[1<<17];
	arena a;
	a.beginning = &mem[0];
	a.end = &mem[sizeof(mem)];
	*/

	LabelInfo labels[100];
	int labelCount = 0;

	int lineNumber = 0;
	int currentInstructionPointer = 0;
	s8 save = assembly;

	// First loop is to get all labels
	// searches for lines in the format <label> <mnemonic> <opcode>
	// <label> is optional
	// insert their address into a an array
	// TODO: replace this with a hash map for better efficiency
	// currently, naive O(n) lookup is done every time it's needed
	while(!s8Equal(assembly, S(""))) // empty string is EOF
	{
		++lineNumber;
		s8 line = GetLine(&assembly);
		line = StripComment(line);
		line = StripWhitespace(line);
		s8 label = GetWord(&line);
		// empty line
		if (s8Equal(label, S("")))
			continue;

		if (GetMnemonicValue(label) == -1) // first word of line is not a known mnemonic
		{
			if (GetMnemonicValue(GetWord(&line)) != -1) // second word is a known mnemonic - first word is a label
			{
				bool labelRedefined = false;
				// Search if label has been defined already
				for (int i = 0; i < labelCount; ++i)
				{
					if (s8Equal(labels[i].label, label))
					{
						// label redefined
						// TODO implement proper error return
						// NOTE: Peter Higginson LMC is fine
						// with redefining labels - it just uses the first definition it finds as the value
						// lmc.awk has a specific error for it. I decided to implement it as an error only if
						// strict mode is enabled
						if (strict)
						{
							printf("label \"label\" redefined\n");
							return;
						}
						labelRedefined = true;
					}
				}

				if (!labelRedefined)
				{
					LabelInfo tmp;
					tmp.label = label;
					tmp.value = currentInstructionPointer++;
					labels[labelCount++] = tmp;
				}
				else
				{
					++currentInstructionPointer;
				}
			}
			else
			{
				printf("Unknown mnemonic \"label\" at line %d", lineNumber);
				return;
			}
		}
		else
		{
			++currentInstructionPointer;
		}
	} 

	/*
	for (int i = 0; i < labelCount; ++i)
	{
		s8Print(labels[i].label);
		printf("%d\n", labels[i].value);
	}
	*/

	lineNumber = 0;
	currentInstructionPointer = 0;
	assembly = save;

	do {
		++lineNumber;
		s8 line = GetLine(&assembly);
		line = StripComment(line);
		line = StripWhitespace(line);
		s8 word = GetWord(&line);
		if (s8Equal(word, S("")))
		{
			continue;
		}
		int opcode = GetMnemonicValue(word);
		if (opcode == -1) opcode = GetMnemonicValue(GetWord(&line));
		if (opcode == 1000)// || (opcode == -1))// && s8iEqual(GetWord(&line), mnemonics[11])))
		{
			s8 operand = GetWord(&line);
			int value;
			IntegerInputError ret = s8ToInteger(operand, &value);
			if (ret == NOT_A_NUMBER)
			{
				if (s8Equal(operand, S("")))
				{
					code->mailBoxes[currentInstructionPointer++] = 0;
				}
				else
				{
					bool foundLabel = false;
					for (int i = 0; i < labelCount; ++i)
					{
						if (s8Equal(labels[i].label, operand))
						{
							code->mailBoxes[currentInstructionPointer++] = labels[i].value;
							foundLabel = true;
						}
					}
					if (!foundLabel)
					{
						printf("ERROR: undefined label %%s (operand)");
						return;
					}
				}
			} else if (ret == NOT_IN_RANGE)
			{
				printf("ERROR: value (operand) is not in range 0, 999)");
				return;
			}
			else
			{
				code->mailBoxes[currentInstructionPointer++] = value;
			}
		} else
		{
			bool takesOperand = ((opcode != 0) && (opcode != 1000) && (opcode % 100 == 0));
			if (takesOperand)
			{
				s8 operand = GetWord(&line);
				int value;
				IntegerInputError ret = s8ToInteger(operand, &value);
				if (ret == NOT_A_NUMBER)
				{
					bool foundLabel = false;
					for (int i = 0; i < labelCount; ++i)
					{
						if (s8Equal(labels[i].label, operand))
						{
							code->mailBoxes[currentInstructionPointer++] = opcode + labels[i].value;
							foundLabel = true;
						}
					}
					if (!foundLabel)
					{
						printf("ERROR: undefined label %%s (operand)");
						return;
					}
				} else if (ret == NOT_IN_RANGE)
				{
					printf("ERROR: value (operand) is not in range 0, 999)");
					return;
				}
				else
				{
					code->mailBoxes[currentInstructionPointer++] = opcode + value;
				}
			}
			else
			{
				code->mailBoxes[currentInstructionPointer++] = opcode;
			}
		}
	} while(!s8Equal(assembly, S("")));

	for (int i = 0; i < 10; ++i)
	{
		for (int j = 0; j < 10; ++j)
		{
			printf("%03i ", code->mailBoxes[i*10+j]);
		}
		printf("\n");
	}

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

	// Tests for arena allocator
	{
		char mem[1<<10];
		arena a;
		a.beginning = &mem[0];
		a.end = &mem[sizeof(mem)];
		arena free = a;

		int* p = alloc(&a, sizeof(int), 128);
		assert(p);
		p = alloc(&a, sizeof(int), 64);
		assert(p);
		p = alloc(&a, sizeof(int), 64);
		assert(p);
		p = alloc(&a, sizeof(int), 128);
		assert(!p);

		a = free;
		p = alloc(&a, sizeof(int), 257);
		assert(!p);
	}
}

#else

int main(void)
{
//	s8 program = S("        lda space\n sta char\n loop    lda char\n out\n lda space\n otc\n lda char\n otc\n add one\n sta char\n sub max\n brz end\n bra loop\n end     hlt\n space   dat 32\n one     dat 1\n max     dat 97\n char    dat\n // start of ASCII character table\n");
//	s8 program = S("        LDA lda1\n STA outputList\n LDA sta1\n STA store\n LDA zero\n STA listSize\n inputLoop INP\n BRZ resetLoop\n store   DAT 380\n LDA store\n ADD increment\n STA store\n LDA listSize\n ADD increment\n STA listSize\n BRA inputLoop\n resetLoop LDA lda1\n STA load1\n ADD increment\n STA load2\n LDA sta1\n STA store1\n ADD increment\n STA store2\n LDA listSize\n SUB increment\n STA loopCount\n LDA zero\n STA isChange\n load1   DAT 580\n STA buffA\n load2   DAT 581\n STA buffB\n cmp     SUB buffA\n BRP nextItem\n swap    LDA buffB\n store1  DAT 380\n LDA buffA\n store2  DAT 381\n LDA increment\n STA isChange\n nextItem LDA store1\n ADD increment\n STA store1\n ADD increment\n STA store2\n LDA load1\n ADD increment\n STA load1\n ADD increment\n STA load2\n LDA loopCount\n SUB increment\n STA loopCount\n BRZ isFinished\n BRA load1\n isFinished LDA isChange\n BRZ outputList\n bra resetLoop\n outputList DAT 580\n OUT\n LDA outputList\n ADD increment\n STA outputList\n LDA listSize\n SUB increment\n STA listSize\n BRZ end\n BRA outputList\n end     HLT\n zero    DAT 0\n buffA   DAT 0\n buffB   DAT 0\n isChange DAT 0\n increment DAT 1\n listSize DAT 0\n loopCount DAT 0\n sta1    DAT 380\n lda1    DAT 580\n");
//	s8 program = S("        INP\n STA VALUE\n LDA ZERO\n STA TRINUM\n STA N\n LOOP    LDA TRINUM\n SUB VALUE\n BRP ENDLOOP\n LDA N\n ADD ONE\n STA N\n ADD TRINUM\n STA TRINUM\n BRA LOOP\n ENDLOOP LDA VALUE\n SUB TRINUM\n BRZ EQUAL\n LDA ZERO\n OUT\n BRA DONE\n EQUAL   LDA N\n OUT\n DONE    HLT\n VALUE   DAT\n TRINUM  DAT\n N       DAT\n ZERO    DAT 000\n ONE     DAT 001\n // Test if input is a triangular number\n // If is sum of 1 to n output n\n // otherwise output zero\n");
//	s8 program = S("        INP\n STA VALUE\n LDA ONE\n STA MULT\n OUTER   LDA ZERO\n STA SUM\n STA TIMES\n INNER   LDA SUM\n ADD VALUE\n STA SUM\n LDA TIMES\n ADD ONE\n STA TIMES\n SUB MULT\n BRZ NEXT\n BRA INNER\n NEXT    LDA SUM\n OUT\n LDA MULT\n ADD ONE\n STA MULT\n SUB VALUE\n BRZ OUTER\n BRP DONE\n BRA OUTER\n DONE    HLT\n VALUE   DAT 0 // Times table for\n MULT    DAT 0 // one input number\n SUM     DAT\n TIMES   DAT\n COUNT   DAT\n ZERO    DAT 000\n ONE     DAT 001");

	s8 program = S("label DAT\nlabel ADD label\nlabel ADD\nnewlabel DAT\nADD newlabel");
	
	LMCContext x = {0} ;

	Assemble(program, &x, false);
}
#endif
