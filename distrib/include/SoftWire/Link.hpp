#ifndef SoftWire_Link_hpp
#define SoftWire_Link_hpp

namespace SoftWire
{
	template<class T>
	class Link : public T
	{
	public:
		Link();
		
		~Link();

		Link *append(const T &t);
		Link *next() const;

	private:
		Link *n;   // Next
		Link *t;   // Tail
	};
}

namespace SoftWire
{
	template<class T>
	Link<T>::Link()
	{
		n = 0;
		t = 0;
	}

	template<class T>
	Link<T>::~Link()
	{
		delete n;
		n = 0;
	}

	template<class T>
	Link<T> *Link<T>::append(const T &e)
	{
		if(t)
		{
			t = t->append(e);
		}
		else
		{
			*(T*)this = e;

			t = n = new Link();
		}

		return t;
	}

	template<class T>
	Link<T> *Link<T>::next() const
	{
		return n;
	}
}

#endif   // SoftWire_Link_hpp
