#include "./jsconsole.h"

#include <iostream>

using namespace std;

JSConsole::JSConsole(QObject *parent) : QObject(parent)
{
}

void JSConsole::log(const QString &msg) const
{
    cerr << "script: "<< msg.toStdString() << endl;
}
