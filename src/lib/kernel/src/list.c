/*
typedef struct list_elem{
        sturct list_elem*prev,*next;
}ListElem;

typedef struct list{
        struct list_elem head;
        struct list_elem tail;
}List;

extern void list_init(struct list*);

extern void list_insert_before(struct list_elem*before,struct list_elem*elem);

extern void list_push(struct list*plist,struct list_elem*elem);

extern void list_append(struct list*plist,struct list_elem*elem);

extern void list_iterate(struct list*plist);

extern void list_append(struct list*plist,struct list_elem*elem);

extern void list_remove(struct list_elem* pelem);

extern struct list_elem* list_pop(struct list*plist);

extern bool list_empty(struct list*plist);

extern uint32_t list_len(struct list*plist);

extern struct list_elem* list_traversal(struct list*plist,bool (*func)(struct list_elem*,int),int arg);

extern bool find_elem(struct list*plist,struct list_elem*obj_elem);

 * */
#include"list.h"
#include"debug.h"
#include"interrupt.h"
#include"string.h"
void list_init(struct list*plist)
{
	ASSERT(plist!=NULL);
	plist->head.prev = NULL;
	plist->head.next = &(plist->tail);
	plist->tail.prev = &(plist->head);
	plist->tail.next = NULL;
}

void list_insert_before(struct list_elem*before,struct list_elem*ele)
{
	ASSERT(before!=NULL&&ele!=NULL);
	enum int_status status=closeInt();	
	before->prev->next = ele;
	ele->next = before;
	ele->prev = before->prev;
	before->prev = ele;
	setIntStatus(status);
}

void list_push(struct list*plist,struct list_elem*elem)
{
	ASSERT(plist!=NULL&&elem!=NULL);
	list_insert_before(plist->head.next,elem);
}

void list_append(struct list*plist,struct list_elem*elem)
{
	ASSERT(plist!=NULL&&elem!=NULL);
	list_insert_before(&plist->tail,elem);
}

void list_remove(struct list_elem*elem)
{
	ASSERT(elem!=NULL);
	enum int_status status = closeInt();
	elem -> prev -> next = elem->next;	
	elem -> next -> prev = elem->prev;
	elem -> prev =  NULL;
	elem -> next =  NULL;
	setIntStatus(status);
}

struct list_elem*list_pop(struct list*plist)
{
	ASSERT(plist!=NULL);
	struct list_elem* p = plist->head.next;
	list_remove(p);
	return p;
}

bool find_elem(struct list*plist,struct list_elem*elem)
{
	ASSERT(plist!=NULL&&elem!=NULL);
	struct list_elem* p = plist->head.next;
	while(p!=&(plist->tail)){
		if(p==elem) return true;
		p=p->next;
	}
	return false;
}


bool list_empty(struct list*plist)
{

	ASSERT(plist!=NULL);
	return (plist->head.next == &(plist->tail));

}

uint32_t list_len(struct list*plist)
{
	ASSERT(plist!=NULL);
	uint32_t len = 0;
	struct list_elem*p=plist->head.next;
	while(p!=&(plist->tail)){
		p=p->next;
		len ++;
	} 
	return len;
}


struct list_elem* list_traversal(struct list*plist,bool (*func)(struct list_elem*,int),int arg)
{
	ASSERT(plist!=NULL&&func!=NULL);
	struct list_elem* p = plist->head.next;
	while(p!=&(plist->tail)){
		if(func(p,arg)){
			return p;
		}
		p=p->next;
	}
	return NULL;
}
