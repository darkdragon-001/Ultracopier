include($$PWD/../Oxygen/interfaceInclude.pri)

!CONFIG(static) {
RESOURCES	+= \
    $$PWD/../Oxygen/interfaceResources.qrc \
    $$PWD/../Oxygen/interfaceResources_unix.qrc \
    $$PWD/../Oxygen/interfaceResources_windows.qrc
}
