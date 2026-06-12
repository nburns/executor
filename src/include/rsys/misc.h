#if !defined(__RSYS_MISC__)
#define __RSYS_MISC__

/*
 * Copyright 1995 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 * $Id: misc.h 63 2004-12-24 18:19:43Z ctm $
 */

#if !defined (nilhandle_H)
extern HIDDEN_Ptr nilhandle_H;
extern HIDDEN_Ptr dodusesit_H;
extern LONGINT trapvectors;
extern LONGINT hyperlong;
extern LONGINT mathones;
extern INTEGER graphlooksat;
extern LONGINT macwritespace;
extern LONGINT LastSPExtra;
extern LONGINT lastlowglobal;
extern INTEGER MCLKPCmiss1;
extern INTEGER MCLKPCmiss2;
#endif

#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
# define nilhandle  (nilhandle_H.p)
# define dodusesit  (dodusesit_H.p)
# define SET_dodusesit(v) (dodusesit_H.p = RM(v))
#else
# define nilhandle  ((Ptr) PPR(nilhandle_H))
# define dodusesit  ((Ptr) PPR(dodusesit_H))
# define SET_dodusesit(v) (dodusesit_H.pp = RPP(v))
#endif

#endif /* !defined(__RSYS_MISC__) */
