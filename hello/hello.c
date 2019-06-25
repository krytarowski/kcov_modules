/*	$NetBSD$	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kamil Rytarowski.
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

typedef struct kcov_desc_raw {
	struct uvm_object *uobj;
	lwpid_t lid;
	char *buf;
} kcov_hello_t;

static void *
kcov_hello_open(void)
{
	kcov_hello_t *hello;

	hello = kmem_zalloc(sizeof(kcov_hello_t), KM_SLEEP);

	/* curlwp can differ for other calls, store the one from open(2) */
	hello->lid = curlwp->l_lid;

	return hello;
}

static void
kcov_hello_free(void *priv)
{
	kcov_hello_t *hello;

	hello = (kcov_hello_t *)priv;

	if (hello->buf != NULL) {
		uvm_deallocate(kernel_map, (vaddr_t)hello->buf, PAGE_SIZE);
	}

	kmem_free(hello, sizeof(*hello));
}

static int
kcov_hello_setbufsize(void *priv, uint64_t nent __unused)
{
	kcov_hello_t *hello;
	int error;

	hello = (kcov_hello_t *)priv;

	if (hello->buf != NULL)
		return EEXIST;

	hello->uobj = uao_create(PAGE_SIZE, 0);

	/* Map the uobj into the kernel address space, as wired. */
	hello->buf = NULL;
	error = uvm_map(kernel_map, (vaddr_t *)&hello->buf, PAGE_SIZE,
	    hello->uobj, 0, 0, UVM_MAPFLAG(UVM_PROT_RW, UVM_PROT_RW,
	    UVM_INH_SHARE, UVM_ADV_RANDOM, 0));
	if (error) {
		uao_detach(hello->uobj);
		return error;
	}
	error = uvm_map_pageable(kernel_map, (vaddr_t)hello->buf,
	    (vaddr_t)hello->buf + PAGE_SIZE, false, 0);
	if (error) {
		uvm_deallocate(kernel_map, (vaddr_t)hello->buf, PAGE_SIZE);
		return error;
	}

	snprintf(hello->buf, sizeof(hello->buf), "Hello world LWP=%d\n",
	    hello->lid);

	return 0;
}

static int
kcov_hello_enable(void *priv, int mode)
{
	kcov_hello_t *hello;

	hello = (kcov_hello_t *)priv;

	if (hello->buf == NULL)
		return ENOBUFS;

	return 0;
}

static int
kcov_hello_disable(void *priv __unused)
{

	return 0;
}

static int
kcov_hello_mmap(void *priv, size_t size, off_t off, struct uvm_object **uobjp)
{
	kcov_hello_t *hello;

	hello = (kcov_hello_t *)priv;

	if ((size + off) > PAGE_SIZE)
		return ENOMEM;

	uao_reference(hello->uobj);

	*uobjp = hello->uobj;

	return 0;
}

static void
kcov_hello_cov_trace_pc(void *priv __unused, intptr_t pc)
{

	debugcon_printf("%" PRIxPTR "\n", pc);

	return;
}

static void
kcov_hello_cov_trace_cmp(void *priv __unused, uint64_t type __unused,
    uint64_t arg1 __unused, uint64_t arg2 __unused, intptr_t pc __unused)
{

	/* Not implemented. */

	return;
}

static struct kcov_ops kcov_hello_ops = {
	.open = kcov_hello_open,
	.free = kcov_hello_free,
	.setbufsize = kcov_hello_setbufsize,
	.enable = kcov_hello_enable,
	.disable = kcov_hello_disable,
	.mmap = kcov_hello_mmap,
	.cov_trace_pc = kcov_hello_cov_trace_pc,
	.cov_trace_cmp = kcov_hello_cov_trace_cmp
};

MODULE(MODULE_CLASS_MISC, kcov_hello, "kcov,debugcon_printf");

static int
kcov_hello_modcmd(modcmd_t cmd, void *arg __unused)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = kcov_ops_set(&kcov_hello_ops);
		if (error)
			return error;
		debugcon_printf("Example module loaded.\n");
		break;

	case MODULE_CMD_FINI:
		error = kcov_ops_unset(&kcov_hello_ops);
		if (error)
			return error;
		debugcon_printf("Example module unloaded.\n");
		break;

	case MODULE_CMD_STAT:
		debugcon_printf("Example module status queried.\n");
		break;

	default:
		return ENOTTY;
	}

	return 0;
}
