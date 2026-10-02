#ifndef OBKDEF_H
#define OBKDEF_H

#include <stdint.h>

#define OBK_NAME_MAX 16

typedef int32_t obk_err_t;

typedef enum obk_err_e {
	OBK_EBUSY = -2,
	OBK_ERROR = -1,                 /**< error */
	OBK_EOK = 0                     /**< no error */	
} obk_err_et;

/**
 * Double List structure
 */
struct obk_list_node
{
    struct obk_list_node *next;                          /**< point to next node. */
    struct obk_list_node *prev;                          /**< point to prev node. */
};
typedef struct obk_list_node obk_list_t;                 /**< Type for lists. */

/**
 * Single List structure
 */
struct obk_slist_node
{
    struct obk_slist_node *next;                         /**< point to next node. */
};
typedef struct obk_slist_node obk_slist_t;               /**< Type for single list. */

/**
 * Single Tree structure
 */
struct obk_stree_node
{
    struct obk_stree_node *next;                          /**< point to next node. */
    struct obk_stree_node *child;                         /**< point to child node. */
};
typedef struct obk_stree_node obk_stree_t;                 /**< Type for tree. */

/**
 * @brief Abstract device structure (like RThread)
 */ 
typedef struct obk_device {
	char					name[OBK_NAME_MAX];
	uint8_t                 type;
	uint8_t                 flags;
	void                   *user_data;
	obk_list_t   			list;                 /**< list node of device */	
} obk_device_t;

/**
 * @brief Abstract device structure (like RThread)
 */ 
typedef struct obk_service {
	char					name[OBK_NAME_MAX];
	uint8_t                 type;
	uint8_t                 flags;
	void                   *user_data;
	obk_list_t   			list;                 /**< list node of sevice */	
} obk_service_t;

#endif // OBKDEF_H