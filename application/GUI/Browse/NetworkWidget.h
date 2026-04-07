#pragma once

#include <QWidget>
#include <QTreeView>
#include <QTableView>
#include <QSplitter>
#include <QStandardItemModel>
#include <QSet>
#include <QMap>

#include <Common/Hash.h>
#include <Common/RemoteCoreController/ICoreConnection.h>
#include <Common/RemoteCoreController/IBrowseResult.h>

#include <Peers/PeerListModel.h>
#include <Peers/PeerListDelegate.h>
#include <Settings/SharedEntryListModel.h>
#include <DownloadMenu.h>

#include <Protos/common.pb.h>

namespace GUI
{
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
      void browseDir(const Common::Hash& peerID, const Protos::Common::Entry& dirEntry, QStandardItem* parentItem);
      QStandardItem* findOrCreateChild(QStandardItem* parent, const QString& name, bool isDir);
      void updatePeerCount(QStandardItem* item);

      static const int ROLE_ENTRY = Qt::UserRole + 1;
      static const int ROLE_PEER_IDS = Qt::UserRole + 2;
      static const int ROLE_IS_DIR = Qt::UserRole + 3;
      static const int ROLE_PEER_ID = Qt::UserRole + 4;

      QSharedPointer<RCC::ICoreConnection> coreConnection;
      PeerListModel& peerListModel;
      const SharedEntryListModel& sharedEntryListModel;

      QSplitter* splitter;
      QTableView* peerTableView;
      PeerListDelegate peerListDelegate;
      QTreeView* fileTreeView;
      QStandardItemModel fileModel;
      DownloadMenu downloadMenu;

      QSet<Common::Hash> browsedPeers;
      QList<QSharedPointer<RCC::IBrowseResult>> activeBrowseResults;
   };
}
