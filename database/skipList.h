#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
	int count;
	int category1;
	int category2;
	int category3;
	int category4;
}stats;

typedef struct{
	int citizenID;
	char *firstName;
	char *lastName;
	char *country;
	char age;
}basicInfo;

typedef struct{
	basicInfo *person;
	char *vaccinated;
	char *dateVaccinated;
}citizenRecord;

typedef struct snode{
	citizenRecord *record;
	struct snode **next;
}node_s;

typedef struct{
	int level;
	int maxlevel;
	node_s *header;
}skipList;

skipList* SkipList_Create(int maxlevel);
void SkipList_Insert(skipList* list, citizenRecord *record);
citizenRecord* SkipList_Search(skipList* list, int key);
int intDate(char* date);
stats* SkipList_Count(skipList* list, char* country, char* date1, char* date2, int sortByAge);
void SkipList_Print(skipList* list);
void SkipList_Delete(skipList* list, int key);
void SkipList_Destroy(skipList* list);

