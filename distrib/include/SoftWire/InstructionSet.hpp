#ifndef SoftWire_InstructionSet_hpp
#define SoftWire_InstructionSet_hpp

#include "Instruction.hpp"

namespace SoftWire
{
	class TokenList;

	class InstructionSet
	{
	public:
		InstructionSet();

		virtual ~InstructionSet();

		const Instruction *instruction(int i);

	private:
		struct Entry
		{
			~Entry() {delete instruction;};

			const char *mnemonic;

			Instruction *instruction;
		};

		Instruction *intrinsicMap;

		static Instruction::Syntax instructionSet[];

		static int numInstructions();
		static int numMnemonics();

		void generateIntrinsics();
	};
}

#endif   // SoftWire_InstructionSet_hpp
