#include "CodeGenerator.hpp"

#include "Error.hpp"

#include <stdio.h>

namespace SoftWire
{
	int CodeGenerator::Variable::stack = 4;

	CodeGenerator::Allocation CodeGenerator::EAX;
	CodeGenerator::Allocation CodeGenerator::ECX;
	CodeGenerator::Allocation CodeGenerator::EDX;
	CodeGenerator::Allocation CodeGenerator::EBX;
	CodeGenerator::Allocation CodeGenerator::ESI;
	CodeGenerator::Allocation CodeGenerator::EDI;

	CodeGenerator::Allocation CodeGenerator::MM0;
	CodeGenerator::Allocation CodeGenerator::MM1;
	CodeGenerator::Allocation CodeGenerator::MM2;
	CodeGenerator::Allocation CodeGenerator::MM3;
	CodeGenerator::Allocation CodeGenerator::MM4;
	CodeGenerator::Allocation CodeGenerator::MM5;
	CodeGenerator::Allocation CodeGenerator::MM6;
	CodeGenerator::Allocation CodeGenerator::MM7;

	CodeGenerator::Allocation CodeGenerator::XMM0;
	CodeGenerator::Allocation CodeGenerator::XMM1;
	CodeGenerator::Allocation CodeGenerator::XMM2;
	CodeGenerator::Allocation CodeGenerator::XMM3;
	CodeGenerator::Allocation CodeGenerator::XMM4;
	CodeGenerator::Allocation CodeGenerator::XMM5;
	CodeGenerator::Allocation CodeGenerator::XMM6;
	CodeGenerator::Allocation CodeGenerator::XMM7;

	float CodeGenerator::sse[8][4];   // Storage for SSE emulation registers

	bool CodeGenerator::emulateSSE = false;
	bool CodeGenerator::copyPropagation = true;
	bool CodeGenerator::dropUnmodified = true;
	bool CodeGenerator::spillUnrelocated = false;
	bool CodeGenerator::loadLoadElimination = true;

	CodeGenerator *CodeGenerator::active = 0;

	CodeGenerator::Variable::~Variable()
	{
		activeCG()->free((OperandREF)(esp-reference));

		stack = reference;
	}

	CodeGenerator::Byte::Byte()
	{
		reference = stack;
		stack += 1;
	}

	CodeGenerator::Byte::operator OperandREG8() const
	{
		return activeCG()->r8(esp-reference);
	}

	CodeGenerator::Word::Word()
	{
		reference = stack;
		stack = (stack + 3) & ~1;
	}

	CodeGenerator::Word::operator OperandREG16() const
	{
		return activeCG()->r16(esp-reference);
	}

	CodeGenerator::Dword::Dword()
	{
		reference = stack;
		stack = (stack + 7) & ~3;
	}

	CodeGenerator::Dword::operator OperandREG32() const
	{
		return activeCG()->r32(esp-reference);
	}

	CodeGenerator::Qword::Qword()
	{
		reference = stack;
		stack = (stack + 15) & ~7;
	}

	CodeGenerator::Qword::operator OperandMMREG() const
	{
		return activeCG()->r64(esp-reference);
	}

	CodeGenerator::Float::Float()
	{
		reference = stack;
		stack = (stack + 7) & ~3;
	}

	CodeGenerator::Float::operator OperandXMMREG() const
	{
		return activeCG()->rSS(esp-reference);
	}

	CodeGenerator::Float4::Float4()
	{
		reference = stack;
		stack = (stack + 31) & ~15;
	}

	CodeGenerator::Float4::operator OperandXMMREG() const
	{
		return activeCG()->r128(esp-reference);
	}

	CodeGenerator::CodeGenerator()
	{
		freeAll();
		active = 0;
	}

	CodeGenerator::~CodeGenerator()
	{
		freeAll();
		active = 0;
	}

	const OperandREG8 CodeGenerator::r8(const OperandREF &ref, bool copy)
	{
		OperandREG32 reg = r32(ref, copy);

		// Make sure we only have al, cl, dl or bl
		if(reg.reg == Encoding::ESI ||
		   reg.reg == Encoding::EDI)
		{
			spill(reg);

			// Need to spill one of al, cl, dl or bl
			Encoding::Reg candidate = Encoding::REG_UNKNOWN;
			unsigned int priority = 0xFFFFFFFF - 2;   // Don't spill most recently used

			if(EAX.priority < priority && real(EAX.reference)) {priority = EAX.priority; candidate = Encoding::EAX;}
			if(ECX.priority < priority && real(ECX.reference)) {priority = ECX.priority; candidate = Encoding::ECX;}
			if(EDX.priority < priority && real(EDX.reference)) {priority = EDX.priority; candidate = Encoding::EDX;}
			if(EBX.priority < priority && real(EBX.reference)) {priority = EBX.priority; candidate = Encoding::EBX;}

			if(candidate == Encoding::REG_UNKNOWN)
			{
				throw Error("Out of physical general purpose registers. Use free().");
			}

			spill(R32(candidate));

			return (OperandREG8)assign(R32(candidate), ref, copy, 1);
		}

		return (OperandREG8)reg;
	}

	const OperandREG8 CodeGenerator::x8(const OperandREF &ref, bool copy)
	{
		free(ref);
		return r8(ref, copy);
	}

	const OperandREG8 CodeGenerator::t8(int i)
	{
		return x8((OperandREF)i);
	}

	const OperandR_M8 CodeGenerator::m8(const OperandREF &ref)
	{
		return (OperandR_M8)m32(ref, 1);
	}

	const OperandREG16 CodeGenerator::r16(const OperandREF &ref, bool copy)
	{
		return (OperandREG16)r32(ref, copy, 2);
	}

	const OperandREG16 CodeGenerator::x16(const OperandREF &ref, bool copy)
	{
		return (OperandREG16)x32(ref, copy, 2);
	}

	const OperandREG16 CodeGenerator::t16(int i)
	{
		return (OperandREG16)t32(i, 2);
	}

	const OperandR_M16 CodeGenerator::m16(const OperandREF &ref)
	{
		return (OperandR_M16)m32(ref, 2);
	}

	const OperandREG32 &CodeGenerator::r32(const OperandREF &ref, bool copy, int partial)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(EAX.reference == ref) {if(EAX.copy) EAX.copy->retain(); EAX.copy = 0; return access(eax);}
		else if(ECX.reference == ref) {if(ECX.copy) ECX.copy->retain(); ECX.copy = 0; return access(ecx);}
		else if(EDX.reference == ref) {if(EDX.copy) EDX.copy->retain(); EDX.copy = 0; return access(edx);}
		else if(EBX.reference == ref) {if(EBX.copy) EBX.copy->retain(); EBX.copy = 0; return access(ebx);}
		else if(ESI.reference == ref) {if(ESI.copy) ESI.copy->retain(); ESI.copy = 0; return access(esi);}
		else if(EDI.reference == ref) {if(EDI.copy) EDI.copy->retain(); EDI.copy = 0; return access(edi);}

		// Search for free registers
		     if(EAX.reference == 0 && EAX.priority == 0) return assign(eax, ref, copy, partial);
		else if(ECX.reference == 0 && ECX.priority == 0) return assign(ecx, ref, copy, partial);
		else if(EDX.reference == 0 && EDX.priority == 0) return assign(edx, ref, copy, partial);
		else if(EBX.reference == 0 && EBX.priority == 0) return assign(ebx, ref, copy, partial);
		else if(ESI.reference == 0 && ESI.priority == 0) return assign(esi, ref, copy, partial);
		else if(EDI.reference == 0 && EDI.priority == 0) return assign(edi, ref, copy, partial);

		// Need to spill one
		Encoding::Reg candidate = Encoding::REG_UNKNOWN;
		unsigned int priority = 0xFFFFFFFF - 2;   // Don't spill most recently used

		if(EAX.priority < priority && real(EAX.reference)) {priority = EAX.priority; candidate = Encoding::EAX;}
		if(ECX.priority < priority && real(ECX.reference)) {priority = ECX.priority; candidate = Encoding::ECX;}
		if(EDX.priority < priority && real(EDX.reference)) {priority = EDX.priority; candidate = Encoding::EDX;}
		if(EBX.priority < priority && real(EBX.reference)) {priority = EBX.priority; candidate = Encoding::EBX;}
		if(ESI.priority < priority && real(ESI.reference)) {priority = ESI.priority; candidate = Encoding::ESI;}
		if(EDI.priority < priority && real(EDI.reference)) {priority = EDI.priority; candidate = Encoding::EDI;}

		if(candidate == Encoding::REG_UNKNOWN)
		{
			throw Error("Out of physical general purpose registers. Use free().");
		}

		spill(R32(candidate));

		return assign(R32(candidate), ref, copy, partial);
	}

	const OperandREG32 &CodeGenerator::x32(const OperandREF &ref, bool copy, int partial)
	{
		free(ref);
		return r32(ref, copy, partial);
	}

	const OperandREG32 &CodeGenerator::t32(int i, int partial)
	{
		if(i < 0 || i >= 6) throw Error("Register allocator t32 index out of range");

		return x32((OperandREF)i, false, partial);
	}

	const OperandR_M32 CodeGenerator::m32(const OperandREF &ref, int partial)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(EAX.reference == ref) return access(eax);
		else if(ECX.reference == ref) return access(ecx);
		else if(EDX.reference == ref) return access(edx);
		else if(EBX.reference == ref) return access(ebx);
		else if(ESI.reference == ref) return access(esi);
		else if(EDI.reference == ref) return access(edi);

		return (OperandR_M32)dword_ptr [ref];
	}

	const OperandREG32 &CodeGenerator::allocate(const OperandREG32 &reg, const OperandREF &ref, int partial)
	{
		if(X32(reg).reference != 0)
		{
			throw Error("%s not available for register allocation", reg.string());
		}

		X32(reg).reference = ref;
		X32(reg).partial = partial;
		X32(reg).modified = false;

		return access(reg);
	}

	const OperandREG32 &CodeGenerator::assign(const OperandREG32 &reg, const OperandREF &ref, bool copy, int partial)
	{
		allocate(reg, ref, partial);

		if(copy && real(ref))
		{
			     if(partial == 1) mov((OperandREG8)reg, byte_ptr [ref]);
			else if(partial == 2) mov((OperandREG16)reg, word_ptr [ref]); 
			else                  mov(reg, dword_ptr [ref]);
		}

		return reg;
	}

	const OperandREG32 &CodeGenerator::access(const OperandREG32 &reg)
	{
		// Decrease priority of other registers
		if(reg.reg != Encoding::EAX && EAX.priority) EAX.priority--;
		if(reg.reg != Encoding::ECX && ECX.priority) ECX.priority--;
		if(reg.reg != Encoding::EDX && EDX.priority) EDX.priority--;
		if(reg.reg != Encoding::EBX && EBX.priority) EBX.priority--;
		if(reg.reg != Encoding::ESI && ESI.priority) ESI.priority--;
		if(reg.reg != Encoding::EDI && EDI.priority) EDI.priority--;

		// Give highest priority
		X32(reg).priority = 0xFFFFFFFF;

		return reg;
	}

	void CodeGenerator::free(const OperandREG32 &reg)
	{
		X32(reg).free();
	}

	void CodeGenerator::spill(const OperandREG32 &reg)
	{
		if(X32(reg).modified || !dropUnmodified)
		{
			if(X32(reg).copy) X32(reg).copy->retain();
			if(X32(reg).load) X32(reg).load->retain();

			if(real(X32(reg).reference))
			{
				     if(X32(reg).partial == 1) mov(byte_ptr [X32(reg).reference], (OperandREG8)reg);
				else if(X32(reg).partial == 2) mov(word_ptr [X32(reg).reference], (OperandREG16)reg);
				else                           mov(dword_ptr [X32(reg).reference], reg);
			}
		}

		free(reg);
	}

	const OperandMMREG &CodeGenerator::r64(const OperandREF &ref, bool copy)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(MM0.reference == ref) {if(MM0.copy) MM0.copy->retain(); MM0.copy = 0; return access(mm0);}
		else if(MM1.reference == ref) {if(MM1.copy) MM1.copy->retain(); MM1.copy = 0; return access(mm1);}
		else if(MM2.reference == ref) {if(MM2.copy) MM2.copy->retain(); MM2.copy = 0; return access(mm2);}
		else if(MM3.reference == ref) {if(MM3.copy) MM3.copy->retain(); MM3.copy = 0; return access(mm3);}
		else if(MM4.reference == ref) {if(MM4.copy) MM4.copy->retain(); MM4.copy = 0; return access(mm4);}
		else if(MM5.reference == ref) {if(MM5.copy) MM5.copy->retain(); MM5.copy = 0; return access(mm5);}
		else if(MM6.reference == ref) {if(MM6.copy) MM6.copy->retain(); MM6.copy = 0; return access(mm6);}
		else if(MM7.reference == ref) {if(MM7.copy) MM7.copy->retain(); MM7.copy = 0; return access(mm7);}

		// Search for free registers
		     if(MM0.reference == 0 && MM0.priority == 0) return assign(mm0, ref, copy);
		else if(MM1.reference == 0 && MM1.priority == 0) return assign(mm1, ref, copy);
		else if(MM2.reference == 0 && MM2.priority == 0) return assign(mm2, ref, copy);
		else if(MM3.reference == 0 && MM3.priority == 0) return assign(mm3, ref, copy);
		else if(MM4.reference == 0 && MM4.priority == 0) return assign(mm4, ref, copy);
		else if(MM5.reference == 0 && MM5.priority == 0) return assign(mm5, ref, copy);
		else if(MM6.reference == 0 && MM6.priority == 0) return assign(mm6, ref, copy);
		else if(MM7.reference == 0 && MM7.priority == 0) return assign(mm7, ref, copy);

		// Need to spill one
		Encoding::Reg candidate = Encoding::REG_UNKNOWN;
		unsigned int priority = 0xFFFFFFFF - 2;   // Don't spill most recently used

		if(MM0.priority < priority && real(MM0.reference)) {priority = MM0.priority; candidate = Encoding::MM0;}
		if(MM1.priority < priority && real(MM1.reference)) {priority = MM1.priority; candidate = Encoding::MM1;}
		if(MM2.priority < priority && real(MM2.reference)) {priority = MM2.priority; candidate = Encoding::MM2;}
		if(MM3.priority < priority && real(MM3.reference)) {priority = MM3.priority; candidate = Encoding::MM3;}
		if(MM4.priority < priority && real(MM4.reference)) {priority = MM4.priority; candidate = Encoding::MM4;}
		if(MM5.priority < priority && real(MM5.reference)) {priority = MM5.priority; candidate = Encoding::MM5;}
		if(MM6.priority < priority && real(MM6.reference)) {priority = MM6.priority; candidate = Encoding::MM6;}
		if(MM7.priority < priority && real(MM7.reference)) {priority = MM7.priority; candidate = Encoding::MM7;}

		if(candidate == Encoding::REG_UNKNOWN)
		{
			throw Error("Out of physical MMX registers. Use free().");
		}

		spill(R64(candidate));

		return assign(R64(candidate), ref, copy);
	}

	const OperandMMREG &CodeGenerator::x64(const OperandREF &ref, bool copy)
	{
		free(ref);
		return r64(ref, copy);
	}

	const OperandMMREG &CodeGenerator::t64(int i)
	{
		if(i < 0 || i >= 8) throw Error("Register allocator t64 index out of range");

		return x64((OperandREF)i);
	}

	const OperandR_M64 CodeGenerator::m64(const OperandREF &ref)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(MM0.reference == ref) return access(mm0);
		else if(MM1.reference == ref) return access(mm1);
		else if(MM2.reference == ref) return access(mm2);
		else if(MM3.reference == ref) return access(mm3);
		else if(MM4.reference == ref) return access(mm4);
		else if(MM5.reference == ref) return access(mm5);
		else if(MM6.reference == ref) return access(mm6);
		else if(MM7.reference == ref) return access(mm7);

		return (OperandR_M64)qword_ptr [ref];
	}

	const OperandMMREG &CodeGenerator::allocate(const OperandMMREG &reg, const OperandREF &ref)
	{
		if(X64(reg).reference != 0)
		{
			throw Error("%s not available for register allocation", reg.string());
		}

		X64(reg).reference = ref;
		X64(reg).modified = false;

		return access(reg);
	}

	const OperandMMREG &CodeGenerator::assign(const OperandMMREG &reg, const OperandREF &ref, bool copy)
	{
		allocate(reg, ref);

		if(copy && real(ref))
		{
			movq(reg, qword_ptr [ref]);
		}

		return reg;
	}

	const OperandMMREG &CodeGenerator::access(const OperandMMREG &reg)
	{
		// Decrease priority of other registers
		if(reg.reg != Encoding::MM0 && MM0.priority) MM0.priority--;
		if(reg.reg != Encoding::MM1 && MM1.priority) MM1.priority--;
		if(reg.reg != Encoding::MM2 && MM2.priority) MM2.priority--;
		if(reg.reg != Encoding::MM3 && MM3.priority) MM3.priority--;
		if(reg.reg != Encoding::MM4 && MM4.priority) MM4.priority--;
		if(reg.reg != Encoding::MM5 && MM5.priority) MM5.priority--;
		if(reg.reg != Encoding::MM6 && MM6.priority) MM6.priority--;
		if(reg.reg != Encoding::MM7 && MM7.priority) MM7.priority--;

		// Give highest priority
		X64(reg).priority = 0xFFFFFFFF;

		return reg;
	}

	void CodeGenerator::free(const OperandMMREG &reg)
	{
		bool free = (X64(reg).priority != 0);

		X64(reg).free();

		if(emulateSSE && free)
		{
			if(!MM0.priority &&
			   !MM1.priority &&
			   !MM2.priority &&
			   !MM3.priority &&
			   !MM4.priority &&
			   !MM5.priority &&
			   !MM6.priority &&
			   !MM7.priority)
			{
				emms();
			}
		}
	}

	void CodeGenerator::spill(const OperandMMREG &reg)
	{
		if(X64(reg).modified || !dropUnmodified)
		{
			if(X64(reg).copy) X64(reg).copy->retain();
			if(X64(reg).load) X64(reg).load->retain();

			if(real(X64(reg).reference))
			{
				movq(qword_ptr [X64(reg).reference], reg);
			}
		}

		free(reg);
	}

	const OperandXMMREG &CodeGenerator::r128(const OperandREF &ref, bool copy, bool ss)
	{
		if(ref == 0 && copy) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(XMM0.reference == ref) {if(XMM0.copy) XMM0.copy->retain(); XMM0.copy = 0; return access(xmm0);}
		else if(XMM1.reference == ref) {if(XMM1.copy) XMM1.copy->retain(); XMM1.copy = 0; return access(xmm1);}
		else if(XMM2.reference == ref) {if(XMM2.copy) XMM2.copy->retain(); XMM2.copy = 0; return access(xmm2);}
		else if(XMM3.reference == ref) {if(XMM3.copy) XMM3.copy->retain(); XMM3.copy = 0; return access(xmm3);}
		else if(XMM4.reference == ref) {if(XMM4.copy) XMM4.copy->retain(); XMM4.copy = 0; return access(xmm4);}
		else if(XMM5.reference == ref) {if(XMM5.copy) XMM5.copy->retain(); XMM5.copy = 0; return access(xmm5);}
		else if(XMM6.reference == ref) {if(XMM6.copy) XMM6.copy->retain(); XMM6.copy = 0; return access(xmm6);}
		else if(XMM7.reference == ref) {if(XMM7.copy) XMM7.copy->retain(); XMM7.copy = 0; return access(xmm7);}

		// Search for free registers
		     if(XMM0.reference == 0 && XMM0.priority == 0) return assign(xmm0, ref, copy, ss);
		else if(XMM1.reference == 0 && XMM1.priority == 0) return assign(xmm1, ref, copy, ss);
		else if(XMM2.reference == 0 && XMM2.priority == 0) return assign(xmm2, ref, copy, ss);
		else if(XMM3.reference == 0 && XMM3.priority == 0) return assign(xmm3, ref, copy, ss);
		else if(XMM4.reference == 0 && XMM4.priority == 0) return assign(xmm4, ref, copy, ss);
		else if(XMM5.reference == 0 && XMM5.priority == 0) return assign(xmm5, ref, copy, ss);
		else if(XMM6.reference == 0 && XMM6.priority == 0) return assign(xmm6, ref, copy, ss);
		else if(XMM7.reference == 0 && XMM7.priority == 0) return assign(xmm7, ref, copy, ss);

		// Need to spill one
		Encoding::Reg candidate = Encoding::REG_UNKNOWN;
		unsigned int priority = 0xFFFFFFFF - 2;   // Don't spill most recently used

		if(XMM0.priority < priority && real(XMM0.reference)) {priority = XMM0.priority; candidate = Encoding::XMM0;}
		if(XMM1.priority < priority && real(XMM1.reference)) {priority = XMM1.priority; candidate = Encoding::XMM1;}
		if(XMM2.priority < priority && real(XMM2.reference)) {priority = XMM2.priority; candidate = Encoding::XMM2;}
		if(XMM3.priority < priority && real(XMM3.reference)) {priority = XMM3.priority; candidate = Encoding::XMM3;}
		if(XMM4.priority < priority && real(XMM4.reference)) {priority = XMM4.priority; candidate = Encoding::XMM4;}
		if(XMM5.priority < priority && real(XMM5.reference)) {priority = XMM5.priority; candidate = Encoding::XMM5;}
		if(XMM6.priority < priority && real(XMM6.reference)) {priority = XMM6.priority; candidate = Encoding::XMM6;}
		if(XMM7.priority < priority && real(XMM7.reference)) {priority = XMM7.priority; candidate = Encoding::XMM7;}

		if(candidate == Encoding::REG_UNKNOWN)
		{
			throw Error("Out of physical SSE registers. Use free().");
		}

		spill(R128(candidate));

		return assign(R128(candidate), ref, copy, ss);
	}

	const OperandXMMREG &CodeGenerator::x128(const OperandREF &ref, bool copy, bool ss)
	{
		free(ref);
		return r128(ref, copy, ss);
	}

	const OperandXMMREG &CodeGenerator::t128(int i, bool ss)
	{
		if(i < 0 || i >= 8) throw Error("Register allocator t128 index out of range");

		return x128((OperandREF)i, ss);
	}

	const OperandR_M128 CodeGenerator::m128(const OperandREF &ref, bool ss)
	{
		if(ref == 0) throw Error("Cannot dereference 0");

		// Check if already allocated
		     if(XMM0.reference == ref) return access(xmm0);
		else if(XMM1.reference == ref) return access(xmm1);
		else if(XMM2.reference == ref) return access(xmm2);
		else if(XMM3.reference == ref) return access(xmm3);
		else if(XMM4.reference == ref) return access(xmm4);
		else if(XMM5.reference == ref) return access(xmm5);
		else if(XMM6.reference == ref) return access(xmm6);
		else if(XMM7.reference == ref) return access(xmm7);

		return (OperandR_M128)xword_ptr [ref];
	}

	const OperandXMMREG &CodeGenerator::allocate(const OperandXMMREG &reg, const OperandREF &ref, bool ss)
	{
		if(X128(reg).reference != 0)
		{
			throw Error("%s not available for register allocation", reg.string());
		}

		X128(reg).reference = ref;
		X128(reg).partial = ss ? 4 : 0;
		X128(reg).modified = false;

		return access(reg);
	}

	const OperandXMMREG &CodeGenerator::assign(const OperandXMMREG &reg, const OperandREF &ref, bool copy, bool ss)
	{
		allocate(reg, ref, ss);

		if(copy && real(ref))
		{
			if(ss) movss(reg, dword_ptr [ref]);
			else   movaps(reg, xword_ptr [ref]);
		}

		return reg;
	}

	const OperandXMMREG &CodeGenerator::access(const OperandXMMREG &reg)
	{
		// Decrease priority of other registers
		if(reg.reg != Encoding::XMM0 && XMM0.priority) XMM0.priority--;
		if(reg.reg != Encoding::XMM1 && XMM1.priority) XMM1.priority--;
		if(reg.reg != Encoding::XMM2 && XMM2.priority) XMM2.priority--;
		if(reg.reg != Encoding::XMM3 && XMM3.priority) XMM3.priority--;
		if(reg.reg != Encoding::XMM4 && XMM4.priority) XMM4.priority--;
		if(reg.reg != Encoding::XMM5 && XMM5.priority) XMM5.priority--;
		if(reg.reg != Encoding::XMM6 && XMM6.priority) XMM6.priority--;
		if(reg.reg != Encoding::XMM7 && XMM7.priority) XMM7.priority--;

		// Give highest priority
		X128(reg).priority = 0xFFFFFFFF;

		return reg;
	}

	void CodeGenerator::free(const OperandXMMREG &reg)
	{
		X128(reg).free();
	}

	void CodeGenerator::spill(const OperandXMMREG &reg)
	{
		if(X128(reg).modified || !dropUnmodified)
		{
			if(X128(reg).copy) X128(reg).copy->retain();
			if(X128(reg).load) X128(reg).load->retain();

			if(real(X128(reg).reference))
			{
				if(X128(reg).partial) movss(dword_ptr [X128(reg).reference], reg);
				else                  movaps(xword_ptr [X128(reg).reference], reg);
			}
		}

		free(reg);
	}

	const OperandXMMREG &CodeGenerator::rSS(const OperandREF &ref, bool copy, bool ss)
	{
		return r128(ref, copy, ss);
	}

	const OperandXMMREG &CodeGenerator::xSS(const OperandREF &ref, bool copy, bool ss)
	{
		return x128(ref, copy, ss);
	}

	const OperandXMMREG &CodeGenerator::tSS(int i, bool ss)
	{
		return t128(i, ss);
	}

	const OperandXMM32 CodeGenerator::mSS(const OperandREF &ref, bool ss)
	{
		return (OperandXMM32)m128(ref, ss);
	}

	bool CodeGenerator::real(const OperandREF &ref)
	{
		return ref.baseReg != Encoding::REG_UNKNOWN ||
		       ref.indexReg != Encoding::REG_UNKNOWN ||
		       ref.scale != 0 ||
		       ref.displacement >= 8;
	}

	const CodeGenerator::AllocationTable CodeGenerator::getAllocationState()
	{
		AllocationTable state;

		if(spillUnrelocated)
		{
			spillAll();
			return state;
		}

			state.eax = EAX;
			state.ecx = ECX;
			state.edx = EDX;
			state.ebx = EBX;
			state.esi = ESI;
			state.edi = EDI;

			state.mm0 = MM0;
			state.mm1 = MM1;
			state.mm2 = MM2;
			state.mm3 = MM3;
			state.mm4 = MM4;
			state.mm5 = MM5;
			state.mm6 = MM6;
			state.mm7 = MM7;

			state.xmm0 = XMM0;
			state.xmm1 = XMM1;
			state.xmm2 = XMM2;
			state.xmm3 = XMM3;
			state.xmm4 = XMM4;
			state.xmm5 = XMM5;
			state.xmm6 = XMM6;
			state.xmm7 = XMM7;

		return state;
	}

	void CodeGenerator::free(const OperandREF &ref)
	{
		     if(EAX.reference == ref) free(eax);
		else if(ECX.reference == ref) free(ecx);
		else if(EDX.reference == ref) free(edx);
		else if(EBX.reference == ref) free(ebx);
		else if(ESI.reference == ref) free(esi);
		else if(EDI.reference == ref) free(edi);

		     if(MM0.reference == ref) free(mm0);
		else if(MM1.reference == ref) free(mm1);
		else if(MM2.reference == ref) free(mm2);
		else if(MM3.reference == ref) free(mm3);
		else if(MM4.reference == ref) free(mm4);
		else if(MM5.reference == ref) free(mm5);
		else if(MM6.reference == ref) free(mm6);
		else if(MM7.reference == ref) free(mm7);

		     if(XMM0.reference == ref) free(xmm0);
		else if(XMM1.reference == ref) free(xmm1);
		else if(XMM2.reference == ref) free(xmm2);
		else if(XMM3.reference == ref) free(xmm3);
		else if(XMM4.reference == ref) free(xmm4);
		else if(XMM5.reference == ref) free(xmm5);
		else if(XMM6.reference == ref) free(xmm6);
		else if(XMM7.reference == ref) free(xmm7);
	}

	void CodeGenerator::spill(const OperandREF &ref)
	{
		     if(EAX.reference == ref) spill(eax);
		else if(ECX.reference == ref) spill(ecx);
		else if(EDX.reference == ref) spill(edx);
		else if(EBX.reference == ref) spill(ebx);
		else if(ESI.reference == ref) spill(esi);
		else if(EDI.reference == ref) spill(edi);

		     if(MM0.reference == ref) spill(mm0);
		else if(MM1.reference == ref) spill(mm1);
		else if(MM2.reference == ref) spill(mm2);
		else if(MM3.reference == ref) spill(mm3);
		else if(MM4.reference == ref) spill(mm4);
		else if(MM5.reference == ref) spill(mm5);
		else if(MM6.reference == ref) spill(mm6);
		else if(MM7.reference == ref) spill(mm7);

		     if(XMM0.reference == ref) spill(xmm0);
		else if(XMM1.reference == ref) spill(xmm1);
		else if(XMM2.reference == ref) spill(xmm2);
		else if(XMM3.reference == ref) spill(xmm3);
		else if(XMM4.reference == ref) spill(xmm4);
		else if(XMM5.reference == ref) spill(xmm5);
		else if(XMM6.reference == ref) spill(xmm6);
		else if(XMM7.reference == ref) spill(xmm7);

		free(ref);
	}

	void CodeGenerator::freeAll()
	{
		free(eax);
		free(ecx);
		free(edx);
		free(ebx);
		free(esi);
		free(edi);

		free(mm0);
		free(mm1);
		free(mm2);
		free(mm3);
		free(mm4);
		free(mm5);
		free(mm6);
		free(mm7);

		free(xmm0);
		free(xmm1);
		free(xmm2);
		free(xmm3);
		free(xmm4);
		free(xmm5);
		free(xmm6);
		free(xmm7);
	}

	void CodeGenerator::spillAll(const AllocationTable &allocationState)
	{
		if(spillUnrelocated)
		{
			spillAll();
			return;
		}

		if(EAX.reference != allocationState.eax.reference) spill(eax);
		if(ECX.reference != allocationState.ecx.reference) spill(ecx);
		if(EDX.reference != allocationState.edx.reference) spill(edx);
		if(EBX.reference != allocationState.ebx.reference) spill(ebx);
		if(ESI.reference != allocationState.esi.reference) spill(esi);
		if(EDI.reference != allocationState.edi.reference) spill(edi);

		if(MM0.reference != allocationState.mm0.reference) spill(mm0);
		if(MM1.reference != allocationState.mm1.reference) spill(mm1);
		if(MM2.reference != allocationState.mm2.reference) spill(mm2);
		if(MM3.reference != allocationState.mm3.reference) spill(mm3);
		if(MM4.reference != allocationState.mm4.reference) spill(mm4);
		if(MM5.reference != allocationState.mm5.reference) spill(mm5);
		if(MM6.reference != allocationState.mm6.reference) spill(mm6);
		if(MM7.reference != allocationState.mm7.reference) spill(mm7);

		if(XMM0.reference != allocationState.xmm0.reference) spill(xmm0);
		if(XMM1.reference != allocationState.xmm1.reference) spill(xmm1);
		if(XMM2.reference != allocationState.xmm2.reference) spill(xmm2);
		if(XMM3.reference != allocationState.xmm3.reference) spill(xmm3);
		if(XMM4.reference != allocationState.xmm4.reference) spill(xmm4);
		if(XMM5.reference != allocationState.xmm5.reference) spill(xmm5);
		if(XMM6.reference != allocationState.xmm6.reference) spill(xmm6);
		if(XMM7.reference != allocationState.xmm7.reference) spill(xmm7);
	}

	void CodeGenerator::spillAll()
	{
		spill(eax);
		spill(ecx);
		spill(edx);
		spill(ebx);
		spill(esi);
		spill(edi);

		spill(mm0);
		spill(mm1);
		spill(mm2);
		spill(mm3);
		spill(mm4);
		spill(mm5);
		spill(mm6);
		spill(mm7);

		spill(xmm0);
		spill(xmm1);
		spill(xmm2);
		spill(xmm3);
		spill(xmm4);
		spill(xmm5);
		spill(xmm6);
		spill(xmm7);
	}

	void CodeGenerator::spillMMX()
	{
		spill(mm0);
		spill(mm1);
		spill(mm2);
		spill(mm3);
		spill(mm4);
		spill(mm5);
		spill(mm6);
		spill(mm7);
	}

	void CodeGenerator::spillMMXcept(const OperandMMREG &reg)
	{
		if(reg.reg != Encoding::MM0) spill(mm0);
		if(reg.reg != Encoding::MM1) spill(mm1);
		if(reg.reg != Encoding::MM2) spill(mm2);
		if(reg.reg != Encoding::MM3) spill(mm3);
		if(reg.reg != Encoding::MM4) spill(mm4);
		if(reg.reg != Encoding::MM5) spill(mm5);
		if(reg.reg != Encoding::MM6) spill(mm6);
		if(reg.reg != Encoding::MM7) spill(mm7);

		emms();
	}

	void CodeGenerator::enableEmulateSSE()
	{
		emulateSSE = true;
		copyPropagation = false;
		dropUnmodified = false;
		spillUnrelocated = true;
		loadLoadElimination = false;
	}

	void CodeGenerator::disableEmulateSSE()
	{
		emulateSSE = false;
	}

	void CodeGenerator::enableCopyPropagation()
	{
		if(!emulateSSE)
		{
			copyPropagation = true;
		}
		else
		{
			copyPropagation = false;
		}
	}

	void CodeGenerator::disableCopyPropagation()
	{
		copyPropagation = false;
	}

	void CodeGenerator::enableDropUnmodified()
	{
		if(!emulateSSE)
		{
			dropUnmodified = true;
		}
		else
		{
			dropUnmodified = false;
		}
	}

	void CodeGenerator::disableDropUnmodified()
	{
		dropUnmodified = false;
	}

	void CodeGenerator::enableSpillUnrelocated()
	{
		spillUnrelocated = true;
	}

	void CodeGenerator::disableSpillUnrelocated()
	{
		if(!emulateSSE)
		{
			spillUnrelocated = false;
		}
		else
		{
			spillUnrelocated = true;
		}
	}

	void CodeGenerator::enableLoadLoadElimination()
	{
		if(!emulateSSE)
		{
			loadLoadElimination = true;
		}
		else
		{
			loadLoadElimination = false;
		}
	}

	void CodeGenerator::disableLoadLoadElimination()
	{
		loadLoadElimination = false;
	}

	Encoding *CodeGenerator::addps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[i][0]]);
			fadd(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);

			fld(dword_ptr [&sse[i][1]]);
			fadd(dword_ptr [&sse[j][1]]);
			fstp(dword_ptr [&sse[i][1]]);

			fld(dword_ptr [&sse[i][2]]);
			fadd(dword_ptr [&sse[j][2]]);
			fstp(dword_ptr [&sse[i][2]]);

			fld(dword_ptr [&sse[i][3]]);
			fadd(dword_ptr [&sse[j][3]]);
			fstp(dword_ptr [&sse[i][3]]);

			return 0;
		}
		else
		{
			return Assembler::addps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::addps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(dword_ptr [&sse[i][0]]);
			fadd((OperandMEM32)(mem128+0));
			fstp(dword_ptr [&sse[i][0]]);

			fld(dword_ptr [&sse[i][1]]);
			fadd((OperandMEM32)(mem128+4));
			fstp(dword_ptr [&sse[i][1]]);

			fld(dword_ptr [&sse[i][2]]);
			fadd((OperandMEM32)(mem128+8));
			fstp(dword_ptr [&sse[i][2]]);

			fld(dword_ptr [&sse[i][3]]);
			fadd((OperandMEM32)(mem128+12));
			fstp(dword_ptr [&sse[i][3]]);
			
			return 0;
		}
		else
		{
			return Assembler::addps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::addps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return addps(xmm, (OperandXMMREG)r_m128);
		else                               return addps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::addss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fadd(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::addss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::addss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fadd((OperandMEM32)mem32);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::addss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::addss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return addss(xmm, (OperandXMMREG)xmm32);
		else                              return addss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::andnps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			not(dword_ptr [&sse[i][0]]);
			and(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			not(dword_ptr [&sse[i][1]]);
			and(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			not(dword_ptr [&sse[i][2]]);
			and(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			not(dword_ptr [&sse[i][3]]);
			and(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::andnps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::andnps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128+0));
			not(dword_ptr [&sse[i][0]]);
			and(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+4));
			not(dword_ptr [&sse[i][1]]);
			and(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+8));
			not(dword_ptr [&sse[i][2]]);
			and(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+12));
			not(dword_ptr [&sse[i][3]]);
			and(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::andnps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::andnps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return andnps(xmm, (OperandXMMREG)r_m128);
		else                               return andnps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::andps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			and(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			and(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			and(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			and(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::andps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::andps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128+0));
			and(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+4));
			and(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+8));
			and(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+12));
			and(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::andps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::andps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return andps(xmm, (OperandXMMREG)r_m128);
		else                               return andps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::cmpps(OperandXMMREG xmmi, OperandXMMREG xmmj, char c)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr [&zero]);		// st2
			fld(dword_ptr [&one]);		// st1

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][0]]);

			fld(dword_ptr [&sse[j][1]]);
			fld(dword_ptr [&sse[i][1]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][1]]);

			fld(dword_ptr [&sse[j][2]]);
			fld(dword_ptr [&sse[i][2]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][2]]);

			fld(dword_ptr [&sse[j][3]]);
			fld(dword_ptr [&sse[i][3]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][3]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}
		else
		{
			return Assembler::cmpps(xmmi, xmmj, c);
		}
	}

	Encoding *CodeGenerator::cmpps(OperandXMMREG xmm, OperandMEM128 mem128, char c)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr [&zero]);		// st2
			fld(dword_ptr [&one]);		// st1

			fld((OperandMEM32)(mem128+0));
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][0]]);

			fld((OperandMEM32)(mem128+4));
			fld(dword_ptr [&sse[i][1]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][1]]);

			fld((OperandMEM32)(mem128+8));
			fld(dword_ptr [&sse[i][2]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][2]]);

			fld((OperandMEM32)(mem128+12));
			fld(dword_ptr [&sse[i][3]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][3]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}
		else
		{
			return Assembler::cmpps(xmm, mem128, c);
		}
	}

	Encoding *CodeGenerator::cmpps(OperandXMMREG xmm, OperandR_M128 r_m128, char c)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return cmpps(xmm, (OperandXMMREG)r_m128, c);
		else                               return cmpps(xmm, (OperandMEM128)r_m128, c);
	}

	Encoding *CodeGenerator::cmpeqps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 0);
	}

	Encoding *CodeGenerator::cmpeqps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 0);
	}

	Encoding *CodeGenerator::cmpeqps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 0);
	}

	Encoding *CodeGenerator::cmpleps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 2);
	}

	Encoding *CodeGenerator::cmpleps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 2);
	}

	Encoding *CodeGenerator::cmpleps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 2);
	}

	Encoding *CodeGenerator::cmpltps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 1);
	}

	Encoding *CodeGenerator::cmpltps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 1);
	}

	Encoding *CodeGenerator::cmpltps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 1);
	}

	Encoding *CodeGenerator::cmpneqps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 4);
	}

	Encoding *CodeGenerator::cmpneqps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 4);
	}

	Encoding *CodeGenerator::cmpneqps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 4);
	}

	Encoding *CodeGenerator::cmpnleps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 6);
	}

	Encoding *CodeGenerator::cmpnleps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 6);
	}

	Encoding *CodeGenerator::cmpnleps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 6);
	}

	Encoding *CodeGenerator::cmpnltps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 5);
	}

	Encoding *CodeGenerator::cmpnltps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 5);
	}

	Encoding *CodeGenerator::cmpnltps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 5);
	}

	Encoding *CodeGenerator::cmpordps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 7);
	}

	Encoding *CodeGenerator::cmpordps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 7);
	}

	Encoding *CodeGenerator::cmpordps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 7);
	}

	Encoding *CodeGenerator::cmpunordps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpps(xmmi, xmmj, 3);
	}

	Encoding *CodeGenerator::cmpunordps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		return cmpps(xmm, mem128, 3);
	}

	Encoding *CodeGenerator::cmpunordps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		return cmpps(xmm, r_m128, 3);
	}

	Encoding *CodeGenerator::cmpss(OperandXMMREG xmmi, OperandXMMREG xmmj, char c)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr [&zero]);		// st2
			fld(dword_ptr [&one]);		// st1

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][0]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}
		else
		{
			return Assembler::cmpss(xmmi, xmmj, c);
		}
	}

	Encoding *CodeGenerator::cmpss(OperandXMMREG xmm, OperandMEM32 mem32, char c)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float zero = 0;
			static float one = 1;
			fld(dword_ptr [&zero]);		// st2
			fld(dword_ptr [&one]);		// st1

			fld((OperandMEM32)(mem32+0));
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			switch(c)
			{
			case 0:   // CMPEQPS
				fcmove(st1);
				fcmovne(st2);
				break;
			case 1:   // CMPLTPS
				fcmovb(st1);
				fcmovnb(st2);
				break;
			case 2:   // CMPLEPS
				fcmovbe(st1);
				fcmovnbe(st2);
				break;
			case 3:   // CMPUNORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			case 4:   // CMPNEQPS
				fcmovne(st1);
				fcmove(st2);
				break;
			case 5:   // CMPNLTPS
				fcmovnb(st1);
				fcmovb(st2);
				break;
			case 6:   // CMPNLEPS
				fcmovnbe(st1);
				fcmovbe(st2);
				break;
			case 7:   // CMPORDPS
				fcmovnu(st1);
				fcmovu(st2);
				break;
			default:
				throw INTERNAL_ERROR;
			}
			fstp(dword_ptr [&sse[i][0]]);

			ffree(st0);
			ffree(st1);

			return 0;
		}
		else
		{
			return Assembler::cmpss(xmm, mem32, c);
		}
	}

	Encoding *CodeGenerator::cmpss(OperandXMMREG xmm, OperandXMM32 xmm32, char c)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return cmpss(xmm, (OperandXMMREG)xmm32, c);
		else                              return cmpss(xmm, (OperandMEM32)xmm32, c);
	}

	Encoding *CodeGenerator::cmpeqss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 0);
	}

	Encoding *CodeGenerator::cmpeqss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 0);
	}

	Encoding *CodeGenerator::cmpeqss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 0);
	}

	Encoding *CodeGenerator::cmpless(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 2);
	}

	Encoding *CodeGenerator::cmpless(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 2);
	}

	Encoding *CodeGenerator::cmpless(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 2);
	}

	Encoding *CodeGenerator::cmpltss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 1);
	}

	Encoding *CodeGenerator::cmpltss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 1);
	}

	Encoding *CodeGenerator::cmpltss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 1);
	}

	Encoding *CodeGenerator::cmpneqss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 4);
	}

	Encoding *CodeGenerator::cmpneqss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 4);
	}

	Encoding *CodeGenerator::cmpneqss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 4);
	}

	Encoding *CodeGenerator::cmpnless(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 6);
	}

	Encoding *CodeGenerator::cmpnless(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 6);
	}

	Encoding *CodeGenerator::cmpnless(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 6);
	}

	Encoding *CodeGenerator::cmpnltss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 5);
	}

	Encoding *CodeGenerator::cmpnltss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 5);
	}

	Encoding *CodeGenerator::cmpnltss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 5);
	}

	Encoding *CodeGenerator::cmpordss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 7);
	}

	Encoding *CodeGenerator::cmpordss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 7);
	}

	Encoding *CodeGenerator::cmpordss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 7);
	}

	Encoding *CodeGenerator::cmpunordss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		return cmpss(xmmi, xmmj, 3);
	}

	Encoding *CodeGenerator::cmpunordss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		return cmpss(xmm, mem32, 3);
	}

	Encoding *CodeGenerator::cmpunordss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		return cmpss(xmm, xmm32, 3);
	}

	Encoding *CodeGenerator::cmpxchg(OperandR_M8 r_m8, OperandREG8 r8)
	{
		markModified(r_m8);
		markModified(al);

		return Assembler::cmpxchg(r_m8, r8);
	}

	Encoding *CodeGenerator::cmpxchg(OperandR_M16 r_m16, OperandREG16 r16)
	{
		markModified(r_m16);
		markModified(ax);

		return Assembler::cmpxchg(r_m16, r16);
	}

	Encoding *CodeGenerator::cmpxchg(OperandR_M32 r_m32, OperandREG32 r32)
	{
		markModified(r_m32);
		markModified(eax);

		return Assembler::cmpxchg(r_m32, r32);
	}

	Encoding *CodeGenerator::lock_cmpxchg(OperandMEM8 m8, OperandREG8 r8)
	{
		markModified(m8);
		markModified(al);

		return Assembler::lock_cmpxchg(m8, r8);
	}

	Encoding *CodeGenerator::lock_cmpxchg(OperandMEM16 m16, OperandREG16 r16)
	{
		markModified(m16);
		markModified(ax);

		return Assembler::lock_cmpxchg(m16, r16);
	}

	Encoding *CodeGenerator::lock_cmpxchg(OperandMEM32 m32, OperandREG32 r32)
	{
		markModified(m32);
		markModified(eax);

		return Assembler::lock_cmpxchg(m32, r32);
	}

	Encoding *CodeGenerator::comiss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::comiss(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::comiss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			
			fld(mem32);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::comiss(xmm, mem32);
		}
	}

	Encoding *CodeGenerator::comiss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return comiss(xmm, (OperandXMMREG)xmm32);
		else                              return comiss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::cvtpi2ps(OperandXMMREG xmm, OperandMMREG mm)
	{
		if(emulateSSE)
		{
			static int dword[2];
			movq(qword_ptr [dword], mm);
			const int i = xmm.reg;
			spillMMX();

			fild(dword_ptr [&dword[0]]);
			fstp(dword_ptr [&sse[i][0]]);
			fild(dword_ptr [&dword[1]]);
			fstp(dword_ptr [&sse[i][1]]);

			return 0;
		}
		else
		{
			return Assembler::cvtpi2ps(xmm, mm);
		}
	}

	Encoding *CodeGenerator::cvtpi2ps(OperandXMMREG xmm, OperandMEM64 mem64)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fild((OperandMEM32)(mem64+0));
			fstp(dword_ptr [&sse[i][0]]);
			fild((OperandMEM32)(mem64+4));
			fstp(dword_ptr [&sse[i][1]]);

			return 0;
		}
		else
		{
			return Assembler::cvtpi2ps(xmm, mem64);
		}
	}

	Encoding *CodeGenerator::cvtpi2ps(OperandXMMREG xmm, OperandR_M64 r_m64)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return cvtpi2ps(xmm, (OperandMMREG)r_m64);
		else                             return cvtpi2ps(xmm, (OperandMEM64)r_m64);
	}

	Encoding *CodeGenerator::cvtps2pi(OperandMMREG mm, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
			const int i = xmm.reg;
		//	short fpuCW1;
		//	short fpuCW2;

		//	fldcw(word_ptr [&fpuCW1]);
		//	fldcw(word_ptr [&fpuCW2]);
		//	and(word_ptr [&fpuCW2], (short)0xF3FF);
		//	fstcw(word_ptr [&fpuCW2]);

			fld(dword_ptr [&sse[i][0]]);
			fistp(dword_ptr [&dword[0]]);
			fld(dword_ptr [&sse[i][1]]);
			fistp(dword_ptr [&dword[1]]);

		//	fstcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr [dword]);

			return 0;
		}
		else
		{
			return Assembler::cvtps2pi(mm, xmm);
		}
	}

	Encoding *CodeGenerator::cvtps2pi(OperandMMREG mm, OperandMEM64 mem64)
	{
		if(emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
		//	short fpuCW1;
		//	short fpuCW2;

		//	fldcw(word_ptr [&fpuCW1]);
		//	fldcw(word_ptr [&fpuCW2]);
		//	and(word_ptr [&fpuCW2], (short)0xF3FF);
		//	fstcw(word_ptr [&fpuCW2]);

			fld((OperandMEM32)(mem64+0));
			fistp(dword_ptr [&dword[0]]);
			fld((OperandMEM32)(mem64+4));
			fistp(dword_ptr [&dword[1]]);

		//	fstcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr [dword]);

			return 0;
		}
		else
		{
			return Assembler::cvtps2pi(mm, mem64);
		}
	}

	Encoding *CodeGenerator::cvtps2pi(OperandMMREG mm, OperandXMM64 xmm64)
	{
		if(xmm64.type == Operand::OPERAND_XMMREG) return cvtps2pi(mm, (OperandXMMREG)xmm64);
		else                              return cvtps2pi(mm, (OperandMEM64)xmm64);
	}

	Encoding *CodeGenerator::cvttps2pi(OperandMMREG mm, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			static int dword[2];
			spillMMXcept(mm);
			const int i = xmm.reg;
			short fpuCW1;
			short fpuCW2;

			fstcw(word_ptr [&fpuCW1]);
			fstcw(word_ptr [&fpuCW2]);
			or(word_ptr [&fpuCW2], (short)0x0C00);
			fldcw(word_ptr [&fpuCW2]);

			fld(dword_ptr [&sse[i][0]]);
			fistp(dword_ptr [&dword[0]]);
			fld(dword_ptr [&sse[i][1]]);
			fistp(dword_ptr [&dword[1]]);

			fldcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr [dword]);

			return 0;
		}
		else
		{
			return Assembler::cvttps2pi(mm, xmm);
		}
	}

	Encoding *CodeGenerator::cvttps2pi(OperandMMREG mm, OperandMEM64 mem64)
	{
		if(emulateSSE)
		{
			static int dword[2];

			spillMMXcept(mm);
			static short fpuCW1;
			static short fpuCW2;

			fstcw(word_ptr [&fpuCW1]);
			fstcw(word_ptr [&fpuCW2]);
			or(word_ptr [&fpuCW2], (short)0x0C00);
			fldcw(word_ptr [&fpuCW2]);

			fld((OperandMEM32)(mem64+0));
			fistp(dword_ptr [&dword[0]]);
			fld((OperandMEM32)(mem64+4));
			fistp(dword_ptr [&dword[1]]);

			fldcw(word_ptr [&fpuCW1]);
			movq(mm, qword_ptr [dword]);

			return 0;
		}
		else
		{
			return Assembler::cvttps2pi(mm, mem64);
		}
	}

	Encoding *CodeGenerator::cvttps2pi(OperandMMREG mm, OperandXMM64 xmm64)
	{
		if(xmm64.type == Operand::OPERAND_XMMREG) return cvttps2pi(mm, (OperandXMMREG)xmm64);
		else                              return cvttps2pi(mm, (OperandMEM64)xmm64);
	}

	Encoding *CodeGenerator::cvtsi2ss(OperandXMMREG xmm, OperandREG32 reg32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static int dword;

			mov(dword_ptr [&dword], reg32);
			fild(dword_ptr [&dword]);
			fstp(dword_ptr [&sse[i][0]]);

			return 0;
		}
		else
		{
			return Assembler::cvtsi2ss(xmm, reg32);
		}
	}

	Encoding *CodeGenerator::cvtsi2ss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fild(mem32);
			fstp(dword_ptr [&sse[i][0]]);

			return 0;
		}
		else
		{
			return Assembler::cvtsi2ss(xmm, mem32);
		}
	}

	Encoding *CodeGenerator::cvtsi2ss(OperandXMMREG xmm, OperandR_M32 r_m32)
	{
		if(r_m32.type == Operand::OPERAND_REG32) return cvtsi2ss(xmm, (OperandREG32)r_m32);
		else                             return cvtsi2ss(xmm, (OperandMEM32)r_m32);
	}

	Encoding *CodeGenerator::cvtss2si(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
		//	short fpuCW1;
		//	short fpuCW2;
			static int dword;

		//	fldcw(word_ptr [&fpuCW1]);
		//	fldcw(word_ptr [&fpuCW2]);
		//	and(word_ptr [&fpuCW2], (short)0xF3FF);
		//	fstcw(word_ptr [&fpuCW2]);

			fld(dword_ptr [&sse[i][0]]);
			fistp(dword_ptr [&dword]);
			mov(reg32, dword_ptr [&dword]);

		//	fstcw(word_ptr [&fpuCW1]);

			return 0;
		}
		else
		{
			return Assembler::cvtss2si(reg32, xmm);
		}
	}

	Encoding *CodeGenerator::cvtss2si(OperandREG32 reg32, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
		//	short fpuCW1;
		//	short fpuCW2;
			static int dword;

		//	fldcw(word_ptr [&fpuCW1]);
		//	fldcw(word_ptr [&fpuCW2]);
		//	and(word_ptr [&fpuCW2], (short)0xF3FF);
		//	fstcw(word_ptr [&fpuCW2]);

			fld(mem32);
			fistp(dword_ptr [&dword]);
			mov(reg32, dword_ptr [&dword]);

		//	fstcw(word_ptr [&fpuCW1]);

			return 0;
		}
		else
		{
			return Assembler::cvtss2si(reg32, mem32);
		}
	}

	Encoding *CodeGenerator::cvtss2si(OperandREG32 reg32, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return cvtss2si(reg32, (OperandXMMREG)xmm32);
		else                              return cvtss2si(reg32, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::cvttss2si(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static short fpuCW1;
			static short fpuCW2;
			static int dword;

			fstcw(word_ptr [&fpuCW1]);
			fstcw(word_ptr [&fpuCW2]);
			or(word_ptr [&fpuCW2], (short)0x0C00);
			fldcw(word_ptr [&fpuCW2]);

			fld(dword_ptr [&sse[i][0]]);
			fistp(dword_ptr [&dword]);
			mov(reg32, dword_ptr [&dword]);

			fldcw(word_ptr [&fpuCW1]);

			return 0;
		}
		else
		{
			return Assembler::cvttss2si(reg32, xmm);
		}
	}

	Encoding *CodeGenerator::cvttss2si(OperandREG32 reg32, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			static short fpuCW1;
			static short fpuCW2;
			static int dword;

			fstcw(word_ptr [&fpuCW1]);
			fstcw(word_ptr [&fpuCW2]);
			or(word_ptr [&fpuCW2], (short)0x0C00);
			fldcw(word_ptr [&fpuCW2]);

			fld(mem32);
			fistp(dword_ptr [&dword]);
			mov(reg32, dword_ptr [&dword]);

			fldcw(word_ptr [&fpuCW1]);

			return 0;
		}
		else
		{
			return Assembler::cvttss2si(reg32, mem32);
		}
	}

	Encoding *CodeGenerator::cvttss2si(OperandREG32 reg32, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return cvttss2si(reg32, (OperandXMMREG)xmm32);
		else                              return cvttss2si(reg32, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::divps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fdiv(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fdiv(dword_ptr [&sse[j][1]]);
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fdiv(dword_ptr [&sse[j][2]]);
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fdiv(dword_ptr [&sse[j][3]]);
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::divps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::divps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fdiv((OperandMEM32)(mem128+0));
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fdiv((OperandMEM32)(mem128+4));
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fdiv((OperandMEM32)(mem128+8));
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fdiv((OperandMEM32)(mem128+12));
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::divps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::divps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return divps(xmm, (OperandXMMREG)r_m128);
		else                               return divps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::divss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fdiv(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::divss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::divss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fdiv((OperandMEM32)mem32);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::divss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::divss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return divss(xmm, (OperandXMMREG)xmm32);
		else                              return divss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::ldmxcsr(OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return Assembler::ldmxcsr(mem32);
		}
	}

	Encoding *CodeGenerator::maskmovq(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			static short qword1[4];
			static short qword2[4];

			movq(qword_ptr [&qword1], mmi);
			movq(qword_ptr [&qword2], mmj);

			test(byte_ptr [&qword2+0], (char)0x80);
			mov(t8(0), byte_ptr [edi+0]);
			cmovnz(t32(0), dword_ptr [&qword1+0]);
			mov(byte_ptr [edi+0], t8(0));

			test(byte_ptr [&qword2+1], (char)0x80);
			mov(t8(0), byte_ptr [edi+1]);
			cmovnz(t32(0), dword_ptr [&qword1+1]);
			mov(byte_ptr [edi+1], t8(0));

			test(byte_ptr [&qword2+2], (char)0x80);
			mov(t8(0), byte_ptr [edi+2]);
			cmovnz(t32(0), dword_ptr [&qword1+2]);
			mov(byte_ptr [edi+2], t8(0));

			test(byte_ptr [&qword2+3], (char)0x80);
			mov(t8(0), byte_ptr [edi+3]);
			cmovnz(t32(0), dword_ptr [&qword1+3]);
			mov(byte_ptr [edi+3], t8(0));

			test(byte_ptr [&qword2+4], (char)0x80);
			mov(t8(0), byte_ptr [edi+4]);
			cmovnz(t32(0), dword_ptr [&qword1+4]);
			mov(byte_ptr [edi+4], t8(0));

			test(byte_ptr [&qword2+5], (char)0x80);
			mov(t8(0), byte_ptr [edi+5]);
			cmovnz(t32(0), dword_ptr [&qword1+5]);
			mov(byte_ptr [edi+5], t8(0));

			test(byte_ptr [&qword2+6], (char)0x80);
			mov(t8(0), byte_ptr [edi+6]);
			cmovnz(t32(0), dword_ptr [&qword1+6]);
			mov(byte_ptr [edi+6], t8(0));

			test(byte_ptr [&qword2+7], (char)0x80);
			mov(t8(0), byte_ptr [edi+7]);
			cmovnz(t32(0), dword_ptr [&qword1+7]);
			mov(byte_ptr [edi+7], t8(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::maskmovq(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::maxps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][1]]);
			fld(dword_ptr [&sse[i][1]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][1]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][2]]);
			fld(dword_ptr [&sse[i][2]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][2]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][3]]);
			fld(dword_ptr [&sse[i][3]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][3]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::maxps(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::maxps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld((OperandMEM32)(mem128+0));
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+4));
			fld(dword_ptr [&sse[i][1]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][1]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+8));
			fld(dword_ptr [&sse[i][2]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][2]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+0));
			fld(dword_ptr [&sse[i][3]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][3]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::maxps(xmm, mem128);
		}
	}

	Encoding *CodeGenerator::maxps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return maxps(xmm, (OperandXMMREG)r_m128);
		else                               return maxps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::maxss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::maxss(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::maxss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::maxss(xmm, mem32);
		}
	}

	Encoding *CodeGenerator::maxss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return maxss(xmm, (OperandXMMREG)xmm32);
		else                              return maxss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::minps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][1]]);
			fld(dword_ptr [&sse[i][1]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][1]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][2]]);
			fld(dword_ptr [&sse[i][2]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][2]]);
			ffree(st0);

			fld(dword_ptr [&sse[j][3]]);
			fld(dword_ptr [&sse[i][3]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][3]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::minps(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::minps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld((OperandMEM32)(mem128+0));
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+4));
			fld(dword_ptr [&sse[i][1]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][1]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+8));
			fld(dword_ptr [&sse[i][2]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][2]]);
			ffree(st0);

			fld((OperandMEM32)(mem128+0));
			fld(dword_ptr [&sse[i][3]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][3]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::minps(xmm, mem128);
		}
	}

	Encoding *CodeGenerator::minps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return minps(xmm, (OperandXMMREG)r_m128);
		else                               return minps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::minss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::minss(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::minss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;

			fld(mem32);
			fld(dword_ptr [&sse[i][0]]);
			fucomi(st0, st1);
			fcmovnb(st1);
			fstp(dword_ptr [&sse[i][0]]);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::minss(xmm, mem32);
		}
	}

	Encoding *CodeGenerator::minss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return minss(xmm, (OperandXMMREG)xmm32);
		else                              return minss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::mov(OperandREG32 r32i, OperandREG32 r32j)
	{
		if(r32i == r32j) return 0;

		if(r32i.reg == Encoding::ESP || r32i.reg == Encoding::EBP ||
		   r32j.reg == Encoding::ESP || r32j.reg == Encoding::EBP)
		{
			return Assembler::mov(r32i, r32j);
		}

		if(loadLoadElimination)
		{
			// If the destination register has been loaded without being used,
			// eliminate loading instruction by preventing retain()
			if(X32(r32i).load)
			{
				X32(r32i).load->reserve();
				X32(r32i).load = 0;
			}
		}

		Encoding *mov = Assembler::mov(r32i, r32j);

		if(loadLoadElimination)
		{
			X32(r32i).load = mov;
			mov->reserve();
		}
		
		if(copyPropagation)
		{
			// Return if not in allocation table
			if(X32(r32i).reference == 0)
			{
				return mov;
			}

			// Attempt copy propagation
			mov->reserve();
			swapAllocation(&X32(r32i), &X32(r32j));
			X32(r32i).copy = mov;
		}

		return mov;
	}

	Encoding *CodeGenerator::mov(OperandREG32 r32, OperandMEM32 m32)
	{
		if(r32.reg == Encoding::ESP || r32.reg == Encoding::EBP)
		{
			return Assembler::mov(r32, m32);
		}

		if(loadLoadElimination)
		{
			// If the destination register has been loaded without being used,
			// eliminate loading instruction by preventing retain()
			if(X32(r32).load)
			{
				X32(r32).load->reserve();
				X32(r32).load = 0;
			}
		}

		Encoding *mov = Assembler::mov(r32, m32);

		if(loadLoadElimination)
		{
			X32(r32).load = mov;
			mov->reserve();
		}

		return mov;
	}

	Encoding *CodeGenerator::mov(OperandREG32 r32, OperandR_M32 r_m32)
	{
		if(Operand::isReg(r_m32)) return mov(r32, (OperandREG32)r_m32);
		else                      return mov(r32, (OperandMEM32)r_m32);
	}

	Encoding *CodeGenerator::movaps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		X128(xmmi).partial = 0;
		X128(xmmj).partial = 0;

		if(xmmi == xmmj) return 0;
		
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}

		if(loadLoadElimination)
		{
			// If the destination register has been loaded without being used,
			// eliminate loading instruction by preventing retain()
			if(X128(xmmi).load)
			{
				X128(xmmi).load->reserve();
				X128(xmmi).load = 0;
			}
		}

		Encoding *movaps = Assembler::movaps(xmmi, xmmj);
		
		if(loadLoadElimination)
		{
			X128(xmmi).load = movaps;
			movaps->reserve();
		}

		if(copyPropagation)
		{
			// Return if not in allocation table
			if(X128(xmmi).reference == 0)
			{
				return movaps;
			}

			// Attempt copy propagation
			movaps->reserve();
			swapAllocation(&X128(xmmi), &X128(xmmj));
			X128(xmmi).copy = movaps;
		}

		return movaps;
	}

	Encoding *CodeGenerator::movaps(OperandXMMREG xmm, OperandMEM128 m128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m128+0));
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(m128+4));
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(m128+8));
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m128+12));
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			if(loadLoadElimination)
			{
				// If the destination register has been loaded without being used,
				// eliminate loading instruction by preventing retain()
				if(X128(xmm).load)
				{
					X128(xmm).load->reserve();
					X128(xmm).load = 0;
				}
			}

			Encoding *movaps = Assembler::movaps(xmm, m128);

			if(loadLoadElimination)
			{
				X128(xmm).load = movaps;
				movaps->reserve();
			}

			return movaps;
		}
	}

	Encoding *CodeGenerator::movaps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return movaps(xmm, (OperandXMMREG)r_m128);
		else                               return movaps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::movaps(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov((OperandMEM32)(m128+0), t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov((OperandMEM32)(m128+4), t32(0));

			mov(t32(0), dword_ptr [&sse[i][2]]);
			mov((OperandMEM32)(m128+8), t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			mov((OperandMEM32)(m128+12), t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movaps(m128, xmm);
		}
	}

	Encoding *CodeGenerator::movaps(OperandR_M128 r_m128, OperandXMMREG xmm)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return movaps((OperandXMMREG)r_m128, xmm);
		else                               return movaps((OperandMEM128)r_m128, xmm);
	}

	Encoding *CodeGenerator::movhlps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][2]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movhlps(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::movhps(OperandXMMREG xmm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m64+0));
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m64+4));
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movhps(xmm, m64);
		}
	}

	Encoding *CodeGenerator::movhps(OperandMEM64 m64, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][2]]);
			mov((OperandMEM32)(m64+0), t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			mov((OperandMEM32)(m64+4), t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movhps(m64, xmm);
		}
	}

	Encoding *CodeGenerator::movhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(xmmi == xmmj) return 0;

		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][2]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movhps(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::movlhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movlhps(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::movlps(OperandXMMREG xmm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(m64+0));
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(m64+4));
			mov(dword_ptr [&sse[i][1]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movlps(xmm, m64);
		}
	}

	Encoding *CodeGenerator::movlps(OperandMEM64 m64, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov((OperandMEM32)(m64+0), t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov((OperandMEM32)(m64+4), t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movlps(m64, xmm);
		}
	}

	Encoding *CodeGenerator::movmskps(OperandREG32 reg32, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			shr(t32(0), 31);
			mov(reg32, t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			shr(t32(0), 31);
			shl(t32(0), 1);
			or(reg32, t32(0));

			mov(t32(0), dword_ptr [&sse[i][2]]);
			shr(t32(0), 31);
			shl(t32(0), 2);
			or(reg32, t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			shr(t32(0), 31);
			shl(t32(0), 3);
			or(reg32, t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movmskps(reg32, xmm);
		}
	}

	Encoding *CodeGenerator::movntps(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov((OperandMEM32)(m128+0), t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov((OperandMEM32)(m128+4), t32(0));

			mov(t32(0), dword_ptr [&sse[i][2]]);
			mov((OperandMEM32)(m128+8), t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			mov((OperandMEM32)(m128+12), t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movntps(m128, xmm);
		}
	}

	Encoding *CodeGenerator::movntq(OperandMEM64 m64, OperandMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov((OperandMEM32)(m64+0), t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov((OperandMEM32)(m64+4), t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movntq(m64, xmm);
		}
	}

	Encoding *CodeGenerator::movq(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(mmi == mmj) return 0;
		
		if(loadLoadElimination)
		{
			// If the destination register has been loaded without being used,
			// eliminate loading instruction by preventing retain()
			if(X64(mmi).load)
			{
				X64(mmi).load->reserve();
				X64(mmi).load = 0;
			}
		}

		Encoding *movq = Assembler::movq(mmi, mmj);

		if(loadLoadElimination)
		{
			X64(mmi).load = movq;
			movq->reserve();
		}

		if(copyPropagation)
		{
			// Return if not in allocation table
			if(X64(mmi).reference == 0)
			{
				return movq;
			}

			// Attempt copy propagation
			movq->reserve();
			swapAllocation(&X64(mmi), &X64(mmj));
			X64(mmi).copy = movq;
		}

		return movq;
	}

	Encoding *CodeGenerator::movq(OperandMMREG mm, OperandMEM64 mem64)
	{
		if(loadLoadElimination)
		{
			// If the destination register has been loaded without being used,
			// eliminate loading instruction by preventing retain()
			if(X64(mm).load)
			{
				X64(mm).load->reserve();
				X64(mm).load = 0;
			}
		}

		Encoding *movq = Assembler::movq(mm, mem64);

		if(loadLoadElimination)
		{
			X64(mm).load = movq;
			movq->reserve();
		}

		return movq;
	}

	Encoding *CodeGenerator::movq(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return movq(mm, (OperandMMREG)r_m64);
		else                             return movq(mm, (OperandMEM64)r_m64);
	}

	Encoding *CodeGenerator::movq(OperandMEM64 mem64, OperandMMREG mm)
	{
		return Assembler::movq(mem64, mm);
	}

	Encoding *CodeGenerator::movq(OperandR_M64 r_m64, OperandMMREG mm)
	{
		if(r_m64 == mm) return 0;

		return Assembler::movq(r_m64, mm);
	}

	Encoding *CodeGenerator::movss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(xmmi == xmmj) return 0;

		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			if(loadLoadElimination)
			{
				// If the destination register has been loaded without being used,
				// eliminate loading instruction by preventing retain()
				if(X128(xmmi).load)
				{
					X128(xmmi).load->reserve();
					X128(xmmi).load = 0;
				}
			}

			Encoding *movss = Assembler::movss(xmmi, xmmj);

			if(loadLoadElimination)
			{
				X128(xmmi).load = movss;
				movss->reserve();
			}

			return movss;
		}
	}

	Encoding *CodeGenerator::movss(OperandXMMREG xmm, OperandMEM32 m32)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), m32);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(dword_ptr [&sse[i][1]], 0);
			mov(dword_ptr [&sse[i][2]], 0);
			mov(dword_ptr [&sse[i][3]], 0);

			free(0);
			return 0;
		}
		else
		{			
			if(loadLoadElimination)
			{
				// If the destination register has been loaded without being used,
				// eliminate loading instruction by preventing retain()
				if(X128(xmm).load)
				{
					X128(xmm).load->reserve();
					X128(xmm).load = 0;
				}
			}

			Encoding *movss = Assembler::movss(xmm, m32);

			if(loadLoadElimination)
			{
				X128(xmm).load = movss;
				movss->reserve();
			}

			return movss;
		}
	}

	Encoding *CodeGenerator::movss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return movss(xmm, (OperandXMMREG)xmm32);
		else                              return movss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::movss(OperandMEM32 m32, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov(m32, t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::movss(m32, xmm);
		}
	}

	Encoding *CodeGenerator::movss(OperandXMM32 xmm32, OperandXMMREG xmm)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return movss((OperandXMMREG)xmm32, xmm);
		else                              return movss((OperandMEM32)xmm32, xmm);
	}

	Encoding *CodeGenerator::movups(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			return movaps(xmmi, xmmj);
		}
		else
		{
			return Assembler::movups(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::movups(OperandXMMREG xmm, OperandMEM128 m128)
	{
		if(emulateSSE)
		{
			return movaps(xmm, m128);
		}
		else
		{
			return Assembler::movups(xmm, m128);
		}
	}

	Encoding *CodeGenerator::movups(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(emulateSSE)
		{
			return movaps(xmm, r_m128);
		}
		else
		{
			return Assembler::movups(xmm, r_m128);
		}
	}

	Encoding *CodeGenerator::movups(OperandMEM128 m128, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			return movaps(m128, xmm);
		}
		else
		{
			return Assembler::movups(m128, xmm);
		}
	}

	Encoding *CodeGenerator::movups(OperandR_M128 r_m128, OperandXMMREG xmm)
	{
		if(emulateSSE)
		{
			return movaps(r_m128, xmm);
		}
		else
		{
			return Assembler::movups(r_m128, xmm);
		}
	}

	Encoding *CodeGenerator::mulps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fmul(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fmul(dword_ptr [&sse[j][1]]);
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fmul(dword_ptr [&sse[j][2]]);
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fmul(dword_ptr [&sse[j][3]]);
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::mulps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::mulps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fmul((OperandMEM32)(mem128+0));
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fmul((OperandMEM32)(mem128+4));
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fmul((OperandMEM32)(mem128+8));
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fmul((OperandMEM32)(mem128+12));
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::mulps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::mulps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return mulps(xmm, (OperandXMMREG)r_m128);
		else                               return mulps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::mulss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fmul(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::mulss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::mulss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fmul((OperandMEM32)mem32);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::mulss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::mulss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return mulss(xmm, (OperandXMMREG)xmm32);
		else                              return mulss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::orps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			or(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			or(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			or(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			or(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::orps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::orps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128+0));
			or(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+4));
			or(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+8));
			or(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+12));
			or(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::orps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::orps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return orps(xmm, (OperandXMMREG)r_m128);
		else                               return orps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::pavgb(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			static unsigned char byte1[8];
			static unsigned char byte2[8];

			movq(qword_ptr [byte1], mmi);
			movq(qword_ptr [byte2], mmj);

			movzx(t32(0), byte_ptr [&byte1[0]]);
			movzx(t32(1), byte_ptr [&byte2[0]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[0]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[1]]);
			movzx(t32(1), byte_ptr [&byte2[1]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[1]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[2]]);
			movzx(t32(1), byte_ptr [&byte2[2]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[2]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[3]]);
			movzx(t32(1), byte_ptr [&byte2[3]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[3]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[4]]);
			movzx(t32(1), byte_ptr [&byte2[4]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[4]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[5]]);
			movzx(t32(1), byte_ptr [&byte2[5]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[5]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[6]]);
			movzx(t32(1), byte_ptr [&byte2[6]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[6]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[7]]);
			movzx(t32(1), byte_ptr [&byte2[7]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[7]], t8(0));

			movq(mmi, qword_ptr [byte1]);

			free(0);
			free(1);
			return 0;
		}
		else
		{
			return Assembler::pavgb(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pavgb(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			static unsigned char byte1[8];

			movq(qword_ptr [byte1], mm);

			static int t1;

			movzx(t32(0), byte_ptr [&byte1[0]]);
			movzx(t32(1), (OperandMEM8)(m64+0));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[0]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[1]]);
			movzx(t32(1), (OperandMEM8)(m64+1));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[1]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[2]]);
			movzx(t32(1), (OperandMEM8)(m64+2));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[2]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[3]]);
			movzx(t32(1), (OperandMEM8)(m64+3));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[3]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[4]]);
			movzx(t32(1), (OperandMEM8)(m64+4));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[4]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[5]]);
			movzx(t32(1), (OperandMEM8)(m64+5));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[5]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[6]]);
			movzx(t32(1), (OperandMEM8)(m64+6));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[6]], t8(0));

			movzx(t32(0), byte_ptr [&byte1[7]]);
			movzx(t32(1), (OperandMEM8)(m64+7));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(byte_ptr [&byte1[7]], t8(0));

			movq(mm, qword_ptr [byte1]);

			free(0);
			free(1);
			return 0;
		}
		else
		{
			return Assembler::pavgb(mm, m64);
		}
	}

	Encoding *CodeGenerator::pavgb(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return pavgb(mm, (OperandMMREG)r_m64);
		else                             return pavgb(mm, (OperandMEM64)r_m64);
	}

	Encoding *CodeGenerator::pavgw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			static unsigned short word1[4];
			static unsigned short word2[4];

			movq(qword_ptr [word1], mmi);
			movq(qword_ptr [word2], mmj);

			movzx(t32(0), word_ptr [&word1[0]]);
			movzx(t32(1), word_ptr [&word2[0]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[0]], t16(0));

			movzx(t32(0), word_ptr [&word1[1]]);
			movzx(t32(1), word_ptr [&word2[1]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[1]], t16(0));

			movzx(t32(0), word_ptr [&word1[2]]);
			movzx(t32(1), word_ptr [&word2[2]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[2]], t16(0));

			movzx(t32(0), word_ptr [&word1[3]]);
			movzx(t32(1), word_ptr [&word2[3]]);
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[3]], t16(0));

			movq(mmi, qword_ptr [word1]);

			free(0);
			free(1);
			return 0;
		}
		else
		{
			return Assembler::pavgw(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pavgw(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			static unsigned char word1[8];

			movq(qword_ptr [word1], mm);

			movzx(t32(0), word_ptr [&word1[0]]);
			movzx(t32(1), (OperandMEM16)(m64+0));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[0]], t16(0));

			movzx(t32(0), word_ptr [&word1[1]]);
			movzx(t32(1), (OperandMEM16)(m64+2));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[1]], t16(0));

			movzx(t32(0), word_ptr [&word1[2]]);
			movzx(t32(1), (OperandMEM16)(m64+4));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[2]], t16(0));

			movzx(t32(0), word_ptr [&word1[3]]);
			movzx(t32(1), (OperandMEM16)(m64+6));
			add(t32(0), t32(1));
			shr(t32(0), 1);
			mov(word_ptr [&word1[3]], t16(0));

			movq(mm, qword_ptr [word1]);

			free(0);
			free(1);
			return 0;
		}
		else
		{
			return Assembler::pavgw(mm, m64);
		}
	}

	Encoding *CodeGenerator::pavgw(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return pavgw(mm, (OperandMMREG)r_m64);
		else                             return pavgw(mm, (OperandMEM64)r_m64);
	}

	Encoding *CodeGenerator::pextrw(OperandREG32 r32, OperandMMREG mm, unsigned char c)
	{
		if(emulateSSE)
		{
			static short word[4];

			movq(qword_ptr [word], mm);
			xor(r32, r32);
			mov((OperandREG16)r32, word_ptr [&word[c & 0x03]]);

			return 0;
		}
		else
		{
			return Assembler::pextrw(r32, mm, c);
		}
	}

	Encoding *CodeGenerator::pinsrw(OperandMMREG mm, OperandREG16 r16, unsigned char c)
	{
		if(emulateSSE)
		{
			static short word[4];

			movq(qword_ptr [word], mm);
			mov(word_ptr [&word[c & 0x03]], r16);
			movq(mm, qword_ptr [word]);

			return 0;
		}
		else
		{
			return Assembler::pinsrw(mm, r16, c);
		}
	}

	Encoding *CodeGenerator::pinsrw(OperandMMREG mm, OperandMEM16 m16, unsigned char c)
	{
		if(emulateSSE)
		{
			static short word[4];

			movq(qword_ptr [word], mm);
			mov(t16(0), m16);
			mov(word_ptr [&word[c & 0x03]], t16(0));
			movq(mm, qword_ptr [word]);

			free(0);
			return 0;
		}
		else
		{
			return Assembler::pinsrw(mm, m16, c);
		}
	}

	Encoding *CodeGenerator::pinsrw(OperandMMREG mm, OperandR_M16 r_m16, unsigned char c)
	{
		if(r_m16.type == Operand::OPERAND_REG16) return pinsrw(mm, (OperandREG16)r_m16, c);
		else                             return pinsrw(mm, (OperandMEM16)r_m16, c);
	}

	Encoding *CodeGenerator::pmaxsw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxsw(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pmaxsw(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxsw(mm, m64);
		}
	}

	Encoding *CodeGenerator::pmaxsw(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxsw(mm, r_m64);
		}
	}

	Encoding *CodeGenerator::pmaxub(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxub(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pmaxub(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxub(mm, m64);
		}
	}

	Encoding *CodeGenerator::pmaxub(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pmaxub(mm, r_m64);
		}
	}

	Encoding *CodeGenerator::pminsw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminsw(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pminsw(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminsw(mm, m64);
		}
	}

	Encoding *CodeGenerator::pminsw(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminsw(mm, r_m64);
		}
	}

	Encoding *CodeGenerator::pminub(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminub(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pminub(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminub(mm, m64);
		}
	}

	Encoding *CodeGenerator::pminub(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(emulateSSE)
		{
			throw Error("Unimplemented SSE instruction emulation");
		}
		else
		{
			return Assembler::pminub(mm, r_m64);
		}
	}

	Encoding *CodeGenerator::pmulhuw(OperandMMREG mmi, OperandMMREG mmj)
	{
		if(emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr [word1], mmi);
			movq(qword_ptr [word2], mmj);
			push(eax);
			push(edx);

			mov(ax, word_ptr [&word1[0]]);
			mul(word_ptr [&word2[0]]);
			mov(word_ptr [&word1[0]], dx);

			mov(ax, word_ptr [&word1[1]]);
			mul(word_ptr [&word2[1]]);
			mov(word_ptr [&word1[1]], dx);

			mov(ax, word_ptr [&word1[2]]);
			mul(word_ptr [&word2[2]]);
			mov(word_ptr [&word1[2]], dx);

			mov(ax, word_ptr [&word1[3]]);
			mul(word_ptr [&word2[3]]);
			mov(word_ptr [&word1[3]], dx);

			pop(edx);
			pop(eax);
			movq(mmi, qword_ptr [word1]);

			return 0;
		}
		else
		{
			return Assembler::pmulhuw(mmi, mmj);
		}
	}

	Encoding *CodeGenerator::pmulhuw(OperandMMREG mm, OperandMEM64 m64)
	{
		if(emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr [word1], mm);
			movq(mm, m64);
			movq(qword_ptr [word2], mm);
			push(eax);
			push(edx);

			mov(ax, word_ptr [&word1[0]]);
			mul(word_ptr [&word2[0]]);
			mov(word_ptr [&word1[0]], dx);

			mov(ax, word_ptr [&word1[1]]);
			mul(word_ptr [&word2[1]]);
			mov(word_ptr [&word1[1]], dx);

			mov(ax, word_ptr [&word1[2]]);
			mul(word_ptr [&word2[2]]);
			mov(word_ptr [&word1[2]], dx);

			mov(ax, word_ptr [&word1[3]]);
			mul(word_ptr [&word2[3]]);
			mov(word_ptr [&word1[3]], dx);

			pop(edx);
			pop(eax);
			movq(mm, qword_ptr [word1]);

			return 0;
		}
		else
		{
			return Assembler::pmulhuw(mm, m64);
		}
	}

	Encoding *CodeGenerator::pmulhuw(OperandMMREG mm, OperandR_M64 r_m64)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return pmulhuw(mm, (OperandMMREG)r_m64);
		else                             return pmulhuw(mm, (OperandMEM64)r_m64);
	}

	Encoding *CodeGenerator::prefetchnta(OperandMEM mem)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return prefetchnta(mem);
		}
	}

	Encoding *CodeGenerator::prefetcht0(OperandMEM mem)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return prefetcht0(mem);
		}
	}

	Encoding *CodeGenerator::prefetcht1(OperandMEM mem)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return prefetcht1(mem);
		}
	}

	Encoding *CodeGenerator::prefetcht2(OperandMEM mem)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return prefetcht2(mem);
		}
	}

	Encoding *CodeGenerator::pshufw(OperandMMREG mmi, OperandMMREG mmj, unsigned char c)
	{
		if(c == 0xE4)
		{
			if(mmi == mmj) return 0;
			else return movq(mmi, mmj);
		}

		if(emulateSSE)
		{
			static short word1[4];
			static short word2[4];

			movq(qword_ptr [word1], mmj);

			mov(t16(0), word_ptr [&word1[(c >> 0) & 0x03]]);
			mov(word_ptr [&word2[0]], t16(0));

			mov(t16(0), word_ptr [&word1[(c >> 2) & 0x03]]);
			mov(word_ptr [&word2[1]], t16(0));

			mov(t16(0), word_ptr [&word1[(c >> 4) & 0x03]]);
			mov(word_ptr [&word2[2]], t16(0));

			mov(t16(0), word_ptr [&word1[(c >> 6) & 0x03]]);
			mov(word_ptr [&word2[3]], t16(0));

			movq(mmi, qword_ptr [word2]);

			free(0);
			return 0;
		}
		else
		{
			return Assembler::pshufw(mmi, mmj, c);
		}
	}

	Encoding *CodeGenerator::pshufw(OperandMMREG mm, OperandMEM64 m64, unsigned char c)
	{
		if(emulateSSE)
		{
			static short word[4];

			mov(t16(0), (OperandMEM16)(m64+((c>>0)&0x03)*2));
			mov(word_ptr [&word[0]], t16(0));

			mov(t16(0), (OperandMEM16)(m64+((c>>2)&0x03)*2));
			mov(word_ptr [&word[1]], t16(0));

			mov(t16(0), (OperandMEM16)(m64+((c>>4)&0x03)*2));
			mov(word_ptr [&word[2]], t16(0));

			mov(t16(0), (OperandMEM16)(m64+((c>>6)&0x03)*2));
			mov(word_ptr [&word[3]], t16(0));

			movq(mm, qword_ptr [word]);

			free(0);
			return 0;
		}
		else
		{
			return Assembler::pshufw(mm, m64, c);
		}
	}

	Encoding *CodeGenerator::pshufw(OperandMMREG mm, OperandR_M64 r_m64, unsigned char c)
	{
		if(r_m64.type == Operand::OPERAND_MMREG) return pshufw(mm, (OperandMMREG)r_m64, c);
		else                             return pshufw(mm, (OperandMEM64)r_m64, c);
	}

	Encoding *CodeGenerator::rcpps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][1]]);
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][2]]);
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][3]]);
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::rcpps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::rcpps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+0));
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+4));
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+8));
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+12));
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::rcpps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::rcpps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return rcpps(xmm, (OperandXMMREG)r_m128);
		else                               return rcpps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::rcpss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::rcpss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::rcpss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)mem32);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::rcpss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::rcpss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return rcpss(xmm, (OperandXMMREG)xmm32);
		else                              return rcpss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::rsqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][1]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][2]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][3]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::rsqrtps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::rsqrtps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+0));
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+4));
			fsqrt();
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+8));
			fsqrt();
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)(mem128+12));
			fsqrt();
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::rsqrtps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::rsqrtps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return rsqrtps(xmm, (OperandXMMREG)r_m128);
		else                               return rsqrtps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::rsqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv(dword_ptr [&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::rsqrtss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::rsqrtss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(dword_ptr [&one]);
			fdiv((OperandMEM32)mem32);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::rsqrtss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::rsqrtss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return rsqrtss(xmm, (OperandXMMREG)xmm32);
		else                              return rsqrtss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::sfence()
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return Assembler::sfence();
		}
	}

	Encoding *CodeGenerator::shufps(OperandXMMREG xmmi, OperandXMMREG xmmj, unsigned char c)
	{
		if(c == 0xE4)
		{
			if(xmmi == xmmj) return 0;
			else return movaps(xmmi, xmmj);
		}

		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[i][(c >> 0) & 0x03]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][(c >> 2) & 0x03]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][(c >> 4) & 0x03]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][(c >> 6) & 0x03]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::shufps(xmmi, xmmj, c);
		}
	}

	Encoding *CodeGenerator::shufps(OperandXMMREG xmm, OperandMEM128 m128, unsigned char c)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][(c >> 0) & 0x03]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][(c >> 2) & 0x03]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(m128+((c>>4)&0x03)*4));
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(m128+((c>>6)&0x03)*4));
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::shufps(xmm, m128, c);
		}
	}

	Encoding *CodeGenerator::shufps(OperandXMMREG xmm, OperandR_M128 r_m128, unsigned char c)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return shufps(xmm, (OperandXMMREG)r_m128, c);
		else                               return shufps(xmm, (OperandMEM128)r_m128, c);
	}

	Encoding *CodeGenerator::sqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[j][1]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[j][2]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[j][3]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::sqrtps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::sqrtps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld((OperandMEM32)(mem128+0));
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			fld((OperandMEM32)(mem128+4));
			fsqrt();
			fstp(dword_ptr [&sse[i][1]]);
			fld((OperandMEM32)(mem128+8));
			fsqrt();
			fstp(dword_ptr [&sse[i][2]]);
			fld((OperandMEM32)(mem128+12));
			fsqrt();
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::sqrtps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::sqrtps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return sqrtps(xmm, (OperandXMMREG)r_m128);
		else                               return sqrtps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::sqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[j][0]]);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::sqrtss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::sqrtss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			static float one = 1.0f;
			fld(mem32);
			fsqrt();
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::sqrtss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::sqrtss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return sqrtss(xmm, (OperandXMMREG)xmm32);
		else                              return sqrtss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::stmxcsr(OperandMEM32 m32)
	{
		if(emulateSSE)
		{
			return 0;
		}
		else
		{
			return Assembler::stmxcsr(m32);
		}
	}

	Encoding *CodeGenerator::subps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fsub(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fsub(dword_ptr [&sse[j][1]]);
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fsub(dword_ptr [&sse[j][2]]);
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fsub(dword_ptr [&sse[j][3]]);
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::subps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::subps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fsub((OperandMEM32)(mem128+0));
			fstp(dword_ptr [&sse[i][0]]);
			fld(dword_ptr [&sse[i][1]]);
			fsub((OperandMEM32)(mem128+4));
			fstp(dword_ptr [&sse[i][1]]);
			fld(dword_ptr [&sse[i][2]]);
			fsub((OperandMEM32)(mem128+8));
			fstp(dword_ptr [&sse[i][2]]);
			fld(dword_ptr [&sse[i][3]]);
			fsub((OperandMEM32)(mem128+12));
			fstp(dword_ptr [&sse[i][3]]);
			return 0;
		}
		else
		{
			return Assembler::subps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::subps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return subps(xmm, (OperandXMMREG)r_m128);
		else                               return subps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::subss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;
			fld(dword_ptr [&sse[i][0]]);
			fsub(dword_ptr [&sse[j][0]]);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::subss(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::subss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			fld(dword_ptr [&sse[i][0]]);
			fsub((OperandMEM32)mem32);
			fstp(dword_ptr [&sse[i][0]]);
			return 0;
		}
		else
		{
			return Assembler::subss(xmm, mem32);
		}
	}
	
	Encoding *CodeGenerator::subss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return subss(xmm, (OperandXMMREG)xmm32);
		else                              return subss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::ucomiss(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			fld(dword_ptr [&sse[j][0]]);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::ucomiss(xmmi, xmmj);
		}
	}

	Encoding *CodeGenerator::ucomiss(OperandXMMREG xmm, OperandMEM32 mem32)
	{
		if(emulateSSE)
		{
			spillMMX();
			const int i = xmm.reg;
			
			fld(mem32);
			fld(dword_ptr [&sse[i][0]]);
			fcomip(st0, st1);
			ffree(st0);

			return 0;
		}
		else
		{
			return Assembler::ucomiss(xmm, mem32);
		}
	}

	Encoding *CodeGenerator::ucomiss(OperandXMMREG xmm, OperandXMM32 xmm32)
	{
		if(xmm32.type == Operand::OPERAND_XMMREG) return ucomiss(xmm, (OperandXMMREG)xmm32);
		else                              return ucomiss(xmm, (OperandMEM32)xmm32);
	}

	Encoding *CodeGenerator::unpckhps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[i][2]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::unpckhps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::unpckhps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][2]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][3]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+8));
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+12));
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::unpckhps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::unpckhps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return unpckhps(xmm, (OperandXMMREG)r_m128);
		else                               return unpckhps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::unpcklps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][0]]);
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::unpcklps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::unpcklps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), dword_ptr [&sse[i][0]]);
			mov(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[i][1]]);
			mov(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+0));
			mov(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+4));
			mov(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::unpcklps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::unpcklps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return unpcklps(xmm, (OperandXMMREG)r_m128);
		else                               return unpcklps(xmm, (OperandMEM128)r_m128);
	}

	Encoding *CodeGenerator::xadd(OperandREG8 r8i, OperandREG8 r8j)
	{
		markModified(r8i);
		markModified(r8j);

		return Assembler::xadd(r8i, r8j);
	}

	Encoding *CodeGenerator::xadd(OperandREG16 r16i, OperandREG16 r16j)
	{
		markModified(r16i);
		markModified(r16j);

		return Assembler::xadd(r16i, r16j);
	}

	Encoding *CodeGenerator::xadd(OperandREG32 r32i, OperandREG32 r32j)
	{
		markModified(r32i);
		markModified(r32j);

		return Assembler::xadd(r32i, r32j);
	}

	Encoding *CodeGenerator::xadd(OperandR_M8 r_m8, OperandREG8 r8)
	{
		markModified(r_m8);
		markModified(r8);

		return Assembler::xadd(r_m8, r8);
	}

	Encoding *CodeGenerator::xadd(OperandR_M16 r_m16, OperandREG16 r16)
	{
		markModified(r_m16);
		markModified(r16);

		return Assembler::xadd(r_m16, r16);
	}

	Encoding *CodeGenerator::xadd(OperandR_M32 r_m32, OperandREG32 r32)
	{
		markModified(r_m32);
		markModified(r32);

		return Assembler::xadd(r_m32, r32);
	}

	Encoding *CodeGenerator::xchg(OperandREG8 r8i, OperandREG8 r8j)
	{
		markModified(r8i);
		markModified(r8j);

		return Assembler::xchg(r8i, r8j);
	}

	Encoding *CodeGenerator::xchg(OperandREG16 r16i, OperandREG16 r16j)
	{
		markModified(r16i);
		markModified(r16j);

		return Assembler::xchg(r16i, r16j);
	}

	Encoding *CodeGenerator::xchg(OperandREG32 r32i, OperandREG32 r32j)
	{
		markModified(r32i);
		markModified(r32j);

		return Assembler::xchg(r32i, r32j);
	}

	Encoding *CodeGenerator::xchg(OperandR_M8 r_m8, OperandREG8 r8)
	{
		markModified(r_m8);
		markModified(r8);

		return Assembler::xchg(r_m8, r8);
	}

	Encoding *CodeGenerator::xchg(OperandR_M16 r_m16, OperandREG16 r16)
	{
		markModified(r_m16);
		markModified(r16);

		return Assembler::xchg(r_m16, r16);
	}

	Encoding *CodeGenerator::xchg(OperandR_M32 r_m32, OperandREG32 r32)
	{
		markModified(r_m32);
		markModified(r32);

		return Assembler::xchg(r_m32, r32);
	}

	Encoding *CodeGenerator::xchg(OperandREG8 r8, OperandR_M8 r_m8)
	{
		markModified(r8);
		markModified(r_m8);

		return Assembler::xchg(r8, r_m8);
	}

	Encoding *CodeGenerator::xchg(OperandREG16 r16, OperandR_M16 r_m16)
	{
		markModified(r16);
		markModified(r_m16);

		return Assembler::xchg(r16, r_m16);
	}

	Encoding *CodeGenerator::xchg(OperandREG32 r32, OperandR_M32 r_m32)
	{
		markModified(r32);
		markModified(r_m32);

		return Assembler::xchg(r32, r_m32);
	}

	Encoding *CodeGenerator::lock_xadd(OperandMEM8 m8, OperandREG8 r8)
	{
		markModified(m8);
		markModified(r8);

		return Assembler::lock_xadd(m8, r8);
	}

	Encoding *CodeGenerator::lock_xadd(OperandMEM16 m16, OperandREG16 r16)
	{
		markModified(m16);
		markModified(r16);

		return Assembler::lock_xadd(m16, r16);
	}

	Encoding *CodeGenerator::lock_xadd(OperandMEM32 m32, OperandREG32 r32)
	{
		markModified(m32);
		markModified(r32);

		return Assembler::lock_xadd(m32, r32);
	}

	Encoding *CodeGenerator::lock_xchg(OperandMEM8 m8, OperandREG8 r8)
	{
		markModified(m8);
		markModified(r8);

		return Assembler::lock_xadd(m8, r8);
	}

	Encoding *CodeGenerator::lock_xchg(OperandMEM16 m16, OperandREG16 r16)
	{
		markModified(m16);
		markModified(r16);

		return Assembler::lock_xadd(m16, r16);
	}

	Encoding *CodeGenerator::lock_xchg(OperandMEM32 m32, OperandREG32 r32)
	{
		markModified(m32);
		markModified(r32);

		return Assembler::lock_xadd(m32, r32);
	}

	Encoding *CodeGenerator::xorps(OperandXMMREG xmmi, OperandXMMREG xmmj)
	{
		if(emulateSSE)
		{
			const int i = xmmi.reg;
			const int j = xmmj.reg;

			mov(t32(0), dword_ptr [&sse[j][0]]);
			xor(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][1]]);
			xor(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][2]]);
			xor(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), dword_ptr [&sse[j][3]]);
			xor(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::xorps(xmmi, xmmj);
		}
	}
	
	Encoding *CodeGenerator::xorps(OperandXMMREG xmm, OperandMEM128 mem128)
	{
		if(emulateSSE)
		{
			const int i = xmm.reg;

			mov(t32(0), (OperandMEM32)(mem128+0));
			xor(dword_ptr [&sse[i][0]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+4));
			xor(dword_ptr [&sse[i][1]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+8));
			xor(dword_ptr [&sse[i][2]], t32(0));

			mov(t32(0), (OperandMEM32)(mem128+12));
			xor(dword_ptr [&sse[i][3]], t32(0));

			free(0);
			return 0;
		}
		else
		{
			return Assembler::xorps(xmm, mem128);
		}
	}
	
	Encoding *CodeGenerator::xorps(OperandXMMREG xmm, OperandR_M128 r_m128)
	{
		if(r_m128.type == Operand::OPERAND_XMMREG) return xorps(xmm, (OperandXMMREG)r_m128);
		else                               return xorps(xmm, (OperandMEM128)r_m128);
	}

	void CodeGenerator::dumpSSE()
	{
		pushad();
		emms();

		static float sse[8][4];

		movups(xword_ptr [sse[0]], xmm0);
		movups(xword_ptr [sse[1]], xmm1);
		movups(xword_ptr [sse[2]], xmm2);
		movups(xword_ptr [sse[3]], xmm3);
		movups(xword_ptr [sse[4]], xmm4);
		movups(xword_ptr [sse[5]], xmm5);
		movups(xword_ptr [sse[6]], xmm6);
		movups(xword_ptr [sse[7]], xmm7);

		static FILE *file;
		static char *perm = "a";
		static char *name;

		if(emulateSSE)
		{
			name = "dumpEmulate.txt";
		}
		else
		{
			name = "dumpNative.txt";
		}

		mov(eax, dword_ptr [&perm]); 
		push(eax);
		mov(ecx, dword_ptr [&name]); 
		push(ecx);
		call((int)fopen);
		add(esp, 8);
		mov(dword_ptr [&file], eax);

		static char *string0 = "xmm0: %f, %f, %f, %f\n";
		static char *string1 = "xmm1: %f, %f, %f, %f\n";
		static char *string2 = "xmm2: %f, %f, %f, %f\n";
		static char *string3 = "xmm3: %f, %f, %f, %f\n";
		static char *string4 = "xmm4: %f, %f, %f, %f\n";
		static char *string5 = "xmm5: %f, %f, %f, %f\n";
		static char *string6 = "xmm6: %f, %f, %f, %f\n";
		static char *string7 = "xmm7: %f, %f, %f, %f\n";
		static char *newline = "\n";

		// fprintf(file, string0, sse[0][0], sse[0][1], sse[0][2], sse[0][3]);
		fld(dword_ptr [&sse[0][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[0][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[0][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[0][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string0]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string1, sse[1][0], sse[1][1], sse[1][2], sse[1][3]);
		fld(dword_ptr [&sse[1][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[1][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[1][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[1][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string1]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string2, sse[2][0], sse[2][1], sse[2][2], sse[2][3]);
		fld(dword_ptr [&sse[2][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[2][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[2][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[2][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string2]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string3, sse[3][0], sse[3][1], sse[3][2], sse[3][3]);
		fld(dword_ptr [&sse[3][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[3][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[3][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[3][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string3]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string4, sse[4][0], sse[4][1], sse[4][2], sse[4][3]);
		fld(dword_ptr [&sse[4][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[4][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[4][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[4][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string4]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string5, sse[5][0], sse[5][1], sse[5][2], sse[5][3]);
		fld(dword_ptr [&sse[5][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[5][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[5][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[5][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string5]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28);

		// fprintf(file, string6, sse[6][0], sse[6][1], sse[6][2], sse[6][3]);
		fld(dword_ptr [&sse[6][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[6][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[6][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[6][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string6]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, string7, sse[7][0], sse[7][1], sse[7][2], sse[7][3]);
		fld(dword_ptr [&sse[7][3]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[7][2]]);
		sub(esp, 8); 
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[7][1]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		fld(dword_ptr [&sse[7][0]]);
		sub(esp, 8);
		fstp(qword_ptr [esp]);
		mov(eax, dword_ptr [&string7]); 
		push(eax);
		mov(ecx, dword_ptr [&file]); 
		push(ecx);
		call((int)fprintf); 
		add(esp, 0x28); 

		// fprintf(file, newline);
		mov(eax, dword_ptr [&newline]);
		push(eax);
		mov(ecx, dword_ptr [&file]);
		push(ecx);
		call((int)fprintf); 
		add(esp, 8);

		// fclose(file);
		mov(eax, dword_ptr [&file]); 
		push(eax);
		call((int)fclose);
		add(esp, 4);

		popad();

	//	int3();
	}

	void CodeGenerator::markModified(const Operand &op)
	{
		if(emulateSSE) return;

		if(Operand::isReg(op))
		{
			if(op.type == Operand::OPERAND_REG32 ||
			   op.type == Operand::OPERAND_REG16 ||
			   op.type == Operand::OPERAND_REG8 ||
			   op.type == Operand::OPERAND_EAX ||
			   op.type == Operand::OPERAND_ECX ||
			   op.type == Operand::OPERAND_AX ||
			   op.type == Operand::OPERAND_DX ||
			   op.type == Operand::OPERAND_CX ||
			   op.type == Operand::OPERAND_AL ||
			   op.type == Operand::OPERAND_CL)
			{
				if(op.reg == Encoding::ESP || op.reg == Encoding::EBP)
				{
					return;
				}

				X32(op.reg).modified = true;
			}
			else if(op.type == Operand::OPERAND_MMREG)
			{
				X64(op.reg).modified = true;
			}
			else if(op.type == Operand::OPERAND_XMMREG)
			{
				X128(op.reg).modified = true;
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
	}

	void CodeGenerator::retainLoad(const Operand &op)
	{
		if(emulateSSE) return;

		if(Operand::isReg(op))
		{
			if(op.type == Operand::OPERAND_REG32 ||
			   op.type == Operand::OPERAND_REG16 ||
			   op.type == Operand::OPERAND_REG8 ||
			   op.type == Operand::OPERAND_EAX ||
			   op.type == Operand::OPERAND_ECX ||
			   op.type == Operand::OPERAND_AX ||
			   op.type == Operand::OPERAND_DX ||
			   op.type == Operand::OPERAND_CX ||
			   op.type == Operand::OPERAND_AL ||
			   op.type == Operand::OPERAND_CL)
			{
				if(op.reg == Encoding::ESP || op.reg == Encoding::EBP)
				{
					return;
				}

				if(X32(op.reg).load)
				{
					X32(op.reg).load->retain();
					X32(op.reg).load = 0;
				}
			}
			else if(op.type == Operand::OPERAND_MMREG)
			{
				if(X64(op.reg).load)
				{
					X64(op.reg).load->retain();
					X64(op.reg).load = 0;
				}
			}
			else if(op.type == Operand::OPERAND_XMMREG)
			{
				if(X128(op.reg).load)
				{
					X128(op.reg).load->retain();
					X128(op.reg).load = 0;
				}
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}
		else if(Operand::isMem(op))
		{
			if(op.baseReg != Encoding::REG_UNKNOWN)
			{
				retainLoad(OperandREG32(op.baseReg));
			}

			if(op.indexReg != Encoding::REG_UNKNOWN)
			{
				retainLoad(OperandREG32(op.indexReg));
			}
		}
	}

	Encoding *CodeGenerator::x86(int instructionID, const Operand &firstOperand, const Operand &secondOperand, const Operand &thirdOperand)
	{
		markModified(firstOperand);
		retainLoad(firstOperand);
		retainLoad(secondOperand);
		retainLoad(thirdOperand);

		active = this;

		return Assembler::x86(instructionID, firstOperand, secondOperand, thirdOperand);
	}

	void CodeGenerator::swapAllocation(Allocation *source, Allocation *destination)
	{
		OperandREF swapRef = source->reference;
		source->reference = destination->reference;
		destination->reference = swapRef;

		int swapPriority = source->priority;
		source->priority = destination->priority;
		destination->priority = swapPriority;

		bool swapModified = source->modified;
		source->modified = destination->modified;
		destination->modified = swapModified;

		int swapPartial = source->partial;
		source->partial = destination->partial;
		destination->partial = swapPartial;
	}

	CodeGenerator::Allocation &CodeGenerator::X32(const OperandREG32 &r32)
	{
		return X32(r32.reg);
	}

	CodeGenerator::Allocation &CodeGenerator::X64(const OperandMMREG &r64)
	{
		return X64(r64.reg);
	}

	CodeGenerator::Allocation &CodeGenerator::X128(const OperandXMMREG &r128)
	{
		return X128(r128.reg);
	}

	CodeGenerator::Allocation &CodeGenerator::X32(Encoding::Reg r32)
	{
		switch(r32)
		{
		case Encoding::EAX: return EAX;
		case Encoding::ECX: return ECX;
		case Encoding::EDX: return EDX;
		case Encoding::EBX: return EBX;
		case Encoding::ESI: return ESI;
		case Encoding::EDI: return EDI;
		default: throw INTERNAL_ERROR;
		}
	}

	CodeGenerator::Allocation &CodeGenerator::X64(Encoding::Reg r64)
	{
		switch(r64)
		{
		case Encoding::MM0: return MM0;
		case Encoding::MM1: return MM1;
		case Encoding::MM2: return MM2;
		case Encoding::MM3: return MM3;
		case Encoding::MM4: return MM4;
		case Encoding::MM5: return MM5;
		case Encoding::MM6: return MM6;
		case Encoding::MM7: return MM7;
		default: throw INTERNAL_ERROR;
		}
	}

	CodeGenerator::Allocation &CodeGenerator::X128(Encoding::Reg r128)
	{
		switch(r128)
		{
		case Encoding::XMM0: return XMM0;
		case Encoding::XMM1: return XMM1;
		case Encoding::XMM2: return XMM2;
		case Encoding::XMM3: return XMM3;
		case Encoding::XMM4: return XMM4;
		case Encoding::XMM5: return XMM5;
		case Encoding::XMM6: return XMM6;
		case Encoding::XMM7: return XMM7;
		default: throw INTERNAL_ERROR;
		}
	}

	const OperandREG8 &CodeGenerator::R8(Encoding::Reg r8)
	{
		switch(r8)
		{
		case Encoding::AL: return al;
		case Encoding::CL: return cl;
		case Encoding::DL: return dl;
		case Encoding::BL: return bl;
		default: throw INTERNAL_ERROR;
		}
	}

	const OperandREG16 &CodeGenerator::R16(Encoding::Reg r16)
	{
		switch(r16)
		{
		case Encoding::AX: return ax;
		case Encoding::CX: return cx;
		case Encoding::DX: return dx;
		case Encoding::BX: return bx;
		case Encoding::SI: return si;
		case Encoding::DI: return di;
		default: throw INTERNAL_ERROR;
		}
	}

	const OperandREG32 &CodeGenerator::R32(Encoding::Reg r32)
	{
		switch(r32)
		{
		case Encoding::EAX: return eax;
		case Encoding::ECX: return ecx;
		case Encoding::EDX: return edx;
		case Encoding::EBX: return ebx;
		case Encoding::ESI: return esi;
		case Encoding::EDI: return edi;
		default: throw INTERNAL_ERROR;
		}
	}

	const OperandMMREG &CodeGenerator::R64(Encoding::Reg r64)
	{
		switch(r64)
		{
		case Encoding::MM0: return mm0;
		case Encoding::MM1: return mm1;
		case Encoding::MM2: return mm2;
		case Encoding::MM3: return mm3;
		case Encoding::MM4: return mm4;
		case Encoding::MM5: return mm5;
		case Encoding::MM6: return mm6;
		case Encoding::MM7: return mm7;
		default: throw INTERNAL_ERROR;
		}
	}

	const OperandXMMREG &CodeGenerator::R128(Encoding::Reg r128)
	{
		switch(r128)
		{
		case Encoding::XMM0: return xmm0;
		case Encoding::XMM1: return xmm1;
		case Encoding::XMM2: return xmm2;
		case Encoding::XMM3: return xmm3;
		case Encoding::XMM4: return xmm4;
		case Encoding::XMM5: return xmm5;
		case Encoding::XMM6: return xmm6;
		case Encoding::XMM7: return xmm7;
		default: throw INTERNAL_ERROR;
		}
	}

	CodeGenerator *CodeGenerator::activeCG()
	{
		if(active)
		{
			return active;
		}
		else
		{
			// If you hit this exception, it means a run-time variable has been used before any other
			// run-time intrinsic has been called, so SoftWire doesn't know where to generate the binary code.
			// Just call any run-time intrinsic first to initialize the stack and avoid this error.
			throw Error("Stack uninitialized!");
		}
	}
}
