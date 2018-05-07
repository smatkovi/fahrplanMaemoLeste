#-------------------------------------------------
#
# Project created by QtCreator 2010-05-31T15:48:58
#
#-------------------------------------------------

QT       += core gui maemo5 xmlpatterns network

TARGET = fahrplan
TEMPLATE = app

TRANSLATIONS = fahrplan_de.ts\
	       fahrplan_fr.ts

SOURCES += main.cpp\
        mainwindow.cpp \
    parser_mobilebahnde.cpp \
    selectstationdialog.cpp \
    parser_abstract.cpp \
    parser_mvvde.cpp \
    configdialog.cpp \
    backupgpsopenstreetmap.cpp \
    fahrplanutils.cpp \
    parser_sbbch.cpp \
    calendardialog.cpp \
    parser_131500comau.cpp \
    qvaluebutton.cpp \
    parser_hafasxml.cpp \
    parser_xmloebbat.cpp \
    parser_xmlsbbch.cpp \
    parser_xmlrejseplanendk.cpp \
    parser_translink.cpp

HEADERS  += mainwindow.h \
    parser_mobilebahnde.h \
    selectstationdialog.h \
    parser_abstract.h \
    parser_mvvde.h \
    configdialog.h \
    backupgpsopenstreetmap.h \
    fahrplanutils.h \
    parser_sbbch.h \
    calendardialog.h \
    parser_131500comau.h \
    qvaluebutton.h \
    parser_hafasxml.h \
    parser_xmloebbat.h \
    parser_xmlsbbch.h \
    parser_xmlrejseplanendk.h \
    parser_translink.h

INSTALLS    += target
target.path  = /opt/fahrplan/bin
	
INSTALLS    += desktop
desktop.path  = /usr/share/applications/hildon
desktop.files  = data/fahrplan.desktop

INSTALLS    += service
service.path  = /usr/share/dbus-1/services
service.files  = data/fahrplan.service 

INSTALLS    += icon
icon.path  = /usr/share/icons
icon.files  = data/fahrplan.png

debian-src.commands = dpkg-buildpackage -S -r -us -uc -d
debian-bin.commands = dpkg-buildpackage -b -r -uc -d
debian-all.depends = debian-src debian-bin

LIBS += -lQtLocation

CONFIG += mobility qdbus
MOBILITY += location

CONFIG += link_pkgconfig
PKGCONFIG += QJson
PKGCONFIG += calendar-backend
LIBS += -lcalendar_backend

symbian {
    TARGET.UID3 = 0xe593ff75
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
    TARGET.CAPABILITY = Location
}

OTHER_FILES += \
    todo.txt \
    debian/control \
    debian/changelog \
    qtc_packaging/debian_fremantle/rules \
    qtc_packaging/debian_fremantle/README \
    qtc_packaging/debian_fremantle/copyright \
    qtc_packaging/debian_fremantle/control \
    qtc_packaging/debian_fremantle/compat \
    qtc_packaging/debian_fremantle/changelog \
    qtc_packaging/debian_fremantle/rules \
    qtc_packaging/debian_fremantle/README \
    qtc_packaging/debian_fremantle/copyright \
    qtc_packaging/debian_fremantle/control \
    qtc_packaging/debian_fremantle/compat \
    qtc_packaging/debian_fremantle/changelog \
    qtc_packaging/debian_fremantle/fahrplan.postinst

RESOURCES += \
    fahrplan.qrc
