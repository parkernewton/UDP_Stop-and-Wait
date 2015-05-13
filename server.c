// server.c 
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
#include <ctype.h> 

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
{
	// s stores the socket identifier 
	// local_checksum calculates the checksum of the payload on the server-side 
	// addr is a type-struct sockaddr_in instance; stores information about the address to which we wish to transmit 
	// addr_size stores the size of the type-struct sockaddr_in instance  
	// p_send and p_recv maintain pointers to instances of type-PACKET in which we will store sent and received packets.  
	int s, i, j, local_checksum; 	
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(addr); 				
	PACKET *p_recv, *p_send; 		 		

	// Establish a UDP socket 
	if((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
		printf("\nError: cannot create a socket.\n"); 
		return -1; 
	}

	// Write 0's to the members of the sockaddr_in struct
	bzero((char *)&addr, sizeof(struct sockaddr_in)); 
 
	// AF_INET denotes address family for our socket as IP 
	addr.sin_family = AF_INET;  

	// We accept any IP address to bind the socket to 
	addr.sin_addr.s_addr = INADDR_ANY; 

	// Specify the server port 
	addr.sin_port = htons(SERVER_PORT);

	// Bind socket to server IP addr and server port
	if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		printf("\nError: server bind failed. %s\n", strerror(errno));   
		close(s); 
		return -1; 
	} 

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
		local_checksum = 0; 

		// Receive the packet from the client 
		recvfrom(s, p_recv, sizeof(*p_recv), 0, (struct sockaddr *) &addr, &addr_size); 

		// Calculate the checksum locally 
		for(j=0; j<strlen(p_recv->payload); ++j)
			local_checksum += p_recv->payload[j];  

		// To test how the checksum algorithm handles a detected error, un-comment the following line. 
		//local_checksum++;  

		// If a checksum error is detected, then we will request a retransmit from the client 
		while(local_checksum != p_recv->checksum){
			printf("\nERROR: Checksum does not match. Requesting a retransmit...\n"); 
			strcpy(p_send->payload, "ERROR_CHECKSUM"); 
			p_send->seq_num = p_recv->seq_num; 
			p_send->checksum = local_checksum; 
			sendto(s, p_send, sizeof(*p_send), 0, (struct sockaddr *) &addr, addr_size); 
			recvfrom(s, p_recv, sizeof(*p_recv), 0, (struct sockaddr *) &addr, &addr_size);

			// Once we receive a retransmitted packet, then re-calculate the checksum locally to ensure the packet was not corrupted again 
			local_checksum = 0; 
			for(j=0; j<strlen(p_recv->payload); ++j)
				local_checksum += p_recv->payload[j];
		}

		// Success ! 
		printf("\nServer Received Data:\nPayload:\t%s\nSequence No.:\t%d\nChecksum:\t%d\n", p_recv->payload, p_recv->seq_num, p_recv->checksum); 

		// Convert each char in the payload to upper-case 
		for(j=0; j<strlen(p_recv->payload); ++j)
			p_send->payload[j] = toupper(p_recv->payload[j]); 
		p_send->payload[j] = '\0'; 

		// Increment the sequence number 
		p_send->seq_num = ++(p_recv->seq_num); 
		p_send->checksum = p_recv->checksum; 

		// Send the awknowledgement reply 
		sendto(s, p_send, sizeof(*p_send), 0, (struct sockaddr *) &addr, addr_size); 
	}

	free(p_send); 
	free(p_recv); 

	// Close the socket 
	close(s); 	
	
	return 0; 
}