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

#include <Hashing/HashingProgressWidget.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFrame>
#include <QFont>
#include <QSizePolicy>
#include <QEvent>

#include <Common/Global.h>
#include <Common/ProtoHelper.h>

// ─── HashingProgressWidget ───────────────────────────────────────────────────

HashingProgressWidget::HashingProgressWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, QWidget* parent)
   : QWidget(parent),
     coreConnection(coreConnection)
{
   QVBoxLayout* vlay = new QVBoxLayout(this);
   vlay->setContentsMargins(8, 8, 8, 8);
   vlay->setSpacing(8);

   // ── Status banner ─────────────────────────────────────────────────────────
   QFrame* statusFrame = new QFrame(this);
   statusFrame->setFrameShape(QFrame::StyledPanel);
   statusFrame->setFrameShadow(QFrame::Sunken);
   QVBoxLayout* sfLay = new QVBoxLayout(statusFrame);
   sfLay->setContentsMargins(8, 6, 8, 6);
   sfLay->setSpacing(4);

   // Icon + text row
   QHBoxLayout* statusRow = new QHBoxLayout();
   lblStatusIcon = new QLabel("●", statusFrame);
   QFont iconFont = lblStatusIcon->font();
   iconFont.setPointSize(16);
   lblStatusIcon->setFont(iconFont);
   lblStatusIcon->setFixedWidth(24);

   lblStatusText = new QLabel(tr("Waiting for core..."), statusFrame);
   QFont textFont = lblStatusText->font();
   textFont.setBold(true);
   textFont.setPointSize(11);
   lblStatusText->setFont(textFont);
   lblStatusText->setWordWrap(true);

   statusRow->addWidget(lblStatusIcon);
   statusRow->addWidget(lblStatusText, 1);
   sfLay->addLayout(statusRow);

   // Progress bar
   progressBar = new QProgressBar(statusFrame);
   progressBar->setRange(0, 10000);
   progressBar->setValue(0);
   progressBar->setTextVisible(false);
   progressBar->setFixedHeight(14);
   progressBar->setVisible(false);
   sfLay->addWidget(progressBar);

   // Percent label + elapsed time
   QHBoxLayout* pctRow = new QHBoxLayout();
   lblProgressPct = new QLabel("", statusFrame);
   lblSinceTime   = new QLabel("", statusFrame);
   lblSinceTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
   pctRow->addWidget(lblProgressPct);
   pctRow->addStretch();
   pctRow->addWidget(lblSinceTime);
   sfLay->addLayout(pctRow);

   vlay->addWidget(statusFrame);

   // ── Shared directories table ──────────────────────────────────────────────
   QLabel* lblDirsTitle = new QLabel(tr("Shared directories:"), this);
   QFont df = lblDirsTitle->font();
   df.setBold(true);
   lblDirsTitle->setFont(df);
   vlay->addWidget(lblDirsTitle);

   tblDirs = new QTableWidget(0, 3, this);
   tblDirs->setHorizontalHeaderLabels(QStringList() << tr("Path") << tr("Total size") << tr("Free space"));
   tblDirs->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
   tblDirs->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   tblDirs->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
   tblDirs->horizontalHeader()->setStretchLastSection(false);
   tblDirs->verticalHeader()->hide();
   tblDirs->setEditTriggers(QAbstractItemView::NoEditTriggers);
   tblDirs->setSelectionMode(QAbstractItemView::SingleSelection);
   tblDirs->setSelectionBehavior(QAbstractItemView::SelectRows);
   tblDirs->setAlternatingRowColors(true);
   tblDirs->setShowGrid(false);
   vlay->addWidget(tblDirs, 1);

   setLayout(vlay);
   setWindowTitle(tr("Indexing"));

   // ── Connections ───────────────────────────────────────────────────────────
   connect(coreConnection.data(), SIGNAL(newState(const Protos::GUI::State&)), this, SLOT(newState(const Protos::GUI::State&)));
   connect(coreConnection.data(), SIGNAL(connected()), this, SLOT(coreConnected()));
   connect(coreConnection.data(), SIGNAL(disconnected(bool)), this, SLOT(coreDisconnected(bool)));
}

HashingProgressWidget::~HashingProgressWidget()
{
}

void HashingProgressWidget::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
   {
      setWindowTitle(tr("Indexing"));
      tblDirs->setHorizontalHeaderLabels(QStringList() << tr("Path") << tr("Total size") << tr("Free space"));
   }
   QWidget::changeEvent(event);
}

// ─── Slots ───────────────────────────────────────────────────────────────────

void HashingProgressWidget::coreConnected()
{
   updateStatusLabel(Protos::GUI::State::Stats::UNKNOWN, 0);
}

void HashingProgressWidget::coreDisconnected(bool)
{
   lblStatusIcon->setText("○");
   lblStatusIcon->setStyleSheet("color: #888;");
   lblStatusText->setText(tr("Not connected"));
   progressBar->setVisible(false);
   lblProgressPct->setText("");
   lblSinceTime->setText("");
   tblDirs->setRowCount(0);
   lastStatus = Protos::GUI::State::Stats::UNKNOWN;
}

void HashingProgressWidget::newState(const Protos::GUI::State& state)
{
   const auto status = state.stats().cache_status();
   const int  progress = static_cast<int>(state.stats().progress());

   if (status != lastStatus)
   {
      operationStartTime = QDateTime::currentDateTime();
      lastStatus = status;
   }

   updateStatusLabel(status, progress);
   updateSharedDirs(state);
}

// ─── Private helpers ─────────────────────────────────────────────────────────

void HashingProgressWidget::updateStatusLabel(Protos::GUI::State::Stats::CacheStatus status, int progress)
{
   QString icon, text, iconColor;
   bool showProgress = false;

   switch (status)
   {
   case Protos::GUI::State::Stats::LOADING_CACHE_IN_PROGRESS:
      icon = "◌"; iconColor = "#E8A000";
      text = tr("Loading cache from disk...");
      showProgress = true;
      break;

   case Protos::GUI::State::Stats::SCANNING_IN_PROGRESS:
      icon = "↺"; iconColor = "#0068C0";
      text = tr("Scanning — discovering files and directories in shared paths");
      showProgress = false;
      break;

   case Protos::GUI::State::Stats::HASHING_IN_PROGRESS:
      icon = "⚙"; iconColor = "#C06000";
      text = tr("Hashing — computing content fingerprints for shared files");
      showProgress = true;
      break;

   case Protos::GUI::State::Stats::UP_TO_DATE:
      icon = "✔"; iconColor = "#007800";
      text = tr("Index is up to date — all shared files are hashed");
      showProgress = false;
      progress = 0;
      break;

   default:
      icon = "●"; iconColor = "#888888";
      text = tr("Waiting for core status...");
      showProgress = false;
      break;
   }

   lblStatusIcon->setText(icon);
   lblStatusIcon->setStyleSheet(QString("color: %1;").arg(iconColor));
   lblStatusText->setText(text);

   progressBar->setVisible(showProgress);
   if (showProgress)
   {
      progressBar->setValue(progress);
      const double pct = progress / 100.0;
      lblProgressPct->setText(QString("%1%").arg(pct, 0, 'f', 1));

      if (operationStartTime.isValid())
      {
         const qint64 secs = operationStartTime.secsTo(QDateTime::currentDateTime());
         if (secs < 60)
            lblSinceTime->setText(tr("Running for %1s").arg(secs));
         else
            lblSinceTime->setText(tr("Running for %1m %2s").arg(secs / 60).arg(secs % 60));
      }
   }
   else
   {
      lblProgressPct->setText("");
      lblSinceTime->setText("");
   }
}

void HashingProgressWidget::updateSharedDirs(const Protos::GUI::State& state)
{
   const int count = state.shared_entry_size();
   tblDirs->setRowCount(count);

   for (int i = 0; i < count; i++)
   {
      const auto& se = state.shared_entry(i);

      // Path
      const QString path = Common::ProtoHelper::getStr(se.entry(), &Protos::Common::SharedEntry::path);
      QTableWidgetItem* pathItem = new QTableWidgetItem(path);
      pathItem->setToolTip(path);
      tblDirs->setItem(i, 0, pathItem);

      // Total size
      QTableWidgetItem* sizeItem = new QTableWidgetItem(Common::Global::formatByteSize(se.size()));
      sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
      tblDirs->setItem(i, 1, sizeItem);

      // Free space
      QTableWidgetItem* freeItem = new QTableWidgetItem(Common::Global::formatByteSize(se.free_space()));
      freeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
      tblDirs->setItem(i, 2, freeItem);
   }
}
