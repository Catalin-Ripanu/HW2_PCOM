#include <netinet/in.h>
#define MESSAGE_UDP_TOPIC 50
#define MESSAGE_UDP_DATA 1500

/* Mesajul UDP */
struct __attribute__((__packed__)) message_udp {
    enum type : uint8_t {
        INT        = 0,
        SHORT_REAL = 1,
        FLOAT      = 2,
        STRING     = 3,
    };

    static constexpr const uint16_t type_size[] = {
        [INT]        = sizeof(uint8_t)+sizeof(int),
        [SHORT_REAL] = sizeof(uint16_t),
        [FLOAT]      = sizeof(uint8_t)+sizeof(int)+sizeof(uint8_t),
        [STRING]     = MESSAGE_UDP_DATA,
    };

    static constexpr const char* str_type[] = {
        [INT]        = "INT",
        [SHORT_REAL] = "SHORT_REAL",
        [FLOAT]      = "FLOAT",
        [STRING]     = "STRING",
    };

    char topic[MESSAGE_UDP_TOPIC];

    enum type type;

    char data[MESSAGE_UDP_DATA];
};
/* Mesajul TCP */
struct __attribute__((__packed__)) message_tcp  {
    enum type : uint8_t {
        ID_CLIENT = 0,
        UNSUBSCRIBE = 1,
        SUBSCRIBE = 2,
        ANSWER_SV = 3,
        INFO_SV = 4,
    };

    struct __attribute__((__packed__)) {
        uint16_t len;
        enum type type;
        struct sockaddr_in cl_dgram_addr;
    } header;

    char data[sizeof(struct message_udp)];
};
/* Un buffer folosit pentru a economisi memorie */
union buffer {
    struct {
        char unused[sizeof(message_tcp)-sizeof(message_udp)];
        message_udp message;
    } message_udp;

    struct message_tcp message_tcp;

    char buffer[sizeof(message_tcp)];
};