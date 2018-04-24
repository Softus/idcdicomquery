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
#include <dcmtk/oflog/configrt.h>
#include <dcmtk/oflog/fileap.h>
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

    QCommandLineOption logConfigOption("log-config",
        tr("The config file to initialize the logger."), tr("FILE"));
    QCommandLineOption logFileOption("log-file",
        tr("The file to write the log output."), tr("FILE"));
    QCommandLineOption logLevelOption("log-level",
        tr("The log output level (off, fatal, error, warn, info, debug, trace)."), tr("LEVEL"));
    parser.addOptions({logConfigOption, logFileOption, logLevelOption});

    // Process the actual command line arguments given by the user.
    parser.process(app);

    if (parser.isSet(logConfigOption))
    {
        auto configFile = parser.value(logConfigOption).toLocal8Bit().constData();
        log4cplus::PropertyConfigurator(configFile).configure();
    }
    if (parser.isSet(logFileOption))
    {
        auto logFile = parser.value(logFileOption).toLocal8Bit().constData();
        log4cplus::SharedAppenderPtr file(new log4cplus::FileAppender(logFile));
        log4cplus::Logger::getRoot().removeAllAppenders();
        log4cplus::Logger::getRoot().addAppender(file);
    }
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
