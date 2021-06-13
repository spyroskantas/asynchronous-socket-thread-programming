#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<dirent.h>
#include<netdb.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include"protocol.h"
#include"database.h"

#define BUFSIZE 64

char **cyclicBuffer;
int bufferSize, sizeOfBloom, accepted, rejected, totalPaths, pathCount, cyclicBufferCount, idx;
pthread_mutex_t mtx, tmtx;
pthread_cond_t cond_nonempty, cond_nonfull;

typedef struct{
	int numThreads, cyclicBufferSize;
	database* dtb;
	linkedList* insertedFiles;
}thread_args;

void report(char** path){
	int i, fd;
	char buf[BUFSIZE], file[BUFSIZE];
	
	sprintf(file, "log_file.%d", getpid());
	if((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
		return;
	for(i=0; path[i]; i++){
		write(fd, path[i], strlen(path[i]));
		write(fd, "\n", 1);
	}
	sprintf(buf, "TOTAL TRAVEL REQUESTS %d\n", accepted+rejected);
	write(fd, buf, strlen(buf));
	sprintf(buf, "ACCEPTED %d\n", accepted);
	write(fd, buf, strlen(buf));
	sprintf(buf, "REJECTED %d", rejected);
	write(fd, buf, strlen(buf));
}

void sendBloom(database* dtb, int* fd){
	int i;
	char *packet;
	node_l *l;
	
	packet = malloc(PACKET_SIZE); //send bloomFilter count
	sprintf(packet, "%d", dtb->listOfFilters->count);
	send_data(fd[1], packet, bufferSize);
	free(packet);
	
	l = dtb->listOfFilters->header->next;
	
	packet = malloc(PACKET_SIZE);
	for(; l!=NULL; l=l->next){
		send_data(fd[1], l->label, bufferSize); //send bloomFilter virusName
		for(i=0; i<sizeOfBloom; i++){ //convert each bloomFilter byte to ascii and send
			sprintf(packet, "%d", ((bloomFilter*)l->item)->bytes[i]);
			send_data(fd[1], packet, bufferSize);
		}
	}
	free(packet);
}

void insertDatabase(database* dtb, linkedList* insertedFiles, char* path){ //insert with update (if file already inserted dont insert)
	size_t len=0;
	int i, age;
	char *file, *line=NULL, *buf;
	char key[BUFSIZE], firstName[BUFSIZE], lastName[BUFSIZE], country[BUFSIZE];
	char virusName[BUFSIZE], vaccinated[BUFSIZE], dateVaccinated[BUFSIZE], date1[BUFSIZE], date2[BUFSIZE];
	DIR *dir;
	FILE *fp;
	node_l *l;
	struct dirent *direntp;
	
	if(!(dir = opendir(path))){
		fprintf(stderr, "monitor #%d: cannot open folder: %s\n", getpid(), path);
		exit(0);
	}
	while((direntp = readdir(dir))){
		if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) continue;
		if(LinkedList_Search(insertedFiles, direntp->d_name)) continue;
		file = malloc(strlen(path) + strlen(direntp->d_name) + 2);
		sprintf(file, "%s/%s", path, direntp->d_name);
		
		if(!(fp=fopen(file, "r"))){
			printf("monitor #%d: could not open file %s\n", getpid(), file);
			free(file);
			continue;
		}
		while(getline(&line, &len, fp) > 0){
			if(!len) continue; //skip empty line
			strcpy(vaccinated, "");
			strcpy(dateVaccinated, "");
			sscanf(line, "%s %s %s %s %d %s %s %s",\
	 		 key, firstName, lastName, country, &age, virusName, vaccinated, dateVaccinated);
	 		
	 		insertCitizenRecord(dtb, sizeOfBloom, key, firstName, lastName, "", age,\
	 		 virusName, vaccinated, dateVaccinated);
		}
		buf = malloc(strlen(direntp->d_name)+1);
		strcpy(buf, direntp->d_name);
		LinkedList_Insert(insertedFiles, NULL, buf);
		fclose(fp);
		free(file);
	}
	closedir(dir);
	free(line);
}

void travelRequest(database* dtb, int* fd){
	int key;
	char *packet;
	citizenRecord *record;
	
	packet = receive_data(fd[0], PACKET_SIZE, bufferSize); //get citizenID
	key = atoi(packet);
	free(packet);
	
	packet = receive_data(fd[0], PACKET_SIZE, bufferSize); //get virusName
	record = vaccineStatus(dtb, key, packet);
	free(packet);
	
	if(strcmp(record->vaccinated, "YES") == 0){
		send_data(fd[1], "YES", bufferSize); //send YES or NO
		send_data(fd[1], record->dateVaccinated, bufferSize); //send dateVaccinated from citizenRecord
	}
	else
		send_data(fd[1], "NO", bufferSize);
	
	packet = receive_data(fd[0], PACKET_SIZE, bufferSize); //get if request was accepted
	if(strcmp(packet, "ACCEPTED") == 0) accepted++;
	else if(strcmp(packet, "REJECTED") == 0) rejected++;
	free(packet);
}

void searchVaccinationStatus(database* dtb, int* fd){
	int key, found;
	char *packet;
	node_l *l, *p;
	basicInfo *person;
	citizenRecord *record;
	
	packet = receive_data(fd[0], PACKET_SIZE, bufferSize); //get citizenID
	key = atoi(packet);
	
	if((p=LinkedList_Search(dtb->listOfRecords, packet))){ //search database for record with id=citizenID
		free(packet);
		send_data(fd[1], "EXISTS", bufferSize); //send EXISTS so parent knows that this monitor has the record
		
		packet = malloc(PACKET_SIZE);
		person = (basicInfo*)p->item;
		sprintf(packet, "%d %s %s AGE %d", person->citizenID, person->firstName, person->lastName, person->age);
		send_data(fd[1], packet, bufferSize); //send person info
		
		sprintf(packet, "%d", dtb->listOfViruses->count);
		send_data(fd[1], packet, bufferSize); //send virus count
		free(packet);
		
		for(l=dtb->listOfViruses->header->next; l; l=l->next){
			send_data(fd[1], l->label, bufferSize); //send virusName
			
			if((record=vaccineStatus(dtb, key, l->label))){
				send_data(fd[1], "YES", bufferSize);
				send_data(fd[1], record->dateVaccinated, bufferSize); //send dateVaccinated
			}
			else
				send_data(fd[1], "NO", bufferSize);
		}
	}
	else
		send_data(fd[1], "NOT", bufferSize);
}


void setnonblocking(int sock){
	int flags;
	if((flags = fcntl(sock, F_GETFL)) < 0){
		perror("fnctl F_GETFL"); exit(-1);
	}
	if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0){
		perror("fnctl O_NONBLOCK"); exit(-1);
	}
}
void destroy_quit(thread_args* args, int sock, char** path){
	int i;
	node_l *l;
	
	close(sock);
	
	report(path);
	for(i=0; path[i]; i++){ //free path
		free(path[i]);
	}free(path);
	
	l = args->insertedFiles->header->next; //free insertedFiles list
	for(; l; l=l->next){
		free(l->item);
		free(l->label);
	}
	LinkedList_Destroy(args->insertedFiles);
	
	Database_Destroy(args->dtb); //free database
	free(args);
	exit(0);
}

void* thread_f(void* argp){
	thread_args *args = (thread_args*)argp;
	
	while(1){
		pthread_mutex_lock(&tmtx); //lock threads mutex
		pthread_mutex_lock(&mtx); //lock producer-consumer mutex
		if(!(pathCount+cyclicBufferCount)) break;
		if(!cyclicBufferCount) //if empty wait
			pthread_cond_wait(&cond_nonempty, &mtx); //unlock mutex, wait for signal
		if(idx == args->cyclicBufferSize) idx=0;
		cyclicBufferCount--; //decrease files-in-buffer number
		insertDatabase(args->dtb, args->insertedFiles, cyclicBuffer[idx++]); //insert files to database
		pthread_mutex_unlock(&mtx); //unlock mutex
		pthread_mutex_unlock(&tmtx); //unlock mutex
		pthread_cond_signal(&cond_nonfull);
	}
	pthread_mutex_unlock(&mtx); //unlock mutex
	pthread_mutex_unlock(&tmtx); //unlock mutex
	pthread_cond_signal(&cond_nonfull);
	pthread_exit(0);
}

void thread_sharing(database* dtb, thread_args* args, int* fd, char** path){
	int i, k, pathCount=totalPaths;
	pthread_t *thr;
	pthread_mutex_init(&mtx, 0);
	pthread_mutex_init(&tmtx, 0);
	pthread_cond_init(&cond_nonempty, 0);
	pthread_cond_init(&cond_nonfull, 0);
	
	thr = malloc(args->numThreads * sizeof(pthread_t));
	cyclicBuffer = malloc(args->cyclicBufferSize * sizeof(char*));
	
	for(i=0; i<args->numThreads; i++) //create threads
		if(pthread_create(thr+i, 0, thread_f, args)){
			perror("pthread_create"); exit(-1);
		}
	
	idx=0; k=0;
	for(i=0; path[i]; i++){ //filling cyclicBuffer
		pthread_mutex_lock(&mtx); //lock mutex
		if(cyclicBufferCount == args->cyclicBufferSize)
			pthread_cond_wait(&cond_nonfull, &mtx);
		if(k == args->cyclicBufferSize) k=0;
		cyclicBuffer[k++] = path[i];
		cyclicBufferCount++; //increase files-in-buffer number
		pathCount--; //decrease total-files number
		pthread_mutex_unlock(&mtx); //unlock mutex
		pthread_cond_signal(&cond_nonempty);
	}
	
	for(i=0; i<args->numThreads; i++) //wait for threads to exit
		pthread_join(*(thr+i), 0);
	
	sendBloom(dtb, fd); //send bloomFilters
	
	pthread_cond_destroy(&cond_nonempty);
	pthread_cond_destroy(&cond_nonfull);
	pthread_mutex_destroy(&mtx);
	free(cyclicBuffer);
	free(thr);
}

int main(int argc, char **argv){
	int i, k, port, sock, numThreads, cyclicBufferSize, opt = 1, fd[2];
	char **path, *packet, hostname[BUFSIZE];
	linkedList *insertedFiles = LinkedList_Create();
	database *dtb = Init_Database();
	struct dirent *direntp;
	DIR *dir;
	FILE *fp;
	struct sockaddr_in server, client;
	struct hostent *rem;
	int addr_len = sizeof(client);
	pthread_t *thr;
	thread_args *thr_args;
	
	accepted = 0;
	rejected = 0;
	
	port = atoi(argv[1]);
	numThreads = atoi(argv[2]);
	bufferSize = atoi(argv[3]);
	cyclicBufferSize = atoi(argv[4]);
	sizeOfBloom = atoi(argv[5]);
	
	path = malloc(((argc-6)+1) * sizeof(char*));
	pathCount = 0;
	for(i=0; i<argc-6; i++){ //get paths and calculate pathCount
		path[i] = malloc(strlen(argv[i+6])+1);
		strcpy(path[i], argv[i+6]);
		totalPaths++;
	}
	path[argc-6] = NULL; //null terminate array
	
	//open server sockets
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket"); exit(1);
	}
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){ //reuse address
		 perror("setsockopt"); exit(-1);
	}
	if(gethostname(hostname, sizeof(hostname)) < 0){ //Find server hostname
		perror("gethostname"); exit(-1);
	}
	if((rem = gethostbyname(hostname)) == NULL){ //Find server address
		perror("gethostbyname"); exit(-1);
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
	if(bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0){
		perror("bind"); exit(1);
	}
	if(listen(sock, 2) < 0){ //listening
		perror("listen"); exit(-1);
	} //accept connections
	if((fd[1] = accept(sock, (struct sockaddr*)&client, (socklen_t*)&addr_len)) < 0){ //writing fd
		perror("accept"); exit(-1);
	}
	if((fd[0] = accept(sock, (struct sockaddr*)&client, (socklen_t*)&addr_len)) < 0){ //reading fd
		perror("accept"); exit(-1);
	}
	setnonblocking(fd[0]); //make socket fds nonblocking
	setnonblocking(fd[1]);
	
	//create threads and read paths
	thr_args = malloc(sizeof(thread_args)); //fill thread arguments
	thr_args->dtb = dtb;
	thr_args->numThreads = numThreads;
	thr_args->insertedFiles = insertedFiles;
	thr_args->cyclicBufferSize = cyclicBufferSize;
	
	thread_sharing(dtb, thr_args, fd, path);
	
	while(1){
		packet = receive_data(fd[0], PACKET_SIZE, bufferSize); //get command
		
		if(strcmp(packet, "/travelRequest") == 0){
			travelRequest(dtb, fd);
		}
		else if(strcmp(packet, "/searchVaccinationStatus") == 0){
			searchVaccinationStatus(dtb, fd);
		}
		else if(strcmp(packet, "/addVaccinationRecords") == 0){
			thread_sharing(dtb, thr_args, fd, path);
		}
		else if(strcmp(packet, "/exit") == 0){
			free(packet);
			destroy_quit(thr_args, sock, path);
		}
		free(packet);
	}
}
