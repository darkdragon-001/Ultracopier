DEFINES += ULTRACOPIER_PLUGIN_ALL_IN_ONE

include(ultracopier-core.pro)

RESOURCES += $$PWD/../plugins/static-plugins.qrc \
    $$PWD/../plugins/CopyEngine/Ultracopier/copyEngineResources.qrc

win32:RESOURCES += $$PWD/../plugins/static-plugins-windows.qrc

LIBS           = -Lplugins -lcopyEngine -linterface -llistener
win32:LIBS += -lpluginLoader -lsessionLoader

build_pass:CONFIG(debug, debug|release) {
LIBS           = -Lplugins -lcopyEngined -linterfaced -llistenerd
win32:LIBS += -lpluginLoaderd -lsessionLoaderd
}

HEADERS -= $$PWD/../AuthPlugin.h
SOURCES -= $$PWD/../AuthPlugin.cpp

RESOURCES -= $$PWD/../resources/resources-windows-qt-plugin.qrc
