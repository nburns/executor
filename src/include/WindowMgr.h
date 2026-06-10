#if !defined (__WINDOW__)
#define __WINDOW__

/*
 * Copyright 1986, 1989, 1990 by Abacus Research and Development, Inc.
 * All rights reserved.
 *
 * $Id: WindowMgr.h 63 2004-12-24 18:19:43Z ctm $
 */

#include "QuickDraw.h"
#include "EventMgr.h"

#define documentProc	0
#define dBoxProc	1
#define plainDBox	2
#define altDBoxProc	3
#define noGrowDocProc	4
#define movableDBoxProc 5
#define rDocProc	16

#define dialogKind	2
#define userKind	8

#define inDesk		0
#define inMenuBar	1
#define inSysWindow	2
#define inContent	3
#define inDrag		4
#define inGrow		5
#define inGoAway	6

#define noConstraint	0
#define hAxisOnly	1
#define vAxisOnly	2

#define wDraw		0
#define wHit		1
#define wCalcRgns	2
#define wNew		3
#define wDispose	4
#define wGrow		5
#define wDrawGIcon	6

#define wNoHit		0
#define wInContent	1
#define wInDrag		2
#define wInGrow		3
#define wInGoAway	4

/* color table entries */

#define wContentColor		0
#define wFrameColor		1
#define wTextColor		2
#define wHiliteColor		3
#define wTitleBarColor		4
#define wHiliteColorLight	5
#define wHiliteColorDark	6
#define wTitleBarLight		7
#define wTitleBarDark		8
#define wDialogLight		9
#define wDialogDark		10
#define wTingeLight		11
#define wTingeDark		12

#define deskPatID	16


typedef GrafPtr WindowPtr;
typedef HIDDEN_GrafPtr HIDDEN_WindowPtr;

typedef CGrafPtr CWindowPtr;

typedef struct __wr WindowRecord;
typedef WindowRecord *WindowPeek;
MAKE_HIDDEN(WindowPeek);

#include "ControlMgr.h"

struct PACKED  __wr {
  GrafPort port;
  INTEGER windowKind;
  BOOLEAN visible;
  BOOLEAN hilited;
  BOOLEAN goAwayFlag;
  BOOLEAN spareFlag;
  PACKED_MEMBER(RgnHandle, strucRgn);
  PACKED_MEMBER(RgnHandle, contRgn);
  PACKED_MEMBER(RgnHandle, updateRgn);
  PACKED_MEMBER(Handle, windowDefProc);
  PACKED_MEMBER(Handle, dataHandle);
  PACKED_MEMBER(StringHandle, titleHandle);
  INTEGER titleWidth;
  PACKED_MEMBER(ControlHandle, controlList);
  PACKED_MEMBER(WindowPeek, nextWindow);
  PACKED_MEMBER(PicHandle, windowPic);
  LONGINT refCon;
};

typedef struct PACKED {
  Rect userState;
  Rect stdState;
} WStateData;

#define inZoomIn  7
#define inZoomOut 8

#define wInZoomIn  5
#define wInZoomOut 6

typedef struct AuxWinRec *AuxWinPtr;
MAKE_HIDDEN(AuxWinPtr);
typedef HIDDEN_AuxWinPtr *AuxWinHandle;
MAKE_HIDDEN(AuxWinHandle);

typedef struct PACKED AuxWinRec {
  PACKED_MEMBER(AuxWinHandle, awNext);
  PACKED_MEMBER(WindowPtr, awOwner);
  PACKED_MEMBER(CTabHandle, awCTable);
  PACKED_MEMBER(Handle, dialogCItem);
  LONGINT awFlags;
  PACKED_MEMBER(CTabHandle, awReserved);
  LONGINT awRefCon;
} AuxWinRec;

#if !defined (WindowList_H)
extern HIDDEN_WindowPeek 	WindowList_H;
extern HIDDEN_GrafPtr 		WMgrPort_H;
extern HIDDEN_CGrafPtr 		WMgrCPort_H;
extern HIDDEN_RgnHandle 	OldStructure_H;
extern HIDDEN_RgnHandle 	OldContent_H;
extern HIDDEN_RgnHandle 	GrayRgn_H;
extern HIDDEN_RgnHandle 	SaveVisRgn_H;
extern HIDDEN_ProcPtr 		DragHook_H;
extern HIDDEN_WindowPtr 	CurActivate_H;
extern HIDDEN_WindowPtr 	CurDeactive_H;
extern HIDDEN_ProcPtr 		DeskHook_H;
extern HIDDEN_WindowPtr 	GhostWindow_H;
extern HIDDEN_AuxWinHandle	AuxWinHead_H;
extern HIDDEN_PixPatHandle DeskCPat_H;
extern INTEGER 	SaveUpdate;
extern INTEGER 	PaintWhite;
extern Pattern 	DragPattern;
extern Pattern 	DeskPattern;
#endif

#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
# define WindowList	(WindowList_H.p)
# define WMgrPort	(WMgrPort_H.p)
# define WMgrCPort	(WMgrCPort_H.p)
# define OldStructure	(OldStructure_H.p)
# define OldContent	(OldContent_H.p)
# define GrayRgn	(GrayRgn_H.p)
# define SaveVisRgn	(SaveVisRgn_H.p)
# define DragHook	(DragHook_H.p)
# define CurActivate	(CurActivate_H.p)
# define CurDeactive	(CurDeactive_H.p)
# define DeskHook	(DeskHook_H.p)
# define GhostWindow	(GhostWindow_H.p)
# define AuxWinHead	(AuxWinHead_H.p)
# define DeskCPat	(DeskCPat_H.p)
# define GET_AuxWinHead()    MR(AuxWinHead_H.p)
# define SET_AuxWinHead(v)   (AuxWinHead_H.p = RM(v))
# define CLEAR_AuxWinHead()  (AuxWinHead_H.p = CLC_NULL)
# define GET_WindowList()    MR(WindowList_H.p)
# define SET_WindowList(v)   (WindowList_H.p = RM(v))
# define GET_WMgrPort()      MR(WMgrPort_H.p)
# define SET_WMgrPort(v)     (WMgrPort_H.p = RM(v))
# define GET_WMgrCPort()     ((CGrafPtr) MR(WMgrCPort_H.p))
# define SET_WMgrCPort(v)    (WMgrCPort_H.p = RM(v))
# define GET_GrayRgn()       MR(GrayRgn_H.p)
# define SET_GrayRgn(v)      (GrayRgn_H.p = RM(v))
# define GET_SaveVisRgn()    MR(SaveVisRgn_H.p)
# define SET_SaveVisRgn(v)   (SaveVisRgn_H.p = RM(v))
# define GET_OldStructure()  MR(OldStructure_H.p)
# define SET_OldStructure(v) (OldStructure_H.p = RM(v))
# define GET_OldContent()    MR(OldContent_H.p)
# define SET_OldContent(v)   (OldContent_H.p = RM(v))
# define GET_CurActivate()   MR(CurActivate_H.p)
# define SET_CurActivate(v)  (CurActivate_H.p = RM(v))
# define GET_CurDeactive()   MR(CurDeactive_H.p)
# define SET_CurDeactive(v)  (CurDeactive_H.p = RM(v))
#else
/* On 64-bit, all .p globals use .pp (Mac address); use GET/SET macros */
# define GET_AuxWinHead()    ((AuxWinHandle) PPR(AuxWinHead_H))
# define SET_AuxWinHead(v)   (AuxWinHead_H.pp = RPP(v))
# define CLEAR_AuxWinHead()  (AuxWinHead_H.pp = 0)
# define GET_WindowList()    ((WindowPeek) PPR(WindowList_H))
# define SET_WindowList(v)   (WindowList_H.pp = RPP(v))
# define GET_WMgrPort()      ((GrafPtr) PPR(WMgrPort_H))
# define SET_WMgrPort(v)     (WMgrPort_H.pp = RPP(v))
# define GET_WMgrCPort()     ((CGrafPtr) PPR(WMgrCPort_H))
# define SET_WMgrCPort(v)    (WMgrCPort_H.pp = RPP(v))
# define GET_GrayRgn()       ((RgnHandle) PPR(GrayRgn_H))
# define SET_GrayRgn(v)      (GrayRgn_H.pp = RPP(v))
# define GET_SaveVisRgn()    ((RgnHandle) PPR(SaveVisRgn_H))
# define SET_SaveVisRgn(v)   (SaveVisRgn_H.pp = RPP(v))
# define GET_OldStructure()  ((RgnHandle) PPR(OldStructure_H))
# define SET_OldStructure(v) (OldStructure_H.pp = RPP(v))
# define GET_OldContent()    ((RgnHandle) PPR(OldContent_H))
# define SET_OldContent(v)   (OldContent_H.pp = RPP(v))
# define GET_CurActivate()   ((WindowPtr) PPR(CurActivate_H))
# define SET_CurActivate(v)  (CurActivate_H.pp = RPP(v))
# define GET_CurDeactive()   ((WindowPtr) PPR(CurDeactive_H))
# define SET_CurDeactive(v)  (CurDeactive_H.pp = RPP(v))
# define GET_GhostWindow()   ((WindowPtr) PPR(GhostWindow_H))
# define SET_GhostWindow(v)  (GhostWindow_H.pp = RPP(v))
# define GET_DeskHook()      ((ProcPtr) PPR(DeskHook_H))
# define SET_DeskHook(v)     (DeskHook_H.pp = RPP(v))
# define GET_DragHook()      ((ProcPtr) PPR(DragHook_H))
# define SET_DragHook(v)     (DragHook_H.pp = RPP(v))
# define GET_DeskCPat()      ((PixPatHandle) PPR(DeskCPat_H))
# define SET_DeskCPat(v)     (DeskCPat_H.pp = RPP(v))
# define WindowList          GET_WindowList()
# define WMgrPort            GET_WMgrPort()
# define WMgrCPort           GET_WMgrCPort()
# define OldStructure        GET_OldStructure()
# define OldContent          GET_OldContent()
# define GrayRgn             GET_GrayRgn()
# define SaveVisRgn          GET_SaveVisRgn()
# define DragHook            GET_DragHook()
# define CurActivate         GET_CurActivate()
# define CurDeactive         GET_CurDeactive()
# define DeskHook            GET_DeskHook()
# define GhostWindow         GET_GhostWindow()
# define AuxWinHead          GET_AuxWinHead()
# define DeskCPat            GET_DeskCPat()
#endif

#if !defined (__STDC__)
extern void SetWTitle(); 
extern void GetWTitle(); 
extern WindowPtr FrontWindow(); 
extern void HiliteWindow(); 
extern void BringToFront(); 
extern void SelectWindow(); 
extern void ShowHide(); 
extern void HideWindow(); 
extern void ShowWindow(); 
extern void SendBehind(); 
extern void DrawGrowIcon(); 
extern void InitWindows(); 
extern void GetWMgrPort(); 
extern WindowPtr NewWindow(); 
extern WindowPtr GetNewWindow(); 
extern void CloseWindow(); 
extern void DisposeWindow(); 
extern void SetWRefCon(); 
extern LONGINT GetWRefCon(); 
extern void SetWindowPic(); 
extern PicHandle GetWindowPic(); 
extern LONGINT PinRect(); 
extern LONGINT DragTheRgn(); 
extern LONGINT DragGrayRgn(); 
extern void ClipAbove(); 
extern BOOLEAN CheckUpdate(); 
extern void SaveOld(); 
extern void PaintOne(); 
extern void PaintBehind(); 
extern void CalcVis(); 
extern void CalcVisBehind(); 
extern void DrawNew(); 
extern INTEGER GetWVariant(); 
extern INTEGER FindWindow(); 
extern BOOLEAN TrackBox(); 
extern BOOLEAN TrackGoAway(); 
extern void ZoomWindow(); 
extern void MoveWindow(); 
extern void DragWindow(); 
extern LONGINT GrowWindow(); 
extern void SizeWindow(); 
extern void InvalRect(); 
extern void InvalRgn(); 
extern void ValidRect(); 
extern void ValidRgn(); 
extern void BeginUpdate(); 
extern void EndUpdate(); 
#else /* __STDC__ */
extern pascal trap void C_SetWTitle( WindowPtr w, StringPtr t ); extern pascal trap void P_SetWTitle( WindowPtr w, StringPtr t); 
extern pascal trap void C_GetWTitle( WindowPtr w, StringPtr t ); extern pascal trap void P_GetWTitle( WindowPtr w, StringPtr t); 
extern pascal trap WindowPtr C_FrontWindow( void  ); extern pascal trap WindowPtr P_FrontWindow( void ); 
extern pascal trap void C_HiliteWindow( WindowPtr w, BOOLEAN flag ); extern pascal trap void P_HiliteWindow( WindowPtr w, BOOLEAN flag); 
extern pascal trap void C_BringToFront( WindowPtr w ); extern pascal trap void P_BringToFront( WindowPtr w); 
extern pascal trap void C_SelectWindow( WindowPtr w ); extern pascal trap void P_SelectWindow( WindowPtr w); 
extern pascal trap void C_ShowHide( WindowPtr w, BOOLEAN flag ); extern pascal trap void P_ShowHide( WindowPtr w, BOOLEAN flag); 
extern pascal trap void C_HideWindow( WindowPtr w ); extern pascal trap void P_HideWindow( WindowPtr w); 
extern pascal trap void C_ShowWindow( WindowPtr w ); extern pascal trap void P_ShowWindow( WindowPtr w); 
extern pascal trap void C_SendBehind( WindowPtr w, WindowPtr behind ); extern pascal trap void P_SendBehind( WindowPtr w, WindowPtr behind); 
extern pascal trap void C_DrawGrowIcon( WindowPtr w ); extern pascal trap void P_DrawGrowIcon( WindowPtr w); 
extern pascal trap void C_InitWindows( void  ); extern pascal trap void P_InitWindows( void ); 
extern pascal trap void C_GetWMgrPort( HIDDEN_GrafPtr *wp ); extern pascal trap void P_GetWMgrPort( HIDDEN_GrafPtr *wp); 
extern pascal trap WindowPtr C_NewWindow( Ptr wst, Rect *r, 
 StringPtr title, BOOLEAN vis, INTEGER procid, WindowPtr behind, 
 BOOLEAN gaflag, LONGINT rc );extern pascal trap WindowPtr P_NewWindow( Ptr wst, Rect *r, 
 StringPtr title, BOOLEAN vis, INTEGER procid, WindowPtr behind, 
 BOOLEAN gaflag, LONGINT rc ); 
extern pascal trap WindowPtr C_GetNewWindow( INTEGER wid, Ptr wst, 
 WindowPtr behind ); extern pascal trap WindowPtr P_GetNewWindow( INTEGER wid, Ptr wst, 
 WindowPtr behind ); 
extern pascal trap void C_CloseWindow( WindowPtr w ); extern pascal trap void P_CloseWindow( WindowPtr w); 
extern pascal trap void C_DisposeWindow( WindowPtr w ); extern pascal trap void P_DisposeWindow( WindowPtr w); 
extern pascal trap void C_SetWRefCon( WindowPtr w, LONGINT data ); extern pascal trap void P_SetWRefCon( WindowPtr w, LONGINT data); 
extern pascal trap LONGINT C_GetWRefCon( WindowPtr w ); extern pascal trap LONGINT P_GetWRefCon( WindowPtr w); 
extern pascal trap void C_SetWindowPic( WindowPtr w, PicHandle p ); extern pascal trap void P_SetWindowPic( WindowPtr w, PicHandle p); 
extern pascal trap PicHandle C_GetWindowPic( WindowPtr w ); extern pascal trap PicHandle P_GetWindowPic( WindowPtr w); 
extern pascal trap LONGINT C_PinRect( Rect *r, Point p ); extern pascal trap LONGINT P_PinRect( Rect *r, Point p); 
extern pascal trap LONGINT C_DragTheRgn( RgnHandle rgn, Point startp, 
 Rect *limit, Rect *slop, INTEGER axis, ProcPtr proc ); extern pascal trap LONGINT P_DragTheRgn( RgnHandle rgn, Point startp, 
 Rect *limit, Rect *slop, INTEGER axis, ProcPtr proc ); 
extern pascal trap LONGINT C_DragGrayRgn( RgnHandle rgn, Point startp, 
 Rect *limit, Rect *slop, INTEGER axis, ProcPtr proc ); extern pascal trap LONGINT P_DragGrayRgn( RgnHandle rgn, Point startp, 
 Rect *limit, Rect *slop, INTEGER axis, ProcPtr proc ); 
extern pascal trap void C_ClipAbove( WindowPeek w ); extern pascal trap void P_ClipAbove( WindowPeek w); 
extern pascal trap BOOLEAN C_CheckUpdate( EventRecord *ev ); extern pascal trap BOOLEAN P_CheckUpdate( EventRecord *ev); 
extern pascal trap void C_SaveOld( WindowPeek w ); extern pascal trap void P_SaveOld( WindowPeek w); 
extern pascal trap void C_PaintOne( WindowPeek w, RgnHandle clobbered ); extern pascal trap void P_PaintOne( WindowPeek w, RgnHandle clobbered); 
extern pascal trap void C_PaintBehind( WindowPeek w, RgnHandle clobbered ); extern pascal trap void P_PaintBehind( WindowPeek w, RgnHandle clobbered); 
extern pascal trap void C_CalcVis( WindowPeek w ); extern pascal trap void P_CalcVis( WindowPeek w); 
extern pascal trap void C_CalcVisBehind( WindowPeek w, 
 RgnHandle clobbered ); extern pascal trap void P_CalcVisBehind( WindowPeek w, 
 RgnHandle clobbered ); 
extern pascal trap void C_DrawNew( WindowPeek w, BOOLEAN flag ); extern pascal trap void P_DrawNew( WindowPeek w, BOOLEAN flag); 
extern pascal trap INTEGER C_GetWVariant( WindowPtr w ); extern pascal trap INTEGER P_GetWVariant( WindowPtr w); 
extern pascal trap INTEGER C_FindWindow( Point p, HIDDEN_WindowPtr *wpp ); extern pascal trap INTEGER P_FindWindow( Point p, HIDDEN_WindowPtr *wpp); 
extern pascal trap BOOLEAN C_TrackBox( WindowPtr wp, 
 Point pt, INTEGER part ); extern pascal trap BOOLEAN P_TrackBox( WindowPtr wp, 
 Point pt, INTEGER part ); 
extern pascal trap BOOLEAN C_TrackGoAway( WindowPtr w, Point p ); extern pascal trap BOOLEAN P_TrackGoAway( WindowPtr w, Point p); 
extern pascal trap void C_ZoomWindow( WindowPtr wp, 
 INTEGER part, BOOLEAN front ); extern pascal trap void P_ZoomWindow( WindowPtr wp, 
 INTEGER part, BOOLEAN front ); 
extern pascal trap void C_MoveWindow( WindowPtr wp, INTEGER h, INTEGER v, 
 BOOLEAN front ); extern pascal trap void P_MoveWindow( WindowPtr wp, INTEGER h, INTEGER v, 
 BOOLEAN front ); 
extern pascal trap void C_DragWindow( WindowPtr wp, Point p, Rect *rp ); extern pascal trap void P_DragWindow( WindowPtr wp, Point p, Rect *rp); 
extern pascal trap LONGINT C_GrowWindow( WindowPtr w, Point startp, 
 Rect *rp ); extern pascal trap LONGINT P_GrowWindow( WindowPtr w, Point startp, 
 Rect *rp ); 
extern pascal trap void C_SizeWindow( WindowPtr w, INTEGER width, 
 INTEGER height, BOOLEAN flag ); extern pascal trap void P_SizeWindow( WindowPtr w, INTEGER width, 
 INTEGER height, BOOLEAN flag ); 
extern pascal trap void C_InvalRect( Rect *r ); extern pascal trap void P_InvalRect( Rect *r); 
extern pascal trap void C_InvalRgn( RgnHandle r ); extern pascal trap void P_InvalRgn( RgnHandle r); 
extern pascal trap void C_ValidRect( Rect *r ); extern pascal trap void P_ValidRect( Rect *r); 
extern pascal trap void C_ValidRgn( RgnHandle r ); extern pascal trap void P_ValidRgn( RgnHandle r); 
extern pascal trap void C_BeginUpdate( WindowPtr w ); extern pascal trap void P_BeginUpdate( WindowPtr w); 
extern pascal trap void C_EndUpdate( WindowPtr w ); extern pascal trap void P_EndUpdate( WindowPtr w);
extern pascal trap void C_SetWinColor (WindowPtr w, CTabHandle new_w_ctab);
extern pascal trap void C_SetDeskCPat( PixPatHandle );
extern pascal trap void P_SetDeskCPat( PixPatHandle );
#endif /* __STDC__ */
#endif /* __WINDOW__ */
