// A server that you can connect multiple clients and have them talk to 
// eachother. Want to be able to mimic basic functions of discord with
// only messaging. Want to implement usernames and friending.
//
// TODO: Get 2 clients connected to the server and have them both send
// a simple message to eachother, requires polling or maybe use fork(). 

// gets IP addreses and returns either IPv4 or IPv6
#include <asm-generic/socket.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <sys/types.h> 
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#define PORT "3490"
#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) { // typecasting to relevant IP type. 
		return &(((struct sockaddr_in*)sa)->sin_addr); 
		// sin_addr is a pointer to a struct in_addr, contains 
		// address in network byte order. 
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);  
}

int main(void) 
{
	int listenerSock, newSock; 
	struct sockaddr_storage remoteaddr;
	struct addrinfo hints, *results, *rp;
	struct pollfd poll_list[5];
	int status; 
	int yes = 1; 
	char msg[256];

	// clear up some memory for hints 
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use the IP of local machine. 

	if ((status = getaddrinfo(NULL, PORT, &hints, &results)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status)); 
	}

	// Creating sockets and binding to IP with members of addrinfo linked list.   
	for(rp = results; rp != NULL; rp = rp->ai_next) {
		if ((listenerSock = socket(rp->ai_family, rp-> ai_socktype, rp->ai_protocol)) == -1) {
			perror("server: socket");
			continue; 
		}

		// clear kernal of address already in use error, yes needs to be pointer. 
		setsockopt(listenerSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));  

		// ai_addr is pointer to struct sockaddr. 
		if (bind(listenerSock, rp->ai_addr, rp->ai_addrlen) == -1) {
			perror("server: bind"); 
			continue; 
		}
		
		break; 
	}
	
	/* provides control over open sockets, second param gets status flags
	 * and stores into the flags int. 
	 * FILE status flags: 
	 * - Shared by duplicate file descriptors that are a result of an open.
	 * - affect how operations act on I/O, in our case, we use NON_BLOCKING. 
	 *
	 *  Why do we want a non-blocking socket? Bcos we want to be able to POLL properly
	 *  When we don't block, you're essentially saying if there's no data in this socket
	 *  block(sleep) until data is ready instead of sending an error. This is when you want
	 *  multiple clients to connect.
	 *
	 *  - Returned default flag status is piped into O_NONBLOCK macro to be changed to the 
	 *  non blocking flags.
	 */ 
	freeaddrinfo(results); // finished binding and creation of sockets.
	
	int flags = fcntl(listenerSock, F_GETFL, 0);
		if(flags == -1){
			perror("Server: fcntl1");
			exit(1);
		}
	if(fcntl(listenerSock, flags | O_NONBLOCK) == -1){
		perror("Server: fcntl2");
		exit(1); 
	} 


	if (listen(listenerSock, BACKLOG) == -1) {
		perror("server: listen");
		exit(1); 
	}
	printf("Server listening .. ");

	// wanna set the first index of poll to be listening socket. 
	// Do we want it to be alerted for recv yes bcos it's gotta be ready to recv data. 

	memset(poll_list, 0, sizeof(poll_list)); 
	poll_list[0].fd = listenerSock;
	poll_list[0].events = POLLIN; // We want to know when it's ready to read. 

	// count the number of fds in the array. 
	int fdCount = 1; 
	
	// Infinite loop that runs poll and checks for existing connections. 
	while(1) {

		int pollCount = poll(poll_list, fdCount, -1); // -1 indefinite blocking.

		if (pollCount == -1){
			perror("server: poll"); 
			exit(1);
		}
		
		// Checking existing connections. First run is only gonna handle the listener fd.  
		for(int i = 0; i < fdCount; i++){
			// We only wanna know when the fd's are ready to receive data.
			if(poll_list[i].revents & POLLIN){
				// Listener socket that accepts new connections each iteration of the inf loop. 
				if(poll_list[i].fd == listenerSock) {
					socklen_t socksize = sizeof remoteaddr; 
					newSock = accept(listenerSock, (struct sockaddr *)&remoteaddr, &socksize);
					if (newSock == -1){
						perror("Server: accept");
						exit(1); 
					}
					// Adds the new connection the the array.
					poll_list[i].fd = newSock;
					poll_list[i].events = POLLIN; 
					
					// make it so that x client is replaced with the username specified by the client. 
					printf("X client joined"); 
				
				// If the array is not the listener socket it's an existing array that we wanna recv from.
				} else {
						// recv returns no of bytes sent out. Later on we can adjust this so that it can send
						// the whole string incase there's a partial send.
						int noBytes = recv(poll_list[i].fd, msg, sizeof(msg), 0);
					
						// Need to know which is the sending fd so we can send each message to respective
						// clients. 
						int sendFd = poll_list[i].fd;  

						// Error handling recv. 
						if (noBytes <= 0){
						// When recv returns 0 for stream socket connection means client shut down.
							if (noBytes == 0){
								printf("Client disconnected."); 
							} else { perror("recv");}
						
						// If there's an error with recv or the client disconnected then we gotta close socket. 
						close(poll_list[i].fd); 
						} else {
							// This is if good data comes in. Here is where we can do the partial send logic (later)
							
						}
					}
				}
			else {
				// If we're the client. 

			}
			
			}

		}
}

