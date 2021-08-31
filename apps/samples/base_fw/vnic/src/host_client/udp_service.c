#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Sixe of RX buffer */
#define RX_BUFFER_SIZE 1024
/* IP address of host*/
#define LINUX_HOST_IP "192.168.0.2"
/* Port to start the server on */
#define SERVER_PORT 4242

int main(int argc, char *argv[])
{
	/* socket address used for the server */
	struct sockaddr_in server_address;
	struct sockaddr_in zephyr_address;

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	/* htons: host to network short: transforms a value in host byte
	 * ordering format to a short value in network byte ordering format
	 */
	server_address.sin_port = htons(SERVER_PORT);

	/* htons: host to network long: same as htons but to long
	 * server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	 */
	int rv = inet_pton(AF_INET, LINUX_HOST_IP, &server_address.sin_addr);

	if (rv != 1) {
		printf("Failed to set ip in socket\n");
		return -1;
	}


	/* create a UDP socket, creation returns -1 on failure */
	int sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		printf("could not create socket\n");
		return -1;
	}

	/* bind it to listen to the incoming connections on the created server
	 * address, will return -1 on error
	 */
	if ((bind(sock, (struct sockaddr *)&server_address,
		  sizeof(server_address))) < 0) {
		printf("could not bind socket\n");
		return -1;
	}

	/* socket address used to store client address */
	struct sockaddr_in client_address;

	int client_address_len = sizeof(client_address);

	printf("Server loop started, port =%d\n", SERVER_PORT);
	char buffer[RX_BUFFER_SIZE];

	/* run indefinitely */
	while (true) {

		memset(&client_address, 0, sizeof(client_address));

		/* read content into buffer from an incoming client */
		int len = recvfrom(sock, buffer, sizeof(buffer), 0,
				   (struct sockaddr *)&client_address,
				   &client_address_len);

		/* inet_ntoa prints user friendly representation of the
		 * ip address
		 */
		buffer[len] = '\0';
		printf("received: '%s' from client %s\n", buffer,
		       inet_ntoa(client_address.sin_addr));

	}

	return 0;
}
