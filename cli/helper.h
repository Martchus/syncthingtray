#ifndef SYNCTHINGCTL_HELPER
#define SYNCTHINGCTL_HELPER

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>
#include <c++utilities/misc/traits.h>

#include <QString>
#include <QStringList>

#include <cstring>
#include <iostream>

namespace Cli {

inline void printProperty(const char *propName, const char *value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    if (*value) {
        std::cout << indentation << propName << CppUtilities::Indentation(static_cast<unsigned char>(30 - strlen(propName))) << value;
        if (suffix) {
            std::cout << ' ' << suffix;
        }
        std::cout << '\n';
    }
}

inline void printProperty(const char *propName, const QString &value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    printProperty(propName, value.toLocal8Bit().data(), suffix, indentation);
}

inline void printProperty(const char *propName, const std::string &value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    printProperty(propName, value.data(), suffix, indentation);
}

inline void printProperty(const char *propName, const QStringList &value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    for (const QString &str : value) {
        printProperty(propName, str, suffix, indentation);
        propName = "";
    }
}

inline void printProperty(
    const char *propName, CppUtilities::TimeSpan timeSpan, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    if (!timeSpan.isNull()) {
        printProperty(propName, timeSpan.toString(CppUtilities::TimeSpanOutputFormat::WithMeasures).data(), suffix, indentation);
    }
}

inline void printProperty(
    const char *propName, CppUtilities::DateTime dateTime, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    if (!dateTime.isNull()) {
        printProperty(propName, dateTime.toString().data(), suffix, indentation);
    }
}

inline void printProperty(const char *propName, bool value, const char *suffix = nullptr, CppUtilities::Indentation indentation = 3)
{
    printProperty(propName, value ? "yes" : "no", suffix, indentation);
}

template <typename NumberType, CppUtilities::Traits::EnableIfAny<std::is_floating_point<NumberType>, std::is_integral<NumberType>> * = nullptr>
inline void printProperty(
    const char *propName, const NumberType value, const char *suffix = nullptr, bool force = false, CppUtilities::Indentation indentation = 3)
{
    if (value >= 0 || force) {
        printProperty(propName, CppUtilities::numberToString<NumberType>(value).data(), suffix, indentation);
    }
}
} // namespace Cli

#endif // SYNCTHINGCTL_HELPER
