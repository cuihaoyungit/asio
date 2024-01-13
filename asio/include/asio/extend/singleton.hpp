#ifndef __Singleton_H__
#define __Singleton_H__
#include <mutex>

//------------------------------------------------------------------
// dynamic create object
// reference from da san shi company code
template<class T>
class DSSSingleton
{
private:
	DSSSingleton() {};
	virtual ~DSSSingleton() {};
private:
	DSSSingleton(DSSSingleton&&) = delete;
	DSSSingleton(const DSSSingleton&) = delete;
	DSSSingleton& operator = (const DSSSingleton&) = delete;
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

	T& operator()() const { return *Instance(); }
	T* operator->() const { return Instance(); }
private:
	inline static void Create()
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (!instance)
		{
			instance = new(std::nothrow) T;
		}
	}
};
template<class T>
T* DSSSingleton<T>::instance = nullptr;
template<class T>
std::mutex DSSSingleton<T>::mtx;


//----------------------------------------------------------------------
//Safe thread dynamic create object
template <typename T>
class SafeSingleton {
public:
	T& operator()() const { return *getInstance(); }
	T* operator->() const { return getInstance(); }
public:
	static T* getInstance() {
		if (!m_instance) {
			std::lock_guard<std::mutex> lock(m_mutex);

			if (!m_instance) {
				m_instance = new(std::nothrow) T();
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

	SafeSingleton(SafeSingleton&&) = delete;
	SafeSingleton(const SafeSingleton&) = delete;
	SafeSingleton& operator=(const SafeSingleton&) = delete;
protected:
	SafeSingleton() {}
	virtual ~SafeSingleton() {}

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
	T& operator()() const { return Instance(); }
	T* operator->() const { return &Instance(); }
};

//-------------------------------------------------------------------------
// Dynamic create object
template <typename T>
class DSingleton {
public:
	static T* getInstance() {
		return m_instance;
	}
	T& operator()() const { return *getInstance(); }
	T* operator->() const { return getInstance(); }

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
			m_instance = new(std::nothrow) T();
		}
		return m_instance;
	}

	// create with args
	template <typename... Args>
	static T* CreateArgs(Args &&... args)
	{
		if (!m_instance) {
			m_instance = new(std::nothrow) T(std::forward<Args>(args)...);
		}
		return m_instance;
	}

	DSingleton(DSingleton&&) = delete;
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
	static T* ms_Singleton;
public:
	PSingleton(void)
	{
		assert(!ms_Singleton);
		ms_Singleton = static_cast<T*>(this);
	}
	virtual ~PSingleton(void)
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

	T& operator()() const { return getSingleton(); }
	T* operator->() const { return getSingletonPtr(); }
private:
	PSingleton(PSingleton&&) = delete;
	PSingleton(const PSingleton&) = delete;
	PSingleton& operator=(const PSingleton&) { return this; }
};

template <typename T>
T* PSingleton<T>::ms_Singleton = nullptr;

//----------------------------------------------------------------------


#endif // __Singleton_H__
