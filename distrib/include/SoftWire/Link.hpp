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

		void append(const T &t);
		Link *next() const;

	private:
		Link *n;
	};
}

namespace SoftWire
{
	template<class T>
	Link<T>::Link()
	{
		n = 0;
	}

	template<class T>
	Link<T>::~Link()
	{
		delete n;
		n = 0;
	}

	template<class T>
	void Link<T>::append(const T &t)
	{
		if(n)
		{
			n->append(t);
		}
		else
		{
			*(T*)this = t;

			n = new Link();
		}
	}

	template<class T>
	Link<T> *Link<T>::next() const
	{
		return n;
	}
}

#endif   // SoftWire_Link_hpp
