#	$NetBSD$

KMOD=	kcov_afl
SRCS=	kcov_afl.c

HAVE_DEBUGCON_PRINTF=no

.if ${HAVE_DEBUGCON_PRINTF} == "yes"
CPPFLAGS+=	-I/~/workspace/debugcon_printf/
CPPFLAGS+=	-DHAVE_DEBUGCON_PRINTF
.endif
CPPFLAGS+=	-DDIAGNOSTIC


.include <bsd.kmodule.mk>
