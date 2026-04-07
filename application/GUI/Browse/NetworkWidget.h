#pragma once

#include <QWidget>
#include <QTableView>
#include <QSplitter>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QIdentityProxyModel>

#include <Common/Hash.h>
#include <Common/RemoteCoreController/ICoreConnection.h>
#include <Common/RemoteCoreController/IBrowseResult.h>

#include <Peers/PeerListModel.h>
#include <Settings/SharedEntryListModel.h>
#include <DownloadMenu.h>

#include <Protos/common.pb.h>

namespace GUI
{
   class PeerSpeedProxy : public QIdentityProxyModel
   {
   public:
      using QIdentityProxyModel::QIdentityProxyModel;
      QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
      QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const override;
   };

   class NetworkWidget : public QWidget
   {
      Q_OBJECT

   public:
      explicit NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent = nullptr);

   protected:
      void changeEvent(QEvent* event) override;
      void keyPressEvent(QKeyEvent* event) override;

   private slots:
      void refreshPeers();
      void coreDisconnected();
      void browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries);
      void browseSubResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries);
      void displayContextMenuDownload(const QPoint& point);
      void download();
      void downloadTo();
      void downloadTo(const QString& path, const Common::Hash& sharedDirID = Common::Hash());

   private:
      void browsePeer(const Common::Hash& peerID);
      void browseDir(const Common::Hash& peerID, const Protos::Common::Entry& dirEntry);
      QStandardItem* findOrAddFile(const QString& name, quint64 size);
      void updatePeerCount(QStandardItem* item);
      void addFileEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID);

      static const int ROLE_ENTRY = Qt::UserRole + 1;
      static const int ROLE_PEER_IDS = Qt::UserRole + 2;
      static const int ROLE_PEER_ID = Qt::UserRole + 3;
      static const int ROLE_SIZE = Qt::UserRole + 4;

      QSharedPointer<RCC::ICoreConnection> coreConnection;
      PeerListModel& peerListModel;
      const SharedEntryListModel& sharedEntryListModel;

      QSplitter* splitter;
      QTableView* peerTableView;
      PeerSpeedProxy peerProxy;
      QTableView* fileTableView;
      QStandardItemModel fileModel;
      QSortFilterProxyModel fileSortProxy;
      DownloadMenu downloadMenu;

      QSet<Common::Hash> browsedPeers;
      QList<QSharedPointer<RCC::IBrowseResult>> activeBrowseResults;
   };
}
