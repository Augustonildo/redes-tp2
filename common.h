#pragma once
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

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

enum ERROR
{
    EQUIPMENT_NOT_FOUND = 1,
    SOURCE_NOT_FOUND = 2,
    TARGET_NOT_FOUND = 3,
    LIMIT_EXCEEDED = 4
};

const char *ERROR_MESSAGES[] = {"Equipment not found", "Source equipment not found", "Target equipment not found", "Equipment limit exceeded"};

const char *OK_MESSAGES[] = {"Success"};

void logexit(const char *msg);
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage);
void sendMessage(const int socket, char *msg);
void sendMessageTo(const int socket, char *msg, const struct sockaddr *addr);