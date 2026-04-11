#pragma once

#include <QWidget>
#include <QTableView>
#include <QSplitter>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QIdentityProxyModel>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QMap>

#include <Common/Hash.h>
#include <Common/RemoteCoreController/ICoreConnection.h>
#include <Common/RemoteCoreController/IBrowseResult.h>

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
      enum FileColumn { COL_NAME = 0, COL_SIZE, COL_STATUS, COL_QUEUE, COL_PROGRESS, COL_DL_SPEED, COL_UL_SPEED, COL_PEERS, COL_COUNT };

      static const int ROLE_PROGRESS = Qt::UserRole + 5;    // int 0-10000

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
      QStandardItem* findFile(const QString& name, quint64 size);
      QStandardItem* addFile(const QString& name, quint64 size);
      void addFileEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID, bool fromMaster);
      void updateFileFromState(const Protos::GUI::State& state);
      static QString statusText(Protos::GUI::State::Download::Status status);
      QString getLocalPath(QStandardItem* item) const;

      static const int ROLE_ENTRY = Qt::UserRole + 1;
      static const int ROLE_PEER_IDS = Qt::UserRole + 2;
      static const int ROLE_PEER_ID = Qt::UserRole + 3;
      static const int ROLE_SIZE = Qt::UserRole + 4;
      // ROLE_PROGRESS is declared public above.
      static const int ROLE_DL_SPEED = Qt::UserRole + 6;    // qint64 bytes/s
      static const int ROLE_UL_SPEED = Qt::UserRole + 7;    // qint64 bytes/s
      static const int ROLE_DOWNLOAD_ID = Qt::UserRole + 8;  // quint64
      static const int ROLE_OWNED = Qt::UserRole + 9;        // bool
      static const int ROLE_BROWSE_GEN = Qt::UserRole + 10;  // quint32
      static const int ROLE_MASTER_GEN = Qt::UserRole + 11;  // quint32 — generation when master last confirmed this file
      static const int ROLE_QUEUE_POS = Qt::UserRole + 12;   // int — 1-based queue position (0 = not queued)

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
      QStandardItemModel fileModel;
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

      // Per-download speed tracking: download ID → previous downloaded_bytes
      QMap<quint64, qint64> prevDownloadedBytes;
      // Per-file upload speed tracking: (name, size) → previous total estimated bytes
      QMap<QPair<QString, quint64>, qint64> prevUploadByFile;
      bool updatePromptShown = false;

      // Ordered list of non-complete download IDs from latest state (index 0 = first in queue).
      QList<quint64> downloadQueue;
   };
}
