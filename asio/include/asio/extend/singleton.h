#ifndef __Singleton_H__
#define __Singleton_H__
#include <mutex>

//------------------------------------------------------------------
// dynamic create object
template<class T>
class DSingleton
{
private:
	DSingleton() {};
	~DSingleton() {};
private:
	DSingleton(const DSingleton&) = delete;
	DSingleton& operator = (const DSingleton&) = delete;
	static T* instance;
	static std::mutex mtx;
public:
	inline static T* Instance()
	{
		if (!instance)
		{
			Create();
		}
		return instance;
	}

	inline static void Delete()
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (instance)
		{
			delete instance;
			instance = nullptr;
		}
	}
private:
	inline static void Create()
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (!instance)
		{
			instance = new T;
		}
	}
};
template<class T>
T* DSingleton<T>::instance = nullptr;
template<class T>
std::mutex DSingleton<T>::mtx;

//----------------------------------------------------------------------
// static create object
template <typename T>
class SSingleton
{
public:
	static T& Instance() {
		static T instance;
		return instance;
	}
	
protected:
	// Move constructor
	SSingleton(SSingleton&&) = delete;
	// Move assignment
	SSingleton& operator =(SSingleton&&) = delete;
private:
	// Copy constructor
	SSingleton(const SSingleton&) = delete;
	// Copy assignment
	const SSingleton& operator =(const SSingleton&) = delete;
	SSingleton() {};
	~SSingleton() {};
};



//----------------------------------------------------------------------
template <typename T> class Singleton
{
protected:
	// TODO: Come up with something better than this!
	// TODO:
	// TODO: This super-nasty piece of nastiness was put in for continued
	// TODO: compatability with MSVC++ and MinGW - the latter apparently
	// TODO: needs this.
	static T* ms_Singleton;
public:
	Singleton(void)
	{
		assert(!ms_Singleton);
		ms_Singleton = static_cast<T*>(this);
	}
	~Singleton(void)
	{
		assert(ms_Singleton);  ms_Singleton = 0;
	}
	static T& getSingleton(void)
	{
		assert(ms_Singleton);  return (*ms_Singleton);
	}
	static T* getSingletonPtr(void)
	{
		return (ms_Singleton);
	}

private:
	Singleton& operator=(const Singleton&) { return this; }
	Singleton(const Singleton&) {}
};

//----------------------------------------------------------------------


#endif // __Singleton_H__
