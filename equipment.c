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
  }
  else if (strcmp(command, "request information from") == 0)
  {
    int recipientId = 04;
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
  size_t count = send(s, buf, strlen(buf) + 1, 0);
  if (count != strlen(buf) + 1)
  {
    logexit("send");
  }

  control serverCommand;
  while (1)
  {
    memset(buf, 0, BUFSZ);
    count = recv(s, buf, BUFSZ, 0);
    if (count == 0 || strcmp(buf, "\0") == 0)
    {
      break; // Connection terminated
    }
    printf("%s", buf);
    count = 0;

    memset(buf, 0, BUFSZ);
    fgets(buf, BUFSZ - 1, stdin);
    buf[strcspn(buf, "\n")] = 0;
    serverCommand = commandInterpreter(buf);
    if (serverCommand.send_server)
    {
      printf("Sent server \n");
      count = send(s, serverCommand.message, strlen(buf) + 1, 0);
      if (count != strlen(buf) + 1)
      {
        logexit("send");
      }
    }
    else
    {
      printf("Didn't send server \n");
      printf("%s\n", serverCommand.message);
    }
  }

  close(s);
  exit(EXIT_SUCCESS);
}