#include "../mapfile.h"


// Manual definitions to avoid including windows.h (worthless trash)

#define GENERIC_READ 0x80000000
#define FILE_SHARE_READ   1
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_READONLY 1
#define PAGE_READONLY  2
#define INVALID_HANDLE_VALUE ((void*)-1)
#define FILE_MAP_READ 4


__declspec(dllimport) __stdcall
void* CreateFileA(
		const char* lpFileName,
		unsigned int dwDesiredAccess,
		unsigned int dwShareMode,
		void* lpSecurityAttributes,
		unsigned int dwCreationDisposition,
		unsigned int dwFlagsAndAttributes,
		void* hTemplateFile
	);

__declspec(dllimport) __stdcall
unsigned int GetFileSize(void* hFile, unsigned int* lpFileSizeHigh);

__declspec(dllimport) __stdcall
int CloseHandle(void* hObject);

__declspec(dllimport) __stdcall
void* CreateFileMappingA(
		void* hFile,
		void* lpFileMappingAttributes,
		unsigned int flProtect,
		unsigned int dwMaximumSizeHigh,
		unsigned int dwMaximumSizeLow,
		char* lpName
	);

__declspec(dllimport) __stdcall
void* MapViewOfFile(
		void* hFileMappingObject,
		unsigned int dwDesiredAccess,
		unsigned int dwFileOffsetHigh,
		unsigned int dwFileOffsetLow,
#ifdef _WIN64
		unsigned long long int dwNumberOfBytesToMap
#else
		unsigned long int dwNumberOfBytesToMap
#endif
	);



// returns the contents of file
// TODO maybe call CreateFileW instead.
static s8 s8FileMap(const char* fileName)
{
	s8 contents = (s8) { 0, 0 };
	if (!fileName) return contents;

	void* fd = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, 0);

	if (fd == INVALID_HANDLE_VALUE) return contents;

	unsigned int fileLen = GetFileSize(fd, 0);

	void* mappingHandle = CreateFileMappingA(fd, 0, PAGE_READONLY, 0, fileLen, 0);
	CloseHandle(fd);

	if (!mappingHandle) return contents;

	void* fileContents = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, fileLen);
	CloseHandle(mappingHandle);

	if (!fileContents)
	{
		return contents;
	}

	contents.len = fileLen;
	contents.str = fileContents;

	return contents;
}
