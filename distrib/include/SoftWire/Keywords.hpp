#ifndef Keywords_hpp
#define Keywords_hpp

#include "Encoding.hpp"

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

	struct Operand
	{
		enum Type
		{
			UNKNOWN	= 0,

			VOID	= 0x00000001,

			ONE		= 0x00000002,
			EXT8	= 0x00000004 | ONE,   // Sign extended
			IMM8	= 0x00000008 | EXT8 | ONE,
			IMM16	= 0x00000010 | IMM8 | EXT8 | ONE,
			IMM32	= 0x00000020 | IMM16 | IMM8 | EXT8 | ONE,
			IMM		= IMM32 | IMM16 | IMM8 | EXT8 | ONE,

			AL		= 0x00000040,
			CL		= 0x00000080,
			REG8	= CL | AL,

			AX		= 0x00000100,
			DX		= 0x00000200,
			CX		= 0x00000400,
			REG16	= CX | DX | AX,

			EAX		= 0x00000800,
			ECX		= 0x00001000,
			REG32	= ECX | EAX,

			CS		= UNKNOWN,   // No need to touch these in 32-bit protected mode
			DS		= UNKNOWN,
			ES		= UNKNOWN,
			SS		= UNKNOWN,
			FS		= UNKNOWN,
			GS		= UNKNOWN,
			SEGREG	= GS | FS | SS | ES | DS | CS,

			ST0		= 0x00002000,
			FPUREG	= 0x00004000 | ST0,
			
			CR		= UNKNOWN,   // You won't need these in a JIT assembler
			DR		= UNKNOWN,
			TR		= UNKNOWN,

			MMREG	= 0x00008000,
			XMMREG	= 0x00010000,

			REG		= XMMREG | MMREG | TR | DR | CR | FPUREG | SEGREG | REG32 | REG16 | REG8,

			MEM8	= 0x00020000,
			MEM16	= 0x00040000,
			MEM32	= 0x00080000,
			MEM64	= 0x00100000,
			MEM80	= UNKNOWN,   // Extended double not supported by NT
			MEM128	= 0x00200000,
			MEM		= MEM128 | MEM80 | MEM64 | MEM32 | MEM16 | MEM8,
		
			XMM32	= MEM32 | XMMREG,
			XMM64	= MEM64 | XMMREG,

			R_M8	= MEM8 | REG8,
			R_M16	= MEM16 | REG16,
			R_M32	= MEM32 | REG32,
			R_M64	= MEM64 | MMREG,
			R_M128	= MEM128 | XMMREG,
			R_M		= MEM | REG,

			MOFFS8	= UNKNOWN,   // Not supported, equivalent available
			MOFFS16	= UNKNOWN,   // Not supported, equivalent available
			MOFFS32	= UNKNOWN,   // Not supported, equivalent available

			STR		= 0x00400000
		};

		Type type;
		const char *notation;
		union
		{
			int value;   // For immediates
			Encoding::Reg reg;   // For registers
		};

		static bool isSubtypeOf(Type type, Type baseType);
		bool isSubtypeOf(Type baseType) const;

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

		static Operand scanReg(const char *string);
		static Operand::Type scanSyntax(const char *string);

		static const Operand registerSet[];
		static const Operand syntaxSet[];

		static const Operand INIT;
		static const Operand NOT_FOUND;
	};
}

#endif   // Keywords_hpp
