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
   L_DEBU(QString("GetEntriesResult created: dir=%1 maxHash=%2").arg(dir ? dir->getFullPath().getPath() : QString("NULL")).arg(maxNbHashesPerEntry));
}

void GetEntriesResult::start()
{
   if (!this->dir)
   {
      L_WARN("Cannot browse directory: directory not found (may have been removed)");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      emit result(this->res);
   }
   else if (this->dir->isScanned())
   {
      L_DEBU(QString("Browse: directory already scanned: %1").arg(this->dir->getFullPath().getPath()));
      this->buildResult();
      if (this->res.status() == Protos::Core::GetEntriesResult::EntryResult::OK)
         L_DEBU(QString("Browse: result built OK, entries=%1").arg(this->res.entries().entry_size()));
      emit result(this->res);
   }
   else
   {
      L_DEBU(QString("Browse: directory not yet scanned, waiting: %1").arg(this->dir->getFullPath().getPath()));
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

   L_DEBU(QString("Browse: directory scan complete: %1").arg(dir ? dir->getFullPath().getPath() : QString("NULL")));

   this->buildResult();

   QMetaObject::invokeMethod(this, "sendResult"); // To send the message 'result' in the main thread.
}

void GetEntriesResult::sendResult()
{
   L_DEBU("Browse: sending result to GUI");
   if (this->dir)
      disconnect(this->dir->getCache(), &Cache::directoryScanned, this, &GetEntriesResult::directoryScanned);
   this->stopTimer();

   emit result(res);
}

void GetEntriesResult::buildResult()
{
   if (!this->dir)
   {
      L_WARN("Cannot build browse results: directory reference is null");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      return;
   }

   Cache* cache = this->dir->getCache();
   if (!cache)
   {
      L_WARN("Cannot build browse results: file cache unavailable");
      this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::DONT_HAVE);
      return;
   }

   QMutexLocker locker(&cache->getMutex());

   this->res.set_status(Protos::Core::GetEntriesResult::EntryResult::OK);

   try
   {
      QLinkedList<Directory*> subDirs = this->dir->getSubDirs();
      L_DEBU(QString("Browse: processing %1 subdirectories").arg(subDirs.size()));
      for (auto it = subDirs.begin(); it != subDirs.end(); ++it)
      {
         Directory* subDir = *it;
         if (!subDir)
         {
            L_WARN("Skipped invalid subdirectory entry while browsing");
            continue;
         }
         subDir->populateEntry(this->res.mutable_entries()->add_entry(), true);
      }

      QLinkedList<File*> files = this->dir->getFiles();
      L_DEBU(QString("Browse: processing %1 files").arg(files.size()));
      for (auto it = files.begin(); it != files.end(); ++it)
      {
         File* file = *it;
         if (!file)
         {
            L_WARN("Skipped invalid file entry while browsing");
            continue;
         }
         if (file->isComplete())
            file->populateEntry(this->res.mutable_entries()->add_entry(), true, this->maxNbHashesPerEntry);
      }

      L_DEBU(QString("Browse: completed, total entries=%1").arg(this->res.entries().entry_size()));
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
