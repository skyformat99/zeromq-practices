/**
 * @file hello.cpp
 *
 * @breif Simple REQ-REP pattern with emulated-REQ with DEALER.
 */
#include <zmq.hpp>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "time_util.h"

namespace {

const int SERVER_NUM = 3;
const int REQUEST_NUM = 5;

zmq::context_t ctx;

const char QUIT_MSG[] = "[quit]";
const char DONE_MSG[] = "[done]";

}

static void* server_task(void* arg)
{
    int seq = (int)arg;
    //zmq::context_t ctx;
    zmq::socket_t rep(ctx, ZMQ_REP);
    char addr[64];
    sprintf(addr, "inproc://hello_server_%d", seq);
    rep.bind(addr);
    fprintf(stderr, " * Server #%d up\n", seq);
    char buf[128];
    while (true) {
        // Receive request messages
        // @note only one message part assumed
        size_t len = rep.recv(buf, 128);
        buf[len] = 0;
        printf("Request: %s\n", buf);
        if (strcmp(buf, QUIT_MSG) == 0) {
            rep.send(DONE_MSG, strlen(DONE_MSG));
            break;
        }
        // Ignore remaining frames
        while (rep.more()) {
            rep.recv(buf, 0);
        }
        // Send reply
        sprintf(buf, "hello from server #%d", seq);
        rep.send(buf, strlen(buf));
    }
    
    return NULL;
}

static void* client_task(void* arg)
{
    //zmq::context_t ctx;
    zmq::socket_t dealer(ctx, ZMQ_DEALER);
    // Connect to multiple REP servers
    char addr[64];
    for (int seq = 0; seq < SERVER_NUM; ++seq) {
        sprintf(addr, "inproc://hello_server_%d", seq);
        dealer.connect(addr);
    }
    fprintf(stderr, " * Client up\n");
    // Send multiple requests in a row
    char buf[128];
    for (int t = 0; t < REQUEST_NUM; ++t) {
        sprintf(buf, "req#%d", t);
        dealer.send("", 0, ZMQ_SNDMORE);
        dealer.send(buf, strlen(buf));
    }
    fprintf(stderr, " * Polling for replies...\n");
    // Poll for replies
    zmq_pollitem_t items[] = {
        { (void*)dealer, 0, ZMQ_POLLIN, 0 }
    };
    int reply_count = 0;
    int done_count = 0;
    while (true) {
        if (reply_count == REQUEST_NUM && done_count < SERVER_NUM) {
            // Send 'done' signal to inform server to quit
            dealer.send("", 0, ZMQ_SNDMORE);
            dealer.send(QUIT_MSG, strlen(QUIT_MSG));
        } else if (done_count == SERVER_NUM) {
            // All servers quit, so break
            break;
        }
        zmq::poll(items, 1);
        if (items[0].revents & ZMQ_POLLIN) {
            // Check and strip the first empty message
            int len = dealer.recv(buf, 128);
            assert(len == 0);
            // Real reply content
            len = dealer.recv(buf, 128);
            buf[len] = 0;
            if (reply_count < REQUEST_NUM) {
                printf("Reply: %s\n", buf);
                reply_count++;
            } else if (strcmp(buf, DONE_MSG) == 0) {
                done_count++;
            }
        }
    }
    
    return NULL;
}

int main()
{
    pthread_t servers[SERVER_NUM];
    for (int t = 0; t < SERVER_NUM; ++t) {
        pthread_create(servers + t, NULL, server_task, (void*)t);
    }
    msleep(2000);
    pthread_t client;
    pthread_create(&client, NULL, client_task, NULL);
    // Wait for completion
    pthread_join(client, NULL);
    for (int t = 0; t < SERVER_NUM; ++t) {
        pthread_join(servers[t], NULL);
    }
}
