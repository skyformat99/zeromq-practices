// zmq_x.h
//
#ifndef ZMQ_X_H
#define ZMQ_X_H

int zmq_bind_x(void* s, const char* addr);
int zmq_unbind_x(void* s, const char* addr);
int zmq_connect_x(void* s, const char* addr);
int zmq_disconnect_x(void* s, const char* addr);
int zmq_socket_monitor_x(void* s, const char* addr, int events);

const char* zmq_endpoint_x(const char* addr);

#ifndef ZMQ_X_COMPILE
# ifdef WIN32

#  define zmq_bind zmq_bind_x
#  define zmq_unbind zmq_unbind_x
#  define zmq_connect zmq_connect_x
#  define zmq_disconnect zmq_disconnect_x
#  define zmq_socket_monitor zmq_socket_monitor_x

# endif
#endif // ZMQ_X_COMPILE

#endif // ZMQ_X_H
