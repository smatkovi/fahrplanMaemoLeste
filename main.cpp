/*******************************************************************************

    This file is a part of Fahrplan for maemo 2009-2010

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include <QtGui/QApplication>
#include <QtDBus>
#include "mainwindow.h"





void myMessageOutput(QtMsgType type, const char *msg)
 {
     switch (type) {
     case QtDebugMsg:
         fprintf(stderr, "===== Debug: %s\n", msg);
         break;
     case QtWarningMsg:
   //      fprintf(stderr, "===== Warning: %s\n", msg); //disabled, we dont want to see anoying warnings
         break;
     case QtCriticalMsg:
         fprintf(stderr, "===== Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "===== Fatal: %s\n", msg);
         abort();
     }
 }







int main(int argc, char *argv[])
{
    qInstallMsgHandler(myMessageOutput); //use our own message handlers
    qDebug() << "Fahrplan start ===============================================";

    QApplication a(argc, argv);

    Q_INIT_RESOURCE(fahrplan);

    QTranslator translator;
    translator.load(QLocale::system().name(), ":/translation");
    a.installTranslator(&translator);

    MainWindow w;

    if (!QDBusConnection::sessionBus().isConnected())
    {
        qWarning("Cannot connect to the D-Bus session bus.");
        exit(1);
    }

    if (!QDBusConnection::sessionBus().registerService("com.nokia.fahrplan"))
    {
        qWarning("%s", qPrintable(QDBusConnection::sessionBus().lastError().message()));
        exit(2);
    }

    if (!QDBusConnection::sessionBus().registerObject("/com/nokia/fahrplan", &w, QDBusConnection::ExportScriptableSlots))
    {
        qWarning("%s", qPrintable(QDBusConnection::sessionBus().lastError().message()));
        exit(3);
    }




#if defined(Q_WS_S60)
    w.showMaximized();
#else
    w.show();
#endif

    qDebug() << "Initialisation done ====================";

    return a.exec();
}
