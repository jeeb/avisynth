#ifndef SoftWire_Error_hpp
#define SoftWire_Error_hpp

namespace SoftWire
{
	class Error
	{
	public:
		Error(const char *format, ...);

		const char *getString() const;

		Error &operator<<(const Error &error);
		Error &operator>>(const Error &error);

	private:
		char string[256];
	};

	#ifndef NDEBUG
		#define INTERNAL_ERROR Error("%s (%d):\n\tInternal error", __FILE__, __LINE__)
		#define EXCEPTION      Error("%s (%d):\n\t", __FILE__, __LINE__) << Error
	#else
		#define INTERNAL_ERROR Error("Internal error")
		#define EXCEPTION      Error	
	#endif
}

#endif   // SoftWire_Error_hpp
