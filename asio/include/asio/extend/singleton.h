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



#endif // __Singleton_H__
