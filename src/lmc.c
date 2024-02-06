#include "lmc.h"
#include "mapfile.h"

#define S(s) (s8) { (unsigned char*)s, (ptrdiff_t)(sizeof(s)-1) }

#ifdef DEBUG

// continuable assertion
#define assert(e) do { if (!(e)) asm volatile("int3; nop"); } while (0)

#elif defined(__has_builtin)
	#if __has_builtin(__builtin_unreachable)
		#define assert(e) do { if (!(e)) __builtin_unreachable(); } while (0) // optimization hint
	#else
		#define assert(e) ((void)(0)) // I think this is needed to handle a "unreachable-less compiler with builtins"
	#endif
#else

#define assert(e) ((void)(0)) // this is what glibc does

#endif

#include <stdbool.h>
#include <limits.h>

static bool s8Equal(s8 a, s8 b)
{
	if (a.len != b.len) return false;

	for (int i = 0; i < a.len; ++i)
	{
		if (a.str[i] != b.str[i])
			return false;
	}

	return true;
}


static bool IsDigit(unsigned char c)
{
	return ((c >= '0') && (c <= '9'));
}

typedef enum
{
	NOT_A_NUMBER = 1,
	NOT_IN_RANGE = 2,
} IntegerInputError;

static IntegerInputError s8ToInteger(s8 s, int* result)
{
	int i = 0;
	bool negate = false;
	unsigned int value = 0;
	unsigned int limit = INT_MAX;

	switch (*s.str)
	{
		case '-':
			i = 1;
			negate = true;
			limit = 0x80000000;
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
	if (negate) value *= -1;

	*result = value;
	return 0;
}

static bool IsLower(unsigned char c)
{
	return (c >= 'a' && c <= 'z');
}

static unsigned char ToUpper(unsigned char c)
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
static bool s8iEqual(s8 a, s8 b);

static int GetMnemonicValue(s8 mnemonic)
{
	if (false) {}

	else if (s8iEqual(mnemonic, mnemonics[0 ])  // HLT
		|| s8iEqual(mnemonic, S("COB")))    // COB
		return 000;
	else if (s8iEqual(mnemonic, mnemonics[1 ])) // ADD
		return 100;
	else if (s8iEqual(mnemonic, mnemonics[2 ])) // SUB
		return 200;
	else if (s8iEqual(mnemonic, mnemonics[3 ])  // STA
		|| s8iEqual(mnemonic, S("STO")))    // STO
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

static bool IsWhitespace(char c)
{
	return ((c == ' ') || (c == '\t') || (c == '\r'));
}

static s8 StripWhitespace(s8 line)
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

static bool IsCommentToken(char c)
{
	return ((c == '#') || (c == '/') || (c == ';'));
}

// Strips comments from the end of the line
// 3 tokens are supported: // ; #
static s8 StripComment(s8 line)
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

static bool IsNewline(char c)
{
	return ((c == '\n') || (c == '\r'));
}

// Extract next line of "buf". Modifies buf to point to the next line too, so it can be called repeatedly until it returns an empty string
static s8 GetLine(s8* buf)
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

static s8 GetWord(s8* line)
{
	assert(line);

	// skip over leading whitespace
	while (line->len > 0 && IsWhitespace(*line->str))
	{
		++line->str;
		--line->len;
	}


	// Find the next whitespace, and end the string before it
	for (int i = 0; i < line->len; ++i)
	{
		if (IsWhitespace(line->str[i]))
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

	// No whitespace found - last word or end of line
	s8 ret = *line;
	line->len -= ret.len;
	line->str += ret.len;
	return ret;
}

// Case insensitive comparison
static bool s8iEqual(s8 a, s8 b)
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

// Perhaps this return value is not well designed... it just makes the allocation the responsibility of other code
// and can't return an error message with more than one "context"
// Shouldn't be too hard to refactor if it becomes a problem
AssemblerError Assemble(s8 assembly, LMCContext* code, bool strict)
{
	assert(code);
	for (int i = 0; i < 100; ++i) code->mailBoxes[i] = 0;

	// static buffer to hold error messages
	// could allocate out of a passed arena, but that would be extra effort for callers
	// I think static is okay for this case
	static unsigned char mem[1<<14];
	buf buffer;
	buffer.buf = &mem[0];
	buffer.capacity = sizeof(mem);
	buffer.len = 0;
	buffer.error = 0;

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
							appends8(&buffer, S("label \""));
							appends8(&buffer, label);
							appends8(&buffer, S("\" redefined"));
							ret.message = bufTos8(&buffer);
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
				appends8(&buffer, S("unknown instruction \""));
				appends8(&buffer, label);
				appends8(&buffer, S("\""));
				ret.message = bufTos8(&buffer);
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
		// The previous loop breaks when currentInstructionPointer > 99
		// So, I can't write many instructions were in the program without some refactoring
		// maybe a TODO, or more likely a waste of time.
		appends8(&buffer, S("program contains more than 100 instructions."));
		ret.message = bufTos8(&buffer);
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
			appends8(&buffer, S("unknown instruction \""));
			appends8(&buffer, word);
			appends8(&buffer, S("\""));
			ret.message = bufTos8(&buffer);
			return ret;
		}
		bool takesAddress = ((opcode != 0) && (opcode % 100 == 0));
		s8 tmp = GetWord(&line);
		if (strict && !takesAddress && !s8Equal(tmp, S("")))
		{
			// Peter higginson LMC and lmc.awk don't care about this, so the check is "opt-in" for strict only
			AssemblerError ret;
			ret.lineNumber = lineNumber;
			appends8(&buffer, S("instruction \""));
			appends8(&buffer, word);
			appends8(&buffer, S("\" does not take an address"));
			ret.message = bufTos8(&buffer);
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
				appends8(&buffer, S("undefined address label \""));
				appends8(&buffer, tmp);
				appends8(&buffer, S("\""));
				ret.message = bufTos8(&buffer);
				return ret;
			}
		}
		// Don't allow address values outside of 0-99, except for usage with DAT
		else if (error == NOT_IN_RANGE || ((address < 0 || address > 99) && (opcode != 1000)))
		{
			AssemblerError ret;
			ret.lineNumber = lineNumber;
			appends8(&buffer, S("address label \""));
			appends8(&buffer, tmp);
			appends8(&buffer, S("\" is out of range [0, 100)"));
			ret.message = bufTos8(&buffer);
			return ret;
		}

		if (strict)
		{
			s8 junk = GetWord(&line);
			line.str -= junk.len;
			line.len += junk.len;
			if (!s8Equal(line, S("")))
			{
				AssemblerError ret;
				ret.lineNumber = lineNumber;
				appends8(&buffer, S("junk \""));
				appends8(&buffer, line);
				appends8(&buffer, S("\" found after address"));
				ret.message = bufTos8(&buffer);
				return ret;
			}
		}

		// handle DAT
		if (opcode == 1000) opcode = 0;
		if (!takesAddress) address = 0;
		code->mailBoxes[currentInstructionPointer++] = opcode + address;
	}

	return (AssemblerError){ -1, {0,0} };
}

RuntimeError Step(LMCContext* code)
{

	if (code->programCounter > 99 || code->programCounter < 0)
	{
		return ERROR_BAD_PC;
	}

	int instruction = code->mailBoxes[code->programCounter];


	if (instruction == 0) // HLT
	{
		return ERROR_HALT;
	}

	RuntimeError ret = ERROR_OK;

	int opcode = instruction / 100;
	int operand = instruction % 100;

	if (opcode == 1) // ADD
	{
		code->accumulator += code->mailBoxes[operand];
	}

	else if (opcode == 2) // SUB
	{
		code->accumulator -= code->mailBoxes[operand];
	}

	else if (opcode == 3) // STA
	{
		code->mailBoxes[operand] = code->accumulator;
	}

	else if (opcode == 4) // Unused
	{
		ret = ERROR_BAD_INSTRUCTION;
	}

	else if (opcode == 5) // LDA
	{
		code->accumulator = code->mailBoxes[operand];
	}

	else if (opcode == 6) // BRA
	{
		code->programCounter = operand;
		// return here to avoid incrementing PC
		return ret;
	}

	else if (opcode == 7) // BRZ
	{
		if (code->accumulator == 0)
		{
			code->programCounter = operand;
			// return here to avoid incrementing PC
			return ret;
		}
	}

	else if (opcode == 8) // BRP
	{
		if ((int)code->accumulator >= 0)
		{
			code->programCounter = operand;
			// return here to avoid incrementing PC
			return ret;
		}
	}


	else if (opcode == 9)
	{
		// These need to interact with external state
		// Need some ideas on how to get it - maybe use function pointer callbacks???
		if (operand == 1) // INP
		{
			int input;
			bool didParse = (*code->inpFunction)(&input, code->inputCtx);
			if (didParse)
			{
				code->accumulator = input;
			}
			else
			{
				ret = ERROR_BAD_INPUT;
			}

		}

		else if (operand == 2) // OUT
		{
			unsigned char mem[12];
			buf buffer;
			buffer.buf = &mem[0];
			buffer.capacity = sizeof(mem);
			buffer.len = 0;
			buffer.error = 0;

			appendInteger(&buffer, (int)code->accumulator);
			appendChar(&buffer, '\n');
			s8 s = bufTos8(&buffer);

			(*code->outFunction)(s.str, s.len, code->outputCtx);
		}

		else if (operand == 22) // OTC
		{
			unsigned char c = code->accumulator;
			s8 s = (s8){&c, 1};
			(*code->outFunction)(s.str, s.len, code->inputCtx);
		}
		else
		{
			ret = ERROR_BAD_INSTRUCTION;
		}
	}
	else
	{
		ret  = ERROR_BAD_INSTRUCTION;
	}

	if (ret == ERROR_OK)
	{
		++code->programCounter;
	}
	return ret;
}

s8 RuntimeError_StrError(LMCContext* code, RuntimeError error)
{
	static unsigned char mem[60];
	buf buffer;
	buffer.buf = &mem[0];
	buffer.capacity = sizeof(mem);
	buffer.len = 0;
	buffer.error = 0;

	switch (error)
	{
		case ERROR_OK:
			return (s8) { 0, 0 }; // no error to report
		case ERROR_HALT:
			break; // normal program termination
		case ERROR_BAD_PC:
			appends8(&buffer, S("Bad PC value "));
			appendInteger(&buffer, code->programCounter);
			break;
		case ERROR_BAD_INSTRUCTION:
			appends8(&buffer, S("Bad instruction value "));
			appendInteger(&buffer, code->mailBoxes[code->programCounter]);
			appends8(&buffer, S(" at "));
			appendInteger(&buffer, code->programCounter);
			break;
		default:
			break;
	}
	return bufTos8(&buffer);
}


#ifdef TEST

#include <stdio.h>
#include <string.h>

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
		s8 test = S("this is a  \t \r line");
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

int main(int argc, char* argv[])
{
	if (argc <= 1) return 0;
	s8 program = s8FileMap(argv[1]);

	if (!program.str) return 2; //s8Equal(program, S(""))) return 1;
	
	LMCContext x = {0} ;
	extern bool InpCallbackDefault(int* input, void* ctx);
	x.inpFunction = InpCallbackDefault;
	extern void OutCallbackDefault(unsigned char* str, ptrdiff_t len, void* ctx);
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
#endif
