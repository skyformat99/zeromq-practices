// server.cpp
//
// Hello-world server binding REP socket
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zmq.h>

int main()
{
    void* ctx = zmq_ctx_new();
    // Create and bind
    void* rep = zmq_socket(ctx, ZMQ_REP);
    zmq_bind(rep, "tcp://*:5555");
    // Receiving
    char buf[16];
    while (1) {
        int size = zmq_recv(rep, buf, 16, 0);
        buf[size] = '\0';
        fprintf(stdout, "# receiving '%s'\n", buf);
        const char msg[] = "world";
        zmq_send(rep, msg, strlen(msg), 0);
    }
    // Destroy and cleanup
    zmq_close(rep);
    zmq_ctx_destroy(ctx);

    return 0;
}
