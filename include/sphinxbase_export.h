#ifndef __SPHINXBASE_EXPORT_H__
#define __SPHINXBASE_EXPORT_H__

/* Win32/WinCE DLL gunk */
#if (defined(_WIN32) || defined(_WIN32_WCE)) \
	&& defined(SPHINXDLL) && !defined(CYGWIN)
#ifdef SPHINXBASE_EXPORTS
#define SPHINXBASE_EXPORT __declspec(dllexport)
#else
#define SPHINXBASE_EXPORT __declspec(dllimport)
#endif
#else /* !_WIN32 */
#define SPHINXBASE_EXPORT
#endif

#endif /* __SPHINXBASE_EXPORT_H__ */
