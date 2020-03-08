#ifndef CLI_JS_CONSOLE_H
#define CLI_JS_CONSOLE_H

#include <QObject>

class JSConsole : public QObject
{
    Q_OBJECT
public:
    explicit JSConsole(QObject *parent = nullptr);

public Q_SLOTS:
    void log(const QString &msg) const;
};

#endif // CLI_JS_CONSOLE_H
