/*
 * Copyright (C) 2013-2018 Softus Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "product.h"

// QT
#include <QApplication>
#include <QCommandLineParser>
#ifdef Q_OS_WIN
#include <QSettings>
#endif
#include <QTranslator>

// DCMTK
#ifdef UNICODE
#define DCMTK_UNICODE_BUG_WORKAROUND
#undef UNICODE
#endif

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/oflog/logger.h>
namespace dcmtk{}
using namespace dcmtk;

#ifdef DCMTK_UNICODE_BUG_WORKAROUND
#define UNICODE
#undef DCMTK_UNICODE_BUG_WORKAROUND
#endif

inline QString tr(const char *str)
{
    return QCoreApplication::translate("main", str);
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    QSettings::setDefaultFormat(QSettings::IniFormat);
#endif
    QApplication::setOrganizationName(ORGANIZATION_DOMAIN);
    QApplication::setApplicationName(PRODUCT_NAME);
    QApplication::setApplicationVersion(PRODUCT_VERSION);

    QApplication app(argc, argv);

    // Translations
    //
    QTranslator  translator;
    auto locale = QLocale::system().name();

    if (translator.load(PRODUCT_NAME "_" + locale, TRANSLATIONS_FOLDER))
    {
        app.installTranslator(&translator);
    }
    app.setWindowIcon(QIcon(":/app/" PRODUCT_NAME));
    QCommandLineParser parser;
    parser.setApplicationDescription(tr("IDC DICOM query utility"));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption logLevelOption("log-level",
        tr("Log output level (off, fatal, error, warn, info, debug, trace)."), tr("level"));
    parser.addOption(logLevelOption);

    // Process the actual command line arguments given by the user.
    parser.process(app);

    if (parser.isSet(logLevelOption))
    {
        auto levelName = parser.value(logLevelOption).toLocal8Bit().constData();
        auto level = log4cplus::getLogLevelManager().fromString(levelName);
        log4cplus::Logger::getRoot().setLogLevel(level);
    }

    MainWindow w;
    w.show();

    return app.exec();
}
