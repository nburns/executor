/* Copyright 1992 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_hfsWorkingdir[] =
	    "$Id: hfsWorkingdir.c 86 2005-05-25 00:47:12Z ctm $";
#endif

#include "rsys/common.h"
#include "OSUtil.h"
#include "FileMgr.h"
#include "rsys/hfs.h"
#include "rsys/file.h"
#include "MemoryMgr.h"

/*
 * TODO: use this working directory stuff in ROMlib
 */

PUBLIC OSErr ROMlib_dirbusy(LONGINT dirid, HVCB *vcbp)
{
#if defined(MAC)
    wdentry *wdp, *ewdp;
    
    for (wdp = (wdentry *) (CL(WDCBsPtr) + sizeof(INTEGER)),
	    ewdp = (wdentry *) (CL(WDCBsPtr) + CW(*(INTEGER *)CL(WDCBsPtr)));
							    wdp != ewdp; wdp++)
	;
    return wdp == ewdp ? noErr : fBsyErr;
#else
    return noErr;
#endif
}

PUBLIC OSErr ROMlib_mkwd(WDPBPtr pb, HVCB *vcbp, LONGINT dirid, LONGINT procid)
{
    wdentry *wdp, *ewdp, *firstfreep;
    OSErr retval;
    INTEGER n_wd_bytes, new_n_wd_bytes;
    Ptr newptr;
    THz saveZone;

    firstfreep = 0;
    for (wdp = (wdentry *) ((char *)GET_WDCBsPtr() + sizeof(INTEGER)),
	       ewdp = (wdentry *) ((char *)GET_WDCBsPtr() + CW(*(INTEGER *)GET_WDCBsPtr()));
							  wdp != ewdp; wdp++) {
	if (!firstfreep && !wdp->vcbp.pp)
	    firstfreep = wdp;
	if ((HVCB *)PPR(wdp->vcbp) == vcbp && CL(wdp->dirid) == dirid &&
						   CL(wdp->procid) == procid) {
	    pb->ioVRefNum = CW(WDPTOWDNUM(wdp));
/*-->*/	    return noErr;
	}
    }
    if (!firstfreep) {
	n_wd_bytes = CW(*(INTEGER *) GET_WDCBsPtr());
	new_n_wd_bytes = (n_wd_bytes - sizeof(INTEGER)) * 2 + sizeof(INTEGER);
	saveZone = GET_TheZone();
	SET_TheZone(GET_SysZone());
	newptr = NewPtr(new_n_wd_bytes);
	SET_TheZone(saveZone);
	if (!newptr)
	    retval = tmwdoErr;
	else {
	    BlockMove( GET_WDCBsPtr(), newptr, n_wd_bytes);
	    DisposPtr( GET_WDCBsPtr() );
	    SET_WDCBsPtr(newptr);
	    *(INTEGER *) newptr = CW(new_n_wd_bytes);
	    firstfreep = (wdentry *) (newptr + n_wd_bytes);
	    retval = noErr;
	}
    } else
	retval = noErr;
    if (retval == noErr) {
	PACKED_ASSIGN(firstfreep->vcbp, vcbp);
	firstfreep->dirid = CL(dirid);
	firstfreep->procid = CL(procid);
	pb->ioVRefNum = CW(WDPTOWDNUM(firstfreep));
	retval = noErr;
    }
    return retval;
}

PUBLIC OSErr hfsPBOpenWD(WDPBPtr pb, BOOLEAN async)
{
    LONGINT dirid;
    OSErr retval;
    filekind kind;
    btparam btparamrec;
    HVCB *vcbp;
    StringPtr namep;
    
    kind = regular|directory;
    retval = ROMlib_findvcbandfile((ioParam *)pb, Cx(pb->ioWDDirID),
						    &btparamrec, &kind, FALSE);
    if (retval != noErr)
	PBRETURN(pb, retval);
    vcbp = btparamrec.vcbp;
    retval = ROMlib_cleancache(vcbp);
    if (retval != noErr)
	PBRETURN(pb, retval);
    namep = (StringPtr)PPR(pb->ioNamePtr);
    if (kind == directory && namep && namep[0])
	dirid =
	      CL(((directoryrec *) DATAPFROMKEY(btparamrec.foundp))->dirDirID);
    else
	dirid = CL(pb->ioWDDirID);
    retval = ROMlib_mkwd(pb, vcbp, dirid, CL(pb->ioWDProcID));

    PBRETURN(pb, retval);
}

PUBLIC OSErr hfsPBCloseWD(WDPBPtr pb, BOOLEAN async)
{
    wdentry *wdp;
    OSErr retval;
    
    retval = noErr;
    if (ISWDNUM(Cx(pb->ioVRefNum))) {
	wdp = WDNUMTOWDP(Cx(pb->ioVRefNum));
	if (wdp)
	    PACKED_ASSIGN0(wdp->vcbp);
	else
	    retval = nsvErr;
    }
    PBRETURN(pb, retval);
}

PUBLIC OSErr hfsPBGetWDInfo(WDPBPtr pb, BOOLEAN async)
{
    OSErr retval;
    wdentry *wdp, *ewdp;
    INTEGER i;
    BOOLEAN foundelsewhere;
    HVCB *vcbp;
    
    foundelsewhere = FALSE;
    retval = noErr;
    wdp = 0;
    if (Cx(pb->ioWDIndex) > 0) {
	i = Cx(pb->ioWDIndex);
	wdp = (wdentry *) ((char *)GET_WDCBsPtr() + sizeof(INTEGER));
	ewdp = (wdentry *) ((char *)GET_WDCBsPtr() + CW(*(INTEGER *)GET_WDCBsPtr()));
	if (Cx(pb->ioVRefNum) < 0) {
	    for (;wdp != ewdp; wdp++)
		if (wdp->vcbp.pp && ((HVCB *)PPR(wdp->vcbp))->vcbVRefNum == pb->ioVRefNum && --i <= 0)
		    break;
	} else if (pb->ioVRefNum == 0) {
	    for (;wdp != ewdp && i > 1; ++wdp)
	      if (wdp->vcbp.pp)
		--i;
	} else /* if (Cx(pb->ioVRefNum) > 0 */ {
	    for (;wdp != ewdp; wdp++)
		if (((HVCB *)PPR(wdp->vcbp))->vcbDrvNum == pb->ioVRefNum && --i <= 0)
		    break;
	}
	if (wdp == ewdp || !wdp->vcbp.pp)
	    wdp = 0;
    } else if (ISWDNUM(Cx(pb->ioVRefNum)))
	wdp = WDNUMTOWDP(Cx(pb->ioVRefNum));
    else {
	vcbp = ROMlib_findvcb(Cx(pb->ioVRefNum), (StringPtr) 0, (LONGINT *) 0,
									 TRUE);
	if (vcbp) {
	    if (pb->ioNamePtr.pp)
		str255assign((StringPtr)PPR(pb->ioNamePtr), (StringPtr) vcbp->vcbVN);
	    pb->ioWDProcID  = 0;
	    pb->ioVRefNum   = pb->ioWDVRefNum = vcbp->vcbVRefNum;
	    pb->ioWDDirID   = CL((vcbp == GET_DefVCBPtr()) ? DefDirID : 2);
	    foundelsewhere = TRUE;
	}
    }
	
    if (!foundelsewhere) {
	if (wdp) {
	    if (pb->ioNamePtr.pp)
		str255assign((StringPtr)PPR(pb->ioNamePtr),
					     (StringPtr) ((HVCB *)PPR(wdp->vcbp))->vcbVN);
	    if (Cx(pb->ioWDIndex) > 0)
		pb->ioVRefNum = ((HVCB *)PPR(wdp->vcbp))->vcbVRefNum;
	    pb->ioWDProcID = wdp->procid;
	    pb->ioWDVRefNum = ((HVCB *)PPR(wdp->vcbp))->vcbVRefNum;
	    pb->ioWDDirID   = wdp->dirid;
	} else
	    retval = nsvErr;
    }
	
    PBRETURN(pb, retval);
}

PUBLIC OSErr
GetWDInfo (INTEGER wd, INTEGER *vrefp, LONGINT *dirp, LONGINT *procp)
{
  OSErr retval;

  WDPBRec wdp;
  memset (&wdp, 0, sizeof wdp);
  wdp.ioVRefNum = CW (wd);
  retval = PBGetWDInfo (&wdp, FALSE);
  if (retval == noErr)
    {
      *vrefp = wdp.ioVRefNum;
      *dirp = wdp.ioWDDirID;
      *procp = wdp.ioWDProcID;
    }
  return retval;
}

PUBLIC void ROMlib_adjustdirid(LONGINT *diridp, HVCB *vcbp, INTEGER vrefnum)
{
    wdentry *wdp;
    
    if (*(ULONGINT *) diridp <= 1 && ISWDNUM(vrefnum)) {
	wdp = WDNUMTOWDP(vrefnum);
	if ((HVCB *)PPR(wdp->vcbp) == vcbp)
	    *diridp = CL(wdp->dirid);
    } else if (*diridp == 0 && !vrefnum /* vcbp == CL(DefVCBPtr) */)
	*diridp = CL(DefDirID);
    if (*diridp == 0)
	*diridp = 2;
}
