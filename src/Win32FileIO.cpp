#ifndef WIN32_MEMORY_CPP
#define WIN32_MEMORY_CPP

#include <windows.h>

struct win32_file
{
    size_t size;
    void *data;
};

static inline void *Win32Alloc(size_t size)
{
    void *result = (void *)VirtualAlloc(NULL, (SIZE_T)size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return (result);
}

static void Win32FileWrite(char *filename, void *data, unsigned int size)
{
    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hFile)
    {
        DWORD NumberOfBytesWritten;
        WriteFile(
            hFile,
            data,
            size,
            &NumberOfBytesWritten,
            NULL);
    }
    CloseHandle(hFile);
}

static win32_file Win32FileRead(char *filename)
{
    win32_file result = {};
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    size_t size = GetFileSize(hFile, NULL);
    if (!size)
    {
        DWORD dwError = GetLastError();
    }
    void *data = Win32Alloc(size);
    DWORD NumberOfBytesRead = 0;
    if (hFile)
    {
        if (!ReadFile(hFile, data, (DWORD)size, &NumberOfBytesRead, NULL))
        {
            DWORD dwError = GetLastError();
        }
    }
    CloseHandle(hFile);
    result.size = size;
    result.data = data;
    return (result);
}

#endif