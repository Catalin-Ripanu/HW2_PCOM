#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <error.h>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define CL_ID_LEN 10

#define DIE(cond, msg, ...) \
    do { \
        if ((cond)) { \
            error_at_line(EXIT_FAILURE, errno, __FILE__, __LINE__, (msg), ##__VA_ARGS__); \
        } \
    } while (0)

static inline char* proc_cl_id(char* cl_id) {
    if (std::strlen(cl_id) > CL_ID_LEN) {
        cl_id[CL_ID_LEN] = '\0';
    }

    return cl_id;
}

static inline in_addr_t parse_addr_str(const char* addr_str) {
    in_addr_t address = inet_addr(addr_str);

    if (address == (in_addr_t)-1) {
        error(EXIT_FAILURE, errno, "could not parse address '%s'", addr_str);
    }

    return address;
}

static inline in_port_t parse_port_str(const char* port_str) {
    in_port_t port;

    if (std::sscanf(port_str, "%" SCNu16, &port) < 1) {
        error(EXIT_FAILURE, errno, "could not parse port '%s'", port_str);
    }

    return htons(port);
}
