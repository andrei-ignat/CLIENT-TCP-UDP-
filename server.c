
#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>

#include "helpers.h"


int match(const char *subject, const char *pattern) {
    while (*subject != '\0' && *pattern != '\0') {
        if (*pattern == '*') {
            while (*(pattern + 1) == '*') pattern++;
            if (*(pattern + 1) == '\0') return 1;
            while (*subject != '\0') {
                if (match(subject, pattern + 1)) return 1;
                subject++;
            }
            return 0; 
        } else if (*pattern == '+') {
            while (*subject != '\0' && *subject != '/') subject++;
            pattern++;
        } else {
            if (*subject != *pattern) return 0;
            subject++;
            pattern++;
        }
    }
    return *subject == '\0' && *pattern == '\0';
}


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

bool verificare2(int numar_clienti,char id1[10]){
	for(int i = 0; i < numar_clienti; i++){
		if(strcmp(id[i],id1) == 0)
		return true;
	}
	return false;
}

int gasire_clients(int numar_clienti,char id1[10]){
	int gasit = -1;
	for(int i = 0; i < numar_clienti; i++){
		if(strcmp(id[i],id1) == 0)
		gasit = i;
	}
	return gasit;
}


bool verificare(struct abonat abonat1[1000], int index,struct packet_to_server *msg) {
	for(int i = 0; i<subscriptions1[index]; i++){
		if(strcmp(abonat1[index].abonati[i],msg->abonat) == 0)
		return true;
	}
	return false;
}

int nr(struct abonat abonat1[1000], int index, struct packet_to_server *msg) {
	int k = 0;
	for(int i = 0; i<subscriptions1[index]; i++) {
		if(strcmp(abonat1[index].abonati[i],msg->abonat) == 0)
		k = i;
	}
	return k;
}

int verificare3(int index){
	int ok = 0;
	for(int i = 0;i<nr_conectati;i++){
		if(conectati[i] == index)
		ok = 1;
		break;
	}
	return ok;
}

void eliminare_conectare(int index) {
	int t;
	for(int i = 0; i < nr_conectati;i++){
		if(conectati[i] == index)
		t = i;
		break;
	}
	for(int k = t; k < nr_conectati - 1; k++){
		conectati[k] = conectati[k+1];
	}
	nr_conectati--;
}

void eliminare_deconectare(int index){
	int t;
	for(int i = 0; i < nr_deconectati;i++){
		if(neconectati[i] == index)
		t = i;
		break;
	}
	for(int k = t; k < nr_deconectati - 1; k++){
		neconectati[k] = neconectati[k+1];
	}
	nr_deconectati--;
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	int listen_tcp, listen_udp, port, numar_clienti = 0;
	char buf[256];
	socklen_t length = 0;
	struct abonat abonat1[100];

	struct sockaddr_in serv_addr, cli_addr, udp_addr;
	

	struct pollfd pfds[103];
	int filedescriptori = 0;

	listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listen_tcp < 0, "socket");

	listen_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(listen_udp < 0, "udp socket");

	port = atoi(argv[1]);
	DIE(port == 0, "atoi");

	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int ret = bind(listen_tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(port);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(listen_udp, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	int nagle = 1;
	ret = setsockopt(listen_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle, sizeof(int));
	DIE(ret < 0, "nagle");

    int tcp1 = listen(listen_tcp, 1000);
	DIE(tcp1 < 0, "listen");

	pfds[filedescriptori].fd = STDIN_FILENO;
	pfds[filedescriptori++].events = POLLIN;

	pfds[filedescriptori].fd = listen_udp;
	pfds[filedescriptori++].events = POLLIN;
	

	pfds[filedescriptori].fd = listen_tcp;
	pfds[filedescriptori++].events = POLLIN;


	while (1) {
		ret = poll(pfds, filedescriptori, -1);
		DIE(ret < 0, "poll");
		
		for(int i = 3; i < filedescriptori; i++) {
			if (pfds[i].revents & POLLIN){
				
			    char buffer[sizeof(struct packet_to_server)];
				memset(buffer, 0, sizeof(struct packet_to_server));
				int rc = recv_all(pfds[i].fd, buffer, sizeof(struct packet_to_server));
				if(rc < 0){
					for(int l = 0; l < filedescriptori; l++){
						close(pfds[l].fd);
		            }
		            exit(EXIT_FAILURE);
	            }
				struct packet_to_server *mesaj = (struct packet_to_server *) buffer;


				if (strcmp(mesaj->tip, "exit") == 0) {
						
					for (int t = i; t < filedescriptori - 1; t++) {
						pfds[t] = pfds[t + 1];   
						
					}
				    filedescriptori--;
					printf("Client %s disconnected.\n", id[i - 3]);
					eliminare_conectare(i - 3);
				    connection1[i - 3] = false;

					neconectati[nr_deconectati] = i - 3;
					nr_deconectati++;

			        close(pfds[i].fd);
				}
				else if (strcmp(mesaj->tip, "unsubscribe") == 0) {
						
					if(verificare(abonat1,i - 3,mesaj) == true) {
						size_t size_to_move = sizeof(abonat1[i - 3].abonati[0]) * (subscriptions1[i - 3] - nr(abonat1,i - 3,mesaj) - 1);
				        memmove(&abonat1[i - 3].abonati[nr(abonat1,i - 3,mesaj)], &abonat1[i - 3].abonati[nr(abonat1,i - 3,mesaj) + 1], size_to_move);
			            subscriptions1[i - 3]--;
			        }
				}
				
				else if (strcmp(mesaj->tip, "subscribe") == 0) {
					if(verificare(abonat1,i - 3,mesaj) == false){
				    strcpy(abonat1[i - 3].abonati[subscriptions1[i - 3]],mesaj->abonat);
				    subscriptions1[i - 3]++;
					}
				}
		        
			}
		}

		for(int i = 0; i <= 2; i++) {
			if(pfds[i].revents & POLLIN) {
				if(pfds[i].fd == STDIN_FILENO){
			
					int ret = read(STDIN_FILENO, buf, sizeof(&buf));					
					if(ret < 0){
						for(int i = 0; i < filedescriptori; i++){
							close(pfds[i].fd);
						}
				        exit(EXIT_FAILURE);
					}

					if (strncmp(buf, "exit",4) == 0) {
						for(int i = 0; i < filedescriptori; i++){
							close(pfds[i].fd);
						}
				        exit(EXIT_FAILURE);
			        }

				}
				else
				if(pfds[i].fd == listen_tcp) {
				
					length = sizeof(struct sockaddr);
					int client_socket = accept(listen_tcp, (struct sockaddr *)&cli_addr, &length);
					if(client_socket < 0){
						for (int i = 0; i < filedescriptori; i++){
							close(pfds[i].fd);
						}
						exit(EXIT_FAILURE);
					}

					char buf1[10];
					memset(buf1,0,10);

					int primire_id = recv_all(client_socket,buf1,10);
					if(primire_id < 0 ){
						for (int i = 0; i < filedescriptori; i++){
							close(pfds[i].fd);
						}
						exit(EXIT_FAILURE);
					}

					if(gasire_clients(numar_clienti,buf1) >= 0) {
						if(verificare3(gasire_clients(numar_clienti,buf1)) == 0) {
							eliminare_deconectare(gasire_clients(numar_clienti,buf1));
							connection1[gasire_clients(numar_clienti,buf1)] = true;
							conectati[nr_conectati] = gasire_clients(numar_clienti,buf1);
							nr_conectati++;
							pfds[filedescriptori].events = POLLIN;
							pfds[filedescriptori].fd = client_socket;
							filedescriptori++;
							printf("New client %s connected from %s:%d.", buf1,inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                            printf("\n");
						}
						else
						{
			                printf("Client %s already connected.", buf1);
							printf("\n");
							close(client_socket);

						}
					
					}
					else
					{   subscriptions1[numar_clienti] = 0;
						connection1[numar_clienti] = true;
						conectati[nr_conectati] = numar_clienti;
						nr_conectati++;

						memcpy(id[numar_clienti],buf1,sizeof(buf1));
						pfds[filedescriptori].events = POLLIN;
						pfds[filedescriptori].fd = client_socket;
						filedescriptori++;
						numar_clienti++;
						printf("New client %s connected from %s:%d\n", buf1, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					}
				}
				else
				if(pfds[i].fd == listen_udp) {
	
					char buffer[1501];
					memset(buffer,0,sizeof(buffer));
					int ret = recvfrom(listen_udp, buffer, sizeof(buffer), 0,(struct sockaddr *)&(udp_addr), &length);
					if(ret < 0) {
						for(int i = 0; i < filedescriptori; i++){
							close(pfds[i].fd);
						}
						exit(EXIT_FAILURE);
					}
					struct udp udp_pack;
					struct tcp tcp_pack;
					memset(&tcp_pack, 0, sizeof(struct tcp));
					memset(&udp_pack,0,sizeof(struct udp));
					memcpy(&udp_pack,&buffer,sizeof(struct udp));
					strcpy(tcp_pack.continut, udp_pack.continut);
					if(sizeof(tcp_pack.continut) < 50)
					tcp_pack.continut[sizeof(tcp_pack.continut) + 1] = '\0';
					tcp_pack.tip2 = udp_pack.tip;
					memset(tcp_pack.buffer,0,sizeof(tcp_pack.buffer));
					memset(tcp_pack.buffer1,0,sizeof(tcp_pack.buffer1));
					tcp_pack.buffer[1501] = '\0';
					tcp_pack.buffer1[1501] = '\0';
					memcpy(tcp_pack.buffer1 ,udp_pack.buffer,sizeof(udp_pack.buffer));
					for (int i = 0; i < filedescriptori - 3; i++) {
						if(connection1[i] == true || (verificare3(gasire_clients(numar_clienti,id[i])) == 0)){
							for (int j = 0; j < subscriptions1[i]; j++) {
								int s1 = (match(tcp_pack.continut,abonat1[i].abonati[j]) != 0);
								if (((strcmp(abonat1[i].abonati[j],tcp_pack.continut) == 0) || s1) && connection1[i] == true && tcp_pack.tip1 == 0) {
									tcp_pack.tip1 = 1;
									int ret = send_all(pfds[i+3].fd, &tcp_pack, sizeof(struct tcp));
									if(ret < 0) {
										for(int k = 0; k < filedescriptori; k++){
											close(pfds[k].fd);
								        }
							     	    exit(EXIT_FAILURE);
								    }
							    }
						    }
					    }
			     	}
			    }
			}
		}
	}

	for(int i = 0; i < filedescriptori; i++){
		close(pfds[i].fd);
	}
	return 0;
}
