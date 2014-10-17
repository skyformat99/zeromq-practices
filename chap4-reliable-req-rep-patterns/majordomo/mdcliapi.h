// mdcliapi.h
//
#ifndef MDCLIAPI_H_
#define MDCLIAPI_H_

#include <czmq.h>

typedef struct _mdcli_t mdcli_t;

mdcli_t* mdcli_new(const char* broker, int verbose);
void mdcli_destroy(mdcli_t** self_p);
zmsg_t* mdcli_send(mdcli_t* session, const char* service, zmsg_t** request_p);

#endif // MDCLIAPI_H_
