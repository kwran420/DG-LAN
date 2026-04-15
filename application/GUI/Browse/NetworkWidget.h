#pragma once

#include <QWidget>
#include <QTableView>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QIdentityProxyModel>
#include <QStyledItemDelegate>
#include <QTimer>

#include <Common/Hash.h>
#include <Common/RemoteCoreController/ICoreConnection.h>
#include <Common/RemoteCoreController/IBrowseResult.h>

#include <Browse/NetworkFileModel.h>
#include <Peers/PeerListModel.h>
#include <Settings/SharedEntryListModel.h>
#include <DownloadMenu.h>

#include <Protos/common.pb.h>
#include <Protos/gui_protocol.pb.h>

namespace GUI
{
   class PeerSpeedProxy : public QIdentityProxyModel
   {
   public:
      using QIdentityProxyModel::QIdentityProxyModel;
      int columnCount(const QModelIndex& parent = QModelIndex()) const override;
      QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
      QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;
      QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
      QVariant data(const QModelIndex& proxyIndex, int role = Qt::DisplayRole) const override;
   };

   class ProgressDelegate : public QStyledItemDelegate
   {
   public:
      using QStyledItemDelegate::QStyledItemDelegate;
      void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
   };

   class FileSortProxy : public QSortFilterProxyModel
   {
   public:
      using QSortFilterProxyModel::QSortFilterProxyModel;
   protected:
      bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
   };

   class NetworkWidget : public QWidget
   {
      Q_OBJECT

   public:
      explicit NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent = nullptr);

      // Column indices for the file model.
      enum FileColumn
      {
         COL_NAME = NetworkFileList::COL_NAME,
         COL_SIZE = NetworkFileList::COL_SIZE,
         COL_STATUS = NetworkFileList::COL_STATUS,
         COL_QUEUE = NetworkFileList::COL_QUEUE,
         COL_PROGRESS = NetworkFileList::COL_PROGRESS,
         COL_DL_SPEED = NetworkFileList::COL_DL_SPEED,
         COL_UL_SPEED = NetworkFileList::COL_UL_SPEED,
         COL_PEERS = NetworkFileList::COL_PEERS,
         COL_COUNT = NetworkFileList::COL_COUNT
      };

      static constexpr int ROLE_PROGRESS = NetworkFileList::ROLE_PROGRESS;

   protected:
      void changeEvent(QEvent* event) override;
      void keyPressEvent(QKeyEvent* event) override;

   private slots:
      void onNewState(const Protos::GUI::State& state);
      void coreDisconnected();
      void browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries);
      void browseSubResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries);
      void displayContextMenuDownload(const QPoint& point);
      void download();
      void downloadTo();
      void downloadTo(const QString& path, const Common::Hash& sharedDirID = Common::Hash());
      void redownload();
      void cancelDownload();
      void openFileLocation();
      void fileDoubleClicked(const QModelIndex& index);
      void openFolder();
      void moveToTop();
      void moveUp();
      void moveDown();
      void moveToBottom();
      void updateButtonStates();

   private:
      void browsePeer(const Common::Hash& peerID);
      void browseDir(const Common::Hash& peerID, const Protos::Common::Entry& dirEntry);
      void addFileEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID, bool fromMaster);

      static constexpr int ROLE_ENTRY = NetworkFileList::ROLE_ENTRY;
      static constexpr int ROLE_PEER_IDS = NetworkFileList::ROLE_PEER_IDS;
      static constexpr int ROLE_PEER_ID = NetworkFileList::ROLE_PEER_ID;
      static constexpr int ROLE_SIZE = NetworkFileList::ROLE_SIZE;
      static constexpr int ROLE_DL_SPEED = NetworkFileList::ROLE_DL_SPEED;
      static constexpr int ROLE_UL_SPEED = NetworkFileList::ROLE_UL_SPEED;
      static constexpr int ROLE_DOWNLOAD_ID = NetworkFileList::ROLE_DOWNLOAD_ID;
      static constexpr int ROLE_OWNED = NetworkFileList::ROLE_OWNED;
      static constexpr int ROLE_BROWSE_GEN = NetworkFileList::ROLE_BROWSE_GEN;
      static constexpr int ROLE_MASTER_GEN = NetworkFileList::ROLE_MASTER_GEN;
      static constexpr int ROLE_QUEUE_POS = NetworkFileList::ROLE_QUEUE_POS;

      QSharedPointer<RCC::ICoreConnection> coreConnection;
      PeerListModel& peerListModel;
      const SharedEntryListModel& sharedEntryListModel;

      QSplitter* splitter;
      QTableView* peerTableView;
      PeerSpeedProxy peerProxy;
      QLineEdit* filterEdit;
      QPushButton* downloadButton;
      QPushButton* redownloadButton;
      QPushButton* deleteButton;
      QPushButton* moveTopButton;
      QPushButton* moveUpButton;
      QPushButton* moveDownButton;
      QPushButton* moveBottomButton;
      QPushButton* openFolderButton;
      QTableView* fileTableView;
      NetworkFileModel networkFileModel;
      FileSortProxy fileSortProxy;
      ProgressDelegate progressDelegate;

      Common::Hash masterPeerID;
      Common::Hash localPeerID;
      QSet<Common::Hash> browsedPeers;
      DownloadMenu downloadMenu;
      QList<QSharedPointer<RCC::IBrowseResult>> activeBrowseResults;
      QTimer rebrowseTimer;
      quint32 browseGeneration = 0;
      quint32 lastLoggedGeneration = 0;
      quint32 lastPrunedGeneration = 0;
      int browsesPendingThisGen = 0;
      bool updatePromptShown = false;
   };
}
