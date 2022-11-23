#include "common.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_RACK_NUMBER 4
#define RACK_SIZE_LIMIT 3
#define MAX_SWITCH_TYPES 4
#define MULTIPLE_SWITCH_GET 2

typedef struct rack
{
  int initialized;
  int switchCount;
  int installedSwitches[MAX_SWITCH_TYPES];
} rack;

typedef struct response
{
  char message[BUFSZ];
  int endConnection;
} response;

response exitHandler()
{
  response response;
  memset(response.message, 0, BUFSZ);
  response.endConnection = 1;
  return response;
}

response resolveHandler(char *message)
{
  response response;
  strcpy(response.message, message);
  response.endConnection = 0;
  return response;
}

response endCommandHandler(char *splittedCommand, char *message)
{
  splittedCommand = strtok(NULL, " ");
  if (splittedCommand != NULL)
    return exitHandler();
  return resolveHandler(message);
}

int isInvalidRack(int rackId)
{
  return (rackId < 1 || rackId > MAX_RACK_NUMBER);
}

int isInvalidSwitch(int switchId)
{
  return (switchId < 1 || switchId > MAX_SWITCH_TYPES);
}

response addHandler(char *splittedCommand, rack *racks)
{
  splittedCommand = strtok(NULL, " ");
  if (!strcmp(splittedCommand, "sw") == 0)
    return exitHandler();

  int addList[RACK_SIZE_LIMIT];
  int countAdd = 0;

  splittedCommand = strtok(NULL, " ");
  while (strcmp(splittedCommand, "in") != 0)
  {
    if (countAdd >= RACK_SIZE_LIMIT)
    {
      return resolveHandler("error rack limit exceeded\n");
    }

    addList[countAdd] = atoi(splittedCommand) - 1;
    if (isInvalidSwitch(addList[countAdd] + 1))
      return resolveHandler("error switch type unknown\n");
    countAdd++;

    splittedCommand = strtok(NULL, " ");
  }

  splittedCommand = strtok(NULL, " ");
  int rackId = atoi(splittedCommand) - 1;
  if (isInvalidRack(rackId + 1))
    return resolveHandler("error rack doesn't exist\n");

  if (racks[rackId].switchCount + countAdd > RACK_SIZE_LIMIT)
  {
    return resolveHandler("error rack limit exceeded\n");
  }

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  int bufferIndex = 0;
  for (int i = 0; i < countAdd; i++)
  {
    if (racks[rackId].installedSwitches[addList[i]])
    {
      sprintf(msg, "error switch 0%d already installed in 0%d\n", addList[i] + 1, rackId + 1);
      return resolveHandler(msg);
    }
    else
    {
      racks[rackId].initialized = 1;
      racks[rackId].installedSwitches[addList[i]] = 1;
      racks[rackId].switchCount++;
    }
  }

  bufferIndex += sprintf(msg, "switch ");
  for (int i = 0; i < countAdd; i++)
  {
    bufferIndex += sprintf(&msg[bufferIndex], "0%d ", addList[i] + 1);
  }
  bufferIndex += sprintf(&msg[bufferIndex], "installed\n");
  return endCommandHandler(splittedCommand, msg);
}

response rmHandler(char *splittedCommand, rack *racks)
{
  splittedCommand = strtok(NULL, " ");
  if (!strcmp(splittedCommand, "sw") == 0)
    return exitHandler();

  splittedCommand = strtok(NULL, " ");
  int switchId = atoi(splittedCommand) - 1;
  if (isInvalidSwitch(switchId + 1))
    return resolveHandler("error switch type unknown\n");

  splittedCommand = strtok(NULL, " ");
  if (!strcmp(splittedCommand, "in") == 0)
    return exitHandler();

  splittedCommand = strtok(NULL, " ");
  int rackId = atoi(splittedCommand) - 1;
  if (isInvalidRack(rackId + 1))
    return resolveHandler("error rack doesn't exist\n");

  if (!racks[rackId].installedSwitches[switchId])
  {
    return resolveHandler("error switch doesn't exist\n");
  }

  racks[rackId].switchCount--;
  racks[rackId].installedSwitches[switchId] = 0;

  char msg[BUFSZ];
  sprintf(msg, "switch 0%d removed from 0%d\n", switchId + 1, rackId + 1);
  return endCommandHandler(splittedCommand, msg);
}

response getHandler(char *splittedCommand, rack *racks)
{
  int getList[MULTIPLE_SWITCH_GET];
  int countGet = 0;

  splittedCommand = strtok(NULL, " ");
  while (strcmp(splittedCommand, "in") != 0 && countGet < MULTIPLE_SWITCH_GET)
  {
    getList[countGet] = atoi(splittedCommand) - 1;
    if (isInvalidSwitch(getList[countGet] + 1))
      return resolveHandler("error switch type unknown\n");
    countGet++;

    splittedCommand = strtok(NULL, " ");
  }

  if (countGet == 0)
    return exitHandler();

  splittedCommand = strtok(NULL, " ");
  int rackId = atoi(splittedCommand) - 1;
  if (isInvalidRack(rackId + 1))
    return resolveHandler("error rack doesn't exist\n");

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  int bufferIndex = 0;
  for (int i = 0; i < countGet; i++)
  {
    if (!racks[rackId].installedSwitches[getList[i]])
    {
      return resolveHandler("error switch doesn't exist\n");
    }
    else
    {
      bufferIndex += sprintf(&msg[bufferIndex], "%d Kbs ", rand() % (5000)); // Arbitrary limit for rand
    }
  }
  bufferIndex += sprintf(&msg[bufferIndex], "\n");
  return endCommandHandler(splittedCommand, msg);
}

response lsHandler(char *splittedCommand, rack *racks)
{
  splittedCommand = strtok(NULL, " ");
  int rackId = atoi(splittedCommand) - 1;
  if (isInvalidRack(rackId + 1))
    return resolveHandler("error rack doesn't exist\n");

  if (racks[rackId].initialized == 0)
  {
    return resolveHandler("error rack doesn't exist\n");
  }

  if (racks[rackId].switchCount == 0)
  {
    return resolveHandler("empty rack\n");
  }

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  int bufferIndex = 0;
  for (int i = 0; i < MAX_SWITCH_TYPES; i++)
  {
    int found = 0;
    if (racks[rackId].installedSwitches[i])
    {
      found++;
      bufferIndex += sprintf(&msg[bufferIndex], "0%d", i + 1);
      if (found != racks[rackId].switchCount)
      {
        bufferIndex += sprintf(&msg[bufferIndex], " ");
      }
    }
  }
  bufferIndex += sprintf(&msg[bufferIndex], "\n");
  return endCommandHandler(splittedCommand, msg);
}

response handleCommands(char *buf, rack *racks)
{
  char *splittedCommand = strtok(buf, " ");

  if (strcmp(splittedCommand, "add") == 0)
  {
    return addHandler(splittedCommand, racks);
  }
  else if (strcmp(splittedCommand, "rm") == 0)
  {
    return rmHandler(splittedCommand, racks);
  }
  else if (strcmp(splittedCommand, "get") == 0)
  {
    return getHandler(splittedCommand, racks);
  }
  else if (strcmp(splittedCommand, "ls") == 0)
  {
    return lsHandler(splittedCommand, racks);
  }
  else
  {
    return exitHandler();
  }
}

void initializeRacks(rack *racks)
{
  for (int i = 0; i < MAX_RACK_NUMBER; i++)
  {
    racks[i].initialized = 0;
    racks[i].switchCount = 0;
    for (int j = 0; j < MAX_SWITCH_TYPES; j++)
    {
      racks[i].installedSwitches[j] = 0;
    }
  }
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  server_sockaddr_init(argv[1], argv[2], &storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if (s == -1)
  {
    logexit("socket");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(s, addr, sizeof(storage)))
  {
    logexit("bind");
  }

  if (0 != listen(s, 10))
  {
    logexit("listen");
  }

  char addrstr[BUFSZ];
  addrtostr(addr, addrstr, BUFSZ);

  rack racks[MAX_RACK_NUMBER];
  initializeRacks(racks);
  while (1)
  {
    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    int csock = accept(s, caddr, &caddrlen);
    if (csock == -1)
    {
      logexit("accept");
    }

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);

    char buf[BUFSZ];
    while (1)
    {
      memset(buf, 0, BUFSZ);
      size_t count = recv(csock, buf, BUFSZ - 1, 0);
      buf[strcspn(buf, "\n")] = 0;

      response response;
      memset(response.message, 0, BUFSZ);

      response = handleCommands(buf, racks);
      count = send(csock, response.message, strlen(response.message) + 1, 0);
      if (count != strlen(response.message) + 1)
      {
        logexit("send");
      }

      if (response.endConnection)
      {
        close(csock);
        break;
      }
    }
  }
  exit(EXIT_SUCCESS);
}