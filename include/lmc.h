#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef bool(*ReadIntCallback)(int* input, void* ctx);
typedef void(*WriteIntCallback)(int c, void* ctx);
typedef void(*WriteCharCallback)(char c, void* ctx);

typedef struct
{
	int mailBoxes[100]; // main memory
	unsigned int accumulator;
	int programCounter;
	void* inputCtx;
	ReadIntCallback inpFunction;
	WriteIntCallback outFunction;
	WriteCharCallback otcFunction;
} LMCContext;

typedef struct
{
	unsigned char* str;
	ptrdiff_t len;
} s8;

// Types of errors, could be bad assembly, or something else
typedef struct
{
	int lineNumber;
	s8 message;
} AssemblerError;

typedef enum
{
	ERROR_OK, // Instruction ran normally
	ERROR_BAD_PC, // PC value isn't valid - reached out of range
	ERROR_HALT, // Halt instruction reached - program execution finished
	ERROR_BAD_INSTRUCTION, // Bad instruction executed (example: 4xx)
	ERROR_BAD_INPUT // Bad integer input given - either not an integer, or overflowed
} RuntimeError;
// errors that can happen during runtime: example PC value is outside of [0, 99]

#ifdef __cplusplus
extern "C" {
#endif

// Assemble string assembly into LMCContext code
AssemblerError Assemble(s8 assembly, LMCContext* code, bool strict);

// Execute next instruction of code
RuntimeError Step(LMCContext* code);

#ifdef __cplusplus
}
#endif
