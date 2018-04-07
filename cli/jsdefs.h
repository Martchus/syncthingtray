// Created via CMake from template jsdefs.h.in
// WARNING! Any changes to this file will be overwritten by the next CMake run!

#ifndef SYNCTHINGCTL_JAVA_SCRIPT_DEFINES
#define SYNCTHINGCTL_JAVA_SCRIPT_DEFINES

#include <QtGlobal>

#if defined(SYNCTHINGCTL_USE_JSENGINE)
# define SYNCTHINGCTL_JS_ENGINE QJSEngine
# define SYNCTHINGCTL_JS_VALUE QJSValue
# define SYNCTHINGCTL_JS_READONLY
# define SYNCTHINGCTL_JS_UNDELETABLE
# define SYNCTHINGCTL_JS_QOBJECT(engine, obj) engine.newQObject(obj)
# define SYNCTHINGCTL_JS_INT(value) value.toInt()
# define SYNCTHINGCTL_JS_IS_VALID_PROG(program) (!program.isError() && program.isCallable())
#elif defined(SYNCTHINGCTL_USE_SCRIPT)
# define SYNCTHINGCTL_JS_ENGINE QScriptEngine
# define SYNCTHINGCTL_JS_VALUE QScriptValue
# define SYNCTHINGCTL_JS_READONLY ,QScriptValue::ReadOnly
# define SYNCTHINGCTL_JS_UNDELETABLE ,QScriptValue::Undeletable
# define SYNCTHINGCTL_JS_QOBJECT(engine, obj) engine.newQObject(obj, QScriptEngine::ScriptOwnership)
# define SYNCTHINGCTL_JS_INT(value) value.toInt32()
# define SYNCTHINGCTL_JS_IS_VALID_PROG(program) (!program.isError() && program.isFunction())
#elif !defined(SYNCTHINGCTL_NO_JSENGINE)
# error "No definition for JavaScript provider present."
#endif

#ifdef SYNCTHINGCTL_JS_ENGINE
QT_FORWARD_DECLARE_CLASS(SYNCTHINGCTL_JS_ENGINE)
#endif
#ifdef SYNCTHINGCTL_JS_VALUE
QT_FORWARD_DECLARE_CLASS(SYNCTHINGCTL_JS_VALUE)
#endif

#endif // SYNCTHINGCTL_JAVA_SCRIPT_DEFINES
