#pragma once

// This is here to keep MSVC from spamming the build output with nonsense
// Call it public domain.

#ifdef _MSC_VER
	// 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
	// C4251 can be ignored in Microsoft Visual C++ 2005 if you are deriving from a type
	// in the Standard C++ Library, compiling a debug release (/MTd) and where the compiler
	// error message refers to _Container_base.
	// Shows up when you export classes that use STL types. Stupid.
	// #pragma warning( disable: 4251 )
	
	// C4273 - inconsistent DLL linkage. how about none?
	#pragma warning( disable: 4273 )
	
	// don't display bogus 'deprecation' and 'unsafe' warnings.
	// See the idiocy: http://msdn.microsoft.com/en-us/magazine/cc163794.aspx
	#define _CRT_SECURE_NO_DEPRECATE
	#define _SCL_SECURE_NO_DEPRECATE
	// Let me demonstrate:
	/**
	 * [peterix@peterix dfhack]$ man wcscpy_s
	 * No manual entry for wcscpy_s
	 *
	 * Proprietary extensions.
	 */
	//'function': was declared deprecated
	#pragma warning( disable: 4996 )
	
	// disable stupid - forcing value to bool 'true' or 'false' (performance warning).
	// When I do this, it's intentional. Always.
	#pragma warning( disable: 4800 )
	
	// disable more stupid - The compiler ignored an unrecognized pragma. GOOD JOB, DON'T SPAM ME WITH THAT
	#pragma warning( disable: 4068 )
	
	// no signed value outside enum range bs
	//#pragma warning( disable: 4341)
	
	// just shut up already - conversion between types loses precision
	//#pragma warning( disable: 4244)
	
	// signed/unsigned mismatch
	//#pragma warning( disable: 4018)
	
	// nonstandard extension used: enum 'df::whatever::etc' used in qualified name
	//#pragma warning( disable: 4482)
#endif
