#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>

static bool IsDigit(char c)
{
	return (c >= '0' && c <= '9');
}

static bool IsWhiteSpace(char c)
{
	return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

bool InpCallbackDefault(int* input, void* ctx)
{
	(void) ctx; // unused since reading from stdin
	char digit;

	bool negate = false;
	unsigned int value = 0;
	unsigned int limit = INT_MAX;
	int bytesRead;

	// Consume all whitespace, up until the first non-whitespace
	while ((bytesRead = read(STDIN_FILENO, &digit, 1) > 0) && IsWhiteSpace(digit)){}

	// EOF, from pipe
	// not possible to do anything useful
	if (bytesRead == 0)
		exit(2);

	switch (digit)
	{
		case '-':
			negate = true;
			limit = 0x80000000;
			break;
		case '+':
			break;
		default:
			if (!IsDigit(digit))
			{
				goto error;
			}
			value += digit - '0';
			break;
	}
	while ((bytesRead = read(STDIN_FILENO, &digit, 1) > 0))
	{
		if (IsWhiteSpace(digit))
		{
			break;
		}
		else if (IsDigit(digit))
		{
			int d = digit - '0';
			// check overflow
			if (value > (limit - d)/10)
			{
				goto error;
			}
			value = value*10 + d;
		}
		else
		{
			goto error;
		}
	}

	if (negate) value *= -1;
	*input = value;
	return true;

error:
			while ((bytesRead = read(STDIN_FILENO, &digit, 1) > 0) && !IsWhiteSpace(digit)){}
			return false;
}

/*
int main(void)
{
	int x;
	while(true)
	{
		if (ReadIntCallbackDefault(&x, 0))
		{
			printf("%d\n", x);
		}
		else
		{
			printf("BAD\n");
		}
	}

}*/
