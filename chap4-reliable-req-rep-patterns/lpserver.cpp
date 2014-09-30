/**
 * @file lpserver.cpp
 *
 * @breif Lazy Pirate server
 * Binds REP socket, resembles hwserver except:
 *  - echo request as-is
 *  - randomly run slowly, or exit to simulate a crash
 */
#include <czmq.h>
#include <assert.h>

#include "time_util.h"

namespace {

const char SERVER_ENDPOINT[] = "tcp://*:5555";

}

int main()
{
    zctx_t* ctx = zctx_new();
    void* server = zsocket_new(ctx, ZMQ_REP);
    zsocket_bind(server, SERVER_ENDPOINT);
    srandom((unsigned int)time(0));

    int cycles = 0;
    while (1) {
        char* request = zstr_recv(server);
        cycles++;

        // Simulate various problems, after a few cycles
        if (cycles > 3 && randof(3) == 0) {
            printf("I: simulating a crash\n");
            break;
        } else if (cycles > 3 && randof(3) == 0) {
            printf("I: simulating CPU overload\n");
            msleep(2000);
        }
        printf("I: normal request (%s)\n", request);
        msleep(1000);
        zstr_send(server, request);
        free(request);
    }

    zctx_destroy(&ctx);
    return 0;
}
