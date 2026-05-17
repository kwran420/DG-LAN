#include <Browse/NetworkWidget.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QKeyEvent>
#include <QApplication>
#include <QByteArray>
#include <QPainter>
#include <QStyleOptionProgressBar>
#include <QMessageBox>
#include <QVersionNumber>
#include <QDesktopServices>

#include <Common/Version.h>

#include <Log.h>
#include <Utils.h>
#include <UpdateChecker.h>
#include <UpdateDialog.h>

// ── PeerSpeedProxy ───────────────────────────────────────────────────────────

int PeerSpeedProxy::columnCount(const QModelIndex& parent) const
{
   if (!sourceModel())
      return 0;
   return sourceModel()->columnCount(mapToSource(parent)) + 3; // +Speed, +Version, +IP
}

QModelIndex PeerSpeedProxy::index(int row, int column, const QModelIndex& parent) const
{
   if (!sourceModel())
      return QModelIndex();
   int srcCols = sourceModel()->columnCount(mapToSource(parent));
   if (column >= srcCols)
      return createIndex(row, column);
   return QIdentityProxyModel::index(row, column, parent);
}

QModelIndex PeerSpeedProxy::mapToSource(const QModelIndex& proxyIndex) const
{
   if (!proxyIndex.isValid() || !sourceModel())
      return QModelIndex();
   int srcCols = sourceModel()->columnCount();
   if (proxyIndex.column() >= srcCols)
      return QModelIndex();
   return QIdentityProxyModel::mapToSource(proxyIndex);
}

QVariant PeerSpeedProxy::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
   {
      if (section == 1) return QString("Peer");
      if (section == 2) return QString("Shared");
      if (section == 3) return QString("Speed");
      if (section == 4) return QString("Version");
      if (section == 5) return QString("IP");
   }
   return QIdentityProxyModel::headerData(section, orientation, role);
}

QVariant PeerSpeedProxy::data(const QModelIndex& proxyIndex, int role) const
{
   const int col = proxyIndex.column();

   if (col == 3) // Speed
   {
      if (role == Qt::DisplayRole)
      {
         QModelIndex col0 = index(proxyIndex.row(), 0, proxyIndex.parent());
         auto ti = QIdentityProxyModel::data(col0, Qt::DisplayRole).value<PeerListModel::TransferInformation>();
         if (ti.lanSpeed > 0)
         {
            quint64 mbps = static_cast<quint64>(ti.lanSpeed) * 8 / 1000000ULL;
            if (mbps >= 1000)
               return QString(QString::number(mbps / 1000) + " Gbps");
            return QString(QString::number(mbps) + " Mbps");
         }
         return QString();
      }
      if (role == Qt::TextAlignmentRole)
         return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
   }
   else if (col == 4) // Version
   {
      if (role == Qt::DisplayRole)
      {
         QModelIndex col0 = index(proxyIndex.row(), 0, proxyIndex.parent());
         return QIdentityProxyModel::data(col0, PeerListModel::ROLE_VERSION);
      }
      if (role == Qt::TextAlignmentRole)
         return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
   }
   else if (col == 5) // IP
   {
      if (role == Qt::DisplayRole)
      {
         QModelIndex col0 = index(proxyIndex.row(), 0, proxyIndex.parent());
         return QIdentityProxyModel::data(col0, PeerListModel::ROLE_IP);
      }
      if (role == Qt::TextAlignmentRole)
         return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
   }
   return QIdentityProxyModel::data(proxyIndex, role);
}

// ── ProgressDelegate ─────────────────────────────────────────────────────────

void ProgressDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
   if (!index.isValid())
      return;

   QStyledItemDelegate::paint(painter, option, QModelIndex()); // Draw background/selection.

   int progress = index.data(NetworkWidget::ROLE_PROGRESS).toInt();
   if (progress <= 0)
      return;

   QStyleOptionProgressBar bar;
   bar.QStyleOption::operator=(option);
   bar.minimum = 0;
   bar.maximum = 10000;
   bar.progress = progress;
   bar.textAlignment = Qt::AlignHCenter | Qt::AlignVCenter;

   double pct = static_cast<double>(progress) / 100.0;
   bar.text = progress >= 10000 ? "100%" : QString("%1%").arg(pct > 99.99 ? 99.99 : pct, 0, 'f', pct < 10.0 ? 1 : 0);
   bar.textVisible = true;

   QApplication::style()->drawControl(QStyle::CE_ProgressBar, &bar, painter);
}
// ── FileSortProxy ─────────────────────────────────────────────────────────────────

bool FileSortProxy::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
   if (left.column() == NetworkWidget::COL_SIZE)
   {
      qulonglong lhs = sourceModel()->data(left, Qt::UserRole).toULongLong();
      qulonglong rhs = sourceModel()->data(right, Qt::UserRole).toULongLong();
      return lhs < rhs;
   }
   return QSortFilterProxyModel::lessThan(left, right);
}
// ── NetworkWidget ────────────────────────────────────────────────────────────

NetworkWidget::NetworkWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, PeerListModel& peerListModel, const SharedEntryListModel& sharedEntryListModel, QWidget* parent) :
   QWidget(parent),
   coreConnection(coreConnection),
   peerListModel(peerListModel),
   sharedEntryListModel(sharedEntryListModel),
   networkFileModel(sharedEntryListModel),
   downloadMenu(sharedEntryListModel){
   QVBoxLayout* mainLayout = new QVBoxLayout(this);
   mainLayout->setContentsMargins(0, 0, 0, 0);

   this->splitter = new QSplitter(Qt::Horizontal, this);
   mainLayout->addWidget(this->splitter);

   // ── Left panel: peer list ──
   this->peerTableView = new QTableView();
   this->peerProxy.setSourceModel(&this->peerListModel);
   this->peerTableView->setModel(&this->peerProxy);
   this->peerTableView->setColumnHidden(0, true);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
   this->peerTableView->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
   this->peerTableView->horizontalHeader()->setHighlightSections(false);
   this->peerTableView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
   this->peerTableView->verticalHeader()->setDefaultSectionSize(QApplication::fontMetrics().height() + 4);
   this->peerTableView->verticalHeader()->setVisible(false);
   this->peerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
   this->peerTableView->setShowGrid(false);
   this->peerTableView->setAlternatingRowColors(false);
   this->peerTableView->setMinimumWidth(200);
   this->splitter->addWidget(this->peerTableView);

   // ── Right panel: filter bar + unified file index ──
   QWidget* filePanel = new QWidget();
   QVBoxLayout* filePanelLayout = new QVBoxLayout(filePanel);
   filePanelLayout->setContentsMargins(0, 0, 0, 0);
   filePanelLayout->setSpacing(0);

   this->filterEdit = new QLineEdit();
   this->filterEdit->setPlaceholderText(tr("Filter files..."));
   this->filterEdit->setClearButtonEnabled(true);

   // ── Action buttons ──
   QHBoxLayout* buttonBar = new QHBoxLayout();
   buttonBar->setContentsMargins(2, 2, 2, 2);
   buttonBar->setSpacing(4);

   this->downloadButton = new QPushButton(tr("Download"));
   this->redownloadButton = new QPushButton(tr("Redownload"));
   this->deleteButton = new QPushButton(tr("Delete"));

   this->moveTopButton = new QPushButton(QString::fromUtf8("\u23EB"));     // ⏫
   this->moveUpButton = new QPushButton(QString::fromUtf8("\u25B2"));      // ▲
   this->moveDownButton = new QPushButton(QString::fromUtf8("\u25BC"));    // ▼
   this->moveBottomButton = new QPushButton(QString::fromUtf8("\u23EC"));  // ⏬

   this->downloadButton->setEnabled(false);
   this->redownloadButton->setEnabled(false);
   this->deleteButton->setEnabled(false);
   this->moveTopButton->setEnabled(false);
   this->moveUpButton->setEnabled(false);
   this->moveDownButton->setEnabled(false);
   this->moveBottomButton->setEnabled(false);

   this->moveTopButton->setToolTip(tr("Move to Top"));
   this->moveUpButton->setToolTip(tr("Move Up"));
   this->moveDownButton->setToolTip(tr("Move Down"));
   this->moveBottomButton->setToolTip(tr("Move to Bottom"));

   this->moveTopButton->setFixedWidth(28);
   this->moveUpButton->setFixedWidth(28);
   this->moveDownButton->setFixedWidth(28);
   this->moveBottomButton->setFixedWidth(28);

   this->openFolderButton = new QPushButton(QString::fromUtf8("\U0001F4C2"));
   this->openFolderButton->setToolTip(tr("Open Shared Folder"));
   this->openFolderButton->setFixedWidth(28);

   buttonBar->addWidget(this->downloadButton);
   buttonBar->addWidget(this->redownloadButton);
   buttonBar->addWidget(this->deleteButton);
   buttonBar->addSpacing(8);
   buttonBar->addWidget(this->moveTopButton);
   buttonBar->addWidget(this->moveUpButton);
   buttonBar->addWidget(this->moveDownButton);
   buttonBar->addWidget(this->moveBottomButton);
   buttonBar->addSpacing(8);
   buttonBar->addWidget(this->openFolderButton);
   buttonBar->addStretch();
   buttonBar->addWidget(this->filterEdit);

   filePanelLayout->addLayout(buttonBar);

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

   this->networkFileModel.itemModel().setHorizontalHeaderLabels({ tr("Name"), tr("Size"), tr("Status"), tr("#"), tr("Progress"), tr("DL Speed"), tr("UL Speed"), tr("Peers") });
   this->fileSortProxy.setSourceModel(&this->networkFileModel.itemModel());
   this->fileSortProxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
   this->fileSortProxy.setFilterKeyColumn(0);
   this->fileTableView->setModel(&this->fileSortProxy);

   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_NAME, QHeaderView::Stretch);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_SIZE, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_STATUS, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_QUEUE, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_PROGRESS, QHeaderView::Fixed);
   this->fileTableView->setColumnWidth(COL_PROGRESS, 120);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_DL_SPEED, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_UL_SPEED, QHeaderView::ResizeToContents);
   this->fileTableView->horizontalHeader()->setSectionResizeMode(COL_PEERS, QHeaderView::ResizeToContents);

   this->fileTableView->setItemDelegateForColumn(COL_PROGRESS, &this->progressDelegate);
   this->fileTableView->sortByColumn(COL_NAME, Qt::AscendingOrder);
   filePanelLayout->addWidget(this->fileTableView);

   this->splitter->addWidget(filePanel);
   this->splitter->setSizes({280, 600});
   this->splitter->setStretchFactor(0, 0);
   this->splitter->setStretchFactor(1, 1);

   // ── Connections ──
   connect(this->fileTableView, &QTableView::customContextMenuRequested, this, &NetworkWidget::displayContextMenuDownload);
   connect(this->filterEdit, &QLineEdit::textChanged, &this->fileSortProxy, &QSortFilterProxyModel::setFilterFixedString);
   connect(&this->downloadMenu, SIGNAL(download()), this, SLOT(download()));
   connect(&this->downloadMenu, SIGNAL(downloadTo()), this, SLOT(downloadTo()));
   connect(&this->downloadMenu, SIGNAL(downloadTo(const QString&, const Common::Hash&)), this, SLOT(downloadTo(const QString&, const Common::Hash&)));

   connect(this->downloadButton, &QPushButton::clicked, this, &NetworkWidget::download);
   connect(this->redownloadButton, &QPushButton::clicked, this, &NetworkWidget::redownload);
   connect(this->deleteButton, &QPushButton::clicked, this, &NetworkWidget::cancelDownload);
   connect(this->moveTopButton, &QPushButton::clicked, this, &NetworkWidget::moveToTop);
   connect(this->moveUpButton, &QPushButton::clicked, this, &NetworkWidget::moveUp);
   connect(this->moveDownButton, &QPushButton::clicked, this, &NetworkWidget::moveDown);
   connect(this->moveBottomButton, &QPushButton::clicked, this, &NetworkWidget::moveToBottom);
   connect(this->fileTableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NetworkWidget::updateButtonStates);
   connect(this->fileTableView, &QTableView::doubleClicked, this, &NetworkWidget::fileDoubleClicked);
   connect(this->openFolderButton, &QPushButton::clicked, this, &NetworkWidget::openFolder);

   connect(this->coreConnection.data(), &RCC::ICoreConnection::newState, this, &NetworkWidget::onNewState);
   connect(this->coreConnection.data(), SIGNAL(disconnected(bool)), this, SLOT(coreDisconnected()));

   // Re-browse all peers periodically to pick up changes.
   connect(&this->rebrowseTimer, &QTimer::timeout, this, [this]() {
      this->browsedPeers.clear();
      this->browseGeneration++;
      this->browsesPendingThisGen = 0;
   });
   this->rebrowseTimer.start(30000);

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

// ── Peer discovery & state handling ──────────────────────────────────────────

void NetworkWidget::onNewState(const Protos::GUI::State& state)
{
   // The first peer is always ourselves.
   if (state.peer_size() > 0)
      this->localPeerID = Common::Hash(state.peer(0).peer_id().hash().data());

   // Track every master as an authoritative file-list source. The highest
   // version master is used only for update enforcement.
   QSet<Common::Hash> newMasterIDs;
   Common::Hash newMasterID;
   QString masterVersion;
   QVersionNumber bestMasterVer;
   for (int i = 0; i < state.peer_size(); ++i)
   {
      if (!state.peer(i).is_master())
         continue;
      const Common::Hash peerID(state.peer(i).peer_id().hash().data());
      newMasterIDs.insert(peerID);
      const QString ver = QString::fromStdString(state.peer(i).core_version());
      const QVersionNumber peerVer = QVersionNumber::fromString(ver);
      if (peerVer > bestMasterVer)
      {
         bestMasterVer = peerVer;
         newMasterID = peerID;
         masterVersion = ver;
      }
   }

   // Check if the highest-version master is running a newer version than us.
   if (!this->updatePromptShown && !masterVersion.isEmpty() && !newMasterID.isNull() && newMasterID != this->localPeerID)
   {
      const QVersionNumber localVer = QVersionNumber::fromString(QString(VERSION));
      if (!bestMasterVer.isNull() && bestMasterVer > localVer)
      {
         this->updatePromptShown = true;

         // Trigger the built-in updater to fetch the installer from GitHub.
         auto* checker = new UpdateChecker(this);
         connect(checker, &UpdateChecker::updateAvailable, this, [this, checker, bestMasterVer, localVer]
            (QString latestVersion, QString releaseUrl, QString downloadUrl) {
            checker->deleteLater();
            UpdateDialog dlg(latestVersion, releaseUrl, downloadUrl, this, true);
            dlg.exec();
            // Forced update: quit regardless of dialog result.
            QApplication::quit();
         });
         connect(checker, &UpdateChecker::upToDate, this, [this, checker, bestMasterVer](QString) {
            checker->deleteLater();
            // Master says newer but GitHub says up-to-date — open releases page.
            QMessageBox box(this);
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle(tr("Update Required"));
            box.setText(tr("The master server is running v%1 but the updater could not find a newer release.\n"
                           "Please check GitHub for the latest version.")
                        .arg(bestMasterVer.toString()));
            box.addButton(tr("Open Downloads"), QMessageBox::AcceptRole);
            box.addButton(tr("Quit"), QMessageBox::RejectRole);
            box.exec();
            QDesktopServices::openUrl(QUrl("https://github.com/kwran420/DG-LAN/releases/latest"));
            QApplication::quit();
         });
         connect(checker, &UpdateChecker::checkFailed, this, [this, checker, bestMasterVer](QString) {
            checker->deleteLater();
            QMessageBox box(this);
            box.setIcon(QMessageBox::Warning);
            box.setWindowTitle(tr("Update Required"));
            box.setText(tr("The master server is running v%1 but the update check failed.\n"
                           "Please download the latest version from GitHub.")
                        .arg(bestMasterVer.toString()));
            box.addButton(tr("Open Downloads"), QMessageBox::AcceptRole);
            box.addButton(tr("Quit"), QMessageBox::RejectRole);
            box.exec();
            QDesktopServices::openUrl(QUrl("https://github.com/kwran420/DG-LAN/releases/latest"));
            QApplication::quit();
         });
         checker->check();
         return;
      }
   }

   if (newMasterIDs != this->masterPeerIDs)
   {
      this->masterPeerIDs = newMasterIDs;
      this->browsedPeers.clear();
      this->activeBrowseResults.clear();
      this->browseGeneration++;
      this->browsesPendingThisGen = 0;
   }

   // Browse every peer we haven't visited in this cycle.
   for (int i = 0; i < state.peer_size(); ++i)
   {
      Common::Hash peerID(state.peer(i).peer_id().hash().data());
      if (!this->browsedPeers.contains(peerID))
         this->browsePeer(peerID);
   }

   // Prune stale file rows only once per generation, after all browse results have returned.
   // This prevents rows from flickering away mid-browse while the user is interacting.
   if (this->activeBrowseResults.isEmpty() && this->browseGeneration > 0
       && this->lastPrunedGeneration < this->browseGeneration
       && this->browsesPendingThisGen == 0)
   {
      this->lastPrunedGeneration = this->browseGeneration;
      this->networkFileModel.pruneStaleRows(this->browseGeneration);
      if (this->lastLoggedGeneration < this->browseGeneration)
      {
         this->lastLoggedGeneration = this->browseGeneration;
         L_USER(QString("[Network] File index updated — %1 files from %2 peers")
            .arg(this->networkFileModel.rowCount()).arg(this->browsedPeers.size()));
      }
   }

   // Update download/upload columns from the state.
   this->networkFileModel.updateFromState(state, this->browseGeneration);
}

void NetworkWidget::coreDisconnected()
{
   this->activeBrowseResults.clear();
   this->masterPeerIDs.clear();
   this->browsedPeers.clear();
   this->networkFileModel.clear();
}

// ── Browse peers ─────────────────────────────────────────────────────────────

void NetworkWidget::browsePeer(const Common::Hash& peerID)
{
   this->browsedPeers.insert(peerID);
   this->browsesPendingThisGen++;
   auto browseResult = this->coreConnection->browse(peerID);

   QByteArray idData(peerID.getData(), Common::Hash::HASH_SIZE);
   browseResult->setProperty("peerID", idData);

   connect(browseResult.data(), SIGNAL(result(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)),
           this, SLOT(browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>&)));
   connect(browseResult.data(), &RCC::IBrowseResult::timeout, [this, browseResult]() {
      this->activeBrowseResults.removeOne(browseResult);
      if (this->browsesPendingThisGen > 0)
         this->browsesPendingThisGen--;
   });
   browseResult->start();
   this->activeBrowseResults.append(browseResult);
}

void NetworkWidget::browseRootResult(const google::protobuf::RepeatedPtrField<Protos::Common::Entries>& entries)
{
   auto* senderObj = sender();
   QByteArray idData = senderObj->property("peerID").toByteArray();
   Common::Hash peerID(idData.constData());
   bool fromMaster = this->masterPeerIDs.contains(peerID);

   for (int i = 0; i < this->activeBrowseResults.size(); ++i)
   {
      if (this->activeBrowseResults[i].data() == senderObj)
      {
         this->activeBrowseResults.removeAt(i);
         break;
      }
   }
   if (this->browsesPendingThisGen > 0)
      this->browsesPendingThisGen--;

   for (int i = 0; i < entries.size(); ++i)
   {
      const auto& entryList = entries.Get(i);
      for (int j = 0; j < entryList.entry_size(); ++j)
      {
         const auto& entry = entryList.entry(j);
         if (entry.type() == Protos::Common::Entry::DIR)
            this->browseDir(peerID, entry);
         else
            this->addFileEntry(entry, peerID, fromMaster);
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
   bool fromMaster = this->masterPeerIDs.contains(peerID);

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
            this->addFileEntry(entry, peerID, fromMaster);
      }
   }
}

// ── File model helpers ───────────────────────────────────────────────────────

void NetworkWidget::addFileEntry(const Protos::Common::Entry& entry, const Common::Hash& peerID, bool fromMaster)
{
   this->networkFileModel.addBrowseEntry(entry, peerID, this->localPeerID, fromMaster, this->browseGeneration);
}

// ── Context menu & download actions ──────────────────────────────────────────

void NetworkWidget::displayContextMenuDownload(const QPoint& point)
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   QMenu menu;
   QAction* dlAction = menu.addAction(tr("Download"));
   QAction* redlAction = menu.addAction(tr("Redownload"));
   QAction* delAction = menu.addAction(tr("Delete"));

   // Check if any selected file is owned locally and has a known path.
   bool canOpenLocation = false;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
       if (item && item->data(ROLE_OWNED).toBool() && !this->networkFileModel.getLocalPath(item).isEmpty())
      {
         canOpenLocation = true;
         break;
      }
   }

   menu.addSeparator();
   QAction* openLocAction = menu.addAction(tr("Open file location"));
   openLocAction->setEnabled(canOpenLocation);

   // Queue reorder actions — only enabled when at least one selected item has an active download.
   bool hasActiveDownload = false;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (item && item->data(ROLE_QUEUE_POS).toInt() > 0)
      {
         hasActiveDownload = true;
         break;
      }
   }

   menu.addSeparator();
   QAction* moveTopAction = menu.addAction(tr("Move to Top"));
   QAction* moveUpAction = menu.addAction(tr("Move Up"));
   QAction* moveDownAction = menu.addAction(tr("Move Down"));
   QAction* moveBottomAction = menu.addAction(tr("Move to Bottom"));
   moveTopAction->setEnabled(hasActiveDownload);
   moveUpAction->setEnabled(hasActiveDownload);
   moveDownAction->setEnabled(hasActiveDownload);
   moveBottomAction->setEnabled(hasActiveDownload);

   QAction* chosen = menu.exec(this->fileTableView->mapToGlobal(point));
   if (chosen == dlAction)
      this->download();
   else if (chosen == redlAction)
      this->redownload();
   else if (chosen == delAction)
      this->cancelDownload();
   else if (chosen == openLocAction)
      this->openFileLocation();
   else if (chosen == moveTopAction)
      this->moveToTop();
   else if (chosen == moveUpAction)
      this->moveUp();
   else if (chosen == moveDownAction)
      this->moveDown();
   else if (chosen == moveBottomAction)
      this->moveToBottom();
}

void NetworkWidget::openFileLocation()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (!item || !item->data(ROLE_OWNED).toBool())
         continue;

       QString path = this->networkFileModel.getLocalPath(item);
      if (!path.isEmpty())
         Utils::openLocation(path);
   }
}

void NetworkWidget::fileDoubleClicked(const QModelIndex& index)
{
   QModelIndex sourceIndex = this->fileSortProxy.mapToSource(index);
   QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
   if (!item)
      return;

   // If owned locally, open the file's location in Explorer.
   if (item->data(ROLE_OWNED).toBool())
   {
       QString path = this->networkFileModel.getLocalPath(item);
      if (!path.isEmpty())
      {
         Utils::openLocation(path);
         return;
      }
   }

   // Otherwise, download if not already in the queue.
   if (item->data(ROLE_DOWNLOAD_ID).toULongLong() != 0)
      return;

   QByteArray entryData = item->data(ROLE_ENTRY).toByteArray();
   QByteArray peerIdData = item->data(ROLE_PEER_ID).toByteArray();
   if (entryData.isEmpty() || peerIdData.isEmpty())
      return;

   Common::Hash peerID(peerIdData.constData());
   if (peerID == this->localPeerID)
      return;

   Protos::Common::Entry entry;
   if (!entry.ParseFromArray(entryData.constData(), entryData.size()))
      return;

   this->coreConnection->download(peerID, entry);
}

void NetworkWidget::openFolder()
{
   QList<Common::SharedEntry> dirs = this->sharedEntryListModel.getSharedDirectories();
   if (dirs.isEmpty())
      return;
   Utils::openLocation(dirs.first().path.getPath());
}

void NetworkWidget::moveToTop()
{
   const QList<quint64>& downloadQueue = this->networkFileModel.downloadQueue();
   if (downloadQueue.isEmpty())
      return;

   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   QList<quint64> idsToMove;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (item)
      {
         quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
         if (dlId != 0 && item->data(ROLE_QUEUE_POS).toInt() > 0)
            idsToMove.append(dlId);
      }
   }
   if (idsToMove.isEmpty())
      return;

   this->coreConnection->moveDownloads(downloadQueue.first(), idsToMove, Protos::GUI::MoveDownloads::BEFORE);
}

void NetworkWidget::moveUp()
{
   const QList<quint64>& downloadQueue = this->networkFileModel.downloadQueue();
   if (downloadQueue.size() < 2)
      return;

   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   int minPos = INT_MAX;
   QList<quint64> idsToMove;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (item)
      {
         int pos = item->data(ROLE_QUEUE_POS).toInt();
         quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
         if (dlId != 0 && pos > 0)
         {
            idsToMove.append(dlId);
            if (pos < minPos)
               minPos = pos;
         }
      }
   }
   if (idsToMove.isEmpty() || minPos <= 1)
      return;

   // The item just above the topmost selected is at downloadQueue index minPos-2.
   quint64 refId = downloadQueue.at(minPos - 2);
   this->coreConnection->moveDownloads(refId, idsToMove, Protos::GUI::MoveDownloads::BEFORE);
}

void NetworkWidget::moveDown()
{
   const QList<quint64>& downloadQueue = this->networkFileModel.downloadQueue();
   if (downloadQueue.size() < 2)
      return;

   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   int maxPos = 0;
   QList<quint64> idsToMove;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (item)
      {
         int pos = item->data(ROLE_QUEUE_POS).toInt();
         quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
         if (dlId != 0 && pos > 0)
         {
            idsToMove.append(dlId);
            if (pos > maxPos)
               maxPos = pos;
         }
      }
   }
   if (idsToMove.isEmpty() || maxPos >= downloadQueue.size())
      return;

   // The item just below the bottommost selected is at downloadQueue index maxPos.
   quint64 refId = downloadQueue.at(maxPos);
   this->coreConnection->moveDownloads(refId, idsToMove, Protos::GUI::MoveDownloads::AFTER);
}

void NetworkWidget::moveToBottom()
{
   const QList<quint64>& downloadQueue = this->networkFileModel.downloadQueue();
   if (downloadQueue.isEmpty())
      return;

   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   QList<quint64> idsToMove;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (item)
      {
         quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
         if (dlId != 0 && item->data(ROLE_QUEUE_POS).toInt() > 0)
            idsToMove.append(dlId);
      }
   }
   if (idsToMove.isEmpty())
      return;

   this->coreConnection->moveDownloads(downloadQueue.last(), idsToMove, Protos::GUI::MoveDownloads::AFTER);
}

void NetworkWidget::download()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
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

      // Don't download from ourselves.
      if (peerID == this->localPeerID)
         continue;

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
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
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

      // Don't download from ourselves.
      if (peerID == this->localPeerID)
         continue;

      this->coreConnection->download(peerID, entry, sharedDirID, path);
   }
}

void NetworkWidget::redownload()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   // Cancel existing downloads first, then re-issue.
   QList<quint64> toCancel;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (!item)
         continue;
      quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
      if (dlId != 0)
         toCancel.append(dlId);
   }

   if (!toCancel.isEmpty())
      this->coreConnection->cancelDownloads(toCancel);

   this->download();
}

void NetworkWidget::cancelDownload()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   if (selectedRows.isEmpty())
      return;

   QList<quint64> toCancel;
   QStringList names;
   for (const QModelIndex& proxyIndex : selectedRows)
   {
       QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (!item)
         continue;
      quint64 dlId = item->data(ROLE_DOWNLOAD_ID).toULongLong();
      if (dlId != 0)
      {
         toCancel.append(dlId);
         names.append(item->text());
      }
   }

   if (toCancel.isEmpty())
      return;

   int ret = QMessageBox::question(
      this,
      tr("Delete Downloads"),
      tr("Delete %n download(s) and their files?\n%1", "", toCancel.size()).arg(names.join(", ")),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No
   );

   if (ret == QMessageBox::Yes)
      this->coreConnection->cancelDownloads(toCancel);
}

void NetworkWidget::updateButtonStates()
{
   QModelIndexList selectedRows = this->fileTableView->selectionModel()->selectedRows();
   bool canDownload = false;
   bool hasDownload = false;
   bool hasQueuedDownload = false;

   QString localHex = QString::fromLatin1(QByteArray(this->localPeerID.getData(), Common::Hash::HASH_SIZE).toHex());

   for (const QModelIndex& proxyIndex : selectedRows)
   {
      QModelIndex sourceIndex = this->fileSortProxy.mapToSource(proxyIndex);
       QStandardItem* item = this->networkFileModel.itemFromIndex(sourceIndex);
      if (!item)
         continue;

      if (item->data(ROLE_DOWNLOAD_ID).toULongLong() != 0)
         hasDownload = true;

      if (item->data(ROLE_QUEUE_POS).toInt() > 0)
         hasQueuedDownload = true;

      // Can download if any non-local peer has this file.
      QStringList peerIds = item->data(ROLE_PEER_IDS).toStringList();
      for (const QString& pid : peerIds)
      {
         if (pid != localHex)
         {
            canDownload = true;
            break;
         }
      }
   }

   this->downloadButton->setEnabled(canDownload);
   this->redownloadButton->setEnabled(canDownload);
   this->deleteButton->setEnabled(hasDownload);
   this->moveTopButton->setEnabled(hasQueuedDownload);
   this->moveUpButton->setEnabled(hasQueuedDownload);
   this->moveDownButton->setEnabled(hasQueuedDownload);
   this->moveBottomButton->setEnabled(hasQueuedDownload);
}
