#ifndef SoftWire_Loader_hpp
#define SoftWire_Loader_hpp

#include "Link.hpp"

namespace SoftWire
{
	class Linker;
	class Encoding;

	class Loader
	{
	public:
		Loader(const Linker &linker);

		~Loader();

		void (*callable(const char *entryLabel = 0))();
		void (*finalize(const char *entryLabel = 0))();
		void *acquire();

		void appendEncoding(const Encoding &encoding);

		const char *getListing();
		void clearListing();

	private:
		const Linker &linker;

		typedef Link<Encoding> Instruction;
		Instruction *instructions;
		unsigned char *machineCode;
		char *listing;

		bool possession;
		bool finalized;

		void loadCode(const char *entryLabel = 0);
		const unsigned char *resolveReference(const char *name) const;
		const unsigned char *resolveLocal(const char *name) const;
		const unsigned char *resolveExternal(const char *name) const;
		int codeLength() const;
	};
}

#endif   // SoftWire_Loader_hpp
