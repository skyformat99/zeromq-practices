// cluster/endpoint.h
//
#ifndef CLUSTER_ENDPOINT_H_
#define CLUSTER_ENDPOINT_H_

#define DECLARE_ENDPOINT(xxx) \
const char* xxx##_endpoint(const char* self, char* buf = 0)

DECLARE_ENDPOINT(state);
DECLARE_ENDPOINT(localfe);
DECLARE_ENDPOINT(localbe);
DECLARE_ENDPOINT(cloud);
DECLARE_ENDPOINT(monitor);

#endif // CLUSTER_ENDPOINT_H_
