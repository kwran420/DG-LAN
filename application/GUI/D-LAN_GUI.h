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
  
#pragma once

#include <QApplication>
#include <QTranslator>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QSharedMemory>
#include <QLocalServer>
#include <QStringList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>

#include <MainWindow.h>
#include <UpdateChecker.h>

#include <Common/RemoteCoreController/Types.h>

namespace GUI
{
   class D_LAN_GUI : public QApplication
   {
      static const QString SHARED_MEMORY_KEYNAME;

      Q_OBJECT
   public:
      class AbortException {};

      D_LAN_GUI(int& argc, char* argv[]);

   protected:
      bool event(QEvent* event);

   private slots:
      void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
      void updateTrayIconMenu();
      void loadLanguage(const QString& filename);
      void mainWindowClosed();
      void showMainWindow();
      void exitGUI();
      void exit(bool stopTheCore = true);

      void checkForUpdates();   // manual tray menu action
      void checkForUpdatesAutomatically();

      // UpdateChecker result slots
      void onUpdateAvailable(QString latestVersion, QString releaseUrl, QString downloadUrl);
      void onUpToDate(QString currentVersion);
      void onUpdateCheckFailed(QString error);
      void onAutoUpdateDownloadFinished();

      // DG-LAN URL scheme (dglan://).
      void handleUrl(const QUrl& url);
      void ipcNewConnection();  // QLocalServer slot — forwards URL from second instance
      void coreReadyForDownloads(); // Drain pending URL queue once connected

   private:
      void startAutoUpdate(const QString& latestVersion, const QString& releaseUrl, const QString& downloadUrl);
      bool launchInstaller(const QString& installerPath);

      QSharedMemory sharedMemory;

      MainWindow* mainWindow;

      QSystemTrayIcon trayIcon;
      QMenu trayIconMenu;

      QSharedPointer<RCC::ICoreConnection> coreConnection;

      QTranslator translator;

      // DG-LAN single-instance IPC server.
      QLocalServer ipcServer;

      // URLs received before the core is connected — drained in coreReadyForDownloads().
      QStringList pendingUrls;

      // Update checker (lives for the lifetime of the app).
      UpdateChecker* updateChecker;

      // True when a manual "Check for Updates" is in flight (show "up to date" dialog).
      bool manualUpdateCheck = false;

      QNetworkAccessManager autoUpdateNam;
      QNetworkReply* autoUpdateReply = nullptr;
      QTimer autoUpdateTimer;
      QString autoUpdateReleaseUrl;
      QString autoUpdateTempFile;
      bool autoUpdateInProgress = false;
   };
}
