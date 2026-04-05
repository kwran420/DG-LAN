/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#pragma once

#include <QDialog>

namespace GUI
{
   class WelcomeDialog : public QDialog
   {
      Q_OBJECT
   public:
      explicit WelcomeDialog(QWidget* parent = nullptr);

      static bool shouldShow();
      static void markShown();

   protected:
      void paintEvent(QPaintEvent* event) override;
   };
}
