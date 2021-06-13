#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<dirent.h>
#include<errno.h>
#include<netdb.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include"protocol.h"
#include"database.h"

#define PORTNUM 9000
#define BUFSIZE 64

int bufSiz, bloomSiz, numMonitors, accepted, rejected;
linkedList *listOfAccepted, *listOfRejected;

typedef struct{
	char *date, *country;
}request;

void report(linkedList* country_monitor){
	int i, fd;
	char buf[BUFSIZE], file[BUFSIZE];
	node_l *l;
	
	sprintf(file, "log_file.%d", getpid());
	if((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
		return;
	
	l = country_monitor->header->next;
	for(; l; l=l->next){
		sprintf(buf, "%s\n", l->label);
		write(fd, buf, strlen(buf));
	}
	
	sprintf(buf, "TOTAL TRAVEL REQUESTS %d\n", accepted+rejected);
	write(fd, buf, strlen(buf));
	sprintf(buf, "ACCEPTED %d\n", accepted);
	write(fd, buf, strlen(buf));
	sprintf(buf, "REJECTED %d", rejected);
	write(fd, buf, strlen(buf));
}

void receiveBloom(linkedList** monitor_bloom, int** fds, int i){
	int k, b, numOfBlooms;
	uint8_t *arr;
	char *packet, *virus;
	bloomFilter *bloom;
	
	monitor_bloom[i] = LinkedList_Create();
	packet = receive_data(fds[i][0], PACKET_SIZE, bufSiz);
	numOfBlooms = atoi(packet);
	free(packet);
	arr = malloc(bloomSiz);
	
	for(k=0; k<numOfBlooms; k++){
		
		virus = receive_data(fds[i][0], PACKET_SIZE, bufSiz);//receive virusName
		
		for(b=0; b<bloomSiz; b++){ //receive bloomFilter bytes
			packet = receive_data(fds[i][0], PACKET_SIZE, bufSiz);
			arr[b] = atoi(packet);
			free(packet);
		}
		bloom = Bloom_Create(bloomSiz, 16);
		Bloom_Set(bloom, arr);
		LinkedList_Insert(monitor_bloom[i], bloom, virus);
	}
	free(arr);
}

void travelRequest(linkedList** monitor_bloom, linkedList* country_monitor, int** fds, char* citizenID, char* date, char* countryFrom, char* countryTo, char* virusName){
	int i;
	char *vaccinated, *dateVaccinated, *country, *virus;
	node_l *l;
	request *req;
	
	if(!strlen(virusName)){
		printf("usage: /travelRequest citizenID date countryFrom countryTo virusName\n");
		return;
	}
	
	if(!(l=LinkedList_Search(country_monitor, countryFrom))){ printf("country not listed\n"); return;}
	i = *(int*)l->item;
	if(!(l=LinkedList_Search(monitor_bloom[i], virusName))){ printf("virus not listed\n"); return;}
	
	req = malloc(sizeof(request));
	req->date = malloc(strlen(date)+1);
	req->country = malloc(strlen(countryTo)+1);
	virus = malloc(strlen(virusName)+1);
	
	strcpy(req->date, date);
	strcpy(req->country, countryTo);
	strcpy(virus, virusName);
	
	if(Bloom_Check((bloomFilter*)l->item, citizenID)){ //if bloom check = maybe
		send_data(fds[i][1], "/travelRequest", bufSiz);
		send_data(fds[i][1], citizenID, bufSiz);
		send_data(fds[i][1], virusName, bufSiz);
		
		vaccinated = receive_data(fds[i][0], PACKET_SIZE, bufSiz);//get from monitor if vaccinated
		
		if(strcmp(vaccinated, "YES") == 0){ //if vaccinated
			dateVaccinated = receive_data(fds[i][0], PACKET_SIZE, bufSiz);
			
			if(intDate(dateVaccinated) > intDate(date)-180){ //6 months = 180 days
				accepted++;
				send_data(fds[i][1], "ACCEPTED", bufSiz);
				LinkedList_Insert(listOfAccepted, req, virus);
				printf("REQUEST ACCEPTED – HAPPY TRAVELS\n\n");
			}
			else{
				rejected++;
				send_data(fds[i][1], "REJECTED", bufSiz);
				LinkedList_Insert(listOfRejected, req, virus);
				printf("REQUEST REJECTED – YOU WILL NEED ANOTHER VACCINATION BEFORE TRAVEL DATE\n\n");
			}
			free(vaccinated);
			return;
		}
		free(vaccinated);
	}
	rejected++;
	send_data(fds[i][1], "REJECTED", bufSiz);
	LinkedList_Insert(listOfRejected, req, virus);
	printf("REQUEST REJECTED – YOU ARE NOT VACCINATED\n\n");
}

void travelStats(char* virusName, char* date1, char* date2, char* countryTo){
	node_l *l;
	int accepted=0, rejected=0;
	
	if(!strlen(date2)){
		printf("usage: /travelStats virusName date1 date2 [country]\n");
		return;
	}
	
	l = listOfAccepted->header->next;
	if(!strlen(countryTo)){
		
		for(; l; l=l->next){
			if(strcmp(l->label, virusName)) continue;
			if(intDate(((request*)l->item)->date) > intDate(date1) && intDate(((request*)l->item)->date) < intDate(date2))
				accepted++;
		}
		l = listOfRejected->header->next;
		for(; l; l=l->next){
			if(strcmp(l->label, virusName)) continue;
			if(intDate(((request*)l->item)->date) > intDate(date1) && intDate(((request*)l->item)->date) < intDate(date2))
				rejected++;
		}
	}
	else{
		for(; l; l=l->next){
			if(strcmp(l->label, virusName)) continue;
			if(intDate(((request*)l->item)->date) > intDate(date1) && intDate(((request*)l->item)->date) < intDate(date2))
				if(strcmp(((request*)l->item)->country, countryTo) == 0)
					accepted++;
		}
		l = listOfRejected->header->next;
		for(; l; l=l->next){
			if(strcmp(l->label, virusName)) continue;
			if(intDate(((request*)l->item)->date) > intDate(date1) && intDate(((request*)l->item)->date) < intDate(date2))
				if(strcmp(((request*)l->item)->country, countryTo) == 0){
					rejected++;}
		}
	}
	printf("\nTOTAL REQUESTS %d\n", accepted+rejected);
	printf("ACCEPTED %d\n", accepted);
	printf("REJECTED %d\n\n", rejected);
}

void addVaccinationRecords(linkedList** monitor_bloom, linkedList* country_monitor, int** fds, char* country){
	int i;
	node_l *l;
	
	if(!strlen(country)){
		printf("usage: /addVaccinationRecords country\n");
		return;
	}
	if(!(l=LinkedList_Search(country_monitor, country))){ printf("country not listed\n"); return;}
	i = *(int*)l->item;
	
	send_data(fds[i][1], "/addVaccinationRecords", bufSiz);
	//free old blooms
	l = monitor_bloom[i]->header->next;
	for(; l; l=l->next){
		Bloom_Destroy((bloomFilter*)l->item);
		free(l->label);
	}
	LinkedList_Destroy(monitor_bloom[i]);
	receiveBloom(monitor_bloom, fds, i);
}

void searchVaccinationStatus(int** fds, char* citizenID){
	int i, k, numViruses;
	char *packet, *virusName, *vaccinated, *date;
	
	for(i=0; i<numMonitors; i++){
		send_data(fds[i][1], "/searchVaccinationStatus", bufSiz);
		send_data(fds[i][1], citizenID, bufSiz);
	}
	i=-1;
	for(k=0; k<numMonitors; k++){ //find which monitor has the record
		packet = receive_data(fds[k][0], PACKET_SIZE, bufSiz);
		if(strcmp(packet, "EXISTS") == 0) i = k;
		free(packet);
	}
	if(i<0){ printf("\ncitizenID does not exist\n\n"); return;}
	
	packet = receive_data(fds[i][0], PACKET_SIZE, bufSiz); //get person info
	printf("\n%s\n", packet);
	free(packet);
	
	packet = receive_data(fds[i][0], PACKET_SIZE, bufSiz); //get virus count
	numViruses = atoi(packet);
	free(packet);
	
	for(k=0; k<numViruses; k++){
		virusName = receive_data(fds[i][0], PACKET_SIZE, bufSiz);
		vaccinated = receive_data(fds[i][0], PACKET_SIZE, bufSiz);
		
		if(strcmp(vaccinated, "YES") == 0){
			date = receive_data(fds[i][0], PACKET_SIZE, bufSiz); //get dateVaccinated
			printf("%s VACCINATED ON %s\n", virusName, date);
			free(date);
		}
		else
			printf("%s NOT YET VACCINATED\n", virusName);
	}puts("");
}

void destroy_quit(linkedList** monitor_bloom, linkedList* country_monitor, int** fds, int* port, char*** paths, int* counts){
	int i, k;
	node_l *l;
	
	report(country_monitor);
	for(i=0; i<numMonitors; i++) //exit monitor processes
		send_data(fds[i][1], "/exit", bufSiz);
	
	//free path array
	for(i=0; i<numMonitors; i++){
		for(k=0; k<counts[i]; k++)
			free(paths[i][k]);
		free(paths[i]);
	}
	free(paths);
	free(counts);
	
	//free bloomFilters
	for(i=0; i<numMonitors; i++){
		l = monitor_bloom[i]->header->next;
		for(; l; l=l->next){
			Bloom_Destroy((bloomFilter*)l->item);
			free(l->label);
		}
		LinkedList_Destroy(monitor_bloom[i]);
	}
	free(monitor_bloom);
	
	//free linkedList matching country with monitor#
	l = country_monitor->header->next;
	for(; l; l=l->next){
		free(l->item);
		free(l->label);
	}
	LinkedList_Destroy(country_monitor);
	
	//free accepted/rejected linkedLists
	l = listOfAccepted->header->next;
	for(; l; l=l->next){
		free(l->item);
		free(l->label);
	}
	l = listOfRejected->header->next;
	for(; l; l=l->next){
		free(l->item);
		free(l->label);
	}
	LinkedList_Destroy(listOfAccepted);
	LinkedList_Destroy(listOfRejected);
	
	//delete named pipes
	for(i=0; i<numMonitors; i++){ 
		close(fds[i][0]);
		close(fds[i][1]);
		free(fds[i]);
	}
	free(fds);
	free(port);
}


void readCommands(linkedList** monitor_bloom, linkedList* country_monitor, int** fds){
	size_t len=0;
	char command[25], *line=NULL, citizenID[25], date1[25], date2[25], countryFrom[25], countryTo[25], virusName[25];
	
	while(1){
		printf("travelMonitor>");
		scanf("%s", command);		//get command
		getline(&line, &len, stdin);
		
		if(strcmp(command, "/travelRequest") == 0){
			strcpy(virusName, "");
			sscanf(line, "%s %s %s %s %s", citizenID, date1, countryFrom, countryTo, virusName);
			travelRequest(monitor_bloom, country_monitor, fds, citizenID, date1, countryFrom, countryTo, virusName);
		}
		else if(strcmp(command, "/travelStats") == 0){
			strcpy(date2, "");
			strcpy(countryTo, "");
			sscanf(line, "%s %s %s %s", virusName, date1, date2, countryTo);
			travelStats(virusName, date1, date2, countryTo);
		}
		else if(strcmp(command, "/addVaccinationRecords") == 0){
			strcpy(countryFrom, "");
			sscanf(line, "%s", countryFrom);
			addVaccinationRecords(monitor_bloom, country_monitor, fds, countryFrom);
		}
		else if(strcmp(command, "/searchVaccinationStatus") == 0){
			strcpy(citizenID, "");
			sscanf(line, "%s", citizenID);
			searchVaccinationStatus(fds, citizenID);
		}
		else if(strcmp(command, "/exit") == 0){
			free(line);
			return;
		}
		else
			printf("unknown command \"%s\"\n", command);
	}
}

void monitorproc(int i, int portnum, char* numThreads, char* bufferSize, char* cyclicBufferSize, char* sizeOfBloom, char** path, int count){
	char port[BUFSIZE], **argv = malloc((count+7) * sizeof(char*));
	pid_t pid;
	
	sprintf(port, "%d", portnum);
	argv[0] = "./monitorServer";
	argv[1] = port;
	argv[2] = numThreads;
	argv[3] = bufferSize;
	argv[4] = cyclicBufferSize;
	argv[5] = sizeOfBloom;
	for(i=0; i<count; i++){
		argv[i+6] = path[i];}
	argv[count+6] = NULL;
	
	while(1){
		if((pid=fork()) < 0) continue;
		if(pid == 0)
			execv("./monitorServer", argv);
		free(argv);
		break;
	}
}

int main(int argc, char** argv){
	size_t len=0;
	int i, k, ret, *port, sock, count, *counts, *iptr, **fds;
	char *buf, *input_dir, *bufferSize, *cyclicBufferSize, *sizeOfBloom, *numThreads;
	char hostname[BUFSIZE], ***paths, command[BUFSIZE], citizenID[BUFSIZE], date1[BUFSIZE];
	char date2[BUFSIZE], countryFrom[BUFSIZE], countryTo[BUFSIZE], virusName[BUFSIZE];
	linkedList **monitor_bloom, *country_monitor = LinkedList_Create();
	struct dirent *direntp;
	DIR *dir;
	node_l *l;
	struct sockaddr_in server;
	struct hostent *rem;
	listOfAccepted = LinkedList_Create();
	listOfRejected = LinkedList_Create();
	
	accepted=0;
	rejected=0;
	
	//read flags & arguments
	if(argc<13){
		printf("usage: ./travelMonitorClient –m numMonitors -b socketBufferSize\
			-c cyclicBufferSize -s sizeOfBloom -i input_dir -t numThreads\n");
		exit(1);
	}
	
	for(i=0; i<argc; i++){
		
		if(strcmp(argv[i], "-m") == 0)
			numMonitors = atoi(argv[++i]);
		
		if(strcmp(argv[i], "-b") == 0){
			bufferSize = malloc(strlen(argv[++i]) + 1);
			strcpy(bufferSize, argv[i]);
		}
		if(strcmp(argv[i], "-c") == 0){
			cyclicBufferSize = malloc(strlen(argv[++i]) + 1);
			strcpy(cyclicBufferSize, argv[i]);
		}
		if(strcmp(argv[i], "-s") == 0){
			sizeOfBloom = malloc(strlen(argv[++i]) + 1);
			strcpy(sizeOfBloom, argv[i]);
		}
		if(strcmp(argv[i], "-i") == 0){
			input_dir = malloc(strlen(argv[++i]) + 1);
			strcpy(input_dir, argv[i]);
		}
		if(strcmp(argv[i], "-t") == 0){
			numThreads = malloc(strlen(argv[++i]) + 1);
			strcpy(numThreads, argv[i]);
		}
	}
	puts("Loading...");
	bufSiz = atoi(bufferSize);
	bloomSiz = atoi(sizeOfBloom);
	
	if(!(dir = opendir(input_dir))){ //open source directory
		fprintf(stderr, "cannot open folder: %s\n", input_dir);
		exit(0);
	}
	count = 0; //count sub-directories
	while((direntp = readdir(dir))){
		if(direntp->d_type == DT_DIR) count++;
	}
	count -= 2; //(.), (..) skipped
	rewinddir(dir);
	
	paths = malloc(numMonitors * sizeof(char**));
	counts = malloc(numMonitors * sizeof(int));
	for(i=0; i<numMonitors; i++){ //calculate how many sub-dirs child i gets
		if(count % numMonitors == i+1)
			counts[i] = count/numMonitors + 1;
		else
			counts[i] = count/numMonitors;
		paths[i] = malloc(counts[i]* sizeof(char*));
	}
	i=0; k=0;
	while((direntp = readdir(dir))){ //share subdirectories round robin
		
		if(direntp->d_type != DT_DIR) continue;
		if(strcmp(direntp->d_name, ".") == 0 || strcmp(direntp->d_name, "..") == 0) continue;
		
		paths[i][k] = malloc(strlen(input_dir) + strlen(direntp->d_name) + 2);
		sprintf(paths[i][k], "%s/%s", input_dir, direntp->d_name);
		buf = malloc(strlen(direntp->d_name)+1); //insert monitor num + country to linkedList
		strcpy(buf, direntp->d_name);
		iptr = malloc(sizeof(int));
		*iptr = i;
		LinkedList_Insert(country_monitor, iptr, buf);
		
		if(i == numMonitors-1){ i=0; k++;}
		else i++;
		
	}closedir(dir);
	
	port = malloc(numMonitors * sizeof(int));
	fds = malloc(numMonitors * sizeof(int*));
	
	k = PORTNUM;
	for(i=0; i<numMonitors; i++){ //get ports
		port[i] = k++;
		fds[i] = malloc(2 * sizeof(int));
		
	}
	for(i=0; i<numMonitors; i++){ //create monitor processes and connect
		monitorproc(i, port[i], numThreads, bufferSize, cyclicBufferSize, sizeOfBloom, paths[i], counts[i]);
		
		if((fds[i][0] = socket(PF_INET, SOCK_STREAM, 0)) < 0){ //open reading socket
			perror("socket"); exit(-1);
		}
		if((fds[i][1] = socket(PF_INET, SOCK_STREAM, 0)) < 0){ //open writing socket
			perror("socket"); exit(-1);
		}
		if(gethostname(hostname, sizeof(hostname)) < 0){ //Find server hostname
			perror("gethostname"); exit(-1);
		}
		if((rem = gethostbyname(hostname)) == NULL){ //Find server address
			perror("gethostbyname"); exit(-1);
		}
		server.sin_family = AF_INET;		//set addr
		server.sin_port = htons(port[i]);
		memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
		
		do{ //try connecting to monitor server i for reading
			ret = connect(fds[i][0], (struct sockaddr*)&server, sizeof(server));

		}while(ret < 0);
		do{ //try connecting to monitor server i for writing
			ret = connect(fds[i][1], (struct sockaddr*)&server, sizeof(server));

		}while(ret < 0);
	}
	monitor_bloom = malloc(numMonitors * sizeof(linkedList*));
	for(i=0; i<numMonitors; i++)//get bloomFilters from all children
		receiveBloom(monitor_bloom, fds, i);
	
	readCommands(monitor_bloom, country_monitor, fds);
	
	free(input_dir);	//free program
	free(numThreads);
	free(bufferSize);
	free(sizeOfBloom);
	free(cyclicBufferSize);
	
	destroy_quit(monitor_bloom, country_monitor, fds, port, paths, counts);
}
