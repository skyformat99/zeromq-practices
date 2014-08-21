/**
 * @file pub.cpp
 *
 * @breif Test server publishes random messages.
 */
#include <czmq.h>
#include <stdlib.h>

#include "time_util.h"

int main(int argc, char* argv[])
{
    char name[] = "pub";
    srand((unsigned)time(0));
    zctx_t* ctx = zctx_new();
    void* pub = zsocket_new(ctx, ZMQ_PUB);
    //zsocket_bind(pub, "ipc://%s-server.ipc", name);
    zsocket_bind(pub, "ipc://pub-server.ipc");
    printf("I: pub server is up\n");
    while (true) {
        int load = rand() % 11;
        printf("I: sending load %d...\n", load);
        int rc = zstr_sendm(pub, name);
        if (rc == -1)
            break;
        rc = zstr_sendf(pub, "%d", load);
        if (rc == -1)
            break;
        msleep(1000);
    }
    zctx_destroy(&ctx);
    return EXIT_SUCCESS;
}
