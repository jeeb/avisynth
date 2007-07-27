#ifndef SoftWire_String_hpp
#define SoftWire_String_hpp

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

namespace SoftWire
{
#ifdef WIN32
	inline int vsnprintf(char *buffer, size_t count, const char *format, va_list argptr)
	{
		return _vsnprintf(buffer, count, format, argptr);
	}

	static int (*snprintf)(char *buffer, size_t count, const char *format, ...) = _snprintf;
#endif

#ifdef __GNUG__
	inline int stricmp(const char *string1, const char *string2)
	{
		return strcasecmp(string1, string2);
	}
#endif

#ifndef strlwr
	inline char *strlwr(char *string)
	{
		int n = strlen(string);

		for(int i = 0; i < n; i++)
		{
			string[i] = tolower(string[i]);
		}

		return string;
	}
#endif

	inline char *strdup(const char *string)
	{
		if(!string) return 0;
		char *duplicate = new char[strlen(string) + 1];
		return strcpy(duplicate, string);
	}
}

#endif   // SoftWire_String_hpp
