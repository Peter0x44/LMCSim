#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	int mailBoxes[100]; // main memory
	int accumulator;
	int programCounter;
	int outbox;
	bool wantsInput;
	bool wantsOutput;
} LMCContext;

typedef struct
{
	unsigned char* str;
	ptrdiff_t len;
} s8;

// Types of errors, could be bad assembly, or something else
typedef enum
{
	lllllll
} AssemblerError;

typedef enum
{
	ERROR_BAD_PC, // ex. type in number that is not in the range (-999, 999)
	ERROR_HALT // Halt instruction reached - program execution finished
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
