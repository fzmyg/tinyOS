#ifndef __LIB_KERNEL_LIST_H__
#define __LIB_KERNEL_LIST_H__

#include"stdint.h"


#define offset(struct_type,struct_mumber_name) ((uint32_t)(&((struct_type *)0)->struct_mumber_name))

#define elem2entry(struct_type,struct_mumber_name,elem_ptr) ((struct type*)((uint32_t)elem_ptr - offset(struct_type,struct_mumber_name)))

typedef struct list_elem{
	struct list_elem*prev,*next;
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

#endif
