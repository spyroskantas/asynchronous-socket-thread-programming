#include "linkedList.h"

linkedList *LinkedList_Create(){
	linkedList *list = malloc(sizeof(linkedList));
	list->header = malloc(sizeof(node_l));
	list->header->next = NULL;
	list->header->label = NULL;
	list->count = 0;
	return list;
}

node_l* LinkedList_Insert(linkedList* list, void* item, char* label){
	node_l *new;
	new = malloc(sizeof(node_l));
	new->item = item;
	new->label = label;
	new->next = list->header->next;
	list->header->next = new;
	list->count++;
	return new;
}

node_l* LinkedList_Search(linkedList* list, char* key){
	node_l *p=list->header->next;
	for(; p; p=p->next)
		if(strcmp(p->label, key) == 0)
			return p;
	return NULL;
}

void LinkedList_Destroy(linkedList* list){
	node_l *tmp;
	while(list->header->next){
		tmp=list->header->next;
		list->header->next = list->header->next->next;
		free(tmp);
	}
	free(list->header);
	free(list);
}
