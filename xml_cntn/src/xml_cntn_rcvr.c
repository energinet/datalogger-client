/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2012 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "xml_cntn_rcvr.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>


struct xml_cntn_rcvr *xml_cntn_rcvr_create()
{
	struct xml_cntn_rcvr *new = malloc(sizeof(*new));
	new->seq = 0;
	new->subcllst = NULL;

	assert(!pthread_cond_init(&new->cond, NULL));
	assert(!pthread_mutex_init(&new->mutex, NULL));

	return new;
}

void xml_cntn_rcvr_delete(struct xml_cntn_rcvr *rcvr)
{
	pthread_cond_destroy(&rcvr->cond);
	pthread_mutex_destroy(&rcvr->mutex);
	free(rcvr);

}

int xml_cntn_rcvr_subcl_activate(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl)
{

	if (pthread_mutex_lock(&rcvr->mutex) != 0){
		fprintf(stderr, "ERROR: No mutex lock in %s\n", __FUNCTION__);
		return -1;
	}

	xml_cntn_subcl_activate(subcl, ++rcvr->seq);
	
	rcvr->subcllst = xml_cntn_subcl_list_add(rcvr->subcllst, subcl);

	pthread_mutex_unlock(&rcvr->mutex);

	return 0;
}

int xml_cntn_rcvr_subcl_cancel(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl)
{
	if (pthread_mutex_lock(&rcvr->mutex) != 0)
		return -1;

	rcvr->subcllst = xml_cntn_subcl_list_rem(rcvr->subcllst, subcl);

	xml_cntn_subcl_cancel(subcl);

	pthread_mutex_unlock(&rcvr->mutex);

	return 0;
}

struct xml_item *xml_cntn_rcvr_subcl_wait(struct xml_cntn_rcvr *rcvr, struct xml_cntn_subcl* subcl, int timeout)
{
	struct xml_item *items = NULL;
	struct timespec ts_timeout;
	int retval = 0;

	clock_gettime(CLOCK_REALTIME, &ts_timeout);

	ts_timeout.tv_sec += timeout/1000;
	ts_timeout.tv_nsec += (timeout%1000)*1000000; 
	if(ts_timeout.tv_nsec >=  1000000000){ 
		ts_timeout.tv_nsec -= 1000000000; 
		ts_timeout.tv_sec += 1;
	}

	if (pthread_mutex_lock(&rcvr->mutex) != 0)
		return NULL;

	while (subcl->item == NULL) {
		if(timeout > 0)
			retval = pthread_cond_timedwait(&rcvr->cond, &rcvr->mutex, &ts_timeout);
		else 
			retval = pthread_cond_wait(&rcvr->cond, &rcvr->mutex);

		if (retval) {
			PRINT_ERR("pthread_cond_timed[wait] failed (seqtag %s, timeout %d): return: %s (%d)", subcl->seqtag, timeout, strerror(retval), retval );
			PRINT_ERR("pthread_cond_timed[wait]: %p %p", &rcvr->cond, &rcvr->mutex);
			break;
		}
	}

	items = subcl->item;
	subcl->item = NULL;

	pthread_mutex_unlock(&rcvr->mutex);

	return items;
}


int xml_cntn_rcvr_tag(struct spxml_cntn *cntn, struct xml_item *item)
{
	struct xml_cntn_rcvr *rcvr =  (struct xml_cntn_rcvr*)cntn->userdata;
	const char *seq = xml_item_get_attr(item, "seq");
	const char *tag = item->name;
	
	int found = 0;

	if (pthread_mutex_lock(&rcvr->mutex) != 0)
		return -1;

	struct xml_cntn_subcl* ptr = rcvr->subcllst;

	while (ptr) {
		if(xml_cntn_subcl_match(ptr, seq, tag)){
			ptr->item = xml_item_list_add(ptr->item, xml_item_create_copy(NULL, item, 0));
			ptr->rcvcnt++;
			found = 1;
			break;
		}
		ptr = ptr->next;
	}
	
	pthread_cond_broadcast(&rcvr->cond);
	pthread_mutex_unlock(&rcvr->mutex);

	if (!found) {
		PRINT_ERR("seq not found %s/%s", seq, tag);
	}

	return found;
	
}


