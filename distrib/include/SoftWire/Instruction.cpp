#include "Instruction.hpp"

#include "Error.hpp"
#include "String.hpp"

#include <stdio.h>
#include <stdlib.h>

namespace SoftWire
{
	Instruction::Instruction(const Syntax &instruction) : syntax(instruction)
	{
		extractOperands(instruction.operands);

		if(secondOperand == Operand::OPERAND_IMM8)
		{
			if(Operand::isSubtypeOf(firstOperand, Operand::OPERAND_R_M16) ||
			   Operand::isSubtypeOf(firstOperand, Operand::OPERAND_R_M32))
			{
				secondOperand = Operand::OPERAND_EXT8;
			}
		}

		syntaxMatch = false;
		flags = instruction.flags;
		next = 0;
	}

	Instruction::~Instruction()
	{
		// Avoid call stack overflow
		while(next)
		{
			Instruction *nextNext = next->next;
			next->next = 0;
			delete next;
			next = nextNext;
		}
		next = 0;
	}

	void Instruction::extractOperands(const char *syntax)
	{
		if(!syntax)
		{
			throw INTERNAL_ERROR;
		}

		specifier = Specifier::TYPE_UNKNOWN;
		firstOperand = Operand::OPERAND_VOID;
		secondOperand = Operand::OPERAND_VOID;
		thirdOperand = Operand::OPERAND_VOID;

		char *string = strdup(syntax);
		const char *token = strtok(string, " ,");

		if(!token)
		{
			delete[] string;
			return;
		}

		specifier = Specifier::scan(token);

		if(specifier != Specifier::TYPE_UNKNOWN)
		{
			token = strtok(0, " ,");

			if(!token)
			{
				delete[] string;
				return;
			}
		}

		firstOperand = Operand::scanSyntax(token);

		if(firstOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if(token == 0)
			{
				delete[] string;
				return;
			}
		}

		secondOperand = Operand::scanSyntax(token);

		if(secondOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if(token == 0)
			{
				delete[] string;
				return;
			}
		}

		thirdOperand = Operand::scanSyntax(token);

		if(thirdOperand != Operand::OPERAND_UNKNOWN)
		{
			token = strtok(0, " ,");

			if(token == 0)
			{
				delete[] string;
				return;
			}
		}

		delete[] string;

		if(token == 0)
		{
			return;
		}
		else
		{
			throw Error("Invalid operand encoding '%s'", syntax);
		}
	}

	void Instruction::attach(Instruction *instruction)
	{
		if(!next)
		{
			next = instruction;
		}
		else
		{
			next->attach(instruction);
		}
	}

	void Instruction::attachNew(const Syntax &instruction)
	{
		if(!next)
		{
			next = new Instruction(instruction);
		}
		else
		{
			next->attachNew(instruction);
		}
	}

	Instruction *Instruction::getNext() const
	{
		return next;
	}

	bool Instruction::matchSyntax() const
	{
		return syntaxMatch;	
	}

	void Instruction::resetMatch()
	{
		syntaxMatch = true;

		if(next)
		{
			next->resetMatch();
		}
	}

	void Instruction::matchMnemonic(const char *mnemonic)
	{
		if(next)
		{
			next->matchMnemonic(mnemonic);
		}

		if(!syntaxMatch) return;

		if(stricmp(syntax.mnemonic, mnemonic) == 0)
		{
			syntaxMatch = true;
		}
		else
		{
			syntaxMatch = false;
		}
	}

	void Instruction::matchSpecifier(Specifier::Type specifier)
	{
		if(next)
		{
			next->matchSpecifier(specifier);
		}

		if(!syntaxMatch) return;

		if(this->specifier == Specifier::TYPE_UNKNOWN)
		{
			if(specifier != Specifier::TYPE_UNKNOWN)
			{
				if(firstOperand == Operand::OPERAND_R_M8 || secondOperand == Operand::OPERAND_R_M8)
				{
					syntaxMatch = specifier == Specifier::TYPE_BYTE;
				}
				else if(firstOperand == Operand::OPERAND_R_M16 || secondOperand == Operand::OPERAND_R_M16)
				{
					syntaxMatch = specifier == Specifier::TYPE_WORD;
				}
				else if(firstOperand == Operand::OPERAND_R_M32 || secondOperand == Operand::OPERAND_R_M32)
				{
					syntaxMatch = specifier == Specifier::TYPE_DWORD;
				}
				else if(firstOperand == Operand::OPERAND_R_M64 || secondOperand == Operand::OPERAND_R_M64)
				{
					syntaxMatch = (specifier == Specifier::TYPE_QWORD || specifier == Specifier::TYPE_MMWORD);
				}
				else if(firstOperand == Operand::OPERAND_R_M128 || secondOperand == Operand::OPERAND_R_M128)
				{
					syntaxMatch = specifier == Specifier::TYPE_XMMWORD;
				}
				else
				{
					syntaxMatch = true;
				}
			}
			else
			{
				syntaxMatch = true;
			}
		}
		else if(this->specifier != Specifier::TYPE_UNKNOWN)   // Explicit specifier
		{
			if(this->specifier == specifier)
			{
				syntaxMatch = true;
			}
			else if(specifier == Specifier::TYPE_UNKNOWN)   // Specifiers are optional
			{
				syntaxMatch = true;
			}
			else
			{
				syntaxMatch = false;
			}
		}
		else
		{
			syntaxMatch = false;
		}
	}

	void Instruction::matchFirstOperand(const Operand &operand)
	{
		if(next)
		{
			next->matchFirstOperand(operand);
		}

		if(!syntaxMatch) return;

		if(operand.isSubtypeOf(firstOperand))
		{
			syntaxMatch = true;
		}
		else if(operand.type == Operand::OPERAND_MEM && firstOperand & Operand::OPERAND_MEM)
		{
			if(syntaxMatch)   // Explicit size specfier
			{
				syntaxMatch = true;
			}
			else if(secondOperand != Operand::OPERAND_UNKNOWN)   // Implicit size specifier
			{
				syntaxMatch = true;
			}
			else
			{
				syntaxMatch = false;
			}
		}
		else
		{
			syntaxMatch = false;
		}
	}

	void Instruction::matchSecondOperand(const Operand &operand)
	{
		if(next)
		{
			next->matchSecondOperand(operand);
		}

		if(!syntaxMatch) return;

		if(operand.isSubtypeOf(secondOperand))
		{
			syntaxMatch = true;
		}
		else if(operand.type == Operand::OPERAND_MEM && secondOperand & Operand::OPERAND_MEM)
		{
			if(syntaxMatch)   // Explicit size specfier
			{
				syntaxMatch = true;
			}
			else if(firstOperand != Operand::OPERAND_UNKNOWN)   // Implicit size specifier
			{
				syntaxMatch = true;
			}
			else
			{
				syntaxMatch = false;
			}
		}
		else
		{
			syntaxMatch = false;
		}
	}

	void Instruction::matchThirdOperand(const Operand &operand)
	{
		if(next)
		{
			next->matchThirdOperand(operand);
		}

		if(!syntaxMatch) return;

		if(operand.isSubtypeOf(thirdOperand))
		{
			syntaxMatch = true;
		}
		else
		{
			syntaxMatch = false;
		}
	}

	const char *Instruction::getMnemonic() const
	{
		return syntax.mnemonic;
	}

	Operand::Type Instruction::getFirstOperand() const
	{
		return firstOperand;
	}

	Operand::Type Instruction::getSecondOperand() const
	{
		return secondOperand;
	}

	Operand::Type Instruction::getThirdOperand() const
	{
		return thirdOperand;
	}

	const char *Instruction::getOperandSyntax() const
	{
		return syntax.operands;
	}

	const char *Instruction::getEncoding() const
	{
		return syntax.encoding;
	}

	bool Instruction::is32Bit() const
	{
		return (flags & CPU_386) == CPU_386;
	}

	int Instruction::approximateSize() const
	{
		const char *format = syntax.encoding;

		if(!format)
		{
			throw INTERNAL_ERROR;
		}

		int size = 0;

		while(*format)
		{
			switch((format[0] << 8) | format[1])
			{
			case LOCK_PRE:
			case CONST_PRE:
			case REPNE_PRE:
			case REP_PRE:
				size += 1;
				break;
			case OFF_PRE:
				if(!is32Bit())
				{
					size += 1;
				}
				break;
			case ADDR_PRE:
				if(!is32Bit())
				{
					size += 1;
				}
				break;
			case ADD_REG:
				break;
			case EFF_ADDR:
			case MOD_RM_0:
			case MOD_RM_1:
			case MOD_RM_2:
			case MOD_RM_3:
			case MOD_RM_4:
			case MOD_RM_5:
			case MOD_RM_6:
			case MOD_RM_7:
				size += 1;
				break;
			case DWORD_IMM:
			case DWORD_REL:
				size += 4;
				break;
			case WORD_IMM:
				size += 2;
				break;
			case BYTE_IMM:
			case BYTE_REL:
				size += 1;
				break;
			default:
				size += 1;
			}

			format += 2;

			if(*format == ' ')	
			{
				format++;
			}
			else if(*format == '\0')
			{
				break;
			}
			else
			{
				throw INTERNAL_ERROR;
			}
		}

		return size;
	}
}
