// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGWIDGETS_GLOBAL
#define SYNCTHINGWIDGETS_GLOBAL

#include <c++utilities/application/global.h>

#ifdef SYNCTHINGWIDGETS_STATIC
#define SYNCTHINGWIDGETS_EXPORT
#define SYNCTHINGWIDGETS_IMPORT
#else
#define SYNCTHINGWIDGETS_EXPORT CPP_UTILITIES_GENERIC_LIB_EXPORT
#define SYNCTHINGWIDGETS_IMPORT CPP_UTILITIES_GENERIC_LIB_IMPORT
#endif

/*!
 * \def SYNCTHINGWIDGETS_EXPORT
 * \brief Marks the symbol to be exported by the syncthingwidgets library.
 */

/*!
 * \def SYNCTHINGWIDGETS_IMPORT
 * \brief Marks the symbol to be imported from the syncthingwidgets library.
 */

#endif // SYNCTHINGWIDGETS_GLOBAL
