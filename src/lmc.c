#include "lmc.h"
#include "mapfile.h"

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

// function doesn't take a size or alignment because I don't expect to allocate anything
// other than chars using it.
void* alloc(arena* a, ptrdiff_t count)
{
	ptrdiff_t available = a->end - a->beginning;
	if (count > available)
	{
		return 0;
	}
	char* p = a->beginning;
	a->beginning += count;
	memset(p, 0, count);
	return p;
}

s8 s8AssemblerError(AssemblerError error, arena* a)
{
	// -1 because tab character won't get copied
	ptrdiff_t messageLen = error.message.len + error.context.len - 1;

	s8 message;
	message.str = alloc(a, messageLen);
	message.len = messageLen;

	s8 ret = message;

	if (!message.str)
	{
		return (s8) { 0 };
	}

	while (error.message.str[0] != '\t')
	{
		assert(error.message.len > 0);
		message.str[0] = error.message.str[0];
		++error.message.str;
		--error.message.len;
		++message.str;
		--message.len;
	}

	++error.message.str;
	--error.message.len;

	//assert(error.context.len > 0);
	for (int i = 0; i < error.context.len; ++i)
	{
		message.str[i] = error.context.str[i];
	}
	message.str += error.context.len;
	message.len -= error.context.len;

	for (int i = 0; i < error.message.len; ++i)
	{
		message.str[i] = error.message.str[i];
	}

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
		else if (s.str[i] == '\t')
		{
			putchar('\\');
			putchar('t');
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
	// skip over any \r, if they are at the beginning
	while (buf->len > 0 && (*buf->str) == '\r')
	{
		++buf->str;
		--buf->len;
	}
	// remove the first newline from the string
	if (buf->len > 0 && (*buf->str) == '\n')
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

// Perhaps this return value is not well designed... it just makes the allocation the responsibility of other code
// and can't return an error message with more than one "context"
// Shouldn't be too hard to refactor if it becomes a problem
AssemblerError Assemble(s8 assembly, LMCContext* code, bool strict)
{
	assert(code);
	memset(code->mailBoxes, 0, 100*sizeof(int));

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

		if (currentInstructionPointer > 99)
			break;

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
							AssemblerError ret;
							ret.lineNumber = lineNumber;
							ret.message = S("label \t redefined");
							ret.context = label;
							return ret;
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
				AssemblerError ret;
				ret.lineNumber = lineNumber;
				ret.message = S("Unknown instruction \t");
				ret.context = label;
				return ret;
			}
		}
		else
		{
			++currentInstructionPointer;
		}
	} 

	if (strict && currentInstructionPointer > 99)
	{
		// Peter Higginson LMC does not care about this, but lmc.awk does
		// so, only check in strict mode
		AssemblerError ret;
		ret.lineNumber = lineNumber;
		ret.message = S("Program contains too many\t instructions");
		// I can't return currentInstructionPointer because it would require an allocation
		ret.context = S("");
		return ret;
	}


	lineNumber = 0;
	currentInstructionPointer = 0;
	assembly = save;

	while (!s8Equal(assembly, S("")))
	{
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
		if (opcode == -1)
		{
			// first word was not a valid mnemonic, so probably a label
			word = GetWord(&line);
			opcode = GetMnemonicValue(word);
		}

		if (opcode == -1)
		{
			// second word is still not a valid mnemonic - can't be a label, either
			AssemblerError ret;
			ret.lineNumber = lineNumber;
			ret.message = S("unknown instruction \t");
			ret.context = word;
			return ret;
		}
		bool takesAddress = ((opcode != 0) && (opcode % 100 == 0));
		s8 tmp = GetWord(&line);
		if (strict && !takesAddress && !s8Equal(tmp, S("")))
		{
			// Peter higginson LMC and lmc.awk don't care about this, so the check is "opt-in" for strict only
			AssemblerError ret;
			ret.lineNumber = lineNumber;
			ret.message = S("instruction \t does not take an address");
			ret.context = word;
			return ret;
		}
		int address = 0;
		IntegerInputError error = s8ToInteger(tmp, &address);
		if (error == NOT_A_NUMBER)
		{
			bool labelFound = false;
			for (int i = 0; i < labelCount; ++i)
			{
				if (s8Equal(labels[i].label, tmp))
				{
					labelFound = true;
					address = takesAddress ? labels[i].value : 0;
				}
			}
			if (!labelFound)
			{
				AssemblerError ret;
				ret.lineNumber = lineNumber;
				ret.message = S("undefined address label \t");
				ret.context = tmp;
				return ret;
			}
		}
		else if (error == NOT_IN_RANGE && (address < 0 || address > 99) && (opcode != 1000))
		{
			AssemblerError ret;
			ret.lineNumber = lineNumber;
			ret.message = S("address label \t is out of range 0, 99");
			ret.context = tmp;
			return ret;
		}

		if (strict)
		{
			s8 word = GetWord(&line);
			line.str -= word.len;
			line.len += word.len;
			if (!s8Equal(line, S("")))
			{
				AssemblerError ret;
				ret.lineNumber = lineNumber;
				ret.message = S("junk \t found after address");
				ret.context = line;
				return ret;
			}
		}

		// handle DAT
		if (opcode == 1000) opcode = 0;
		if (!takesAddress) address = 0;
		code->mailBoxes[currentInstructionPointer++] = opcode + address;
	}

	return (AssemblerError){0};
}

RuntimeError Step(LMCContext* code)
{
	assert(code);
	for (int i = 0; i < 100; ++i)
	{
		assert(code->mailBoxes[i] >= 0 && code->mailBoxes[i] < 1000);
	}

	if (code->programCounter > 99 || code->programCounter < 0)
	{
		return ERROR_BAD_PC;
	}

	int instruction = code->mailBoxes[code->programCounter++];

	if (instruction == 0) // HLT
	{
		return ERROR_HALT;
	}

	int opcode = instruction / 100;
	int operand = instruction % 100;

	if (opcode == 1) // ADD
	{
		code->accumulator += code->mailBoxes[operand];
		return ERROR_OK;
	}

	if (opcode == 2) // SUB
	{
		code->accumulator -= code->mailBoxes[operand];
		return ERROR_OK;
	}

	if (opcode == 3) // STA
	{
		code->mailBoxes[operand] = code->accumulator;
		return ERROR_OK;
	}

	if (opcode == 4) // Unused
	{
		return ERROR_BAD_INSTRUCTION;
	}

	if (opcode == 5) // LDA
	{
		code->accumulator = code->mailBoxes[operand];
		return ERROR_OK;
	}

	if (opcode == 6) // BRA
	{
		code->programCounter = operand;
		return ERROR_OK;
	}

	if (opcode == 7) // BRZ
	{
		if (code->accumulator == 0)
		{
			code->programCounter = operand;
		}
		return ERROR_OK;
	}

	if (opcode == 8)
	{
		if (code->accumulator >= 0)
		{
			code->programCounter = operand;
		}
		return ERROR_OK;
	}

	if (opcode == 9)
	{
		if (operand == 1) // INP
		{
			// UNIMPLEMENTED
		}

		if (operand == 2) // OUT
		{
			printf("%d\n", code->accumulator);
			return ERROR_OK;
		}

		if (operand == 22) // OTC
		{
			putchar((unsigned char) code->accumulator);
			return ERROR_OK;
		}

		return ERROR_BAD_INSTRUCTION;
	}
	return ERROR_BAD_INSTRUCTION;
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

		char* p = alloc(&a, 512);
		assert(p);
		p = alloc(&a, 256);
		assert(p);
		p = alloc(&a, 256);
		assert(p);
		p = alloc(&a, 2048);
		assert(!p);

		a = free;
		p = alloc(&a, 1025);
		assert(!p);
	}
}

#else

int main(int argc, char* argv[])
{
//	s8 program = S("        lda space\n sta char\n loop    lda char\n out\n lda space\n otc\n lda char\n otc\n add one\n sta char\n sub max\n brz end\n bra loop\n end     hlt\n space   dat 32\n one     dat 1\n max     dat 97\n char    dat\n // start of ASCII character table\n");
//	s8 program = S("        LDA lda1\n STA outputList\n LDA sta1\n STA store\n LDA zero\n STA listSize\n inputLoop INP\n BRZ resetLoop\n store   DAT 380\n LDA store\n ADD increment\n STA store\n LDA listSize\n ADD increment\n STA listSize\n BRA inputLoop\n resetLoop LDA lda1\n STA load1\n ADD increment\n STA load2\n LDA sta1\n STA store1\n ADD increment\n STA store2\n LDA listSize\n SUB increment\n STA loopCount\n LDA zero\n STA isChange\n load1   DAT 580\n STA buffA\n load2   DAT 581\n STA buffB\n cmp     SUB buffA\n BRP nextItem\n swap    LDA buffB\n store1  DAT 380\n LDA buffA\n store2  DAT 381\n LDA increment\n STA isChange\n nextItem LDA store1\n ADD increment\n STA store1\n ADD increment\n STA store2\n LDA load1\n ADD increment\n STA load1\n ADD increment\n STA load2\n LDA loopCount\n SUB increment\n STA loopCount\n BRZ isFinished\n BRA load1\n isFinished LDA isChange\n BRZ outputList\n bra resetLoop\n outputList DAT 580\n OUT\n LDA outputList\n ADD increment\n STA outputList\n LDA listSize\n SUB increment\n STA listSize\n BRZ end\n BRA outputList\n end     HLT\n zero    DAT 0\n buffA   DAT 0\n buffB   DAT 0\n isChange DAT 0\n increment DAT 1\n listSize DAT 0\n loopCount DAT 0\n sta1    DAT 380\n lda1    DAT 580\n");
//	s8 program = S("        INP\n STA VALUE\n LDA ZERO\n STA TRINUM\n STA N\n LOOP    LDA TRINUM\n SUB VALUE\n BRP ENDLOOP\n LDA N\n ADD ONE\n STA N\n ADD TRINUM\n STA TRINUM\n BRA LOOP\n ENDLOOP LDA VALUE\n SUB TRINUM\n BRZ EQUAL\n LDA ZERO\n OUT\n BRA DONE\n EQUAL   LDA N\n OUT\n DONE    HLT\n VALUE   DAT\n TRINUM  DAT\n N       DAT\n ZERO    DAT 000\n ONE     DAT 001\n // Test if input is a triangular number\n // If is sum of 1 to n output n\n // otherwise output zero\n");
//	s8 program = S("        INP\n STA VALUE\n LDA ONE\n STA MULT\n OUTER   LDA ZERO\n STA SUM\n STA TIMES\n INNER   LDA SUM\n ADD VALUE\n STA SUM\n LDA TIMES\n ADD ONE\n STA TIMES\n SUB MULT\n BRZ NEXT\n BRA INNER\n NEXT    LDA SUM\n OUT\n LDA MULT\n ADD ONE\n STA MULT\n SUB VALUE\n BRZ OUTER\n BRP DONE\n BRA OUTER\n DONE    HLT\n VALUE   DAT 0 // Times table for\n MULT    DAT 0 // one input number\n SUM     DAT\n TIMES   DAT\n COUNT   DAT\n ZERO    DAT 000\n ONE     DAT 001");

	//s8 program = S("\n\n\nDat 10\n\n");
	//s8 program = S("loop    lda counter\n add one\n otc\n sta counter\n sub begin\n sub twentysix\n brp end\n bra loop\n end     HLT\n counter DAT 96\n one     DAT 1\n twentysix DAT 26\n begin   DAT 96\n ");
//	s8 program = S("        lda space\n sta char\n loop    lda char\n otc\n add one\n sta char\n sub max\n brz end\n bra loop\n end     hlt\n space   dat 32\n one     dat 1\n max     dat 127\n char    dat\n // output the basic ASCII characters\n ");
	s8 program;
	int fd = s8FileMap(argv[1], &program);
	if (fd == -1)
	{
		return 1;
	}
	
	LMCContext x = {0} ;

	AssemblerError ret = Assemble(program, &x, true);

	char mem[1<<17];
	arena a;
	a.beginning = &mem[0];
	a.end = &mem[sizeof(mem)];

	if (ret.lineNumber != 0)
	{
		printf("%d\n", ret.lineNumber);
		s8 message = s8AssemblerError(ret, &a);
		s8Print(message);
	}

	for (int i = 0; i < 10; ++i)
	{
		for (int j = 0; j < 9; ++j)
		{
			printf("%03i ", x.mailBoxes[i*10+j]);
		}
		printf("%03i", x.mailBoxes[i*10+9]);
		printf("\n");
	}

	while (true)
	{
		RuntimeError ret = Step(&x);

		if (ret != ERROR_OK) break;
	}
	printf("\n");
	s8FileUnmap(fd, program);
}
#endif
