#ifndef SoftWire_File_hpp
#define SoftWire_File_hpp

#include "Error.hpp"

#include <stdio.h>
#include <io.h>

#ifdef __unix__
	#include <sys/stat.h>
#endif

namespace SoftWire
{
#ifdef __unix__
	static int _filelength(int handle)
	{
		struct stat statbuf;
		
		if(fstat(handle, &statbuf) != 0)
		{
			throw INTERNAL_ERROR;
		}

		return statbuf.st_size;
	}
#endif
}

#endif   // SoftWire_File_hpp
