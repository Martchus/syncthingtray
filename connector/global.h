// Created via CMake from template global.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef LIB_SYNCTHING_CONNECTOR_GLOBAL
#define LIB_SYNCTHING_CONNECTOR_GLOBAL

#include <c++utilities/application/global.h>

#ifdef LIB_SYNCTHING_CONNECTOR_STATIC
#define LIB_SYNCTHING_CONNECTOR_EXPORT
#define LIB_SYNCTHING_CONNECTOR_IMPORT
#else
#define LIB_SYNCTHING_CONNECTOR_EXPORT CPP_UTILITIES_GENERIC_LIB_EXPORT
#define LIB_SYNCTHING_CONNECTOR_IMPORT CPP_UTILITIES_GENERIC_LIB_IMPORT
#endif

/*!
 * \def LIB_SYNCTHING_CONNECTOR_EXPORT
 * \brief Marks the symbol to be exported by the syncthingconnector library.
 */

/*!
 * \def LIB_SYNCTHING_CONNECTOR_IMPORT
 * \brief Marks the symbol to be imported from the syncthingconnector library.
 */

#endif // LIB_SYNCTHING_CONNECTOR_GLOBAL
