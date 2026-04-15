#include <Browse/NetworkFileModel.h>

#include <QColor>
#include <QFileInfo>
#include <QObject>
#include <QSet>

#include <Common/Global.h>
#include <Common/ProtoHelper.h>

using namespace GUI;

namespace
{
   const QBrush OWNED_BRUSH(QColor(0, 140, 0));
   const QBrush ERROR_BRUSH(QColor(200, 120, 0));
}

NetworkFileModel::NetworkFileModel(const SharedEntryListModel& sharedEntryListModel) :
   sharedEntryListModel(sharedEntryListModel)
{
}

QStandardItemModel& NetworkFileModel::itemModel()
{
   return this->fileModel;
}

const QStandardItemModel& NetworkFileModel::itemModel() const
{
   return this->fileModel;
}

QStandardItem* NetworkFileModel::item(int row, NetworkFileList::Column column) const
{
   return this->fileModel.item(row, column);
}

QStandardItem* NetworkFileModel::itemFromIndex(const QModelIndex& index) const
{
   return this->fileModel.itemFromIndex(index);
}

int NetworkFileModel::rowCount() const
{
   return this->fileModel.rowCount();
}

void NetworkFileModel::clear()
{
   this->prevDownloadedBytes.clear();
   this->prevUploadByFile.clear();
   this->downloadQueueIds.clear();
   this->fileModel.removeRows(0, this->fileModel.rowCount());
}

void NetworkFileModel::removeRow(int row)
{
   this->fileModel.removeRow(row);
}

const QList<quint64>& NetworkFileModel::downloadQueue() const
{
   return this->downloadQueueIds;
}

void NetworkFileModel::addBrowseEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID, const Common::Hash& localPeerID, bool fromMaster, quint32 browseGeneration)
{
   const QString name = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::name);
   const quint64 size = entry.size();

   QStandardItem* item = this->findFile(name, size);
   if (!item)
   {
      if (!fromMaster)
         return;
      item = this->addFile(name, size);
   }

   const QVariant existingEntry = item->data(NetworkFileList::ROLE_ENTRY);
   if (!existingEntry.isValid())
   {
      QByteArray entryData;
      entryData.resize(entry.ByteSizeLong());
      entry.SerializeToArray(entryData.data(), entryData.size());
      item->setData(entryData, NetworkFileList::ROLE_ENTRY);
      item->setData(static_cast<qulonglong>(size), NetworkFileList::ROLE_SIZE);
   }

   const QByteArray peerIdBytes(peerID.getData(), Common::Hash::HASH_SIZE);
   if (peerID != localPeerID || !item->data(NetworkFileList::ROLE_PEER_ID).isValid())
      item->setData(peerIdBytes, NetworkFileList::ROLE_PEER_ID);

   if (fromMaster)
      item->setData(browseGeneration, NetworkFileList::ROLE_MASTER_GEN);

   const quint32 itemGen = item->data(NetworkFileList::ROLE_BROWSE_GEN).toUInt();
   QStringList peerIds;
   if (itemGen == browseGeneration)
      peerIds = item->data(NetworkFileList::ROLE_PEER_IDS).toStringList();
   item->setData(browseGeneration, NetworkFileList::ROLE_BROWSE_GEN);

   const QString peerIdStr = QString::fromLatin1(peerIdBytes.toHex());
   if (!peerIds.contains(peerIdStr))
      peerIds.append(peerIdStr);
   item->setData(peerIds, NetworkFileList::ROLE_PEER_IDS);

   if (QStandardItem* peersItem = this->fileModel.item(item->row(), NetworkFileList::COL_PEERS))
      peersItem->setText(QString::number(peerIds.size()));

   if (entry.owned_locally())
      this->markOwned(item->row(), item);
}

void NetworkFileModel::pruneStaleRows(quint32 browseGeneration)
{
   for (int row = this->fileModel.rowCount() - 1; row >= 0; --row)
   {
      QStandardItem* item = this->fileModel.item(row, NetworkFileList::COL_NAME);
      if (!item)
         continue;

      const quint32 masterGen = item->data(NetworkFileList::ROLE_MASTER_GEN).toUInt();
      if (masterGen < browseGeneration && item->data(NetworkFileList::ROLE_DOWNLOAD_ID).toULongLong() == 0)
         this->fileModel.removeRow(row);
   }
}

void NetworkFileModel::updateFromState(const Protos::GUI::State& state, quint32 browseGeneration)
{
   QSet<quint64> activeDownloadIds;
   QMap<quint64, qint64> currentDownloadedBytes;

   this->downloadQueueIds.clear();
   for (int d = 0; d < state.download_size(); ++d)
   {
      const auto& dl = state.download(d);
      if (dl.status() != Protos::GUI::State::Download::COMPLETE)
         this->downloadQueueIds.append(dl.id());
   }

   for (int d = 0; d < state.download_size(); ++d)
   {
      const auto& dl = state.download(d);
      const quint64 dlId = dl.id();
      activeDownloadIds.insert(dlId);
      currentDownloadedBytes[dlId] = dl.downloaded_bytes();

      const QString name = Common::ProtoHelper::getStr(dl.local_entry(), &Protos::Common::Entry::name);
      const quint64 size = dl.local_entry().size();

      for (int row = 0; row < this->fileModel.rowCount(); ++row)
      {
         QStandardItem* item = this->fileModel.item(row, NetworkFileList::COL_NAME);
         if (!item || item->text() != name || item->data(NetworkFileList::ROLE_SIZE).toULongLong() != size)
            continue;

         item->setData(static_cast<qulonglong>(dlId), NetworkFileList::ROLE_DOWNLOAD_ID);

         const int queuePos = (dl.status() != Protos::GUI::State::Download::COMPLETE)
            ? this->downloadQueueIds.indexOf(dlId) + 1
            : 0;
         item->setData(queuePos, NetworkFileList::ROLE_QUEUE_POS);

         if (QStandardItem* queueItem = this->fileModel.item(row, NetworkFileList::COL_QUEUE))
         {
            queueItem->setText(queuePos > 0 ? QString::number(queuePos) : QString());
            queueItem->setData(queuePos, NetworkFileList::ROLE_QUEUE_POS);
         }

         if (QStandardItem* statusItem = this->fileModel.item(row, NetworkFileList::COL_STATUS))
         {
            if (dl.status() == Protos::GUI::State::Download::COMPLETE)
               statusItem->setText(QObject::tr("Owned"));
            else
               statusItem->setText(statusText(dl.status()));
         }

         int progress = 0;
         if (dl.status() != Protos::GUI::State::Download::COMPLETE && size > 0)
            progress = static_cast<int>(dl.downloaded_bytes() * 10000 / size);

         if (QStandardItem* progressItem = this->fileModel.item(row, NetworkFileList::COL_PROGRESS))
            progressItem->setData(progress, NetworkFileList::ROLE_PROGRESS);

         if (QStandardItem* dlSpeedItem = this->fileModel.item(row, NetworkFileList::COL_DL_SPEED))
         {
            qint64 speed = 0;
            if (this->prevDownloadedBytes.contains(dlId) && dl.status() == Protos::GUI::State::Download::DOWNLOADING)
               speed = dl.downloaded_bytes() - this->prevDownloadedBytes[dlId];

            if (speed > 0)
               dlSpeedItem->setText(Common::Global::formatByteSize(speed) + "/s");
            else
               dlSpeedItem->setText(QString());
         }

         if (isErrorStatus(dl.status()))
         {
            this->setRowForeground(row, ERROR_BRUSH, QObject::tr("Download stuck — right-click or use Redownload button"));
         }
         else if (!item->data(NetworkFileList::ROLE_OWNED).toBool())
         {
            this->setRowForeground(row, QBrush());
         }

         if (dl.status() == Protos::GUI::State::Download::COMPLETE && !item->data(NetworkFileList::ROLE_OWNED).toBool())
            this->markOwned(row, item);

         if (dl.peer_id_size() > 0 && item->data(NetworkFileList::ROLE_BROWSE_GEN).toUInt() == browseGeneration)
         {
            QStringList peerIds = item->data(NetworkFileList::ROLE_PEER_IDS).toStringList();
            for (int p = 0; p < dl.peer_id_size(); ++p)
            {
               const QString pid = QString::fromLatin1(QByteArray(dl.peer_id(p).hash().data(), Common::Hash::HASH_SIZE).toHex());
               if (!peerIds.contains(pid))
                  peerIds.append(pid);
            }
            item->setData(peerIds, NetworkFileList::ROLE_PEER_IDS);
            if (QStandardItem* peersItem = this->fileModel.item(row, NetworkFileList::COL_PEERS))
               peersItem->setText(QString::number(peerIds.size()));
         }

         break;
      }
   }

   for (int row = 0; row < this->fileModel.rowCount(); ++row)
   {
      if (QStandardItem* ulItem = this->fileModel.item(row, NetworkFileList::COL_UL_SPEED))
         ulItem->setText(QString());
   }

   QMap<QPair<QString, quint64>, qint64> currentUploadByFile;
   for (int u = 0; u < state.upload_size(); ++u)
   {
      const auto& ul = state.upload(u);
      const QString name = Common::ProtoHelper::getStr(ul.file(), &Protos::Common::Entry::name);
      const quint64 size = ul.file().size();

      qint64 estimatedBytes = 0;
      if (ul.nb_part() > 0 && ul.current_part() > 0)
      {
         const double fraction =
            (static_cast<double>(ul.current_part() - 1) + static_cast<double>(ul.progress()) / 10000.0)
            / static_cast<double>(ul.nb_part());
         estimatedBytes = static_cast<qint64>(fraction * static_cast<double>(size));
      }
      currentUploadByFile[qMakePair(name, size)] += estimatedBytes;
   }

   for (auto it = currentUploadByFile.constBegin(); it != currentUploadByFile.constEnd(); ++it)
   {
      const QString& name = it.key().first;
      const quint64 size = it.key().second;
      const qint64 estimatedBytes = it.value();

      for (int row = 0; row < this->fileModel.rowCount(); ++row)
      {
         QStandardItem* item = this->fileModel.item(row, NetworkFileList::COL_NAME);
         if (!item || item->text() != name || item->data(NetworkFileList::ROLE_SIZE).toULongLong() != size)
            continue;

         if (QStandardItem* ulItem = this->fileModel.item(row, NetworkFileList::COL_UL_SPEED))
         {
            qint64 speed = 0;
            if (this->prevUploadByFile.contains(it.key()))
               speed = estimatedBytes - this->prevUploadByFile[it.key()];

            if (speed > 0)
               ulItem->setText(Common::Global::formatByteSize(speed) + "/s");
            else
               ulItem->setText(QObject::tr("Uploading"));
         }
         break;
      }
   }

   for (int row = 0; row < this->fileModel.rowCount(); ++row)
   {
      QStandardItem* item = this->fileModel.item(row, NetworkFileList::COL_NAME);
      if (!item)
         continue;

      const quint64 dlId = item->data(NetworkFileList::ROLE_DOWNLOAD_ID).toULongLong();
      if (dlId == 0 || activeDownloadIds.contains(dlId))
         continue;

      this->clearTransientDownloadState(item);
   }

   this->prevDownloadedBytes = currentDownloadedBytes;
   this->prevUploadByFile = currentUploadByFile;
}

QString NetworkFileModel::getLocalPath(QStandardItem* item) const
{
   const QByteArray entryData = item->data(NetworkFileList::ROLE_ENTRY).toByteArray();
   if (entryData.isEmpty())
      return QString();

   Protos::Common::Entry entry;
   if (!entry.ParseFromArray(entryData.constData(), entryData.size()))
      return QString();

   const QString entryName = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::name);
   const QString entryPath = Common::ProtoHelper::getStr(entry, &Protos::Common::Entry::path);

   for (const Common::SharedEntry& localDir : this->sharedEntryListModel.getSharedDirectories())
   {
      QString candidatePath = localDir.path.getPath();
      if (!candidatePath.endsWith('/'))
         candidatePath.append('/');

      if (!entryPath.isEmpty())
      {
         QString rel = entryPath;
         if (rel.startsWith('/'))
            rel = rel.mid(1);
         candidatePath.append(rel);
      }

      candidatePath.append(entryName);

      if (QFileInfo::exists(candidatePath))
         return candidatePath;
   }

   return QString();
}

QStandardItem* NetworkFileModel::findFile(const QString& name, quint64 size)
{
   const int rowCount = this->fileModel.rowCount();
   for (int i = 0; i < rowCount; ++i)
   {
      QStandardItem* item = this->fileModel.item(i, NetworkFileList::COL_NAME);
      if (item && item->text() == name && item->data(NetworkFileList::ROLE_SIZE).toULongLong() == size)
         return item;
   }
   return nullptr;
}

QStandardItem* NetworkFileModel::addFile(const QString& name, quint64 size)
{
   QList<QStandardItem*> row;
   for (int col = 0; col < NetworkFileList::COL_COUNT; ++col)
   {
      QStandardItem* item = new QStandardItem();
      item->setEditable(false);
      row.append(item);
   }

   row[NetworkFileList::COL_NAME]->setText(name);
   row[NetworkFileList::COL_SIZE]->setText(size > 0 ? Common::Global::formatByteSize(size) : QString());
   row[NetworkFileList::COL_SIZE]->setData(static_cast<qulonglong>(size), Qt::UserRole);
   row[NetworkFileList::COL_SIZE]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
   row[NetworkFileList::COL_QUEUE]->setTextAlignment(Qt::AlignCenter);
   row[NetworkFileList::COL_PEERS]->setTextAlignment(Qt::AlignCenter);
   row[NetworkFileList::COL_PEERS]->setText("1");
   row[NetworkFileList::COL_DL_SPEED]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
   row[NetworkFileList::COL_UL_SPEED]->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

   this->fileModel.appendRow(row);
   return row[NetworkFileList::COL_NAME];
}

void NetworkFileModel::setRowForeground(int row, const QBrush& brush, const QString& tooltip)
{
   for (int col = 0; col < NetworkFileList::COL_COUNT; ++col)
   {
      if (QStandardItem* colItem = this->fileModel.item(row, static_cast<NetworkFileList::Column>(col)))
      {
         colItem->setForeground(brush);
         colItem->setToolTip(tooltip);
      }
   }
}

void NetworkFileModel::markOwned(int row, QStandardItem* item)
{
   item->setData(true, NetworkFileList::ROLE_OWNED);
   this->setRowForeground(row, OWNED_BRUSH);

   if (QStandardItem* statusItem = this->fileModel.item(row, NetworkFileList::COL_STATUS))
   {
      if (statusItem->text().isEmpty())
         statusItem->setText(QObject::tr("Owned"));
   }
}

void NetworkFileModel::clearTransientDownloadState(QStandardItem* item)
{
   if (QStandardItem* statusItem = this->fileModel.item(item->row(), NetworkFileList::COL_STATUS))
   {
      if (item->data(NetworkFileList::ROLE_OWNED).toBool())
         statusItem->setText(QObject::tr("Owned"));
      else
         statusItem->setText(QString());
   }

   if (QStandardItem* queueItem = this->fileModel.item(item->row(), NetworkFileList::COL_QUEUE))
   {
      queueItem->setText(QString());
      queueItem->setData(0, NetworkFileList::ROLE_QUEUE_POS);
   }

   if (QStandardItem* progressItem = this->fileModel.item(item->row(), NetworkFileList::COL_PROGRESS))
      progressItem->setData(0, NetworkFileList::ROLE_PROGRESS);

   if (QStandardItem* dlSpeedItem = this->fileModel.item(item->row(), NetworkFileList::COL_DL_SPEED))
      dlSpeedItem->setText(QString());

   item->setData(QVariant(), NetworkFileList::ROLE_DOWNLOAD_ID);
   item->setData(0, NetworkFileList::ROLE_QUEUE_POS);
}

bool NetworkFileModel::isErrorStatus(Protos::GUI::State::Download::Status status)
{
   switch (status)
   {
   case Protos::GUI::State::Download::UNABLE_TO_RETRIEVE_THE_HASHES:
   case Protos::GUI::State::Download::HASH_MISMATCH:
   case Protos::GUI::State::Download::TRANSFER_ERROR:
   case Protos::GUI::State::Download::UNABLE_TO_OPEN_THE_FILE:
   case Protos::GUI::State::Download::FILE_IO_ERROR:
   case Protos::GUI::State::Download::FILE_NON_EXISTENT:
   case Protos::GUI::State::Download::GOT_TOO_MUCH_DATA:
   case Protos::GUI::State::Download::UNABLE_TO_GET_ENTRIES:
   case Protos::GUI::State::Download::NO_SOURCE:
   case Protos::GUI::State::Download::ENTRY_NOT_FOUND:
      return true;
   default:
      return false;
   }
}

QString NetworkFileModel::statusText(Protos::GUI::State::Download::Status status)
{
   switch (status)
   {
   case Protos::GUI::State::Download::QUEUED:                         return QObject::tr("Queued");
   case Protos::GUI::State::Download::GETTING_THE_HASHES:             return QObject::tr("Hashing...");
   case Protos::GUI::State::Download::DOWNLOADING:                    return QObject::tr("Downloading");
   case Protos::GUI::State::Download::COMPLETE:                       return QObject::tr("Complete");
   case Protos::GUI::State::Download::PAUSED:                         return QObject::tr("Paused");
   case Protos::GUI::State::Download::UNKNOWN_PEER_SOURCE:            return QObject::tr("Peer offline");
   case Protos::GUI::State::Download::ENTRY_NOT_FOUND:                return QObject::tr("Not found");
   case Protos::GUI::State::Download::NO_SOURCE:                      return QObject::tr("No source");
   case Protos::GUI::State::Download::NO_SHARED_DIRECTORY_TO_WRITE:   return QObject::tr("No incoming dir");
   case Protos::GUI::State::Download::NO_ENOUGH_FREE_SPACE:           return QObject::tr("No space");
   case Protos::GUI::State::Download::UNABLE_TO_CREATE_THE_FILE:      return QObject::tr("Can't create file");
   case Protos::GUI::State::Download::UNABLE_TO_CREATE_THE_DIRECTORY: return QObject::tr("Can't create dir");
   case Protos::GUI::State::Download::TRANSFER_ERROR:                 return QObject::tr("Transfer error");
   case Protos::GUI::State::Download::HASH_MISMATCH:                  return QObject::tr("Hash mismatch");
   case Protos::GUI::State::Download::REMOTE_SCANNING_IN_PROGRESS:    return QObject::tr("Remote scanning...");
   case Protos::GUI::State::Download::LOCAL_SCANNING_IN_PROGRESS:     return QObject::tr("Local scanning...");
   case Protos::GUI::State::Download::UNABLE_TO_RETRIEVE_THE_HASHES:  return QObject::tr("Stale hashes — Redownload");
   case Protos::GUI::State::Download::UNABLE_TO_OPEN_THE_FILE:        return QObject::tr("Can't open file");
   case Protos::GUI::State::Download::FILE_IO_ERROR:                  return QObject::tr("File I/O error");
   case Protos::GUI::State::Download::FILE_NON_EXISTENT:              return QObject::tr("File missing");
   case Protos::GUI::State::Download::GOT_TOO_MUCH_DATA:              return QObject::tr("Excess data");
   case Protos::GUI::State::Download::UNABLE_TO_GET_ENTRIES:          return QObject::tr("Can't get entries");
   default:                                                           return QObject::tr("Error");
   }
}
