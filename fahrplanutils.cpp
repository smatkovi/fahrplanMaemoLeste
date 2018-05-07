#include "fahrplanutils.h"

fahrplanUtils::fahrplanUtils()
{
}

QStringList fahrplanUtils::reverseQStringList(const QStringList &list)
{
    if (list.isEmpty())
    {
        return QStringList();
    }

    QStringList reversedList = list;
    const int listSize = list.size();
    const int maxSwap = list.size() / 2;
    for (int i = 0; i < maxSwap; ++i)
    {
        qSwap(reversedList[i], reversedList[listSize - 1 -i]);
    }

    return reversedList;
}

QString fahrplanUtils::leadingZeros(int number, int presision)
{
    QString s;
    s.setNum(number);
    s = s.toUpper();
    presision -= s.length();
    while(presision>0){
        s.prepend('0');
        presision--;
    }
    return s;
}

/*
 * Replace common used umlaut chars to a maybe wrong but
 * readable format (used for calendar export)
 */
QString fahrplanUtils::removeUmlauts(QString text)
{
    text.replace("ä", "ae");
    text.replace("ö", "oe");
    text.replace("ü", "ue");
    text.replace("Ä", "Ae");
    text.replace("Ö", "Oe");
    text.replace("Ü", "üe");
    text.replace("ß", "ss");
    text.replace("é", "e");
    text.replace("è", "e");
    text.replace("á", "a");
    text.replace("à", "a");
    text.replace("ú", "u");
    text.replace("ù", "u");
    text.replace(QChar(160), " "); //space from nbsp
    text.replace("<br>", "\n"); //html linewrap
    text.replace("<br/>", "\n"); //html linewrap
    text.replace("<br />", "\n"); //html linewrap
    return text;
}
