# Copyright (C) 2018 Softus Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

lessThan(QT_MAJOR_VERSION, 5): error (QT 5.0 or newer is required)

isEmpty(PREFIX): PREFIX = /usr
TARGET   = idcdicomquery
TEMPLATE = app
VERSION  = 1.0.1

DEFINES += PREFIX=$$PREFIX \
    PRODUCT_NAME=\\\"$$TARGET\\\" \
    PRODUCT_VERSION=\\\"$$VERSION\\\"

CONFIG  += link_pkgconfig c++11
QT      += core gui widgets network

SOURCES += src/main.cpp\
        src/mainwindow.cpp

HEADERS += src/mainwindow.h \
    src/QWaitCursor \
    src/qwaitcursor.h \
    src/QUtf8Settings \
    src/qutf8settings.h \
    src/product.h

FORMS   += ui/mainwindow.ui

TRANSLATIONS += idcdicomquery_ru.ts

# DCMTK isn't pgk-config compatible at all.
LIBS         += -ldcmnet -ldcmdata -loflog -lofstd -lz
win32:LIBS   += -ladvapi32 -liphlpapi -lnetapi32 -luser32 -lws2_32 -lwsock32

# MacOS specific
ICON = ui/idcdicomquery.icns
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
QMAKE_TARGET_BUNDLE_PREFIX = github.Softus

# Windows specific
RC_ICONS = ui/idcdicomquery.ico
QMAKE_TARGET_COPYRIGHT = Softus Inc. <team@softus.org>
QMAKE_TARGET_DESCRIPTION = Utility program to query a DICOM server and retrieve studies from the server.

# Linux specific
target.path=$$PREFIX/bin
man.files=doc/idcdicomquery.1
man.path=$$PREFIX/share/man/man1
shortcut.files = idcdicomquery.desktop
shortcut.path = $$PREFIX/share/applications
icon.files = ui/idcdicomquery.png
icon.path = $$PREFIX/share/icons
translations.files = idcdicomquery_ru.qm
translations.path  = $$PREFIX/share/idcdicomquery/translations

INSTALLS += target man shortcut icon translations

RESOURCES += \
    ui/idcdicomquery.qrc
