// mdwrkapi.h
//
// Majordomo Protocol worker API
//
#ifndef MDWRKAPI_H_
#define MDWRKAPI_H_

#include <czmq.h>

typedef struct _mdwrk_t mdwrk_t;

mdwrk_t* mdwrk_new(const char* broker, const char* service, int verbose);
void mdwrk_destroy(mdwrk_t** self_p);
zmsg_t* mdwrk_recv(mdwrk_t* self, zmsg_t** reply_p);

void mdwrk_set_heartbeat_intv(mdwrk_t* self, int heartbeat_intv);

#endif // MDWRKAPI_H_
