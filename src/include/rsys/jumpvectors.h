#if !defined(__RSYS_JUMPVECTORS__)
#define __RSYS_JUMPVECTORS__

#if !defined (JFLUSH_H)
extern HIDDEN_ProcPtr JFLUSH_H, JResUnknown1_H, JResUnknown2_H;
#endif

#if (SIZEOF_CHAR_P == 4) && !FORCE_EXPERIMENTAL_PACKED_MACROS
# define JFLUSH      (JFLUSH_H.p)
# define JResUnknown1 (JResUnknown1_H.p)
# define JResUnknown2 (JResUnknown2_H.p)
#else
# define JFLUSH      ((ProcPtr) PPR(JFLUSH_H))
# define JResUnknown1 ((ProcPtr) PPR(JResUnknown1_H))
# define JResUnknown2 ((ProcPtr) PPR(JResUnknown2_H))
#endif

#endif /* !defined(__RSYS_JUMPVECTORS__) */
