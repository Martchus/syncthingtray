#ifndef SYNCTHINGCTL_HELPER
#define SYNCTHINGCTL_HELPER

#include <c++utilities/application/commandlineutils.h>
#include <c++utilities/chrono/datetime.h>
#include <c++utilities/chrono/timespan.h>
#include <c++utilities/conversion/stringconversion.h>

#include <QString>
#include <QStringList>

#include <cstring>
#include <iostream>

namespace Cli {

inline void printProperty(const char *propName, const char *value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    if (*value) {
        std::cout << indentation << propName << ApplicationUtilities::Indentation(30 - strlen(propName)) << value;
        if (suffix) {
            std::cout << ' ' << suffix;
        }
        std::cout << '\n';
    }
}

inline void printProperty(const char *propName, const QString &value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    printProperty(propName, value.toLocal8Bit().data(), suffix, indentation);
}

inline void printProperty(
    const char *propName, const QStringList &value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    for (const QString &str : value) {
        printProperty(propName, str, suffix, indentation);
        propName = "";
    }
}

inline void printProperty(
    const char *propName, ChronoUtilities::TimeSpan timeSpan, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    if (!timeSpan.isNull()) {
        printProperty(propName, timeSpan.toString(ChronoUtilities::TimeSpanOutputFormat::WithMeasures).data(), suffix, indentation);
    }
}

inline void printProperty(
    const char *propName, ChronoUtilities::DateTime dateTime, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    if (!dateTime.isNull()) {
        printProperty(propName, dateTime.toString().data(), suffix, indentation);
    }
}

inline void printProperty(const char *propName, bool value, const char *suffix = nullptr, ApplicationUtilities::Indentation indentation = 3)
{
    printProperty(propName, value ? "yes" : "no", suffix, indentation);
}

template <typename intType>
inline void printProperty(
    const char *propName, const intType value, const char *suffix = nullptr, bool force = false, ApplicationUtilities::Indentation indentation = 3)
{
    if (value != 0 || force) {
        printProperty(propName, ConversionUtilities::numberToString<intType>(value).data(), suffix, indentation);
    }
}
}

#endif // SYNCTHINGCTL_HELPER
