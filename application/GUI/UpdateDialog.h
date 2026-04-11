/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>

namespace GUI
{
   /**
    * Modal dialog shown when an update is available.
    * If a direct installer download URL is available it will download the
    * installer inline and launch it via ShellExecute, then quit the app.
    * Falls back to opening the GitHub release page in the browser.
    */
   class UpdateDialog : public QDialog
   {
      Q_OBJECT
   public:
      explicit UpdateDialog(const QString& latestVersion,
                            const QString& releaseUrl,
                            const QString& downloadUrl,
                            QWidget* parent = nullptr,
                            bool forced = false);

   protected:
      void paintEvent(QPaintEvent* event) override;

   private slots:
      void startDownload();
      void onProgress(qint64 received, qint64 total);
      void onDownloadDone();

   private:
      QNetworkAccessManager* m_dlNam;
      QNetworkReply*         m_reply   = nullptr;
      QProgressBar*          m_progress;
      QLabel*                m_status;
      QPushButton*           m_btnAction;
      QPushButton*           m_btnLater;
      QString                m_downloadUrl;
      QString                m_releaseUrl;
      QString                m_tempFile;
   };

   /**
    * Small dialog shown when the user manually clicks "Check for Updates"
    * and they are already on the latest version.
    */
   class UpToDateDialog : public QDialog
   {
      Q_OBJECT
   public:
      explicit UpToDateDialog(const QString& currentVersion,
                              QWidget* parent = nullptr);
   };
}
