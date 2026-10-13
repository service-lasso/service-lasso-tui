
/* Author : Stephen Smalley, <stephen.smalley.work@gmail.com> */

/*
 * Updated : Karl MacMillan <kmacmillan@mentalrootkit.com>
 *
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* FLASK */

/*
 * Implementation of the hash table type.
 */

#include <stdlib.h>
#include <string.h>
#include <sepol/policydb/hashtab.h>

#include "private.h"

hashtab_t hashtab_create(unsigned int (*hash_value)(hashtab_t h,
						    const_hashtab_key_t key),
			 int (*keycmp)(hashtab_t h, const_hashtab_key_t key1,
				       const_hashtab_key_t key2),
			 unsigned int size)
{
	hashtab_t p;
#ifdef SERVICE_LASSO_OWNED_SEPOL
 sl_sepol_memory_owner *original=sl_sepol_source_current_owner();if(!original)return NULL;
#endif

	p = (hashtab_t)malloc(sizeof(hashtab_val_t));
	if (p == NULL)
		return p;

	*p = (hashtab_val_t){
#ifdef SERVICE_LASSO_OWNED_SEPOL
 .sl_source_owner=original,
#endif
		.size = size,
		.nel = 0,
		.hash_value = hash_value,
		.keycmp = keycmp,
		.htable = (hashtab_ptr_t *)calloc(size, sizeof(hashtab_ptr_t)),
	};

	if (p->htable == NULL) {
		free(p);
		return NULL;
	}

	return p;
}

#ifndef SERVICE_LASSO_OWNED_SEPOL
static void hashtab_check_resize(hashtab_t h)
{
	unsigned int hvalue, i, old_size, new_size = h->size;
	hashtab_ptr_t *new_htable, *dst, cur, next;

	while (new_size <= h->nel && new_size * 2 != 0)
		new_size *= 2;

	if (h->size == new_size)
		return;

	new_htable = calloc(new_size, sizeof(*new_htable));
	if (!new_htable)
		return;

	old_size = h->size;
	h->size = new_size;

	/* Move all elements to the new htable */
	for (i = 0; i < old_size; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			hvalue = h->hash_value(h, cur->key);
			dst = &new_htable[hvalue];
			while (*dst && h->keycmp(h, cur->key, (*dst)->key) > 0)
				dst = &(*dst)->next;

			next = cur->next;

			cur->next = *dst;
			*dst = cur;

			cur = next;
		}
	}
	free(h->htable);
	h->htable = new_htable;
}

#else
static int hashtab_check_resize(hashtab_t h){
 unsigned int hvalue,i,old_size,new_size=h->size;hashtab_ptr_t *new_htable,*dst,cur,next;
 if(sl_sepol_source_cleanup_failed()||sl_sepol_source_access_original(h->sl_source_owner,1)||!h->size)return SEPOL_ENOMEM;
 while(new_size<=h->nel&&new_size*2!=0)new_size*=2;
 if(h->size==new_size)return SEPOL_OK;
 new_htable=calloc(new_size,sizeof(*new_htable));if(!new_htable)return SEPOL_ENOMEM;
 old_size=h->size;
 sl_sepol_memory_attempt *size_change=sl_sepol_source_scalar_before(&h->size,h->size,new_size,sizeof h->size,__FILE__,__LINE__);
 if(!size_change)return SEPOL_ENOMEM;h->size=new_size;size_change->returned=1;
 for(i=0;i<old_size;i++){
  cur=h->htable[i];
  while(cur){
   hvalue=h->hash_value(h,cur->key);if(sl_sepol_source_cleanup_failed()||hvalue>=new_size)return SEPOL_ENOMEM;dst=&new_htable[hvalue];
   while(*dst){int comparison=h->keycmp(h,cur->key,(*dst)->key);if(sl_sepol_source_cleanup_failed())return SEPOL_ENOMEM;if(comparison<=0)break;dst=&(*dst)->next;}
   next=cur->next;
   sl_sepol_memory_attempt *next_change=sl_sepol_source_pointer_before(&cur->next,cur->next,*dst,sizeof cur->next,__FILE__,__LINE__);
   if(!next_change)return SEPOL_ENOMEM;cur->next=*dst;next_change->returned=1;
   sl_sepol_memory_attempt *bucket_change=sl_sepol_source_pointer_before(dst,*dst,cur,sizeof *dst,__FILE__,__LINE__);
   if(!bucket_change)return SEPOL_ENOMEM;*dst=cur;bucket_change->returned=1;cur=next;
  }
 }
 free(h->htable);if(sl_sepol_source_cleanup_failed())return SEPOL_ENOMEM;
 sl_sepol_memory_attempt *table_change=sl_sepol_source_pointer_before(&h->htable,h->htable,new_htable,sizeof h->htable,__FILE__,__LINE__);
 if(!table_change)return SEPOL_ENOMEM;h->htable=new_htable;table_change->returned=1;return SEPOL_OK;
}
#endif

int hashtab_insert(hashtab_t h, hashtab_key_t key, hashtab_datum_t datum)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif

	unsigned int hvalue;
	hashtab_ptr_t prev, cur, newnode;

	if (!h || h->nel == UINT32_MAX)
		return SEPOL_ENOMEM;

#ifdef SERVICE_LASSO_OWNED_SEPOL
	if(hashtab_check_resize(h)!=SEPOL_OK)return SEPOL_ENOMEM;
#else
	hashtab_check_resize(h);
#endif

	hvalue = h->hash_value(h, key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif

	for (prev = NULL, cur = h->htable[hvalue]; cur;
	     prev = cur, cur = cur->next) {
		int cmp;

		cmp = h->keycmp(h, key, cur->key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif
		if (cmp > 0)
			continue;
		if (cmp == 0)
			return SEPOL_EEXIST;
		break;
	}

	newnode = (hashtab_ptr_t)malloc(sizeof(hashtab_node_t));
	if (newnode == NULL)
		return SEPOL_ENOMEM;
	*newnode = (hashtab_node_t){
		.key = key,
		.datum = datum,
	};
#ifdef SERVICE_LASSO_OWNED_SEPOL
 hashtab_ptr_t *destination=prev?&prev->next:&h->htable[hvalue];
 sl_sepol_memory_attempt *next_change=sl_sepol_source_pointer_before(&newnode->next,newnode->next,*destination,sizeof newnode->next,__FILE__,__LINE__);
 if(!next_change)return SEPOL_ENOMEM;newnode->next=*destination;next_change->returned=1;
 sl_sepol_memory_attempt *destination_change=sl_sepol_source_pointer_before(destination,*destination,newnode,sizeof *destination,__FILE__,__LINE__);
 if(!destination_change)return SEPOL_ENOMEM;*destination=newnode;destination_change->returned=1;
 sl_sepol_memory_attempt *count_change=sl_sepol_source_scalar_before(&h->nel,h->nel,h->nel+1,sizeof h->nel,__FILE__,__LINE__);
 if(!count_change)return SEPOL_ENOMEM;h->nel++;count_change->returned=1;
#else
	if (prev) {
		newnode->next = prev->next;
		prev->next = newnode;
	} else {
		newnode->next = h->htable[hvalue];
		h->htable[hvalue] = newnode;
	}

	h->nel++;
#endif
	return SEPOL_OK;
}

int hashtab_remove(hashtab_t h, hashtab_key_t key,
		   void (*destroy)(hashtab_key_t k, hashtab_datum_t d,
				   void *args),
		   void *args)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif

	unsigned int hvalue;
	hashtab_ptr_t cur, last;

	if (!h)
		return SEPOL_ENOENT;

	hvalue = h->hash_value(h, key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif

	for (last = NULL, cur = h->htable[hvalue]; cur;
	     last = cur, cur = cur->next) {
		int cmp;

		cmp = h->keycmp(h, key, cur->key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif
		if (cmp > 0)
			continue;
		if (cmp == 0)
			break;
		return SEPOL_ENOENT;
	}

	if (cur == NULL)
		return SEPOL_ENOENT;

#ifdef SERVICE_LASSO_OWNED_SEPOL
 hashtab_ptr_t *destination=last?&last->next:&h->htable[hvalue];
 sl_sepol_memory_attempt *detach=sl_sepol_source_pointer_before(destination,*destination,cur->next,sizeof *destination,__FILE__,__LINE__);
 if(!detach)return SEPOL_ENOMEM;*destination=cur->next;detach->returned=1;
#else
	if (last == NULL)
		h->htable[hvalue] = cur->next;
	else
		last->next = cur->next;
#endif

	if (destroy)
		destroy(cur->key, cur->datum, args);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif
	free(cur);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||sl_sepol_source_access_original(h->sl_source_owner,1))return SEPOL_ENOMEM;
 sl_sepol_memory_attempt *count_change=sl_sepol_source_scalar_before(&h->nel,h->nel,h->nel-1,sizeof h->nel,__FILE__,__LINE__);
 if(!count_change)return SEPOL_ENOMEM;h->nel--;count_change->returned=1;
#else
	h->nel--;
#endif
	return SEPOL_OK;
}

hashtab_datum_t hashtab_search(hashtab_t h, const_hashtab_key_t key)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,0)))return NULL;
#endif

	unsigned int hvalue;
	hashtab_ptr_t cur;

	if (!h)
		return NULL;

	hvalue = h->hash_value(h, key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,0)))return NULL;
#endif
	for (cur = h->htable[hvalue]; cur; cur = cur->next) {
		int cmp;

		cmp = h->keycmp(h, key, cur->key);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,0)))return NULL;
#endif
		if (cmp > 0)
			continue;
		if (cmp == 0)
			return cur->datum;
		break;
	}

	return NULL;
}

void hashtab_destroy(hashtab_t h)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return;
#endif

	unsigned int i;
	hashtab_ptr_t cur, temp;

	if (!h)
		return;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			temp = cur;
			cur = cur->next;
			free(temp);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return;
#endif
		}
#ifdef SERVICE_LASSO_OWNED_SEPOL
		SL_SEPOL_CLEANUP_POINTER(h->htable[i],NULL);
#else
		h->htable[i] = NULL;
#endif
	}

	free(h->htable);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||sl_sepol_source_access_original(h->sl_source_owner,1))return;
#endif
	free(h);
}

int hashtab_map(hashtab_t h,
		int (*apply)(hashtab_key_t k, hashtab_datum_t d, void *args),
		void *args)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif

	unsigned int i;
	hashtab_ptr_t cur;
	int ret;

	if (!h)
		return SEPOL_OK;

	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		while (cur != NULL) {
			ret = apply(cur->key, cur->datum, args);
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,1)))return SEPOL_ENOMEM;
#endif
			if (ret)
				return ret;
			cur = cur->next;
		}
	}
	return SEPOL_OK;
}

void hashtab_hash_eval(hashtab_t h, const char *tag)
{
#ifdef SERVICE_LASSO_OWNED_SEPOL
 if(sl_sepol_source_cleanup_failed()||(h&&sl_sepol_source_access_original(h->sl_source_owner,0)))return;
#endif

	unsigned int i;
	size_t chain_len, slots_used, max_chain_len, chain2_len_sum;
	hashtab_ptr_t cur;

	slots_used = 0;
	max_chain_len = 0;
	chain2_len_sum = 0;
	for (i = 0; i < h->size; i++) {
		cur = h->htable[i];
		if (cur) {
			slots_used++;
			chain_len = 0;
			while (cur) {
				chain_len++;
				cur = cur->next;
			}

			if (chain_len > max_chain_len)
				max_chain_len = chain_len;
			chain2_len_sum += chain_len * chain_len;
		}
	}

	printf("%s:  %d entries and %zu/%d buckets used, longest chain length %zu, chain length^2 %zu, normalized chain length^2 %.2f\n",
	       tag, h->nel, slots_used, h->size, max_chain_len, chain2_len_sum,
	       chain2_len_sum ? (float)chain2_len_sum / slots_used : 0);
}
