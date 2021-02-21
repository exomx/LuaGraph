#ifndef INCLUDED_LINKEDLIST_H
#define INCLUDED_LINKEDLIST_H
//this file is for linked lists
typedef struct listNode listNode;
struct listNode{
	listNode* next, * prev;
	void* data;
	int alloc;
};
typedef struct linkedList linkedList;
struct linkedList {
	listNode* head;
	int count;
};

void LIST_AddElement(linkedList* list, void* data) {
	listNode* tmp_node = calloc(1, sizeof(listNode));
	tmp_node->data = data;
	tmp_node->alloc = 1;
	if (!list->head)
		list->head = tmp_node, tmp_node->next = NULL, tmp_node->prev = NULL;
	else { //travese the list until the end is found
		if (!list->head->next)
			tmp_node->prev = list->head, tmp_node->prev->next = tmp_node, tmp_node->next = NULL;
		else {
			listNode* tmp_node_travlist = list->head;
			for (int i = 0; i < list->count; ++i) {
				if (tmp_node_travlist->next)
					tmp_node_travlist = tmp_node_travlist->next;
					
			}
			tmp_node_travlist->next = tmp_node, tmp_node->prev = tmp_node_travlist, tmp_node->next = NULL;
		}
	
	}
	++list->count;
}
int LIST_AddElementViaAlloc(linkedList* list, void* data) {
	listNode* tmp_node = calloc(1, sizeof(listNode));
	tmp_node->data = data;
	tmp_node->alloc = 1;
	if (!list->head)
		list->head = tmp_node, tmp_node->next = NULL, tmp_node->prev = NULL;
	else { //travese the list until the end is found
		if (!list->head->next)
			tmp_node->prev = list->head, tmp_node->prev->next = tmp_node, tmp_node->next = NULL;
		else {
			listNode* tmp_node_travlist = list->head;
			for (int i = 0; i < list->count; ++i) {
				if (tmp_node_travlist->alloc != 0)
					if (tmp_node_travlist->next)
						tmp_node_travlist = tmp_node_travlist->next;
					else
						return 1;
			}
			tmp_node_travlist->data = tmp_node->data, tmp_node_travlist->alloc = 1;
			return 0;
		}

	}
	
}
int LIST_RemoveAt(linkedList* list, int location, bool remove_data) {
	if (list->head) {
		listNode* tmp_node = list->head;
		for (int i = 0; i < location; ++i) {
			if (tmp_node->next)
				tmp_node = tmp_node->next;
			//else
			//	printf("element %d could not be found", location); return 1;
		}
		if (tmp_node->next && tmp_node->prev)
			tmp_node->prev->next = tmp_node->next, tmp_node->next->prev = tmp_node->prev;
		else if (list->head == tmp_node)
			list->head = tmp_node->next, tmp_node->next->prev = NULL;
		else if (!tmp_node->next)
			tmp_node->prev->next = NULL;
		else
			return 1;
		if (remove_data)
			free(tmp_node->data);
		free(tmp_node);
	}
	return 0;
}
int LIST_EmptyAt(linkedList* list, int location) {
	if (list->head) {
		listNode* tmp_node = list->head;
		for (int i = 0; i < location; ++i) {
			if (tmp_node->next)
				tmp_node = tmp_node->next;
			//else
			//	printf("element %d could not be found", location); return 1;
		}
		tmp_node->alloc = 0;
	}
	return 0;
}
void* LIST_At(const linkedList* const list, int location) {
	if (list->head) {
		listNode* tmp_node = list->head;
		for (int i = 0; i < location; ++i) {
			if (tmp_node->next)
				tmp_node = tmp_node->next;
			//else
			//	printf("element %d could not be found", location); return 2.0;
		}
		if (tmp_node->alloc)
			return tmp_node->data;
		else
			return NULL;
	}
	else
		return NULL;
}

void LIST_EditAt(const linkedList* const list, int location, void* new_data) {
	if (list->head) {
		listNode* tmp_node = list->head;
		for (int i = 0; i < location; ++i) {
			if (tmp_node->next)
				tmp_node = tmp_node->next;
			//else
			//	printf("element %d could not be found", location); return 2.0;
		}
		tmp_node->data = new_data;
	}

}

#endif
