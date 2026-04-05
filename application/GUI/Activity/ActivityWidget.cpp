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

#include <Activity/ActivityWidget.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QEvent>

#include <Common/Global.h>
#include <Common/ProtoHelper.h>

// ─── ActivityWidget ──────────────────────────────────────────────────────────

ActivityWidget::ActivityWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, QWidget* parent)
   : QWidget(parent),
     coreConnection(coreConnection)
{
   // ── Layout ────────────────────────────────────────────────────────────────
   QVBoxLayout* vlay = new QVBoxLayout(this);
   vlay->setContentsMargins(4, 4, 4, 4);
   vlay->setSpacing(4);

   // Title bar row
   QHBoxLayout* hlay = new QHBoxLayout();

   lblTitle = new QLabel(tr("Activity Log"), this);
   QFont f = lblTitle->font();
   f.setBold(true);
   lblTitle->setFont(f);

   btnClear = new QPushButton(tr("Clear"), this);
   btnClear->setFixedWidth(60);

   hlay->addWidget(lblTitle);
   hlay->addStretch();
   hlay->addWidget(btnClear);

   vlay->addLayout(hlay);

   // Event list
   listWidget = new QListWidget(this);
   listWidget->setAlternatingRowColors(true);
   listWidget->setWordWrap(true);
   listWidget->setSelectionMode(QAbstractItemView::NoSelection);
   listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   vlay->addWidget(listWidget);

   setLayout(vlay);
   setWindowTitle(tr("Activity"));

   // ── Connections ───────────────────────────────────────────────────────────
   connect(btnClear, SIGNAL(clicked()), this, SLOT(clearLog()));
   connect(coreConnection.data(), SIGNAL(newState(const Protos::GUI::State&)), this, SLOT(newState(const Protos::GUI::State&)));
   connect(coreConnection.data(), SIGNAL(connected()), this, SLOT(coreConnected()));
   connect(coreConnection.data(), SIGNAL(disconnected(bool)), this, SLOT(coreDisconnected(bool)));
}

ActivityWidget::~ActivityWidget()
{
}

void ActivityWidget::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
   {
      lblTitle->setText(tr("Activity Log"));
      btnClear->setText(tr("Clear"));
      setWindowTitle(tr("Activity"));
   }
   QWidget::changeEvent(event);
}

// ─── Slots ───────────────────────────────────────────────────────────────────

void ActivityWidget::coreConnected()
{
   addEvent(EV_INFO, tr("Connected to core"));
   prevPeers.clear();
   prevUploadIds.clear();
   prevDownloadIds.clear();
   prevCacheStatus = -1;
}

void ActivityWidget::coreDisconnected(bool forced)
{
   addEvent(EV_WARN, forced ? tr("Connection to core lost") : tr("Disconnected from core"));
   prevPeers.clear();
   prevUploadIds.clear();
   prevDownloadIds.clear();
   prevCacheStatus = -1;
}

void ActivityWidget::newState(const Protos::GUI::State& state)
{
   // ── Peer join / leave detection ───────────────────────────────────────────
   QHash<QByteArray, QString> currPeers;
   // state.peer(0) is always ourself; skip index 0 for join/leave events
   for (int i = 1; i < state.peer_size(); i++)
   {
      QByteArray id = QByteArray::fromStdString(state.peer(i).peer_id().hash());
      QString nick = Common::ProtoHelper::getStr(state.peer(i), &Protos::GUI::State::Peer::nick);
      currPeers.insert(id, nick);

      if (!prevPeers.contains(id))
      {
         const quint64 sharing = state.peer(i).sharing_amount();
         addEvent(EV_PEER_JOIN,
            tr("Peer joined: %1  [sharing %2]")
               .arg(nick)
               .arg(Common::Global::formatByteSize(sharing)));
      }
   }

   for (auto it = prevPeers.cbegin(); it != prevPeers.cend(); ++it)
      if (!currPeers.contains(it.key()))
         addEvent(EV_PEER_LEAVE, tr("Peer left: %1").arg(it.value()));

   prevPeers = currPeers;

   // ── Cache / indexing status changes ──────────────────────────────────────
   const int cacheStatus = static_cast<int>(state.stats().cache_status());
   if (cacheStatus != prevCacheStatus)
   {
      switch (state.stats().cache_status())
      {
      case Protos::GUI::State::Stats::LOADING_CACHE_IN_PROGRESS:
         addEvent(EV_HASH, tr("Loading file cache from disk..."));
         break;
      case Protos::GUI::State::Stats::SCANNING_IN_PROGRESS:
         addEvent(EV_HASH, tr("Scanning shared directories for new/changed files..."));
         break;
      case Protos::GUI::State::Stats::HASHING_IN_PROGRESS:
         addEvent(EV_HASH, tr("Hashing files to build content index..."));
         break;
      case Protos::GUI::State::Stats::UP_TO_DATE:
         addEvent(EV_INFO, tr("File index is up to date"));
         break;
      default:
         break;
      }
      prevCacheStatus = cacheStatus;
   }

   // ── Upload events (our files being sent to others) ────────────────────────
   QSet<quint64> currUploadIds;
   for (int i = 0; i < state.upload_size(); i++)
   {
      const quint64 uid = state.upload(i).id();
      currUploadIds.insert(uid);
      if (!prevUploadIds.contains(uid))
      {
         const QString fileName = Common::ProtoHelper::getStr(state.upload(i).file(), &Protos::Common::Entry::name);
         addEvent(EV_UPLOAD, tr("Upload started: %1").arg(fileName));
      }
   }
   for (quint64 uid : prevUploadIds)
      if (!currUploadIds.contains(uid))
         addEvent(EV_UPLOAD, tr("Upload completed"));

   prevUploadIds = currUploadIds;

   // ── Download events ───────────────────────────────────────────────────────
   QSet<quint64> currDownloadIds;
   for (int i = 0; i < state.download_size(); i++)
   {
      const quint64 did = state.download(i).id();
      currDownloadIds.insert(did);
      if (!prevDownloadIds.contains(did))
      {
         const QString fileName = Common::ProtoHelper::getStr(state.download(i).local_entry(), &Protos::Common::Entry::name);
         addEvent(EV_DOWNLOAD, tr("Download queued: %1  [from %2]")
            .arg(fileName)
            .arg(Common::ProtoHelper::getStr(state.download(i), &Protos::GUI::State::Download::peer_source_nick)));
      }
   }
   prevDownloadIds = currDownloadIds;
}

void ActivityWidget::clearLog()
{
   listWidget->clear();
}

// ─── Private helpers ─────────────────────────────────────────────────────────

void ActivityWidget::addEvent(EventType type, const QString& message)
{
   trimLog();

   const QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
   QString prefix;
   QColor  color;
   QColor  bg;

   switch (type)
   {
   case EV_PEER_JOIN:
      prefix = "[+peer] "; color = QColor(0, 130, 0);   bg = QColor(235, 255, 235); break;
   case EV_PEER_LEAVE:
      prefix = "[-peer] "; color = QColor(180, 60, 0);  bg = QColor(255, 240, 235); break;
   case EV_UPLOAD:
      prefix = "[upload]"; color = QColor(0, 80, 180);  bg = QColor(235, 242, 255); break;
   case EV_DOWNLOAD:
      prefix = "[dload] "; color = QColor(100, 0, 180); bg = QColor(245, 235, 255); break;
   case EV_HASH:
      prefix = "[index] "; color = QColor(120, 80, 0);  bg = QColor(255, 252, 230); break;
   case EV_WARN:
      prefix = "[warn]  "; color = QColor(180, 0, 0);   bg = QColor(255, 235, 235); break;
   default: // EV_INFO
      prefix = "[info]  "; color = QColor(60, 60, 60);  bg = QColor(248, 248, 248); break;
   }

   QListWidgetItem* item = new QListWidgetItem(
      QString("%1  %2  %3").arg(ts).arg(prefix).arg(message));
   item->setForeground(color);
   item->setBackground(bg);
   listWidget->addItem(item);

   // Auto-scroll to bottom
   listWidget->scrollToBottom();
}

void ActivityWidget::trimLog()
{
   while (listWidget->count() >= MAX_EVENTS)
      delete listWidget->takeItem(0);
}
