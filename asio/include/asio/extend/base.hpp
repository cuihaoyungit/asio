#ifndef __BASE_HPP__
#define __BASE_HPP__
#include <cstdint>
#include <iostream>

/*************************************************************************
	Simplification of some 'unsigned' types
*************************************************************************/

typedef	unsigned long		ulong;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned char		uchar;

typedef long long			int64;
typedef int					int32;
typedef short				int16;
typedef char				int8;

typedef unsigned long long  uint64;
typedef unsigned int        uint32;
typedef unsigned short      uint16;
typedef unsigned char       uint8;

typedef float               float32;
typedef double              float64;

//Typedefs
#ifdef _UNICODE
using String = std::wstring;
#else
using String = std::string;
#endif //#ifdef _UNICODE

// stl basic type
#if 0
typedef std::int64_t        int64;
typedef std::uint64_t       uint64;	
typedef std::int32_t        int32;
typedef std::uint32_t       uint32;
typedef std::int16_t        int16;
typedef std::uint16_t       uint16;
typedef std::int8_t         int8;
typedef std::uint8_t        uint8;
#endif


// win32 basic types
#if 0
typedef unsigned long       DWORD;
typedef unsigned char       BYTE;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef double	            DOUBLE;
typedef int                 INT;
typedef unsigned int        UINT;
typedef long long			INT64;
typedef unsigned long long  UINT64;
#endif

/*************************************************************************
	Dynamic Library import / export control conditional
	(Define CEGUIBASE_EXPORTS to export symbols, else they are imported)
*************************************************************************/
#if (defined(_MSC_VER) || defined( __WIN32__ ) || defined( _WIN32 ) || defined(_WINDOWS) || defined(_WIN64) || defined( __WIN64__ )) && !defined(ASIO_STATIC)
#   ifdef ASEXPORTS
#       define EXPORT __declspec(dllexport)
#   else
#       define EXPORT __declspec(dllimport)
#   endif
#elif defined(__GNUC__)
#       define EXPORT __attribute__((visibility("default")))
#else
#		define EXPORT
#endif

#if defined(__cplusplus)

#ifndef EXTERN_C
#define EXTERN_C            extern "C"
#endif

#ifndef BEGIN_EXTERN_C
#define BEGIN_EXTERN_C      extern "C" {
#endif

#ifndef END_EXTERN_C
#define END_EXTERN_C        } // extern "C"
#endif

#else

#define EXTERN_C			extern
#define BEGIN_EXTERN_C
#define END_EXTERN_C

#endif

// Interface
#ifndef Interface
#define MyInterface class
#endif // !Interface

// SharedPoint
#define SHARD(obj) std::shared_ptr<obj>


/// @name namespace cocos2d
/// @{
#ifdef __cplusplus
#define NS_CC_BEGIN                     namespace cocos2d {
#define NS_CC_END                       }
#define USING_NS_CC                     using namespace cocos2d
#define NS_CC                           ::cocos2d
#else
#define NS_CC_BEGIN 
#define NS_CC_END 
#define USING_NS_CC 
#define NS_CC
#endif 
//  end of namespace group
/// @}


#define CC_SAFE_DELETE(p)           do { delete (p); (p) = nullptr; } while(0)
#define CC_SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)
#define CC_SAFE_FREE(p)             do { if(p) { free(p); (p) = nullptr; } } while(0)
#define CC_SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define CC_SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define CC_SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)
#define CC_BREAK_IF(cond)           if(cond) break


#ifndef ASIO_TRY
#   define ASIO_TRY try
#endif
#ifndef ASIO_CATCH
#   define ASIO_CATCH(e) catch (e)
#endif
#ifndef ASIO_THROW
#   define ASIO_THROW(e) throw e
#endif
#ifndef ASIO_RETHROW
#   define ASIO_RETHROW throw
#endif


#if defined(__GNUC__) && ((__GNUC__ >= 5) || ((__GNUG__ == 4) && (__GNUC_MINOR__ >= 4))) \
    || (defined(__clang__) && (__clang_major__ >= 3)) || (_MSC_VER >= 1800)
#define CC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &) = delete; \
    TypeName &operator =(const TypeName &) = delete;
#else
#define CC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName &); \
    TypeName &operator =(const TypeName &);
#endif



/** @def CREATE_FUNC(__TYPE__)
 * Define a create function for a specific type, such as Layer.
 *
 * @param __TYPE__  class type to add create(), such as Layer.
 */
#define CREATE_FUNC(__TYPE__) \
static __TYPE__* create() \
{ \
    __TYPE__ *pRet = new(std::nothrow) __TYPE__(); \
    if (pRet && pRet->init()) \
    { \
        pRet->autorelease(); \
        return pRet; \
    } \
    else \
    { \
        delete pRet; \
        pRet = nullptr; \
        return nullptr; \
    } \
}

 /** @def NODE_FUNC(__TYPE__)
  * Define a node function for a specific type, such as Layer.
  *
  * @param __TYPE__  class type to add node(), such as Layer.
  * @deprecated  This interface will be deprecated sooner or later.
  */
#define NODE_FUNC(__TYPE__) \
CC_DEPRECATED_ATTRIBUTE static __TYPE__* node() \
{ \
    __TYPE__ *pRet = new(std::nothrow) __TYPE__(); \
    if (pRet && pRet->init()) \
    { \
        pRet->autorelease(); \
        return pRet; \
    } \
    else \
    { \
        delete pRet; \
        pRet = NULL; \
        return NULL; \
    } \
}


  // new callbacks based on C++11
#define FUN_CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define FUN_CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define FUN_CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define FUN_CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ##__VA_ARGS__)



#endif // __BASE_HPP__
