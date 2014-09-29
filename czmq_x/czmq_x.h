// czmq_x.h
//
// Simple hack to make 'ipc' and 'inproc' work by mapping the endpoint to
// internal 'tcp' endpoint, and use 'tcp' socket internally.
//
// With the help of 'zmq_x' lib
//
#ifndef CZMQ_X_H_
#define CZMQ_X_H_

#ifdef __cplusplus
extern "C" {
#endif

int zsocket_bind_x(void* self, const char* format, ...);
int zsocket_unbind_x(void* self, const char* format, ...);
int zsocket_connect_x(void* self, const char* format, ...);
int zsocket_disconnect_x(void* self, const char* format, ...);

#include <czmq.h>
int czmq_init_x(zctx_t* ctx);

#if !defined(CZMQ_X_COMPILE) && defined (WIN32)
#  define zsocket_bind zsocket_bind_x
#  define zsocket_unbind zsocket_unbind_x
#  define zsocket_connect zsocket_connect_x
#  define zsocket_disconnect zsocket_disconnect_x
#endif // CZMQ_X_COMPILE && WIN32

#ifdef __cplusplus
}
#endif

#endif // CZMQ_X_H_
