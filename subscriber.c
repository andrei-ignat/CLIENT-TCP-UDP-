#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int recv_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while(bytes_remaining > 0) {
    int bytes = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
    if (bytes <= 0) {
      return bytes;
    }
    bytes_received += bytes;
    bytes_remaining -= bytes;
  }

  return bytes_received;
}

int send_all(int sockfd, void *buffer, size_t len) {
  size_t bytes_sent = 0;
  size_t bytes_remaining = len;
  char *buff = buffer;

  while (bytes_remaining > 0) {
    int bytes = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
    if (bytes <= 0) {
      return bytes; 
    }
    bytes_sent += bytes;
    bytes_remaining -= bytes;
  }

  return bytes_sent;
}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int tcp;
    uint16_t port;
    struct sockaddr_in serv_addr;

    struct pollfd pfds[2];
    int filedescriptori = 0;

    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "eroare");

    tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp < 0, "eroare");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	int ret = inet_pton(AF_INET,argv[2], &serv_addr.sin_addr.s_addr);
	DIE(ret <= 0, "eroare");

	int nagle = 1;
	ret = setsockopt(tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle, 4);
	DIE(ret < 0, "eroare");

	ret = connect(tcp, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "eroare");

	ret = send_all(tcp, argv[1], 10);
	DIE(ret < 0, "eroare");

    pfds[0].fd = tcp;
    pfds[0].events = POLLIN;

    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;

    while (1) {
        int ret = poll(pfds, 2, -1);
        DIE(ret < 0, "poll");

        if (pfds[0].revents & POLLIN) {
    
            struct tcp packet;
            char buffer1[sizeof(struct tcp)];
            memset(buffer1, 0,sizeof(struct tcp));
			memset(&packet, 0, sizeof(struct tcp));
         
            char continut[1501];
            memset(continut,0,1501);
            
            char tip[11];
            memset(tip,0,11);

			int ret = recv_all(tcp, &buffer1, sizeof(struct tcp));
			DIE(ret <= 0, "eroare");

            memcpy(&packet,&buffer1,sizeof(struct tcp));

            switch(packet.tip2) {
            case 0: {
                 uint32_t rez = ntohl(*(uint32_t *) (packet.buffer1 +1));
                 memcpy(tip, "INT",11);
                 if(packet.buffer1[0] == 0)
                 snprintf(continut,sizeof(continut),"%d",rez);
                 else
                 snprintf(continut,sizeof(continut),"%d",-rez);
                 break;
            }
            case 1: {
                 uint16_t rez = ntohs(*(uint16_t *) (packet.buffer1));
                 double rezultat = (rez * 1.00) / 100.0;
                 memcpy(tip, "SHORT_REAL",11);
                 sprintf(continut, "%.2f", rezultat);
                 break;
            }
            case 2: {
                uint32_t number = ntohl(* (uint32_t *)(packet.buffer1 + 1));
                float number1 = pow(10, packet.buffer1[5]);
                memcpy(tip,"FLOAT",11);
                if(packet.buffer1[0] == 0)
                sprintf(continut, "%.4f", (float)(number / number1));
                else
                sprintf(continut, "%.4f", -(float)(number / number1));
                break;
            }
            default: {
                memcpy(tip,"STRING",11);
                memcpy(continut,packet.buffer1,sizeof(packet.buffer1));
                break;
            }
            }
            printf("%s - %s - %s\n",packet.continut, tip, continut);
        }
        else if (pfds[1].revents & POLLIN) {
            struct packet_to_server pachet;
            char buf[100];
            memset(buf, 0, 100);
            fgets(buf,100,stdin);
            memset(&pachet, 0, sizeof(struct packet_to_server));
            char word1[100];
            char word2[100];
            sscanf(buf, "%s %s", word1, word2);

            if (strcmp(buf, "exit\n") == 0) {
                memcpy(pachet.tip,"exit",4);
                int ret = send_all(tcp, &pachet, sizeof(struct packet_to_server));
                DIE(ret < 0, "eroare");
                for(int i = 0; i < filedescriptori; i++){
                    close(pfds[i].fd);

                }
                exit(EXIT_FAILURE);
            }

            else if (strncmp(word1, "unsubscribe",11) == 0) {
                memcpy(pachet.tip,"unsubscribe",11);
                memcpy(pachet.abonat,word2, sizeof(pachet.abonat));
                int ret = send_all(tcp, &pachet, sizeof(struct packet_to_server));
                DIE(ret < 0, "eroare");
                printf("Unsubscribed to topic %s\n",pachet.abonat);
            }

            else if (strncmp(word1, "subscribe",11) == 0) {
                memcpy(pachet.tip,"subscribe",11);
    			memcpy(pachet.abonat, word2, sizeof(pachet.abonat));
                int ret = send_all(tcp, &pachet, sizeof(struct packet_to_server));
                DIE(ret < 0, "eroare");
                printf("Subscribed to topic %s\n",pachet.abonat);
            }
            
        }
    }
    close(tcp);

    return 0;
}