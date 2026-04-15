#pragma once

#include <QBrush>
#include <QMap>
#include <QModelIndex>
#include <QStandardItemModel>

#include <Common/Hash.h>

#include <Settings/SharedEntryListModel.h>

#include <Protos/common.pb.h>
#include <Protos/gui_protocol.pb.h>

namespace GUI
{
   namespace NetworkFileList
   {
      enum Column
      {
         COL_NAME = 0,
         COL_SIZE,
         COL_STATUS,
         COL_QUEUE,
         COL_PROGRESS,
         COL_DL_SPEED,
         COL_UL_SPEED,
         COL_PEERS,
         COL_COUNT
      };

      inline constexpr int ROLE_ENTRY = Qt::UserRole + 1;
      inline constexpr int ROLE_PEER_IDS = Qt::UserRole + 2;
      inline constexpr int ROLE_PEER_ID = Qt::UserRole + 3;
      inline constexpr int ROLE_SIZE = Qt::UserRole + 4;
      inline constexpr int ROLE_PROGRESS = Qt::UserRole + 5;
      inline constexpr int ROLE_DL_SPEED = Qt::UserRole + 6;
      inline constexpr int ROLE_UL_SPEED = Qt::UserRole + 7;
      inline constexpr int ROLE_DOWNLOAD_ID = Qt::UserRole + 8;
      inline constexpr int ROLE_OWNED = Qt::UserRole + 9;
      inline constexpr int ROLE_BROWSE_GEN = Qt::UserRole + 10;
      inline constexpr int ROLE_MASTER_GEN = Qt::UserRole + 11;
      inline constexpr int ROLE_QUEUE_POS = Qt::UserRole + 12;
   }

   class NetworkFileModel
   {
   public:
      explicit NetworkFileModel(const SharedEntryListModel& sharedEntryListModel);

      QStandardItemModel& itemModel();
      const QStandardItemModel& itemModel() const;
      QStandardItem* item(int row, NetworkFileList::Column column = NetworkFileList::COL_NAME) const;
      QStandardItem* itemFromIndex(const QModelIndex& index) const;
      int rowCount() const;

      void clear();
      void removeRow(int row);
      void addBrowseEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID, const Common::Hash& localPeerID, bool fromMaster, quint32 browseGeneration);
      void pruneStaleRows(quint32 browseGeneration);
      void updateFromState(const Protos::GUI::State& state, quint32 browseGeneration);
      QString getLocalPath(QStandardItem* item) const;

      const QList<quint64>& downloadQueue() const;

   private:
      QStandardItem* findFile(const QString& name, quint64 size);
      QStandardItem* addFile(const QString& name, quint64 size);
      void setRowForeground(int row, const QBrush& brush, const QString& tooltip = QString());
      void markOwned(int row, QStandardItem* item);
      void clearTransientDownloadState(QStandardItem* item);
      static bool isErrorStatus(Protos::GUI::State::Download::Status status);
      static QString statusText(Protos::GUI::State::Download::Status status);

      const SharedEntryListModel& sharedEntryListModel;
      QStandardItemModel fileModel;
      QMap<quint64, qint64> prevDownloadedBytes;
      QMap<QPair<QString, quint64>, qint64> prevUploadByFile;
      QList<quint64> downloadQueueIds;
   };
}
