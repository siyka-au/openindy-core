#ifndef OI_CORE_CORE_EXPORT_H_
#define OI_CORE_CORE_EXPORT_H_

// Symbol visibility for the openindy-core shared library, with no dependency
// on Qt. OI_CORE_LIB is defined while building the library itself.
#if defined(_WIN32) || defined(__CYGWIN__)
#define OI_CORE_DECL_EXPORT __declspec(dllexport)
#define OI_CORE_DECL_IMPORT __declspec(dllimport)
#else
#define OI_CORE_DECL_EXPORT __attribute__((visibility("default")))
#define OI_CORE_DECL_IMPORT __attribute__((visibility("default")))
#endif

#if defined(OI_CORE_LIB)
#define OI_CORE_EXPORT OI_CORE_DECL_EXPORT
#else
#define OI_CORE_EXPORT OI_CORE_DECL_IMPORT
#endif

#endif  // OI_CORE_CORE_EXPORT_H_
