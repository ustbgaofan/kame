/*	$NetBSD: fssvar.h,v 1.5 2004/02/24 15:12:51 wiz Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_DEV_FSSVAR_H
#define _SYS_DEV_FSSVAR_H

struct fss_set {
	char		*fss_mount;	/* Mount point of file system */
	char		*fss_bstore;	/* Path of backing store */
	blksize_t	fss_csize;	/* Preferred cluster size */
};

struct fss_get {
	char		fsg_mount[MNAMELEN]; /* Mount point of file system */
	struct timeval	fsg_time;	/* Time this snapshot was taken */
	blksize_t	fsg_csize;	/* Current cluster size */
	blkcnt_t	fsg_mount_size;	/* # clusters on file system */
	blkcnt_t	fsg_bs_size;	/* # clusters on backing store */
};

#define FSSIOCSET	_IOW('F', 0, struct fss_set)	/* Configure */
#define FSSIOCGET	_IOR('F', 1, struct fss_get)	/* Status */
#define FSSIOCCLR	_IO('F', 2)			/* Unconfigure */

#ifdef _KERNEL

#define FSS_CLUSTER_MAX	(1<<24)		/* Upper bound of clusters. The
					   sc_copied map uses up to
					   FSS_CLUSTER_MAX/NBBY bytes */

#define FSS_LOCK(sc, s) \
	do { \
		(s) = splbio(); \
		simple_lock(&(sc)->sc_slock); \
	} while (/*CONSTCOND*/0)

#define FSS_UNLOCK(sc, s) \
	do { \
		simple_unlock(&(sc)->sc_slock); \
		splx((s)); \
	} while (/*CONSTCOND*/0)

/* Device to softc, NULL on error */
#define FSS_DEV_TO_SOFTC(dev) \
	(minor((dev)) < 0 || minor((dev)) >= NFSS ? NULL : \
	    &fss_softc[minor((dev))])

/* Check if still valid */
#define FSS_ISVALID(sc) \
	(((sc)->sc_flags & (FSS_ACTIVE|FSS_ERROR)) == FSS_ACTIVE)

/* Offset to cluster */
#define FSS_BTOCL(sc, off) \
	((off) >> (sc)->sc_clshift)

/* Cluster to offset */
#define FSS_CLTOB(sc, cl) \
	((off_t)(cl) << (sc)->sc_clshift)      

/* Offset from start of cluster */
#define FSS_CLOFF(sc, off) \
	((off) & (sc)->sc_clmask)

/* Size of cluster */
#define FSS_CLSIZE(sc) \
	(1 << (sc)->sc_clshift)

/* Offset to backing store block */
#define FSS_BTOFSB(sc, off) \
	((off) >> (sc)->sc_bs_bshift)

/* Backing store block to offset */
#define FSS_FSBTOB(sc, blk) \
	((off_t)(blk) << (sc)->sc_bs_bshift)

/* Offset from start of backing store block */
#define FSS_FSBOFF(sc, off) \
	((off) & (sc)->sc_bs_bmask)

/* Size of backing store block */
#define FSS_FSBSIZE(sc) \
	(1 << (sc)->sc_bs_bshift)

typedef enum {
	FSS_READ,
	FSS_WRITE
} fss_io_type;

typedef enum {
	FSS_CACHE_FREE	= 0,		/* Cache entry is free */
	FSS_CACHE_BUSY	= 1,		/* Cache entry is read from device */
	FSS_CACHE_VALID	= 2		/* Cache entry contains valid data */
} fss_cache_type;

struct fss_cache {
	fss_cache_type	fc_type;	/* Current state */
	struct fss_softc *fc_softc;	/* Backlink to our softc */
	volatile int	fc_xfercount;	/* Number of outstanding transfers */
	u_int32_t	fc_cluster;	/* Cluster number of this entry */
	caddr_t		fc_data;	/* Data */
};

struct fss_softc {
	int		sc_unit;	/* Logical unit number */
	struct simplelock sc_slock;	/* Protect this softc */
	volatile int	sc_flags;	/* Flags */
#define FSS_ACTIVE	0x01		/* Snapshot is active */
#define FSS_ERROR	0x02		/* I/O error occurred */
#define FSS_BS_THREAD	0x04		/* Kernel thread is running */
#define FSS_EXCL	0x08		/* Exclusive access granted */
#define FSS_BS_ALLOC	0x10		/* Allocate backing store */
	struct mount	*sc_mount;	/* Mount point */
	char		sc_mntname[MNAMELEN]; /* Mount point */
	struct timeval	sc_time;	/* Time this snapshot was taken */
	dev_t		sc_bdev;	/* Underlying block device */
	struct vnode	*sc_mount_vp;	/* Underlying spec vnode */
	struct vnode	*sc_bs_vp;	/* Our backing store */
	off_t		sc_bs_size;	/* Its size in bytes */
	int		sc_bs_bshift;	/* Shift of backing store block */
	u_int32_t	sc_bs_bmask;	/* Mask of backing store block */
	struct proc	*sc_bs_proc;	/* Our kernel thread */
	int		sc_clshift;	/* Shift of cluster size */
	u_int32_t	sc_clmask;	/* Mask of cluster size */
	u_int32_t	sc_clcount;	/* # clusters in file system */
	u_int8_t	*sc_copied;	/* Map of clusters already copied */
	long		sc_clresid;	/* Bytes in last cluster */
	int		sc_cache_size;	/* Number of entries in sc_cache */
	struct fss_cache *sc_cache;	/* Cluster cache */
	struct bufq_state sc_bufq;	/* Transfer queue */
	u_int32_t	sc_clnext;	/* Next free cluster on backing store */
	int		sc_indir_size;	/* # clusters for indirect mapping */
	u_int8_t	*sc_indir_valid; /* Map of valid indirect clusters */
	u_int32_t	sc_indir_cur;	/* Current indir cluster number */
	int		sc_indir_dirty;	/* Current indir cluster modified */
	u_int32_t	*sc_indir_data;	/* Current indir cluster data */
};

int fss_umount_hook(struct mount *, int);

#endif /* _KERNEL */

#endif /* !_SYS_DEV_FSSVAR_H */
