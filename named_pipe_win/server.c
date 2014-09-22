/**
 * @file server.c
 *
 * @breif Server-side that creates the named pipe and listens for client
 * connections.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

int main(int argc, char* argv[])
{
    TCHAR pipe_name[256] = {0};
    if (argc != 2) {
        printf("Usage: %s pipe_name\n", argv[0]);
        return 0;
    }
    _stprintf(pipe_name, _T("\\\\.\\pipe\\%S"),
            argc >= 2 ? argv[1] : "named-pipe");

    _tprintf(_T("I: preparing pipe server at '%ls'...\n"), pipe_name);
    while (1) {
        HANDLE pipe = CreateNamedPipe(
                pipe_name,
                PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND,
                PIPE_WAIT,
                1,
                1024,
                1024,
                1 * 1000,  // default timeout for "WaitNamedPipe" call
                NULL);
        if (pipe == INVALID_HANDLE_VALUE) {
            printf("E: failed to create named pipe - %d", GetLastError());
            break;
        }
        char buf[1024] = {0};
        DWORD len = 0;

        ConnectNamedPipe(pipe, NULL);
        ReadFile(pipe, buf, 1024, &len, NULL);
        if (len > 0) {
            buf[len] = 0;
            printf("I: message received - %s\n", buf);
        }
        CloseHandle(pipe);
    }
}
