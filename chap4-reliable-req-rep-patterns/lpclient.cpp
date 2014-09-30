/**
 * @file lpclient.cpp
 *
 * @breif Lazy Pirate Client
 * Use zmq_poll to do a safe request-reply
 * To run, start lpserver and then randomly kill/restart it
 */
#include <czmq.h>

namespace {

const int REQUEST_TIMEOUT = 2500;  // msec, (> 1000!)
const int REQUEST_RETRIES = 3;     // before abandon
const char SERVER_ENDPOINT[] = "tcp://127.0.0.1:5555";

}

int main()
{
    zctx_t* ctx = zctx_new();
    printf("I: connecting to server '%s'...\n", SERVER_ENDPOINT);
    void* client = zsocket_new(ctx, ZMQ_REQ);
    zsocket_connect(client, SERVER_ENDPOINT);
    srandom((unsigned int)time(0));

    int interrupted = 0;
    int retries_left = REQUEST_RETRIES;
    int request_seq = 0;
    char request[32] = { 0 };
    while (retries_left && !interrupted) {
        sprintf(request, "%04d-%0x", ++request_seq, randof(0x10000));
        zstr_send(client, request);

        int expect_reply = 1;
        while (expect_reply && !interrupted) {
            // Poll client for reply, with timeout
            zmq_pollitem_t items[] = {
                { client, 0, ZMQ_POLLIN, 0 }
            };
            int rc = zmq_poll(items, 1, REQUEST_TIMEOUT * ZMQ_POLL_MSEC);
            if (rc == -1) {
                interrupted = 1;
                break;  // Interrupted
            }
            if (items[0].revents & ZMQ_POLLIN) {
                char* reply = zstr_recv(client);
                if (!reply) {
                    interrupted = 1;
                    break;  // Interrupted
                }
                if (strcmp(reply, request) == 0) {
                    printf("I: server replied ok (%s)\n", reply);
                    retries_left = REQUEST_RETRIES;
                    expect_reply = 0;
                } else if (retries_left--) {
                    printf("E: malformed reply from server: %s\n", reply);
                }
                free(reply);
            } else if (--retries_left == 0) {
                printf("E: server seems to be offline, abandoning\n");
                break;
            } else {
                printf("W: no response from server, retrying...\n");
                // Old socket is confused, close it and open an new one
                zsocket_destroy(ctx, client);
                printf("I: reconnecting to server...\n");
                client = zsocket_new(ctx, ZMQ_REQ);
                zsocket_connect(client, SERVER_ENDPOINT);
                // Resend the request
                zstr_send(client, request);
            }
        }
    }

    zctx_destroy(&ctx);
    return 0;
}
