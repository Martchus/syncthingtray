// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef LIB_SYNCTHING_GLOBAL
#define LIB_SYNCTHING_GLOBAL

#include <c++utilities/application/global.h>

#ifdef LIB_SYNCTHING_STATIC
#define LIB_SYNCTHING_EXPORT
#define LIB_SYNCTHING_IMPORT
#else
#define LIB_SYNCTHING_EXPORT CPP_UTILITIES_GENERIC_LIB_EXPORT
#define LIB_SYNCTHING_IMPORT CPP_UTILITIES_GENERIC_LIB_IMPORT
#endif

/*!
 * \def LIB_SYNCTHING_EXPORT
 * \brief Marks the symbol to be exported by the syncthing library.
 */

/*!
 * \def LIB_SYNCTHING_IMPORT
 * \brief Marks the symbol to be imported from the syncthing library.
 */

#endif // LIB_SYNCTHING_GLOBAL
