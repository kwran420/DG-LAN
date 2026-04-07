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

#include <QMdiArea>
#include <QSharedPointer>

#include <Common/RemoteCoreController/ICoreConnection.h>

#include <BusyIndicator.h>

#include <Peers/PeerListModel.h>
#include <Settings/SharedEntryListModel.h>
#include <Downloads/DownloadsWidget.h>
#include <Uploads/UploadsWidget.h>
#include <Browse/BrowseWidget.h>
#include <Browse/NetworkWidget.h>
#include <Search/SearchWidget.h>
#include <Taskbar/Taskbar.h>

namespace GUI
{
   class MdiArea : public QMdiArea
   {
      Q_OBJECT
   public:
      explicit MdiArea(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, SharedEntryListModel& sharedEntryListModel, Taskbar taskbar, QWidget* parent = 0);
      ~MdiArea();

      void focusNthWindow(int num);
      void closeCurrentWindow();

   public slots:
      void openBrowseWindow(const Common::Hash& peerID);
      void openSearchWindow(const Protos::Common::FindPattern& findPattern, bool local = false);

      void showDownloads();
      void showUploads();

   signals:

   protected:
      void changeEvent(QEvent* event);
      bool eventFilter(QObject* obj, QEvent* event);

   private slots:
      void newState(const Protos::GUI::State& state);
      void coreConnected();
      void coreDisconnected(bool forced);

      void tabMoved(int from, int to);
      void subWindowActivated(QMdiSubWindow* mdiWindow);

      void removeWidget(QWidget* widget);

      void onGlobalProgressChanged(quint64 completed, quint64 total);

   private:
      QString getBusyIndicatorToolTip() const;

      void addDownloadsWindow();
      void removeDownloadsWindow();

      void addUploadsWindow();
      void removeUploadsWindow();

      BrowseWidget* addBrowseWindow(const Common::Hash& peerID);

   private slots:
      BrowseWidget* addBrowseWindow(const Common::Hash& peerID, const Protos::Common::Entry& remoteEntry);

   private:
      SearchWidget* addSearchWindow(const Protos::Common::FindPattern& findPattern, bool local = false);

      void removeAllWindows();

      QSharedPointer<RCC::ICoreConnection> coreConnection;
      PeerListModel& peerListModel;
      Taskbar taskbar;

      QTabBar* mdiAreaTabBar;

      // Permanent windows.
      DownloadsWidget*       downloadsWidget;
      UploadsWidget*         uploadsWidget;
      NetworkWidget*         networkWidget;

      QList<BrowseWidget*> browseWidgets;
      QList<SearchWidget*> searchWidgets;

      // This widget is shown on the tab of the downloads page. It is visible only after D-LAN has started and during the loading
      // of the cache (before the downloads are loaded).
      // This widget is owned by the tab bar of the 'QMdiArea'.
      BusyIndicator* downloadsBusyIndicator;

      SharedEntryListModel& sharedEntryListModel;
   };
}
