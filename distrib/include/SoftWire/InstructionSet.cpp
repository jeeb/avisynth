#include "InstructionSet.hpp"

#include "String.hpp"
#include "Error.hpp"
#include "Scanner.hpp"
#include "Token.hpp"
#include "Operand.hpp"

#include <stdlib.h>

namespace SoftWire
{
	InstructionSet::InstructionSet()
	{
		Instruction **instructionList = new Instruction*[numInstructions()];
		intrinsicMap = new Instruction*[numInstructions()];

		for(int k = 0; k < numInstructions(); k++)
		{
			instructionList[k] = new Instruction(instructionSet[k]);
			intrinsicMap[k] = instructionList[k];
		}

		qsort(instructionList, numInstructions(), sizeof(Instruction*), compareSyntax);

		instructionMap = new Entry[numMnemonics()];

		int j = 0;
		int i = 0;

		while(i < numInstructions())
		{
			instructionMap[j].mnemonic = instructionList[i]->getMnemonic();
			instructionMap[j].instruction = instructionList[i++];

			while(i < numInstructions() && stricmp(instructionList[i - 1]->getMnemonic(), instructionList[i]->getMnemonic()) == 0)
			{
				instructionMap[j].instruction->attach(instructionList[i++]);
			}

			j++;
		}

		delete[] instructionList;

		if(j != numMnemonics())
		{
			throw INTERNAL_ERROR;
		}

	//	generateIntrinsics();   // Uncomment this line when you make changes to the instruction set
	}

	InstructionSet::~InstructionSet()
	{
		delete[] instructionMap;
		delete[] intrinsicMap;
	}

	const Instruction *InstructionSet::instruction(int i)
	{
		return intrinsicMap[i];
	}

	Instruction *InstructionSet::query(const char *mnemonic) const
	{
		if(!instructionMap)
		{
			throw INTERNAL_ERROR;
		}

		Entry *query = (Entry*)bsearch(mnemonic, instructionMap, numMnemonics(), sizeof(Entry), compareEntry);

		if(!query)
		{
			return 0;
		}

		query->instruction->resetMatch();

		return query->instruction;
	}

	int InstructionSet::compareSyntax(const void *element1, const void *element2)
	{
		return stricmp((*(Instruction**)element1)->getMnemonic(), (*(Instruction**)element2)->getMnemonic());
	}

	int InstructionSet::compareEntry(const void *mnemonic, const void *entry)
	{
		return stricmp((char*)mnemonic, ((Entry*)entry)->mnemonic);
	}

	Instruction::Syntax InstructionSet::instructionSet[] =
	{
		/*
			Encoding syntax:
			----------------
			+r Add register value to opcode
			/# Value for Mod R/M register field encoding
			/r Effective address encoding
			ib Byte immediate
			iw Word immediate
			id Dword immediate
			-b Byte relative address
			-i Word or dword relative address
			p0 LOCK instruction prefix (F0h)
			p2 REPNE/REPNZ instruction prefix (F2h)
			p3 REP/REPE/REPZ instruction prefix (F3h) (also SSE prefix)
			po Offset override prefix (66h)
			pa Address override prefix (67h)

			Read Keywords.cpp for operands syntax
		*/

		// x86 instruction set
		{"AAA",				"",							"37",					Instruction::CPU_8086},
		{"AAS",				"",							"3F",					Instruction::CPU_8086},
		{"AAD",				"",							"D5 0A",				Instruction::CPU_8086},
		{"AAD",				"imm",						"D5 ib",				Instruction::CPU_8086},
		{"AAM",				"",							"D4 0A",				Instruction::CPU_8086},
		{"AAM",				"imm",						"D4 ib",				Instruction::CPU_8086},
		{"ADC",				"r/m8,reg8",				"10 /r",				Instruction::CPU_8086},
		{"ADC",				"r/m16,reg16",				"po 11 /r",				Instruction::CPU_8086},
		{"ADC",				"r/m32,reg32",				"po 11 /r",				Instruction::CPU_386},
		{"LOCK ADC",		"mem8,reg8",				"p0 10 /r",				Instruction::CPU_8086},
		{"LOCK ADC",		"mem16,reg16",				"p0 po 11 /r",			Instruction::CPU_8086},
		{"LOCK ADC",		"mem32,reg32",				"p0 po 11 /r",			Instruction::CPU_386},
		{"ADC",				"reg8,r/m8",				"12 /r",				Instruction::CPU_8086},
		{"ADC",				"reg16,r/m16",				"po 13 /r",				Instruction::CPU_8086},
		{"ADC",				"reg32,r/m32",				"po 13 /r",				Instruction::CPU_386},
		{"ADC",				"BYTE r/m8,imm8",			"80 /2 ib",				Instruction::CPU_8086},
		{"ADC",				"WORD r/m16,imm16",			"po 81 /2 iw",			Instruction::CPU_8086},
		{"ADC",				"DWORD r/m32,imm32",		"po 81 /2 id",			Instruction::CPU_386},
		{"ADC",				"WORD r/m16,imm8",			"po 83 /2 ib",			Instruction::CPU_8086},
		{"ADC",				"DWORD r/m32,imm8",			"po 83 /2 ib",			Instruction::CPU_386},
		{"LOCK ADC",		"BYTE mem8,imm8",			"p0 80 /2 ib",			Instruction::CPU_8086},
		{"LOCK ADC",		"WORD mem16,imm16",			"p0 po 81 /2 iw",		Instruction::CPU_8086},
		{"LOCK ADC",		"DWORD mem32,imm32",		"p0 po 81 /2 id",		Instruction::CPU_386},
		{"LOCK ADC",		"WORD mem16,imm8",			"p0 po 83 /2 ib",		Instruction::CPU_8086},
		{"LOCK ADC",		"DWORD mem32,imm8",			"p0 po 83 /2 ib",		Instruction::CPU_386},
		{"ADC",				"AL,imm8",					"14 ib",				Instruction::CPU_8086},
		{"ADC",				"AX,imm16",					"po 15 iw",				Instruction::CPU_8086},
		{"ADC",				"EAX,imm32",				"po 15 id",				Instruction::CPU_386},
		{"ADD",				"r/m8,reg8",				"00 /r",				Instruction::CPU_8086},
		{"ADD",				"r/m16,reg16",				"po 01 /r",				Instruction::CPU_8086},
		{"ADD",				"r/m32,reg32",				"po 01 /r",				Instruction::CPU_386},
		{"LOCK ADD",		"mem8,reg8",				"p0 00 /r",				Instruction::CPU_8086},
		{"LOCK ADD",		"mem16,reg16",				"p0 po 01 /r",			Instruction::CPU_8086},
		{"LOCK ADD",		"mem32,reg32",				"p0 po 01 /r",			Instruction::CPU_386},
		{"ADD",				"reg8,r/m8",				"02 /r",				Instruction::CPU_8086},
		{"ADD",				"reg16,r/m16",				"po 03 /r",				Instruction::CPU_8086},
		{"ADD",				"reg32,r/m32",				"po 03 /r",				Instruction::CPU_386},
		{"ADD",				"BYTE r/m8,imm8",			"80 /0 ib",				Instruction::CPU_8086},
		{"ADD",				"WORD r/m16,imm16",			"po 81 /0 iw",			Instruction::CPU_8086},
		{"ADD",				"DWORD r/m32,imm32",		"po 81 /0 id",			Instruction::CPU_386},
		{"ADD",				"WORD r/m16,imm8",			"po 83 /0 ib",			Instruction::CPU_8086},
		{"ADD",				"DWORD r/m32,imm8",			"po 83 /0 ib",			Instruction::CPU_386},
		{"LOCK ADD",		"BYTE mem8,imm8",			"p0 80 /0 ib",			Instruction::CPU_8086},
		{"LOCK ADD",		"WORD mem16,imm16",			"p0 po 81 /0 iw",		Instruction::CPU_8086},
		{"LOCK ADD",		"DWORD mem32,imm32",		"p0 po 81 /0 id",		Instruction::CPU_386},
		{"LOCK ADD",		"WORD mem16,imm8",			"p0 po 83 /0 ib",		Instruction::CPU_8086},
		{"LOCK ADD",		"DWORD mem32,imm8",			"p0 po 83 /0 ib",		Instruction::CPU_386},
		{"ADD",				"AL,imm8",					"04 ib",				Instruction::CPU_8086},
		{"ADD",				"AX,imm16",					"po 05 iw",				Instruction::CPU_8086},
		{"ADD",				"EAX,imm32",				"po 05 id",				Instruction::CPU_386},
		{"ADDPD",			"xmmreg,r/m128",			"66 0F 58 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"ADDPS",			"xmmreg,r/m128",			"0F 58 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ADDSD",			"xmmreg,xmm64",				"p2 0F 58 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"ADDSS",			"xmmreg,xmm32",				"p3 0F 58 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ADDSUBPD",		"xmmreg,r/m128",			"66 0F D0 /r",			Instruction::CPU_PNI},
		{"ADDSUBPS",		"xmmreg,r/m128",			"F2 0F D0 /r",			Instruction::CPU_PNI},
		{"AND",				"r/m8,reg8",				"20 /r",				Instruction::CPU_8086},
		{"AND",				"r/m16,reg16",				"po 21 /r",				Instruction::CPU_8086},
		{"AND",				"r/m32,reg32",				"po 21 /r",				Instruction::CPU_386},
		{"LOCK AND",		"mem8,reg8",				"p0 20 /r",				Instruction::CPU_8086},
		{"LOCK AND",		"mem16,reg16",				"p0 po 21 /r",			Instruction::CPU_8086},
		{"LOCK AND",		"mem32,reg32",				"p0 po 21 /r",			Instruction::CPU_386},
		{"AND",				"reg8,r/m8",				"22 /r",				Instruction::CPU_8086},
		{"AND",				"reg16,r/m16",				"po 23 /r",				Instruction::CPU_8086},
		{"AND",				"reg32,r/m32",				"po 23 /r",				Instruction::CPU_386},
		{"AND",				"BYTE r/m8,imm8",			"80 /4 ib",				Instruction::CPU_8086},
		{"AND",				"WORD r/m16,imm16",			"po 81 /4 iw",			Instruction::CPU_8086},
		{"AND",				"DWORD r/m32,imm32",		"po 81 /4 id",			Instruction::CPU_386},
		{"AND",				"WORD r/m16,imm8",			"po 83 /4 ib",			Instruction::CPU_8086},
		{"AND",				"DWORD r/m32,imm8",			"po 83 /4 ib",			Instruction::CPU_386},
		{"LOCK AND",		"BYTE mem8,imm8",			"80 /4 ib",				Instruction::CPU_8086},
		{"LOCK AND",		"WORD mem16,imm16",			"p0 po 81 /4 iw",		Instruction::CPU_8086},
		{"LOCK AND",		"DWORD mem32,imm32",		"p0 po 81 /4 id",		Instruction::CPU_386},
		{"LOCK AND",		"WORD mem16,imm8",			"p0 po 83 /4 ib",		Instruction::CPU_8086},
		{"LOCK AND",		"DWORD mem32,imm8",			"p0 po 83 /4 ib",		Instruction::CPU_386},
		{"AND",				"AL,imm8",					"24 ib",				Instruction::CPU_8086},
		{"AND",				"AX,imm16",					"po 25 iw",				Instruction::CPU_8086},
		{"AND",				"EAX,imm32",				"po 25 id",				Instruction::CPU_386},
		{"ANDNPD",			"xmmreg,r/m128",			"66 0F 55 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"ANDNPS",			"xmmreg,r/m128",			"0F 55 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"ANDPD",			"xmmreg,r/m128",			"66 0F 54 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"ANDPS",			"xmmreg,r/m128",			"0F 54 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	//	{"ARPL",			"r/m16,reg16",				"63 /r",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"BOUND",			"reg16,mem",				"po 62 /r",				Instruction::CPU_186},
		{"BOUND",			"reg32,mem",				"po 62 /r",				Instruction::CPU_386},
		{"BSF",				"reg16,r/m16",				"po 0F BC /r",			Instruction::CPU_386},
		{"BSF",				"reg32,r/m32",				"po 0F BC /r",			Instruction::CPU_386},
		{"BSR",				"reg16,r/m16",				"po 0F BD /r",			Instruction::CPU_386},
		{"BSR",				"reg32,r/m32",				"po 0F BD /r",			Instruction::CPU_386},
		{"BSWAP",			"reg32",					"po 0F C8 +r",			Instruction::CPU_486},
		{"BT",				"r/m16,reg16",				"po 0F A3 /r",			Instruction::CPU_386},
		{"BT",				"r/m32,reg32",				"po 0F A3 /r",			Instruction::CPU_386},
		{"BT",				"WORD r/m16,imm8",			"po 0F BA /4 ib",		Instruction::CPU_386},
		{"BT",				"DWORD r/m32,imm8",			"po 0F BA /4 ib",		Instruction::CPU_386},
		{"BTC",				"r/m16,reg16",				"po 0F BB /r",			Instruction::CPU_386},
		{"BTC",				"r/m32,reg32",				"po 0F BB /r",			Instruction::CPU_386},
		{"BTC",				"WORD r/m16,imm8",			"po 0F BA /7 ib",		Instruction::CPU_386},
		{"BTC",				"DWORD r/m32,imm8",			"po 0F BA /7 ib",		Instruction::CPU_386},
		{"BTR",				"r/m16,reg16",				"po 0F B3 /r",			Instruction::CPU_386},
		{"BTR",				"r/m32,reg32",				"po 0F B3 /r",			Instruction::CPU_386},
		{"BTR",				"WORD r/m16,imm8",			"po 0F BA /6 ib",		Instruction::CPU_386},
		{"BTR",				"DWORD r/m32,imm8",			"po 0F BA /6 ib",		Instruction::CPU_386},
		{"BTS",				"r/m16,reg16",				"po 0F AB /r",			Instruction::CPU_386},
		{"BTS",				"r/m32,reg32",				"po 0F AB /r",			Instruction::CPU_386},
		{"BTS",				"WORD r/m16,imm8",			"po 0F BA /5 ib",		Instruction::CPU_386},
		{"BTS",				"DWORD r/m32,imm8",			"po 0F BA /5 ib",		Instruction::CPU_386},
		{"LOCK BTC",		"mem16,reg16",				"p0 po 0F BB /r",		Instruction::CPU_386},
		{"LOCK BTC",		"mem32,reg32",				"p0 po 0F BB /r",		Instruction::CPU_386},
		{"LOCK BTC",		"WORD mem16,imm8",			"p0 po 0F BA /7 ib",	Instruction::CPU_386},
		{"LOCK BTC",		"DWORD mem32,imm8",			"p0 po 0F BA /7 ib",	Instruction::CPU_386},
		{"LOCK BTR",		"mem16,reg16",				"p0 po 0F B3 /r",		Instruction::CPU_386},
		{"LOCK BTR",		"mem32,reg32",				"p0 po 0F B3 /r",		Instruction::CPU_386},
		{"LOCK BTR",		"WORD mem16,imm8",			"p0 po 0F BA /6 ib",	Instruction::CPU_386},
		{"LOCK BTR",		"DWORD mem32,imm8",			"p0 po 0F BA /6 ib",	Instruction::CPU_386},
		{"LOCK BTS",		"mem16,reg16",				"p0 po 0F AB /r",		Instruction::CPU_386},
		{"LOCK BTS",		"mem32,reg32",				"p0 po 0F AB /r",		Instruction::CPU_386},
		{"LOCK BTS",		"WORD mem16,imm8",			"p0 po 0F BA /5 ib",	Instruction::CPU_386},
		{"LOCK BTS",		"DWORD mem32,imm8",			"p0 po 0F BA /5 ib",	Instruction::CPU_386},
		{"CALL",			"imm",						"E8 -i",				Instruction::CPU_8086},
	//	{"CALL",			"imm:imm16",				"po 9A iw iw",			Instruction::CPU_8086},
	//	{"CALL",			"imm:imm32",				"po 9A id iw",			Instruction::CPU_386},
	//	{"CALL",			"FAR mem16",				"po FF /3",				Instruction::CPU_8086},
	//	{"CALL",			"FAR mem32",				"po FF /3",				Instruction::CPU_386},
		{"CALL",			"WORD r/m16",				"po FF /2",				Instruction::CPU_8086},
		{"CALL",			"DWORD r/m32",				"po FF /2",				Instruction::CPU_386},
		{"CBW",				"",							"po 98",				Instruction::CPU_8086},
		{"CWD",				"",							"po 99",				Instruction::CPU_8086},
		{"CDQ",				"",							"po 99",				Instruction::CPU_386},
		{"CWDE",			"",							"po 98",				Instruction::CPU_386},
		{"CLC",				"",							"F8",					Instruction::CPU_8086},
		{"CLD",				"",							"FC",					Instruction::CPU_8086},
		{"CLI",				"",							"FA",					Instruction::CPU_8086},
	//	{"CLTS",			"",							"0F 06",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"CLFLUSH",			"mem",						"0F AE /7",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CMC",				"",							"F5",					Instruction::CPU_8086},
		{"CMOVO",			"reg16,r/m16",				"po 0F 40 /r",			Instruction::CPU_P6},
		{"CMOVNO",			"reg16,r/m16",				"po 0F 41 /r",			Instruction::CPU_P6},
		{"CMOVB",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVC",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVNEA",			"reg16,r/m16",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVAE",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVNB",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVNC",			"reg16,r/m16",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVE",			"reg16,r/m16",				"po 0F 44 /r",			Instruction::CPU_P6},
		{"CMOVZ",			"reg16,r/m16",				"po 0F 44 /r",			Instruction::CPU_P6},
		{"CMOVNE",			"reg16,r/m16",				"po 0F 45 /r",			Instruction::CPU_P6},
		{"CMOVNZ",			"reg16,r/m16",				"po 0F 45 /r",			Instruction::CPU_P6},
		{"CMOVBE",			"reg16,r/m16",				"po 0F 46 /r",			Instruction::CPU_P6},
		{"CMOVNA",			"reg16,r/m16",				"po 0F 46 /r",			Instruction::CPU_P6},
		{"CMOVA",			"reg16,r/m16",				"po 0F 47 /r",			Instruction::CPU_P6},
		{"CMOVNBE",			"reg16,r/m16",				"po 0F 47 /r",			Instruction::CPU_P6},
		{"CMOVS",			"reg16,r/m16",				"po 0F 48 /r",			Instruction::CPU_P6},
		{"CMOVNS",			"reg16,r/m16",				"po 0F 49 /r",			Instruction::CPU_P6},
		{"CMOVP",			"reg16,r/m16",				"po 0F 4A /r",			Instruction::CPU_P6},
		{"CMOVPE",			"reg16,r/m16",				"po 0F 4A /r",			Instruction::CPU_P6},
		{"CMOVNP",			"reg16,r/m16",				"po 0F 4B /r",			Instruction::CPU_P6},
		{"CMOVPO",			"reg16,r/m16",				"po 0F 4B /r",			Instruction::CPU_P6},
		{"CMOVL",			"reg16,r/m16",				"po 0F 4C /r",			Instruction::CPU_P6},
		{"CMOVNGE",			"reg16,r/m16",				"po 0F 4C /r",			Instruction::CPU_P6},
		{"CMOVGE",			"reg16,r/m16",				"po 0F 4D /r",			Instruction::CPU_P6},
		{"CMOVNL",			"reg16,r/m16",				"po 0F 4D /r",			Instruction::CPU_P6},
		{"CMOVLE",			"reg16,r/m16",				"po 0F 4E /r",			Instruction::CPU_P6},
		{"CMOVNG",			"reg16,r/m16",				"po 0F 4E /r",			Instruction::CPU_P6},
		{"CMOVG",			"reg16,r/m16",				"po 0F 4F /r",			Instruction::CPU_P6},
		{"CMOVNLE",			"reg16,r/m16",				"po 0F 4F /r",			Instruction::CPU_P6},
		{"CMOVO",			"reg32,r/m32",				"po 0F 40 /r",			Instruction::CPU_P6},
		{"CMOVNO",			"reg32,r/m32",				"po 0F 41 /r",			Instruction::CPU_P6},
		{"CMOVB",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVC",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVNEA",			"reg32,r/m32",				"po 0F 42 /r",			Instruction::CPU_P6},
		{"CMOVAE",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVNB",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVNC",			"reg32,r/m32",				"po 0F 43 /r",			Instruction::CPU_P6},
		{"CMOVE",			"reg32,r/m32",				"po 0F 44 /r",			Instruction::CPU_P6},
		{"CMOVZ",			"reg32,r/m32",				"po 0F 44 /r",			Instruction::CPU_P6},
		{"CMOVNE",			"reg32,r/m32",				"po 0F 45 /r",			Instruction::CPU_P6},
		{"CMOVNZ",			"reg32,r/m32",				"po 0F 45 /r",			Instruction::CPU_P6},
		{"CMOVBE",			"reg32,r/m32",				"po 0F 46 /r",			Instruction::CPU_P6},
		{"CMOVNA",			"reg32,r/m32",				"po 0F 46 /r",			Instruction::CPU_P6},
		{"CMOVA",			"reg32,r/m32",				"po 0F 47 /r",			Instruction::CPU_P6},
		{"CMOVNBE",			"reg32,r/m32",				"po 0F 47 /r",			Instruction::CPU_P6},
		{"CMOVS",			"reg32,r/m32",				"po 0F 48 /r",			Instruction::CPU_P6},
		{"CMOVNS",			"reg32,r/m32",				"po 0F 49 /r",			Instruction::CPU_P6},
		{"CMOVP",			"reg32,r/m32",				"po 0F 4A /r",			Instruction::CPU_P6},
		{"CMOVPE",			"reg32,r/m32",				"po 0F 4A /r",			Instruction::CPU_P6},
		{"CMOVNP",			"reg32,r/m32",				"po 0F 4B /r",			Instruction::CPU_P6},
		{"CMOVPO",			"reg32,r/m32",				"po 0F 4B /r",			Instruction::CPU_P6},
		{"CMOVL",			"reg32,r/m32",				"po 0F 4C /r",			Instruction::CPU_P6},
		{"CMOVNGE",			"reg32,r/m32",				"po 0F 4C /r",			Instruction::CPU_P6},
		{"CMOVGE",			"reg32,r/m32",				"po 0F 4D /r",			Instruction::CPU_P6},
		{"CMOVNL",			"reg32,r/m32",				"po 0F 4D /r",			Instruction::CPU_P6},
		{"CMOVLE",			"reg32,r/m32",				"po 0F 4E /r",			Instruction::CPU_P6},
		{"CMOVNG",			"reg32,r/m32",				"po 0F 4E /r",			Instruction::CPU_P6},
		{"CMOVG",			"reg32,r/m32",				"po 0F 4F /r",			Instruction::CPU_P6},
		{"CMOVNLE",			"reg32,r/m32",				"po 0F 4F /r",			Instruction::CPU_P6},
		{"CMP",				"r/m8,reg8",				"38 /r",				Instruction::CPU_8086},
		{"CMP",				"r/m16,reg16",				"po 39 /r",				Instruction::CPU_8086},
		{"CMP",				"r/m32,reg32",				"po 39 /r",				Instruction::CPU_386},
		{"CMP",				"reg8,r/m8",				"3A /r",				Instruction::CPU_8086},
		{"CMP",				"reg16,r/m16",				"po 3B /r",				Instruction::CPU_8086},
		{"CMP",				"reg32,r/m32",				"po 3B /r",				Instruction::CPU_386},
		{"CMP",				"BYTE r/m8,imm8",			"80 /7 ib",				Instruction::CPU_8086},
		{"CMP",				"WORD r/m16,imm16",			"po 81 /7 iw",			Instruction::CPU_8086},
		{"CMP",				"DWORD r/m32,imm32",		"po 81 /7 id",			Instruction::CPU_386},
		{"CMP",				"WORD r/m16,imm8",			"po 83 /7 ib",			Instruction::CPU_8086},
		{"CMP",				"DWORD r/m32,imm8",			"po 83 /7 ib",			Instruction::CPU_386},
		{"CMP",				"AL,imm8",					"3C ib",				Instruction::CPU_8086},
		{"CMP",				"AX,imm16",					"po 3D iw",				Instruction::CPU_8086},
		{"CMP",				"EAX,imm32",				"po 3D id",				Instruction::CPU_386},
		{"CMPPD",			"xmmreg,r/m128,imm8",		"66 0F C2 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CMPEQPD",			"xmmreg,r/m128",			"66 0F C2 /r 00",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPLTPD",			"xmmreg,r/m128",			"66 0F C2 /r 01",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPLEPD",			"xmmreg,r/m128",			"66 0F C2 /r 02",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPUNORDPD",		"xmmreg,r/m128",			"66 0F C2 /r 03",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNEQPD",		"xmmreg,r/m128",			"66 0F C2 /r 04",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNLTPD",		"xmmreg,r/m128",			"66 0F C2 /r 05",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNLEPD",		"xmmreg,r/m128",			"66 0F C2 /r 06",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPORDPD",		"xmmreg,r/m128",			"66 0F C2 /r 07",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CMPPS",			"xmmreg,r/m128,imm8",		"0F C2 /r ib",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPEQPS",			"xmmreg,r/m128",			"0F C2 /r 00",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPLEPS",			"xmmreg,r/m128",			"0F C2 /r 02",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPLTPS",			"xmmreg,r/m128",			"0F C2 /r 01",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNEQPS",		"xmmreg,r/m128",			"0F C2 /r 04",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNLEPS",		"xmmreg,r/m128",			"0F C2 /r 06",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNLTPS",		"xmmreg,r/m128",			"0F C2 /r 05",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPORDPS",		"xmmreg,r/m128",			"0F C2 /r 07",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPUNORDPS",		"xmmreg,r/m128",			"0F C2 /r 03",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPSB",			"",							"A6",					Instruction::CPU_8086},
		{"CMPSW",			"",							"po A7",				Instruction::CPU_8086},
		{"CMPSD",			"",							"po A7",				Instruction::CPU_386},
		{"REPE CMPSB",		"",							"p3 A6",				Instruction::CPU_8086},
		{"REPE CMPSW",		"",							"p3 po A7",				Instruction::CPU_8086},
		{"REPE CMPSD",		"",							"p3 po A7",				Instruction::CPU_386},
		{"REPNE CMPSB",		"",							"p2 A6",				Instruction::CPU_8086},
		{"REPNE CMPSW",		"",							"p2 po A7",				Instruction::CPU_8086},
		{"REPNE CMPSD",		"",							"p2 po A7",				Instruction::CPU_386},
		{"REPZ CMPSB",		"",							"p3 A6",				Instruction::CPU_8086},
		{"REPZ CMPSW",		"",							"p3 po A7",				Instruction::CPU_8086},
		{"REPZ CMPSD",		"",							"p3 po A7",				Instruction::CPU_386},
		{"REPNZ CMPSB",		"",							"p2 A6",				Instruction::CPU_8086},
		{"REPNZ CMPSW",		"",							"p2 po A7",				Instruction::CPU_8086},
		{"REPNZ CMPSD",		"",							"p2 po A7",				Instruction::CPU_386},
		{"CMPSD",			"xmmreg,xmm64,imm8",		"p2 0F C2 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CMPEQSD",			"xmmreg,xmm64",				"p2 0F C2 /r 00",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPLTSD",			"xmmreg,xmm64",				"p2 0F C2 /r 01",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPLESD",			"xmmreg,xmm64",				"p2 0F C2 /r 02",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPUNORDSD",		"xmmreg,xmm64",				"p2 0F C2 /r 03",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNEQSD",		"xmmreg,xmm64",				"p2 0F C2 /r 04",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNLTSD",		"xmmreg,xmm64",				"p2 0F C2 /r 05",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPNLESD",		"xmmreg,xmm64",				"p2 0F C2 /r 06",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},  
		{"CMPORDSD",		"xmmreg,xmm64",				"p2 0F C2 /r 07",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CMPSS",			"xmmreg,xmm32,imm8",		"p3 0F C2 /r ib",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPEQSS",			"xmmreg,xmm32",				"p3 0F C2 /r 00",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPLESS",			"xmmreg,xmm32",				"p3 0F C2 /r 02",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPLTSS",			"xmmreg,xmm32",				"p3 0F C2 /r 01",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNEQSS",		"xmmreg,xmm32",				"p3 0F C2 /r 04",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNLESS",		"xmmreg,xmm32",				"p3 0F C2 /r 06",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPNLTSS",		"xmmreg,xmm32",				"p3 0F C2 /r 05",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPORDSS",		"xmmreg,xmm32",				"p3 0F C2 /r 07",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPUNORDSS",		"xmmreg,xmm32",				"p3 0F C2 /r 03",		Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CMPXCHG",			"r/m8,reg8",				"0F B0 /r",				Instruction::CPU_PENTIUM},
		{"CMPXCHG",			"r/m16,reg16",				"po 0F B1 /r",			Instruction::CPU_PENTIUM},
		{"CMPXCHG",			"r/m32,reg32",				"po 0F B1 /r",			Instruction::CPU_PENTIUM},
		{"LOCK CMPXCHG",	"mem8,reg8",				"p0 0F B0 /r",			Instruction::CPU_PENTIUM},
		{"LOCK CMPXCHG",	"mem16,reg16",				"p0 po 0F B1 /r",		Instruction::CPU_PENTIUM},
		{"LOCK CMPXCHG",	"mem32,reg32",				"p0 po 0F B1 /r",		Instruction::CPU_PENTIUM},
	//	{"CMPXCHG486",		"r/m8,reg8",				"0F A6 /r",				Instruction::CPU_486 | Instruction::CPU_UNDOC},
	//	{"CMPXCHG486",		"r/m16,reg16",				"po 0F A7 /r",			Instruction::CPU_486 | Instruction::CPU_UNDOC},
	//	{"CMPXCHG486",		"r/m32,reg32",				"po 0F A7 /r",			Instruction::CPU_486 | Instruction::CPU_UNDOC},
		{"CMPXCHG8B",		"mem",						"0F C7 /1",				Instruction::CPU_PENTIUM},
		{"LOCK CMPXCHG8B",	"mem",						"p0 0F C7 /1",			Instruction::CPU_PENTIUM},
		{"COMISD",			"xmmreg,xmm64",				"66 0F 2F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"COMISS",			"xmmreg,xmm32",				"0F 2F /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CPUID",			"",							"0F A2",				Instruction::CPU_PENTIUM},
		{"CVTDQ2PD",		"xmmreg,xmm64",				"p3 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTDQ2PS",		"xmmreg,r/m128",			"0F 5B /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"CVTPD2DQ",		"xmmreg,r/m128",			"p2 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPD2PI",		"mmreg,r/m128",				"66 0F 2D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPD2PS",		"xmmreg,r/m128",			"66 0F 5A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPI2PD",		"xmmreg,r/m64",				"66 0F 2A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPS2DQ",		"xmmreg,r/m128",			"66 0F 5B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPS2PD",		"xmmreg,xmm64",				"0F 5A /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTSD2SI",		"reg32,xmm64",				"p2 0F 2D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTSI2SD",		"xmmreg,r/m32",				"p2 0F 2A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTSS2SD",		"xmmreg,xmm32",				"p3 0F 5A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTTPD2DQ",		"xmmreg,r/m128",			"66 0F E6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTTPD2PI",		"mmreg,r/m128",				"66 0F 2C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTTPS2DQ",		"xmmreg,r/m128",			"p3 0F 5B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTTSD2SI",		"reg32,xmm64",				"p2 0F 2C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"CVTPI2PS",		"xmmreg,r/m64",				"0F 2A /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CVTPS2PI",		"mmreg,xmm64",				"0F 2D /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CVTTPS2PI",		"mmreg,xmm64",				"0F 2C /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CVTSI2SS",		"xmmreg,r/m32",				"p3 0F 2A /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CVTSS2SI",		"reg32,xmm32",				"p3 0F 2D /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"CVTTSS2SI",		"reg32,xmm32",				"p3 0F 2C /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"DAA",				"",							"27",					Instruction::CPU_8086},
		{"DAS",				"",							"2F",					Instruction::CPU_8086},
		{"DEC",				"reg16",					"po 48 +r",				Instruction::CPU_8086},
		{"DEC",				"reg32",					"po 48 +r",				Instruction::CPU_386},
		{"DEC",				"BYTE r/m8",				"FE /1",				Instruction::CPU_8086},
		{"DEC",				"WORD r/m16",				"po FF /1",				Instruction::CPU_8086},
		{"DEC",				"DWORD r/m32",				"po FF /1",				Instruction::CPU_386},
		{"LOCK DEC",		"BYTE mem8",				"p0 FE /1",				Instruction::CPU_8086},
		{"LOCK DEC",		"WORD mem16",				"p0 po FF /1",			Instruction::CPU_8086},
		{"LOCK DEC",		"DWORD mem32",				"p0 po FF /1",			Instruction::CPU_386},
		{"DIV",				"BYTE r/m8",				"p0 F6 /6",				Instruction::CPU_8086},
		{"DIV",				"WORD r/m16",				"po F7 /6",				Instruction::CPU_8086},
		{"DIV",				"DWORD r/m32",				"po F7 /6",				Instruction::CPU_386},
		{"DIVPD",			"xmmreg,r/m128",			"66 0F 5E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"DIVPS",			"xmmreg,r/m128",			"0F 5E /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"DIVSD",			"xmmreg,xmm64",				"p2 0F 5E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"DIVSS",			"xmmreg,xmm32",				"p3 0F 5E /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"EMMS",			"",							"0F 77",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
	//	{"ENTER",			"imm,imm",					"C8 iw ib",				Instruction::CPU_186},
		{"F2XM1",			"",							"D9 F0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FABS",			"",							"D9 E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADD",			"DWORD mem32",				"D8 /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADD",			"QWORD mem64",				"DC /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADD",			"fpureg",					"D8 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADD",			"ST0,fpureg",				"D8 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FADD",			"TO fpureg",				"DC C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADD",			"fpureg,ST0",				"DC C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADDP",			"",							"DE C1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADDP",			"fpureg",					"DE C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FADDP",			"fpureg,ST0",				"DE C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FBLD",			"mem80",					"DF /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FBSTP",			"mem80",					"DF /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCHS",			"",							"D9 E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCLEX",			"",							"9B DB E2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNCLEX",			"",							"DB E2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCMOVB",			"fpureg",					"DA C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVB",			"ST0,fpureg",				"DA C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVBE",			"fpureg",					"DA D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVBE",			"ST0,fpureg",				"DA D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVE",			"fpureg",					"DA C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVE",			"ST0,fpureg",				"DA C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNB",			"fpureg",					"DB C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNB",			"ST0,fpureg",				"DB C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNBE",		"fpureg",					"DB D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNBE",		"ST0,fpureg",				"DB D0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNE",			"fpureg",					"DB C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNE",			"ST0,fpureg",				"DB C8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNU",			"fpureg",					"DB D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVNU",			"ST0,fpureg",				"DB D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVU",			"fpureg",					"DA D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCMOVU",			"ST0,fpureg",				"DA D8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCOM",			"DWORD mem32",				"D8 /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOM",			"QWORD mem64",				"DC /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOM",			"fpureg",					"D8 D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOM",			"ST0,fpureg",				"D8 D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMP",			"DWORD mem32",				"D8 /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMP",			"QWORD mem64",				"DC /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMP",			"fpureg",					"D8 D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMP",			"ST0,fpureg",				"D8 D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMPP",			"",							"DE D9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FCOMI",			"fpureg",					"DB F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCOMI",			"ST0,fpureg",				"DB F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCOMIP",			"fpureg",					"DF F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCOMIP",			"ST0,fpureg",				"DF F0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FCOS",			"",							"D9 FF",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FDECSTP",			"",							"D9 F6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDISI",			"",							"9B DB E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNDISI",			"",							"DB E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FENI",			"",							"9B DB E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNENI",			"",							"DB E0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIV",			"DWORD mem32",				"D8 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIV",			"QWORD mem64",				"DC /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIV",			"fpureg",					"D8 F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIV",			"ST0,fpureg",				"D8 F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FDIV",			"TO fpureg",				"DC F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIV",			"fpureg,ST0",				"DC F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVR",			"DWORD mem32",				"D8 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVR",			"QWORD mem64",				"DC /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVR",			"fpureg",					"D8 F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVR",			"ST0,fpureg",				"D8 F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FDIVR",			"TO fpureg",				"DC F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVR",			"fpureg,ST0",				"DC F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVP",			"",							"DE F9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVP",			"fpureg",					"DE F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVP",			"fpureg,ST0",				"DE F8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVRP",			"",							"DE F1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVRP",			"fpureg",					"DE F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FDIVRP",			"fpureg,ST0",				"DE F0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FEMMS",			"",							"0F 0E",				Instruction::CPU_3DNOW},
		{"FFREE",			"fpureg",					"DD C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FFREEP",			"fpureg",					"DF C0 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU | Instruction::CPU_UNDOC},
		{"FIADD",			"WORD mem16",				"DE /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIADD",			"DWORD mem32",				"DA /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FICOM",			"WORD mem16",				"DE /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FICOM",			"DWORD mem32",				"DA /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FICOMP",			"WORD mem16",				"DE /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FICOMP",			"DWORD mem32",				"DA /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIDIV",			"WORD mem16",				"DE /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIDIV",			"DWORD mem32",				"DA /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIDIVR",			"WORD mem16",				"DE /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIDIVR",			"DWORD mem32",				"DA /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FILD",			"WORD mem16",				"DF /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FILD",			"DWORD mem32",				"DB /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FILD",			"QWORD mem64",				"DF /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIST",			"WORD mem16",				"DF /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIST",			"DWORD mem32",				"DB /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISTP",			"WORD mem16",				"DF /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISTP",			"DWORD mem32",				"DB /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISTP",			"QWORD mem64",				"DF /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISTTP",			"WORD mem16",				"DF /1",				Instruction::CPU_PNI},
		{"FISTTP",			"DWORD mem32",				"DB /1",				Instruction::CPU_PNI},
		{"FISTTP",			"QWORD mem64",				"DD /1",				Instruction::CPU_PNI},
		{"FIMUL",			"WORD mem16",				"DE /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FIMUL",			"DWORD mem32",				"DA /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FINCSTP",			"",							"D9 F7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FINIT",			"",							"9B DB E3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNINIT",			"",							"DB E3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISUB",			"WORD mem16",				"DE /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISUB",			"DWORD mem32",				"DA /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISUBR",			"WORD mem16",				"DE /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FISUBR",			"DWORD mem32",				"DA /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLD",				"DWORD mem32",				"D9 /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLD",				"QWORD mem64",				"DD /0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FLD",				"mem80",					"DB /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLD",				"fpureg",					"D9 C0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLD1",			"",							"D9 E8",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDL2E",			"",							"D9 EA",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDL2T",			"",							"D9 E9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDLG2",			"",							"D9 EC",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDLN2",			"",							"D9 ED",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDPI",			"",							"D9 EB",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDZ",			"",							"D9 EE",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDCW",			"mem16",					"D9 /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FLDENV",			"mem",						"D9 /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"DWORD mem32",				"D8 /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"QWORD mem64",				"DC /1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"",							"D8 C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"fpureg",					"D8 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"ST0,fpureg",				"D8 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FMUL",			"TO fpureg",				"DC C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMUL",			"fpureg,ST0",				"DC C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMULP",			"fpureg",					"DE C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMULP",			"fpureg,ST0",				"DE C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FMULP",			"",							"DE C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNOP",			"",							"D9 D0",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FPATAN",			"",							"D9 F3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FPTAN",			"",							"D9 F2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FPREM",			"",							"D9 F8",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FPREM1",			"",							"D9 F5",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FRNDINT",			"",							"D9 FC",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSAVE",			"mem",						"9B DD /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNSAVE",			"mem",						"DD /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FRSTOR",			"mem",						"DD /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSCALE",			"",							"D9 FD",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSETPM",			"",							"DB E4",				Instruction::CPU_286 | Instruction::CPU_FPU},
		{"FSIN",			"",							"D9 FE",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FSINCOS",			"",							"D9 FB",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FSQRT",			"",							"D9 FA",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FST",				"DWORD mem32",				"D9 /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FST",				"QWORD mem64",				"DD /2",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FST",				"fpureg",					"DD D0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTP",			"DWORD mem32",				"D9 /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTP",			"QWORD mem64",				"DD /3",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FSTP",			"mem80",					"DB /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTP",			"fpureg",					"DD D8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTCW",			"mem16",					"9B D9 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNSTCW",			"mem16",					"D9 /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTENV",			"mem",						"9B D9 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNSTENV",			"mem",						"D9 /6",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTSW",			"mem16",					"9B DD /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSTSW",			"AX",						"9B DF E0",				Instruction::CPU_286 | Instruction::CPU_FPU},
		{"FNSTSW",			"mem16",					"DD /7",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FNSTSW",			"AX",						"DF E0",				Instruction::CPU_286 | Instruction::CPU_FPU},
		{"FSUB",			"DWORD mem32",				"D8 /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUB",			"QWORD mem64",				"DC /4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUB",			"fpureg",					"D8 E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUB",			"ST0,fpureg",				"D8 E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FSUB",			"TO fpureg",				"DC E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUB",			"fpureg,ST0",				"DC E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBR",			"DWORD mem32",				"D8 /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBR",			"QWORD mem64",				"DC /5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBR",			"fpureg",					"D8 E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBR",			"ST0,fpureg",				"D8 E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FSUBR",			"TO fpureg",				"DC E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBR",			"fpureg,ST0",				"DC E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBP",			"",							"DE E9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBP",			"fpureg",					"DE E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBP",			"fpureg,ST0",				"DE E8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBRP",			"",							"DE E1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBRP",			"fpureg",					"DE E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FSUBRP",			"fpureg,ST0",				"DE E0 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FTST",			"",							"D9 E4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FUCOM",			"fpureg",					"DD E0 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FUCOM",			"ST0,fpureg",				"DD E0 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FUCOMP",			"fpureg",					"DD E8 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FUCOMP",			"ST0,fpureg",				"DD E8 +r",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FUCOMPP",			"",							"DA E9",				Instruction::CPU_386 | Instruction::CPU_FPU},
		{"FUCOMI",			"fpureg",					"DB E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FUCOMI",			"ST0,fpureg",				"DB E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FUCOMIP",			"fpureg",					"DF E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FUCOMIP",			"ST0,fpureg",				"DF E8 +r",				Instruction::CPU_P6 | Instruction::CPU_FPU},
		{"FWAIT",			"",							"9B",					Instruction::CPU_8086},
		{"FXAM",			"",							"D9 E5",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FXCH",			"",							"D9 C9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FXCH",			"fpureg",					"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FXCH",			"fpureg,ST0",				"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FXCH",			"ST0,fpureg",				"D9 C8 +r",				Instruction::CPU_8086 | Instruction::CPU_FPU},
	//	{"FXRSTOR",			"mem",						"0F AE /1",				Instruction::CPU_P6 | Instruction::CPU_SSE | Instruction::CPU_FPU},
	//	{"FXSAVE",			"mem",						"0F AE /0",				Instruction::CPU_P6 | Instruction::CPU_SSE | Instruction::CPU_FPU},
		{"FXTRACT",			"",							"D9 F4",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FYL2X",			"",							"D9 F1",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"FYL2XP1",			"",							"D9 F9",				Instruction::CPU_8086 | Instruction::CPU_FPU},
		{"HLT",				"",							"F4",					Instruction::CPU_8086},
		{"HADDPD",			"xmmreg,r/m128",			"66 0F 7C /r",			Instruction::CPU_PNI},
		{"HADDPS",			"xmmreg,r/m128",			"F2 0F 7C /r",			Instruction::CPU_PNI},
		{"HSUBPD",			"xmmreg,r/m128",			"66 0F 7D /r",			Instruction::CPU_PNI},
		{"HSUBPS",			"xmmreg,r/m128",			"F2 0F 7D /r",			Instruction::CPU_PNI},
	//	{"IBTS",			"r/m16,reg16",				"po 0F A7 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"IBTS",			"r/m32,reg32",				"po 0F A7 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
		{"IDIV",			"BYTE r/m8",				"F6 /7",				Instruction::CPU_8086},
		{"IDIV",			"WORD r/m16",				"po F7 /7",				Instruction::CPU_8086},
		{"IDIV",			"DWORD r/m32",				"po F7 /7",				Instruction::CPU_386},
		{"IMUL",			"BYTE r/m8",				"F6 /5",				Instruction::CPU_8086},
		{"IMUL",			"WORD r/m16",				"po F7 /5",				Instruction::CPU_8086},
		{"IMUL",			"DWORD r/m32",				"po F7 /5",				Instruction::CPU_386},
		{"IMUL",			"reg16,r/m16",				"po 0F AF /r",			Instruction::CPU_386},
		{"IMUL",			"reg32,r/m32",				"po 0F AF /r",			Instruction::CPU_386},
		{"IMUL",			"reg16,imm8",				"po 6B /r ib",			Instruction::CPU_286},
		{"IMUL",			"reg16,imm16",				"po 69 /r iw",			Instruction::CPU_286},
		{"IMUL",			"reg32,imm8",				"po 6B /r ib",			Instruction::CPU_386},
		{"IMUL",			"reg32,imm32",				"po 69 /r id",			Instruction::CPU_386},
		{"IMUL",			"reg16,r/m16,imm8",			"po 6B /r ib",			Instruction::CPU_286},
		{"IMUL",			"reg16,r/m16,imm16",		"po 69 /r iw",			Instruction::CPU_286},
		{"IMUL",			"reg32,r/m32,imm8",			"po 6B /r ib",			Instruction::CPU_386},
		{"IMUL",			"reg32,r/m32,imm32",		"po 69 /r id",			Instruction::CPU_386},
		{"IN",				"AL,imm8",					"E4 ib",				Instruction::CPU_8086},
		{"IN",				"AX,imm8",					"po E5 ib",				Instruction::CPU_8086},
		{"IN",				"EAX,imm8",					"po E5 ib",				Instruction::CPU_386},
		{"IN",				"AL,DX",					"EC",					Instruction::CPU_8086},
		{"IN",				"AX,DX",					"po ED",				Instruction::CPU_8086},
		{"IN",				"EAX,DX",					"po ED",				Instruction::CPU_386},
		{"INC",				"reg16",					"po 40 +r",				Instruction::CPU_8086},
		{"INC",				"reg32",					"po 40 +r",				Instruction::CPU_386},
		{"INC",				"BYTE r/m8",				"FE /0",				Instruction::CPU_8086},
		{"INC",				"WORD r/m16",				"po FF /0",				Instruction::CPU_8086},
		{"INC",				"DWORD r/m32",				"po FF /0",				Instruction::CPU_386},
		{"LOCK INC",		"BYTE mem8",				"p0 FE /0",				Instruction::CPU_8086},
		{"LOCK INC",		"WORD mem16",				"p0 po FF /0",			Instruction::CPU_8086},
		{"LOCK INC",		"DWORD mem32",				"p0 po FF /0",			Instruction::CPU_386},
		{"INSB",			"",							"6C",					Instruction::CPU_186},
		{"INSW",			"",							"po 6D",				Instruction::CPU_186},
		{"INSD",			"",							"po 6D",				Instruction::CPU_386},
		{"REP INSB",		"",							"p3 6C",				Instruction::CPU_186},
		{"REP INSW",		"",							"p3 po 6D",				Instruction::CPU_186},
		{"REP INSD",		"",							"p3 po 6D",				Instruction::CPU_386},
	//	{"INT",				"imm8",						"CD ib",				Instruction::CPU_8086},
	//	{"INT1",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
	//	{"ICEBP",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
	//	{"INT01",			"",							"F1",					Instruction::CPU_P6 | Instruction::CPU_UNDOC},
		{"INT3",			"",							"CC",					Instruction::CPU_8086},
		{"INT03",			"",							"CC",					Instruction::CPU_8086},
		{"INTO",			"",							"CE",					Instruction::CPU_8086},
	//	{"INVD",			"",							"0F 08",				Instruction::CPU_486},
	//	{"INVLPG",			"mem",						"0F 01 /7",				Instruction::CPU_486},
	//	{"IRET",			"",							"CF",					Instruction::CPU_8086},
	//	{"IRETW",			"",							"po CF",				Instruction::CPU_8086},
	//	{"IRETD",			"",							"po CF",				Instruction::CPU_386},
		{"JCXZ",			"NEAR imm8",				"po E3 -b",				Instruction::CPU_8086},
		{"JECXZ",			"NEAR imm8",				"po E3 -b",				Instruction::CPU_386},
		{"JMP",				"imm",						"E9 -i",				Instruction::CPU_8086},
		{"JMP",				"NEAR imm8",				"EB -b",				Instruction::CPU_8086},
	//	{"JMP",				"imm:imm16",				"po EA iw iw",			Instruction::CPU_8086},
	//	{"JMP",				"imm:imm32",				"po EA id iw",			Instruction::CPU_386},
		{"JMP",				"mem",						"po FF /5",				Instruction::CPU_8086},
	//	{"JMP",				"FAR mem",					"po FF /5",				Instruction::CPU_386},
		{"JMP",				"WORD r/m16",				"po FF /4",				Instruction::CPU_8086},
		{"JMP",				"DWORD r/m32",				"po FF /4",				Instruction::CPU_386},
		{"JO",				"NEAR imm8",				"70 -b",				Instruction::CPU_8086},
		{"JNO",				"NEAR imm8",				"71 -b",				Instruction::CPU_8086},
		{"JB",				"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
		{"JC",				"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
		{"JNAE",			"NEAR imm8",				"72 -b",				Instruction::CPU_8086},
		{"JAE",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
		{"JNB",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
		{"JNC",				"NEAR imm8",				"73 -b",				Instruction::CPU_8086},
		{"JE",				"NEAR imm8",				"74 -b",				Instruction::CPU_8086},
		{"JZ",				"NEAR imm8",				"74 -b",				Instruction::CPU_8086},
		{"JNE",				"NEAR imm8",				"75 -b",				Instruction::CPU_8086},
		{"JNZ",				"NEAR imm8",				"75 -b",				Instruction::CPU_8086},
		{"JBE",				"NEAR imm8",				"76 -b",				Instruction::CPU_8086},
		{"JNA",				"NEAR imm8",				"76 -b",				Instruction::CPU_8086},
		{"JA",				"NEAR imm8",				"77 -b",				Instruction::CPU_8086},
		{"JNBE",			"NEAR imm8",				"77 -b",				Instruction::CPU_8086},
		{"JS",				"NEAR imm8",				"78 -b",				Instruction::CPU_8086},
		{"JNS",				"NEAR imm8",				"79 -b",				Instruction::CPU_8086},
		{"JP",				"NEAR imm8",				"7A -b",				Instruction::CPU_8086},
		{"JPE",				"NEAR imm8",				"7A -b",				Instruction::CPU_8086},
		{"JNP",				"NEAR imm8",				"7B -b",				Instruction::CPU_8086},
		{"JPO",				"NEAR imm8",				"7B -b",				Instruction::CPU_8086},
		{"JL",				"NEAR imm8",				"7C -b",				Instruction::CPU_8086},
		{"JNGE",			"NEAR imm8",				"7C -b",				Instruction::CPU_8086},
		{"JGE",				"NEAR imm8",				"7D -b",				Instruction::CPU_8086},
		{"JNL",				"NEAR imm8",				"7D -b",				Instruction::CPU_8086},
		{"JLE",				"NEAR imm8",				"7E -b",				Instruction::CPU_8086},
		{"JNG",				"NEAR imm8",				"7E -b",				Instruction::CPU_8086},
		{"JG",				"NEAR imm8",				"7F -b",				Instruction::CPU_8086},
		{"JNLE",			"NEAR imm8",				"7F -b",				Instruction::CPU_8086},
		{"JO",				"imm",						"0F 80 -i",				Instruction::CPU_386},
		{"JNO",				"imm",						"0F 81 -i",				Instruction::CPU_386},
		{"JB",				"imm",						"0F 82 -i",				Instruction::CPU_386},
		{"JC",				"imm",						"0F 82 -i",				Instruction::CPU_386},
		{"JNAE",			"imm",						"0F 82 -i",				Instruction::CPU_386},
		{"JAE",				"imm",						"0F 83 -i",				Instruction::CPU_386},
		{"JNB",				"imm",						"0F 83 -i",				Instruction::CPU_386},
		{"JNC",				"imm",						"0F 83 -i",				Instruction::CPU_386},
		{"JE",				"imm",						"0F 84 -i",				Instruction::CPU_386},
		{"JZ",				"imm",						"0F 84 -i",				Instruction::CPU_386},
		{"JNE",				"imm",						"0F 85 -i",				Instruction::CPU_386},
		{"JNZ",				"imm",						"0F 85 -i",				Instruction::CPU_386},
		{"JBE",				"imm",						"0F 86 -i",				Instruction::CPU_386},
		{"JNA",				"imm",						"0F 86 -i",				Instruction::CPU_386},
		{"JA",				"imm",						"0F 87 -i",				Instruction::CPU_386},
		{"JNBE",			"imm",						"0F 87 -i",				Instruction::CPU_386},
		{"JS",				"imm",						"0F 88 -i",				Instruction::CPU_386},
		{"JNS",				"imm",						"0F 89 -i",				Instruction::CPU_386},
		{"JP",				"imm",						"0F 8A -i",				Instruction::CPU_386},
		{"JPE",				"imm",						"0F 8A -i",				Instruction::CPU_386},
		{"JNP",				"imm",						"0F 8B -i",				Instruction::CPU_386},
		{"JPO",				"imm",						"0F 8B -i",				Instruction::CPU_386},
		{"JL",				"imm",						"0F 8C -i",				Instruction::CPU_386},
		{"JNGE",			"imm",						"0F 8C -i",				Instruction::CPU_386},
		{"JGE",				"imm",						"0F 8D -i",				Instruction::CPU_386},
		{"JNL",				"imm",						"0F 8D -i",				Instruction::CPU_386},
		{"JLE",				"imm",						"0F 8E -i",				Instruction::CPU_386},
		{"JNG",				"imm",						"0F 8E -i",				Instruction::CPU_386},
		{"JG",				"imm",						"0F 8F -i",				Instruction::CPU_386},
		{"JNLE",			"imm",						"0F 8F -i",				Instruction::CPU_386},
		{"LAHF",			"",							"9F",					Instruction::CPU_8086},
	//	{"LAR",				"reg16,r/m16",				"po 0F 02 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LAR",				"reg32,r/m32",				"po 0F 02 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"LDS",				"reg16,mem",				"po C5 /r",				Instruction::CPU_8086},
		{"LDS",				"reg32,mem",				"po C5 /r",				Instruction::CPU_8086},
		{"LES",				"reg16,mem",				"po C4 /r",				Instruction::CPU_8086},
		{"LES",				"reg32,mem",				"po C4 /r",				Instruction::CPU_8086},
		{"LFS",				"reg16,mem",				"po 0F B4 /r",			Instruction::CPU_386},
		{"LFS",				"reg32,mem",				"po 0F B4 /r",			Instruction::CPU_386},
		{"LGS",				"reg16,mem",				"po 0F B5 /r",			Instruction::CPU_386},
		{"LGS",				"reg32,mem",				"po 0F B5 /r",			Instruction::CPU_386},
		{"LSS",				"reg16,mem",				"po 0F B2 /r",			Instruction::CPU_386},
		{"LSS",				"reg32,mem",				"po 0F B2 /r",			Instruction::CPU_386},
		{"LDMXCSR",			"mem32",					"0F AE /2",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"LEA",				"reg16,mem",				"po 8D /r",				Instruction::CPU_8086},
		{"LEA",				"reg32,mem",				"po 8D /r",				Instruction::CPU_386},
		{"LEAVE",			"",							"C9",					Instruction::CPU_186},
		{"LFENCE",			"",							"0F AE E8",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
	//	{"LGDT",			"mem",						"0F 01 /2",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LIDT",			"mem",						"0F 01 /3",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LLDT",			"r/m16",					"0F 00 /2",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LMSW",			"r/m16",					"0F 01 /6",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LOADALL",			"",							"0F 07",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"LOADALL286",		"",							"0F 05",				Instruction::CPU_286 | Instruction::CPU_UNDOC},
		{"LODSB",			"",							"AC",					Instruction::CPU_8086},
		{"LODSW",			"",							"po AD",				Instruction::CPU_8086},
		{"LODSD",			"",							"po AD",				Instruction::CPU_386},
		{"REP LODSB",		"",							"p3 AC",				Instruction::CPU_8086},
		{"REP LODSW",		"",							"p3 po AD",				Instruction::CPU_8086},
		{"REP LODSD",		"",							"p3 po AD",				Instruction::CPU_386},
		{"LOOP",			"imm",						"E2 -b",				Instruction::CPU_8086},
		{"LOOP",			"imm,CX",					"pa E2 -b",				Instruction::CPU_8086},
		{"LOOP",			"imm,ECX",					"pa E2 -b",				Instruction::CPU_386},
		{"LOOPE",			"imm",						"E1 -b",				Instruction::CPU_8086},
		{"LOOPE",			"imm,CX",					"pa E1 -b",				Instruction::CPU_8086},
		{"LOOPE",			"imm,ECX",					"pa E1 -b",				Instruction::CPU_386},
		{"LOOPZ",			"imm",						"E1 -b",				Instruction::CPU_8086},
		{"LOOPZ",			"imm,CX",					"pa E1 -b",				Instruction::CPU_8086},
		{"LOOPZ",			"imm,ECX",					"pa E1 -b",				Instruction::CPU_386},
		{"LOOPNE",			"imm",						"E0 -b",				Instruction::CPU_8086},
		{"LOOPNE",			"imm,CX",					"pa E0 -b",				Instruction::CPU_8086},
		{"LOOPNE",			"imm,ECX",					"pa E0 -b",				Instruction::CPU_386},
		{"LOOPNZ",			"imm",						"E0 -b",				Instruction::CPU_8086},
		{"LOOPNZ",			"imm,CX",					"pa E0 -b",				Instruction::CPU_8086},
		{"LOOPNZ",			"imm,ECX",					"pa E0 -b",				Instruction::CPU_386},
	//	{"LSL",				"reg16,r/m16",				"po 0F 03 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LSL",				"reg32,r/m32",				"po 0F 03 /r",			Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"LTR",				"r/m16",					"0F 00 /3",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"MASKMOVDQU",		"xmmreg,xmmreg",			"66 0F F7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MASKMOVQ",		"mmreg,mmreg",				"0F F7 /r",				Instruction::CPU_KATMAI},
		{"MAXPD",			"xmmreg,r/m128",			"66 0F 5F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MAXPS",			"xmmreg,r/m128",			"0F 5F /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MAXSD",			"xmmreg,xmm64",				"p2 0F 5F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MAXSS",			"xmmreg,xmm32",				"p3 0F 5F /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MFENCE",			"",							"0F AE F0",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MINPD",			"xmmreg,r/m128",			"66 0F 5D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MINPS",			"xmmreg,r/m128",			"0F 5D /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MINSD",			"xmmreg,xmm64",				"p2 0F 5D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MINSS",			"xmmreg,xmm32",				"p3 0F 5D /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MONITOR",			"",							"0F 01 C8",				Instruction::CPU_PNI},
		{"MWAIT",			"",							"0F 01 C9",				Instruction::CPU_PNI},
		{"MOV",				"r/m8,reg8",				"88 /r",				Instruction::CPU_8086},
		{"MOV",				"r/m16,reg16",				"po 89 /r",				Instruction::CPU_8086},
		{"MOV",				"r/m32,reg32",				"po 89 /r",				Instruction::CPU_386},
		{"MOV",				"reg8,r/m8",				"8A /r",				Instruction::CPU_8086},
		{"MOV",				"reg16,r/m16",				"po 8B /r",				Instruction::CPU_8086},
		{"MOV",				"reg32,r/m32",				"po 8B /r",				Instruction::CPU_386},
		{"MOV",				"reg8,imm8",				"B0 +r ib",				Instruction::CPU_8086},
		{"MOV",				"reg16,imm16",				"po B8 +r iw",			Instruction::CPU_8086},
		{"MOV",				"reg32,imm32",				"po B8 +r id",			Instruction::CPU_386},
		{"MOV",				"BYTE r/m8,imm8",			"C6 /0 ib",				Instruction::CPU_8086},
		{"MOV",				"WORD r/m16,imm16",			"po C7 /0 iw",			Instruction::CPU_8086},
		{"MOV",				"DWORD r/m32,imm32",		"po C7 /0 id",			Instruction::CPU_386},
	//	{"MOV",				"AL,memoffs8",				"A0 id",				Instruction::CPU_8086},
	//	{"MOV",				"AX,memoffs16",				"po A1 id",				Instruction::CPU_8086},
	//	{"MOV",				"EAX,memoffs32",			"po A1 id",				Instruction::CPU_386},
	//	{"MOV",				"memoffs8,AL",				"A2 id",				Instruction::CPU_8086},
	//	{"MOV",				"memoffs16,AX",				"po A3 id",				Instruction::CPU_8086},
	//	{"MOV",				"memoffs32,EAX",			"po A3 id",				Instruction::CPU_386},
	//	{"MOV",				"r/m16,segreg",				"po 8C /r",				Instruction::CPU_8086},
	//	{"MOV",				"r/m32,segreg",				"po 8C /r",				Instruction::CPU_386},
	//	{"MOV",				"segreg,r/m16",				"po 8E /r",				Instruction::CPU_8086},
	//	{"MOV",				"segreg,r/m32",				"po 8E /r",				Instruction::CPU_386},
	//	{"MOV",				"reg32,CR",					"0F 20 /r",				Instruction::CPU_386},
	//	{"MOV",				"reg32,DR",					"0F 21 /r",				Instruction::CPU_386},
	//	{"MOV",				"reg32,TR",					"0F 24 /r",				Instruction::CPU_386},
	//	{"MOV",				"CR,reg32",					"0F 22 /r",				Instruction::CPU_386},
	//	{"MOV",				"DR,reg32",					"0F 23 /r",				Instruction::CPU_386},
	//	{"MOV",				"TR,reg32",					"0F 26 /r",				Instruction::CPU_386},
		{"MOVAPD",			"xmmreg,r/m128",			"66 0F 28 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVAPD",			"r/m128,xmmreg",			"66 0F 29 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVAPS",			"xmmreg,r/m128",			"0F 28 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVAPS",			"r/m128,xmmreg",			"0F 29 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVD",			"mmreg,r/m32",				"0F 6E /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"MOVD",			"r/m32,mmreg",				"0F 7E /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"MOVD",			"xmmreg,r/m32",				"66 0F 6E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVD",			"r/m32,xmmreg",				"66 0F 7E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVDQ2Q",			"mmreg,xmmreg",				"p2 OF D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVDQA",			"xmmreg,r/m128",			"66 OF 6F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVDQA",			"r/m128,xmmreg",			"66 OF 7F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVDQU",			"xmmreg,r/m128",			"p3 OF 6F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVDQU",			"r/m128,xmmreg",			"p3 OF 7F /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"LDDQU",			"xmmreg,mem",				"F2 0F F0 /r",			Instruction::CPU_PNI},  
		{"MOVHPD",			"xmmreg,mem64",				"66 OF 16 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVHPD",			"mem64,xmmreg",				"66 OF 17 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVHLPS",			"xmmreg,xmmreg",			"0F 12 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVLPD",			"xmmreg,mem64",				"66 OF 12 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVLPD",			"mem64,xmmreg",				"66 OF 13 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVHPS",			"xmmreg,mem64",				"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVHPS",			"mem64,xmmreg",				"0F 17 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVHPS",			"xmmreg,xmmreg",			"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVLHPS",			"xmmreg,xmmreg",			"0F 16 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVLPS",			"xmmreg,mem64",				"0F 12 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVLPS",			"mem64,xmmreg",				"0F 13 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVMSKPD",		"reg32,xmmreg",				"66 0F 50 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVMSKPS",		"reg32,xmmreg",				"0F 50 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVNTDQ",			"mem128,xmmreg",			"66 0F E7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVNTI",			"mem32,reg32",				"0F C3 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVNTPD",			"mem128,xmmreg",			"66 0F 2B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVNTPS",			"mem128,xmmreg",			"0F 2B /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVNTQ",			"mem64,mmreg",				"0F E7 /r",				Instruction::CPU_KATMAI},
		{"MOVQ",			"mmreg,r/m64",				"0F 6F /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"MOVQ",			"r/m64,mmreg",				"0F 7F /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"MOVQ",			"xmmreg,xmm64",				"p3 0F 7E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVQ",			"xmm64,xmmreg",				"66 0F D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVQ2DQ",			"xmmreg,mmreg",				"p3 OF D6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVSB",			"",							"A4",					Instruction::CPU_8086},
		{"MOVSW",			"",							"po A5",				Instruction::CPU_8086},
		{"MOVSD",			"",							"po A5",				Instruction::CPU_386},
		{"REP MOVSB",		"",							"p3 A4",				Instruction::CPU_8086},
		{"REP MOVSW",		"",							"p3 po A5",				Instruction::CPU_8086},
		{"REP MOVSD",		"",							"p3 po A5",				Instruction::CPU_386},
		{"MOVSD",			"xmmreg,xmm64",				"p2 0F 10 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVSD",			"xmm64,xmmreg",				"p2 0F 11 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVSS",			"xmmreg,xmm32",				"p3 0F 10 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVSS",			"xmm32,xmmreg",				"p3 0F 11 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVSX",			"reg16,r/m8",				"po 0F BE /r",			Instruction::CPU_386},
		{"MOVSX",			"reg32,r/m8",				"po 0F BE /r",			Instruction::CPU_386},
		{"MOVSX",			"reg32,r/m16",				"po 0F BF /r",			Instruction::CPU_386},
		{"MOVSHDUP",		"xmmreg,r/m128",			"F3 0F 16 /r",			Instruction::CPU_PNI},
		{"MOVSLDUP",		"xmmreg,r/m128",			"F3 0F 12 /r",			Instruction::CPU_PNI},
		{"MOVDDUP",			"xmmreg,r/m128",			"F2 0F 12 /r",			Instruction::CPU_PNI},
		{"MOVZX",			"reg16,r/m8",				"po 0F B6 /r",			Instruction::CPU_386},
		{"MOVZX",			"reg32,r/m8",				"po 0F B6 /r",			Instruction::CPU_386},
		{"MOVZX",			"reg32,r/m16",				"po 0F B7 /r",			Instruction::CPU_386},
		{"MOVUPD",			"xmmreg,r/m128",			"66 0F 10 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"MOVUPD",			"r/m128,xmmreg",			"66 0F 11 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MOVUPS",			"xmmreg,r/m128",			"0F 10 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MOVUPS",			"r/m128,xmmreg",			"0F 11 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MUL",				"BYTE r/m8",				"F6 /4",				Instruction::CPU_8086},
		{"MUL",				"WORD r/m16",				"po F7 /4",				Instruction::CPU_8086},
		{"MUL",				"DWORD r/m32",				"po F7 /4",				Instruction::CPU_386},
		{"MULPD",			"xmmreg,r/m128",			"66 0F 59 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MULPS",			"xmmreg,r/m128",			"0F 59 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"MULSD",			"xmmreg,xmm64",				"p2 0F 59 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"MULSS",			"xmmreg,xmm32",				"p3 0F 59 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"NEG",				"BYTE r/m8",				"F6 /3",				Instruction::CPU_8086},
		{"NEG",				"WORD r/m16",				"po F7 /3",				Instruction::CPU_8086},
		{"NEG",				"DWORD r/m32",				"po F7 /3",				Instruction::CPU_386},
		{"LOCK NEG",		"BYTE mem8",				"p0 F6 /3",				Instruction::CPU_8086},
		{"LOCK NEG",		"WORD mem16",				"p0 po F7 /3",			Instruction::CPU_8086},
		{"LOCK NEG",		"DWORD mem32",				"p0 po F7 /3",			Instruction::CPU_386},
		{"NOT",				"BYTE r/m8",				"F6 /2",				Instruction::CPU_8086},
		{"NOT",				"WORD r/m16",				"po F7 /2",				Instruction::CPU_8086},
		{"NOT",				"DWORD r/m32",				"po F7 /2",				Instruction::CPU_386},
		{"LOCK NOT",		"BYTE mem8",				"p0 F6 /2",				Instruction::CPU_8086},
		{"LOCK NOT",		"WORD mem16",				"p0 po F7 /2",			Instruction::CPU_8086},
		{"LOCK NOT",		"DWORD mem32",				"p0 po F7 /2",			Instruction::CPU_386},
		{"NOP",				"",							"90",					Instruction::CPU_8086},
		{"OR",				"r/m8,reg8",				"08 /r",				Instruction::CPU_8086},
		{"OR",				"r/m16,reg16",				"po 09 /r",				Instruction::CPU_8086},
		{"OR",				"r/m32,reg32",				"po 09 /r",				Instruction::CPU_386},
		{"LOCK OR",			"mem8,reg8",				"p0 08 /r",				Instruction::CPU_8086},
		{"LOCK OR",			"mem16,reg16",				"p0 po 09 /r",			Instruction::CPU_8086},
		{"LOCK OR",			"mem32,reg32",				"p0 po 09 /r",			Instruction::CPU_386},
		{"OR",				"reg8,r/m8",				"0A /r",				Instruction::CPU_8086},
		{"OR",				"reg16,r/m16",				"po 0B /r",				Instruction::CPU_8086},
		{"OR",				"reg32,r/m32",				"po 0B /r",				Instruction::CPU_386},
		{"OR",				"BYTE r/m8,imm8",			"80 /1 ib",				Instruction::CPU_8086},
		{"OR",				"WORD r/m16,imm16",			"po 81 /1 iw",			Instruction::CPU_8086},
		{"OR",				"DWORD r/m32,imm32",		"po 81 /1 id",			Instruction::CPU_386},
		{"OR",				"WORD r/m16,imm8",			"po 83 /1 ib",			Instruction::CPU_8086},
		{"OR",				"DWORD r/m32,imm8",			"po 83 /1 ib",			Instruction::CPU_386},
		{"LOCK OR",			"BYTE mem8,imm8",			"p0 80 /1 ib",			Instruction::CPU_8086},
		{"LOCK OR",			"WORD mem16,imm16",			"p0 po 81 /1 iw",		Instruction::CPU_8086},
		{"LOCK OR",			"DWORD mem32,imm32",		"p0 po 81 /1 id",		Instruction::CPU_386},
		{"LOCK OR",			"WORD mem16,imm8",			"p0 po 83 /1 ib",		Instruction::CPU_8086},
		{"LOCK OR",			"DWORD mem32,imm8",			"p0 po 83 /1 ib",		Instruction::CPU_386},
		{"OR",				"AL,imm8",					"0C ib",				Instruction::CPU_8086},
		{"OR",				"AX,imm16",					"po 0D iw",				Instruction::CPU_8086},
		{"OR",				"EAX,imm32",				"po 0D id",				Instruction::CPU_386},
		{"ORPD",			"xmmreg,r/m128",			"66 0F 56 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"ORPS",			"xmmreg,r/m128",			"0F 56 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"OUT",				"imm8,AL",					"E6 ib",				Instruction::CPU_8086},
		{"OUT",				"imm8,AX",					"po E7 ib",				Instruction::CPU_8086},
		{"OUT",				"imm8,EAX",					"po E7 ib",				Instruction::CPU_386},
		{"OUT",				"DX,AL",					"EE",					Instruction::CPU_8086},
		{"OUT",				"DX,AX",					"po EF",				Instruction::CPU_8086},
		{"OUT",				"DX,EAX",					"po EF",				Instruction::CPU_386},
		{"OUTSB",			"",							"6E",					Instruction::CPU_186},
		{"OUTSW",			"",							"po 6F",				Instruction::CPU_186},
		{"OUTSD",			"",							"po 6F",				Instruction::CPU_386},
		{"REP OUTSB",		"",							"p3 6E",				Instruction::CPU_186},
		{"REP OUTSW",		"",							"p3 po 6F",				Instruction::CPU_186},
		{"REP OUTSD",		"",							"p3 po 6F",				Instruction::CPU_386},
		{"PACKSSDW",		"mmreg,r/m64",				"0F 6B /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PACKSSWB",		"mmreg,r/m64",				"0F 63 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PACKUSWB",		"mmreg,r/m64",				"0F 67 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PACKSSDW",		"xmmreg,r/m128",			"66 0F 6B /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PACKSSWB",		"xmmreg,r/m128",			"66 0F 63 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PACKUSWB",		"xmmreg,r/m128",			"66 0F 67 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDB",			"mmreg,r/m64",				"0F FC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDW",			"mmreg,r/m64",				"0F FD /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDD",			"mmreg,r/m64",				"0F FE /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDB",			"xmmreg,r/m128",			"66 0F FC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDW",			"xmmreg,r/m128",			"66 0F FD /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDD",			"xmmreg,r/m128",			"66 0F FE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDQ",			"mmreg,r/m64",				"0F D4 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDQ",			"xmmreg,r/m128",			"66 0F D4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDSB",			"mmreg,r/m64",				"0F EC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDSW",			"mmreg,r/m64",				"0F ED /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDSB",			"xmmreg,r/m128",			"66 0F EC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDSW",			"xmmreg,r/m128",			"66 0F ED /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDUSB",			"mmreg,r/m64",				"0F DC /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDUSW",			"mmreg,r/m64",				"0F DD /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PADDUSB",			"xmmreg,r/m128",			"66 0F DC /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PADDUSW",			"xmmreg,r/m128",			"66 0F DD /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PADDSIW",			"mmreg,r/m64",				"0F 51 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PAND",			"mmreg,r/m64",				"0F DB /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PANDN",			"mmreg,r/m64",				"0F DF /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PAND",			"xmmreg,r/m128",			"66 0F DB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PANDN",			"xmmreg,r/m128",			"66 0F DF /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PAUSE",			"",							"p3 90",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PAVEB",			"mmreg,r/m64",				"0F 50 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PAVGB",			"mmreg,r/m64",				"0F E0 /r",				Instruction::CPU_KATMAI},
		{"PAVGW",			"mmreg,r/m64",				"0F E3 /r",				Instruction::CPU_KATMAI},
		{"PAVGB",			"xmmreg,r/m128",			"66 0F E0 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PAVGW",			"xmmreg,r/m128",			"66 0F E3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PAVGUSB",			"mmreg,r/m64",				"0F 0F /r BF",			Instruction::CPU_3DNOW},
		{"PCMPEQB",			"mmreg,r/m64",				"0F 74 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPEQW",			"mmreg,r/m64",				"0F 75 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPEQD",			"mmreg,r/m64",				"0F 76 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPGTB",			"mmreg,r/m64",				"0F 64 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPGTW",			"mmreg,r/m64",				"0F 65 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPGTD",			"mmreg,r/m64",				"0F 66 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PCMPEQB",			"xmmreg,r/m128",			"66 0F 74 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PCMPEQW",			"xmmreg,r/m128",			"66 0F 75 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PCMPEQD",			"xmmreg,r/m128",			"66 0F 76 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PCMPGTB",			"xmmreg,r/m128",			"66 0F 64 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PCMPGTW",			"xmmreg,r/m128",			"66 0F 65 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PCMPGTD",			"xmmreg,r/m128",			"66 0F 66 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PDISTIB",			"mmreg,mem64",				"0F 54 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PEXTRW",			"reg32,mmreg,imm8",			"0F C5 /r ib",			Instruction::CPU_KATMAI},
		{"PEXTRW",			"reg32,xmmreg,imm8",		"66 0F C5 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PF2ID",			"mmreg,r/m64",				"0F 0F /r 1D",			Instruction::CPU_3DNOW},
		{"PF2IW",			"mmreg,r/m64",				"0F 0F /r 1C",			Instruction::CPU_ATHLON},
		{"PFACC",			"mmreg,r/m64",				"0F 0F /r AE",			Instruction::CPU_3DNOW},
		{"PFADD",			"mmreg,r/m64",				"0F 0F /r 9E",			Instruction::CPU_3DNOW},
		{"PFCMPEQ",			"mmreg,r/m64",				"0F 0F /r B0",			Instruction::CPU_3DNOW},
		{"PFCMPGE",			"mmreg,r/m64",				"0F 0F /r 90",			Instruction::CPU_3DNOW},
		{"PFCMPGT",			"mmreg,r/m64",				"0F 0F /r A0",			Instruction::CPU_3DNOW},
		{"PFMAX",			"mmreg,r/m64",				"0F 0F /r A4",			Instruction::CPU_3DNOW},
		{"PFMIN",			"mmreg,r/m64",				"0F 0F /r 94",			Instruction::CPU_3DNOW},
		{"PFMUL",			"mmreg,r/m64",				"0F 0F /r B4",			Instruction::CPU_3DNOW},
		{"PFNACC",			"mmreg,r/m64",				"0F 0F /r 8A",			Instruction::CPU_ATHLON},
		{"PFPNACC",			"mmreg,r/m64",				"0F 0F /r 8E",			Instruction::CPU_ATHLON},
		{"PFRCP",			"mmreg,r/m64",				"0F 0F /r 96",			Instruction::CPU_3DNOW},
		{"PFRCPIT1",		"mmreg,r/m64",				"0F 0F /r A6",			Instruction::CPU_3DNOW},
		{"PFRCPIT2",		"mmreg,r/m64",				"0F 0F /r B6",			Instruction::CPU_3DNOW},
		{"PFRSQIT1",		"mmreg,r/m64",				"0F 0F /r A7",			Instruction::CPU_3DNOW},
		{"PFRSQRT",			"mmreg,r/m64",				"0F 0F /r 97",			Instruction::CPU_3DNOW},
		{"PFSUB",			"mmreg,r/m64",				"0F 0F /r 9A",			Instruction::CPU_3DNOW},
		{"PFSUBR",			"mmreg,r/m64",				"0F 0F /r AA",			Instruction::CPU_3DNOW},
		{"PI2FD",			"mmreg,r/m64",				"0F 0F /r 0D",			Instruction::CPU_3DNOW},
		{"PI2FW",			"mmreg,r/m64",				"0F 0F /r 0C",			Instruction::CPU_ATHLON},
		{"PINSRW",			"mmreg,r/m16,imm8",			"0F C4 /r ib",			Instruction::CPU_KATMAI},
		{"PINSRW",			"xmmreg,r/m16,imm8",		"66 0F C4 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMACHRIW",		"mmreg,mem64",				"0F 5E /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMADDWD",			"mmreg,r/m64",				"0F F5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PMADDWD",			"xmmreg,r/m128",			"66 0F F5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMAGW",			"mmreg,r/m64",				"0F 52 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMAXSW",			"xmmreg,r/m128",			"66 0F EE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMAXSW",			"mmreg,r/m64",				"0F EE /r",				Instruction::CPU_KATMAI},
		{"PMAXUB",			"mmreg,r/m64",				"0F DE /r",				Instruction::CPU_KATMAI},
		{"PMAXUB",			"xmmreg,r/m128",			"66 0F DE /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMINSW",			"mmreg,r/m64",				"0F EA /r",				Instruction::CPU_KATMAI},
		{"PMINSW",			"xmmreg,r/m128",			"66 0F EA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMINUB",			"mmreg,r/m64",				"0F DA /r",				Instruction::CPU_KATMAI},
		{"PMINUB",			"xmmreg,r/m128",			"66 0F DA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMOVMSKB",		"reg32,mmreg",				"0F D7 /r",				Instruction::CPU_KATMAI},
		{"PMOVMSKB",		"reg32,xmmreg",				"66 0F D7 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMULHRWA",		"mmreg,r/m64",				"0F 0F /r B7",			Instruction::CPU_3DNOW},
		{"PMULHRWC",		"mmreg,r/m64",				"0F 59 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMULHRIW",		"mmreg,r/m64",				"0F 5D /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMULHUW",			"mmreg,r/m64",				"0F E4 /r",				Instruction::CPU_KATMAI},
		{"PMULHUW",			"xmmreg,r/m128",			"66 0F E4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMULHW",			"mmreg,r/m64",				"0F E5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PMULLW",			"mmreg,r/m64",				"0F D5 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PMULHW",			"xmmreg,r/m128",			"66 0F E5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PMULLW",			"xmmreg,r/m128",			"66 0F D5 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMULUDQ",			"mmreg,r/m64",				"0F F4 /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PMULUDQ",			"xmmreg,r/m128",			"66 0F F4 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PMVZB",			"mmreg,mem64",				"0F 58 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMVNZB",			"mmreg,mem64",				"0F 5A /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMVLZB",			"mmreg,mem64",				"0F 5B /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PMVGEZB",			"mmreg,mem64",				"0F 5C /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"POP",				"reg16",					"po 58 +r",				Instruction::CPU_8086},
		{"POP",				"reg32",					"po 58 +r",				Instruction::CPU_386},
		{"POP",				"WORD r/m16",				"po 8F /0",				Instruction::CPU_8086},
		{"POP",				"DWORD r/m32",				"po 8F /0",				Instruction::CPU_386},
	//	{"POP",				"CS",						"0F",					Instruction::CPU_8086 | Instruction::CPU_UNDOC},
	//	{"POP",				"DS",						"1F",					Instruction::CPU_8086},
	//	{"POP",				"ES",						"07",					Instruction::CPU_8086},
	//	{"POP",				"SS",						"17",					Instruction::CPU_8086},
	//	{"POP",				"FS",						"0F A1",				Instruction::CPU_386},
	//	{"POP",				"GS",						"0F A9",				Instruction::CPU_386},
		{"POPA",			"",							"61",					Instruction::CPU_186},
		{"POPAW",			"",							"po 61",				Instruction::CPU_186},
		{"POPAD",			"",							"po 61",				Instruction::CPU_386},
		{"POPF",			"",							"9D",					Instruction::CPU_186},
		{"POPFW",			"",							"po 9D",				Instruction::CPU_186},
		{"POPFD",			"",							"po 9D",				Instruction::CPU_386},
		{"POR",				"mmreg,r/m64",				"0F EB /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"POR",				"xmmreg,r/m128",			"66 0F EB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PREFETCH",		"mem",						"0F 0D /0",				Instruction::CPU_3DNOW},
		{"PREFETCHW",		"mem",						"0F 0D /1",				Instruction::CPU_3DNOW},
		{"PREFETCHNTA",		"mem",						"0F 18 /0",				Instruction::CPU_KATMAI},
		{"PREFETCHT0",		"mem",						"0F 18 /1",				Instruction::CPU_KATMAI},
		{"PREFETCHT1",		"mem",						"0F 18 /2",				Instruction::CPU_KATMAI},
		{"PREFETCHT2",		"mem",						"0F 18 /3",				Instruction::CPU_KATMAI},
		{"PSADBW",			"mmreg,r/m64",				"0F F6 /r",				Instruction::CPU_KATMAI},
		{"PSADBW",			"xmmreg,r/m128",			"66 0F F6 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSHUFD",			"xmmreg,r/m128,imm8",		"66 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSHUFHW",			"xmmreg,r/m128,imm8",		"p3 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSHUFLW",			"xmmreg,r/m128,imm8",		"p2 0F 70 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSHUFW",			"mmreg,r/m64,imm8",			"0F 70 /r ib",			Instruction::CPU_KATMAI},
		{"PSLLW",			"mmreg,r/m64",				"0F F1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLW",			"mmreg,imm8",				"0F 71 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLW",			"xmmreg,r/m128",			"66 0F F1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSLLW",			"xmmreg,imm8",				"66 0F 71 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSLLD",			"mmreg,r/m64",				"0F F2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLD",			"mmreg,imm8",				"0F 72 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLD",			"xmmreg,r/m128",			"66 0F F2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSLLD",			"xmmreg,imm8",				"66 0F 72 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSLLQ",			"mmreg,r/m64",				"0F F3 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLQ",			"mmreg,imm8",				"0F 73 /6 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSLLQ",			"xmmreg,r/m128",			"66 0F F3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSLLQ",			"xmmreg,imm8",				"66 0F 73 /6 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRAW",			"mmreg,r/m64",				"0F E1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRAW",			"mmreg,imm8",				"0F 71 /4 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRAW",			"xmmreg,r/m128",			"66 0F E1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSRAW",			"xmmreg,imm8",				"66 0F 71 /4 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRAD",			"mmreg,r/m64",				"0F E2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRAD",			"mmreg,imm8",				"0F 72 /4 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRAD",			"xmmreg,r/m128",			"66 0F E2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRAD",			"xmmreg,imm8",				"66 0F 72 /4 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLW",			"mmreg,r/m64",				"0F D1 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLW",			"mmreg,imm8",				"0F 71 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLW",			"xmmreg,r/m128",			"66 0F D1 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLW",			"xmmreg,imm8",				"66 0F 71 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLD",			"mmreg,r/m64",				"0F D2 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLD",			"mmreg,imm8",				"0F 72 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLD",			"xmmreg,r/m128",			"66 0F D2 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLD",			"xmmreg,imm8",				"66 0F 72 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLQ",			"mmreg,r/m64",				"0F D3 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLQ",			"mmreg,imm8",				"0F 73 /2 ib",			Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSRLQ",			"xmmreg,r/m128",			"66 0F D3 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSRLQ",			"xmmreg,imm8",				"66 0F 73 /2 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSRLDQ",			"xmmreg,imm8",				"66 0F 73 /3 ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSUBB",			"mmreg,r/m64",				"0F F8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBW",			"mmreg,r/m64",				"0F F9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBD",			"mmreg,r/m64",				"0F FA /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBQ",			"mmreg,r/m64",				"0F FB /r",				Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSUBB",			"xmmreg,r/m128",			"66 0F F8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBW",			"xmmreg,r/m128",			"66 0F F9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBD",			"xmmreg,r/m128",			"66 0F FA /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBQ",			"xmmreg,r/m128",			"66 0F FB /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSUBSB",			"mmreg,r/m64",				"0F E8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBSW",			"mmreg,r/m64",				"0F E9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBSB",			"xmmreg,r/m128",			"66 0F E8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBSW",			"xmmreg,r/m128",			"66 0F E9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBUSB",			"mmreg,r/m64",				"0F D8 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBUSW",			"mmreg,r/m64",				"0F D9 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PSUBUSB",			"xmmreg,r/m128",			"66 0F D8 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"PSUBUSW",			"xmmreg,r/m128",			"66 0F D9 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PSUBSIW",			"mmreg,r/m64",				"0F 55 /r",				Instruction::CPU_CYRIX | Instruction::CPU_MMX},
		{"PSWAPD",			"mmreg,r/m64",				"0F 0F /r BB",			Instruction::CPU_ATHLON},
		{"PUNPCKHBW",		"mmreg,r/m64",				"0F 68 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKHWD",		"mmreg,r/m64",				"0F 69 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKHDQ",		"mmreg,r/m64",				"0F 6A /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKHBW",		"xmmreg,r/m128",			"66 0F 68 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKHWD",		"xmmreg,r/m128",			"66 0F 69 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKHDQ",		"xmmreg,r/m128",			"66 0F 6A /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKHQDQ",		"xmmreg,r/m128",			"66 0F 6D /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKLBW",		"mmreg,r/m64",				"0F 60 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKLWD",		"mmreg,r/m64",				"0F 61 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKLDQ",		"mmreg,r/m64",				"0F 62 /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PUNPCKLBW",		"xmmreg,r/m128",			"66 0F 60 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKLWD",		"xmmreg,r/m128",			"66 0F 61 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKLDQ",		"xmmreg,r/m128",			"66 0F 62 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUNPCKLQDQ",		"xmmreg,r/m128",			"66 0F 6C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"PUSH",			"reg16",					"po 50 +r",				Instruction::CPU_8086},
		{"PUSH",			"reg32",					"po 50 +r",				Instruction::CPU_386},
		{"PUSH",			"WORD r/m16",				"po FF /6",				Instruction::CPU_8086},
		{"PUSH",			"DWORD r/m32",				"po FF /6",				Instruction::CPU_386},
	//	{"PUSH",			"CS",						"0E",					Instruction::CPU_8086},
	//	{"PUSH",			"DS",						"1E",					Instruction::CPU_8086},
	//	{"PUSH",			"ES",						"06",					Instruction::CPU_8086},
	//	{"PUSH",			"SS",						"16",					Instruction::CPU_8086},
	//	{"PUSH",			"FS",						"0F A0",				Instruction::CPU_386},
	//	{"PUSH",			"GS",						"0F A8",				Instruction::CPU_386},
		{"PUSH",			"BYTE imm8",				"6A ib",				Instruction::CPU_286},
		{"PUSH",			"WORD imm16",				"po 68 iw",				Instruction::CPU_286},
		{"PUSH",			"DWORD imm32",				"po 68 id",				Instruction::CPU_386},
		{"PUSHA",			"",							"60",					Instruction::CPU_186},
		{"PUSHAD",			"",							"po 60",				Instruction::CPU_386},
		{"PUSHAW",			"",							"po 60",				Instruction::CPU_186},
		{"PUSHF",			"",							"9C",					Instruction::CPU_186},
		{"PUSHFD",			"",							"po 9C",				Instruction::CPU_386},
		{"PUSHFW",			"",							"po 9C",				Instruction::CPU_186},
		{"PXOR",			"mmreg,r/m64",				"0F EF /r",				Instruction::CPU_PENTIUM | Instruction::CPU_MMX},
		{"PXOR",			"xmmreg,r/m128",			"66 0F EF /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"RCL",				"BYTE r/m8,1",				"D0 /2",				Instruction::CPU_8086},
		{"RCL",				"r/m8,CL",					"D2 /2",				Instruction::CPU_8086},
		{"RCL",				"BYTE r/m8,imm8",			"C0 /2 ib",				Instruction::CPU_286},
		{"RCL",				"WORD r/m16,1",				"po D1 /2",				Instruction::CPU_8086},
		{"RCL",				"r/m16,CL",					"po D3 /2",				Instruction::CPU_8086},
		{"RCL",				"WORD r/m16,imm8",			"po C1 /2 ib",			Instruction::CPU_286},
		{"RCL",				"DWORD r/m32,1",			"po D1 /2",				Instruction::CPU_386},
		{"RCL",				"r/m32,CL",					"po D3 /2",				Instruction::CPU_386},
		{"RCL",				"DWORD r/m32,imm8",			"po C1 /2 ib",			Instruction::CPU_386},
		{"RCR",				"BYTE r/m8,1",				"D0 /3",				Instruction::CPU_8086},
		{"RCR",				"r/m8,CL",					"D2 /3",				Instruction::CPU_8086},
		{"RCR",				"BYTE r/m8,imm8",			"C0 /3 ib",				Instruction::CPU_286},
		{"RCR",				"WORD r/m16,1",				"po D1 /3",				Instruction::CPU_8086},
		{"RCR",				"r/m16,CL",					"po D3 /3",				Instruction::CPU_8086},
		{"RCR",				"WORD r/m16,imm8",			"po C1 /3 ib",			Instruction::CPU_286},
		{"RCR",				"DWORD r/m32,1",			"po D1 /3",				Instruction::CPU_386},
		{"RCR",				"r/m32,CL",					"po D3 /3",				Instruction::CPU_386},
		{"RCR",				"DWORD r/m32,imm8",			"po C1 /3 ib",			Instruction::CPU_386},
		{"RCPPS",			"xmmreg,r/m128",			"0F 53 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"RCPSS",			"xmmreg,xmm32",				"p3 0F 53 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"RDMSR",			"",							"0F 32",				Instruction::CPU_PENTIUM},
		{"RDPMC",			"",							"0F 33",				Instruction::CPU_P6},
	//	{"RDSHR",			"",							"0F 36",				Instruction::CPU_P6 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
		{"RDTSC",			"",							"0F 31",				Instruction::CPU_PENTIUM},
		{"RET",				"",							"C3",					Instruction::CPU_8086},
		{"RET",				"imm16",					"C2 iw",				Instruction::CPU_8086},
	//	{"RETF",			"",							"CB",					Instruction::CPU_8086},
	//	{"RETF",			"imm16",					"CA iw",				Instruction::CPU_8086},
	//	{"RETN",			"",							"C3",					Instruction::CPU_8086},
	//	{"RETN",			"imm16",					"C2 iw",				Instruction::CPU_8086},
		{"ROL",				"BYTE r/m8,1",				"D0 /0",				Instruction::CPU_8086},
		{"ROL",				"r/m8,CL",					"D2 /0",				Instruction::CPU_8086},
		{"ROL",				"BYTE r/m8,imm8",			"C0 /0 ib",				Instruction::CPU_286},
		{"ROL",				"WORD r/m16,1",				"po D1 /0",				Instruction::CPU_8086},
		{"ROL",				"r/m16,CL",					"po D3 /0",				Instruction::CPU_8086},
		{"ROL",				"WORD r/m16,imm8",			"po C1 /0 ib",			Instruction::CPU_286},
		{"ROL",				"DWORD r/m32,1",			"po D1 /0",				Instruction::CPU_386},
		{"ROL",				"r/m32,CL",					"po D3 /0",				Instruction::CPU_386},
		{"ROL",				"DWORD r/m32,imm8",			"po C1 /0 ib",			Instruction::CPU_386},
		{"ROR",				"BYTE r/m8,1",				"D0 /1",				Instruction::CPU_8086},
		{"ROR",				"r/m8,CL",					"D2 /1",				Instruction::CPU_8086},
		{"ROR",				"BYTE r/m8,imm8",			"C0 /1 ib",				Instruction::CPU_286},
		{"ROR",				"WORD r/m16,1",				"po D1 /1",				Instruction::CPU_8086},
		{"ROR",				"r/m16,CL",					"po D3 /1",				Instruction::CPU_8086},
		{"ROR",				"WORD r/m16,imm8",			"po C1 /1 ib",			Instruction::CPU_286},
		{"ROR",				"DWORD r/m32,1",			"po D1 /1",				Instruction::CPU_386},
		{"ROR",				"r/m32,CL",					"po D3 /1",				Instruction::CPU_386},
		{"ROR",				"DWORD r/m32,imm8",			"po C1 /1 ib",			Instruction::CPU_386},
	//	{"RSDC",			"segreg,mem80",				"0F 79 /r",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
	//	{"RSLDT",			"mem80",					"0F 7B /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
		{"RSM",				"",							"0F AA",				Instruction::CPU_PENTIUM},
		{"RSQRTPS",			"xmmreg,r/m128",			"0F 52 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"RSQRTSS",			"xmmreg,xmm32",				"p3 0F 52 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	//	{"RSTS",			"mem80",					"0F 7D /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
		{"SAHF",			"",							"9E",					Instruction::CPU_8086},
		{"SAL",				"BYTE r/m8,1",				"D0 /4",				Instruction::CPU_8086},
		{"SAL",				"r/m8,CL",					"D2 /4",				Instruction::CPU_8086},
		{"SAL",				"BYTE r/m8,imm8",			"C0 /4 ib",				Instruction::CPU_286},
		{"SAL",				"WORD r/m16,1",				"po D1 /4",				Instruction::CPU_8086},
		{"SAL",				"r/m16,CL",					"po D3 /4",				Instruction::CPU_8086},
		{"SAL",				"WORD r/m16,imm8",			"po C1 /4 ib",			Instruction::CPU_286},
		{"SAL",				"DWORD r/m32,1",			"po D1 /4",				Instruction::CPU_386},
		{"SAL",				"r/m32,CL",					"po D3 /4",				Instruction::CPU_386},
		{"SAL",				"DWORD r/m32,imm8",			"po C1 /4 ib",			Instruction::CPU_386},
		{"SAR",				"BYTE r/m8,1",				"D0 /7",				Instruction::CPU_8086},
		{"SAR",				"r/m8,CL",					"D2 /7",				Instruction::CPU_8086},
		{"SAR",				"BYTE r/m8,imm8",			"C0 /7 ib",				Instruction::CPU_286},
		{"SAR",				"WORD r/m16,1",				"po D1 /7",				Instruction::CPU_8086},
		{"SAR",				"r/m16,CL",					"po D3 /7",				Instruction::CPU_8086},
		{"SAR",				"WORD r/m16,imm8",			"po C1 /7 ib",			Instruction::CPU_286},
		{"SAR",				"DWORD r/m32,1",			"po D1 /7",				Instruction::CPU_386},
		{"SAR",				"r/m32,CL",					"po D3 /7",				Instruction::CPU_386},
		{"SAR",				"DWORD r/m32,imm8",			"po C1 /7 ib",			Instruction::CPU_386},
	//	{"SALC",			"",							"D6",					Instruction::CPU_8086 | Instruction::CPU_UNDOC},
		{"SBB",				"r/m8,reg8",				"18 /r",				Instruction::CPU_8086},
		{"SBB",				"r/m16,reg16",				"po 19 /r",				Instruction::CPU_8086},
		{"SBB",				"r/m32,reg32",				"po 19 /r",				Instruction::CPU_386},
		{"LOCK SBB",		"mem8,reg8",				"p0 18 /r",				Instruction::CPU_8086},
		{"LOCK SBB",		"mem16,reg16",				"p0 po 19 /r",			Instruction::CPU_8086},
		{"LOCK SBB",		"mem32,reg32",				"p0 po 19 /r",			Instruction::CPU_386},
		{"SBB",				"reg8,r/m8",				"1A /r",				Instruction::CPU_8086},
		{"SBB",				"reg16,r/m16",				"po 1B /r",				Instruction::CPU_8086},
		{"SBB",				"reg32,r/m32",				"po 1B /r",				Instruction::CPU_386},
		{"SBB",				"BYTE r/m8,imm8",			"80 /3 ib",				Instruction::CPU_8086},
		{"SBB",				"WORD r/m16,imm16",			"po 81 /3 iw",			Instruction::CPU_8086},
		{"SBB",				"DWORD r/m32,imm32",		"po 81 /3 id",			Instruction::CPU_386},
		{"SBB",				"WORD r/m16,imm8",			"po 83 /3 ib",			Instruction::CPU_8086},
		{"SBB",				"DWORD r/m32,imm8",			"po 83 /3 ib",			Instruction::CPU_8086},
		{"LOCK SBB",		"BYTE mem8,imm8",			"p0 80 /3 ib",			Instruction::CPU_8086},
		{"LOCK SBB",		"WORD mem16,imm16",			"p0 po 81 /3 iw",		Instruction::CPU_8086},
		{"LOCK SBB",		"DWORD mem32,imm32",		"p0 po 81 /3 id",		Instruction::CPU_386},
		{"LOCK SBB",		"WORD mem16,imm8",			"p0 po 83 /3 ib",		Instruction::CPU_8086},
		{"LOCK SBB",		"DWORD mem32,imm8",			"p0 po 83 /3 ib",		Instruction::CPU_8086},
		{"SBB",				"AL,imm8",					"1C ib",				Instruction::CPU_8086},
		{"SBB",				"AX,imm16",					"po 1D iw",				Instruction::CPU_8086},
		{"SBB",				"EAX,imm32",				"po 1D id",				Instruction::CPU_386},
		{"SCASB",			"",							"AE",					Instruction::CPU_8086},
		{"SCASW",			"",							"po AF",				Instruction::CPU_8086},
		{"SCASD",			"",							"po AF",				Instruction::CPU_386},
		{"REP SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
		{"REP SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
		{"REP SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
		{"REPE SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
		{"REPE SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
		{"REPE SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
		{"REPNE SCASB",		"",							"p2 AE",				Instruction::CPU_8086},
		{"REPNE SCASW",		"",							"p2 po AF",				Instruction::CPU_8086},
		{"REPNE SCASD",		"",							"p2 po AF",				Instruction::CPU_386},
		{"REPZ SCASB",		"",							"p3 AE",				Instruction::CPU_8086},
		{"REPZ SCASW",		"",							"p3 po AF",				Instruction::CPU_8086},
		{"REPZ SCASD",		"",							"p3 po AF",				Instruction::CPU_386},
		{"REPNZ SCASB",		"",							"p2 AE",				Instruction::CPU_8086},
		{"REPNZ SCASW",		"",							"p2 po AF",				Instruction::CPU_8086},
		{"REPNZ SCASD",		"",							"p2 po AF",				Instruction::CPU_386},
		{"SETO",			"r/m8",						"0F 90 /2",				Instruction::CPU_386},
		{"SETNO",			"r/m8",						"0F 91 /2",				Instruction::CPU_386},
		{"SETB",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
		{"SETC",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
		{"SETNEA",			"r/m8",						"0F 92 /2",				Instruction::CPU_386},
		{"SETAE",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
		{"SETNB",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
		{"SETNC",			"r/m8",						"0F 93 /2",				Instruction::CPU_386},
		{"SETE",			"r/m8",						"0F 94 /2",				Instruction::CPU_386},
		{"SETZ",			"r/m8",						"0F 94 /2",				Instruction::CPU_386},
		{"SETNE",			"r/m8",						"0F 95 /2",				Instruction::CPU_386},
		{"SETNZ",			"r/m8",						"0F 95 /2",				Instruction::CPU_386},
		{"SETBE",			"r/m8",						"0F 96 /2",				Instruction::CPU_386},
		{"SETNA",			"r/m8",						"0F 96 /2",				Instruction::CPU_386},
		{"SETA",			"r/m8",						"0F 97 /2",				Instruction::CPU_386},
		{"SETNBE",			"r/m8",						"0F 97 /2",				Instruction::CPU_386},
		{"SETS",			"r/m8",						"0F 98 /2",				Instruction::CPU_386},
		{"SETNS",			"r/m8",						"0F 99 /2",				Instruction::CPU_386},
		{"SETP",			"r/m8",						"0F 9A /2",				Instruction::CPU_386},
		{"SETPE",			"r/m8",						"0F 9A /2",				Instruction::CPU_386},
		{"SETNP",			"r/m8",						"0F 9B /2",				Instruction::CPU_386},
		{"SETPO",			"r/m8",						"0F 9B /2",				Instruction::CPU_386},
		{"SETL",			"r/m8",						"0F 9C /2",				Instruction::CPU_386},
		{"SETNGE",			"r/m8",						"0F 9C /2",				Instruction::CPU_386},
		{"SETGE",			"r/m8",						"0F 9D /2",				Instruction::CPU_386},
		{"SETNL",			"r/m8",						"0F 9D /2",				Instruction::CPU_386},
		{"SETLE",			"r/m8",						"0F 9E /2",				Instruction::CPU_386},
		{"SETNG",			"r/m8",						"0F 9E /2",				Instruction::CPU_386},
		{"SETG",			"r/m8",						"0F 9F /2",				Instruction::CPU_386},
		{"SETNLE",			"r/m8",						"0F 9F /2",				Instruction::CPU_386},
		{"SFENCE",			"",							"0F AE F8",				Instruction::CPU_KATMAI},
	//	{"SGDT",			"mem",						"0F 01 /0",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"SIDT",			"mem",						"0F 01 /1",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"SLDT",			"r/m16",					"0F 00 /0",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"SHL",				"BYTE r/m8,1",				"D0 /4",				Instruction::CPU_8086},
		{"SHL",				"r/m8,CL",					"D2 /4",				Instruction::CPU_8086},
		{"SHL",				"BYTE r/m8,imm8",			"C0 /4 ib",				Instruction::CPU_286},
		{"SHL",				"WORD r/m16,1",				"po D1 /4",				Instruction::CPU_8086},
		{"SHL",				"r/m16,CL",					"po D3 /4",				Instruction::CPU_8086},
		{"SHL",				"WORD r/m16,imm8",			"po C1 /4 ib",			Instruction::CPU_286},
		{"SHL",				"DWORD r/m32,1",			"po D1 /4",				Instruction::CPU_386},
		{"SHL",				"r/m32,CL",					"po D3 /4",				Instruction::CPU_386},
		{"SHL",				"DWORD r/m32,imm8",			"po C1 /4 ib",			Instruction::CPU_386},
		{"SHR",				"BYTE r/m8,1",				"D0 /5",				Instruction::CPU_8086},
		{"SHR",				"r/m8,CL",					"D2 /5",				Instruction::CPU_8086},
		{"SHR",				"BYTE r/m8,imm8",			"C0 /5 ib",				Instruction::CPU_286},
		{"SHR",				"WORD r/m16,1",				"po D1 /5",				Instruction::CPU_8086},
		{"SHR",				"r/m16,CL",					"po D3 /5",				Instruction::CPU_8086},
		{"SHR",				"WORD r/m16,imm8",			"po C1 /5 ib",			Instruction::CPU_286},
		{"SHR",				"DWORD r/m32,1",			"po D1 /5",				Instruction::CPU_386},
		{"SHR",				"r/m32,CL",					"po D3 /5",				Instruction::CPU_386},
		{"SHR",				"DWORD r/m32,imm8",			"po C1 /5 ib",			Instruction::CPU_386},
		{"SHLD",			"r/m16,reg16,imm8",			"po 0F A4 /r ib",		Instruction::CPU_386},
		{"SHLD",			"r/m32,reg32,imm8",			"po 0F A4 /r ib",		Instruction::CPU_386},
		{"SHLD",			"r/m16,reg16,CL",			"po 0F A5 /r",			Instruction::CPU_386},
		{"SHLD",			"r/m32,reg32,CL",			"po 0F A5 /r",			Instruction::CPU_386},
		{"SHRD",			"r/m16,reg16,imm8",			"po 0F AC /r ib",		Instruction::CPU_386},
		{"SHRD",			"r/m32,reg32,imm8",			"po 0F AC /r ib",		Instruction::CPU_386},
		{"SHRD",			"r/m16,reg16,CL",			"po 0F AD /r",			Instruction::CPU_386},
		{"SHRD",			"r/m32,reg32,CL",			"po 0F AD /r",			Instruction::CPU_386},
		{"SHUFPD",			"xmmreg,r/m128,imm8",		"66 0F C6 /r ib",		Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"SHUFPS",			"xmmreg,r/m128,imm8",		"0F C6 /r ib",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	//	{"SMI",				"",							"F1",					Instruction::CPU_386 | Instruction::CPU_UNDOC},
		{"SMINT",			"",							"0F 38",				Instruction::CPU_P6 | Instruction::CPU_CYRIX},
		{"SMINTOLD",		"",							"0F 7E",				Instruction::CPU_486 | Instruction::CPU_CYRIX},
	//	{"SMSW",			"r/m16",					"0F 01 /4",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"SQRTPD",			"xmmreg,r/m128",			"66 0F 51 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"SQRTPS",			"xmmreg,r/m128",			"0F 51 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"SQRTSD",			"xmmreg,xmm64",				"p2 0F 51 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"SQRTSS",			"xmmreg,xmm32",				"p3 0F 51 /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"STC",				"",							"F9",					Instruction::CPU_8086},
		{"STD",				"",							"FD",					Instruction::CPU_8086},
		{"STI",				"",							"FB",					Instruction::CPU_8086},
		{"STMXCSR",			"mem32",					"0F AE /3",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"STOSB",			"",							"AA",					Instruction::CPU_8086},
		{"STOSW",			"",							"po AB",				Instruction::CPU_8086},
		{"STOSD",			"",							"po AB",				Instruction::CPU_386},
		{"REP STOSB",		"",							"p3 AA",				Instruction::CPU_8086},
		{"REP STOSW",		"",							"p3 po AB",				Instruction::CPU_8086},
		{"REP STOSD",		"",							"p3 po AB",				Instruction::CPU_386},
	//	{"STR",				"r/m16",					"0F 00 /1",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"SUB",				"r/m8,reg8",				"28 /r",				Instruction::CPU_8086},
		{"SUB",				"r/m16,reg16",				"po 29 /r",				Instruction::CPU_8086},
		{"SUB",				"r/m32,reg32",				"po 29 /r",				Instruction::CPU_386},
		{"LOCK SUB",		"mem8,reg8",				"p0 28 /r",				Instruction::CPU_8086},
		{"LOCK SUB",		"mem16,reg16",				"p0 po 29 /r",			Instruction::CPU_8086},
		{"LOCK SUB",		"mem32,reg32",				"p0 po 29 /r",			Instruction::CPU_386},
		{"SUB",				"reg8,r/m8",				"2A /r",				Instruction::CPU_8086},
		{"SUB",				"reg16,r/m16",				"po 2B /r",				Instruction::CPU_8086},
		{"SUB",				"reg32,r/m32",				"po 2B /r",				Instruction::CPU_386},
		{"SUB",				"BYTE r/m8,imm8",			"80 /5 ib",				Instruction::CPU_8086},
		{"SUB",				"WORD r/m16,imm16",			"po 81 /5 iw",			Instruction::CPU_8086},
		{"SUB",				"DWORD r/m32,imm32",		"po 81 /5 id",			Instruction::CPU_386},
		{"SUB",				"WORD r/m16,imm8",			"po 83 /5 ib",			Instruction::CPU_8086},
		{"SUB",				"DWORD r/m32,imm8",			"po 83 /5 ib",			Instruction::CPU_386},
		{"LOCK SUB",		"BYTE mem8,imm8",			"p0 80 /5 ib",			Instruction::CPU_8086},
		{"LOCK SUB",		"WORD mem16,imm16",			"p0 po 81 /5 iw",		Instruction::CPU_8086},
		{"LOCK SUB",		"DWORD mem32,imm32",		"p0 po 81 /5 id",		Instruction::CPU_386},
		{"LOCK SUB",		"WORD mem16,imm8",			"p0 po 83 /5 ib",		Instruction::CPU_8086},
		{"LOCK SUB",		"DWORD mem32,imm8",			"p0 po 83 /5 ib",		Instruction::CPU_386},
		{"SUB",				"AL,imm8",					"2C ib",				Instruction::CPU_8086},
		{"SUB",				"AX,imm16",					"po 2D iw",				Instruction::CPU_8086},
		{"SUB",				"EAX,imm32",				"po 2D id",				Instruction::CPU_386},
		{"SUBPD",			"xmmreg,r/m128",			"66 0F 5C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"SUBPS",			"xmmreg,r/m128",			"0F 5C /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"SUBSD",			"xmmreg,xmm64",				"p2 0F 5C /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2},
		{"SUBSS",			"xmmreg,xmm32",				"p3 0F 5C /r",			Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	//	{"SVDC",			"mem80,segreg",				"0F 78 /r",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
	//	{"SVLDT",			"mem80",					"0F 7A /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
	//	{"SVTS",			"mem80",					"0F 7C /0",				Instruction::CPU_486 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
	//	{"SYSCALL",			"",							"0F 05",				Instruction::CPU_P6 | Instruction::CPU_AMD},
		{"SYSENTER",		"",							"0F 34",				Instruction::CPU_P6},
	//	{"SYSEXIT",			"",							"0F 36",				Instruction::CPU_P6 | Instruction::CPU_PRIV},
	//	{"SYSRET",			"",							"0F 07",				Instruction::CPU_P6 | Instruction::CPU_AMD | Instruction::CPU_PRIV},
		{"TEST",			"r/m8,reg8",				"84 /r",				Instruction::CPU_8086},
		{"TEST",			"r/m16,reg16",				"po 85 /r",				Instruction::CPU_8086},
		{"TEST",			"r/m32,reg32",				"po 85 /r",				Instruction::CPU_386},
		{"TEST",			"BYTE r/m8,imm8",			"F6 /0 ib",				Instruction::CPU_8086},
		{"TEST",			"WORD r/m16,imm16",			"po F7 /0 iw",			Instruction::CPU_8086},
		{"TEST",			"DWORD r/m32,imm32",		"po F7 /0 id",			Instruction::CPU_386},
		{"TEST",			"AL,imm8",					"A8 ib",				Instruction::CPU_8086},
		{"TEST",			"AX,imm16",					"po A9 iw",				Instruction::CPU_8086},
		{"TEST",			"EAX,imm32",				"po A9 id",				Instruction::CPU_386},
		{"UCOMISD",			"xmmreg,xmm64",				"66 0F 2E /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"UCOMISS",			"xmmreg,xmm32",				"0F 2E /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"UD2",				"",							"0F 0B",				Instruction::CPU_286},
	//	{"UMOV",			"r/m8,reg8",				"0F 10 /r",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"UMOV",			"r/m16,reg16",				"po 0F 11 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"UMOV",			"r/m32,reg32",				"po 0F 11 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"UMOV",			"reg8,r/m8",				"0F 12 /r",				Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"UMOV",			"reg16,r/m16",				"po 0F 13 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"UMOV",			"reg32,r/m32",				"po 0F 13 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
		{"UNPCKHPD",		"xmmreg,r/m128",			"66 0F 15 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"UNPCKHPS",		"xmmreg,r/m128",			"0F 15 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
		{"UNPCKLPD",		"xmmreg,r/m128",			"66 0F 14 /r",			Instruction::CPU_WILLAMETTE | Instruction::CPU_SSE2}, 
		{"UNPCKLPS",		"xmmreg,r/m128",			"0F 14 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},
	//	{"VERR",			"r/m16",					"0F 00 /4",				Instruction::CPU_286 | Instruction::CPU_PRIV},
	//	{"VERW",			"r/m16",					"0F 00 /5",				Instruction::CPU_286 | Instruction::CPU_PRIV},
		{"WAIT",			"",							"9B",					Instruction::CPU_8086},
	//	{"WBINVD",			"",							"0F 09",				Instruction::CPU_486},
		{"WRMSR",			"",							"0F 30",				Instruction::CPU_PENTIUM},
	//	{"WRSHR",			"",							"0F 37",				Instruction::CPU_P6 | Instruction::CPU_CYRIX | Instruction::CPU_SMM},
		{"XADD",			"r/m8,reg8",				"0F C0 /r",				Instruction::CPU_486},
		{"XADD",			"r/m16,reg16",				"po 0F C1 /r",			Instruction::CPU_486},
		{"XADD",			"r/m32,reg32",				"po 0F C1 /r",			Instruction::CPU_486},
		{"LOCK XADD",		"mem8,reg8",				"p0 0F C0 /r",			Instruction::CPU_486},
		{"LOCK XADD",		"mem16,reg16",				"p0 po 0F C1 /r",		Instruction::CPU_486},
		{"LOCK XADD",		"mem32,reg32",				"p0 po 0F C1 /r",		Instruction::CPU_486},
	//	{"XBTS",			"reg16,r/m16",				"po 0F A6 /r",			Instruction::CPU_386 | Instruction::CPU_UNDOC},
	//	{"XBTS",			"reg32,r/m32",				"po 0F A6 /r",			Instruction::::CPU_386 | Instruction::CPU_UNDOC},
		{"XCHG",			"reg8,r/m8",				"86 /r",				Instruction::CPU_8086},
		{"XCHG",			"reg16,r/m16",				"po 87 /r",				Instruction::CPU_8086},
		{"XCHG",			"reg32,r/m32",				"po 87 /r",				Instruction::CPU_386},
		{"XCHG",			"r/m8,reg8",				"86 /r",				Instruction::CPU_8086},
		{"XCHG",			"r/m16,reg16",				"po 87 /r",				Instruction::CPU_8086},
		{"XCHG",			"r/m32,reg32",				"po 87 /r",				Instruction::CPU_386},
		{"LOCK XCHG",		"mem8,reg8",				"p0 86 /r",				Instruction::CPU_8086},
		{"LOCK XCHG",		"mem16,reg16",				"p0 po 87 /r",			Instruction::CPU_8086},
		{"LOCK XCHG",		"mem32,reg32",				"p0 po 87 /r",			Instruction::CPU_386},
		{"XCHG",			"AX,reg16",					"po 90 +r",				Instruction::CPU_8086},
		{"XCHG",			"EAX,reg32",				"po 90 +r",				Instruction::CPU_386},
		{"XCHG",			"reg16,AX",					"po 90 +r",				Instruction::CPU_8086},
		{"XCHG",			"reg32,EAX",				"po 90 +r",				Instruction::CPU_386},
		{"XLATB",			"",							"D7",					Instruction::CPU_8086},
		{"XOR",				"r/m8,reg8",				"30 /r",				Instruction::CPU_8086},
		{"XOR",				"r/m16,reg16",				"po 31 /r",				Instruction::CPU_8086},
		{"XOR",				"r/m32,reg32",				"po 31 /r",				Instruction::CPU_386},
		{"LOCK XOR",		"mem8,reg8",				"p0 30 /r",				Instruction::CPU_8086},
		{"LOCK XOR",		"mem16,reg16",				"p0 po 31 /r",			Instruction::CPU_8086},
		{"LOCK XOR",		"mem32,reg32",				"p0 po 31 /r",			Instruction::CPU_386},
		{"XOR",				"reg8,r/m8",				"32 /r",				Instruction::CPU_8086},
		{"XOR",				"reg16,r/m16",				"po 33 /r",				Instruction::CPU_8086},
		{"XOR",				"reg32,r/m32",				"po 33 /r",				Instruction::CPU_386},
		{"XOR",				"BYTE r/m8,imm8",			"80 /6 ib",				Instruction::CPU_8086},
		{"XOR",				"WORD r/m16,imm16",			"po 81 /6 iw",			Instruction::CPU_8086},
		{"XOR",				"DWORD r/m32,imm32",		"po 81 /6 id",			Instruction::CPU_386},
		{"XOR",				"WORD r/m16,imm8",			"po 83 /6 ib",			Instruction::CPU_8086},
		{"XOR",				"DWORD r/m32,imm8",			"po 83 /6 ib",			Instruction::CPU_386},
		{"LOCK XOR",		"BYTE mem8,imm8",			"p0 80 /6 ib",			Instruction::CPU_8086},
		{"LOCK XOR",		"WORD mem16,imm16",			"p0 po 81 /6 iw",		Instruction::CPU_8086},
		{"LOCK XOR",		"DWORD mem32,imm32",		"p0 po 81 /6 id",		Instruction::CPU_386},
		{"LOCK XOR",		"WORD mem16,imm8",			"p0 po 83 /6 ib",		Instruction::CPU_8086},
		{"LOCK XOR",		"DWORD mem32,imm8",			"p0 po 83 /6 ib",		Instruction::CPU_386},
		{"XOR",				"AL,imm8",					"34 ib",				Instruction::CPU_8086},
		{"XOR",				"AX,imm16",					"po 35 iw",				Instruction::CPU_8086},
		{"XOR",				"EAX,imm32",				"po 35 id",				Instruction::CPU_386},
		{"XORPS",			"xmmreg,r/m128",			"0F 57 /r",				Instruction::CPU_KATMAI | Instruction::CPU_SSE},

		// Special 'instructions', indicated by the 'p1' prefix
		{"DB",				"",							"p1 ib"},
		{"DW",				"",							"p1 iw"},
		{"DD",				"",							"p1 id"},
		{"DB",				"imm8",						"p1 ib"},
		{"DW",				"imm16",					"p1 iw"},
		{"DD",				"imm32",					"p1 id"},
		{"DB",				"mem",						"p1 01"},
		{"DW",				"mem",						"p1 02"},
		{"DD",				"mem",						"p1 04"},
		{"DB",				"str",						"p1 00"},
		{"ALIGN",			"imm",						"p1 90"},
	};

	int InstructionSet::numInstructions()
	{
		return sizeof(instructionSet) / sizeof(Instruction::Syntax);
	}

	int InstructionSet::numMnemonics()
	{
		static int n = 0;

		if(n == 0)
		{
			n = 1;

			for(int i = 0; i + 1 < numInstructions(); i++)
			{
				int j = 0;

				for(; j < i; j++)
				{
					if(stricmp(instructionSet[i].mnemonic, instructionSet[j].mnemonic) == 0)
					{
						break;
					}
				}

				if(j == i)
				{
					n++;
				}
			}
		}

		return n;
	}

	void InstructionSet::generateIntrinsics()
	{
		if(!intrinsicMap)
		{
			throw INTERNAL_ERROR;
		}

		FILE *file = fopen("Intrinsics.hpp", "w");

		fprintf(file, "/* Automatically generated file, do not modify */\n"
		              "/* To regenerate this file uncomment generateIntrinsics() in InstructionSet.cpp */\n\n");

		fprintf(file, "#ifndef SOFTWIRE_NO_INTRINSICS\n\n");

		fprintf(file, "typedef OperandIMM IMM;\n");
		fprintf(file, "typedef OperandAL AL;\n");
		fprintf(file, "typedef OperandAX AX;\n");
		fprintf(file, "typedef OperandEAX EAX;\n");
		fprintf(file, "typedef OperandDX DX;\n");
		fprintf(file, "typedef OperandCL CL;\n");
		fprintf(file, "typedef OperandCX CX;\n");
		fprintf(file, "typedef OperandECX ECX;\n");
		fprintf(file, "typedef OperandST0 ST0;\n");
		fprintf(file, "typedef OperandREG8 REG8;\n");
		fprintf(file, "typedef OperandREG16 REG16;\n");
		fprintf(file, "typedef OperandREG32 REG32;\n");
		fprintf(file, "typedef OperandFPUREG FPUREG;\n");
		fprintf(file, "typedef OperandMMREG MMREG;\n");
		fprintf(file, "typedef OperandXMMREG XMMREG;\n");
		fprintf(file, "typedef OperandMEM8 MEM8;\n");
		fprintf(file, "typedef OperandMEM16 MEM16;\n");
		fprintf(file, "typedef OperandMEM32 MEM32;\n");
		fprintf(file, "typedef OperandMEM64 MEM64;\n");
		fprintf(file, "typedef OperandMEM128 MEM128;\n");
		fprintf(file, "typedef OperandR_M8 R_M8;\n");
		fprintf(file, "typedef OperandR_M16 R_M16;\n");
		fprintf(file, "typedef OperandR_M32 R_M32;\n");
		fprintf(file, "typedef OperandR_M64 R_M64;\n");
		fprintf(file, "typedef OperandR_M128 R_M128;\n");
		fprintf(file, "typedef OperandXMM32 XMM32;\n");
		fprintf(file, "typedef OperandXMM64 XMM64;\n");
		fprintf(file, "typedef OperandREF REF;\n");
		fprintf(file, "typedef OperandSTR STR;\n");
		fprintf(file, "\n");
		fprintf(file, "typedef Encoding* enc;\n");
		fprintf(file, "\n");

		struct InstructionSignature
		{
			const char *mnemonic;
			
			const char *firstOperand;
			const char *secondOperand;
			const char *thirdOperand;

			Operand::Type firstType;
			Operand::Type secondType;
			Operand::Type thirdType;
		};

		InstructionSignature *uniqueSignature = new InstructionSignature[10000];
		int n = 0;   // Number of unique instructions

		for(int t = 0; t < numInstructions(); t++)
		{
			Instruction *instruction = intrinsicMap[t];

			char mnemonic[256] = {0};
			strcpy(mnemonic, instruction->getMnemonic());
			strlwr(mnemonic);
			if(mnemonic[3] == ' ') mnemonic[3] = '_';	// Append REP prefix
			if(mnemonic[4] == ' ') mnemonic[4] = '_';   // Append LOCK prefix
			if(mnemonic[5] == ' ') mnemonic[5] = '_';   // Append REPNE/REPNZ prefix

			Operand::Type t1 = instruction->getFirstOperand();
			Operand::Type t2 = instruction->getSecondOperand();
			Operand::Type t3 = instruction->getThirdOperand();
			
			const Operand::Register subtypeTable[] =
			{
				{Operand::OPERAND_VOID,		0},

				{Operand::OPERAND_REF,		"REF"},
				{Operand::OPERAND_STR,		"char*"},

				{Operand::OPERAND_IMM,		"int"},
				{Operand::OPERAND_EXT8,		"char"},
				{Operand::OPERAND_IMM8,		"char"},   // No need to discern between them in C++
				{Operand::OPERAND_IMM16,	"short"},
				{Operand::OPERAND_IMM32,	"int"},

				{Operand::OPERAND_REG8,		"REG8"},
				{Operand::OPERAND_REG16,	"REG16"},
				{Operand::OPERAND_REG32,	"REG32"},
				{Operand::OPERAND_FPUREG,	"FPUREG"},
				{Operand::OPERAND_MMREG,	"MMREG"},
				{Operand::OPERAND_XMMREG,	"XMMREG"},

				// Specializations that don't offer significant benefit
				// Placed lower to avoid outputting them
				{Operand::OPERAND_AL,		"AL"},
				{Operand::OPERAND_AX,		"AX"},
				{Operand::OPERAND_EAX,		"EAX"},
				{Operand::OPERAND_DX,		"DX"},
				{Operand::OPERAND_CL,		"CL"},
				{Operand::OPERAND_CX,		"CX"},
				{Operand::OPERAND_ECX,		"ECX"},
				{Operand::OPERAND_ST0,		"ST0"},

				{Operand::OPERAND_MEM8,		"MEM8"},
				{Operand::OPERAND_MEM16,	"MEM16"},
				{Operand::OPERAND_MEM32,	"MEM32"},
				{Operand::OPERAND_MEM64,	"MEM64"},
				{Operand::OPERAND_MEM128,	"MEM128"},

				{Operand::OPERAND_R_M8,		"R_M8"},
				{Operand::OPERAND_R_M16,	"R_M16"},
				{Operand::OPERAND_R_M32,	"R_M32"},
				{Operand::OPERAND_R_M64,	"R_M64"},
				{Operand::OPERAND_R_M128,	"R_M128"},

				{Operand::OPERAND_XMM32,	"XMM32"},
				{Operand::OPERAND_XMM64,	"XMM64"}
			};

			for(int i = 0; i < sizeof(subtypeTable) / sizeof(Operand::Register); i++)
			for(int j = 0; j < sizeof(subtypeTable) / sizeof(Operand::Register); j++)
			for(int k = 0; k < sizeof(subtypeTable) / sizeof(Operand::Register); k++)
			{
				if(Operand::isSubtypeOf(subtypeTable[i].type, t1))
				if(Operand::isSubtypeOf(subtypeTable[j].type, t2))
				if(Operand::isSubtypeOf(subtypeTable[k].type, t3))
				{
					int u = 0;
				
					for(u = 0; u < n; u++)
					{
						// Must have unique signature
						if(instruction->getMnemonic() == uniqueSignature[u].mnemonic)
						if(subtypeTable[i].notation == uniqueSignature[u].firstOperand)
						if(subtypeTable[j].notation == uniqueSignature[u].secondOperand)
						if(subtypeTable[k].notation == uniqueSignature[u].thirdOperand)
						{
							break;
						}

						// Don't output specialized instructions
						if(instruction->getMnemonic() == uniqueSignature[u].mnemonic)
						if(Operand::isSubtypeOf(subtypeTable[i].type, uniqueSignature[u].firstType))
						if(Operand::isSubtypeOf(subtypeTable[j].type, uniqueSignature[u].secondType))
						if(Operand::isSubtypeOf(subtypeTable[k].type, uniqueSignature[u].thirdType))
						{
							break;
						}
					}

					if(u < n)
					{
						continue;
					}

					fprintf(file, "enc %s(", mnemonic);
					if(subtypeTable[i].notation) fprintf(file, "%s a", subtypeTable[i].notation);
					if(subtypeTable[j].notation) fprintf(file, ",%s b", subtypeTable[j].notation);
					if(subtypeTable[k].notation) fprintf(file, ",%s c", subtypeTable[k].notation);
					fprintf(file, "){return x86(%d", t);
					if(subtypeTable[i].notation)
					{
						fprintf(file, ",");
						if(Operand::isSubtypeOf(subtypeTable[i].type, Operand::OPERAND_IMM) &&
						   subtypeTable[i].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(IMM)");
						}
						else if(Operand::isSubtypeOf(subtypeTable[i].type, Operand::OPERAND_STR) &&
						        subtypeTable[i].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(STR)");
						}
						fprintf(file, "a");
					}
					if(subtypeTable[j].notation)
					{
						fprintf(file, ",");
						if(Operand::isSubtypeOf(subtypeTable[j].type, Operand::OPERAND_IMM) &&
							subtypeTable[j].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(IMM)");
						}
						else if(Operand::isSubtypeOf(subtypeTable[j].type, Operand::OPERAND_STR) &&
						        subtypeTable[j].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(STR)");
						}
						fprintf(file, "b");
					}
					if(subtypeTable[k].notation)
					{
						fprintf(file, ",");
						if(Operand::isSubtypeOf(subtypeTable[k].type, Operand::OPERAND_IMM) &&
						   subtypeTable[k].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(IMM)");
						}
						else if(Operand::isSubtypeOf(subtypeTable[k].type, Operand::OPERAND_STR) &&
						        subtypeTable[k].type != Operand::OPERAND_REF)
						{
							fprintf(file, "(STR)");
						}
						fprintf(file, "c");
					}
					fprintf(file, ");}\n");

					printf("%s\n", instruction->getMnemonic());

					uniqueSignature[n].mnemonic = instruction->getMnemonic();
					uniqueSignature[n].firstOperand = subtypeTable[i].notation;
					uniqueSignature[n].secondOperand = subtypeTable[j].notation;
					uniqueSignature[n].thirdOperand = subtypeTable[k].notation;
					uniqueSignature[n].firstType = subtypeTable[i].type;
					uniqueSignature[n].secondType = subtypeTable[j].type;
					uniqueSignature[n].thirdType = subtypeTable[k].type;
					n++;
				}
			}
		}

		fprintf(file, "\n#endif   // SOFTWIRE_NO_INTRINSICS\n");

		delete[] uniqueSignature;
		fclose(file);
	}
}
