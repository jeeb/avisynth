#ifndef SoftWire_Assembler_hpp
#define SoftWire_Assembler_hpp

#include "Operand.hpp"

namespace SoftWire
{
	class Synthesizer;
	class Instruction;
	class Scanner;
	class Parser;
	class Linker;
	class Loader;
	class Error;
	class InstructionSet;

	class Assembler
	{
	public:
		Assembler(const char *fileName = 0);

		~Assembler();

		// Run-time intrinsics
		void label(const char *label);
		#include "Intrinsics.hpp"

		// Methods for passing data references
		static void defineExternal(void *pointer, const char *name);
		static void defineSymbol(int value, const char *name);

		// Retrieve assembly code
		void (*callable(const char *entryLabel = 0))();
		void (*finalize(const char *entryLable = 0))();
		void *acquire();

		// Error and debugging methods
		const char *getErrors() const;
		const char *getListing() const;
		void clearListing() const;
		void setEchoFile(const char *echoFile, const char *mode = "wt");
		void annotate(const char *format, ...);

	private:
		char *entryLabel;

		static InstructionSet *instructionSet;
		static int referenceCount;

		Scanner *scanner;
		Parser *parser;
		Synthesizer *synthesizer;
		Linker *linker;
		Loader *loader;

		char *errors;
		char *echoFile;

		void assembleFile();
		void assembleLine();

		int x86(int instructionID,
		        const Operand &firstOperand = VOID,
		        const Operand &secondOperand = VOID,
		        const Operand &thirdOperand = VOID);   // Assemble run-time intrinsic

		void handleError(const char *error);
	};

	#define ASM_EXPORT(x) Assembler::defineExternal((void*)&x, #x);
	#define ASM_DEFINE(x) Assembler::defineSymbol(x, #x);
}

#endif   // SoftWire_Assembler_hpp
