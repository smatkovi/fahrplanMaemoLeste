# Console probe for any backend, the way the application drives it (through
# the Fahrplan object and its parser thread), see leste-probe.cpp. Needs no
# display, so it can run on the device over ssh:
#
#   cd tools/leste-probe && qmake && make
#   ./leste-probe "Austria" "Wien Hbf" "Linz Hbf"

QT += core gui network xml concurrent
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
TARGET = leste-probe

MOC_DIR = tmp
OBJECTS_DIR = tmp
INCLUDEPATH += $$PWD/../../src

DEFINES += BUILD_FOR_QT5 BUILD_FOR_MAEMO_LESTE FAHRPLAN_VERSION=\\\"probe\\\" FAHRPLAN_SETTINGS_NAMESPACE=\\\"smurfy\\\"

HEADERS += \
    $$files($$PWD/../../src/parser/*.h) \
    $$files($$PWD/../../src/models/*.h) \
    $$PWD/../../src/fahrplan.h \
    $$PWD/../../src/fahrplan_backend_manager.h \
    $$PWD/../../src/fahrplan_parser_thread.h \
    $$PWD/../../src/calendarthreadwrapper.h \
    $$PWD/../../src/calendar_sfos_wrapper.h

SOURCES += \
    leste-probe.cpp \
    $$files($$PWD/../../src/parser/*.cpp) \
    $$files($$PWD/../../src/models/*.cpp) \
    $$PWD/../../src/fahrplan.cpp \
    $$PWD/../../src/fahrplan_backend_manager.cpp \
    $$PWD/../../src/fahrplan_parser_thread.cpp \
    $$PWD/../../src/calendarthreadwrapper.cpp \
    $$PWD/../../src/calendar_sfos_wrapper.cpp \
    $$PWD/../../3rdparty/gauss-kruger-cpp/gausskruger.cpp

# not part of the application either
HEADERS -= $$PWD/../../src/parser/parser_mobilebahnde.h
SOURCES -= $$PWD/../../src/parser/parser_mobilebahnde.cpp

LIBS += -lz
