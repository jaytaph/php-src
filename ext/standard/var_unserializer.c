/* Generated by re2c 0.9.4 on Tue Mar  1 23:56:03 2005 */
#line 1 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
/*
   +----------------------------------------------------------------------+
   | PHP Version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Sascha Schumann <sascha@schumann.cx>                        |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php.h"
#include "ext/standard/php_var.h"
#include "php_incomplete_class.h"

/* {{{ reference-handling for unserializer: var_* */
#define VAR_ENTRIES_MAX 1024

typedef struct {
	zval *data[VAR_ENTRIES_MAX];
	int used_slots;
	void *next;
} var_entries;

static inline void var_push(php_unserialize_data_t *var_hashx, zval **rval)
{
	var_entries *var_hash = var_hashx->first, *prev = NULL;

	while (var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		prev = var_hash;
		var_hash = var_hash->next;
	}

	if (!var_hash) {
		var_hash = emalloc(sizeof(var_entries));
		var_hash->used_slots = 0;
		var_hash->next = 0;

		if (!var_hashx->first)
			var_hashx->first = var_hash;
		else
			prev->next = var_hash;
	}

	var_hash->data[var_hash->used_slots++] = *rval;
}

static inline void var_push_dtor(php_unserialize_data_t *var_hashx, zval **rval)
{
	var_entries *var_hash = var_hashx->first_dtor, *prev = NULL;

	while (var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		prev = var_hash;
		var_hash = var_hash->next;
	}

	if (!var_hash) {
		var_hash = emalloc(sizeof(var_entries));
		var_hash->used_slots = 0;
		var_hash->next = 0;

		if (!var_hashx->first_dtor)
			var_hashx->first_dtor = var_hash;
		else
			prev->next = var_hash;
	}

	(*rval)->refcount++;
	var_hash->data[var_hash->used_slots++] = *rval;
}

PHPAPI void var_replace(php_unserialize_data_t *var_hashx, zval *ozval, zval **nzval)
{
	int i;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			if (var_hash->data[i] == ozval) {
				var_hash->data[i] = *nzval;
				return;
			}
		}
		var_hash = var_hash->next;
	}
}

static int var_access(php_unserialize_data_t *var_hashx, int id, zval ***store)
{
	var_entries *var_hash = var_hashx->first;
	
	while (id >= VAR_ENTRIES_MAX && var_hash && var_hash->used_slots == VAR_ENTRIES_MAX) {
		var_hash = var_hash->next;
		id -= VAR_ENTRIES_MAX;
	}

	if (!var_hash) return !SUCCESS;

	if (id < 0 || id >= var_hash->used_slots) return !SUCCESS;

	*store = &var_hash->data[id];

	return SUCCESS;
}

PHPAPI void var_destroy(php_unserialize_data_t *var_hashx)
{
	void *next;
	int i;
	var_entries *var_hash = var_hashx->first;
	
	while (var_hash) {
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}
	
	var_hash = var_hashx->first_dtor;
	
	while (var_hash) {
		for (i = 0; i < var_hash->used_slots; i++) {
			zval_ptr_dtor(&var_hash->data[i]);
		}
		next = var_hash->next;
		efree(var_hash);
		var_hash = next;
	}
}

/* }}} */

#define YYFILL(n) do { } while (0)
#define YYCTYPE unsigned char
#define YYCURSOR cursor
#define YYLIMIT limit
#define YYMARKER marker


#line 154 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"




static inline long parse_iv2(const unsigned char *p, const unsigned char **q)
{
	char cursor;
	long result = 0;
	int neg = 0;

	switch (*p) {
		case '-':
			neg++;
			/* fall-through */
		case '+':
			p++;
	}
	
	while (1) {
		cursor = (char)*p;
		if (cursor >= '0' && cursor <= '9') {
			result = result * 10 + cursor - '0';
		} else {
			break;
		}
		p++;
	}
	if (q) *q = p;
	if (neg) return -result;
	return result;
}

static inline long parse_iv(const unsigned char *p)
{
	return parse_iv2(p, NULL);
}

/* no need to check for length - re2c already did */
static inline size_t parse_uiv(const unsigned char *p)
{
	unsigned char cursor;
	size_t result = 0;

	if (*p == '+') {
		p++;
	}
	
	while (1) {
		cursor = *p;
		if (cursor >= '0' && cursor <= '9') {
			result = result * 10 + (size_t)(cursor - (unsigned char)'0');
		} else {
			break;
		}
		p++;
	}
	return result;
}

#define UNSERIALIZE_PARAMETER zval **rval, const unsigned char **p, const unsigned char *max, php_unserialize_data_t *var_hash TSRMLS_DC
#define UNSERIALIZE_PASSTHRU rval, p, max, var_hash TSRMLS_CC

static inline int process_nested_data(UNSERIALIZE_PARAMETER, HashTable *ht, int elements)
{
	while (elements-- > 0) {
		zval *key, *data, **old_data;

		ALLOC_INIT_ZVAL(key);

		if (!php_var_unserialize(&key, p, max, NULL TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			return 0;
		}

		if (Z_TYPE_P(key) != IS_LONG && Z_TYPE_P(key) != IS_STRING) {
			zval_dtor(key);
			FREE_ZVAL(key);
			return 0;
		}

		ALLOC_INIT_ZVAL(data);

		if (!php_var_unserialize(&data, p, max, var_hash TSRMLS_CC)) {
			zval_dtor(key);
			FREE_ZVAL(key);
			zval_dtor(data);
			FREE_ZVAL(data);
			return 0;
		}

		switch (Z_TYPE_P(key)) {
			case IS_LONG:
				if (zend_hash_index_find(ht, Z_LVAL_P(key), (void **)&old_data)==SUCCESS) {
					var_push_dtor(var_hash, old_data);
				}
				zend_hash_index_update(ht, Z_LVAL_P(key), &data, sizeof(data), NULL);
				break;
			case IS_STRING:
				if (zend_hash_find(ht, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, (void **)&old_data)==SUCCESS) {
					var_push_dtor(var_hash, old_data);
				}
				zend_hash_update(ht, Z_STRVAL_P(key), Z_STRLEN_P(key) + 1, &data, sizeof(data), NULL);
				break;
		}
		
		zval_dtor(key);
		FREE_ZVAL(key);

		if (elements && *(*p-1) != ';' &&  *(*p-1) != '}') {
			(*p)--;
			return 0;
		}
	}

	return 1;
}

static inline int finish_nested_data(UNSERIALIZE_PARAMETER)
{
	if (*((*p)++) == '}') 
		return 1;

#if SOMETHING_NEW_MIGHT_LEAD_TO_CRASH_ENABLE_IF_YOU_ARE_BRAVE
	zval_ptr_dtor(rval);
#endif
	return 0;
}

static inline int object_common1(UNSERIALIZE_PARAMETER, zend_class_entry *ce)
{
	int elements;

	elements = parse_iv2((*p) + 2, p);

	(*p) += 2;
	
	object_init_ex(*rval, ce);
	return elements;
}

static inline int object_common2(UNSERIALIZE_PARAMETER, int elements)
{
	zval *retval_ptr = NULL;
	zval fname;

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_OBJPROP_PP(rval), elements)) {
		return 0;
	}

	INIT_PZVAL(&fname);
	ZVAL_STRINGL(&fname, "__wakeup", sizeof("__wakeup") - 1, 0);
	call_user_function_ex(CG(function_table), rval, &fname, &retval_ptr, 0, 0, 1, NULL TSRMLS_CC);

	if (retval_ptr)
		zval_ptr_dtor(&retval_ptr);

	return finish_nested_data(UNSERIALIZE_PASSTHRU);

}

static char *str_tolower_copy(char *dest, const char *source, unsigned int length)
{
	register unsigned char *str = (unsigned char*)source;
	register unsigned char *result = (unsigned char*)dest;
	register unsigned char *end = str + length;

	while (str < end) {
		*result++ = tolower((int)*str++);
	}
	*result = *end;

	return dest;
}

PHPAPI int php_var_unserialize(UNSERIALIZE_PARAMETER)
{
	const unsigned char *cursor, *limit, *marker, *start;
	zval **rval_ref;

	limit = cursor = *p;
	
	if (var_hash && cursor[0] != 'R') {
		var_push(var_hash, rval);
	}

	start = cursor;

	
	

#line 7 "<stdout>"
{
	YYCTYPE yych;
	unsigned int yyaccept;
	static unsigned char yybm[] = {
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	128, 128, 128, 128, 128, 128, 128, 128, 
	128, 128,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0, 
	};
	goto yy0;
yy1:	++YYCURSOR;
yy0:
	if((YYLIMIT - YYCURSOR) < 7) YYFILL(7);
	yych = *YYCURSOR;
	if(yych <= 'd'){
		if(yych <= 'R'){
			if(yych <= 'N'){
				if(yych <= 'M')	goto yy16;
				goto yy6;
			} else {
				if(yych <= 'O')	goto yy13;
				if(yych <= 'Q')	goto yy16;
				goto yy3;
			}
		} else {
			if(yych <= 'a'){
				if(yych <= '`')	goto yy16;
				goto yy11;
			} else {
				if(yych <= 'b')	goto yy7;
				if(yych <= 'c')	goto yy16;
				goto yy9;
			}
		}
	} else {
		if(yych <= 'q'){
			if(yych <= 'i'){
				if(yych <= 'h')	goto yy16;
				goto yy8;
			} else {
				if(yych == 'o')	goto yy12;
				goto yy16;
			}
		} else {
			if(yych <= '|'){
				if(yych <= 'r')	goto yy5;
				if(yych <= 's')	goto yy10;
				goto yy16;
			} else {
				if(yych <= '}')	goto yy14;
				if(yych <= 0xBF)	goto yy16;
				goto yy2;
			}
		}
	}
yy2:	YYCURSOR = YYMARKER;
	switch(yyaccept){
	case 0:	goto yy4;
	}
yy3:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy87;
	goto yy4;
yy4:
#line 586 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{ return 0; }
#line 102 "<stdout>"
yy5:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy81;
	goto yy4;
yy6:	yych = *++YYCURSOR;
	if(yych == ';')	goto yy79;
	goto yy4;
yy7:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy75;
	goto yy4;
yy8:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy69;
	goto yy4;
yy9:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy45;
	goto yy4;
yy10:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy38;
	goto yy4;
yy11:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy31;
	goto yy4;
yy12:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy24;
	goto yy4;
yy13:	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	if(yych == ':')	goto yy17;
	goto yy4;
yy14:	++YYCURSOR;
	goto yy15;
yy15:
#line 580 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	/* this is the case where we have less data than planned */
	php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Unexpected end of serialized data");
	return 0; /* not sure if it should be 0 or 1 here? */
}
#line 147 "<stdout>"
yy16:	yych = *++YYCURSOR;
	goto yy4;
yy17:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy19;
	if(yych != '+')	goto yy2;
	goto yy18;
yy18:	yych = *++YYCURSOR;
	if(yybm[0+yych] & 128)	goto yy19;
	goto yy2;
yy19:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy20;
yy20:	if(yybm[0+yych] & 128)	goto yy19;
	if(yych != ':')	goto yy2;
	goto yy21;
yy21:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy22;
yy22:	++YYCURSOR;
	goto yy23;
yy23:
#line 491 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	size_t len, len2, len3, maxlen;
	int elements;
	char *class_name;
	zend_class_entry *ce;
	int incomplete_class = 0;
	
	zval *user_func;
	zval *retval_ptr;
	zval **args[1];
	zval *arg_func_name;
	
	INIT_PZVAL(*rval);
	len2 = len = parse_uiv(start + 2);
	maxlen = max - YYCURSOR;
	if (maxlen < len || len == 0) {
		*p = start + 2;
		return 0;
	}

	class_name = (char*)YYCURSOR;

	YYCURSOR += len;

	if (*(YYCURSOR) != '"') {
		*p = YYCURSOR;
		return 0;
	}
	if (*(YYCURSOR+1) != ':') {
		*p = YYCURSOR+1;
		return 0;
	}
	
	class_name = str_tolower_copy((char *)emalloc(len+1), class_name, len);
	class_name[len] = '\0';
	
	len3 = strspn(class_name, "0123456789_abcdefghijklmnopqrstuvwxyz");
	if (len3 != len)
	{
		*p = YYCURSOR + len3 - len;
		efree(class_name);
		return 0;
	}

	if (zend_hash_find(CG(class_table), class_name, len + 1, (void **) &ce) != SUCCESS) {
		if ((PG(unserialize_callback_func) == NULL) || (PG(unserialize_callback_func)[0] == '\0')) {
			incomplete_class = 1;
			ce = PHP_IC_ENTRY;
		} else {
			MAKE_STD_ZVAL(user_func);
			ZVAL_STRING(user_func, PG(unserialize_callback_func), 1);

			args[0] = &arg_func_name;
			MAKE_STD_ZVAL(arg_func_name);
			ZVAL_STRING(arg_func_name, class_name, 1);
				
			if (call_user_function_ex(CG(function_table), NULL, user_func, &retval_ptr, 1, args, 0, NULL TSRMLS_CC) != SUCCESS) {
				zend_error(E_WARNING, "'unserialize_callback_func' defined (%s) but not found", user_func->value.str.val);
				incomplete_class = 1;
				ce = PHP_IC_ENTRY;
			} else {
				if (zend_hash_find(CG(class_table), class_name, len + 1, (void **) &ce) != SUCCESS) {
					zend_error(E_WARNING, "'unserialize_callback_func' (%s) hasn't defined the class it was called for", user_func->value.str.val);
					incomplete_class = 1;
					ce = PHP_IC_ENTRY;
				} else {
#ifdef ZEND_ENGINE_2
					ce = *(zend_class_entry **)ce; /* Bad hack, TBF! */
#endif	
				}
			}
		}
	} else {
#ifdef ZEND_ENGINE_2
		ce = *(zend_class_entry **)ce; /* Bad hack, TBF! */
#endif	
	}

	*p = YYCURSOR;
	elements = object_common1(UNSERIALIZE_PASSTHRU, ce);

	if (incomplete_class) {
		php_store_class_name(*rval, class_name, len2 TSRMLS_CC);
	}
	efree(class_name);

	return object_common2(UNSERIALIZE_PASSTHRU, elements);
}
#line 260 "<stdout>"
yy24:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy25;
	} else {
		if(yych <= '-')	goto yy25;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy26;
		goto yy2;
	}
yy25:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy26;
yy26:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy27;
yy27:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy26;
	if(yych >= ';')	goto yy2;
	goto yy28;
yy28:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy29;
yy29:	++YYCURSOR;
	goto yy30;
yy30:
#line 483 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{

	INIT_PZVAL(*rval);
	
	return object_common2(UNSERIALIZE_PASSTHRU,
			object_common1(UNSERIALIZE_PASSTHRU, ZEND_STANDARD_CLASS_DEF_PTR));
}
#line 298 "<stdout>"
yy31:	yych = *++YYCURSOR;
	if(yych == '+')	goto yy32;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy33;
	goto yy2;
yy32:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy33;
yy33:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy34;
yy34:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy33;
	if(yych >= ';')	goto yy2;
	goto yy35;
yy35:	yych = *++YYCURSOR;
	if(yych != '{')	goto yy2;
	goto yy36;
yy36:	++YYCURSOR;
	goto yy37;
yy37:
#line 461 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	int elements = parse_iv(start + 2);

	*p = YYCURSOR;

	if (elements < 0) {
		return 0;
	}

	INIT_PZVAL(*rval);
	Z_TYPE_PP(rval) = IS_ARRAY;
	ALLOC_HASHTABLE(Z_ARRVAL_PP(rval));

	zend_hash_init(Z_ARRVAL_PP(rval), elements + 1, NULL, ZVAL_PTR_DTOR, 0);

	if (!process_nested_data(UNSERIALIZE_PASSTHRU, Z_ARRVAL_PP(rval), elements)) {
		return 0;
	}

	return finish_nested_data(UNSERIALIZE_PASSTHRU);
}
#line 345 "<stdout>"
yy38:	yych = *++YYCURSOR;
	if(yych == '+')	goto yy39;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy40;
	goto yy2;
yy39:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy40;
yy40:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy41;
yy41:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy40;
	if(yych >= ';')	goto yy2;
	goto yy42;
yy42:	yych = *++YYCURSOR;
	if(yych != '"')	goto yy2;
	goto yy43;
yy43:	++YYCURSOR;
	goto yy44;
yy44:
#line 433 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	size_t len, maxlen;
	char *str;

	len = parse_uiv(start + 2);
	maxlen = max - YYCURSOR;
	if (maxlen < len) {
		*p = start + 2;
		return 0;
	}

	str = (char*)YYCURSOR;

	YYCURSOR += len;

	if (*(YYCURSOR) != '"') {
		*p = YYCURSOR;
		return 0;
	}

	YYCURSOR += 2;
	*p = YYCURSOR;

	INIT_PZVAL(*rval);
	ZVAL_STRINGL(*rval, str, len, 1);
	return 1;
}
#line 398 "<stdout>"
yy45:	yych = *++YYCURSOR;
	if(yych <= '/'){
		if(yych <= ','){
			if(yych == '+')	goto yy49;
			goto yy2;
		} else {
			if(yych <= '-')	goto yy47;
			if(yych <= '.')	goto yy52;
			goto yy2;
		}
	} else {
		if(yych <= 'I'){
			if(yych <= '9')	goto yy50;
			if(yych <= 'H')	goto yy2;
			goto yy48;
		} else {
			if(yych != 'N')	goto yy2;
			goto yy46;
		}
	}
yy46:	yych = *++YYCURSOR;
	if(yych == 'A')	goto yy68;
	goto yy2;
yy47:	yych = *++YYCURSOR;
	if(yych <= '/'){
		if(yych == '.')	goto yy52;
		goto yy2;
	} else {
		if(yych <= '9')	goto yy50;
		if(yych != 'I')	goto yy2;
		goto yy48;
	}
yy48:	yych = *++YYCURSOR;
	if(yych == 'N')	goto yy64;
	goto yy2;
yy49:	yych = *++YYCURSOR;
	if(yych == '.')	goto yy52;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy50;
yy50:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy51;
yy51:	if(yych <= ':'){
		if(yych <= '.'){
			if(yych <= '-')	goto yy2;
			goto yy62;
		} else {
			if(yych <= '/')	goto yy2;
			if(yych <= '9')	goto yy50;
			goto yy2;
		}
	} else {
		if(yych <= 'E'){
			if(yych <= ';')	goto yy55;
			if(yych <= 'D')	goto yy2;
			goto yy57;
		} else {
			if(yych == 'e')	goto yy57;
			goto yy2;
		}
	}
yy52:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy53;
yy53:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy54;
yy54:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy53;
		if(yych <= ':')	goto yy2;
		goto yy55;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy57;
		} else {
			if(yych == 'e')	goto yy57;
			goto yy2;
		}
	}
yy55:	++YYCURSOR;
	goto yy56;
yy56:
#line 426 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_DOUBLE(*rval, zend_strtod(start + 2, NULL));
	return 1;
}
#line 496 "<stdout>"
yy57:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy58;
	} else {
		if(yych <= '-')	goto yy58;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy59;
		goto yy2;
	}
yy58:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych == '+')	goto yy61;
		goto yy2;
	} else {
		if(yych <= '-')	goto yy61;
		if(yych <= '/')	goto yy2;
		if(yych >= ':')	goto yy2;
		goto yy59;
	}
yy59:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy60;
yy60:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy59;
	if(yych == ';')	goto yy55;
	goto yy2;
yy61:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy59;
	goto yy2;
yy62:	++YYCURSOR;
	if((YYLIMIT - YYCURSOR) < 4) YYFILL(4);
	yych = *YYCURSOR;
	goto yy63;
yy63:	if(yych <= ';'){
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy62;
		if(yych <= ':')	goto yy2;
		goto yy55;
	} else {
		if(yych <= 'E'){
			if(yych <= 'D')	goto yy2;
			goto yy57;
		} else {
			if(yych == 'e')	goto yy57;
			goto yy2;
		}
	}
yy64:	yych = *++YYCURSOR;
	if(yych != 'F')	goto yy2;
	goto yy65;
yy65:	yych = *++YYCURSOR;
	if(yych != ';')	goto yy2;
	goto yy66;
yy66:	++YYCURSOR;
	goto yy67;
yy67:
#line 411 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);

	if (!strncmp(start + 2, "NAN", 3)) {
		ZVAL_DOUBLE(*rval, php_get_nan());
	} else if (!strncmp(start + 2, "INF", 3)) {
		ZVAL_DOUBLE(*rval, php_get_inf());
	} else if (!strncmp(start + 2, "-INF", 4)) {
		ZVAL_DOUBLE(*rval, -php_get_inf());
	}

	return 1;
}
#line 573 "<stdout>"
yy68:	yych = *++YYCURSOR;
	if(yych == 'N')	goto yy65;
	goto yy2;
yy69:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy70;
	} else {
		if(yych <= '-')	goto yy70;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy71;
		goto yy2;
	}
yy70:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy71;
yy71:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy72;
yy72:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy71;
	if(yych != ';')	goto yy2;
	goto yy73;
yy73:	++YYCURSOR;
	goto yy74;
yy74:
#line 404 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_LONG(*rval, parse_iv(start + 2));
	return 1;
}
#line 610 "<stdout>"
yy75:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= '2')	goto yy2;
	goto yy76;
yy76:	yych = *++YYCURSOR;
	if(yych != ';')	goto yy2;
	goto yy77;
yy77:	++YYCURSOR;
	goto yy78;
yy78:
#line 397 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_BOOL(*rval, parse_iv(start + 2));
	return 1;
}
#line 628 "<stdout>"
yy79:	++YYCURSOR;
	goto yy80;
yy80:
#line 390 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	*p = YYCURSOR;
	INIT_PZVAL(*rval);
	ZVAL_NULL(*rval);
	return 1;
}
#line 639 "<stdout>"
yy81:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy82;
	} else {
		if(yych <= '-')	goto yy82;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy83;
		goto yy2;
	}
yy82:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy83;
yy83:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy84;
yy84:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy83;
	if(yych != ';')	goto yy2;
	goto yy85;
yy85:	++YYCURSOR;
	goto yy86;
yy86:
#line 367 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	int id;

 	*p = YYCURSOR;
	if (!var_hash) return 0;

	id = parse_iv(start + 2) - 1;
	if (id == -1 || var_access(var_hash, id, &rval_ref) != SUCCESS) {
		return 0;
	}
	
	if (*rval == *rval_ref) return 0;

	if (*rval != NULL) {
	zval_ptr_dtor(rval);
	}
	*rval = *rval_ref;
	(*rval)->refcount++;
	(*rval)->is_ref = 1;
	
	return 1;
}
#line 689 "<stdout>"
yy87:	yych = *++YYCURSOR;
	if(yych <= ','){
		if(yych != '+')	goto yy2;
		goto yy88;
	} else {
		if(yych <= '-')	goto yy88;
		if(yych <= '/')	goto yy2;
		if(yych <= '9')	goto yy89;
		goto yy2;
	}
yy88:	yych = *++YYCURSOR;
	if(yych <= '/')	goto yy2;
	if(yych >= ':')	goto yy2;
	goto yy89;
yy89:	++YYCURSOR;
	if(YYLIMIT == YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	goto yy90;
yy90:	if(yych <= '/')	goto yy2;
	if(yych <= '9')	goto yy89;
	if(yych != ';')	goto yy2;
	goto yy91;
yy91:	++YYCURSOR;
	goto yy92;
yy92:
#line 346 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"
{
	int id;

 	*p = YYCURSOR;
	if (!var_hash) return 0;

	id = parse_iv(start + 2) - 1;
	if (id == -1 || var_access(var_hash, id, &rval_ref) != SUCCESS) {
		return 0;
	}

	if (*rval != NULL) {
	zval_ptr_dtor(rval);
	}
	*rval = *rval_ref;
	(*rval)->refcount++;
	(*rval)->is_ref = 1;
	
	return 1;
}
#line 737 "<stdout>"
}
#line 588 "/usr/src/php/php_4_3/ext/standard/var_unserializer.re"


	return 0;
}
