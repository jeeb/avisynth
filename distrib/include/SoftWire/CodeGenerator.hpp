#ifndef SoftWire_CodeGenerator_hpp
#define SoftWire_CodeGenerator_hpp

#include "Assembler.hpp"

namespace SoftWire
{
	class CodeGenerator : public Assembler
	{
	public:
		CodeGenerator();

		const OperandREG32 &r32(const OperandREF &ref, bool copy = true);
		const OperandREG32 &x32(const OperandREF &ref, bool copy = false);
		const OperandR_M32 m32(const OperandREF &ref);
		const OperandREG32 &allocate(const OperandREG32 &reg, const OperandREF &ref, bool copy = false);
		const OperandREG32 &assign(const OperandREG32 &reg, const OperandREF &ref, bool copy = true);
		const OperandREG32 &access(const OperandREG32 &reg);
		void free(const OperandREG32 &reg);
		void spill(const OperandREG32 &reg);

		const OperandMMREG &r64(const OperandREF &ref, bool copy = true);
		const OperandMMREG &x64(const OperandREF &ref, bool copy = false);
		const OperandR_M64 m64(const OperandREF &ref);
		const OperandMMREG &allocate(const OperandMMREG &reg, const OperandREF &ref, bool copy = false);
		const OperandMMREG &assign(const OperandMMREG &reg, const OperandREF &ref, bool copy = true);
		const OperandMMREG &access(const OperandMMREG &reg);
		void free(const OperandMMREG &reg);
		void spill(const OperandMMREG &reg);

		const OperandXMMREG &r128(const OperandREF &ref, bool copy = true);
		const OperandXMMREG &x128(const OperandREF &ref, bool copy = false);
		const OperandR_M128 m128(const OperandREF &ref);
		const OperandXMMREG &allocate(const OperandXMMREG &reg, const OperandREF &ref, bool copy = false);
		const OperandXMMREG &assign(const OperandXMMREG &reg, const OperandREF &ref, bool copy = true);
		const OperandXMMREG &access(const OperandXMMREG &reg);
		void free(const OperandXMMREG &reg);
		void spill(const OperandXMMREG &reg);

		bool real(const OperandREF &ref);

		void free(const OperandREF &ref);
		void spill(const OperandREF &ref);

		void freeAll();
		void spillAll();

	private:
		OperandREF physicalEAX;
		OperandREF physicalECX;
		OperandREF physicalEDX;
		OperandREF physicalEBX;
		OperandREF physicalESI;
		OperandREF physicalEDI;

		OperandREF physicalMM0;
		OperandREF physicalMM1;
		OperandREF physicalMM2;
		OperandREF physicalMM3;
		OperandREF physicalMM4;
		OperandREF physicalMM5;
		OperandREF physicalMM6;
		OperandREF physicalMM7;

		OperandREF physicalXMM0;
		OperandREF physicalXMM1;
		OperandREF physicalXMM2;
		OperandREF physicalXMM3;
		OperandREF physicalXMM4;
		OperandREF physicalXMM5;
		OperandREF physicalXMM6;
		OperandREF physicalXMM7;

		unsigned int priorityEAX;
		unsigned int priorityECX;
		unsigned int priorityEDX;
		unsigned int priorityEBX;
		unsigned int priorityESI;
		unsigned int priorityEDI;

		unsigned int priorityMM0;
		unsigned int priorityMM1;
		unsigned int priorityMM2;
		unsigned int priorityMM3;
		unsigned int priorityMM4;
		unsigned int priorityMM5;
		unsigned int priorityMM6;
		unsigned int priorityMM7;

		unsigned int priorityXMM0;
		unsigned int priorityXMM1;
		unsigned int priorityXMM2;
		unsigned int priorityXMM3;
		unsigned int priorityXMM4;
		unsigned int priorityXMM5;
		unsigned int priorityXMM6;
		unsigned int priorityXMM7;
	};
}

#endif   // SoftWire_CodeGenerator_hpp
