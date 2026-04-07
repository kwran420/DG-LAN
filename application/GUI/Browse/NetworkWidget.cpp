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

#include <Browse/NetworkWidget.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QIcon>
#include <QKeyEvent>

#include <Common/ProtoHelper.h>

#include <Log.h>
#include <Utils.h>

NetworkWidget::NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent) :
   QWidget(parent),
   coreConnection(coreConnection),
   peerListModel(peerListModel),
   sharedEntryListModel(sharedEntryListModel),
   downloadMenu(sharedEntryListModel),
   currentBrowseModel(nullptr)
{
   QVBoxLayout* mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(0, 0, 0, 0);

   this->splitter = new QSplitter(Qt::Horizontal, this);
   mainLayout->addWidget(this->splitter);

   this->peerListView = new QListView();
   this->peerListView->setModel(&this->peerListModel);
   this->peerListView->setMaximumWidth(200);
   this->splitter->addWidget(this->peerListView);

   this->browseTreeView = new QTreeView();
   this->browseTreeView->header()->setVisible(false);
   this->browseTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
   this->browseTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
   this->browseTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
   this->browseTreeView->setAlternatingRowColors(true);
   this->splitter->addWidget(this->browseTreeView);

   this->placeholder = new QLabel(tr("Select a peer to browse their files"));
   this->placeholder->setAlignment(Qt::AlignCenter);
   this->splitter->addWidget(this->placeholder);
   this->browseTreeView->hide();

   this->splitter->setStretchFactor(0, 0);
   this->splitter->setStretchFactor(1, 1);
   this->splitter->setStretchFactor(2, 1);

   connect(this->peerListView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(peerSelected(const QModelIndex&, const QModelIndex&)));
   connect(this->browseTreeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(displayContextMenuDownload(const QPoint&)));
   connect(this->browseTreeView, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(entryDoubleClicked(const QModelIndex&)));

   connect(&this->downloadMenu, SIGNAL(download()), this, SLOT(download()));
   connect(&this->downloadMenu, SIGNAL(downloadTo()), this, SLOT(downloadTo()));
   connect(&this->downloadMenu, SIGNAL(downloadTo(const QString&, const Common::Hash&)), this, SLOT(downloadTo(const QString&, const Common::Hash&)));

   this->setWindowTitle(tr("Network"));
}

NetworkWidget::~NetworkWidget()
{
   delete this->currentBrowseModel;
}

void NetworkWidget::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
      this->setWindowTitle(tr("Network"));
   QWidget::changeEvent(event);
}

void NetworkWidget::keyPressEvent(QKeyEvent* event)
{
   if (event->key() == Qt::Key_Return)
   {
      const QModelIndexList& selectedRows = this->browseTreeView->selectionModel()->selectedRows();
      for (QListIterator<QModelIndex> i(selectedRows); i.hasNext();)
         this->openFile(i.next());
   }
   else
      QWidget::keyPressEvent(event);
}

void NetworkWidget::peerSelected(const QModelIndex& current, const QModelIndex& /*previous*/)
{
   if (!current.isValid())
      return;

   Common::Hash peerID = this->peerListModel.getPeerID(current.row());
   if (peerID == this->currentPeerID && this->currentBrowseModel)
      return;

   delete this->currentBrowseModel;
   this->currentBrowseModel = new BrowseModel(this->coreConnection, this->sharedEntryListModel, peerID);
   this->currentPeerID = peerID;

   this->browseTreeView->setModel(this->currentBrowseModel);
   this->browseTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
   this->browseTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   this->placeholder->hide();
   this->browseTreeView->show();
}

void NetworkWidget::displayContextMenuDownload(const QPoint& point)
{
   if (!this->currentBrowseModel || this->currentPeerID.isNull())
      return;

   QPoint globalPosition = this->browseTreeView->mapToGlobal(point);
   if (this->coreConnection->getRemoteID() == this->currentPeerID)
   {
      if (this->coreConnection->isLocal())
      {
         QMenu menu;
         menu.addAction(QIcon(":/icons/ressources/explore_folder.png"), tr("Open location"), [this]() {
            QModelIndexList selectedRows = this->browseTreeView->selectionModel()->selectedRows();
            QSet<QString> locations;
            for (QListIterator<QModelIndex> i(selectedRows); i.hasNext();)
               locations.insert(this->currentBrowseModel->getPath(i.next(), true));
            Utils::openLocations(locations.values());
         });
         menu.exec(globalPosition);
      }
   }
   else
   {
      this->downloadMenu.show(globalPosition);
   }
}

void NetworkWidget::entryDoubleClicked(const QModelIndex& index)
{
   this->openFile(index);
}

void NetworkWidget::download()
{
   if (!this->currentBrowseModel || this->currentPeerID.isNull())
      return;

   if (this->currentBrowseModel->nbSharedDirs() == 0)
   {
      QStringList dirs = Utils::askForDirectoriesToDownloadTo(this->coreConnection);
      if (!dirs.isEmpty())
         this->downloadTo(dirs.first(), Common::Hash());
      return;
   }

   QModelIndexList selectedRows = this->browseTreeView->selectionModel()->selectedRows();
   for (QListIterator<QModelIndex> i(selectedRows); i.hasNext();)
      this->coreConnection->download(this->currentPeerID, this->currentBrowseModel->getEntry(i.next()));
}

void NetworkWidget::downloadTo()
{
   QStringList dirs = Utils::askForDirectoriesToDownloadTo(this->coreConnection);
   if (!dirs.isEmpty())
      this->downloadTo(dirs.first());
}

void NetworkWidget::downloadTo(const QString& path, const Common::Hash& sharedDirID)
{
   if (!this->currentBrowseModel || this->currentPeerID.isNull())
      return;

   QModelIndexList selectedRows = this->browseTreeView->selectionModel()->selectedRows();
   for (QListIterator<QModelIndex> i(selectedRows); i.hasNext();)
      this->coreConnection->download(this->currentPeerID, this->currentBrowseModel->getEntry(i.next()), sharedDirID, path);
}

void NetworkWidget::openFile(const QModelIndex& index) const
{
   if (!this->currentBrowseModel)
      return;
   if (this->coreConnection->getRemoteID() == this->currentPeerID && !this->currentBrowseModel->isDir(index))
      Utils::openFile(this->currentBrowseModel->getPath(index));
}
