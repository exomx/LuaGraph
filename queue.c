#include "mainheader.h"

void QUE_Init(queue* que) {
	que->tail = NULL, que->count = 0;
}

void QUE_AddElement(queue* que, void* data) {
	queueelement* tmp_element = calloc(1, sizeof(queueelement));
	tmp_element->data = data, tmp_element->next = NULL, tmp_element->prev = NULL;
	if (!que->count) {
		que->tail = tmp_element;
	}
	else {
		queueelement* prev = que->tail;
		for (int i = 0; i < que->count - 1; ++i) {
			prev = prev->next;
		}
		tmp_element->prev = prev, prev->next = tmp_element;
	}
	que->count++;
}
void* QUE_Get(queue* que) {
	void* tmp_data = NULL;
	if (que->tail) {
		queueelement* next = que->tail->next;
		tmp_data = que->tail->data;
		free(que->tail); //data shouldn't be freed, the user should do that themselves
		que->tail = next;
		que->count--;
	}
	return tmp_data;
}