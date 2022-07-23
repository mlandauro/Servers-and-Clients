#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "msg.h"

void Usage(char *progname);
void PrintOut(int fd, struct sockaddr *addr, size_t addrlen);
void PrintReverseDNS(struct sockaddr *addr, size_t addrlen);
void PrintServerSide(int client_fd, int sock_family);
int  Listen(char *portnum, int *sock_family);
void *HandleClient(void *arg);
//void *clientSocket(void *arg);

//FILE *outfile;

struct client
{
	int client_fd;
	struct sockaddr_storage caddr;
	socklen_t caddr_len;
	int sock_family;
};

int 
main(int argc, char **argv) {
  // Expect the port number as a command line argument.
 // while(1){//have server always listening
  struct client c;
  if (argc != 2) {
    Usage(argv[0]);
  }

  int sock_family;
  int listen_fd = Listen(argv[1], &sock_family);

  if (listen_fd <= 0) {
    // We failed to bind/listen to a socket.  Quit with failure.
    printf("Couldn't bind to any addresses.\n");
    return EXIT_FAILURE;
  }

  //open database file
  FILE *outfile;
  outfile = fopen("database.dat", "w+");
  if(outfile == NULL){
  	fprintf(stderr, "\nError opened file\n");
	exit(1);
  }
  fclose(outfile);
   // Loop forever, accepting a connection from a client and doing
  // an echo trick to it.
  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_fd,
                           (struct sockaddr *)(&caddr),
                           &caddr_len);
    if (client_fd < 0) {
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK))
        continue;
      printf("Failure on accept:%s \n ", strerror(errno));
      break;
    }
	// Create thread here to deal with specified client
	// may need struct to hold client parameters
	
	c.client_fd = client_fd;
	c.caddr = caddr;
	c.caddr_len = caddr_len;
	c.sock_family = sock_family;

	pthread_t p;
	pthread_create(&p, NULL, HandleClient, (void *)&c);

//    HandleClient(client_fd,
  //               (struct sockaddr *)(&caddr),
    //             caddr_len,
      //           sock_family);
  }
//  fclose(outfile);
  // Close socket
  close(listen_fd);
  pthread_exit((void *)0);
  return EXIT_SUCCESS;
}

void Usage(char *progname) {
  printf("usage: %s port \n", progname);
  exit(EXIT_FAILURE);
}

void 
PrintOut(int fd, struct sockaddr *addr, size_t addrlen) {
  printf("Socket [%d] is bound to: \n", fd);
  if (addr->sa_family == AF_INET) {
    // Print out the IPV4 address and port

    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = (struct sockaddr_in *)(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    printf(" IPv4 address %s", astring);
    printf(" and port %d\n", ntohs(in4->sin_port));

  } else if (addr->sa_family == AF_INET6) {
    // Print out the IPV6 address and port

    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    printf("IPv6 address %s", astring);
    printf(" and port %d\n", ntohs(in6->sin6_port));

  } else {
    printf(" ???? address and port ???? \n");
  }
}

void 
PrintReverseDNS(struct sockaddr *addr, size_t addrlen) {
  char hostname[1024];  // ought to be big enough.
  if (getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0) != 0) {
    sprintf(hostname, "[reverse DNS failed]");
  }
  printf("DNS name: %s \n", hostname);
}

void 
PrintServerSide(int client_fd, int sock_family) {
  char hname[1024];
  hname[0] = '\0';

  printf("Server side interface is ");
  if (sock_family == AF_INET) {
    // The server is using an IPv4 address.
    struct sockaddr_in srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET, &srvr.sin_addr, addrbuf, INET_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  } else {
    // The server is using an IPv6 address.
    struct sockaddr_in6 srvr;
    socklen_t srvrlen = sizeof(srvr);
    char addrbuf[INET6_ADDRSTRLEN];
    getsockname(client_fd, (struct sockaddr *) &srvr, &srvrlen);
    inet_ntop(AF_INET6, &srvr.sin6_addr, addrbuf, INET6_ADDRSTRLEN);
    printf("%s", addrbuf);
    // Get the server's dns name, or return it's IP address as
    // a substitute if the dns lookup fails.
    getnameinfo((const struct sockaddr *) &srvr,
                srvrlen, hname, 1024, NULL, 0, 0);
    printf(" [%s]\n", hname);
  }
}

int 
Listen(char *portnum, int *sock_family) {

  // Populate the "hints" addrinfo structure for getaddrinfo().
  // ("man addrinfo")
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;       // IPv6 (also handles IPv4 clients)
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "in6addr_any" address
  hints.ai_flags |= AI_V4MAPPED;    // use v4-mapped v6 if no v6 found
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // Use argv[1] as the string representation of our portnumber to
  // pass in to getaddrinfo().  getaddrinfo() returns a list of
  // address structures via the output parameter "result".
  struct addrinfo *result;
  int res = getaddrinfo(NULL, portnum, &hints, &result);

  // Did addrinfo() fail?
  if (res != 0) {
	printf( "getaddrinfo failed: %s", gai_strerror(res));
    return -1;
  }

  // Loop through the returned address structures until we are able
  // to create a socket and bind to one.  The address structures are
  // linked in a list through the "ai_next" field of result.
  int listen_fd = -1;
  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      printf("socket() failed:%s \n ", strerror(errno));
      listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!  Print out the information about what
      // we bound to.
      PrintOut(listen_fd, rp->ai_addr, rp->ai_addrlen);

      // Return to the caller the address family.
      *sock_family = rp->ai_family;
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(listen_fd);
    listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure.
  if (listen_fd == -1)
    return listen_fd;

  // Success. Tell the OS that we want this to be a listening socket.
  if (listen(listen_fd, SOMAXCONN) != 0) {
    printf("Failed to mark socket as listening:%s \n ", strerror(errno));
    close(listen_fd);
    return -1;
  }

  // Return to the client the listening file descriptor.
  return listen_fd;
}

void 
*HandleClient(void *arg) {
	FILE *outfile;  
  printf("Thread id: %ld\n", (syscall(SYS_gettid)));
 
  struct client *c = arg;
  int c_fd = c -> client_fd;
  struct sockaddr *addr = (struct sockaddr *) (&(c -> caddr));
  size_t addrlen = c -> caddr_len;
  int sock_family = c -> sock_family;
  // Print out information about the client.
  //               (struct sockaddr *)(&caddr),
  printf("\nNew client connection \n" );
  PrintOut(c_fd, addr, addrlen);
  PrintReverseDNS(addr, addrlen);
  PrintServerSide(c_fd, sock_family);

//	outfile = fopen("database.dat", "w+");
  // Loop, reading data and echo'ing it back, until the client
  // closes the connection.
  int entries = 0;
  while (1) {
  entries = 0;//reset entries
    //char clientbuf[1024];
//    outfile = fopen("databasa.dat", "w+");	
    struct msg m;
    struct record r;
    ssize_t res = read(c_fd, (void*)&m, sizeof(struct msg));
	

    if (res == -1) {
      if ((errno == EAGAIN) || (errno == EINTR))
        continue;

	  printf(" Error on client socket:%s \n ", strerror(errno));
      break;
    }

    if(res != sizeof(struct msg)){
    	//return error message
	m.type = FAIL;
	write(c_fd, &m, sizeof(struct msg));
    }
	
	r = m.rd;
	struct record temp;	
//    clientbuf[res] = '\0';
    printf("name: %s", r.name);
    printf("id: %u\n", r.id);
	char a[20] = "empty";

    if(m.type  == 1){
    	outfile = fopen("database.dat", "r+");
  	if(outfile == NULL){
  		fprintf(stderr, "\nError opened file\n");
		exit(1);
  	}
    	printf("user sent put cmd\n");
	while(fread(&temp, sizeof(struct record), 1, outfile)){
		printf("%s\n", temp.name);
		if(strcmp(temp.name, a) == 0){
	//		printf("enters if\n");
			fseek(outfile, (entries*512), SEEK_SET);
	//		printf("entries: %d\n", entries);
			break; //break out of loop and write
		}
		entries++;
	}
	if(fwrite(&r, sizeof(struct record),1, outfile) != 0)
		m.type = 4;
	else
		m.type = 5;
	fclose(outfile);
	//break;
    } else if(m.type == 2 ){
    	//struct record temp;
    	outfile = fopen("database.dat", "a+");
  	if(outfile == NULL){
  		fprintf(stderr, "\nError opened file\n");
		exit(1);
  	}
    	printf("user sent get cmd\n");
	while(fread(&temp, sizeof(struct record), 1, outfile)){
		if(temp.id == r.id){
			//printf("struct found\n");
			m.rd = temp;
			m.type = 4;
			break;
		}
		m.type = 5;
	}
	fclose(outfile);
	//break;
    } else if(m.type == 3){
    	//struct record temp;
    	outfile = fopen("database.dat", "r+");
    	printf("user sent delete cmd\n");
	if(outfile == NULL){
		fprintf(stderr, "\nError opened file\n");
		exit(1);
	}
	while(fread(&temp, sizeof(struct record), 1, outfile)){
		if(temp.id == r.id){//found record so add empty record
			r.id = -1;
			bzero(r.name, MAX_NAME_LENGTH);
			memcpy(r.name, a, strlen(a));
			fseek(outfile, (entries * 512), SEEK_SET);//set pointer to offset
			//update entry
			if(fwrite(&r, sizeof(struct record), 1, outfile) != 0)
				m.type = 4;
			else
				m.type = 5;
			break;
		}
		entries++;
	}
	fclose(outfile);
//	break;
    } else if (m.type == 0 || m.type == 5 || m.type == 4){
    	break;
    } else {
    	printf("type: %d\n", m.type);
    	printf("user sent invalid cmd\n");
	continue;
    }
  	 write(c_fd, &m, sizeof(struct msg));
    // Really should do this in a loop in case of EAGAIN, EINTR,
    // or short write, but I'm lazy.  Don't be like me. ;)
//    write(c_fd, "You typed: ", strlen("You typed: "));
//    write(c_fd, clientbuf, strlen(clientbuf));
  }
//  fclose(outfile);
pthread_exit(NULL);
  //close(c_fd);
  return(NULL);
}
