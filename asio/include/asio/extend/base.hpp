#ifndef __BASE_HPP__
#define __BASE_HPP__
#include <cstdint>

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

#include <iostream>
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




#endif // __BASE_HPP__
