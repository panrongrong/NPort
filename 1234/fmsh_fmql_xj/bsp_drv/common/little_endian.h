/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_BYTEORDER_LITTLE_ENDIAN_H
#define _UAPI_LINUX_BYTEORDER_LITTLE_ENDIAN_H

#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __LITTLE_ENDIAN_BITFIELD
#define __LITTLE_ENDIAN_BITFIELD
#endif

#define __constant_htonl(x) (( UINT32)___constant_swab32((x)))
#define __constant_ntohl(x) ___constant_swab32(( UINT32)(x))
#define __constant_htons(x) (( UINT16)___constant_swab16((x)))
#define __constant_ntohs(x) ___constant_swab16(( UINT16)(x))
#define __constant_cpu_to_le64(x) (( __le64)(__u64)(x))
#define __constant_le64_to_cpu(x) (( __u64)(__le64)(x))
#define __constant_cpu_to_le32(x) (( UINT32)(UINT32)(x))
#define __constant_le32_to_cpu(x) (( UINT32)(UINT32)(x))
#define __constant_cpu_to_le16(x) (( __le16)(UINT16)(x))
#define __constant_le16_to_cpu(x) (( UINT16)(__le16)(x))
#define __constant_cpu_to_be64(x) (( __be64)___constant_swab64((x)))
#define __constant_be64_to_cpu(x) ___constant_swab64(( __u64)(__be64)(x))
#define __constant_cpu_to_be32(x) (( UINT32)___constant_swab32((x)))
#define __constant_be32_to_cpu(x) ___constant_swab32(( UINT32)(UINT32)(x))
#define __constant_cpu_to_be16(x) (( UINT16)___constant_swab16((x)))
#define __constant_be16_to_cpu(x) ___constant_swab16(( UINT16)(UINT16)(x))
#define __cpu_to_le64(x) (( __le64)(__u64)(x))
#define __le64_to_cpu(x) (( __u64)(__le64)(x))
#define __cpu_to_le32(x) (( UINT32)(UINT32)(x))
#define le32_to_cpu(x) (( UINT32)(UINT32)(x))
#define __cpu_to_le16(x) (( __le16)(UINT16)(x))
#define __le16_to_cpu(x) (( UINT16)(__le16)(x))
#define __cpu_to_be64(x) (( __be64)__swab64((x)))
#define __be64_to_cpu(x) __swab64(( __u64)(__be64)(x))
#define __cpu_to_be32(x) (( UINT32)__swab32((x)))
#define __be32_to_cpu(x) __swab32(( UINT32)(UINT32)(x))
#define __cpu_to_be16(x) (( UINT16)__swab16((x)))
#define __be16_to_cpu(x) __swab16(( UINT16)(UINT16)(x))
#if 0
static  __le64 __cpu_to_le64p(const __u64 *p)
{
	return ( __le64)*p;
}
static  __u64 __le64_to_cpup(const __le64 *p)
{
	return ( __u64)*p;
}
static  UINT32 __cpu_to_le32p(const UINT32 *p)
{
	return ( UINT32)*p;
}
static  UINT32 __le32_to_cpup(const UINT32 *p)
{
	return ( UINT32)*p;
}
static  __le16 __cpu_to_le16p(const UINT16 *p)
{
	return ( __le16)*p;
}
static  UINT16 __le16_to_cpup(const __le16 *p)
{
	return ( UINT16)*p;
}
static  __be64 __cpu_to_be64p(const __u64 *p)
{
	return ( __be64)__swab64p(p);
}
static  __u64 __be64_to_cpup(const __be64 *p)
{
	return __swab64p((__u64 *)p);
}
static  UINT32 __cpu_to_be32p(const UINT32 *p)
{
	return ( UINT32)__swab32p(p);
}
static  UINT32 __be32_to_cpup(const UINT32 *p)
{
	return __swab32p((UINT32 *)p);
}
static  UINT16 __cpu_to_be16p(const UINT16 *p)
{
	return ( UINT16)__swab16p(p);
}
static  UINT16 __be16_to_cpup(const UINT16 *p)
{
	return __swab16p((UINT16 *)p);
}
#define __cpu_to_le64s(x) do { (void)(x); } while (0)
#define __le64_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_le32s(x) do { (void)(x); } while (0)
#define __le32_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_le16s(x) do { (void)(x); } while (0)
#define __le16_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_be64s(x) __swab64s((x))
#define __be64_to_cpus(x) __swab64s((x))
#define __cpu_to_be32s(x) __swab32s((x))
#define __be32_to_cpus(x) __swab32s((x))
#define __cpu_to_be16s(x) __swab16s((x))
#define __be16_to_cpus(x) __swab16s((x))
#endif

#endif /* _UAPI_LINUX_BYTEORDER_LITTLE_ENDIAN_H */
