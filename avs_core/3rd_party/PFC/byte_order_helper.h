#ifndef _PFC_BYTE_ORDER_HELPER_
#define _PFC_BYTE_ORDER_HELPER_

namespace byte_order_helper {

#ifdef _M_IX86
#define PFC_BYTE_ORDER_IS_BIG_ENDIAN 0
#endif

#ifdef PFC_BYTE_ORDER_IS_BIG_ENDIAN
#define PFC_BYTE_ORDER_IS_LITTLE_ENDIAN (!(PFC_BYTE_ORDER_IS_BIG_ENDIAN))
#endif
	//if not defined, use machine_is_big_endian() to detect in runtime

#ifndef PFC_BYTE_ORDER_IS_BIG_ENDIAN
	bool machine_is_big_endian();
#else
	inline bool machine_is_big_endian() {return PFC_BYTE_ORDER_IS_BIG_ENDIAN;}
#endif

	inline bool machine_is_little_endian() {return !machine_is_big_endian();}

	void swap_order(void * ptr,unsigned bytes);
	void order_be_to_native(void * ptr,unsigned bytes);
	void order_le_to_native(void * ptr,unsigned bytes);
	void order_native_to_be(void * ptr,unsigned bytes);
	void order_native_to_le(void * ptr,unsigned bytes);

	inline DWORD swap_dword(DWORD param) {swap_order(&param,sizeof(param));return param;}
	inline WORD swap_word(WORD param) {swap_order(&param,sizeof(param));return param;}

	inline DWORD dword_be_to_native(DWORD param) {order_be_to_native(&param,sizeof(param));return param;}
	inline DWORD dword_le_to_native(DWORD param) {order_le_to_native(&param,sizeof(param));return param;}
	inline DWORD dword_native_to_be(DWORD param) {order_native_to_be(&param,sizeof(param));return param;}
	inline DWORD dword_native_to_le(DWORD param) {order_native_to_le(&param,sizeof(param));return param;}

	inline WORD word_be_to_native(WORD param) {order_be_to_native(&param,sizeof(param));return param;}
	inline WORD word_le_to_native(WORD param) {order_le_to_native(&param,sizeof(param));return param;}
	inline WORD word_native_to_be(WORD param) {order_native_to_be(&param,sizeof(param));return param;}
	inline WORD word_native_to_le(WORD param) {order_native_to_le(&param,sizeof(param));return param;}
};

#endif
