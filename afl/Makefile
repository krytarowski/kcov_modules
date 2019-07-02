#	$NetBSD$

KMOD=	kcov_afl
SRCS=	kcov_afl.c

$(error uncomment clone and setup debugcon_printf as below)
# CPPFLAGS+=	-I/~/workspace/debugcon_printf/
CPPFLAGS+=	-DDIAGNOSTIC


.include <bsd.kmodule.mk>
