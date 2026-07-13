#ifdef _WIN64
#define POINTERTYPE unsigned long long
#else
#define POINTERTYPE unsigned int
#endif

class pointer
{
protected:
	void* Address;

public:
	inline pointer ()
	{
		this->Address = nullptr;
		return;
	}

	inline pointer (POINTERTYPE Address)
	{
		this->Address = reinterpret_cast<void*>(Address);
		return;
	}

	inline pointer (void* Address)
	{
		this->Address = Address;
		return;
	}

	inline void* operator += (unsigned int Other)
	{
		return (this->Address = reinterpret_cast<void*>(reinterpret_cast<POINTERTYPE>(this->Address) + Other));
	}

	inline operator void* ()
	{
		return this->Address;
	}

	inline operator DWORD ()
	{
		return reinterpret_cast<DWORD>(this->Address);
	}
};
