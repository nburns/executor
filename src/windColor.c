/* Copyright 1994, 1995 by Abacus Research and
 * Development, Inc.  All rights reserved.
 */

#if !defined (OMIT_RCSID_STRINGS)
char ROMlib_rcsid_windColor[] =
		"$Id: windColor.c 63 2004-12-24 18:19:43Z ctm $";
#endif


/* color window manager functions; introduced in IM-V or beyond */

#include "rsys/common.h"
#include "QuickDraw.h"
#include "CQuickDraw.h"
#include "WindowMgr.h"
#include "MemoryMgr.h"

#include "rsys/cquick.h"
#include "rsys/wind.h"

/* return a pointer to the auxilary window record
   associated with `w' */

AuxWinHandle default_aux_win = NULL;

#define BLACK_RGB { CWC (0), CWC (0), CWC (0), }
#define WHITE_RGB { CWC (0xFFFF), CWC (0xFFFF), CWC (0xFFFF), }

/* bw window colortable */
const ColorSpec default_bw_win_ctab[] =
{
  { CWC (wContentColor),     WHITE_RGB },
  { CWC (wFrameColor),       BLACK_RGB },
  { CWC (wTextColor),        BLACK_RGB },
  { CWC (wHiliteColor),      BLACK_RGB },
  { CWC (wTitleBarColor),    WHITE_RGB },
  { CWC (wHiliteColorLight), WHITE_RGB },
  { CWC (wHiliteColorDark),  BLACK_RGB },
  { CWC (wTitleBarLight),    WHITE_RGB },
  { CWC (wTitleBarDark),     BLACK_RGB },
  { CWC (wDialogLight),      BLACK_RGB },
  { CWC (wDialogDark),       BLACK_RGB },
  { CWC (wTingeLight),       BLACK_RGB },
  { CWC (wTingeDark),        BLACK_RGB },
};

#define LT_BLUISH_RGB { CWC (0xCCCC), CWC (0xCCCC), CWC (0xFFFF) }
#define DK_BLUISH_RGB { CWC (0x3333), CWC (0x3333), CWC (0x6666) }
/* the default `bluish' window color table */
const ColorSpec default_color_win_ctab[] =
{
  { CWC (wContentColor),     WHITE_RGB },
  { CWC (wFrameColor),       BLACK_RGB },
  { CWC (wTextColor),        BLACK_RGB },
  { CWC (wHiliteColor),      BLACK_RGB },
  { CWC (wTitleBarColor),    WHITE_RGB },
  { CWC (wHiliteColorLight), WHITE_RGB },
  { CWC (wHiliteColorDark),  BLACK_RGB },
  { CWC (wTitleBarLight),    WHITE_RGB },
  { CWC (wTitleBarDark),     BLACK_RGB },
  { CWC (wDialogLight),      LT_BLUISH_RGB },
  { CWC (wDialogDark),       BLACK_RGB },
  { CWC (wTingeLight),       LT_BLUISH_RGB },
  { CWC (wTingeDark),        DK_BLUISH_RGB },
};
#undef DK_BLUISH_RGB
#undef LT_BLUISH_RGB

#define GRAY_RGB { CWC (0x8888), CWC (0x8888), CWC (0x8888) }
/* stolen from the default colortable excel
   tries to install */
const ColorSpec default_system6_color_win_ctab[] =
{
  { CWC (wContentColor),     WHITE_RGB },
  { CWC (wFrameColor),       BLACK_RGB },
  { CWC (wTextColor),        BLACK_RGB },
  { CWC (wHiliteColor),      BLACK_RGB },
  { CWC (wTitleBarColor),    WHITE_RGB },
};
#undef GRAY_RGB
#undef WHITE_RGB
#undef BLACK_RGB

void
wind_color_init (void)
{
  ZONE_SAVE_EXCURSION
    (SysZone,
     {
       default_aux_win = (AuxWinHandle) NewHandle (sizeof (AuxWinRec));
       SETP0 (default_aux_win, awNext);
       SETP0 (default_aux_win, awOwner);
       SETP (default_aux_win, awCTable,
	     (CTabHandle) GetResource (TICK("wctb"), 0));
       SETP0 (default_aux_win, dialogCItem);
       HxX (default_aux_win, awFlags) = 0;
       SETP0 (default_aux_win, awReserved);
       HxX (default_aux_win, awRefCon) = 0;
     });
}

HIDDEN_AuxWinHandle *
lookup_aux_win (WindowPtr w)
{
  HIDDEN_AuxWinHandle *t;

#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
  for (t = (HIDDEN_AuxWinHandle *) &AuxWinHead_H;
       t->p && HxP (MR (t->p), awOwner) != w;
       t = (HIDDEN_AuxWinHandle *) &HxX (MR (t->p), awNext))
    ;
#else
  for (t = &AuxWinHead_H;
       t->pp && HxP (STARH (t), awOwner) != w;
       t = &HxX (STARH (t), awNext))
    ;
#endif
  return t;
}

P2 (PUBLIC pascal trap, void, SetWinColor,
    WindowPtr, w,
    CTabHandle, new_w_ctab)
{
  if (w)
    {
      AuxWinHandle aux_w;

      aux_w = STARH (lookup_aux_win (w));

      if (!aux_w)
	{
	  AuxWinHandle t_aux_w;

	  t_aux_w = GET_AuxWinHead ();
	  aux_w = (AuxWinHandle) NewHandle (sizeof (AuxWinRec));
	  SET_AuxWinHead (aux_w);
	  SETP (aux_w, awNext, t_aux_w);
	  SETP (aux_w, awOwner, w);
	  SETP (aux_w, awCTable, new_w_ctab);
	  SETP0 (aux_w, dialogCItem);
	  HxX (aux_w, awFlags)    = 0;
	  SETP0 (aux_w, awReserved);
	  HxX (aux_w, awRefCon)   = 0;
	}
      else
	SETP (aux_w, awCTable, new_w_ctab);

      if (CGrafPort_p (w))
	{
	  ColorSpec *w_ctab_table;
	  RGBColor *color;

	  w_ctab_table = CTAB_TABLE (new_w_ctab);
	  color = &w_ctab_table[wContentColor].rgb;

	  CPORT_RGB_BK_COLOR (w) = *color;

	  PORT_BK_COLOR_X (w) = CL (Color2Index (color));

	  if (WINDOW_VISIBLE_X (w))
	    THEPORT_SAVE_EXCURSION
	      (w,
	       {
		 RgnHandle t;
		 t = NewRgn ();

		 CopyRgn (WINDOW_CONT_REGION (w), t);
		 OffsetRgn (t,
			    CW (PORT_BOUNDS (w).left),
			    CW (PORT_BOUNDS (w).top));
		 EraseRgn (t);
		 DisposeRgn (t);
	       });
	}

      if (WINDOW_VISIBLE_X (w))
	{
	  SetPort (GET_WMgrPort ());

	  SetClip (WINDOW_STRUCT_REGION (w));
	  ClipAbove ((WindowPeek)w);
	  WINDCALL (w, wDraw, 0);
	}
    }
  else
    {
      abort ();
    }
}

/* NOTE: all documentation on this function is either vague to the
   point of certain mis-interpretation, or plain wrong.

   it's second argument is `AuxWinHandle *' (or `AuxWinHndl *' in
   Think C speak), and returns the aux window entry for the given
   window; if NULL is passed, an handle to a `AuxWinRec' is placed in
   the aux_w_out argument which contains the default window color
   table

   they named it correctly, but noone got the documentation right */

P2 (PUBLIC pascal trap, BOOLEAN, GetAuxWin,
    WindowPtr, w,
    HIDDEN_AuxWinHandle *, aux_win_out)
{
  if (! w)
    {
      HPTR_WRITE (aux_win_out, default_aux_win);
      return TRUE;
    }
  else
    {
      AuxWinHandle t;

      t = STARH (lookup_aux_win (w));
      if (t)
	{
	  HPTR_WRITE (aux_win_out, t);
	  return TRUE;
	}
      else
	{
	  HPTR_WRITE (aux_win_out, default_aux_win);
	  return FALSE;
	}
    }
}
