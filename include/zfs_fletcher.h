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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2013 Saso Kiselkov. All rights reserved.
 * Copyright (c) 2016 by Delphix. All rights reserved.
 */

#ifndef	_ZFS_FLETCHER_H
#define	_ZFS_FLETCHER_H

#include <sys/types.h>
#include <sys/spa.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * fletcher checksum functions
 */

void fletcher_init(zio_cksum_t *);

void fletcher_2_native(const void *, size_t, const void *, zio_cksum_t *);
void fletcher_2_byteswap(const void *, size_t, const void *, zio_cksum_t *);
int fletcher_2_incremental_native(void *, size_t, void *);
int fletcher_2_incremental_byteswap(void *, size_t, void *);
void fletcher_4_native(const void *, size_t, const void *, zio_cksum_t *);
void fletcher_4_byteswap(const void *, size_t, const void *, zio_cksum_t *);
int fletcher_4_incremental_native(void *, size_t, void *);
int fletcher_4_incremental_byteswap(void *, size_t, void *);

#ifdef	__cplusplus
}
#endif

#endif	/* _ZFS_FLETCHER_H */
