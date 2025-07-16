QT       += core gui network widgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += lrelease

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    kworker.cpp \
    main.cpp \
    dashboard.cpp \
    motion.cpp \
    scanner.cpp \
    utility.cpp \
    visual.cpp

HEADERS += \
    dashboard.h \
    kworker.h \
    motion.h \
    scanner.h \
    utility.h \
    visual.h

FORMS += \
    dashboard.ui

TRANSLATIONS += \
    Ark_Alarm_V2_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# ———————————— OpenCV ————————————
INCLUDEPATH += "C:/msys64/mingw64/include/opencv4"
LIBS += -L"C:/msys64/mingw64/lib" \
        -lopencv_core \
        -lopencv_imgproc \
        -lopencv_highgui \
        -lopencv_imgcodecs

# ————————— Tesseract OCR & Leptonica —————————
INCLUDEPATH += "C:/msys64/mingw64/include"
LIBS += -L"C:/msys64/mingw64/lib" \
        -ltesseract \
        -lleptonica

# ————————— Windows API —————————
win32: LIBS += -lUser32 -lgdi32 -lws2_32 -liphlpapi
