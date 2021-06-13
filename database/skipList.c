#include "skipList.h"


skipList* SkipList_Create(int maxlevel){
	int i;
	skipList *list = malloc(sizeof(skipList));
	list->level = 1;
	list->maxlevel = maxlevel;
	list->header = malloc(sizeof(node_s));
	list->header->next = malloc(sizeof(node_s*) * maxlevel);
	//init header node next pointers to NULL
	for(i=0; i<maxlevel; i++)
		list->header->next[i] = NULL;
	return list;
}

int randomLevel(int max){
	int level = 1;
	while(rand() < RAND_MAX/2 && level < max) //rand() < RAND_MAX/2 has 1/2 chance like flip a coin
		level++;
	return level;
}

void SkipList_Insert(skipList* list, citizenRecord *record){
	int i, level, key = record->person->citizenID;
	node_s *update[list->maxlevel];
	node_s *new, *p = list->header;
	
	//start search from top level storing pointers to previous nodes before descenting level
	for(i=list->level-1; i>=0; i--){
		for(; p->next[i]; p=p->next[i]){
			if(p->next[i]->record->person->citizenID > key) break;
			if(p->next[i]->record->person->citizenID == key) return;
		}
		//update[i] holds the pointer to the i'th level node which will be affected by the new
		update[i] = p;
	}
	
	level = randomLevel(list->maxlevel);
	if(level > list->level){
		for(i=list->level; i<level; i++)
			update[i] = list->header;
		list->level = level;
	}
	new = malloc(sizeof(node_s));
	new->record = record;
	new->next = malloc(sizeof(node_s*) * list->maxlevel);
	for(i=0; i<level; i++){
		new->next[i] = update[i]->next[i];
		update[i]->next[i] = new;
	}
}

citizenRecord* SkipList_Search(skipList* list, int key){
	int i;
	node_s *p = list->header;
	for(i=list->level-1; i>=0; i--){
		for(; p->next[i]; p=p->next[i]){
			if(p->next[i]->record->person->citizenID > key) break;
			if(p->next[i]->record->person->citizenID == key){
				return p->next[i]->record;
			}
		}
	}
	return NULL;
}

int intDate(char* date){   // string dd/mm/yy to int for comparisons, counting days from 01-01-2000
	int i, n, k=0, val=0, arr[]={1, 30, 365}, *weight=arr, length=strlen(date);
	char *tmp = calloc(5, sizeof(char));
	
	for(i=0; i<length+1; i++){
		if(date[i] == '-'){
			n=atoi(tmp)-1;
			val += *weight * n;
			k=0; weight++;
			continue;
		}
		else if(date[i] == '\0'){
			n=atoi(tmp)-2000;
			free(tmp);
			return val += *weight * n;
		}
		tmp[k] = date[i];
		k++;
	}
}

stats* SkipList_Count(skipList* list, char* country, char* date1, char* date2, int sortByAge){
	node_s *p = list->header->next[0];
	stats *st = calloc(1, sizeof(stats));
	//traverse level 0 which has all records
	for(; p; p=p->next[0]){
		if(strcmp(p->record->person->country, country) == 0){
			if(!strlen(date1) || strcmp(p->record->vaccinated, "NO") == 0 || \
			 (intDate(date1) < intDate(p->record->dateVaccinated)) && \
			 (intDate(p->record->dateVaccinated) < intDate(date2))){
				st->count++;
				if(sortByAge)
					if(p->record->person->age < 20) st->category1++;
					else if(p->record->person->age < 40) st->category2++;
					else if(p->record->person->age < 60) st->category3++;
					else st->category4++;
			}
		}
	}
	return st;
}

void SkipList_Print(skipList* list){
	node_s *p = list->header->next[0];
	for(; p; p=p->next[0])
		printf("%d %s %s %s %d\n",\
		 p->record->person->citizenID, p->record->person->firstName,\
		  p->record->person->lastName, p->record->person->country, p->record->person->age);
}

void SkipList_Delete(skipList* list, int key){
	int i, found=0, nodeLevel=0;
	node_s *update[list->maxlevel];
	node_s *p = list->header;
	
	//start search from top level storing pointers to previous nodes before descenting level
	for(i=list->level-1; i>=0; i--){
		for(; p->next[i]; p=p->next[i]){
			if(p->next[i]->record->person->citizenID > key) break;
			if(p->next[i]->record->person->citizenID == key){
				found=1; break;     
			}
		}
		if(found){
			if(!nodeLevel) nodeLevel = i+1; //level of node to be deleted
			update[i] = p; //update[i] holds the pointer to the i'th level node which will be affected by deleted node
		}
	}
	
	if(!(p=p->next[0])) return; //record does not exist
	if(p->record->person->citizenID != key) return; //record does not exist
	
	for(i=0; i<nodeLevel; i++)
		update[i]->next[i] = p->next[i];
	
	free(p->record->vaccinated);
	free(p->record->dateVaccinated);
	free(p->record);
	free(p->next);
	free(p);
	
	//update list's level
	for(i=list->level-1; i>=0; i--)
		if(list->header->next[i]){
			list->level = i+1;
			return;
		}
}

void SkipList_Destroy(skipList* list){
	node_s *p = list->header;
	while(p->next[0])
		SkipList_Delete(list, p->next[0]->record->person->citizenID);
	
	free(list->header->next);
	free(list->header);
	free(list);
}
