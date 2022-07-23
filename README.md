# Servers-and-Clients
## Micaela Landauro
### UTD CS 3377 Spring 2022
This project was one of my final projects in my UNIX class

### DESCRIPTION
  The main focus of this project was to implement a database server and client using TCP/IP.
  
### HOW IT WORKS
  Database Server: Implements a concurrent server using threads. Here the parent thread
    initializes a listening socket that waits for a client connection. Once the connection
    is established a handler thread is created to handle client requests. The handler thread
    processes the given request from the client (PUT, GET, or DELETE) and returns a response
    (SUCCESS or FAIL) allowing a database of students to be created. The handler terminates
    only when the client closes the connection. The server never terminates, it waits for the
    next request.
    
  Database Client: The user interacts with the client database. It starts by setting up a
    connection with the server and the prompts the user to choose an operation (PUT, GET,
    DELETE), it sends the corresponding message to the server and then waits on a FAIL or 
    SUCCESS response from the server.
  
