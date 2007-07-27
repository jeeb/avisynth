#ifndef SoftWire_Assembler_hpp
#define SoftWire_Assembler_hpp

#include "Operand.hpp"

namespace SoftWire
{
	class Synthesizer;
	class Instruction;
	class Linker;
	class Loader;
	class Error;
	class InstructionSet;

	class Assembler
	{
	public:
		Assembler();

		virtual ~Assembler();

		// Run-time intrinsics
		void label(const char *label);
		#include "Intrinsics.hpp"

		// Retrieve binary code
		void (*callable(const char *entryLabel = 0))();
		void (*finalize(const char *entryLable = 0))();
		void *acquire();

		// Error and debugging methods
		const char *getListing() const;
		void clearListing() const;
		void setEchoFile(const char *echoFile, const char *mode = "wt");
		void annotate(const char *format, ...);
		void reset();
		int instructionCount();

		static void enableListing();   // Default on
		static void disableListing();

		static const OperandAL al;
		static const OperandCL cl;
		static const OperandREG8 dl;
		static const OperandREG8 bl;
		static const OperandREG8 ah;
		static const OperandREG8 ch;
		static const OperandREG8 dh;
		static const OperandREG8 bh;

		static const OperandAX ax;
		static const OperandCX cx;
		static const OperandDX dx;
		static const OperandREG16 bx;
		static const OperandREG16 sp;
		static const OperandREG16 bp;
		static const OperandREG16 si;
		static const OperandREG16 di;

		static const OperandEAX eax;
		static const OperandECX ecx;
		static const OperandREG32 edx;
		static const OperandREG32 ebx;
		static const OperandREG32 esp;
		static const OperandREG32 ebp;
		static const OperandREG32 esi;
		static const OperandREG32 edi;

		static const OperandST0 st;
		static const OperandST0 st0;
		static const OperandFPUREG st1;
		static const OperandFPUREG st2;
		static const OperandFPUREG st3;
		static const OperandFPUREG st4;
		static const OperandFPUREG st5;
		static const OperandFPUREG st6;
		static const OperandFPUREG st7;

		static const OperandMMREG mm0;
		static const OperandMMREG mm1;
		static const OperandMMREG mm2;
		static const OperandMMREG mm3;
		static const OperandMMREG mm4;
		static const OperandMMREG mm5;
		static const OperandMMREG mm6;
		static const OperandMMREG mm7;

		static const OperandXMMREG xmm0;
		static const OperandXMMREG xmm1;
		static const OperandXMMREG xmm2;
		static const OperandXMMREG xmm3;
		static const OperandXMMREG xmm4;
		static const OperandXMMREG xmm5;
		static const OperandXMMREG xmm6;
		static const OperandXMMREG xmm7;

		static const OperandMEM8 byte_ptr;
		static const OperandMEM16 word_ptr;
		static const OperandMEM32 dword_ptr;
		static const OperandMEM64 mmword_ptr;
		static const OperandMEM64 qword_ptr;
		static const OperandMEM128 xmmword_ptr;
		static const OperandMEM128 xword_ptr;

	protected:
		virtual Encoding *x86(int instructionID,
		                      const Operand &firstOperand = Operand::OPERAND_VOID,
		                      const Operand &secondOperand = Operand::OPERAND_VOID,
		                      const Operand &thirdOperand = Operand::OPERAND_VOID);   // Assemble run-time intrinsic

	private:
		char *entryLabel;

		static InstructionSet *instructionSet;
		static int referenceCount;

		Synthesizer *synthesizer;
		Linker *linker;
		Loader *loader;

		char *echoFile;

		static bool listingEnabled;
	};
}

#endif   // SoftWire_Assembler_hpp
