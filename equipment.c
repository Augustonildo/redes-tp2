#include "common.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int equipmentId = 0;
int serverSocket;
int equipmentsInstalled[MAX_EQUIPMENT_NUMBER];

void initializeEquipments()
{
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    equipmentsInstalled[i] = 0;
  }
}

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
    for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
    {
      if (equipmentsInstalled[i] && equipmentId != (i + 1))
        printf("%02d ", i + 1);
    }
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
    equipmentsInstalled[receivedId - 1] = 1;
    if (equipmentId == 0)
    {
      equipmentId = receivedId;
      printf("New ID: %02d\n", equipmentId);
    }
    else
    {
      printf("Equipment %02d added\n", receivedId);
    }
    return;
  case RES_LIST:
    splittedResponse = strtok(NULL, " ");
    while (splittedResponse != NULL)
    {
      equipmentsInstalled[atoi(splittedResponse) - 1] = 1;
      splittedResponse = strtok(NULL, " ");
    }
    return;
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

void *receiveMessageThread(void *data)
{
  char buf[BUFSZ];
  while (1)
  {
    recv(serverSocket, buf, BUFSZ, 0);
    handleResponse(buf);
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  addrparse(argv[1], argv[2], &storage);

  serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
  if (serverSocket == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != connect(serverSocket, addr, sizeof(storage)))
  {
    logexit("connect");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  initializeEquipments();

  char buf[BUFSZ];
  memset(buf, 0, BUFSZ);
  sprintf(buf, "%d", REQ_ADD);
  sendMessage(serverSocket, buf);

  pthread_t receiveThread;
  pthread_create(&receiveThread, NULL, receiveMessageThread, NULL);

  control serverCommand;
  while (1)
  {
    memset(buf, 0, BUFSZ);
    fgets(buf, BUFSZ - 1, stdin);
    buf[strcspn(buf, "\n")] = 0;
    serverCommand = commandInterpreter(buf);
    if (serverCommand.send_server)
    {
      sendMessage(serverSocket, serverCommand.message);
    }
    else
    {
      printf("%s\n", serverCommand.message);
    }
  }

  close(serverSocket);
  exit(EXIT_SUCCESS);
}