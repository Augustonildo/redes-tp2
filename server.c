#include "common.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_EQUIPMENT_NUMBER 10

typedef struct slot
{
  int id;
  int available;
} slot;

int equipmentCount = 0;
slot slots[MAX_EQUIPMENT_NUMBER];

void initializeSlots()
{
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    slots[i].available = 1;
    slots[i].id = i + 1;
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

response addNewEquipment()
{
  int newId;
  for (int i = 0; i < MAX_EQUIPMENT_NUMBER; i++)
  {
    if (slots[i].available)
    {
      newId = slots[i].id;
      slots[i].available = 0;
      equipmentCount++;
      break;
    }
  }

  printf("Equipment %02d added\n", newId);

  char msg[BUFSZ];
  memset(msg, 0, BUFSZ);
  sprintf(msg, "New ID: %02d\n", newId);
  return resolveHandler(msg);
}

response removeEquipment(int id)
{
  slots[id - 1].available = 1;
  equipmentCount--;

  printf("Equipment %02d removed\n", id);
  return exitHandler("Success");
}

response handleCommands(char *buf)
{
  if (strcmp(buf, "close connection") == 0)
  {
    return removeEquipment(02);
  }

  if (strcmp(buf, "list equipment") == 0)
  {
    return resolveHandler("Equipments A,B,C,D\n");
  }

  if (strcmp(buf, "request information from 04") == 0)
  {
    printf("Equipment 0X not found\n");
    return resolveHandler("Target equipment not found\n");
  }

  printf("Mensagem não identificada\n");
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
  size_t count;

  char caddrstr[BUFSZ];
  addrtostr(caddr, caddrstr, BUFSZ);

  if (equipmentCount >= MAX_EQUIPMENT_NUMBER)
  {
    char *mensagemServidorCheio = "Equipment limit exceeded";
    count = send(cdata->csock, mensagemServidorCheio, strlen(mensagemServidorCheio) + 1, 0);
    if (count != strlen(mensagemServidorCheio) + 1)
    {
      logexit("send");
    }
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
  }

  response response;
  memset(response.message, 0, BUFSZ);

  response = addNewEquipment();
  count = send(cdata->csock, response.message, strlen(response.message) + 1, 0);
  if (count != strlen(response.message) + 1)
  {
    logexit("send");
  }

  char buf[BUFSZ];
  while (1)
  {
    memset(buf, 0, BUFSZ);
    memset(response.message, 0, BUFSZ);
    count = recv(cdata->csock, buf, BUFSZ - 1, 0);
    buf[strcspn(buf, "\n")] = 0;

    response = handleCommands(buf);
    count = send(cdata->csock, response.message, strlen(response.message) + 1, 0);
    if (count != strlen(response.message) + 1)
    {
      logexit("send");
    }

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

  initializeSlots();
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