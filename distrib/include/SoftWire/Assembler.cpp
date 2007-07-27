#include "Assembler.hpp"

#include "Token.hpp"
#include "Scanner.hpp"
#include "Parser.hpp"
#include "Linker.hpp"
#include "Loader.hpp"
#include "Error.hpp"
#include "String.hpp"
#include "Operand.hpp"
#include "Synthesizer.hpp"
#include "InstructionSet.hpp"

#include <time.h>

namespace SoftWire
{
	const OperandAL Assembler::al;
	const OperandCL Assembler::cl;
	const OperandREG8 Assembler::dl(Encoding::DL);
	const OperandREG8 Assembler::bl(Encoding::BL);
	const OperandREG8 Assembler::ah(Encoding::AH);
	const OperandREG8 Assembler::ch(Encoding::CH);
	const OperandREG8 Assembler::dh(Encoding::DH);
	const OperandREG8 Assembler::bh(Encoding::BH);

	const OperandAX Assembler::ax;
	const OperandCX Assembler::cx;
	const OperandDX Assembler::dx;
	const OperandREG16 Assembler::bx(Encoding::BX);
	const OperandREG16 Assembler::sp(Encoding::SP);
	const OperandREG16 Assembler::bp(Encoding::BP);
	const OperandREG16 Assembler::si(Encoding::SI);
	const OperandREG16 Assembler::di(Encoding::DI);

	const OperandEAX Assembler::eax;
	const OperandECX Assembler::ecx;
	const OperandREG32 Assembler::edx(Encoding::EDX);
	const OperandREG32 Assembler::ebx(Encoding::EBX);
	const OperandREG32 Assembler::esp(Encoding::ESP);
	const OperandREG32 Assembler::ebp(Encoding::EBP);
	const OperandREG32 Assembler::esi(Encoding::ESI);
	const OperandREG32 Assembler::edi(Encoding::EDI);

	const OperandST0 Assembler::st;
	const OperandST0 Assembler::st0;
	const OperandFPUREG Assembler::st1(Encoding::ST1);
	const OperandFPUREG Assembler::st2(Encoding::ST2);
	const OperandFPUREG Assembler::st3(Encoding::ST3);
	const OperandFPUREG Assembler::st4(Encoding::ST4);
	const OperandFPUREG Assembler::st5(Encoding::ST5);
	const OperandFPUREG Assembler::st6(Encoding::ST6);
	const OperandFPUREG Assembler::st7(Encoding::ST7);

	const OperandMMREG Assembler::mm0(Encoding::MM0);
	const OperandMMREG Assembler::mm1(Encoding::MM1);
	const OperandMMREG Assembler::mm2(Encoding::MM2);
	const OperandMMREG Assembler::mm3(Encoding::MM3);
	const OperandMMREG Assembler::mm4(Encoding::MM4);
	const OperandMMREG Assembler::mm5(Encoding::MM5);
	const OperandMMREG Assembler::mm6(Encoding::MM6);
	const OperandMMREG Assembler::mm7(Encoding::MM7);

	const OperandXMMREG Assembler::xmm0(Encoding::XMM0);
	const OperandXMMREG Assembler::xmm1(Encoding::XMM1);
	const OperandXMMREG Assembler::xmm2(Encoding::XMM2);
	const OperandXMMREG Assembler::xmm3(Encoding::XMM3);
	const OperandXMMREG Assembler::xmm4(Encoding::XMM4);
	const OperandXMMREG Assembler::xmm5(Encoding::XMM5);
	const OperandXMMREG Assembler::xmm6(Encoding::XMM6);
	const OperandXMMREG Assembler::xmm7(Encoding::XMM7);

	const OperandMEM8 Assembler::byte_ptr;
	const OperandMEM16 Assembler::word_ptr;
	const OperandMEM32 Assembler::dword_ptr;
	const OperandMEM64 Assembler::mmword_ptr;
	const OperandMEM64 Assembler::qword_ptr;
	const OperandMEM128 Assembler::xmmword_ptr;
	const OperandMEM128 Assembler::xword_ptr;

	InstructionSet *Assembler::instructionSet = 0;
	int Assembler::referenceCount = 0;
	bool Assembler::listingEnabled = true;

	Assembler::Assembler(const char *sourceFile)
	{
		try
		{
			errors = new char[1];
			errors[0] = '\0';
			echoFile = 0;

			if(sourceFile)
			{
				entryLabel = strdup(sourceFile);
				char *dot = strrchr(entryLabel, '.');

				if(dot)
				{
					*dot = '\0';
				}
				else
				{
					throw Error("Input file must have extension");
				}
			}
			else
			{
				entryLabel = 0;
			}

			if(!instructionSet)
			{
				instructionSet = new InstructionSet();
			}
			referenceCount++;

			linker = new Linker();
			loader = new Loader(*linker);
			synthesizer = new Synthesizer();

			if(sourceFile)
			{
				scanner = new Scanner();
				parser = new Parser(*scanner, *synthesizer, *instructionSet);

				scanner->scanFile(sourceFile);
				assembleFile();
			}
			else
			{
				scanner = 0;
				parser = 0;
			}
		}
		catch(const Error &error)
		{
			handleError(error.getString());
		}
	}

	Assembler::Assembler(const char *sourceString, const char *entryPoint)
	{
		try
		{
			errors = new char[1];
			errors[0] = '\0';
			echoFile = 0;

			if (entryPoint)
			{
				entryLabel = strdup(entryPoint);
			}
			else
			{
				entryLabel = 0;
			}

			if(!instructionSet)
			{
				instructionSet = new InstructionSet();
			}
			referenceCount++;

			linker = new Linker();
			loader = new Loader(*linker);
			synthesizer = new Synthesizer();

			scanner = new Scanner();
			parser = new Parser(*scanner, *synthesizer, *instructionSet);

			scanner->scanString(sourceString);
			assembleFile();
		}
		catch(const Error &error)
		{
			handleError(error.getString());
		}
	}

	Assembler::~Assembler()
	{
		delete[] entryLabel;
		entryLabel = 0;

		delete linker;
		linker = 0;

		delete loader;
		loader = 0;

		delete scanner;
		scanner = 0;

		delete synthesizer;
		synthesizer = 0;

		referenceCount--;
		if(!referenceCount)
		{
			delete instructionSet;
			instructionSet = 0;
		}

		delete parser;
		parser = 0;

		delete[] errors;
		errors = 0;

		delete[] echoFile;
		echoFile = 0;
	}

	void Assembler::assembleFile()
	{
		if(!scanner)
		{
			throw INTERNAL_ERROR;
		}

		scanner->rewind();

		while(!scanner->isEndOfFile())
		{
			assembleLine();

			if(scanner->isEndOfLine() && !scanner->isEndOfFile())
			{
				scanner->advance();
			}
		}

		delete scanner;
		scanner = 0;

		delete parser;
		parser = 0;
	}

	void Assembler::defineExternal(void *pointer, const char *name)
	{
		Linker::defineExternal(pointer, name);
	}

	void Assembler::defineSymbol(int value, const char *name)
	{
		Scanner::defineSymbol(value, name);
	}

	void (*Assembler::callable(const char *entryLabel))()
	{
		if(!loader || errors[0] != '\0') return 0;

		if(entryLabel)
		{
			return loader->callable(entryLabel);
		}
		else
		{
			return loader->callable(this->entryLabel);
		}
	}

	void (*Assembler::finalize(const char *entryLabel))()
	{
		if(!loader) throw Error("Assembler could not be finalized (cannot re-finalize)");

		delete linker;
		linker = 0;

		delete scanner;
		scanner = 0;

		delete synthesizer;
		synthesizer = 0;

		delete parser;
		parser = 0;

		delete[] errors;
		errors = 0;

		delete[] echoFile;
		echoFile = 0;

		if(entryLabel)
		{
			delete[] this->entryLabel;
			this->entryLabel = 0;

			return loader->callable(entryLabel);
		}
		else
		{
			return loader->callable(this->entryLabel);
		}
	}

	void *Assembler::acquire()
	{
		if(!loader) return 0;

		return loader->acquire();
	}

	void Assembler::assembleLine()
	{
		if(!parser || !loader) throw INTERNAL_ERROR;

		const Encoding &encoding = parser->parseLine();
		loader->appendEncoding(encoding);
	}

	const char *Assembler::getErrors() const
	{
		return errors;
	}

	void Assembler::handleError(const char *error)
	{
		const char *sourceLine = "<intrinsic>";
		if(parser) sourceLine = parser->skipLine();

		int previousLength = 0;
		if(errors) previousLength = strlen(errors);
		int newLength = previousLength + strlen(error) + strlen(sourceLine) + 16;

		delete[] errors;
		errors = new char[newLength];

		strcpy(errors, "error: ");
		strcat(errors, error);
		strcat(errors, "\n\t");
		strcat(errors, sourceLine);
		strcat(errors, "\n");
	}

	const char *Assembler::getListing() const
	{
		return loader->getListing();
	}

	void Assembler::clearListing() const
	{
		loader->clearListing();
	}

	void Assembler::setEchoFile(const char *echoFile, const char *mode)
	{
		if(!listingEnabled) return;

		if(this->echoFile)
		{
			delete[] this->echoFile;
			this->echoFile = 0;
		}

		if(echoFile)
		{
			this->echoFile = strdup(echoFile);

			FILE *file = fopen(echoFile, mode);
			const int t = time(0);
			fprintf(file, "\n;%s\n", ctime((long*)&t));
			fclose(file);
		}
	}

	void Assembler::annotate(const char *format, ...)
	{
		if(!echoFile) return;

		char buffer[256];
		va_list argList;

		va_start(argList, format);
		vsnprintf(buffer, 256, format, argList);
		va_end(argList);

		FILE *file = fopen(echoFile, "at");
		fprintf(file, "; ");
		fprintf(file, buffer);
		fprintf(file, "\n");
		fclose(file);
	}

	void Assembler::enableListing()
	{
		listingEnabled = true;
	}

	void Assembler::disableListing()
	{
		listingEnabled = false;
	}

	Encoding *Assembler::x86(int instructionID, const Operand &firstOperand, const Operand &secondOperand, const Operand &thirdOperand)
	{
		try
		{
			if(!loader || !synthesizer || !instructionSet) throw INTERNAL_ERROR;

			const Instruction *instruction = instructionSet->instruction(instructionID);

			if(echoFile)
			{
				FILE *file = fopen(echoFile, "at");

				fprintf(file, "\t%s", instruction->getMnemonic());
				if(!Operand::isVoid(firstOperand)) fprintf(file, "\t%s", firstOperand.string());
				if(!Operand::isVoid(secondOperand)) fprintf(file, ",\t%s", secondOperand.string());
				if(!Operand::isVoid(thirdOperand)) fprintf(file, ",\t%s", thirdOperand.string());
				fprintf(file, "\n");

				fclose(file);
			}

			synthesizer->reset();

			synthesizer->encodeFirstOperand(firstOperand);
			synthesizer->encodeSecondOperand(secondOperand);
			synthesizer->encodeThirdOperand(thirdOperand);
			const Encoding &encoding = synthesizer->encodeInstruction(instruction);

			return loader->appendEncoding(encoding);
		}
		catch(Error &error)
		{
			handleError(error.getString());
		}

		return 0;
	}

	void Assembler::label(const char *label)
	{
		try
		{
			if(!loader || !synthesizer) return;

			if(echoFile)
			{
				FILE *file = fopen(echoFile, "at");
				fprintf(file, "%s:\n", label);
				fclose(file);
			}

			synthesizer->reset();

			synthesizer->defineLabel(label);
			const Encoding &encoding = synthesizer->encodeInstruction(0);

			loader->appendEncoding(encoding);
		}
		catch(Error &error)
		{
			handleError(error.getString());
		}
	}
};
