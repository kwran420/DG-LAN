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
  
#include <Log/LogModel.h>
using namespace GUI;

#include <QDateTime>

#include <Common/Settings.h>
#include <Common/Global.h>
#include <Common/LogManager/Builder.h>

LogModel::LogModel(QSharedPointer<RCC::ICoreConnection> coreConnection) :
   coreConnection(coreConnection),
   prevCacheStatus(-1)
{
   connect(this->coreConnection.data(), &RCC::ICoreConnection::newLogMessages, this, &LogModel::newLogEntries);

   this->loggerHook = LM::Builder::newLoggerHook(LM::Severity(LM::SV_FATAL_ERROR | LM::SV_ERROR | LM::SV_END_USER | LM::SV_WARNING));
   connect(this->loggerHook.data(), &LM::ILoggerHook::newLogEntry, this, &LogModel::newLogEntry);

   connect(this->coreConnection.data(), &RCC::ICoreConnection::newState, this, &LogModel::newState);
   connect(this->coreConnection.data(), &RCC::ICoreConnection::connected, this, &LogModel::coreConnected);
   connect(this->coreConnection.data(), &RCC::ICoreConnection::disconnected, this, &LogModel::coreDisconnected);
}

int LogModel::rowCount(const QModelIndex& /*parent*/) const
{
   return this->entries.size();
}

int LogModel::columnCount(const QModelIndex& /*parent*/) const
{
   return 2;
}

QVariant LogModel::data(const QModelIndex& index, int role) const
{
   if (role != Qt::DisplayRole || index.row() >= this->entries.size())
      return QVariant();

   const QSharedPointer<LM::IEntry>& entry = this->entries[index.row()];

   switch (index.column())
   {
   case 0:
      return entry->getDateStr(false);

   case 1:
      {
         QString message;
         switch (entry->getSeverity())
         {
         case LM::SV_FATAL_ERROR:
            message.append("[Fatal Error] ");
            break;
         case LM::SV_ERROR:
            message.append("[Error] ");
            break;
         default:;
         }
         message.append(entry->getMessage());
         return message;
      }

   default:
      return QVariant();
   }
}

LM::Severity LogModel::getSeverity(int row) const
{
   if (row >= this->entries.size())
      return LM::SV_UNKNOWN;
   return this->entries[row]->getSeverity();
}

void LogModel::newLogEntry(QSharedPointer<LM::IEntry> entry)
{
   this->newLogEntries(QList<QSharedPointer<LM::IEntry>> { entry });
}

void LogModel::newLogEntries(const QList<QSharedPointer<LM::IEntry>>& entries)
{
   QList<QSharedPointer<LM::IEntry>> filteredEntries;

   // Report Warnings only in DEBUG mode and do not repeat several same messages.
   for (QListIterator<QSharedPointer<LM::IEntry>> i(entries); i.hasNext();)
   {
      const QSharedPointer<LM::IEntry>& entry = i.next();
#ifndef DEBUG
      if (entry->getSeverity() != LM::SV_WARNING)
#endif
      {
         if (filteredEntries.isEmpty() || entry->getMessage() != filteredEntries.last()->getMessage())
            filteredEntries << entry;
      }
   }

   if (!filteredEntries.isEmpty() && !this->entries.isEmpty() && filteredEntries.last()->getMessage() == this->entries.last()->getMessage())
      filteredEntries.removeLast();

   if (filteredEntries.isEmpty())
      return;

   this->beginInsertRows(QModelIndex(), this->entries.size(), this->entries.size() + filteredEntries.size() - 1);
   this->entries << filteredEntries;
   this->endInsertRows();

   static const quint32 MAX_LOG_MESSAGE_DISPLAYED = SETTINGS.get<quint32>("max_log_message_displayed");
   if (quint32(this->entries.size()) > MAX_LOG_MESSAGE_DISPLAYED)
   {
      this->beginRemoveRows(QModelIndex(), 0, quint32(this->entries.size()) - MAX_LOG_MESSAGE_DISPLAYED - 1);
      this->entries.erase(this->entries.begin(), this->entries.begin() + (quint32(this->entries.size()) - MAX_LOG_MESSAGE_DISPLAYED));
      this->endRemoveRows();
   }
}

void LogModel::newState(const Protos::GUI::State& state)
{
   QList<QSharedPointer<LM::IEntry>> batch;
   const QDateTime now = QDateTime::currentDateTime();

   // ── Peer join / leave detection (index 0 = ourself, skip) ────────────────
   QHash<QByteArray, QString> currPeers;
   for (int i = 1; i < state.peer_size(); ++i)
   {
      const QByteArray id = QByteArray::fromStdString(state.peer(i).peer_id().hash());
      const QString nick = QString::fromStdString(state.peer(i).nick());
      currPeers.insert(id, nick);

      if (!this->prevPeers.contains(id))
      {
         const quint64 sharing = state.peer(i).sharing_amount();
         batch << LM::Builder::newEntry(now, LM::SV_END_USER,
            QString("[Network] %1 joined  (sharing %2)").arg(nick).arg(Common::Global::formatByteSize(sharing)));
      }
   }
   for (auto it = this->prevPeers.cbegin(); it != this->prevPeers.cend(); ++it)
      if (!currPeers.contains(it.key()))
         batch << LM::Builder::newEntry(now, LM::SV_END_USER,
            QString("[Network] %1 left the network").arg(it.value()));
   this->prevPeers = currPeers;

   // ── Cache / file index status changes ────────────────────────────────────
   const int cacheStatus = static_cast<int>(state.stats().cache_status());
   if (cacheStatus != this->prevCacheStatus)
   {
      QString msg;
      switch (state.stats().cache_status())
      {
      case Protos::GUI::State::Stats::LOADING_CACHE_IN_PROGRESS:
         msg = "[File Index] Loading saved index from disk...";
         break;
      case Protos::GUI::State::Stats::SCANNING_IN_PROGRESS:
         msg = "[File Index] Scanning shared folders for changes...";
         break;
      case Protos::GUI::State::Stats::HASHING_IN_PROGRESS:
         msg = "[File Index] Hashing files to build content index...";
         break;
      case Protos::GUI::State::Stats::UP_TO_DATE:
         msg = "[File Index] Index is up to date";
         break;
      default:
         break;
      }
      if (!msg.isEmpty())
         batch << LM::Builder::newEntry(now, LM::SV_END_USER, msg);
      this->prevCacheStatus = cacheStatus;
   }

   if (!batch.isEmpty())
      this->newLogEntries(batch);
}

void LogModel::coreConnected()
{
   this->prevPeers.clear();
   this->prevCacheStatus = -1;
}

void LogModel::coreDisconnected(bool)
{
   this->prevPeers.clear();
   this->prevCacheStatus = -1;
}
