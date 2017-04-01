// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGTESTHELPER_GLOBAL
#define SYNCTHINGTESTHELPER_GLOBAL

#include <c++utilities/application/global.h>

#ifdef SYNCTHINGTESTHELPER_STATIC
# define SYNCTHINGTESTHELPER_EXPORT
# define SYNCTHINGTESTHELPER_IMPORT
#else
# define SYNCTHINGTESTHELPER_EXPORT LIB_EXPORT
# define SYNCTHINGTESTHELPER_IMPORT LIB_IMPORT
#endif

/*!
 * \def SYNCTHINGTESTHELPER_EXPORT
 * \brief Marks the symbol to be exported by the syncthingtesthelper library.
 */

/*!
 * \def SYNCTHINGTESTHELPER_IMPORT
 * \brief Marks the symbol to be imported from the syncthingtesthelper library.
 */

#endif // SYNCTHINGTESTHELPER_GLOBAL
