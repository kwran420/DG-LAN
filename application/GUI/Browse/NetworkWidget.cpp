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
            return QString(Common::Global::formatByteSize(ti.lanSpeed) + "/s");
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

   // Right panel: unified file list
   this->fileTreeView = new QTreeView();
   this->fileTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
   this->fileTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
   this->fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
   this->fileTreeView->setAlternatingRowColors(true);

   this->fileModel.setHorizontalHeaderLabels({ tr("Name"), tr("Size"), tr("Peers") });
   this->fileTreeView->setModel(&this->fileModel);
   this->fileTreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
   this->fileTreeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   this->fileTreeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   this->splitter->addWidget(this->fileTreeView);

   this->splitter->setStretchFactor(0, 0);
   this->splitter->setStretchFactor(1, 1);

   connect(this->fileTreeView, &QTreeView::customContextMenuRequested, this, &NetworkWidget::displayContextMenuDownload);
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

   // Store peerID in the result's property so we can retrieve it in the slot  
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

   // Remove this result from active list
   for (int i = 0; i < this->activeBrowseResults.size(); ++i)
   {
      if (this->activeBrowseResults[i].data() == senderObj)
      {
         this->activeBrowseResults.removeAt(i);
         break;
      }
   }

   if (entries.size() == 0)
      return;

   // Root entries: each Entries message contains children of one shared directory
   for (int i = 0; i < entries.size(); ++i)
   {
      const auto& entryList = entries.Get(i);
      for (int j = 0; j < entryList.entry_size(); ++j)
      {
         const auto& entry = entryList.entry(j);
         const QString name = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::name);
         QStandardItem* item = this->findOrCreateChild(nullptr, name, entry.type() == Protos::Common::Entry::DIR);

         // Store entry data
         QVariant existingEntry = item->data(ROLE_ENTRY);
         if (!existingEntry.isValid())
         {
            QByteArray entryData;
            entryData.resize(entry.ByteSizeLong());
            entry.SerializeToArray(entryData.data(), entryData.size());
            item->setData(entryData, ROLE_ENTRY);
         }

         // Track which peers have this entry
         QByteArray peerIdBytes(peerID.getData(), Common::Hash::HASH_SIZE);
         item->setData(peerIdBytes, ROLE_PEER_ID);
         QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
         QString peerIdStr = QString::fromLatin1(peerIdBytes.toHex());
         if (!peerIds.contains(peerIdStr))
            peerIds.append(peerIdStr);
         item->setData(peerIds, ROLE_PEER_IDS);
         this->updatePeerCount(item);

         // Recursively browse directories
         if (entry.type() == Protos::Common::Entry::DIR)
            this->browseDir(peerID, entry, item);
      }
   }
}

void NetworkWidget::browseDir(const Common::Hash& peerID, const Protos::Common::Entry& dirEntry, QStandardItem* parentItem)
{
   auto browseResult = this->coreConnection->browse(peerID, dirEntry);

   QByteArray idData(peerID.getData(), Common::Hash::HASH_SIZE);
   browseResult->setProperty("peerID", idData);

   // Store parent item path for later lookup
   QModelIndex parentIndex = this->fileModel.indexFromItem(parentItem);
   browseResult->setProperty("parentRow", parentIndex.row());
   browseResult->setProperty("parentPath", parentItem->text());

   // Build a persistent path from root to this item for re-lookup
   QStringList pathParts;
   QStandardItem* cur = parentItem;
   while (cur)
   {
      pathParts.prepend(cur->text());
      cur = cur->parent();
   }
   browseResult->setProperty("treePath", pathParts);

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
   QStringList treePath = senderObj->property("treePath").toStringList();

   // Remove this result from active list
   for (int i = 0; i < this->activeBrowseResults.size(); ++i)
   {
      if (this->activeBrowseResults[i].data() == senderObj)
      {
         this->activeBrowseResults.removeAt(i);
         break;
      }
   }

   if (entries.size() == 0 || treePath.isEmpty())
      return;

   // Re-find the parent item by traversing the tree path
   QStandardItem* parentItem = nullptr;
   for (const QString& part : treePath)
   {
      parentItem = this->findOrCreateChild(parentItem, part, true);
   }

   const auto& entryList = entries.Get(0);
   for (int j = 0; j < entryList.entry_size(); ++j)
   {
      const auto& entry = entryList.entry(j);
      const QString name = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::name);
      bool isDir = entry.type() == Protos::Common::Entry::DIR;
      QStandardItem* item = this->findOrCreateChild(parentItem, name, isDir);

      // Store entry data
      QVariant existingEntry = item->data(ROLE_ENTRY);
      if (!existingEntry.isValid())
      {
         QByteArray entryData;
         entryData.resize(entry.ByteSizeLong());
         entry.SerializeToArray(entryData.data(), entryData.size());
         item->setData(entryData, ROLE_ENTRY);
      }

      // Track peers
      QByteArray peerIdBytes(peerID.getData(), Common::Hash::HASH_SIZE);
      item->setData(peerIdBytes, ROLE_PEER_ID);
      QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
      QString peerIdStr = QString::fromLatin1(peerIdBytes.toHex());
      if (!peerIds.contains(peerIdStr))
         peerIds.append(peerIdStr);
      item->setData(peerIds, ROLE_PEER_IDS);
      this->updatePeerCount(item);

      if (isDir)
         this->browseDir(peerID, entry, item);
   }
}

QStandardItem* NetworkWidget::findOrCreateChild(QStandardItem* parent, const QString& name, bool isDir)
{
   // Search existing children
   const int rowCount = parent ? parent->rowCount() : this->fileModel.rowCount();
   for (int i = 0; i < rowCount; ++i)
   {
      QStandardItem* child = parent ? parent->child(i, 0) : this->fileModel.item(i, 0);
      if (child && child->text() == name)
         return child;
   }

   // Create new row with 3 columns: Name, Size, Peers
   QStandardItem* nameItem = new QStandardItem(name);
   nameItem->setEditable(false);
   nameItem->setData(isDir, ROLE_IS_DIR);

   QStandardItem* sizeItem = new QStandardItem();
   sizeItem->setEditable(false);
   sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

   QStandardItem* peersItem = new QStandardItem("1");
   peersItem->setEditable(false);
   peersItem->setTextAlignment(Qt::AlignCenter);

   if (parent)
      parent->appendRow({ nameItem, sizeItem, peersItem });
   else
      this->fileModel.appendRow({ nameItem, sizeItem, peersItem });

   return nameItem;
}

void NetworkWidget::updatePeerCount(QStandardItem* item)
{
   QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
   int count = peerIds.size();

   // Update peers column (column 2)
   QStandardItem* parent = item->parent();
   int row = item->row();
   QStandardItem* peersItem = parent ? parent->child(row, 2) : this->fileModel.item(row, 2);
   if (peersItem)
      peersItem->setText(QString::number(count));

   // Update size column (column 1) from stored entry
   QByteArray entryData = item->data(ROLE_ENTRY).toByteArray();
   if (!entryData.isEmpty())
   {
      Protos::Common::Entry entry;
      if (entry.ParseFromArray(entryData.constData(), entryData.size()) && entry.size() > 0)
      {
         QStandardItem* sizeItem = parent ? parent->child(row, 1) : this->fileModel.item(row, 1);
         if (sizeItem)
            sizeItem->setText(Common::Global::formatByteSize(entry.size()));
      }
   }
}

void NetworkWidget::displayContextMenuDownload(const QPoint& point)
{
   QModelIndexList selectedRows = this->fileTreeView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   QPoint globalPosition = this->fileTreeView->mapToGlobal(point);
   this->downloadMenu.show(globalPosition);
}

void NetworkWidget::download()
{
   QModelIndexList selectedRows = this->fileTreeView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   for (const QModelIndex& index : selectedRows)
   {
      QStandardItem* item = this->fileModel.itemFromIndex(index);
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
   QModelIndexList selectedRows = this->fileTreeView->selectionModel()->selectedRows();
   for (const QModelIndex& index : selectedRows)
   {
      QStandardItem* item = this->fileModel.itemFromIndex(index);
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
