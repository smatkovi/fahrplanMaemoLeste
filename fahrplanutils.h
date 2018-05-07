#ifndef FAHRPLANUTILS_H
#define FAHRPLANUTILS_H

#include <QtGui>

class fahrplanUtils
{
public:
    fahrplanUtils();
    static QStringList reverseQStringList(const QStringList &list);
    static QString removeUmlauts(QString text);
    static QString leadingZeros(int number, int presision);
};

#endif // FAHRPLANUTILS_H
