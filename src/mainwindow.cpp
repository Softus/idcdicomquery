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
#include "ui_mainwindow.h"
#include "product.h"
#include "QUtf8Settings"
#include "QWaitCursor"

// QT
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>

// DCMTK
#ifdef UNICODE
#define DCMTK_UNICODE_BUG_WORKAROUND
#undef UNICODE
#endif

#define HAVE_CONFIG_H
#include <dcmtk/config/osconfig.h>   /* make sure OS specific configuration is included first */
#include <dcmtk/dcmdata/dcuid.h>
#include "dcmtk/dcmnet/dfindscu.h"
#include "dcmtk/dcmnet/scu.h"
#include <dcmtk/oflog/logger.h>
namespace dcmtk{}
using namespace dcmtk;

#if OFFIS_DCMTK_VERSION_NUMBER < 363
#define FEM_none false
#endif

#ifdef DCMTK_UNICODE_BUG_WORKAROUND
#define UNICODE
#undef DCMTK_UNICODE_BUG_WORKAROUND
#endif

Q_DECLARE_METATYPE(DcmDataset)
Q_DECLARE_METATYPE(DcmTag)

#define DEFAULT_DICOM_PORT 11112
#ifdef Q_OS_LINUX
#define DEFAULT_VIEWER "nautilus"
#define DEFAULT_VIEWER_ARGS QStringList()
#else
#define DEFAULT_VIEWER "explorer"
#define DEFAULT_VIEWER_ARGS "/select,%1"
#endif
#define STATUS_TIMEOUT 5000

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QUtf8Settings settings;

    auto dateGroup = new QActionGroup(this);
    ui->actionDay->setActionGroup(dateGroup);
    ui->actionWeek->setActionGroup(dateGroup);
    if (settings.value("date-filter").toString() == "week")
    {
        ui->actionWeek->setChecked(true);
    }
    connect(dateGroup, SIGNAL(triggered(QAction*)), this, SLOT(loadList()));

    qRegisterMetaType<DcmDataset>();
    qRegisterMetaType<DcmTag>();

    settings.beginGroup("Display");

    auto keys = settings.allKeys();
    ui->results->setColumnCount(keys.count());

    for (int i = 0; i < keys.count(); ++i)
    {
        auto key = keys[i];

        DcmTag tag;
        if (DcmTag::findTagFromName(key.toUtf8(), tag).bad())
        {
            qWarning() << "Unknown tag in Display section" << key;
            continue;
        }

        auto item = ui->results->headerItem();
        auto text = settings.value(key, key).toString();
        if (text.isEmpty())
        {
            text = tag.getTagName();
        }
        item->setText(i, text);
        item->setData(i, Qt::UserRole, QVariant::fromValue(tag));
    }
    settings.endGroup();

    restoreGeometry(settings.value("mainwindow-geometry").toByteArray());
    setWindowState(Qt::WindowState(settings.value("mainwindow-state").toInt()));
}

MainWindow::~MainWindow()
{
    delete ui;
}


class FindSCUCallback : public DcmFindSCUCallback
{
    MainWindow* owner;

public:
    FindSCUCallback
        ( MainWindow* owner
        )
        : owner(owner)
    {
    }

    virtual ~FindSCUCallback()
    {
    }

    virtual void callback
        ( T_DIMSE_C_FindRQ *req
        , int responseCount
        , T_DIMSE_C_FindRSP *rsp
        , DcmDataset *ds
        )
    {
        Q_UNUSED(req);
        Q_UNUSED(responseCount);
        Q_UNUSED(rsp);
        owner->onRowFetched(ds);
    }
};

void MainWindow::loadList()
{
    QWaitCursor wait(this);
    FindSCUCallback cb (this);
    OFList<OFString> overrideKeys;
    ui->results->clear();

    auto today = QDate::currentDate();
    QString dateFilter("StudyDate=");
    if (ui->actionWeek->isChecked())
    {
        statusBar()->showMessage(tr("Loading list (this week)"), STATUS_TIMEOUT);
        dateFilter.append(today.addDays(-7).toString("yyyyMMdd")).append("-");
    }
    else
    {
        statusBar()->showMessage(tr("Loading list (today)"), STATUS_TIMEOUT);
    }
    dateFilter.append(today.toString("yyyyMMdd"));
    overrideKeys.push_back(dateFilter.toLocal8Bit().constData());

    QUtf8Settings settings;
    settings.beginGroup("Query");
    for (auto key : settings.allKeys())
    {
        overrideKeys.push_back(key.append("=")
            .append(settings.value(key).toString()).toLocal8Bit().constData());
    }
    settings.endGroup();

    auto timeout = settings.value("timeout", 0).toInt();
    auto pduSize = settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt();
    auto clientAPTitle = settings.value("client-aetitle",
        qApp->applicationName().toUpper()).toString();
    auto serverAPTitle = settings.value("server-aetitle").toString();
    auto serverAddress = settings.value("server-address").toString();
    auto serverPort    = settings.value("server-port", DEFAULT_DICOM_PORT).toUInt();
    auto transferSyntax = (E_TransferSyntax)settings.value("transfer-syntax", EXS_Unknown).toInt();
    DcmFindSCU findscu;
    auto cond = findscu.initializeNetwork(timeout);
    if (cond.good())
    {
        cond = findscu.performQuery(
            serverAddress.toLocal8Bit(),
            serverPort,
            clientAPTitle.toLocal8Bit(),
            serverAPTitle.toLocal8Bit(),
            UID_FINDStudyRootQueryRetrieveInformationModel,
            transferSyntax,
            timeout == 0? DIMSE_BLOCKING: DIMSE_NONBLOCKING,
            timeout,
            pduSize,
            false,
            false,
            1,
            FEM_none,
            0,
            &overrideKeys,
            &cb);
    }

    if (cond.bad())
    {
        QMessageBox::critical(this, windowTitle(), cond.text());
    }
    statusBar()->showMessage(tr("Done"), STATUS_TIMEOUT);
}

void MainWindow::hideEvent(QHideEvent *evt)
{
    QUtf8Settings settings;
    settings.setValue("date-filter", ui->actionDay->isChecked() ? "day" : "week");
    settings.setValue("mainwindow-geometry", saveGeometry());

    // Do not save window state, if it is in fullscreen mode or minimized
    //
    auto state = int(windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen));
    settings.setValue("mainwindow-state", state);
    QWidget::hideEvent(evt);
}


void MainWindow::showEvent(QShowEvent *e)
{
    // Refresh the list right after the window is shown
    //
    QTimer::singleShot(500, this, SLOT(loadList()));
    QWidget::showEvent(e);
}

void MainWindow::onRowFetched(DcmDataset *ds)
{
    QStringList data;
    auto header = ui->results->headerItem();
    for (int i = 0; i < header->columnCount(); ++i)
    {
        auto tag = header->data(i, Qt::UserRole).value<DcmTag>();
        const char* str;
        if (ds->findAndGetString(tag, str).good())
        {
            data << str;
        }
    }

    auto item = new QTreeWidgetItem(data);
    item->setData(0, Qt::UserRole, QVariant::fromValue(DcmDataset(*ds)));
    ui->results->invisibleRootItem()->addChild(item);
}

class GetDcmSCU : public DcmSCU
{
    MainWindow* parent;
public:
    GetDcmSCU(MainWindow* parent)
        : parent(parent)
    {
    }

protected:
    virtual void notifyRECEIVEProgress(const unsigned long byteCount)
    {
        parent->statusBar()->showMessage(parent->tr("%1 bytes received").arg(byteCount));
    }

    virtual void notifyInstanceStored
        ( const OFString &filename
        , const OFString &sopClassUID
        , const OFString &sopInstanceUID) const
    {
        Q_UNUSED(sopClassUID);
        Q_UNUSED(sopInstanceUID);

        QUtf8Settings settings;
        auto viewer       = settings.value("viewer", DEFAULT_VIEWER).toString();
        auto args         = settings.value("viewer-args", DEFAULT_VIEWER_ARGS).toStringList();
        auto tmpFile      = QString::fromLocal8Bit(filename.c_str());
        bool appendToList = true;

        auto strFile = tmpFile + ".dcm";
        if (!QFile::rename(tmpFile, strFile))
        {
            strFile = tmpFile;
        }

        for (QString& arg : args)
        {
            if (arg.contains("%1"))
            {
                arg.replace("%1", strFile);
                appendToList = false;
            }
        }

        if (appendToList)
        {
            args << strFile;
        }

        QProcess viewerProcess;
        qDebug() << "starting" << viewer << args;
        if (viewerProcess.execute(viewer, args) < 0)
        {
            QMessageBox::critical(parent, parent->windowTitle(), parent->tr(
                "Failed to execute %1\n\n%2").arg(viewer).arg(viewerProcess.errorString()));
        }
        parent->statusBar()->showMessage(parent->tr("%1 download complete").arg(strFile),
            STATUS_TIMEOUT);
    }
};

void MainWindow::onItemDoubleClicked
    ( QTreeWidgetItem *item
    , int column
    )
{
    Q_UNUSED(column);

    QWaitCursor wait(this);
    auto ds = item->data(0, Qt::UserRole).value<DcmDataset>();

    QUtf8Settings settings;
    auto timeout       = settings.value("timeout", 0).toInt();
    auto pduSize       = settings.value("pdu-size", ASC_DEFAULTMAXPDU).toInt();
    auto clientAPTitle = settings.value("client-aetitle",
        qApp->applicationName().toUpper()).toString();
    auto serverAPTitle = settings.value("server-aetitle").toString();
    auto serverAddress = settings.value("server-address").toString();
    auto serverPort    = settings.value("server-port", DEFAULT_DICOM_PORT).toUInt();
    auto folder        = settings.value("download-folder").toString();
    if (folder.isEmpty())
    {
        folder = QDir::tempPath();
    }

    statusBar()->showMessage(tr("Downloading %1 to %2").arg(item->text(0)).arg(folder));

    GetDcmSCU scu(this);
    scu.setMaxReceivePDULength(pduSize);
    scu.setACSETimeout(timeout);
    scu.setDIMSEBlockingMode(timeout == 0? DIMSE_BLOCKING: DIMSE_NONBLOCKING);
    scu.setDIMSETimeout(timeout);
    scu.setAETitle(clientAPTitle.toLocal8Bit().constData());
    scu.setPeerHostName(serverAddress.toLocal8Bit().constData());
    scu.setPeerPort(serverPort);
    scu.setPeerAETitle(serverAPTitle.toLocal8Bit().constData());
    scu.setVerbosePCMode(log4cplus::Logger::getRoot().getLogLevel() < log4cplus::WARN_LOG_LEVEL);
    OFList<OFString> syntaxes;
#if __BYTE_ORDER == __LITTLE_ENDIAN
    syntaxes.push_back(UID_LittleEndianExplicitTransferSyntax);
    syntaxes.push_back(UID_BigEndianExplicitTransferSyntax);
#else
    syntaxes.push_back(UID_BigEndianExplicitTransferSyntax);
    syntaxes.push_back(UID_LittleEndianExplicitTransferSyntax);
#endif
    syntaxes.push_back(UID_LittleEndianImplicitTransferSyntax);

    scu.addPresentationContext(UID_GETStudyRootQueryRetrieveInformationModel, syntaxes);

    // Add storage presentation contexts (long list of storage SOP classes, uncompressed.
    //
    for (Uint16 j = 0; j < numberOfDcmLongSCUStorageSOPClassUIDs; j++)
    {
      scu.addPresentationContext(dcmLongSCUStorageSOPClassUIDs[j], syntaxes, ASC_SC_ROLE_SCP);
    }

    scu.setStorageMode(DCMSCU_STORAGE_BIT_PRESERVING);
    scu.setStorageDir(folder.toLocal8Bit().constData());

    T_ASC_PresentationContextID pcid = 0;
    auto cond = scu.initNetwork();
    if (cond.good())
    {
        cond = scu.negotiateAssociation();
    }
    if (cond.good())
    {
        pcid = scu.findPresentationContextID(UID_GETStudyRootQueryRetrieveInformationModel, "");
        if (pcid == 0)
        {
            cond = makeOFCondition(0, 1, OF_error,
                tr("Accepted presentation context ID not found").toUtf8());
        }
    }
    if (cond.good())
    {
        OFList<RetrieveResponse*> responses;
        cond = scu.sendCGETRequest(pcid, &ds, &responses);
        for (auto iter = responses.begin(); iter != responses.end(); iter = responses.erase(iter))
        {
            (*iter)->print();
            delete (*iter);
        }
    }

    if (cond.bad())
    {
        QMessageBox::critical(this, windowTitle(), cond.text());
    }

    statusBar()->clearMessage();
}

void MainWindow::showAbout()
{
    auto title = tr("About %1").arg(tr(PRODUCT_FULL_NAME));

    // Product name and version.
    //
    auto str = tr("<h1>%1</h1><h2>Version %2</h2>").arg(tr(PRODUCT_FULL_NAME)).arg(PRODUCT_VERSION);

    // Third party libraries.
    //
    str.append(tr("<p>Based on:</p><ul>"))
        .append(tr("<li><a href=\"https://www.qt.io/\">QT</a> %1</li>").arg(QT_VERSION_STR))
        .append(tr("<li><a href=\"http://dcmtk.org/\">DCMTK</a> %1</li>")
            .arg(OFFIS_DCMTK_VERSION_STRING))
        .append("</ul>");

    // Copyright & warranty.
    //
    str.append(tr("<p>Copyright (C) 2018 <a href=\"%1\">%2</a>. All rights reserved.</p>")
            .arg(PRODUCT_SITE_URL, ORGANIZATION_FULL_NAME))
        .append(tr("<p>The program is provided AS IS with NO WARRANTY OF ANY KIND,<br/>"
             "INCLUDING THE WARRANTY OF DESIGN,<br/>"
             "MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.</p>"));

    QMessageBox::about(this, title, str);
}
