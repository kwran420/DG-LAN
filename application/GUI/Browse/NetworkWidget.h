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

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QSplitter>
#include <QLabel>

#include <Common/Hash.h>
#include <Common/RemoteCoreController/ICoreConnection.h>

#include <Peers/PeerListModel.h>
#include <Browse/BrowseModel.h>
#include <Settings/SharedEntryListModel.h>
#include <DownloadMenu.h>

namespace GUI
{
   class NetworkWidget : public QWidget
   {
      Q_OBJECT

   public:
      explicit NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent = nullptr);
      ~NetworkWidget();

   protected:
      void changeEvent(QEvent* event) override;
      void keyPressEvent(QKeyEvent* event) override;

   private slots:
      void peerSelected(const QModelIndex& current, const QModelIndex& previous);
      void displayContextMenuDownload(const QPoint& point);
      void entryDoubleClicked(const QModelIndex& index);
      void download();
      void downloadTo();
      void downloadTo(const QString& path, const Common::Hash& sharedDirID = Common::Hash());

   private:
      void openFile(const QModelIndex& index) const;

      QSharedPointer<RCC::ICoreConnection> coreConnection;
      PeerListModel& peerListModel;
      const SharedEntryListModel& sharedEntryListModel;

      QSplitter* splitter;
      QListView* peerListView;
      QTreeView* browseTreeView;
      QLabel* placeholder;
      DownloadMenu downloadMenu;

      BrowseModel* currentBrowseModel;
      Common::Hash currentPeerID;
   };
}
