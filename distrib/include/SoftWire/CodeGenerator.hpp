#ifndef SoftWire_CodeGenerator_hpp
#define SoftWire_CodeGenerator_hpp

#undef free   // Defined by memory manager

#include "Assembler.hpp"

namespace SoftWire
{
	class CodeGenerator : public Assembler
	{
		struct Allocation
		{
			Allocation()
			{
				free();
			}

			void free()
			{
				reference = 0;
				priority = 0;
				copy = 0;
				load = 0;
				partial = 0;
				modified = false;
			}

			OperandREF reference;
			unsigned int priority;
			Encoding *copy;   // For copy propagation
			Encoding *load;   // For load-load elimination
			int partial;     // Number of bytes used, 0/1/2 for general-purpose, 0/4 for SSE, 0 means all
			bool modified;
		};

		class Variable
		{
		public:
			~Variable();

		protected:
			static int stack;
			int reference;
		};

	public:
		struct AllocationTable
		{
			Allocation eax;
			Allocation ecx;
			Allocation edx;
			Allocation ebx;
			Allocation esi;
			Allocation edi;

			Allocation mm0;
			Allocation mm1;
			Allocation mm2;
			Allocation mm3;
			Allocation mm4;
			Allocation mm5;
			Allocation mm6;
			Allocation mm7;

			Allocation xmm0;
			Allocation xmm1;
			Allocation xmm2;
			Allocation xmm3;
			Allocation xmm4;
			Allocation xmm5;
			Allocation xmm6;
			Allocation xmm7;
		};

		class Byte : public Variable
		{
		public:
			Byte();

			operator OperandREG8() const;
		};

		typedef Byte Char;

		class Word : public Variable
		{
		public:
			Word();

			operator OperandREG16() const;
		};

		typedef Word Byte2;
		typedef Word Char2;
		typedef Word Short;

		class Dword : public Variable
		{
		public:
			Dword();

			operator OperandREG32() const;
		};

		typedef Dword Byte4;
		typedef Dword Char4;
		typedef Dword Word2;
		typedef Dword Short2;
		typedef Dword Int;

		class Qword : public Variable
		{
		public:
			Qword();

			operator OperandMMREG() const;
		};

		typedef Qword Byte8;
		typedef Qword Char8;
		typedef Qword Word4;
		typedef Qword Short4;
		typedef Qword Dword2;
		typedef Qword Int2;

		class Float : public Variable
		{
		public:
			Float();

			operator OperandXMMREG() const;
		};

		class Float4 : public Variable
		{
		public:
			Float4();

			operator OperandXMMREG() const;
		};

		CodeGenerator();

		~CodeGenerator();

		// Register allocation
		const OperandREG8 r8(const OperandREF &ref, bool copy = true);
		const OperandREG8 x8(const OperandREF &ref, bool copy = false);
		const OperandREG8 t8(int i);
		const OperandR_M8 m8(const OperandREF &ref);

		const OperandREG16 r16(const OperandREF &ref, bool copy = true);
		const OperandREG16 x16(const OperandREF &ref, bool copy = false);
		const OperandREG16 t16(int i);
		const OperandR_M16 m16(const OperandREF &ref);

		const OperandREG32 &r32(const OperandREF &ref, bool copy = true, int partial = 0);
		const OperandREG32 &x32(const OperandREF &ref, bool copy = false, int partial = 0);
		const OperandREG32 &t32(int i, int partial = 0);
		const OperandR_M32 m32(const OperandREF &ref, int partial = 0);
		const OperandREG32 &allocate(const OperandREG32 &reg, const OperandREF &ref, int partial);
		const OperandREG32 &assign(const OperandREG32 &reg, const OperandREF &ref, bool copy, int partial);
		const OperandREG32 &access(const OperandREG32 &reg);
		void free(const OperandREG32 &reg);
		void spill(const OperandREG32 &reg);

		const OperandMMREG &r64(const OperandREF &ref, bool copy = true);
		const OperandMMREG &x64(const OperandREF &ref, bool copy = false);
		const OperandMMREG &t64(int i);
		const OperandR_M64 m64(const OperandREF &ref);
		const OperandMMREG &allocate(const OperandMMREG &reg, const OperandREF &ref);
		const OperandMMREG &assign(const OperandMMREG &reg, const OperandREF &ref, bool copy);
		const OperandMMREG &access(const OperandMMREG &reg);
		void free(const OperandMMREG &reg);
		void spill(const OperandMMREG &reg);

		const OperandXMMREG &r128(const OperandREF &ref, bool copy = true, bool ss = false);
		const OperandXMMREG &x128(const OperandREF &ref, bool copy = false, bool ss = false);
		const OperandXMMREG &t128(int i, bool ss = false);
		const OperandR_M128 m128(const OperandREF &ref, bool ss = false);
		const OperandXMMREG &allocate(const OperandXMMREG &reg, const OperandREF &ref, bool ss);
		const OperandXMMREG &assign(const OperandXMMREG &reg, const OperandREF &ref, bool copy, bool ss);
		const OperandXMMREG &access(const OperandXMMREG &reg);
		void free(const OperandXMMREG &reg);
		void spill(const OperandXMMREG &reg);

		const OperandXMMREG &rSS(const OperandREF &ref, bool copy = true, bool ss = true);
		const OperandXMMREG &xSS(const OperandREF &ref, bool copy = false, bool ss = true);
		const OperandXMMREG &tSS(int i, bool ss = true);
		const OperandXMM32 mSS(const OperandREF &ref, bool ss = true);

		bool real(const OperandREF &ref);

		const AllocationTable getAllocationState();

		void free(const OperandREF &ref);
		void spill(const OperandREF &ref);

		void freeAll();
		void spillAll(const AllocationTable &allocationState);   // Restore state to minimize spills
		void spillAll();
		void spillMMX();   // Specifically for using FPU after MMX
		void spillMMXcept(const OperandMMREG &reg);   // Empty MMX state but leave one associated

		// Optimization flags
		static void enableEmulateSSE();   // Default off
		static void disableEmulateSSE();

		static void enableCopyPropagation();   // Default on
		static void disableCopyPropagation();

		static void enableDropUnmodified();   // Default on
		static void disableDropUnmodified();

		static void enableSpillUnrelocated();   // Default off
		static void disableSpillUnrelocated();

		static void enableLoadLoadElimination();   // Default on
		static void disableLoadLoadElimination();

		// Overloaded to optimize or emulate
		Encoding *addps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *addps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *addps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *addss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *addss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *addss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *andnps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *andnps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *andnps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *andps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *andps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *andps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *cmpps(OperandXMMREG xmmi, OperandXMMREG xmmj, char c);
		Encoding *cmpps(OperandXMMREG xmm, OperandMEM128 mem128, char c);
		Encoding *cmpps(OperandXMMREG xmm, OperandR_M128 r_m128, char c);

		Encoding *cmpeqps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpeqps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpeqps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpleps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpleps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpleps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpltps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpltps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpltps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpneqps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpneqps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpneqps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpnleps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpnleps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpnleps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpnltps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpnltps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpnltps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpordps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpordps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpordps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *cmpunordps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpunordps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *cmpunordps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *cmpss(OperandXMMREG xmmi, OperandXMMREG xmmj, char c);
		Encoding *cmpss(OperandXMMREG xmm, OperandMEM32 mem32, char c);
		Encoding *cmpss(OperandXMMREG xmm, OperandXMM32 xmm32, char c);

		Encoding *cmpeqss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpeqss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpeqss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpless(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpless(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpless(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpltss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpltss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpltss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpneqss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpneqss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpneqss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpnless(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpnless(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpnless(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpnltss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpnltss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpnltss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpordss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpordss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpordss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *cmpunordss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *cmpunordss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cmpunordss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *cmpxchg(OperandR_M8 r_m8, OperandREG8 r8);
		Encoding *cmpxchg(OperandR_M16 r_m16, OperandREG16 r16);
		Encoding *cmpxchg(OperandR_M32 r_m32, OperandREG32 r32);

		Encoding *lock_cmpxchg(OperandMEM8 m8, OperandREG8 r8);
		Encoding *lock_cmpxchg(OperandMEM16 m16, OperandREG16 r16);
		Encoding *lock_cmpxchg(OperandMEM32 m32, OperandREG32 r32);

		Encoding *comiss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *comiss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *comiss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *cvtpi2ps(OperandXMMREG xmm, OperandMMREG mm);
		Encoding *cvtpi2ps(OperandXMMREG xmm, OperandMEM64 mem64);
		Encoding *cvtpi2ps(OperandXMMREG xmm, OperandR_M64 r_m64);

		Encoding *cvtps2pi(OperandMMREG mm, OperandXMMREG xmm);
		Encoding *cvtps2pi(OperandMMREG mm, OperandMEM64 mem64);
		Encoding *cvtps2pi(OperandMMREG mm, OperandXMM64 xmm64);

		Encoding *cvttps2pi(OperandMMREG mm, OperandXMMREG xmm);
		Encoding *cvttps2pi(OperandMMREG mm, OperandMEM64 mem64);
		Encoding *cvttps2pi(OperandMMREG mm, OperandXMM64 xmm64);

		Encoding *cvtsi2ss(OperandXMMREG xmm, OperandREG32 reg32);
		Encoding *cvtsi2ss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *cvtsi2ss(OperandXMMREG xmm, OperandR_M32 r_m32);

		Encoding *cvtss2si(OperandREG32 reg32, OperandXMMREG xmm);
		Encoding *cvtss2si(OperandREG32 reg32, OperandMEM32 mem32);
		Encoding *cvtss2si(OperandREG32 reg32, OperandXMM32 xmm32);

		Encoding *cvttss2si(OperandREG32 reg32, OperandXMMREG xmm);
		Encoding *cvttss2si(OperandREG32 reg32, OperandMEM32 mem32);
		Encoding *cvttss2si(OperandREG32 reg32, OperandXMM32 xmm32);

		Encoding *divps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *divps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *divps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *divss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *divss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *divss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *ldmxcsr(OperandMEM32 mem32);

		Encoding *maskmovq(OperandMMREG mmi, OperandMMREG mmj);

		Encoding *maxps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *maxps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *maxps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *maxss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *maxss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *maxss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *minps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *minps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *minps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *minss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *minss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *minss(OperandXMMREG xmm, OperandXMM32 xmm32);

		using Assembler::mov;
		Encoding *mov(OperandREG32 r32i, OperandREG32 r32j);
		Encoding *mov(OperandREG32 r32, OperandMEM32 m32);
		Encoding *mov(OperandREG32 r32, OperandR_M32 r_m32);

		Encoding *movaps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movaps(OperandXMMREG xmm, OperandMEM128 m128);
		Encoding *movaps(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *movaps(OperandMEM128 m128, OperandXMMREG xmm);
		Encoding *movaps(OperandR_M128 r_m128, OperandXMMREG xmm);

		Encoding *movhlps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movhps(OperandXMMREG xmm, OperandMEM64 m64);
		Encoding *movhps(OperandMEM64 m64, OperandXMMREG xmm);
		Encoding *movhps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movlhps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movlps(OperandXMMREG xmm, OperandMEM64 m64);
		Encoding *movlps(OperandMEM64 m64, OperandXMMREG xmm);

		Encoding *movmskps(OperandREG32 r32, OperandXMMREG xmm);

		Encoding *movntps(OperandMEM128 m128, OperandXMMREG xmm);
		Encoding *movntq(OperandMEM64 m64, OperandMMREG mm);

		Encoding *movq(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *movq(OperandMMREG mm, OperandMEM64 mem64);
		Encoding *movq(OperandMMREG mm, OperandR_M64 r_m64);
		Encoding *movq(OperandMEM64 mem64, OperandMMREG mm);
		Encoding *movq(OperandR_M64 r_m64, OperandMMREG mm);

		Encoding *movss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movss(OperandXMMREG xmm, OperandMEM32 m32);
		Encoding *movss(OperandXMMREG xmm, OperandXMM32 xmm32);
		Encoding *movss(OperandMEM32 m32, OperandXMMREG xmm);
		Encoding *movss(OperandXMM32 xmm32, OperandXMMREG xmm);

		Encoding *movups(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *movups(OperandXMMREG xmm, OperandMEM128 m128);
		Encoding *movups(OperandXMMREG xmm, OperandR_M128 r_m128);
		Encoding *movups(OperandMEM128 m128, OperandXMMREG xmm);
		Encoding *movups(OperandR_M128 r_m128, OperandXMMREG xmm);

		Encoding *mulps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *mulps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *mulps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *mulss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *mulss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *mulss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *orps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *orps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *orps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *pavgb(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pavgb(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pavgb(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pavgw(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pavgw(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pavgw(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pextrw(OperandREG32 r32, OperandMMREG mm, unsigned char c);
		Encoding *pinsrw(OperandMMREG mm, OperandREG16 r16, unsigned char c);
		Encoding *pinsrw(OperandMMREG mm, OperandMEM16 m16, unsigned char c);
		Encoding *pinsrw(OperandMMREG mm, OperandR_M16 r_m16, unsigned char c);

		Encoding *pmaxsw(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pmaxsw(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pmaxsw(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pmaxub(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pmaxub(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pmaxub(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pminsw(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pminsw(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pminsw(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pminub(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pminub(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pminub(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *pmulhuw(OperandMMREG mmi, OperandMMREG mmj);
		Encoding *pmulhuw(OperandMMREG mm, OperandMEM64 m64);
		Encoding *pmulhuw(OperandMMREG mm, OperandR_M64 r_m64);

		Encoding *prefetchnta(OperandMEM mem);
		Encoding *prefetcht0(OperandMEM mem);
		Encoding *prefetcht1(OperandMEM mem);
		Encoding *prefetcht2(OperandMEM mem);

		Encoding *pshufw(OperandMMREG mmi, OperandMMREG mmj, unsigned char c);
		Encoding *pshufw(OperandMMREG mm, OperandMEM64 m64, unsigned char c);
		Encoding *pshufw(OperandMMREG mm, OperandR_M64 r_m64, unsigned char c);

		Encoding *rcpps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *rcpps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *rcpps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *rcpss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *rcpss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *rcpss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *rsqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *rsqrtps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *rsqrtps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *rsqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *rsqrtss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *rsqrtss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *sfence();

		Encoding *shufps(OperandXMMREG xmmi, OperandXMMREG xmmj, unsigned char c);
		Encoding *shufps(OperandXMMREG xmm, OperandMEM128 m128, unsigned char c);
		Encoding *shufps(OperandXMMREG xmm, OperandR_M128 r_m128, unsigned char c);

		Encoding *sqrtps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *sqrtps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *sqrtps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *sqrtss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *sqrtss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *sqrtss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *stmxcsr(OperandMEM32 m32);

		Encoding *subps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *subps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *subps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *subss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *subss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *subss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *ucomiss(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *ucomiss(OperandXMMREG xmm, OperandMEM32 mem32);
		Encoding *ucomiss(OperandXMMREG xmm, OperandXMM32 xmm32);

		Encoding *unpckhps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *unpckhps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *unpckhps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *unpcklps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *unpcklps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *unpcklps(OperandXMMREG xmm, OperandR_M128 r_m128);

		Encoding *xadd(OperandREG8 r8i, OperandREG8 r8j);
		Encoding *xadd(OperandREG16 r16i, OperandREG16 r16j);
		Encoding *xadd(OperandREG32 r32i, OperandREG32 r32j);
		Encoding *xadd(OperandR_M8 r_m8, OperandREG8 r8);
		Encoding *xadd(OperandR_M16 r_m16, OperandREG16 r16);
		Encoding *xadd(OperandR_M32 r_m32, OperandREG32 r32);
		Encoding *xchg(OperandREG8 r8i, OperandREG8 r8j);
		Encoding *xchg(OperandREG16 r16i, OperandREG16 r16j);
		Encoding *xchg(OperandREG32 r32i, OperandREG32 r32j);
		Encoding *xchg(OperandR_M8 r_m8, OperandREG8 r8);
		Encoding *xchg(OperandR_M16 r_m16, OperandREG16 r16);
		Encoding *xchg(OperandR_M32 r_m32, OperandREG32 r32);
		Encoding *xchg(OperandREG8 r8, OperandR_M8 r_m8);
		Encoding *xchg(OperandREG16 r16, OperandR_M16 r_m16);
		Encoding *xchg(OperandREG32 r32, OperandR_M32 r_m32);

		Encoding *lock_xadd(OperandMEM8 m8, OperandREG8 r8);
		Encoding *lock_xadd(OperandMEM16 m16, OperandREG16 r16);
		Encoding *lock_xadd(OperandMEM32 m32, OperandREG32 r32);
		Encoding *lock_xchg(OperandMEM8 m8, OperandREG8 r8);
		Encoding *lock_xchg(OperandMEM16 m16, OperandREG16 r16);
		Encoding *lock_xchg(OperandMEM32 m32, OperandREG32 r32);

		Encoding *xorps(OperandXMMREG xmmi, OperandXMMREG xmmj);
		Encoding *xorps(OperandXMMREG xmm, OperandMEM128 mem128);
		Encoding *xorps(OperandXMMREG xmm, OperandR_M128 r_m128);

		// Debugging tools
		void dumpSSE();

		// Active code generator, used for variables stack
		static CodeGenerator *activeCG();

	private:
		// Overloaded to detect modified/unmodified registers
		virtual Encoding *x86(int instructionID,
		                      const Operand &firstOperand = Operand::OPERAND_VOID,
		                      const Operand &secondOperand = Operand::OPERAND_VOID,
		                      const Operand &thirdOperand = Operand::OPERAND_VOID);

		void markModified(const Operand &op);   // Used to prevent spilling of unmodified registers
		void retainLoad(const Operand &op);   // Retain the instruction that loads this register
		void swapAllocation(Allocation *source, Allocation *destination);   // Used for copy propagation

		// Get allocation data of corresponding operand
		static Allocation &X32(const OperandREG32 &r32);
		static Allocation &X64(const OperandMMREG &r64);
		static Allocation &X128(const OperandXMMREG &r128);
		static Allocation &X32(Encoding::Reg r32);
		static Allocation &X64(Encoding::Reg r64);
		static Allocation &X128(Encoding::Reg r128);

		// Get register of corresponding operand
		static const OperandREG8 &R8(Encoding::Reg r8);
		static const OperandREG16 &R16(Encoding::Reg r16);
		static const OperandREG32 &R32(Encoding::Reg r32);
		static const OperandMMREG &R64(Encoding::Reg r64);
		static const OperandXMMREG &R128(Encoding::Reg r128);

		static CodeGenerator *active;

		// Current allocation data
		static Allocation EAX;
		static Allocation ECX;
		static Allocation EDX;
		static Allocation EBX;
		static Allocation ESI;
		static Allocation EDI;

		static Allocation MM0;
		static Allocation MM1;
		static Allocation MM2;
		static Allocation MM3;
		static Allocation MM4;
		static Allocation MM5;
		static Allocation MM6;
		static Allocation MM7;

		static Allocation XMM0;
		static Allocation XMM1;
		static Allocation XMM2;
		static Allocation XMM3;
		static Allocation XMM4;
		static Allocation XMM5;
		static Allocation XMM6;
		static Allocation XMM7;

		static float sse[8][4];   // Storage for SSE emulation registers

		static bool emulateSSE;
		static bool copyPropagation;
		static bool dropUnmodified;
		static bool spillUnrelocated;
		static bool loadLoadElimination;
	};
}

#endif   // SoftWire_CodeGenerator_hpp
