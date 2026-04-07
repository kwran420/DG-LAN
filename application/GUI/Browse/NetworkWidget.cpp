#include <Browse/NetworkWidget.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QKeyEvent>
#include <QApplication>
#include <QByteArray>

#include <Common/ProtoHelper.h>
#include <Common/Global.h>

#include <Log.h>
#include <Utils.h>

QVariant PeerSpeedProxy::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
   {
      if (section == 1) return QString("Peer");
      if (section == 2) return QString("Speed");
   }
   return QIdentityProxyModel::headerData(section, orientation, role);
}

QVariant PeerSpeedProxy::data(const QModelIndex& proxyIndex, int role) const
{
   if (proxyIndex.column() == 2)
   {
      if (role == Qt::DisplayRole)
      {
         QModelIndex col0 = index(proxyIndex.row(), 0, proxyIndex.parent());
         auto ti = QIdentityProxyModel::data(col0, Qt::DisplayRole).value<PeerListModel::TransferInformation>();
         if (ti.lanSpeed > 0)
         {
            quint64 bps = static_cast<quint64>(ti.lanSpeed) * 8;
            if (bps >= 1000000000ULL)
               return QString(QString::number(bps / 1000000000.0, 'f', 1) + " Gbps");
            if (bps >= 1000000ULL)
               return QString(QString::number(bps / 1000000.0, 'f', 0) + " Mbps");
            if (bps >= 1000ULL)
               return QString(QString::number(bps / 1000.0, 'f', 0) + " Kbps");
            return QString(QString::number(bps) + " bps");
         }
         return QString();
      }
      if (role == Qt::TextAlignmentRole)
         return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
   }
   return QIdentityProxyModel::data(proxyIndex, role);
}

NetworkWidget::NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent) :
   QWidget(parent),
   coreConnection(coreConnection),
   peerListModel(peerListModel),
   sharedEntryListModel(sharedEntryListModel),
   downloadMenu(sharedEntryListModel)
{
   QVBoxLayout* mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(0, 0, 0, 0);

   this->splitter = new QSplitter(Qt::Horizontal, this);
   mainLayout->addWidget(this->splitter);

   // Left panel: peer list with names and LAN speed
   this->peerTableView = new QTableView();
   this->peerProxy.setSourceModel(&this->peerListModel);
   this->peerTableView->setModel(&this->peerProxy);
   this->peerTableView->setColumnHidden(0, true);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   this->peerTableView->horizontalHeader()->setHighlightSections(false);
   this->peerTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   this->peerTableView->verticalHeader()->setDefaultSectionSize(QApplication::fontMetrics().height() + 4);
   this->peerTableView->verticalHeader()->setVisible(false);
   this->peerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
   this->peerTableView->setShowGrid(false);
   this->peerTableView->setAlternatingRowColors(false);
   this->peerTableView->setMaximumWidth(300);
   this->splitter->addWidget(this->peerTableView);

   // Right panel: flat file list (database of all files across peers)
   this->fileTableView = new QTableView();
   this->fileTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
   this->fileTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
   this->fileTableView->setContextMenuPolicy(Qt::CustomContextMenu);
   this->fileTableView->setAlternatingRowColors(true);
   this->fileTableView->setShowGrid(false);
   this->fileTableView->verticalHeader()->setVisible(false);
   this->fileTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   this->fileTableView->verticalHeader()->setDefaultSectionSize(QApplication::fontMetrics().height() + 4);
   this->fileTableView->setSortingEnabled(true);

   this->fileModel.setHorizontalHeaderLabels({ tr("Name"), tr("Size"), tr("Peers") });
   this->fileSortProxy.setSourceModel(&this->fileModel);
   this->fileTableView->setModel(&this->fileSortProxy);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   this->fileTableView->sortByColumn(0, Qt::AscendingOrder);
   this->splitter->addWidget(this->fileTableView);

   this->splitter->setStretchFactor(0, 0);
   this->splitter->setStretchFactor(1, 1);

   connect(this->fileTableView, &QTableView::customContextMenuRequested, this, &NetworkWidget::displayContextMenuDownload);
   connect(&this->downloadMenu, SIGNAL(download()), this, SLOT(download()));
   connect(&this->downloadMenu, SIGNAL(downloadTo()), this, SLOT(downloadTo()));
   connect(&this->downloadMenu, SIGNAL(downloadTo(const QString&, const Common::Hash&)), this, SLOT(downloadTo(const QString&, const Common::Hash&)));

   // Auto-browse peers when the peer list changes
   connect(&this->peerListModel, &QAbstractItemModel::layoutChanged, this, &NetworkWidget::refreshPeers);
   connect(&this->peerListModel, &QAbstractItemModel::rowsInserted, this, &NetworkWidget::refreshPeers);
   connect(this->coreConnection.data(), SIGNAL(disconnected(bool)), this, SLOT(coreDisconnected()));

   this->setWindowTitle(tr("Network"));
}

void NetworkWidget::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
      this->setWindowTitle(tr("Network"));
   QWidget::changeEvent(event);
}

void NetworkWidget::keyPressEvent(QKeyEvent* event)
{
   if (event->key() != Qt::Key_Return)
   {
      QWidget::keyPressEvent(event);
      return;
   }
   this->download();
}

void NetworkWidget::refreshPeers()
{
   const int count = this->peerListModel.rowCount();
   for (int i = 0; i < count; ++i)
   {
      const Common::Hash peerID = this->peerListModel.getPeerID(i);
      if (peerID.isNull())
         continue;
      if (this->browsedPeers.contains(peerID))
         continue;
      this->browsePeer(peerID);
   }
}

void NetworkWidget::coreDisconnected()
{
   this->activeBrowseResults.clear();
   this->browsedPeers.clear();
   this->fileModel.removeRows(0, this->fileModel.rowCount());
}

void NetworkWidget::browsePeer(const Common::Hash& peerID)
{
   this->browsedPeers.insert(peerID);
   auto browseResult = this->coreConnection->browse(peerID);

   QByteArray idData(peerID.getData(), Common::Hash::HASH_SIZE);
   browseResult->setProperty("peerID", idData);

   connect(browseResult.data(), SIGNAL(result(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)),
           this, SLOT(browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)));
   connect(browseResult.data(), &RCC::IBrowseResult::timeout, [this, browseResult]() {
      this->activeBrowseResults.removeOne(browseResult);
   });
   browseResult->start();
   this->activeBrowseResults.append(browseResult);
}

void NetworkWidget::browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries)
{
   auto* senderObj = sender();
   QByteArray idData = senderObj->property("peerID").toByteArray();
   Common::Hash peerID(idData.constData());

   for (int i = 0; i < this->activeBrowseResults.size(); ++i)
   {
      if (this->activeBrowseResults[i].data() == senderObj)
      {
         this->activeBrowseResults.removeAt(i);
         break;
      }
   }

   for (int i = 0; i < entries.size(); ++i)
   {
      const auto& entryList = entries.Get(i);
      for (int j = 0; j < entryList.entry_size(); ++j)
      {
         const auto& entry = entryList.entry(j);
         if (entry.type() == Protos::Common::Entry::DIR)
            this->browseDir(peerID, entry);
         else
            this->addFileEntry(entry, peerID);
      }
   }
}

void NetworkWidget::browseDir(const Common::Hash& peerID, const Protos::Common::Entry& dirEntry)
{
   auto browseResult = this->coreConnection->browse(peerID, dirEntry);

   QByteArray idData(peerID.getData(), Common::Hash::HASH_SIZE);
   browseResult->setProperty("peerID", idData);

   connect(browseResult.data(), SIGNAL(result(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)),
           this, SLOT(browseSubResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)));
   connect(browseResult.data(), &RCC::IBrowseResult::timeout, [this, browseResult]() {
      this->activeBrowseResults.removeOne(browseResult);
   });
   browseResult->start();
   this->activeBrowseResults.append(browseResult);
}

void NetworkWidget::browseSubResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries)
{
   auto* senderObj = sender();
   QByteArray idData = senderObj->property("peerID").toByteArray();
   Common::Hash peerID(idData.constData());

   for (int i = 0; i < this->activeBrowseResults.size(); ++i)
   {
      if (this->activeBrowseResults[i].data() == senderObj)
      {
         this->activeBrowseResults.removeAt(i);
         break;
      }
   }

   for (int i = 0; i < entries.size(); ++i)
   {
      const auto& entryList = entries.Get(i);
      for (int j = 0; j < entryList.entry_size(); ++j)
      {
         const auto& entry = entryList.entry(j);
         if (entry.type() == Protos::Common::Entry::DIR)
            this->browseDir(peerID, entry);
         else
            this->addFileEntry(entry, peerID);
      }
   }
}

void NetworkWidget::addFileEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID)
{
   const QString name = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::name);
   quint64 size = entry.size();

   QStandardItem* item = this->findOrAddFile(name, size);

   QVariant existingEntry = item->data(ROLE_ENTRY);
   if (!existingEntry.isValid())
   {
      QByteArray entryData;
      entryData.resize(entry.ByteSizeLong());
      entry.SerializeToArray(entryData.data(), entryData.size());
      item->setData(entryData, ROLE_ENTRY);
      item->setData(static_cast<qulonglong>(size), ROLE_SIZE);
   }

   QByteArray peerIdBytes(peerID.getData(), Common::Hash::HASH_SIZE);
   item->setData(peerIdBytes, ROLE_PEER_ID);
   QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
   QString peerIdStr = QString::fromLatin1(peerIdBytes.toHex());
   if (!peerIds.contains(peerIdStr))
      peerIds.append(peerIdStr);
   item->setData(peerIds, ROLE_PEER_IDS);
   this->updatePeerCount(item);
}

QStandardItem* NetworkWidget::findOrAddFile(const QString& name, quint64 size)
{
   const int rowCount = this->fileModel.rowCount();
   for (int i = 0; i < rowCount; ++i)
   {
      QStandardItem* item = this->fileModel.item(i, 0);
      if (item && item->text() == name && item->data(ROLE_SIZE).toULongLong() == size)
         return item;
   }

   QStandardItem* nameItem = new QStandardItem(name);
   nameItem->setEditable(false);

   QStandardItem* sizeItem = new QStandardItem();
   sizeItem->setEditable(false);
   sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
   if (size > 0)
      sizeItem->setText(Common::Global::formatByteSize(size));

   QStandardItem* peersItem = new QStandardItem("1");
   peersItem->setEditable(false);
   peersItem->setTextAlignment(Qt::AlignCenter);

   this->fileModel.appendRow({ nameItem, sizeItem, peersItem });
   return nameItem;
}

void NetworkWidget::updatePeerCount(QStandardItem* item)
{
   QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
   int row = item->row();
   QStandardItem* peersItem = this->fileModel.item(row, 2);
   if (peersItem)
      peersItem->setText(QString::number(peerIds.size()));
}

void NetworkWidget::displayContextMenuDownload(const QPoint& point)
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   QPoint globalPosition = this->fileTableView->mapToGlobal(point);
   this->downloadMenu.show(globalPosition);
}

void NetworkWidget::download()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   for (const QModelIndex& proxyIndex : selectedRows)
   {
      QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
      QStandardItem* item = this->fileModel.itemFromIndex(sourceIndex);
      if (!item)
         continue;

      QByteArray entryData = item->data(ROLE_ENTRY).toByteArray();
      QByteArray peerIdData = item->data(ROLE_PEER_ID).toByteArray();
      if (entryData.isEmpty() || peerIdData.isEmpty())
         continue;

      Protos::Common::Entry entry;
      if (!entry.ParseFromArray(entryData.constData(), entryData.size()))
         continue;

      Common::Hash peerID(peerIdData.constData());
      this->coreConnection->download(peerID, entry);
   }
}

void NetworkWidget::downloadTo()
{
   QStringList dirs = Utils::askForDirectoriesToDownloadTo(this->coreConnection);
   if (!dirs.isEmpty())
      this->downloadTo(dirs.first());
}

void NetworkWidget::downloadTo(const QString& path, const Common::Hash& sharedDirID)
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   for (const QModelIndex& proxyIndex : selectedRows)
   {
      QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
      QStandardItem* item = this->fileModel.itemFromIndex(sourceIndex);
      if (!item)
         continue;

      QByteArray entryData = item->data(ROLE_ENTRY).toByteArray();
      QByteArray peerIdData = item->data(ROLE_PEER_ID).toByteArray();
      if (entryData.isEmpty() || peerIdData.isEmpty())
         continue;

      Protos::Common::Entry entry;
      if (!entry.ParseFromArray(entryData.constData(), entryData.size()))
         continue;

      Common::Hash peerID(peerIdData.constData());
      this->coreConnection->download(peerID, entry, sharedDirID, path);
   }
}
