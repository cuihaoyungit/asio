#ifndef __Singleton_H__
#define __Singleton_H__
#include <mutex>

//------------------------------------------------------------------
// dynamic create object
/*
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

*/
//----------------------------------------------------------------------
//Safe thread dynamic create object
template <typename T>
class SafeSingleton {
public:
	static T* getInstance() {
		if (!m_instance) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (!m_instance) {
				m_instance = new T();
			}
		}
		return m_instance;
	}

	static void Delete()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_instance)
		{
			delete m_instance;
			m_instance = nullptr;
		}
	}

	SafeSingleton(const SafeSingleton&) = delete;
	SafeSingleton& operator=(const SafeSingleton&) = delete;

protected:
	SafeSingleton() {}
	~SafeSingleton() {}

private:
	static T* m_instance;
	static std::mutex m_mutex;
};

template <typename T>
T* SafeSingleton<T>::m_instance = nullptr;

template <typename T>
std::mutex SafeSingleton<T>::m_mutex;

//----------------------------------------------------------------------
// Static create object
template <typename T>
class SSingleton
{
protected:
	SSingleton() {};
	~SSingleton() {};
	// Move constructor
	SSingleton(SSingleton&&) = delete;
	// Move assignment
	SSingleton& operator =(SSingleton&&) = delete;
private:
	// Copy constructor
	SSingleton(const SSingleton&) = delete;
	// Copy assignment
	const SSingleton& operator =(const SSingleton&) = delete;
public:
	static T& Instance() {
		static T instance;
		return instance;
	}
};

//-------------------------------------------------------------------------
// Dynamic create object
template <typename T>
class DSingleton {
public:
	static T* getInstance() {
		return m_instance;
	}

	static void Delete()
	{
		if (m_instance)
		{
			delete m_instance;
			m_instance = nullptr;
		}
	}

	static T* Create() {
		if (!m_instance) {
			m_instance = new T();
		}
		return m_instance;
	}

	// create with args
	template <typename... Args>
	static T* CreateArgs(Args &&... args)
	{
		if (!m_instance) {
			m_instance = new T(std::forward<Args>(args)...);
		}
		return m_instance;
	}

	DSingleton(const DSingleton&) = delete;
	DSingleton& operator=(const DSingleton&) = delete;

protected:
	DSingleton() {}
	virtual ~DSingleton() {}

private:
	static T* m_instance;
};

template <typename T>
T* DSingleton<T>::m_instance = nullptr;

//----------------------------------------------------------------------
//Save pointer of object
template <typename T> 
class PSingleton
{
protected:
	// TODO: Come up with something better than this!
	// TODO:
	// TODO: This super-nasty piece of nastiness was put in for continued
	// TODO: compatability with MSVC++ and MinGW - the latter apparently
	// TODO: needs this.
	static T* ms_Singleton = nullptr;
public:
	PSingleton(void)
	{
		assert(!ms_Singleton);
		ms_Singleton = static_cast<T*>(this);
	}
	~PSingleton(void)
	{
		assert(ms_Singleton);  
		ms_Singleton = nullptr;
	}
	static T& getSingleton(void)
	{
		assert(ms_Singleton);  
		return (*ms_Singleton);
	}
	static T* getSingletonPtr(void)
	{
		return (ms_Singleton);
	}

private:
	PSingleton& operator=(const PSingleton&) { return this; }
	PSingleton(const PSingleton&) {}
};

//----------------------------------------------------------------------


#endif // __Singleton_H__
