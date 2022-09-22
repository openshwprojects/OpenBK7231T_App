/*
 * cJSON_N.h
 *
 *  Created on: Apr 26, 2022
 *      Author: Dell
 */

#ifndef APPS_OPENBK7231T_APP_SRC_CJSON_N_H_
#define APPS_OPENBK7231T_APP_SRC_CJSON_N_H_


#include <stddef.h>

#define JSON_VALUE_STRING	1
#define JSON_VALUE_NUMBER	2
#define JSON_VALUE_OBJECT	3
#define JSON_VALUE_ARRAY	4
#define JSON_VALUE_TRUE		5
#define JSON_VALUE_FALSE	6
#define JSON_VALUE_NULL		7

typedef struct __json_value json_value_t;
typedef struct __json_object json_object_t;
typedef struct __json_array json_array_t;

#ifdef __cplusplus
extern "C"
{
#endif

json_value_t *json_value_parse(const char *doc);
json_value_t *json_value_create(int type, ...);
void json_value_destroy(json_value_t *val);
#ifndef json_int_t
#ifndef _MSC_VER
#include <inttypes.h>
#include <stdlib.h>

#define json_int_t uint64_t
#else
#define json_int_t __int64
#endif
#endif
int json_value_type(const json_value_t *val);
const char *json_value_string(const json_value_t *val);
json_int_t json_value_number(const json_value_t *val);
json_object_t *json_value_object(const json_value_t *val);
json_array_t *json_value_array(const json_value_t *val);

const json_value_t *json_object_find(const char *name,
									 const json_object_t *obj);
int json_object_size(const json_object_t *obj);
const char *json_object_next_name(const char *prev,
								  const json_object_t *obj);
const json_value_t *json_object_next_value(const json_value_t *prev,
										   const json_object_t *obj);
const json_value_t *json_object_append(json_object_t *obj,
									   const char *name,
									   int type, ...);

int json_array_size(const json_array_t *arr);
const json_value_t *json_array_next_value(const json_value_t *val,
										  const json_array_t *arr);
const json_value_t *json_array_append(json_array_t *arry,
									  int type, ...);

#ifdef __cplusplus
}
#endif

#define json_object_for_each(name, val, obj) \
	for (name = NULL, val = NULL; \
		 name = json_object_next_name(name, obj), \
		 val = json_object_next_value(val, obj), val; )

#define json_array_for_each(val, arr) \
	for (val = NULL; val = json_array_next_value(val, arr), val; )



#endif /* APPS_OPENBK7231T_APP_SRC_CJSON_N_H_ */
