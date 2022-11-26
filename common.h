#pragma once
#include <stdlib.h>
#include <arpa/inet.h>

#define BUFSZ 1024

#define MAX_EQUIPMENT_NUMBER 10

enum COMMAND_TYPE
{
    REQ_ADD = 1,
    REQ_RM = 2,
    RES_ADD = 3,
    RES_LIST = 4,
    REQ_INF = 5,
    RES_INF = 6,
    ERROR = 7,
    OK = 8,
};

void logexit(const char *msg);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage);