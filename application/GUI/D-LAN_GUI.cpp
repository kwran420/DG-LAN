/**
  * D-LAN - A decentralized LAN file sharing software.
  * Copyright (C) 2010-2012 Greg Burri <greg.burri@gmail.com>
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */
  
#include <D-LAN_GUI.h>
using namespace GUI;

#include <QMessageBox>
#include <QPushButton>
#include <QLocalSocket>
#include <QUrlQuery>
#include <QTimer>

#include <Common/LogManager/Builder.h>
#include <Common/Constants.h>
#include <Common/Settings.h>
#include <Common/Languages.h>
#include <Common/Hash.h>

#include <Common/RemoteCoreController/Builder.h>

#include <Protos/common.pb.h>

#include <Log.h>
#include <WelcomeDialog.h>
#include <UpdateChecker.h>
#include <UpdateDialog.h>

const QString D_LAN_GUI::SHARED_MEMORY_KEYNAME("DG-LAN GUI instance");
static const QString IPC_SERVER_NAME("DG-LAN-GUI-IPC");

/**
  * @class GUI::D_LAN_GUI
  * This class control the trayIcon and create the main window.
  * The main window can be hid and deleted, the tray icon will still remain and will permit to relaunch the main window.
  */

D_LAN_GUI::D_LAN_GUI(int& argc, char* argv[]) :
   QApplication(argc, argv),
   mainWindow(0),
   trayIcon(QIcon(":/icons/ressources/icon.png")),
   coreConnection(RCC::Builder::newCoreConnection(SETTINGS.get<quint32>("socket_timeout"))),
   updateChecker(new UpdateChecker(this))
{
   this->installTranslator(&this->translator);
   QLocale current = QLocale::system();
   if (SETTINGS.isSet("language"))
      current = SETTINGS.get<QLocale>("language");
   Common::Languages langs(QCoreApplication::applicationDirPath() + "/" + Common::Constants::LANGUAGE_DIRECTORY);
   this->loadLanguage(langs.getBestMatchLanguage(Common::Languages::ExeType::GUI, current).filename);

   // Collect any dglan:// URL passed on the command line.
   QString startupUrl;
   for (int i = 1; i < argc; ++i)
   {
      const QString arg = QString::fromLocal8Bit(argv[i]);
      if (arg.startsWith("dglan://", Qt::CaseInsensitive))
         startupUrl = arg;
   }

   // Single-instance check via QLocalServer.
   // If another instance is running, forward any URL to it and exit.
   if (!SETTINGS.get<bool>("multiple_instance_allowed"))
   {
      // Try connecting to an already-running instance.
      QLocalSocket probe;
      probe.connectToServer(IPC_SERVER_NAME);
      if (probe.waitForConnected(500))
      {
         // Another instance is alive. Forward URL (or a bare ping) and exit.
         const QByteArray payload = startupUrl.isEmpty() ? QByteArray("show") : startupUrl.toUtf8();
         probe.write(payload + '\n');
         probe.flush();
         probe.waitForBytesWritten(500);
         probe.disconnectFromServer();
         throw AbortException();
      }
   }

   // Start our own IPC server so future instances can talk to us.
   QLocalServer::removeServer(IPC_SERVER_NAME); // Remove stale socket from a previous crash.
   this->ipcServer.listen(IPC_SERVER_NAME);
   connect(&this->ipcServer, SIGNAL(newConnection()), this, SLOT(ipcNewConnection()));

   // Keep QSharedMemory for backward compat / Linux crash detection.
#ifndef Q_OS_LINUX
   if (!SETTINGS.get<bool>("multiple_instance_allowed"))
   {
      this->sharedMemory.lock();
      this->sharedMemory.setKey(SHARED_MEMORY_KEYNAME);
      this->sharedMemory.create(1); // Best-effort; don't block on failure here.
      this->sharedMemory.unlock();
   }
#endif

   this->setQuitOnLastWindowClosed(false);

   this->showMainWindow();

   RCC::ICoreConnection* coreConnectionPointer = this->coreConnection.data();
   connect(coreConnectionPointer, SIGNAL(localCoreStatusChanged()), this, SLOT(updateTrayIconMenu()));
   connect(coreConnectionPointer, SIGNAL(connected()), this, SLOT(updateTrayIconMenu()));
   connect(coreConnectionPointer, SIGNAL(connected()), this, SLOT(coreReadyForDownloads()));
   connect(coreConnectionPointer, SIGNAL(disconnected(bool)), this, SLOT(updateTrayIconMenu()));

   connect(&this->trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

   // Wire update checker signals.
   connect(this->updateChecker, &UpdateChecker::updateAvailable,
           this, &D_LAN_GUI::onUpdateAvailable);
   connect(this->updateChecker, &UpdateChecker::upToDate,
           this, &D_LAN_GUI::onUpToDate);
   connect(this->updateChecker, &UpdateChecker::checkFailed,
           this, &D_LAN_GUI::onUpdateCheckFailed);

   // Auto check on launch (delayed so the window is up first).
   if (UpdateChecker::isAutoCheckEnabled())
      QTimer::singleShot(3000, this->updateChecker, &UpdateChecker::check);

   this->updateTrayIconMenu();

   this->trayIcon.setContextMenu(&this->trayIconMenu);
   this->trayIcon.setToolTip("DG-LAN");
   this->trayIcon.show();

   // Queue any URL we received at startup.
   if (!startupUrl.isEmpty())
      this->pendingUrls << startupUrl;
}

bool D_LAN_GUI::event(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
      this->updateTrayIconMenu();

   return QApplication::event(event);
}

void D_LAN_GUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
   if (reason == QSystemTrayIcon::Trigger)
      this->showMainWindow();
}

void D_LAN_GUI::updateTrayIconMenu()
{
   this->trayIconMenu.clear();
   this->trayIconMenu.addAction(tr("Show DG-LAN"), this, SLOT(showMainWindow()));
   this->trayIconMenu.addAction(tr("Check for Updates..."), this, SLOT(checkForUpdates()));
   if (this->coreConnection->getLocalCoreStatus() == RCC::RUNNING_AS_SERVICE) // We cannot stop a parent process without killing his child (case with RCC::RUNNING_AS_SUB_PROCESS).
      this->trayIconMenu.addAction(tr("Stop the user interface"), this, SLOT(exitGUI()));
   this->trayIconMenu.addSeparator();
   this->trayIconMenu.addAction(tr("Exit"), this, SLOT(exit()));
}

/**
  * Load a translation file. If 'filename' is empty the default language is loaded.
  */
void D_LAN_GUI::loadLanguage(const QString& filename)
{
   this->translator.load(filename, QCoreApplication::applicationDirPath() + "/" + Common::Constants::LANGUAGE_DIRECTORY);
}

void D_LAN_GUI::mainWindowClosed()
{
   if (this->coreConnection->isConnected())
      this->trayIcon.showMessage("DG-LAN", "DG-LAN Core is still running in the background. Select 'Exit' from the tray menu to stop it.");
   this->coreConnection->disconnectFromCore();
   this->mainWindow = nullptr;
}

void D_LAN_GUI::showMainWindow()
{
   if (this->mainWindow)
   {
      this->mainWindow->setWindowState(Qt::WindowActive);
      this->mainWindow->raise();
      this->mainWindow->activateWindow();
   }
   else
   {
      this->mainWindow = new MainWindow(this->coreConnection);
      connect(this->mainWindow, SIGNAL(languageChanged(QString)), this, SLOT(loadLanguage(QString)));
      connect(this->mainWindow, SIGNAL(destroyed()), this, SLOT(mainWindowClosed()));
      connect(this->mainWindow, &MainWindow::checkForUpdatesRequested, this, &D_LAN_GUI::checkForUpdates);
      this->mainWindow->show();

      // Show the welcome dialog on the very first ever run.
      if (WelcomeDialog::shouldShow())
      {
         WelcomeDialog::markShown();
         QTimer::singleShot(200, this, [this]() {
            WelcomeDialog dlg(this->mainWindow);
            dlg.exec();
         });
      }
   }
}

/**
  * Stop only the GUI.
  */
void D_LAN_GUI::exitGUI()
{
   this->exit(false);
}

void D_LAN_GUI::exit(bool stopTheCore)
{
   this->trayIcon.hide();

   if (stopTheCore)
      this->coreConnection->stopLocalCore();

   if (this->mainWindow)
   {
      disconnect(this->mainWindow, SIGNAL(destroyed()), this, SLOT(mainWindowClosed()));
      delete this->mainWindow;
   }

   this->quit();
}

// ---------------------------------------------------------------------------
// Update checker slots
// ---------------------------------------------------------------------------

void D_LAN_GUI::checkForUpdates()
{
   this->manualUpdateCheck = true;
   this->updateChecker->check();
}

void D_LAN_GUI::onUpdateAvailable(QString latestVersion, QString releaseUrl, QString downloadUrl)
{
   if (this->mainWindow)
   {
      UpdateDialog dlg(latestVersion, releaseUrl, downloadUrl, this->mainWindow);
      dlg.exec();
   }
   else
   {
      // No window open — show a tray notification.
      this->trayIcon.showMessage(
         "DG-LAN Update Available",
         QString("Version %1 is ready. Right-click the tray icon → Check for Updates to download.")
            .arg(latestVersion),
         QSystemTrayIcon::Information, 8000);
   }
   this->manualUpdateCheck = false;
}

void D_LAN_GUI::onUpToDate(QString currentVersion)
{
   // Only pop a dialog for manual checks — silent on auto-launch checks.
   if (this->manualUpdateCheck)
   {
      QWidget* parent = this->mainWindow ? this->mainWindow : nullptr;
      UpToDateDialog dlg(currentVersion, parent);
      dlg.exec();
   }
   this->manualUpdateCheck = false;
}

void D_LAN_GUI::onUpdateCheckFailed(QString error)
{
   if (this->manualUpdateCheck)
   {
      this->trayIcon.showMessage(
         "DG-LAN — Update Check Failed",
         QString("Could not reach GitHub: %1").arg(error),
         QSystemTrayIcon::Warning, 5000);
   }
   this->manualUpdateCheck = false;
}

// ---------------------------------------------------------------------------
// DG-LAN URL scheme  (dglan://download?peer=HEX&hash=HEX&size=N&name=FILE&path=/)
// ---------------------------------------------------------------------------

/**
 * Called when the core signals it's connected and ready.
 * Drains any URLs that arrived before the connection was up.
 */
void D_LAN_GUI::coreReadyForDownloads()
{
   for (const QString& url : this->pendingUrls)
      this->handleUrl(QUrl(url));
   this->pendingUrls.clear();
}

/**
 * Receives a line from a second instance over the local IPC socket.
 * The line is either "show" or a dglan:// URL.
 */
void D_LAN_GUI::ipcNewConnection()
{
   QLocalSocket* socket = this->ipcServer.nextPendingConnection();
   if (!socket)
      return;

   // Read a full line (the second instance writes URL + '\n').
   connect(socket, &QLocalSocket::readyRead, this, [this, socket]() {
      while (socket->canReadLine())
      {
         const QString line = QString::fromUtf8(socket->readLine()).trimmed();
         if (line == "show")
            this->showMainWindow();
         else if (line.startsWith("dglan://", Qt::CaseInsensitive))
         {
            this->showMainWindow();
            if (this->coreConnection->isConnected())
               this->handleUrl(QUrl(line));
            else
               this->pendingUrls << line;
         }
      }
      socket->deleteLater();
   });
}

/**
 * Parse a dglan:// URL and queue the download.
 *
 * URL format:
 *   dglan://download?peer=<peerIDhex>&hash=<sharedEntryIDhex>&size=<bytes>&name=<filename>&path=<rel-path>
 *
 * - peer  : hex-encoded peer ID (use Hash::toStr() / Hash::fromStr())
 * - hash  : hex-encoded shared entry ID that identifies the file on the remote peer
 * - size  : file size in bytes
 * - name  : display name / filename
 * - path  : relative path within the shared entry (default "/")
 *
 * Generating a link from a website:
 *   When the user browses peer files via a web UI you already know the peer ID and
 *   entry hash (from the Search/Browse result), so you can simply build the URL:
 *     dglan://download?peer=<Hash.toStr()>&hash=<sharedHash.toStr()>&size=<N>&name=<fname>&path=/
 */
void D_LAN_GUI::handleUrl(const QUrl& url)
{
   if (url.scheme().compare("dglan", Qt::CaseInsensitive) != 0 ||
       url.host().compare("download", Qt::CaseInsensitive) != 0)
      return;

   QUrlQuery q(url);
   const QString peerHex = q.queryItemValue("peer");
   const QString hashHex = q.queryItemValue("hash");
   const quint64 size    = q.queryItemValue("size").toULongLong();
   const QString name    = QUrl::fromPercentEncoding(q.queryItemValue("name").toUtf8());
   const QString path    = QUrl::fromPercentEncoding(q.queryItemValue("path").toUtf8());

   if (peerHex.isEmpty() || hashHex.isEmpty() || name.isEmpty())
   {
      qWarning() << "DG-LAN: handleUrl — missing required fields in URL:" << url.toString();
      return;
   }

   const Common::Hash peerID    = Common::Hash::fromStr(peerHex);
   const Common::Hash entryHash = Common::Hash::fromStr(hashHex);

   if (peerID.isNull() || entryHash.isNull())
   {
      qWarning() << "DG-LAN: handleUrl — invalid peer or hash in URL:" << url.toString();
      return;
   }

   // Build the Protos::Common::Entry.
   Protos::Common::Entry entry;
   entry.set_type(Protos::Common::Entry::FILE);
   entry.set_name(name.toStdString());
   entry.set_size(size);
   entry.set_path(path.isEmpty() ? "/" : path.toStdString());
   entry.mutable_shared_entry()->mutable_id()->set_hash(entryHash.getData(), Common::Hash::HASH_SIZE);

   qDebug() << "DG-LAN: URL download — peer=" << peerHex << "name=" << name << "size=" << size;
   this->coreConnection->download(peerID, entry);
}
