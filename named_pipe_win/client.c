/**
 * @file client.c
 *
 * @breif Client-side that connects to a known named pipe, sends messages and
 * output replies.
 */
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
    TCHAR pipe_name[256] = {0};
    if (argc != 2) {
        printf("Usage: %s pipe_name\n", argv[0]);
        return 0;
    }
    _stprintf(pipe_name, _T("\\\\.\\pipe\\%S"),
            argc >= 2 ? argv[1] : "named-pipe");

    if (!WaitNamedPipe(pipe_name, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(_T("E: named pipe '%ls' not avaiable - %d\n"), pipe_name,
                GetLastError());
        return 1;
    }
    _tprintf(_T("I: connecting named pipe at '%ls'...\n"), pipe_name);
    HANDLE pipe = CreateFile(
            pipe_name,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        printf("E: failed to connect to the named pipe - %d\n", GetLastError());
        return -1;
    }
    char buf[] = "hello from client";
    DWORD len = strlen(buf);
    printf("I: sending message '%s'...\n", buf);
    WriteFile(pipe, buf, len, &len, NULL);
    printf("I: %d bytes sent\n", len);
    CloseHandle(pipe);

    return 0;
}
