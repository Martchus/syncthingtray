// Created via CMake from template jsincludes.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGCTL_JAVA_SCRIPT_INCLUDES
#define SYNCTHINGCTL_JAVA_SCRIPT_INCLUDES

#include <QtGlobal>

#if defined(SYNCTHINGCTL_USE_JSENGINE)
# include <QJSEngine>
# include <QJSValue>
#elif defined(SYNCTHINGCTL_USE_SCRIPT)
# include <QScriptEngine>
# include <QScriptValue>
#elif !defined(SYNCTHINGCTL_NO_JSENGINE)
# error "No definition for JavaScript provider present."
#endif

#endif // SYNCTHINGCTL_JAVA_SCRIPT_INCLUDES
