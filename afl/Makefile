#	$NetBSD$

KMOD=	kcov_afl
SRCS=	kcov_afl.c

HAVE_DEBUGCON_PRINTF=0

.if ${HAVE_DEBUGCON_PRINTF} == 1
CPPFLAGS_DBGCON!=pkg-config --cflags debugcon_printf
CPPFLAGS+=${CPPFLAGS_DBGCON}
CPPFLAGS+=	-DHAVE_DEBUGCON_PRINTF
.endif

CPPFLAGS+=	-DDIAGNOSTIC

.include <bsd.kmodule.mk>
