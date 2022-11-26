#include "common.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct equipment
{
  int id;
  int installed;
  struct sockaddr *address;
} equipment;

int equipmentCount = 0;
equipment equipments[MAX_EQUIPMENT_NUMBER];

void initializeEquipments()
{
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    equipments[i].installed = 0;
    equipments[i].id = i + 1;
  }
}

typedef struct response
{
  char message[BUFSZ];
  int endConnection;
} response;

response exitHandler(char *message)
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

response addNewEquipment(struct sockaddr *clientAddress)
{
  int newId;
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (!equipments[i].installed)
    {
      newId = equipments[i].id;
      equipments[i].installed = 1;
      equipments[i].address = clientAddress;
      equipmentCount++;
      break;
    }
  }

  printf("Equipment %02d added\n", newId);

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  sprintf(msg, "%02d %02d\n", RES_ADD, newId);
  return resolveHandler(msg);
}

response removeEquipment(int id)
{
  equipments[id - 1].installed = 0;
  equipmentCount--;

  printf("Equipment %02d removed\n", id);
  return exitHandler("Success");
}

response handleCommands(char *buf, struct sockaddr *clientAddress)
{
  char *splittedCommand = strtok(buf, " ");
  int commandId = atoi(splittedCommand);

  switch (commandId)
  {
  case REQ_ADD:
    return addNewEquipment(clientAddress);
  case REQ_RM:
    splittedCommand = strtok(NULL, " ");
    return removeEquipment(atoi(splittedCommand));
  case REQ_INF:
    printf("Equipment 0X not found\n");
    return resolveHandler("Target equipment not found\n");
  default:
    break;
  }

  printf("Unknown message: %s\n", buf);
  return exitHandler("ERROR");
}

struct client_data
{
  int csock;
  struct sockaddr_storage storage;
};

void *client_thread(void *data)
{
  struct client_data *cdata = (struct client_data *)data;
  struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

  char caddrstr[BUFSZ];
  addrtostr(caddr, caddrstr, BUFSZ);

  if (equipmentCount >= MAX_EQUIPMENT_NUMBER)
  {
    char mensagemServidorCheio[BUFSZ];
    memset(mensagemServidorCheio, 0, BUFSZ);
    sprintf(mensagemServidorCheio, "%02d %02d", ERROR, LIMIT_EXCEEDED);
    sendMessage(cdata->csock, mensagemServidorCheio);
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
  }

  response response;
  memset(response.message, 0, BUFSZ);

  char buf[BUFSZ];
  while (1)
  {
    memset(buf, 0, BUFSZ);
    memset(response.message, 0, BUFSZ);

    recv(cdata->csock, buf, BUFSZ - 1, 0);
    buf[strcspn(buf, "\n")] = 0;

    response = handleCommands(buf, caddr);
    sendMessage(cdata->csock, response.message);

    if (response.endConnection)
    {
      close(cdata->csock);
      break;
    }
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
  struct sockaddr_storage storage;
  server_sockaddr_init(argv[1], &storage);

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

  initializeEquipments();
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

    struct client_data *cdata = malloc(sizeof(*cdata));
    if (!cdata)
    {
      logexit("malloc");
    }
    cdata->csock = csock;
    memcpy(&(cdata->storage), &cstorage, sizeof(storage));

    pthread_t tid;
    pthread_create(&tid, NULL, client_thread, cdata);
  }
  exit(EXIT_SUCCESS);
}