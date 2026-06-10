/* Copyright 1986, 1988, 1989, 1990, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_ctlInit[] =
		"$Id: ctlInit.c 86 2005-05-25 00:47:12Z ctm $";
#endif

/* Forward declarations in ControlMgr.h (DO NOT DELETE THIS LINE) */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "WindowMgr.h"
#include "ControlMgr.h"
#include "ToolboxUtil.h"
#include "ResourceMgr.h"
#include "MemoryMgr.h"
#include "OSUtil.h"

#include "rsys/ctl.h"
#include "rsys/wind.h"
#include "rsys/cquick.h"
#include "rsys/resource.h"

PUBLIC BOOLEAN ROMlib_cdef0_is_rectangular = FALSE;

P9(PUBLIC pascal trap, ControlHandle, NewControl, WindowPtr, wst, Rect *, r,
		  StringPtr, title, BOOLEAN, vis, INTEGER, value, INTEGER, min,
		  INTEGER, max, INTEGER, procid, LONGINT, rc)	/* IMI-319 */
{
    ControlHandle retval;
    GrafPtr gp;
    AuxCtlHandle tmp, auxctlhead;
    Handle temph;
    
    retval = (ControlHandle) NewHandle((Size) sizeof(ControlRecord));
    {
        Handle native_h = GetResource(TICK("CDEF"), procid>>4);
        temph = RM(native_h);
        SETP (retval, contrlDefProc, native_h);
        if (!HxZ (retval, contrlDefProc)) {
            DisposHandle((Handle) retval);
            return 0;
        }
    }

    if ((procid>>4) == 0)
      {
	INTEGER id;
	ResType type;
	Str255 name;

	GetResInfo (MR (temph), &id, &type, name);
	ROMlib_cdef0_is_rectangular =
	  (EqualString (name,
			(StringPtr) "\023rectangular buttons", FALSE, FALSE));
      }

    if (!title)
	title = (StringPtr) "";	/* nothing ("\p" == "") */
    gp = thePort;
    SetPort ((GrafPtr) wst);
    
    tmp = GET_AuxCtlHead();
    auxctlhead = (AuxCtlHandle) NewHandle(sizeof(AuxCtlRec));
    SET_AuxCtlHead(auxctlhead);

    HASSIGN_6
      (auxctlhead,
       acNext, RM (tmp),
       acOwner, RM (retval),
       acCTable, (CCTabHandle) RM (GetResource (TICK("cctb"), 0)),
       acFlags, CW (procid & 0x0F),
       acReserved, CLC (0),
       acRefCon, CLC (0));
    str255assign (CTL_TITLE (retval), title);

    if (!HxZ(auxctlhead, acCTable))
      {
	warning_unexpected ("no 'cctb', 0, probably old system file ");
	SETP (auxctlhead, acCTable, default_ctl_ctab);
      }

    HASSIGN_11
      (retval,
       contrlRect, *r,
       nextControl, WINDOW_CONTROL_LIST_X (wst),
       contrlOwner, RM (wst),
       contrlVis, vis ? 255 : 0,
       contrlHilite, 0,
       contrlValue, CW (value),
       contrlMin, CW (min),
       contrlMax, CW (max),
       contrlData, CLC_NULL,
       contrlAction, CLC_NULL,
       contrlRfCon, CL (rc));
    
    PACKED_ASSIGN (WINDOW_CONTROL_LIST_X (wst), retval);
    
    
    CTL_CALL_EXCURSION
      (retval,
       {
	 CTLCALL (retval, initCntl, 0);
	 if (WINDOW_VISIBLE_X (wst) && vis)
	   CTLCALL (retval, drawCntl, 0);
       });
    
    SetPort (gp);
    return retval;
}

P2(PUBLIC pascal trap, ControlHandle, GetNewControl,		/* IMI-321 */
					        INTEGER, cid, WindowPtr, wst)
{
    typedef contrlrestype *wp;
    MAKE_HIDDEN(wp);
    HIDDEN_wp *wh;
    ControlHandle retval;
    Handle ctab_res_h;

    wh = (HIDDEN_wp *) GetResource(TICK("CNTL"), cid);
    if (!wh)
        return 0;
    if (!HPTR_VAL (wh))
	LoadResource((Handle) wh);
    ctab_res_h = ROMlib_getrestid (TICK ("cctb"), cid);
    retval = NewControl(wst, &(HxX(wh, _crect)),
	    (StringPtr) ((char *) &HxX(wh, _crect) + 22),	/* _ctitle */
            Hx(wh, _cvisible) ? 255 : 0, Hx(wh, _cvalue), Hx(wh, _cmin),
	    Hx(wh, _cmax), Hx(wh, _cprocid),
	    CL(*(LONGINT *)((char *)&HxX(wh, _crect) + 18)));	/* _crefcon */
    if (ctab_res_h)
      SetCtlColor (retval, (CCTabHandle) ctab_res_h);
    return retval;
}

P2 (PUBLIC pascal trap, void, SetCtlColor, ControlHandle, ctl, CCTabHandle, ctab)
{
  AuxCtlHandle aux_c;

  if (ctl)
    {
      aux_c = DEREF_AUX_LOOKUP (lookup_aux_ctl (ctl));
      if (!aux_c)
	{
	  AuxCtlHandle t_aux_c;

	  /* allocate one */
	  t_aux_c = GET_AuxCtlHead ();
	  aux_c = (AuxCtlHandle) NewHandle (sizeof (AuxCtlRec));
	  SET_AuxCtlHead (aux_c);
	  SETP (aux_c, acNext, t_aux_c);
	  SETP (aux_c, acOwner, ctl);

	  SETP (aux_c, acCTable, ctab);

	  HxX(aux_c, acFlags) = 0;
	  HxX(aux_c, acReserved) = 0;
	  HxX(aux_c, acRefCon) = 0;
	}
      else
	SETP (aux_c, acCTable, ctab);

      if (CTL_VIS (ctl))
	Draw1Control (ctl);
    }
  else
    {
      abort ();
    }
}

P1(PUBLIC pascal trap, void, DisposeControl, ControlHandle, c)	/* IMI-321 */
{
    MAKE_HIDDEN(ControlHandle);
    HIDDEN_ControlHandle *t;
    MAKE_HIDDEN(AuxCtlHandle);
    HIDDEN_AuxCtlHandle *auxhp;
    AuxCtlHandle saveauxh;
    
    HideControl(c);
    CTL_CALL_EXCURSION
      (c,
       {
	 CTLCALL(c, dispCntl, 0);
       });
    
    for (t = (HIDDEN_ControlHandle *) &(((WindowPeek)(HxP(c, contrlOwner)))->controlList);
	      HPTR_VAL(t) && STARH(t) != c;
	      t = (HIDDEN_ControlHandle *) &((STARH(STARH(t)))->nextControl))
        ;
    if (HPTR_VAL(t))
        HPTR_COPY_FIELD(t, HxX(c, nextControl));
#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
    for (auxhp = &AuxCtlHead_H;
	 auxhp->p && (STARH(STARH(auxhp)))->acOwner != c;
	 auxhp = (HIDDEN_AuxCtlHandle *) &(STARH(STARH(auxhp)))->acNext)
	;
    if (auxhp->p) {
	saveauxh = STARH(auxhp);
	auxhp->p = STARH(STARH(auxhp))->acNext;
	DisposHandle((Handle) saveauxh);
    }
#else
    for (auxhp = &AuxCtlHead_H;
	 auxhp->pp && HxP(STARH(auxhp), acOwner) != c;
	 auxhp = &HxX(STARH(auxhp), acNext))
	;
    if (auxhp->pp) {
	saveauxh = STARH(auxhp);
	auxhp->pp = HxX(STARH(auxhp), acNext).pp;
	DisposHandle((Handle) saveauxh);
    }
#endif

    DisposHandle((Handle) c);
}

P1(PUBLIC pascal trap, void, KillControls, WindowPtr, w)	/* IMI-321 */
{
    ControlHandle c, t;
    
    for (c = WINDOW_CONTROL_LIST(w); c ; ) {
        t = c;
        c = HxP(c, nextControl);
        DisposeControl(t);
    }
}
