#include"protocol.h"
#include<errno.h>

void send_data(int fd, char* packet, int bufferSize){
	int n=0, sum=0, size = strlen(packet);
	//printf("sent: %s\n", packet);
	do{
		if((n=write(fd, packet+sum, bufferSize-sum%bufferSize)) == -1) n=0;
		sum+=n;
		
	}while(sum < size || sum % bufferSize);
	
	write(fd, "", 1);
}

char* receive_data(int fd, int packetSize, int bufferSize){
	int n=0, sum=0;
	char buf[bufferSize+1], *packet = malloc(packetSize);
	fd_set rfds;
	
	strcpy(packet, "");
	
	while(1){
		
		while(1){//read bufferSize
			buf[n]='\0';
			strcat(packet, buf);
			sum+=n;
			if(!(sum%bufferSize) && sum) break;
			FD_ZERO(&rfds);
			FD_SET(fd, &rfds);
			if(select(fd+1, &rfds, NULL, NULL, NULL) < 0){n=0; continue;}
			if((n=read(fd, buf, bufferSize-sum%bufferSize)) < 0){n=0; continue;}
		}
		
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		if(select(fd+1, &rfds, NULL, NULL, NULL) < 0){n=0; continue;}
		if((n=read(fd, buf, 1)) == -1){n=0; continue;}
		if(!(*buf)) break;
		
	}
	//printf("reveived: %s\n", packet);
	return packet;
}
