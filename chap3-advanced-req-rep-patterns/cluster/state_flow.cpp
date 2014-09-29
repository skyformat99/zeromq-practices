/**
 * @file state_flow.cpp
 *
 * @breif Broker peering simulation (part 1)
 * Prototypes of the state flow
 */
#include <czmq.h>
#include <stdlib.h>

#include "endpoints.h"

int main(int argc, char* argv[])
{
    // First argument is this broker's name
    // Other arguments are our peers' name
    if (argc < 2) {
#ifdef WIN32
        printf("syntax: state_flow me_port {other_port}...\n");
#else
        printf("syntax: state_flow me {other}...\n");
#endif // WIN32
        exit(0);
    }
    char* self = argv[1];
    printf("I: preparing broker at %s...\n", self);
    srand((unsigned)time(0));
    
    zctx_t* ctx = zctx_new();

    // Bind state backend to endpoint
    void* statebe = zsocket_new(ctx, ZMQ_PUB);
    int rc = zsocket_bind(statebe, endpoints::state(self));
    if (rc == -1) {
        int last_errno = zmq_errno();
        char* endpoint = zsocket_last_endpoint(statebe);
        printf("I: failed to bind PUB socket to '%s' - %s\n", endpoint,
            zmq_strerror(last_errno));
        free(endpoint);
        zctx_destroy(&ctx);
        exit(1);
    }
    
    // Connect statefe to all peers
    void* statefe = zsocket_new(ctx, ZMQ_SUB);
    zsocket_set_subscribe(statefe, "");
    for (int i = 2; i < argc; ++i) {
        char* peer = argv[i];
        printf("I: connecting to state backend at %s...\n", peer);
        zsocket_connect(statefe, endpoints::state(peer));
    }
    
    // The main loop sends out messages to peers, and collects status messages
    // back from peers. The zmq_poll timeout defines the heartbeat interval.
    
    zmq_pollitem_t items[] = {
        { statefe, 0, ZMQ_POLLIN, 0 }
    };
    while (true) {
        int rc = zmq_poll(items, 1, 1000 * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;  // Interrupted
        if (items[0].revents & ZMQ_POLLIN) {
            // Handle incoming status messages
            char* peer_name = zstr_recv(statefe);
            char* available = zstr_recv(statefe);
            printf("%s - %s workers free\n", peer_name, available);
            free(peer_name);
            free(available);
        } else {
            // Send random values for worker availability
            int available = rand() % 11;
            printf("I: sending status report %d...\n", available);
            zstr_sendm(statebe, self);
            zstr_sendf(statebe, "%d", available);
        }
    }
    
    // Cleanup
    zctx_destroy(&ctx);
    return EXIT_SUCCESS;
}
