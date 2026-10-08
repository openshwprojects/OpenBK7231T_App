#ifndef __OBKHELPER_H__
#define __OBKHELPER_H__

//#include "obkdef.h"

#ifdef __cplusplus
extern "C" {
#endif

/**@{*/

/**
 * obk_container_of - return the start address of struct type, while ptr is the
 * member of struct type.
 */
#define obk_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))


/**
 * @brief initialize a list object
 */
#define OBK_LIST_OBJECT_INIT(object) { &(object), &(object) }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
static inline void obk_list_init(obk_list_t *l)
{
    l->next = l->prev = l;
}

/**
 * @brief insert a node after a list
 *
 * @param l list to insert it
 * @param n new node to be inserted
 */
static inline void obk_list_insert_after(obk_list_t *l, obk_list_t *n)
{
    l->next->prev = n;
    n->next = l->next;

    l->next = n;
    n->prev = l;
}

/**
 * @brief insert a node before a list
 *
 * @param n new node to be inserted
 * @param l list to insert it
 */
static inline void obk_list_insert_before(obk_list_t *l, obk_list_t *n)
{
    l->prev->next = n;
    n->prev = l->prev;

    l->prev = n;
    n->next = l;
}

/**
 * @brief remove node from list.
 * @param n the node to remove from the list.
 */
static inline void obk_list_remove(obk_list_t *n)
{
    n->next->prev = n->prev;
    n->prev->next = n->next;

    n->next = n->prev = n;
}

/**
 * @brief tests whether a list is empty
 * @param l the list to test.
 */
static inline int obk_list_isempty(const obk_list_t *l)
{
    return l->next == l;
}

/**
 * @brief get the list length
 * @param l the list to get.
 */
static inline unsigned int obk_list_len(const obk_list_t *l)
{
    unsigned int len = 0;
    const obk_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define obk_list_entry(node, type, member) \
    obk_container_of(node, type, member)

/**
 * obk_list_for_each - iterate over a list
 * @param pos the obk_list_t * to use as a loop cursor.
 * @param head the head for your list.
 */
#define obk_list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * obk_list_for_each_safe - iterate over a list safe against removal of list entry
 * @param pos the obk_list_t * to use as a loop cursor.
 * @param n another obk_list_t * to use as temporary storage
 * @param head the head for your list.
 */
#define obk_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
        pos = n, n = pos->next)

/**
 * obk_list_for_each_entry  -   iterate over list of given type
 * @param pos the type * to use as a loop cursor.
 * @param head the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define obk_list_for_each_entry(pos, head, member) \
    for (pos = obk_list_entry((head)->next, obk_typeof(*pos), member); \
         &pos->member != (head); \
         pos = obk_list_entry(pos->member.next, obk_typeof(*pos), member))

/**
 * obk_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @param pos the type * to use as a loop cursor.
 * @param n another type * to use as temporary storage
 * @param head the head for your list.
 * @param member the name of the list_struct within the struct.
 */
#define obk_list_for_each_entry_safe(pos, n, head, member) \
    for (pos = obk_list_entry((head)->next, obk_typeof(*pos), member), \
         n = obk_list_entry(pos->member.next, obk_typeof(*pos), member); \
         &pos->member != (head); \
         pos = n, n = obk_list_entry(n->member.next, obk_typeof(*n), member))

/**
 * obk_list_first_entry - get the first element from a list
 * @param ptr the list head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define obk_list_first_entry(ptr, type, member) \
    obk_list_entry((ptr)->next, type, member)

/**
 * @brief initialize a tree
 *
 * @param t tree to be initialized
 */
static inline void obk_stree_init(obk_stree_t *t)
{
    t->next = t->child = (void *)0;
}

/**
 * @brief append a node tree
 *
 * @param t tree to append it
 * @param n new node to be append
 */
static inline void obk_stree_append(obk_stree_t *t, obk_stree_t *n)
{    
	obk_stree_t *__node;

    __node = t;
    while (__node->next) __node = __node->next;

    /* append the node to the tail */
    __node->next = n;
    n->next = (void *)0;
}

/**
 * @brief add a node tree as child
 *
 * @param t tree to add child
 * @param n new node-child
 */
static inline void obk_stree_add_child(obk_stree_t *tn, obk_stree_t *n)
{
    tn->child = n;
}

/**
 * @brief get the struct for this single tree node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define obk_stree_entry(node, type, member) \
    obk_container_of(node, type, member)

/**
 * obk_stree_for_each - iterate over a single list
 * @param pos the obk_slist_t * to use as a loop cursor.
 * @param head the head for your single list.
 */
#define obk_stree_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)
	
/**
 * @brief initialize a single list
 *
 * @param l the single list to be initialized
 */
static inline void obk_slist_init(obk_slist_t *l)
{
    l->next = (void *)0;
}

static inline void obk_slist_append(obk_slist_t *l, obk_slist_t *n)
{
    struct obk_slist_node *node;

    node = l;
    while (node->next) node = node->next;

    /* append the node to the tail */
    node->next = n;
    n->next = (void *)0;
}

static inline void obk_slist_insert(obk_slist_t *l, obk_slist_t *n)
{
    n->next = l->next;
    l->next = n;
}

static inline unsigned int obk_slist_len(const obk_slist_t *l)
{
    unsigned int len = 0;
    const obk_slist_t *list = l->next;
    while (list != (void *)0)
    {
        list = list->next;
        len ++;
    }

    return len;
}

static inline obk_slist_t *obk_slist_pop(obk_slist_t *l)
{
    struct obk_slist_node *node = l;

    /* remove node */
    node = node->next;
    if (node != (obk_slist_t *)0)
    {
        ((struct obk_slist_node *)l)->next = node->next;
    }

    return node;
}

#define obk_slist_push(list, item) do {       \
        if ((list) && (item)) {               \
		    (item)->next = (list)->next;      \
			(list)->next = (item);            \
	    }                                     \
    } while (0)

static inline obk_slist_t *obk_slist_remove(obk_slist_t *l, obk_slist_t *n)
{
    /* remove slist head */
    struct obk_slist_node *node = l;
    while (node->next && node->next != n) node = node->next;

    /* remove node */
    if (node->next != (obk_slist_t *)0) node->next = node->next->next;

    return l;
}

static inline obk_slist_t *obk_slist_first(obk_slist_t *l)
{
    return l->next;
}

static inline obk_slist_t *obk_slist_tail(obk_slist_t *l)
{
    while (l->next) l = l->next;

    return l;
}

static inline obk_slist_t *obk_slist_next(obk_slist_t *n)
{
    return n->next;
}

static inline int obk_slist_isempty(obk_slist_t *l)
{
    return l->next == (void *)0;
}

/**
 * @brief get the struct for this single list node
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define obk_slist_entry(node, type, member) \
    obk_container_of(node, type, member)

/**
 * obk_slist_for_each - iterate over a single list
 * @param pos the obk_slist_t * to use as a loop cursor.
 * @param head the head for your single list.
 */
#define obk_slist_for_each(pos, head) \
    for (pos = (head)->next; pos != NULL; pos = pos->next)

/**
 * obk_slist_for_each_entry  -   iterate over single list of given type
 * @param pos the type * to use as a loop cursor.
 * @param head the head for your single list.
 * @param member the name of the list_struct within the struct.
 */
#define obk_slist_for_each_entry(pos, head, member) \
    for (pos = ((head)->next == (NULL) ? (NULL) : obk_slist_entry((head)->next, obk_typeof(*pos), member)); \
         pos != (NULL) && &pos->member != (NULL); \
         pos = (pos->member.next == (NULL) ? (NULL) : obk_slist_entry(pos->member.next, obk_typeof(*pos), member)))

/**
 * obk_slist_first_entry - get the first element from a slist
 * @param ptr the slist head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define obk_slist_first_entry(ptr, type, member) \
    obk_slist_entry((ptr)->next, type, member)

/**
 * obk_slist_tail_entry - get the tail element from a slist
 * @param ptr the slist head to take the element from.
 * @param type the type of the struct this is embedded in.
 * @param member the name of the slist_struct within the struct.
 *
 * Note, that slist is expected to be not empty.
 */
#define obk_slist_tail_entry(ptr, type, member) \
    obk_slist_entry(obk_slist_tail(ptr), type, member)


#endif //__OBKHELPER_H__