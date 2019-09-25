QT += core widgets

TARGET = BSFlashManager
TEMPLATE = app
CONFIG += c99 c++11

CONFIG(debug, debug|release) {
    DESTDIR = debug
}
CONFIG(release, debug|release) {
    DESTDIR = release
}

OBJECTS_DIR = obj/$$DESTDIR
MOC_DIR = $$OBJECTS_DIR
RCC_DIR = $$OBJECTS_DIR

# OS-specific metadata and stuff
# win32:RC_FILE = src/windows.rc
# macx:ICON = src/images/main.icns

# build on OS X with xcode/clang and libc++
macx:QMAKE_CXXFLAGS += -stdlib=libc++

LIBS += -L$$DESTDIR -llibusb-1.0

RESOURCES += \
    src/mainwindow.qrc

FORMS += \
    src/mainwindow.ui \
    src/usbdump.ui

HEADERS += \
    src/endian.h \
    src/mainwindow.h \
    src/mempackitem.h \
    src/mempackmodel.h \
    src/usb/device.h \
    src/usb/inlretro.h \
    src/usbdump.h

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/mempackitem.cpp \
    src/mempackmodel.cpp \
    src/usb/device.cpp \
    src/usb/inlretro.cpp \
    src/usbdump.cpp
