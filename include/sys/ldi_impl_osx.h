/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 1994, 2010, Oracle and/or its affiliates. All rights reserved.
 */
/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright (c) 2013, Joyent, Inc.  All rights reserved.
 */
/*
 * Copyright (c) 2015, Evan Susarret.  All rights reserved.
 */
/*
 * Portions of this document are copyright Oracle and Joyent.
 * OS X implementation of ldi_ named functions for ZFS written by
 * Evan Susarret in 2015.
 */

#ifndef _SYS_LDI_IMPL_OSX_H
#define	_SYS_LDI_IMPL_OSX_H

#include <sys/ldi_osx.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * OS X
 */
#define	LDI_TYPE_INVALID	0x0	/* uninitialized */
#define	LDI_TYPE_IOKIT		0x1	/* IOMedia device */
#define	LDI_TYPE_VNODE		0x2	/* vnode (bdev) device */

/*
 * OS X
 */
#define	LDI_STATUS_OFFLINE	0x0	/* device offline (dead-end) */
#define	LDI_STATUS_CLOSED	0x1	/* just initialized or closed */
#define	LDI_STATUS_CLOSING	0x2	/* close in-progress */
#define	LDI_STATUS_OPENING	0x3	/* open in-progress */
#define	LDI_STATUS_ONLINE	0x4	/* device is open and active */
typedef uint_t ldi_status_t;

/*
 * LDI hash definitions
 */
#define	LH_HASH_SZ		32	/* number of hash lists */

/*
 * Flag for LDI handle's lh_flags field
 */
#define	LH_FLAGS_NOTIFY	0x0001	/* invoked in context of a notify */

#define	LDI_LIST_T		/* list_t instead of simple linked list */

/*
 * LDI handle (OS X)
 */
struct ldi_handle {
	/* protected by ldi_handle_hash_lock */
#ifdef LDI_LIST_T
	list_node_t		lh_node;	/* list membership */
#else
	struct ldi_handle	*lh_next;	/* list membership */
#endif
	uint_t			lh_ref;		/* active references */
	uint_t			lh_flags;	/* for notify event */

	/* protected by handle lh_lock */
	kmutex_t		lh_lock;	/* internal lock */
	kcondvar_t		lh_cv;		/* for concurrent open */
	ldi_status_t		lh_status;	/* Closed, Offline, Online */
	uint_t			lh_openref;	/* open client count */

	/* unique/static fields in the handle */
	uint_t			lh_type;	/* IOKit or vnode */
	uint_t			lh_fmode;	/* FREAD | FWRITE */
	dev_t			lh_dev;		/* device number */
	union dev_ptr {
		void		*media;		/* IOMedia */
		vnode_t		*devvp;		/* vnode */
	} lh_un;
	void			*lh_client;	/* IOService */
	void			*lh_notifier;	/* IONotifier */
};

/* Shared functions */
int handle_status_change(struct ldi_handle *, int);
void handle_hold(struct ldi_handle *);
void handle_release(struct ldi_handle *);
ldi_status_t handle_open_start(struct ldi_handle *);
void handle_open_done(struct ldi_handle *, ldi_status_t);

/* Handle IOKit functions */
struct ldi_handle *handle_alloc_iokit(dev_t, int);
int handle_register_notifier(struct ldi_handle *);
int handle_close_iokit(struct ldi_handle *);
int handle_free_ioservice(struct ldi_handle *);
int handle_alloc_ioservice(struct ldi_handle *);
int handle_remove_notifier(struct ldi_handle *);
int ldi_open_media_by_dev(dev_t, int, ldi_handle_t *);
int ldi_open_media_by_path(char *, int, ldi_handle_t *);
int handle_get_size_iokit(struct ldi_handle *, uint64_t *, uint64_t *);
int handle_sync_iokit(struct ldi_handle *);
int buf_strategy_iokit(ldi_buf_t *, struct ldi_handle *);

/* Handle vnode functions */
struct ldi_handle *handle_alloc_vnode(dev_t, int);
int handle_close_vnode(struct ldi_handle *);
// static int handle_open_vnode(struct ldi_handle *, char *);
int handle_get_size_vnode(struct ldi_handle *, uint64_t *, uint64_t *);
int handle_sync_vnode(struct ldi_handle *lhp);
dev_t dev_from_path(char *path);
int ldi_open_vnode_by_path(char *, dev_t, int, ldi_handle_t *);
int buf_strategy_vnode(ldi_buf_t *, struct ldi_handle *);

/*
 * LDI event information
 */
typedef struct ldi_ev_callback_impl {
	struct ldi_handle	*lec_lhp;
#ifdef illumos
	dev_info_t		*lec_dip;
#endif
	dev_t			lec_dev;
	int			lec_spec;
	int			(*lec_notify)();
	void			(*lec_finalize)();
	void			*lec_arg;
	void			*lec_cookie;
	void			*lec_id;
	list_node_t		lec_list;
} ldi_ev_callback_impl_t;

/*
 * Members of "struct ldi_ev_callback_list" are protected by their le_lock
 * member.  The struct is currently only used once, as a file-level global,
 * and the locking protocol is currently implemented in ldi_ev_lock() and
 * ldi_ev_unlock().
 *
 * When delivering events to subscribers, ldi_invoke_notify() and
 * ldi_invoke_finalize() will walk the list of callbacks: le_head.  It is
 * possible that an invoked callback function will need to unregister an
 * arbitrary number of callbacks from this list.
 *
 * To enable ldi_ev_remove_callbacks() to remove elements from the list
 * without breaking the walk-in-progress, we store the next element in the
 * walk direction on the struct as le_walker_next and le_walker_prev.
 */
struct ldi_ev_callback_list {
	kmutex_t		le_lock;
	kcondvar_t		le_cv;
	int			le_busy;
	void			*le_thread;
	list_t			le_head;
	ldi_ev_callback_impl_t	*le_walker_next;
	ldi_ev_callback_impl_t	*le_walker_prev;
};

int ldi_invoke_notify(dev_info_t *, dev_t, int, char *, void *);
void ldi_invoke_finalize(dev_info_t *, dev_t, int, char *, int, void *);
int e_ddi_offline_notify(dev_info_t *);
void e_ddi_offline_finalize(dev_info_t *, int);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* _SYS_LDI_IMPL_OSX_H */