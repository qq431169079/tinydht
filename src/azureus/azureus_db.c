/***************************************************************************
 *   Copyright (C) 2007 by Saritha Kalyanam   				   *
 *   kalyanamsaritha@gmail.com                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "azureus_db.h"
#include "debug.h"
#include "crypto.h"

struct azureus_db_val *
azureus_db_val_new(u8 *val, int val_len)
{
    struct azureus_db_val *v = NULL;

    ASSERT((val_len > 0) && val);

    v = (struct azureus_db_val *) malloc(sizeof(struct azureus_db_val));
    if (!v) {
        return NULL;
    }

    bzero(v, sizeof(struct azureus_db_val));
    v->len = val_len;
    memcpy(v->data, val, val_len);

    return v;
}

void
azureus_db_val_delete(struct azureus_db_val *v)
{
    free(v);
}

struct azureus_db_valset *
azureus_db_valset_new(struct val_list_head *head, int n_vals)
{
    struct azureus_db_valset *vs = NULL;
    struct azureus_db_val *v = NULL, *pv = NULL;
    int ret;

    ASSERT(head && n_vals);

    vs = (struct azureus_db_valset *) malloc(sizeof(struct azureus_db_valset));
    if (!vs) {
        return NULL;
    }

    bzero(vs, sizeof(struct azureus_db_valset));
    vs->n_vals = n_vals;

    TAILQ_FOREACH(v, head, next) {
        ret = azureus_db_valset_add_val(vs, v->data, v->len);
        if (ret != SUCCESS) {
            return ret;
        }
    }

    return vs;
}

void
azureus_db_valset_delete(struct azureus_db_valset *vs)
{
    struct azureus_db_val *v = NULL;

    ASSERT(vs);

    while (vs->val_list.tqh_first != NULL) {
        v = TAILQ_FIRST(&vs->val_list);
        TAILQ_REMOVE(&vs->val_list, 
                vs->val_list.tqh_first, next);
        azureus_db_val_delete(v);
    }

    free(vs);
}

int
azureus_db_valset_add_val(struct azureus_db_valset *vs, u8 *val, int val_len)
{
    struct azureus_db_val *v = NULL;

    ASSERT(vs && (val_len > 0) && val);

    v = azureus_db_val_new(val, val_len);
    if (!v) {
        return FAILURE;
    }

    vs->n_vals++;

    TAILQ_INSERT_TAIL(&vs->val_list, v, next);

    return SUCCESS;
}

struct azureus_db_key *
azureus_db_key_new(u8 *data, int len)
{
    struct azureus_db_key *k = NULL;

    ASSERT(data && len);

    k = (struct azureus_db_key *) malloc(sizeof(struct azureus_db_key));
    if (!k) {
        return NULL;
    }

    bzero(k, sizeof(struct azureus_db_key));
    k->len = k;
    memcpy(k->data, data, len);

    return k;
}

void
azureus_db_key_delete(struct azureus_db_key *key)
{
    free(key);
}

struct azureus_db_item *
azureus_db_item_new(struct azureus_dht *dht)
{
    struct azureus_db_item *item = NULL;

    item = (struct azureus_db_item *) malloc(sizeof(struct azureus_db_item));
    if (!item) {
        return NULL;
    }

    bzero(item, sizeof(struct azureus_db_item));
    item->dht = dht;
    item->cr_time = dht_get_current_time();

    TAILQ_INIT(&item->valset.val_list);
        
    return item;
}

void
azureus_db_item_delete(struct azureus_db_item *item)
{
    struct azureus_db_val *v = NULL;

    ASSERT(item);

    while (item->valset.val_list.tqh_first != NULL) {
        v = TAILQ_FIRST(&item->valset.val_list);
        TAILQ_REMOVE(&item->valset.val_list, 
                item->valset.val_list.tqh_first, next);
        azureus_db_val_delete(v);
    }

    TAILQ_REMOVE(&item->dht->db_list, item, next);
    free(item);

    return;
}

int
azureus_db_item_set_key(struct azureus_db_item *item, u8 *key, int key_len)
{
    u8 digest[20];
    int ret;

    ASSERT(item && key && (key_len > 0));

    bzero(&item->key, sizeof(struct azureus_db_key));

    ret = crypto_get_sha1_digest(key, key_len, digest);
    if (ret != SUCCESS) {
        return ret;
    }

    memcpy(item->key.data, digest, 20);
    item->key.len = 20;

    return SUCCESS;
}

int
azureus_db_item_add_val(struct azureus_db_item *item, u8 *val, int val_len)
{
    int ret;

    ASSERT(item && (val_len > 0) && val);

    ret = azureus_db_valset_add_val(&item->valset, val, val_len);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

bool
azureus_db_item_match_key(struct azureus_db_item *item, u8 *key, int key_len)
{
    u8 digest[20];
    int ret;

    ASSERT(item && key && (key_len > 0));

    ret = crypto_get_sha1_digest(key, key_len, digest);
    if (ret != SUCCESS) {
        return FALSE;
    }

    ASSERT(item->key.len == 20);

    if (memcmp(item->key.data, digest, 20) == 0) {
        return TRUE;
    }

    return FALSE;
}
