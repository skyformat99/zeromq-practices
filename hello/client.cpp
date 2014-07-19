// client.cpp
//
// Hello-world client that connects using DEALER socket
//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int main()
{
    void* ctx = zmq_ctx_new();
    // Create and connect
    void* req = zmq_socket(ctx, ZMQ_DEALER);
    zmq_connect(req, "tcp://localhost:5555");
    // Send 3 hello request in a row(which is not possible in REQ-REP pattern)
    const int req_nums = 3;
    for (int i = 0; i < req_nums; ++i) {
        const char msg[] = "hello";
        fprintf(stdout, "# sending '%s'\n", msg);
        // First frame is empty, to simulate REQ socket
        zmq_send(req, "", 0, ZMQ_SNDMORE);
        zmq_send(req, msg, strlen(msg), 0);
    }
    // Read 3 replies
    char buf[16];
    for (int i = 0; i < req_nums; ++i) {
        int size = zmq_recv(req, buf, 16, 0);
        buf[size] = '\0';
        // assert(size == 0);
        if (size != 0) {
            fprintf(stdout, "#%d receiving invalid message '%s'\n", i, buf);
        }
        while (1) {
            int more = 0;
            size_t option_len = 0;
            zmq_getsockopt(req, ZMQ_RCVMORE, &more, &option_len);
            if (!more) break;
            int len = zmq_recv(req, buf, 16, 0);
            buf[len] = '\0';
            fprintf(stdout, "#%d receiving '%s'\n", i, buf);
        }
    }
    // Close and cleanup
    zmq_close(req);
    zmq_ctx_destroy(ctx);

    return 0;
}
