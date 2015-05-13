// client.c 
// Parker Newton 
// 5-12-15 
// COEN 146L - T 2:15 pm 

// This program uses the Linux UDP Socket API to implement a stop-and-wait method 
//		for sending and receiving packets to and from a remote server.
// A simple checksum algorithm is implemented, and upon failure requests a retransmit from the the client. 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define SERVER_PORT 10063
#define MESSAGE_SIZE 1000 
#define NUM_PACKETS 5

// Each PACKET instance contains the following fields: 
// 		seq_num -- a sequence number to distinguish between transmitted packets 
// 		checksum -- error-detection mechanism 
// 		payload[] -- C-string that contains the data to transmit. 
typedef struct packet { 
	unsigned short seq_num; 
	unsigned short checksum; 
	char payload[MESSAGE_SIZE];
} PACKET; 

int main(int argc, char *argv[])
// Main driver function 
// Expects the following program parameters: 
// 		1) name of remote server host. 

{
	// identifier used to maintain the current sequence no. we are processing 
	// s stores the socket identifier 
	// local_checksum calculates the checksum of the payload on the client-side 
	// msgs[] are hard coded payloads which we wish to transmit. 
	// addr is a type-struct sockaddr_in instance; stores information about the address to which we wish to transmit 
	// addr_size stores the size of the type-struct sockaddr_in instance 
	// srvr is a type-struct hostent instance; stores information about the remote host to which we wish to transmit 
	// p_send and p_recv maintain pointers to instances of type-PACKET in which we will store sent and received packets. 
	unsigned int identifier = 0; 
	int s, i, j, local_checksum; 		  
	char *msgs[] = {"first", "second", "third", "fourth", "fifth"};  
	struct sockaddr_in addr; 
	socklen_t addr_size = sizeof(addr);		
	struct hostent *srvr; 	
	PACKET *p_send, *p_recv; 

	if(argc < 2){
		printf("\nError: not enough args to program.\n"); 
		return -1; 
	}

	// Establish a UDP socket 
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("\nError: cannot create a socket.\n"); 
		return -1; 
	}

	// Locate the remote server host 
	if((srvr = gethostbyname(argv[1])) == NULL){
		printf("\nError: could not verify host.\n");  
		close(s);
		return -1; 
	} 

	// Write 0's to the members of the sockaddr_in struct
	bzero((char *)&addr, sizeof(struct sockaddr_in)); 
 
	// AF_INET denotes address family for our socket as IP 
	addr.sin_family = AF_INET; 

	// Specify the IP address of the server 
	bcopy((char *)srvr->h_addr, (char *)&addr.sin_addr, srvr->h_length); 

	// Specify the server port 
	addr.sin_port = htons(SERVER_PORT); 
	
	// Allocate memory for our sent and received packet instances 
	if((p_send = (PACKET *)malloc(sizeof(PACKET))) == NULL){
		printf("\nERROR: memory could not be allocated. \n"); 
		close(s); 
		return -1; 
	}
	if((p_recv = (PACKET *)malloc(sizeof(PACKET))) == NULL){
		printf("\nERROR: memory could not be allocated. \n"); 
		free(p_send); 
		close(s); 
		return -1; 
	}

	for(i=0; i<NUM_PACKETS; ++i){
		strcpy(p_send->payload, msgs[i]); 
		p_send->seq_num = identifier; 
		local_checksum = 0; 
		for(j=0; j<strlen(p_send->payload); ++j) 
			local_checksum += p_send->payload[j]; 
		p_send->checksum = local_checksum; 

		// Send the packet over our UDP comm link 
		sendto(s, p_send, sizeof(*p_send), 0, (struct sockaddr *) &addr, addr_size); 

		// Receive the aknowledgement reply from the remote server 
		recvfrom(s, p_recv, sizeof(*p_recv), 0, (struct sockaddr *) &addr, &addr_size); 

		// If our checksum algorithm detects an error, then we will retransmit and wait for our aknowledgement reply 
		while(strcmp(p_recv->payload, "ERROR_CHECKSUM") == 0){ 
			sendto(s, p_send, sizeof(*p_send), 0, (struct sockaddr *) &addr, addr_size); 
			recvfrom(s, p_recv, sizeof(*p_recv), 0, (struct sockaddr *) &addr, &addr_size); 
		}

		// Process the received data packet 
		printf("\nClient Received Data:\n \nPayload:\t%s\nSequence No.:\t%d\nChecksum:\t%d\n", p_recv->payload, p_recv->seq_num, p_recv->checksum); 

		// Set the identifier to the received sequence number 
		// This will serve as the sequence number for the next packet to send 
		identifier = p_recv->seq_num; 
	}

	free(p_send); 
	free(p_recv); 

	// Close the socket 
	close(s); 	

	return 0; 
}