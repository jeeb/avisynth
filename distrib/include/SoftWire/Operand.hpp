#ifndef SoftWire_Operand_hpp
#define SoftWire_Operand_hpp

#include "Encoding.hpp"
#include "Error.hpp"

#undef NEAR
#undef VOID

namespace SoftWire
{
	struct Specifier
	{
		enum Type
		{
			UNKNOWN = 0,

			NEAR,
			SHORT = NEAR,
		//	FAR,
			
			BYTE,
			WORD,
			DWORD,
		//	TWORD,   // 80-bit long double not supported
			QWORD,
			MMWORD = QWORD,
			XMMWORD,
			XWORD = XMMWORD,
			OWORD = XMMWORD,

			PTR
		};

		Type type;
		const char *notation;

		static Specifier::Type scan(const char *string);

		static const Specifier specifierSet[];
	};

	struct OperandREG;

	struct Operand
	{
		enum Type
		{
			UNKNOWN	= 0,

			VOID	= 0x00000001,

			ONE		= 0x00000002,
			EXT8	= 0x00000004 | ONE,   // Sign extended
			REF		= 0x00000008,
			IMM8	= 0x00000010 | EXT8 | ONE,
			IMM16	= 0x00000020 | IMM8 | EXT8 | ONE,
			IMM32	= 0x00000040 | REF | IMM16 | IMM8 | EXT8 | ONE,
			IMM		= IMM32 | IMM16 | IMM8 | EXT8 | ONE,

			AL		= 0x00000080,
			CL		= 0x00000100,
			REG8	= CL | AL,

			AX		= 0x00000200,
			DX		= 0x00000400,
			CX		= 0x00000800,
			REG16	= CX | DX | AX,

			EAX		= 0x00001000,
			ECX		= 0x00002000,
			REG32	= ECX | EAX,

			CS		= 0,   // No need to touch these in 32-bit protected mode
			DS		= 0,
			ES		= 0,
			SS		= 0,
			FS		= 0,
			GS		= 0,
			SEGREG	= GS | FS | SS | ES | DS | CS,

			ST0		= 0x00004000,
			FPUREG	= 0x00008000 | ST0,
			
			CR		= 0,   // You won't need these in a JIT assembler
			DR		= 0,
			TR		= 0,

			MMREG	= 0x00010000,
			XMMREG	= 0x00020000,

			REG		= XMMREG | MMREG | TR | DR | CR | FPUREG | SEGREG | REG32 | REG16 | REG8,

			MEM8	= 0x00040000,
			MEM16	= 0x00080000,
			MEM32	= 0x00100000,
			MEM64	= 0x00200000,
			MEM80	= 0,   // Extended double not supported by NT
			MEM128	= 0x00400000,
			MEM		= MEM128 | MEM80 | MEM64 | MEM32 | MEM16 | MEM8,
		
			XMM32	= MEM32 | XMMREG,
			XMM64	= MEM64 | XMMREG,

			R_M8	= MEM8 | REG8,
			R_M16	= MEM16 | REG16,
			R_M32	= MEM32 | REG32,
			R_M64	= MEM64 | MMREG,
			R_M128	= MEM128 | XMMREG,
			R_M		= MEM | REG,

			MOFFS8	= 0,   // Not supported, equivalent available
			MOFFS16	= 0,   // Not supported, equivalent available
			MOFFS32	= 0,   // Not supported, equivalent available

			STR		= 0x00800000 | REF
		};

		Type type;
		const char *notation;

		union
		{
			int value;   // For immediates
			Encoding::Reg reg;   // For registers
			Encoding::Reg baseReg;   // For memory references;
		};

		Encoding::Reg indexReg;
		int scale;
		int displacement;

		bool operator==(Operand &op)
		{
			return type == op.type &&
			       baseReg == op.baseReg &&
				   indexReg == op.indexReg &&
				   scale == op.scale &&
				   displacement == op.displacement;
		}

		bool operator!=(Operand &op)
		{
			return type != op.type ||
			       baseReg != op.baseReg ||
				   indexReg != op.indexReg ||
				   scale != op.scale ||
				   displacement != op.displacement;
		}

		static bool isSubtypeOf(Type type, Type baseType);
		bool isSubtypeOf(Type baseType) const;

		const char *string() const;

		static bool isVoid(Type type);
		static bool isImm(Type type);
		static bool isReg(Type type);
		static bool isMem(Type type);
		static bool isR_M(Type type);
		static bool isStr(Type type);

		static bool isVoid(const Operand &operand);
		static bool isImm(const Operand &operand);
		static bool isReg(const Operand &operand);
		static bool isMem(const Operand &operand);
		static bool isR_M(const Operand &operand);
		static bool isStr(const Operand &operand);

		static OperandREG scanReg(const char *string);

		const char *regName() const;
		const char *indexName() const;

		static Operand::Type scanSyntax(const char *string);

		static const Operand registerSet[];
		static const Operand syntaxSet[];

		static const Operand INIT;
		static const Operand NOT_FOUND;
	};

	struct OperandVOID : virtual Operand
	{
		OperandVOID()
		{
			type = Operand::VOID;
		}
	};

	struct OperandIMM : virtual Operand
	{
		OperandIMM(int imm = 0)
		{
			type = Operand::IMM;
			value = imm;
			notation = 0;
		}
	};

	struct OperandREF : virtual Operand
	{
		OperandREF(const void *ref = 0)
		{
			type = Operand::REF;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = (int)ref;
			notation = 0;
		}

		OperandREF(const char *ref)
		{
			type = Operand::IMM;
			notation = ref;
		}

		OperandREF(int ref)
		{
			type = Operand::REF;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = ref;
			notation = 0;
		}

		const OperandREF operator+(const void *disp) const
		{
			OperandREF returnReg;

			returnReg.baseReg = baseReg;
			returnReg.indexReg = indexReg;
			returnReg.scale = scale;
			returnReg.displacement = displacement + (int)disp;

			return returnReg;
		}

		const OperandREF operator+(int disp) const
		{
			OperandREF returnReg;

			returnReg.baseReg = baseReg;
			returnReg.indexReg = indexReg;
			returnReg.scale = scale;
			returnReg.displacement = displacement + disp;

			return returnReg;
		}

		const OperandREF operator-(int disp) const
		{
			OperandREF returnReg;

			returnReg.baseReg = baseReg;
			returnReg.indexReg = indexReg;
			returnReg.scale = scale;
			returnReg.displacement = displacement - disp;

			return returnReg;
		}

		bool operator==(const OperandREF &ref) const
		{
			return baseReg == ref.baseReg &&
			       indexReg == ref.indexReg &&
				   scale == ref.scale &&
				   displacement == ref.displacement;
		}

		bool operator!=(const OperandREF &ref) const
		{
			return !(*this == ref);
		}
	};

	struct OperandMEM : virtual Operand
	{
		OperandMEM()
		{
			type = Operand::MEM;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandMEM(const OperandREF &ref)
		{
			type = Operand::MEM;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM operator[](const OperandREF &ref) const
		{
			return OperandMEM(ref);
		}
	};

	struct OperandMEM8 : OperandMEM
	{
		OperandMEM8() {};

		OperandMEM8(const OperandREF &ref)
		{
			type = Operand::MEM8;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM8 operator[](const OperandREF &ref) const
		{
			return OperandMEM8(ref);
		}
	};

	struct OperandMEM16 : OperandMEM
	{
		OperandMEM16() {};

		OperandMEM16(const OperandREF &ref)
		{
			type = Operand::MEM16;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM16 operator[](const OperandREF &ref) const
		{
			return OperandMEM16(ref);
		}
	};

	struct OperandMEM32 : OperandMEM
	{
		OperandMEM32() {};

		OperandMEM32(const OperandREF &ref)
		{
			type = Operand::MEM32;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM32 operator[](const OperandREF &ref) const
		{
			return OperandMEM32(ref);
		}
	};

	struct OperandMEM64 : OperandMEM
	{
		OperandMEM64() {};

		OperandMEM64(const OperandREF &ref)
		{
			type = Operand::MEM64;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM64 operator[](const OperandREF &ref) const
		{
			return OperandMEM64(ref);
		}
	};

	struct OperandMEM128 : OperandMEM
	{
		OperandMEM128() {};

		OperandMEM128(const OperandREF &ref)
		{
			type = Operand::MEM128;
			baseReg = ref.baseReg;
			indexReg = ref.indexReg;
			scale = ref.scale;
			displacement = ref.displacement;
			notation = ref.notation;
		}

		OperandMEM128 operator[](const OperandREF &ref) const
		{
			return OperandMEM128(ref);
		}
	};

	struct OperandR_M8 : virtual Operand
	{
		OperandR_M8()
		{
			type = Operand::R_M8;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandR_M8(const OperandMEM8 &mem)
		{
			type = Operand::MEM8;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandR_M16 : virtual Operand
	{
		OperandR_M16()
		{
			type = Operand::R_M16;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandR_M16(const OperandMEM16 &mem)
		{
			type = Operand::MEM16;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandR_M32 : virtual Operand
	{
		OperandR_M32()
		{
			type = Operand::R_M32;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandR_M32(const OperandMEM32 &mem)
		{
			type = Operand::MEM32;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandR_M64 : virtual Operand
	{
		OperandR_M64()
		{
			type = Operand::R_M64;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandR_M64(const OperandMEM64 &mem)
		{
			type = Operand::MEM64;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandR_M128 : virtual Operand
	{
		OperandR_M128()
		{
			type = Operand::R_M128;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandR_M128(const OperandMEM128 &mem)
		{
			type = Operand::MEM128;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandXMM32 : virtual Operand
	{
		OperandXMM32()
		{
			type = Operand::XMM32;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandXMM32(const OperandMEM32 &mem)
		{
			type = Operand::MEM32;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandXMM64 : virtual Operand
	{
		OperandXMM64()
		{
			type = Operand::XMM64;
			baseReg = Encoding::REG_UNKNOWN;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		OperandXMM64(const OperandMEM64 &mem)
		{
			type = Operand::MEM64;
			baseReg = mem.baseReg;
			indexReg = mem.indexReg;
			scale = mem.scale;
			displacement = mem.displacement;
			notation = mem.notation;
		}
	};

	struct OperandREG : virtual Operand
	{
		OperandREG(const Operand &reg = Operand::INIT)
		{
			type = reg.type;
			this->reg = reg.reg;
			notation = reg.notation;
		}
	};

	struct OperandREG8 : OperandR_M8, OperandREG
	{
		OperandREG8(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::REG8;
			this->reg = reg;
			notation = 0;
		}
	};

	struct OperandREG16 : OperandR_M16, OperandREG
	{
		OperandREG16(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::REG16;
			this->reg = reg;
			notation = 0;
		}
	};

	struct OperandREG32;

	struct OperandREGxX : OperandREF
	{
		OperandREGxX(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::REG32;
			this->reg = reg;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		friend const OperandREF operator+(const OperandREGxX &ref1, const OperandREG32 &ref2);
		friend const OperandREGxX operator+(const OperandREGxX &ref, void *disp);
		friend const OperandREGxX operator+(const OperandREGxX &ref, int disp);
		friend const OperandREGxX operator-(const OperandREGxX &ref, int disp);

		friend const OperandREF operator+(const OperandREG32 &ref2, const OperandREGxX &ref1);
		friend const OperandREGxX operator+(void *disp, const OperandREGxX &ref);
		friend const OperandREGxX operator+(int disp, const OperandREGxX &ref);
		friend const OperandREGxX operator-(int disp, const OperandREGxX &ref);
	};

	struct OperandREG32 : OperandR_M32, OperandREF, OperandREG
	{
		OperandREG32(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::REG32;
			this->reg = reg;
			indexReg = Encoding::REG_UNKNOWN;
			scale = 0;
			displacement = 0;
			notation = 0;
		}

		friend const OperandREF operator+(const OperandREG32 ref, const OperandREG32 &ref2);

		friend const OperandREG32 operator+(const OperandREG32 ref, void *disp);
		friend const OperandREG32 operator+(const OperandREG32 ref, int disp);
		friend const OperandREG32 operator-(const OperandREG32 ref, int disp);
		friend const OperandREGxX operator*(const OperandREG32 ref, int scale);

		friend const OperandREG32 operator+(void *disp, const OperandREG32 ref);
		friend const OperandREG32 operator+(int disp, const OperandREG32 ref);
		friend const OperandREG32 operator-(int disp, const OperandREG32 ref);
		friend const OperandREGxX operator*(int scale, const OperandREG32 ref);
	};

	struct OperandFPUREG : virtual Operand, OperandREG
	{
		OperandFPUREG(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::FPUREG;
			this->reg = reg;
			notation = 0;
		}
	};

	struct OperandMMREG : OperandR_M64, OperandREG
	{
		OperandMMREG(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::MMREG;
			this->reg = reg;
			notation = 0;
		}
	};

	struct OperandXMMREG : OperandR_M128, OperandXMM32, OperandXMM64, OperandREG
	{
		OperandXMMREG(Encoding::Reg reg = Encoding::REG_UNKNOWN)
		{
			type = Operand::XMMREG;
			this->reg = reg;
			notation = 0;
		}
	};

	struct OperandAL : OperandREG8
	{
		OperandAL()
		{
			type = Operand::AL;
			reg = Encoding::AL;
			notation = "al";
		}
	};

	struct OperandCL : OperandREG8
	{
		OperandCL()
		{
			type = Operand::CL;
			reg = Encoding::CL;
			notation = "cl";
		}
	};

	struct OperandAX : OperandREG16
	{
		OperandAX()
		{
			type = Operand::AX;
			reg = Encoding::AX;
			notation = "ax";
		}
	};

	struct OperandDX : OperandREG16
	{
		OperandDX()
		{
			type = Operand::DX;
			reg = Encoding::DX;
			notation = "dx";
		}
	};

	struct OperandCX : OperandREG16
	{
		OperandCX()
		{
			type = Operand::CX;
			reg = Encoding::CX;
			notation = "cx";
		}
	};

	struct OperandEAX : OperandREG32
	{
		OperandEAX()
		{
			type = Operand::EAX;
			reg = Encoding::EAX;
			notation = "eax";
		}
	};

	struct OperandECX : OperandREG32
	{
		OperandECX()
		{
			type = Operand::ECX;
			reg = Encoding::ECX;
			notation = "ecx";
		}
	};

	struct OperandST0 : OperandFPUREG
	{
		OperandST0()
		{
			type = Operand::ST0;
			reg = Encoding::ST0;
			notation = "st0";
		}
	};

	struct OperandSTR : virtual Operand
	{
		OperandSTR(const char *string)
		{
			type = Operand::STR;
			notation = string;
		}
	};

	static const OperandVOID VOID;

	static const OperandAL al;
	static const OperandCL cl;
	static const OperandREG8 reg8;
	static const OperandREG8 dl(Encoding::DL);
	static const OperandREG8 bl(Encoding::BL);
	static const OperandREG8 ah(Encoding::AH);
	static const OperandREG8 ch(Encoding::CH);
	static const OperandREG8 dh(Encoding::DH);
	static const OperandREG8 bh(Encoding::BH);

	static const OperandAX ax;
	static const OperandCX cx;
	static const OperandDX dx;
	static const OperandREG16 reg16;
	static const OperandREG16 bx(Encoding::BX);
	static const OperandREG16 sp(Encoding::SP);
	static const OperandREG16 bp(Encoding::BP);
	static const OperandREG16 si(Encoding::SI);
	static const OperandREG16 di(Encoding::DI);

	static const OperandEAX eax;
	static const OperandECX ecx;
	static const OperandREG32 reg32;
	static const OperandREG32 edx(Encoding::EDX);
	static const OperandREG32 ebx(Encoding::EBX);
	static const OperandREG32 esp(Encoding::ESP);
	static const OperandREG32 ebp(Encoding::EBP);
	static const OperandREG32 esi(Encoding::ESI);
	static const OperandREG32 edi(Encoding::EDI);

	static const OperandST0 st;
	static const OperandST0 st0;
	static const OperandFPUREG fpureg;
	static const OperandFPUREG st1(Encoding::ST1);
	static const OperandFPUREG st2(Encoding::ST2);
	static const OperandFPUREG st3(Encoding::ST3);
	static const OperandFPUREG st4(Encoding::ST4);
	static const OperandFPUREG st5(Encoding::ST5);
	static const OperandFPUREG st6(Encoding::ST6);
	static const OperandFPUREG st7(Encoding::ST7);

	static const OperandMMREG mmreg;
	static const OperandMMREG mm0(Encoding::MM0);
	static const OperandMMREG mm1(Encoding::MM1);
	static const OperandMMREG mm2(Encoding::MM2);
	static const OperandMMREG mm3(Encoding::MM3);
	static const OperandMMREG mm4(Encoding::MM4);
	static const OperandMMREG mm5(Encoding::MM5);
	static const OperandMMREG mm6(Encoding::MM6);
	static const OperandMMREG mm7(Encoding::MM7);

	static const OperandXMMREG xmmreg;
	static const OperandXMMREG xmm0(Encoding::XMM0);
	static const OperandXMMREG xmm1(Encoding::XMM1);
	static const OperandXMMREG xmm2(Encoding::XMM2);
	static const OperandXMMREG xmm3(Encoding::XMM3);
	static const OperandXMMREG xmm4(Encoding::XMM4);
	static const OperandXMMREG xmm5(Encoding::XMM5);
	static const OperandXMMREG xmm6(Encoding::XMM6);
	static const OperandXMMREG xmm7(Encoding::XMM7);

	static const OperandMEM mem;
	static const OperandMEM8 mem8;
	static const OperandMEM16 mem16;
	static const OperandMEM32 mem32;
	static const OperandMEM64 mem64;
	static const OperandMEM128 mem128;

	static const OperandMEM8 byte;
	static const OperandMEM16 word;
	static const OperandMEM32 dword;
	static const OperandMEM64 mmword;
	static const OperandMEM64 qword;
	static const OperandMEM128 xmmword;
	static const OperandMEM128 xword;

	static const OperandMEM8 byte_ptr;
	static const OperandMEM16 word_ptr;
	static const OperandMEM32 dword_ptr;
	static const OperandMEM64 mmword_ptr;
	static const OperandMEM64 qword_ptr;
	static const OperandMEM128 xmmword_ptr;
	static const OperandMEM128 xword_ptr;
}

namespace SoftWire
{
	inline const OperandREF operator+(const OperandREGxX &ref1, const OperandREG32 &ref2)
	{
		OperandREF returnReg;

		returnReg.baseReg = ref2.baseReg;
		returnReg.indexReg = ref1.indexReg;
		returnReg.scale = ref1.scale;
		returnReg.displacement = ref1.displacement + ref2.displacement;

		return returnReg;
	}

	inline const OperandREGxX operator+(const OperandREGxX &ref, void *disp)
	{
		OperandREGxX returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement + (int)disp;

		return returnReg;
	}

	inline const OperandREGxX operator+(const OperandREGxX &ref, int disp)
	{
		OperandREGxX returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement + disp;

		return returnReg;
	}

	inline const OperandREGxX operator-(const OperandREGxX &ref, int disp)
	{
		OperandREGxX returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement - disp;

		return returnReg;
	}

	inline const OperandREF operator+(const OperandREG32 &ref2, const OperandREGxX &ref1)
	{
		return ref1 + ref2;
	}

	inline const OperandREGxX operator+(void *disp, const OperandREGxX &ref)
	{
		return ref + disp;
	}

	inline const OperandREGxX operator+(int disp, const OperandREGxX &ref)
	{
		return ref + disp;
	}

	inline const OperandREGxX operator-(int disp, const OperandREGxX &ref)
	{
		return ref + disp;
	}

	inline const OperandREF operator+(const OperandREG32 ref1, const OperandREG32 &ref2)
	{
		OperandREF returnReg;

		returnReg.baseReg = ref1.baseReg;
		returnReg.indexReg = ref2.indexReg;
		returnReg.scale = 1;
		returnReg.displacement = ref1.displacement + ref2.displacement;

		return returnReg;
	}

	inline const OperandREG32 operator+(const OperandREG32 ref, void *disp)
	{
		OperandREG32 returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement + (int)disp;

		return returnReg;
	}

	inline const OperandREG32 operator+(const OperandREG32 ref, int disp)
	{
		OperandREG32 returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement + disp;

		return returnReg;
	}

	inline const OperandREG32 operator-(const OperandREG32 ref, int disp)
	{
		OperandREG32 returnReg;

		returnReg.baseReg = ref.baseReg;
		returnReg.indexReg = ref.indexReg;
		returnReg.scale = ref.scale;
		returnReg.displacement = ref.displacement - disp;

		return returnReg;
	}

	inline const OperandREGxX operator*(const OperandREG32 ref, int scale)
	{
		OperandREGxX returnReg;

		returnReg.baseReg = Encoding::REG_UNKNOWN;
		returnReg.indexReg = ref.baseReg;
		returnReg.scale = scale;
		returnReg.displacement = ref.displacement;

		return returnReg;
	}

	inline const OperandREG32 operator+(void *disp, const OperandREG32 ref)
	{
		return ref + disp;
	}

	inline const OperandREG32 operator+(int disp, const OperandREG32 ref)
	{
		return ref + disp;
	}

	inline const OperandREG32 operator-(int disp, const OperandREG32 ref)
	{
		return ref - disp;
	}

	inline const OperandREGxX operator*(int scale, const OperandREG32 ref)
	{
		return ref * scale;
	}
}

#endif   // SoftWire_Operand_hpp
