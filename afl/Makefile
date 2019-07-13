#	$NetBSD$

KMOD=	kcov_afl
SRCS=	kcov_afl.c

HAVE_DEBUGCON_PRINTF=no

CPPFLAGS+=	-I/root/workspace/kcov_4/debugcon_printf/
CPPFLAGS+=	-DHAVE_DEBUGCON_PRINTF
CPPFLAGS+=	-DDIAGNOSTIC


.include <bsd.kmodule.mk>
