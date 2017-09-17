#ifndef DATA_COLORS_H
#define DATA_COLORS_H

#include <QColor>

namespace Data {

/*!
 * \brief The Colors namespace defines default colors (regular and bright version).
 */
namespace Colors {

inline QColor gray(bool bright)
{
    return bright ? QColor(Qt::lightGray) : QColor(Qt::darkGray);
}

inline QColor red(bool bright)
{
    return bright ? QColor(0xFF9A7E) : QColor(Qt::red);
}

inline QColor green(bool bright)
{
    return bright ? QColor(0xA8FF41) : QColor(Qt::darkGreen);
}

inline QColor blue(bool bright)
{
    return bright ? QColor(0x8BCFFF) : QColor(Qt::blue);
}

inline QColor orange(bool bright)
{
    return bright ? QColor(0xFFC500) : QColor(0xA85900);
}
} // namespace Colors

} // namespace Data

#endif // DATA_COLORS_H
