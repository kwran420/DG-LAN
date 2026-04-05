/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#pragma once

#include <QDialog>

namespace GUI
{
   /**
    * Modal dialog shown when an update is available.
    * Displays the new version tag and a "Download" button that
    * opens the GitHub releases page in the default browser.
    */
   class UpdateDialog : public QDialog
   {
      Q_OBJECT
   public:
      /**
       * @param latestVersion  Version tag from GitHub, e.g. "v1.3.0"
       * @param releaseUrl     URL to the release page on GitHub
       * @param silent         If true, show a tray notification instead of
       *                       opening the dialog (for background checks).
       */
      explicit UpdateDialog(const QString& latestVersion,
                            const QString& releaseUrl,
                            QWidget* parent = nullptr);

   protected:
      void paintEvent(QPaintEvent* event) override;
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
