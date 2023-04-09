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
// sol2 single class
// sample singleton.cpp

template <typename T>
struct DynamicSingleton {
private:
	DynamicSingleton() {
	}

public:
	static std::shared_ptr<T> getInstance();

	// destructor must be public to work with
	// std::shared_ptr and friends
	// if you need it to be private, you must implement
	// a custom deleter with access to the private members
	// (e.g., a deleter struct defined in this class)
	virtual ~DynamicSingleton() {
	}
};

template <typename T>
std::shared_ptr<T> DynamicSingleton::getInstance() {
	static std::weak_ptr<T> instance;
	static std::mutex m;

	m.lock();
	auto ret = instance.lock();
	if (!ret) {
		ret.reset(new T());
		instance = ret;
	}
	m.unlock();

	return ret;
}



#endif // __Singleton_H__
