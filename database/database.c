#include "database.h"

database* Init_Database(){
	database *dtb = malloc(sizeof(database));
	dtb->listOfFilters = LinkedList_Create();
	dtb->listOfVacSkiplists = LinkedList_Create();
	dtb->listOfNotVacSkiplists = LinkedList_Create();
	dtb->listOfViruses = LinkedList_Create();
	dtb->listOfRecords = LinkedList_Create();
	dtb->listOfCountries = LinkedList_Create();
	return dtb;
}


citizenRecord* vaccineStatus(database* dtb, int key, char* virusName){
	node_l *p;
	citizenRecord *record;
	
	if((p=LinkedList_Search(dtb->listOfVacSkiplists, virusName)))
		if((record=SkipList_Search((skipList*)p->item, key)))
			return record;
	
	return NULL;
}

void insertCitizenRecord(database* dtb, int bloomSize, char* key, char* firstName, char* lastName, char* country, int age, char* virusName, char* vaccinated, char* dateVaccinated){
	char *buf;
	node_l *p, *tmp;
	citizenRecord *record;
	
	//skip faulty entries
	if(!strlen(virusName)) return;
	if(strcmp(vaccinated, "NO") == 0 && strlen(dateVaccinated)) return;
	
	//search database for virusName, insert if it does not exist
	if(!(tmp=LinkedList_Search(dtb->listOfViruses, virusName))){
		buf = malloc(sizeof(char)*strlen(virusName)+1);
		strcpy(buf, virusName);	
		tmp = LinkedList_Insert(dtb->listOfViruses, buf, buf);
	}
	
	if(strcmp(vaccinated, "YES") == 0){
		//search bloomfilter list for filter related to virusname
		if(!(p=LinkedList_Search(dtb->listOfFilters, virusName))) //create bloomfilter for virusname
			p = LinkedList_Insert(dtb->listOfFilters, Bloom_Create(bloomSize, BLOOM_HASHES), tmp->label);
		
		Bloom_Insert((bloomFilter*)p->item, key);
		
		//search vaccinated people skiplists for skiplist related to virusname
		if(!(p=LinkedList_Search(dtb->listOfVacSkiplists, virusName))) //create skiplist for virusname
			p = LinkedList_Insert(dtb->listOfVacSkiplists, SkipList_Create(MAX_LEVEL), tmp->label);
		
		//return, if entry id already exists
		if((record=SkipList_Search((skipList*)p->item, atoi(key)))){
			//printf("ERROR: CITIZEN %s ALREADY VACCINATED ON %s\n", key, record->dateVaccinated);
			return;
		}
		
		//if record exists in not vaccinated skiplist remove it
		if((tmp=LinkedList_Search(dtb->listOfNotVacSkiplists, virusName)))
			SkipList_Delete((skipList*)tmp->item, atoi(key));
		
	}
	else{
		//search not vaccinated people skiplists for skiplist related to virusname
		if(!(p=LinkedList_Search(dtb->listOfNotVacSkiplists, virusName))) //create skiplist for virusname	
			p = LinkedList_Insert(dtb->listOfNotVacSkiplists, SkipList_Create(MAX_LEVEL), tmp->label);
		
		else	//return, if entry id already exists
			if(SkipList_Search((skipList*)p->item, atoi(key))){
				//printf("ERROR IN RECORD %s\n", key);
				return;
			}
	}
	
	record = malloc(sizeof(citizenRecord));
	record->vaccinated = malloc(sizeof(char)*strlen(vaccinated)+1);
	record->dateVaccinated = malloc(sizeof(char)*strlen(dateVaccinated)+1);
	strcpy(record->vaccinated, vaccinated);
	strcpy(record->dateVaccinated, dateVaccinated);
	
	//search database for citizen info, insert if it does not exist
	if((tmp=LinkedList_Search(dtb->listOfRecords, key))){
		record->person = (basicInfo*)tmp->item;
	}
	else{//add person info to linked list
		record->person = malloc(sizeof(basicInfo));
		record->person->citizenID = atoi(key);
		record->person->firstName = malloc(sizeof(char)*strlen(firstName)+1);
		record->person->lastName = malloc(sizeof(char)*strlen(lastName)+1);
		strcpy(record->person->firstName, firstName);
		strcpy(record->person->lastName, lastName);
		record->person->age = age;
		
		//search database for country
		if((tmp=LinkedList_Search(dtb->listOfCountries, country)))
			record->person->country = (char*)tmp->item;
			
		else{//add country to country list
			buf = malloc(sizeof(char)*strlen(country)+1);
			strcpy(buf, country);
			record->person->country = buf;
			LinkedList_Insert(dtb->listOfCountries, buf, buf);
		}
		buf = malloc(sizeof(char*)*strlen(key)+1);
		strcpy(buf, key);
		LinkedList_Insert(dtb->listOfRecords, record->person, buf);
	}
	SkipList_Insert((skipList*)p->item, record);
	//printf("INSERTED RECORD %s\n", key);
}

void Database_Destroy(database *dtb){
	node_l *p;
	
	p = dtb->listOfFilters->header->next;			//free listOfFilters
	for(; p; p=p->next)
		Bloom_Destroy((bloomFilter*)p->item);
	LinkedList_Destroy(dtb->listOfFilters);
	
	p = dtb->listOfVacSkiplists->header->next;		//free listOfVacSkiplists
	for(; p; p=p->next)
		SkipList_Destroy((skipList*)p->item);
	LinkedList_Destroy(dtb->listOfVacSkiplists);
	
	p = dtb->listOfNotVacSkiplists->header->next;		//free listOfNotVacSkiplists
	for(; p; p=p->next)
		SkipList_Destroy((skipList*)p->item);
		
	LinkedList_Destroy(dtb->listOfNotVacSkiplists);
	
	p = dtb->listOfViruses->header->next;			//free listOfViruses
	for(; p; p=p->next){
		free(p->item);
	}
	LinkedList_Destroy(dtb->listOfViruses);
	
	p = dtb->listOfRecords->header->next;			//free listOfRecords
	for(; p; p=p->next){
		free(((basicInfo*)p->item)->firstName);
		free(((basicInfo*)p->item)->lastName);
		free(p->label);
		free(p->item);
	}
	LinkedList_Destroy(dtb->listOfRecords);
		
	p = dtb->listOfCountries->header->next;		//free listOfCountries
	for(; p; p=p->next)
		free(p->item);
	LinkedList_Destroy(dtb->listOfCountries);
	
	free(dtb);
}