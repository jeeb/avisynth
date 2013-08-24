#include "pfc.h"

#ifndef PFC_BYTE_ORDER_IS_BIG_ENDIAN

bool byte_order_helper::machine_is_big_endian()
{
	BYTE temp[4];
	*(DWORD*)temp = 0xDEADBEEF;
	return temp[0]==0xDE;
}

#endif

void byte_order_helper::swap_order(void * p_ptr,unsigned bytes)
{
	BYTE * ptr = (BYTE*)p_ptr;
	BYTE t;
	unsigned n;
	for(n=0;n<bytes>>1;n++)
	{
		t = ptr[n];
		ptr[n] = ptr[bytes-n-1];
		ptr[bytes-n-1] = t;
	}
}

#ifdef PFC_BYTE_ORDER_IS_BIG_ENDIAN

void byte_order_helper::order_be_to_native(void * ptr,unsigned bytes)
{
#if !PFC_BYTE_ORDER_IS_BIG_ENDIAN
	swap_order(ptr,bytes);
#endif
}

void byte_order_helper::order_le_to_native(void * ptr,unsigned bytes)
{
#if PFC_BYTE_ORDER_IS_BIG_ENDIAN
	swap_order(ptr,bytes);
#endif
}

void byte_order_helper::order_native_to_be(void * ptr,unsigned bytes)
{
#if !PFC_BYTE_ORDER_IS_BIG_ENDIAN
	swap_order(ptr,bytes);
#endif
}

void byte_order_helper::order_native_to_le(void * ptr,unsigned bytes)
{
#if PFC_BYTE_ORDER_IS_BIG_ENDIAN
	swap_order(ptr,bytes);
#endif
}

#else

void byte_order_helper::order_be_to_native(void * ptr,unsigned bytes)
{
	if (!machine_is_big_endian()) swap_order(ptr,bytes);
}

void byte_order_helper::order_le_to_native(void * ptr,unsigned bytes)
{
	if (machine_is_big_endian()) swap_order(ptr,bytes);
}

void byte_order_helper::order_native_to_be(void * ptr,unsigned bytes)
{
	if (!machine_is_big_endian()) swap_order(ptr,bytes);
}

void byte_order_helper::order_native_to_le(void * ptr,unsigned bytes)
{
	if (machine_is_big_endian()) swap_order(ptr,bytes);
}

#endif