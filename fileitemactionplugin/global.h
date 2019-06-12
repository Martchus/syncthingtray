// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGFILEITEMACTION_GLOBAL
#define SYNCTHINGFILEITEMACTION_GLOBAL

#include <c++utilities/application/global.h>

#ifdef SYNCTHINGFILEITEMACTION_STATIC
#define SYNCTHINGFILEITEMACTION_EXPORT
#define SYNCTHINGFILEITEMACTION_IMPORT
#else
#define SYNCTHINGFILEITEMACTION_EXPORT CPP_UTILITIES_GENERIC_LIB_EXPORT
#define SYNCTHINGFILEITEMACTION_IMPORT CPP_UTILITIES_GENERIC_LIB_IMPORT
#endif

/*!
 * \def SYNCTHINGFILEITEMACTION_EXPORT
 * \brief Marks the symbol to be exported by the syncthingfileitemaction library.
 */

/*!
 * \def SYNCTHINGFILEITEMACTION_IMPORT
 * \brief Marks the symbol to be imported from the syncthingfileitemaction library.
 */

#endif // SYNCTHINGFILEITEMACTION_GLOBAL
