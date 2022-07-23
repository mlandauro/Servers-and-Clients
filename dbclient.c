#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <inttypes.h>

#include "msg.h"

#define BUF 1024

void Usage(char *progname);

int LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen);

int Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd);
int 
main(int argc, char **argv) {
  if (argc != 3) {
    Usage(argv[0]);
  }

  unsigned short port = 0;
  if (sscanf(argv[2], "%hu", &port) != 1) {
    Usage(argv[0]);
  }

  // Get an appropriate sockaddr structure.
  struct sockaddr_storage addr;
  size_t addrlen;
  if (!LookupName(argv[1], port, &addr, &addrlen)) {
    Usage(argv[0]);
  }

  // Connect to the remote host.
  int socket_fd;
  if (!Connect(&addr, addrlen, &socket_fd)) {
    Usage(argv[0]);
  }

  // Read something from the client.
  // Will only read BUF-1 characters at most.
  char readbuf[BUF];
  int8_t choice, flag;
	flag = 1;
//  char cmd[20];
//  char name[50];
//  char id[10];
  
  // Write something to the remote host.
  while (1) {
//  	strncpy(name, "", sizeof(name));
//	strncpy(id, "", sizeof(id));
	
	struct record r;
	struct msg m;
	printf("Enter your choice (1 to put, 2 to get, 3 to delete, 0 to quit): ");
	scanf("%" SCNd8 "%*c", &choice);
	switch (choice){
		case 1:
			//put
			m.type = PUT;
			printf("Enter the name: ");
			fgets(r.name, 50, stdin);
			printf("Enter the id: ");
			scanf("%u", &r.id);
//			fgets(r.id, 10, stdin);	
			break;
		case 2:
			//get
			m.type = GET;
			printf("Enter the id: ");
			scanf("%u", &r.id);
			break;
		case 3:
			//delete
			m.type = DEL;
			printf("Enter the id: ");
			scanf("%u", &r.id);
			break;
		default:
			strcpy(readbuf, "quit");
			flag = 0;
	}
	if(flag == 0)
		break;
//	name[strlen(name)-1] = ' ';
//	id[strlen(id)-1] = ' ';

//	sprintf(readbuf, "%s %s%s", cmd, name, id);

//    int res = strlen(readbuf);
	m.rd = r;
   int wres = write(socket_fd, &m, sizeof(struct msg));
    if (wres == 0) {
     printf("socket closed prematurely \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }
    if (wres == -1) {
      if (errno == EINTR)
        continue;
      printf("socket write failure \n");
      close(socket_fd);
      return EXIT_FAILURE;
    }

    //read msg from server
    struct msg Mserver;
    struct record Mrecord;
    ssize_t res = read(socket_fd, (void*)&Mserver, sizeof(struct msg));
//	printf("got server msg\n");
    if(res == -1){
    	if((errno == EAGAIN) || (errno == EINTR))
		continue;
	printf(" Error on server socket: %s \n ", strerror(errno));
	break;
    }

    if(res != sizeof(struct msg)){
    	printf("Error on reading server msg\n");
    }
    Mrecord = Mserver.rd;

	if((m.type == 2) && (Mserver.type == 4)){
		printf("name: %sid: %u\n", Mrecord.name, Mrecord.id);
		
	}

	switch(m.type){
	case 1:
		printf("Put ");
		break;
	case 2:
		printf("Get ");
		break;
	case 3:
		printf("Delete ");
		break;
	}
    	if(Mserver.type == 4){
		printf(" success\n");
	} else if(Mserver.type == 5){
		printf(" failed\n");
	}


//    if (flag == 0){
//    	break;
//    }
   // break;
  }

  // Clean up.
  close(socket_fd);
  return EXIT_SUCCESS;
}

void 
Usage(char *progname) {
  printf("usage: %s  hostname port \n", progname);
  exit(EXIT_FAILURE);
}

int 
LookupName(char *name,
                unsigned short port,
                struct sockaddr_storage *ret_addr,
                size_t *ret_addrlen) {
  struct addrinfo hints, *results;
  int retval;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // Do the lookup by invoking getaddrinfo().
  if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
    printf( "getaddrinfo failed: %s", gai_strerror(retval));
    return 0;
  }

  // Set the port in the first result.
  if (results->ai_family == AF_INET) {
    struct sockaddr_in *v4addr =
            (struct sockaddr_in *) (results->ai_addr);
    v4addr->sin_port = htons(port);
  } else if (results->ai_family == AF_INET6) {
    struct sockaddr_in6 *v6addr =
            (struct sockaddr_in6 *)(results->ai_addr);
    v6addr->sin6_port = htons(port);
  } else {
    printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
    freeaddrinfo(results);
    return 0;
  }

  // Return the first result.
  assert(results != NULL);
  memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
  *ret_addrlen = results->ai_addrlen;

  // Clean up.
  freeaddrinfo(results);
  return 1;
}

int 
Connect(const struct sockaddr_storage *addr,
             const size_t addrlen,
             int *ret_fd) {
  // Create the socket.
  int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
  if (socket_fd == -1) {
    printf("socket() failed: %s", strerror(errno));
    return 0;
  }

  // Connect the socket to the remote host.
  int res = connect(socket_fd,
                    (const struct sockaddr *)(addr),
                    addrlen);
  if (res == -1) {
    printf("connect() failed: %s", strerror(errno));
    return 0;
  }

  *ret_fd = socket_fd;
  return 1;
}
