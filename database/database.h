#include "skipList.h"
#include "linkedList.h"
#include "bloomFilter.h"

#define MAX_LEVEL 100
#define BLOOM_HASHES 16

typedef struct{
	linkedList* listOfFilters;
	linkedList* listOfVacSkiplists;
	linkedList* listOfNotVacSkiplists;
	linkedList* listOfViruses;
	linkedList* listOfRecords;
	linkedList* listOfCountries;
}database;

database* Init_Database();
void vaccineStatusBloom(database* dtb, char* key, char* virusName);
citizenRecord* vaccineStatus(database* dtb, int key, char* virusName);
void insertCitizenRecord(database* dtb, int bloomSize, char* key, char* firstName, char* lastName, char* country, int age, char* virusName, char* vaccinated, char* dateVaccinated);
void Database_Destroy(database *dtb);