#ifndef SoftWire_Loader_hpp
#define SoftWire_Loader_hpp

#include "Link.hpp"

#pragma warning( push )
#pragma warning (disable: 4512) // assignment operator could not be generated

namespace SoftWire
{
	class Linker;
	class Encoding;

	class Loader
	{
	public:
		Loader(const Linker &linker, bool x64 = false); // Default to true on X64 platform

		virtual ~Loader();

		void (*callable(const char *entryLabel = 0))();
		void (*finalize(const char *entryLabel = 0))();
		void *acquire();

		Encoding *appendEncoding(const Encoding &encoding);

		const char *getListing();
		void clearListing();
		void reset();
		int instructionCount();

	private:
		const Linker &linker;

		typedef Link<Encoding> Instruction;
		Instruction *instructions;
		unsigned char *machineCode;
		char *listing;

		const bool x64;   // Long mode
		bool possession;
		bool finalized;

		void loadCode(const char *entryLabel = 0);
		const unsigned char *resolveReference(const char *name, const Instruction *position) const;
		const unsigned char *resolveLocal(const char *name, const Instruction *position) const;
		const unsigned char *resolveExternal(const char *name) const;
		int codeLength() const;
	};
}

#pragma warning( pop )

#endif   // SoftWire_Loader_hpp
