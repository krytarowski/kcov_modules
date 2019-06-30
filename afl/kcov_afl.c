/*	$NetBSD$	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/kcov.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/systm.h>
#include <uvm/uvm.h>

#include <debugcon_printf.h>

/* -------------------------- AFL Long HASH --------------------------- */
#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull
#define BITS_PER_LONG 	64

static uint64_t
_long_hash64(uint64_t val, unsigned int bits) {
	return val * GOLDEN_RATIO_64 >> (64 - bits);
}

typedef struct afl_ctx {
	uint8_t *afl_area;
	struct uvm_object *afl_uobj;
	size_t afl_bsize;
	uint64_t afl_prev_loc;
	lwpid_t lid;
} kcov_afl_t;

static void *
kcov_afl_open(void)
{
	kcov_afl_t *afl;

	afl = kmem_zalloc(sizeof(*afl), KM_SLEEP);

	/* curlwp can differ for other calls, store the one from open(2) */
	afl->lid = curlwp->l_lid;

	return afl;
}

static void
kcov_afl_free(void *priv)
{
	kcov_afl_t *afl;

	afl = (kcov_afl_t *)priv;

	if (afl->afl_area != NULL) {
		uvm_deallocate(kernel_map, (vaddr_t)afl->afl_area, afl->afl_bsize);
	}

	kmem_free(afl, sizeof(*afl));
}

static int
kcov_afl_setbufsize(void *priv, uint64_t size)
{
	kcov_afl_t *afl;
	int error;

	afl = (kcov_afl_t *)priv;

	if (afl->afl_area != NULL)
		return EEXIST;

	afl->afl_bsize = size;
	afl->afl_uobj = uao_create(afl->afl_bsize, 0);

	/* Map the uobj into the kernel address space, as wired. */
	afl->afl_area = NULL;
	error = uvm_map(kernel_map, (vaddr_t *)&afl->afl_area, afl->afl_bsize,
	    afl->afl_uobj, 0, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
	    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
	if (error) {
		uao_detach(afl->afl_uobj);
		return error;
	}
	error = uvm_map_pageable(kernel_map, (vaddr_t)afl->afl_area,
	    (vaddr_t)afl->afl_area + afl->afl_bsize, false, 0);
	if (error) {
		uvm_deallocate(kernel_map, (vaddr_t)afl->afl_area, afl->afl_bsize);
		return error;
	}

	return 0;
}

static int
kcov_afl_enable(void *priv, int mode)
{
	kcov_afl_t *afl;

	afl = (kcov_afl_t *)priv;

	if (afl->afl_area == NULL)
		return ENOBUFS;

	return 0;
}

static int
kcov_afl_disable(void *priv __unused)
{

	return 0;
}

static int
kcov_afl_mmap(void *priv, size_t size, off_t off, struct uvm_object **uobjp)
{
	kcov_afl_t *afl;

	afl = (kcov_afl_t *)priv;

	if ((size + off) > afl->afl_bsize)
		return ENOMEM;

	uao_reference(afl->afl_uobj);

	*uobjp = afl->afl_uobj;

	return 0;
}

static void
kcov_afl_cov_trace_pc(void *priv, intptr_t pc)
{
	kcov_afl_t *afl = priv;

//	debugcon_printf("#: '%x'\n", pc);

	++afl->afl_area[(afl->afl_prev_loc ^ pc) & (afl->afl_bsize-1)];
	afl->afl_prev_loc = _long_hash64(pc, BITS_PER_LONG);

	return;
}

static void
kcov_afl_cov_trace_cmp(void *priv __unused, uint64_t type __unused,
    uint64_t arg1 __unused, uint64_t arg2 __unused, intptr_t pc __unused)
{

	/* Not implemented. */

	return;
}

static struct kcov_ops kcov_afl_ops = {
	.open = kcov_afl_open,
	.free = kcov_afl_free,
	.setbufsize = kcov_afl_setbufsize,
	.enable = kcov_afl_enable,
	.disable = kcov_afl_disable,
	.mmap = kcov_afl_mmap,
	.cov_trace_pc = kcov_afl_cov_trace_pc,
	.cov_trace_cmp = kcov_afl_cov_trace_cmp
};

MODULE(MODULE_CLASS_MISC, kcov_afl, "kcov,debugcon_printf");

static int
kcov_afl_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = kcov_ops_set(&kcov_afl_ops);
		if (error)
			return error;
		debugcon_printf("AFL module loaded.\n");
		break;

	case MODULE_CMD_FINI:
		error = kcov_ops_unset(&kcov_afl_ops);
		if (error)
			return error;
		debugcon_printf("AFL module unloaded.\n");
		break;

	case MODULE_CMD_STAT:
		debugcon_printf("AFL module status queried.\n");
		break;

	default:
		return ENOTTY;
	}

	return 0;
}
