// zmq_x.h
//
#ifndef ZMQ_X_H
#define ZMQ_X_H

#ifdef __cplusplus
extern "C" {
#endif

int zmq_bind_x(void* s, const char* addr);
int zmq_unbind_x(void* s, const char* addr);
int zmq_connect_x(void* s, const char* addr);
int zmq_disconnect_x(void* s, const char* addr);
int zmq_socket_monitor_x(void* s, const char* addr, int events);

int zmq_init_x(void* ctx);
const char* zmq_endpoint_x(const char* addr);

#if !defined(ZMQ_X_COMPILE) && defined(WIN32)
#  define zmq_bind zmq_bind_x
#  define zmq_unbind zmq_unbind_x
#  define zmq_connect zmq_connect_x
#  define zmq_disconnect zmq_disconnect_x
#  define zmq_socket_monitor zmq_socket_monitor_x
#endif // ZMQ_X_COMPILE && WIN32

#ifdef __cplusplus
}
#endif

#endif // ZMQ_X_H
