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

		~InstructionSet();

		const Instruction *instruction(int i);
		Instruction *query(const char *mnemonic) const;

	private:
		struct Entry
		{
			~Entry() {delete instruction;};

			const char *mnemonic;

			Instruction *instruction;
		};

		Entry *instructionMap;
		Instruction **intrinsicMap;

		static int compareSyntax(const void *syntax1, const void *syntax2);
		static int compareEntry(const void *mnemonic, const void *entry);

		static Instruction::Syntax instructionSet[];

		static int numInstructions();
		static int numMnemonics();

		void generateIntrinsics();
	};
}

#endif   // SoftWire_InstructionSet_hpp
