#if !defined (_SYSERR_H_)
#define _SYSERR_H_

/*
 * Copyright 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 * $Id: SysErr.h 63 2004-12-24 18:19:43Z ctm $
 */

typedef enum { EXIST_YES = 0, EXIST_NO = 0xFF } exist_enum_t;

#if !defined (DSAlertTab_H)
extern HIDDEN_Ptr DSAlertTab_H;
extern Rect DSAlertRect;
extern Byte 	WWExist;
extern Byte 	QDExist;
#endif

#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
# define DSAlertTab        (DSAlertTab_H.p)
# define GET_DSAlertTab()  (MR(DSAlertTab_H.p))
# define SET_DSAlertTab(v) (DSAlertTab_H.p = RM(v))
#else
# define GET_DSAlertTab()  ((Ptr) PPR(DSAlertTab_H))
# define SET_DSAlertTab(v) (DSAlertTab_H.pp = RPP(v))
# define DSAlertTab        GET_DSAlertTab()
#endif

extern char syserr_msg[];

extern pascal void C_SysError (short errorcode);

#endif /* !_SYSERR_H_ */
