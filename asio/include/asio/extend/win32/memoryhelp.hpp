#pragma once
#ifndef __DbgMemLeak_H__
#define __DbgMemLeak_H__

//
// only included in cpp file
//
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
#include <crtdbg.h>
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
#ifndef _CRTDBG_MAP_ALLOC
	#define _CRTDBG_MAP_ALLOC
#endif
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif
//////////////////////////////////////////////////////////////////////////

class DbgMemLeak
{
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
	_CrtMemState startMemState;
	_CrtMemState endMemState;
#endif

public:
	explicit DbgMemLeak()
	{
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
		memset(&startMemState, 0, sizeof(_CrtMemState));
		memset(&endMemState, 0, sizeof(_CrtMemState));
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		_CrtMemCheckpoint(&startMemState);
		// Enable MS Visual C++ debug heap memory leaks dump on exit
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
	};

	~DbgMemLeak()
	{
#if defined(WIN32) && defined(_MSC_VER) &&  defined(_DEBUG)
		{
			_CrtMemCheckpoint(&endMemState);

			_CrtMemState diffMemState;
			_CrtMemDifference(&diffMemState, &startMemState, &endMemState);
			_CrtMemDumpStatistics(&diffMemState);
		}
#endif
	}
};

#endif //__DbgMemLeak_H__
