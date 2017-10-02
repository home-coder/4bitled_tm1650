#ifndef _LIST_H
#define _LIST_H

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
				(type *)( (char *)__mptr - offsetof(type,member) );})

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->prev = list;
	list->next = list;
}

static inline void __list_add(struct list_head *new, \
	struct list_head *list, struct list_head *next)
{
	list->next = new;
	new->next = next;
	new->prev = list;
	next->prev = new;
}

static inline void list_add(struct list_head *new, struct list_head *list)
{
	__list_add(new, list, list->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *list)
{
	__list_add(new, list->prev, list);
}

/*
 FIXME:how to free del list ?
*/
static inline void list_del(struct list_head *ptr)
{
	ptr->prev->next = ptr->next;
	ptr->next->prev = ptr->prev;
	//TODO
	ptr->next = (void *)0;
	ptr->prev = (void *)0;
}

static inline void list_replace(struct list_head *old, struct list_head *new)
{
	old->prev->next = new;
	new->next = old->next;
	new->prev = old->prev;
	old->next->prev = new;
}

/*
list_entry, list_for_each, list_for_each_entry
*/
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(ptr, list) \
	for (ptr = (list)->next; ptr != (list); ptr = ptr->next)

#define list_for_each_entry(ptr, head, member) \
	for (ptr = list_entry(head->next, typeof(*ptr), member); \
		&ptr->member != head; ptr = list_entry(ptr->member.next, \
			typeof(*ptr), member))

#endif /*_LIST_H*/
