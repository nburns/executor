#if !defined (__arch_arm64_h__)
#define __arch_arm64_h__

#define LITTLEENDIAN
#define SYN68K

#define swap16(v) ((uint16_t) __builtin_bswap16 ((uint16_t) (v)))
#define swap32(v) ((uint32_t) __builtin_bswap32 ((uint32_t) (v)))

#endif /* !defined (__arch_arm64_h__) */
