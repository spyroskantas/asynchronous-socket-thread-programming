#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node{
	char *label;
	void *item;
	struct node *next;
}node_l;

typedef struct{
	int count;
	node_l *header;
}linkedList;


linkedList *LinkedList_Create();
node_l* LinkedList_Insert(linkedList* list, void* item, char* label);
node_l* LinkedList_Search(linkedList* list, char* key);
void LinkedList_Destroy(linkedList* list);

