#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>

#define PACKET_SIZE 256

void send_data(int fd, char* packet, int bufferSize);
char* receive_data(int fd, int packetSize, int bufferSize);
