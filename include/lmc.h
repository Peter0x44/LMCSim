#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef bool(*InpCallback)(int* input, void* ctx);
typedef void(*OutCallback)(unsigned char* str, ptrdiff_t len, void* ctx);

typedef struct
{
	int mailBoxes[100]; // main memory
	unsigned int accumulator;
	int programCounter;
	void* inputCtx;
	void* outputCtx;
	InpCallback inpFunction;
	OutCallback outFunction;
} LMCContext;

typedef struct
{
	unsigned char* str;
	ptrdiff_t len;
} s8;

// a bit of an API inconsistency to return a struct from one function and an enum from another - oh well
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

// Get error string from runtime error
s8 RuntimeError_StrError(LMCContext* code, RuntimeError error);

#ifdef __cplusplus
}
#endif
