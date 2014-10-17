// @file mdclient.c
//
// Majordomo Protocol client example
// Uses the mdcli API to hide all MDP aspects
//
#include "mdcliapi.h"

int main(int argc, char* argv[])
{
    int verbose = argc > 1 && streq(argv[1], "-v");
    mdcli_t* session = mdcli_new("tcp://127.0.0.1:5555", verbose);

    int count;
    for (count = 0; count < 100000; count++) {
        zmsg_t* request = zmsg_new();
        zmsg_pushstrf(request, "#%05d hello world", count);
        zmsg_t* reply = mdcli_send(session, "echo", &request);
        if (reply)
            zmsg_destroy(&reply);
        else
            break;
    }
    printf("%d requests/replies processed\n", count);
    mdcli_destroy(&session);
    return 0;
}
