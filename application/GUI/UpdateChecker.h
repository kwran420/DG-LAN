/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

namespace GUI
{
   /**
    * Checks GitHub Releases API for a newer version of DG-LAN.
    *
    * Usage:
    *   auto* uc = new UpdateChecker(this);
    *   connect(uc, &UpdateChecker::updateAvailable, ...);
    *   connect(uc, &UpdateChecker::upToDate,        ...);
    *   connect(uc, &UpdateChecker::checkFailed,     ...);
    *   uc->check();
    */
   class UpdateChecker : public QObject
   {
      Q_OBJECT
   public:
      explicit UpdateChecker(QObject* parent = nullptr);

      /** Start an async version check against GitHub. Safe to call multiple times. */
      void check();

      /** Returns true if auto-check on launch is enabled in user settings. */
      static bool isAutoCheckEnabled();

      /** Enable or disable the auto-check on launch setting. */
      static void setAutoCheckEnabled(bool enabled);

   signals:
      /** Emitted when GitHub reports a newer version.
       *  @param downloadUrl  Direct URL to the installer asset (may be empty if no assets). */
      void updateAvailable(QString latestVersion, QString releaseUrl, QString downloadUrl);

      /** Emitted when already on the latest version. */
      void upToDate(QString currentVersion);

      /** Emitted when the check could not be completed (network error, etc.). */
      void checkFailed(QString errorMessage);

   private slots:
      void onReplyFinished(QNetworkReply* reply);

   private:
      QNetworkAccessManager* nam;
   };
}
