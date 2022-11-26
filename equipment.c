#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int equipmentId = 0;

typedef struct control
{
  char message[BUFSZ];
  int send_server;
} control;

control commandInterpreter(char *command)
{
  control serverCommand;
  if (strcmp(command, "close connection") == 0)
  {
    serverCommand.send_server = 1;
    sprintf(serverCommand.message, "%d %d", REQ_RM, equipmentId);
  }
  else if (strcmp(command, "list equipment") == 0)
  {
    serverCommand.send_server = 0;
    sprintf(serverCommand.message, "Equipments A,B,C,D");
    // todo show connected equipments
  }
  else if (strncmp(command, "request information from ", strlen("request information from ")) == 0)
  {
    int recipientId = 04;
    // todo change to read recipient id
    serverCommand.send_server = 1;
    sprintf(serverCommand.message, "%d %d %d", REQ_INF, equipmentId, recipientId);
  }
  else
  {
    serverCommand.send_server = 0;
    sprintf(serverCommand.message, "Mensagem não identificada: %s\n", command);
  }

  return serverCommand;
}

void handleResponse(char *response)
{
  char *splittedResponse = strtok(response, " ");
  int messageId = atoi(splittedResponse);

  switch (messageId)
  {
  case RES_ADD:
    splittedResponse = strtok(NULL, " ");
    int receivedId = atoi(splittedResponse);
    if (equipmentId == 0)
    {
      equipmentId = receivedId;
      printf("New ID: %02d\n", equipmentId);
    }
    else
    {
      printf("Equipment %02d", receivedId);
    }
    return;
  case RES_LIST:
    // todo something
  case RES_INF:
    // todo something
  case ERROR:
    splittedResponse = strtok(NULL, " ");
    int errorCode = atoi(splittedResponse) - 1;
    printf("%s\n", ERROR_MESSAGES[errorCode]);
    return;
  case OK:
    // todo something
  default:
    break;
  }

  printf("Unknown response: %s\n", response);
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  addrparse(argv[1], argv[2], &storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != connect(s, addr, sizeof(storage)))
  {
    logexit("connect");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  char buf[BUFSZ];
  memset(buf, 0, BUFSZ);
  sprintf(buf, "%d", REQ_ADD);
  sendMessage(s, buf);

  size_t count;
  control serverCommand;
  while (1)
  {
    memset(buf, 0, BUFSZ);
    count = recv(s, buf, BUFSZ, 0);
    if (count == 0 || strcmp(buf, "\0") == 0)
    {
      break; // Connection terminated
    }
    handleResponse(buf);

    count = 0;
    memset(buf, 0, BUFSZ);
    fgets(buf, BUFSZ - 1, stdin);
    buf[strcspn(buf, "\n")] = 0;
    serverCommand = commandInterpreter(buf);
    if (serverCommand.send_server)
    {
      sendMessage(s, serverCommand.message);
    }
    else
    {
      printf("%s\n", serverCommand.message);
    }
  }

  close(s);
  exit(EXIT_SUCCESS);
}