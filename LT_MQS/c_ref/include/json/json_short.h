/*
    json_short.h
    abbreviation for json function
    write by zouql  20120924
*/

#ifndef JSON_SHORT_H
#define JSON_SHORT_H

#include "json.h" 

/*
Functions
json_object * 	json_object_get (struct json_object *this)
void 	json_object_put (struct json_object *this)
int 	json_object_is_type (struct json_object *this, enum json_type type)
enum json_type 	json_object_get_type (struct json_object *this)
char * 	json_object_to_json_string (struct json_object *this)
json_object * 	json_object_new_object ()
lh_table * 	json_object_get_object (struct json_object *this)
void 	json_object_object_add (struct json_object *this, char *key, struct json_object *val)
json_object * 	json_object_object_get (struct json_object *this, char *key)
void 	json_object_object_del (struct json_object *this, char *key)
json_object * 	json_object_new_array ()
array_list * 	json_object_get_array (struct json_object *this)
int 	json_object_array_length (struct json_object *this)
int 	json_object_array_add (struct json_object *this, struct json_object *val)
int 	json_object_array_put_idx (struct json_object *this, int idx, struct json_object *val)
json_object * 	json_object_array_get_idx (struct json_object *this, int idx)
json_object * 	json_object_new_boolean (boolean b)
boolean 	json_object_get_boolean (struct json_object *this)
json_object * 	json_object_new_int (int i)
int 	json_object_get_int (struct json_object *this)
json_object * 	json_object_new_double (double d)
double 	json_object_get_double (struct json_object *this)
json_object * 	json_object_new_string (char *s)
json_object * 	json_object_new_string_len (char *s, int len)
char * 	json_object_get_string (struct json_object *this)
*/


#define jo_get              json_object_get
#define jo_put              json_object_put
#define jo_is_type          json_object_is_type
#define jo_get_type         json_object_get_type
#define jo_to_json_string   json_object_to_json_string
#define jo_new_object       json_object_new_object
#define jo_get_object       json_object_get_object
#define joo_add             json_object_object_add
#define joo_get             json_object_object_get
#define joo_del             json_object_object_del
#define jo_new_array        json_object_new_array
#define jo_get_array        json_object_get_array
#define joa_length          json_object_array_length
#define joa_add             json_object_array_add
#define joa_put_idx         json_object_array_put_idx
#define joa_replace			json_object_array_put_idx
#define joa_insert_idx		json_object_array_insert_idx
#define joa_get_idx         json_object_array_get_idx
#define joa_del_idx         json_object_array_del
#define joa_remove			json_object_array_remove
#define joa_remove_if		json_object_array_remove_if
#define joa_clear			json_object_array_clear
#define joa_sort			json_object_array_sort

#define jo_new_boolean      json_object_new_boolean
#define jo_get_boolean      json_object_get_boolean
#define jo_new_int          json_object_new_int
#define jo_get_int          json_object_get_int
#define jo_new_double       json_object_new_double
#define jo_get_double       json_object_get_double
#define jo_new_string       json_object_new_string
#define jo_new_string_len   json_object_new_string_len
#define jo_get_string       json_object_get_string
#define jo_path				json_object_path


#define joo_get_boolean(obj,key)    json_object_get_boolean(json_object_object_get((obj),(key)))
#define joo_get_int(obj,key)    json_object_get_int(json_object_object_get((obj),(key)))
#define joo_get_double(obj,key)    json_object_get_double(json_object_object_get((obj),(key)))
#define joo_get_string(obj,key)    json_object_get_string(json_object_object_get((obj),(key)))


#define joo_set_boolean(obj,key,b)   json_object_object_add((obj),(key),json_object_new_boolean(b))
#define joo_set_int(obj,key,i)       json_object_object_add((obj),(key),json_object_new_int(i))
#define joo_set_double(obj,key,d)    json_object_object_add((obj),(key),json_object_new_double(d))
#define joo_set_string(obj,key,s)    json_object_object_add((obj),(key),json_object_new_string(s))

#define joo_int_add(obj1, obj2, key) joo_set_int((obj1), (key), joo_get_int((obj1), (key)) + joo_get_int((obj2), (key)))

#endif /*JSON_SHORT_H*/


