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
  
#include <priv/GetEntriesResult.h>
using namespace FM;

#include <QMutexLocker>

#include <Common/Settings.h>

#include <priv/Log.h>

GetEntriesResult::GetEntriesResult(Directory* dir, int maxNbHashesPerEntry) :
   IGetEntriesResult(SETTINGS.get<quint32>("get_entries_timeout")), dir(dir), maxNbHashesPerEntry(maxNbHashesPerEntry)
{
   qRegisterMetaType<Protos::Core::GetEntriesResult::EntryResult>("Protos::Core::GetEntriesResult::EntryResult");
   L_WARN(QString("FM::GetEntriesResult CTOR dir=%1 maxHash=%2").arg(dir ? dir->getFullPath().getPath() : QString("NULL")).arg(maxNbHashesPerEntry));
}

void GetEntriesResult::start()
{
   if (!this->dir)
   {
      L_WARN("FM::GetEntriesResult::start(): null directory -> DONT_HAVE");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      emit result(this->res);
   }
   else if (this->dir->isScanned())
   {
      L_WARN(QString("FM::GetEntriesResult::start(): directory scanned: %1").arg(this->dir->getFullPath().getPath()));
      this->buildResult();
      if (this->res.status() == Protos::Core::GetEntriesResult::EntryResult::OK)
         L_WARN(QString("FM::GetEntriesResult::start(): buildResult OK, entries=%1").arg(this->res.entries().entry_size()));
      emit result(this->res);
   }
   else
   {
      L_WARN(QString("FM::GetEntriesResult::start(): directory NOT YET scanned: %1 \u2014 waiting...").arg(this->dir->getFullPath().getPath()));
      connect(this->dir->getCache(), &Cache::directoryScanned, this, &GetEntriesResult::directoryScanned, Qt::DirectConnection);
      this->startTimer();
   }
}

/**
  * This method is called in the 'FileUpdater' thread.
  */
void GetEntriesResult::directoryScanned(Directory* dir)
{
   if (dir != this->dir)
      return;

   L_WARN(QString("FM::GetEntriesResult::directoryScanned(): %1").arg(dir ? dir->getFullPath().getPath() : QString("NULL")));

   this->buildResult();

   QMetaObject::invokeMethod(this, "sendResult"); // To send the message 'result' in the main thread.
}

void GetEntriesResult::sendResult()
{
   L_WARN("FM::GetEntriesResult::sendResult()");
   if (this->dir)
      disconnect(this->dir->getCache(), &Cache::directoryScanned, this, &GetEntriesResult::directoryScanned);
   this->stopTimer();

   emit result(res);
}

void GetEntriesResult::buildResult()
{
   if (!this->dir)
   {
      L_WARN("FM::GetEntriesResult::buildResult(): dir is NULL — aborting");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      return;
   }

   Cache* cache = this->dir->getCache();
   if (!cache)
   {
      L_WARN("FM::GetEntriesResult::buildResult(): cache is NULL — aborting");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      return;
   }

   QMutexLocker locker(&cache->getMutex());

   this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::OK);

   try
   {
      QLinkedList<Directory*> subDirs = this->dir->getSubDirs();
      L_WARN(QString("FM::GetEntriesResult::buildResult(): %1 subdirs, iterating...").arg(subDirs.size()));
      for (auto it = subDirs.begin(); it != subDirs.end(); ++it)
      {
         Directory* subDir = *it;
         if (!subDir)
         {
            L_WARN("FM::GetEntriesResult::buildResult(): null subDir pointer — skipping");
            continue;
         }
         subDir->populateEntry(this->res.mutable_entries()->add_entry());
      }

      QLinkedList<File*> files = this->dir->getFiles();
      L_WARN(QString("FM::GetEntriesResult::buildResult(): %1 files, iterating...").arg(files.size()));
      for (auto it = files.begin(); it != files.end(); ++it)
      {
         File* file = *it;
         if (!file)
         {
            L_WARN("FM::GetEntriesResult::buildResult(): null file pointer — skipping");
            continue;
         }
         if (file->isComplete())
            file->populateEntry(this->res.mutable_entries()->add_entry(), false, this->maxNbHashesPerEntry);
      }

      L_WARN(QString("FM::GetEntriesResult::buildResult(): DONE entries=%1").arg(this->res.entries().entry_size()));
   }
   catch (const std::exception& e)
   {
      L_ERRO(QString("FM::GetEntriesResult::buildResult(): EXCEPTION: %1").arg(e.what()));
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::ERROR_UNKNOWN);
   }
   catch (...)
   {
      L_ERRO("FM::GetEntriesResult::buildResult(): UNKNOWN EXCEPTION");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::ERROR_UNKNOWN);
   }
}
