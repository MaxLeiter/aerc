#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "email/headers.h"
#include "config.h"
#include "state.h"
#include "ui.h"
#include "util/stringop.h"
#include "util/list.h"
#include "worker.h"

void set_status(struct account_state *account, enum account_status state,
		const char *fmt, ...) {
	free(account->status.text);
	if (fmt == NULL) {
		fmt = "Unknown error occured";
	}
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	char *buf = malloc(len + 1);
	va_start(args, fmt);
	vsnprintf(buf, len + 1, fmt, args);
	va_end(args);

	account->status.text = buf;
	account->status.status = state;

	//http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x#6725161
	#ifdef __MACH__
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	account->status.since.tv_sec = mts.tv_sec;
	account->status.since.tv_nsec = mts.tv_nsec;
	
	#else
	clock_gettime(CLOCK_MONOTONIC, &account->status.since);
	#endif
	request_rerender(); // TODO: just rerender the status bar
}

static int get_mbox_compare(const void *_mbox, const void *_name) {
	const struct aerc_mailbox *mbox = _mbox;
	const char *name = _name;
	return strcmp(mbox->name, name);
}

struct aerc_mailbox *get_aerc_mailbox(struct account_state *account,
		const char *name) {
	if (!account->mailboxes || !name) {
		return NULL;
	}
	int i = list_seq_find(account->mailboxes, get_mbox_compare, name);
	if (i == -1) {
		return NULL;
	}
	return account->mailboxes->items[i];
}

void free_aerc_mailbox(struct aerc_mailbox *mbox, bool messages) {
	if (!mbox) return;
	free(mbox->name);
	free_flat_list(mbox->flags);
	if (messages) {
		// TODO: Free messages
	}
	free(mbox);
}

void free_aerc_message(struct aerc_message *msg) {
	if (!msg) return;
	free_flat_list(msg->flags);
	if (msg->headers) {
		for (size_t i = 0; i < msg->headers->length; ++i) {
			struct email_header *header = msg->headers->items[i];
			free(header->key);
			free(header->value);
		}
	}
	free_flat_list(msg->headers);
	free(msg);
}

const char *get_message_header(struct aerc_message *msg, char *key) {
	for (size_t i = 0; i < msg->headers->length; ++i) {
		struct email_header *header = msg->headers->items[i];
		if (strcmp(header->key, key) == 0) {
			return header->value;
		}
	}
	return NULL;
}

bool get_mailbox_flag(struct aerc_mailbox *mbox, char *flag) {
	if (!mbox->flags) return false;
	for (size_t i = 0; i < mbox->flags->length; ++i) {
		const char *_flag = mbox->flags->items[i];
		if (strcmp(flag, _flag) == 0) {
			return true;
		}
	}
	return false;
}

bool get_message_flag(struct aerc_message *msg, char *flag) {
	if (!msg->flags) return false;
	for (size_t i = 0; i < msg->flags->length; ++i) {
		const char *_flag = msg->flags->items[i];
		if (strcmp(flag, _flag) == 0) {
			return true;
		}
	}
	return false;
}

struct account_config *config_for_account(const char *name) {
	for (size_t i = 0; i < config->accounts->length; ++i) {
		struct account_config *c = config->accounts->items[i];
		if (strcmp(c->name, name) == 0) {
			return c;
		}
	}
	return NULL;
}
