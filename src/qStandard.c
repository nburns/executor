/* Copyright 1986, 1988, 1989, 1990 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_qStandard[] =
		    "$Id: qStandard.c 63 2004-12-24 18:19:43Z ctm $";
#endif

/* Forward declarations in QuickDraw.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "rsys/pstuff.h"

#if !defined (BINCOMPAT)
#define PTRCAST (Ptr)
#else /* BINCOMPAT */
#define PTRCAST
#endif /* BINCOMPAT */

P1(PUBLIC pascal trap, void, SetStdProcs, QDProcs *, procs)
{
    PACKED_ASSIGN(procs->textProc, PTRCAST P_StdText);
    PACKED_ASSIGN(procs->lineProc, PTRCAST P_StdLine);
    PACKED_ASSIGN(procs->rectProc, PTRCAST P_StdRect);
    PACKED_ASSIGN(procs->rRectProc, PTRCAST P_StdRRect);
    PACKED_ASSIGN(procs->ovalProc, PTRCAST P_StdOval);
    PACKED_ASSIGN(procs->arcProc, PTRCAST P_StdArc);
    PACKED_ASSIGN(procs->polyProc, PTRCAST P_StdPoly);
    PACKED_ASSIGN(procs->rgnProc, PTRCAST P_StdRgn);
    PACKED_ASSIGN(procs->bitsProc, PTRCAST P_StdBits);
    PACKED_ASSIGN(procs->commentProc, PTRCAST P_StdComment);
    PACKED_ASSIGN(procs->txMeasProc, PTRCAST P_StdTxMeas);
    PACKED_ASSIGN(procs->getPicProc, PTRCAST P_StdGetPic);
    PACKED_ASSIGN(procs->putPicProc, PTRCAST P_StdPutPic);
}
