#ifndef SoftWire_Error_hpp
#define SoftWire_Error_hpp

namespace SoftWire
{
	class Error
	{
	public:
		Error(const char *format, ...);

		const char *getString() const;

	private:
		char string[256];
	};

	#define INTERNAL_ERROR Error("Internal error in %s (%d)", __FILE__, __LINE__);
}

#endif   // SoftWire_Error_hpp
