# -------------------------------------------------
# Project created by QtCreator 2009-08-28T21:45:08
# -------------------------------------------------
QT += sql
QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
CONFIG += x86 \
    ppc
TARGET = FlashCards
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    flashcardtextitem.cpp \
    flashcard.cpp \
    flashcardpictureitem.cpp \
    resizehandle.cpp
HEADERS += mainwindow.h \
    flashcardtextitem.h \
    flashcard.h \
    flashcardpictureitem.h \
    resizehandle.h
FORMS += mainwindow.ui \
    newCardDialog.ui
