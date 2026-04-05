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

#pragma once

#include <functional>
#include <limits>

#include <QObject>
#include <QPair>
#include <QList>
#include <QStringList>
#include <QMutex>
#include <QRecursiveMutex>
#include <QSharedPointer>

#include <Protos/file_cache.pb.h>
#include <Protos/core_protocol.pb.h>

#include <Common/Uncopyable.h>
#include <Common/SharedEntry.h>
#include <Common/Path.h>

#include <priv/FileUpdater/DirWatcher.h>
#include <priv/Cache/SharedEntry.h>
#include <priv/Cache/Chunk.h>
#include <priv/Cache/FilePool.h>

namespace FM
{
   class Entry;
   class File;
   class Directory;
   class FileUpdater;

   class Cache : public QObject, Common::Uncopyable
   {
      Q_OBJECT
   public:
      Cache();
      ~Cache();

      void forall(std::function<void(Entry*)> fun) const;

      Protos::Common::Entries getProtoSharedEntries() const;
      Protos::Common::Entries getProtoEntries(const Protos::Common::Entry& dir, int maxNbHashesPerEntry = std::numeric_limits<int>::max()) const;
      Directory* getDirectory(const Protos::Common::Entry& dir) const;

      Entry* getEntry(const QString& path) const;
      File* getFile(const Protos::Common::Entry& fileEntry) const;
      QList<QSharedPointer<IChunk>> newFile(Protos::Common::Entry& fileEntry);
      void newDirectory(Protos::Common::Entry& dirEntry);

      QList<Common::SharedEntry> getSharedEntries() const;
      SharedDirectory* getSharedEntry(const Common::Hash& ID) const;
      SharedEntry* getSharedEntry(const QString& path) const;
      void setSharedPaths(const QList<Common::Path>& paths);
      QPair<Common::SharedEntry, QString> addASharedEntry(const QString& absoluteDir);
      void removeSharedEntry(SharedEntry* dir, Directory* dir2 = nullptr);

      SharedDirectory* getSuperSharedEntry(const QString& path) const;
      QList<SharedEntry*> getSubSharedEntries(const QString& path) const;
      bool isShared(const QString& path) const;

      Directory* getFittestDirectory(const QString& path) const;

      void createSharedEntries(const Protos::FileCache::Hashes& hashes);
      void populateHashes(Protos::FileCache::Hashes& hashes) const;

      quint64 getAmount() const;

      FilePool& getFilePool() { return this->filePool; }
      QRecursiveMutex& getMutex() const { return this->mutex; }

      void onEntryAdded(Entry* entry);
      void onEntryRemoved(Entry* entry);
      void onEntryRenamed(Entry* entry, const QString& oldName);
      void onEntryResizing(Entry* entry);
      void onEntryResized(Entry* entry, qint64 oldSize);

      void onChunkHashKnown(const QSharedPointer<Chunk>& chunk);
      void onChunkRemoved(const QSharedPointer<Chunk>& chunk);

      void onScanned(Directory* dir);

   public slots:
      void deleteEntry(Entry* entry);

   signals:
      void entryAdded(Entry* entry);
      void entryRemoved(Entry* entry);
      void entryRenamed(Entry* entry, const QString& oldName);
      void entryResizing(Entry* entry);
      void entryResized(Entry* entry, qint64 oldSize);

      void chunkHashKnown(const QSharedPointer<Chunk>& chunk);
      void chunkRemoved(const QSharedPointer<Chunk>& chunk);
      void directoryScanned(Directory* dir);

      void newSharedEntry(SharedEntry* dir);
      void sharedEntryRemoved(SharedEntry* dir, Directory* dir2);

   private:
      static Common::SharedEntry makeSharedEntry(const SharedEntry* entry);
      SharedDirectory* createShareEntry(const QString path, const Common::Hash& ID = Common::Hash(), int pos = -1);
      void createSharedEntries(const QStringList& dirs, const QList<Common::Hash>& ids = QList<Common::Hash>());

      Directory* getWriteableDirectory(const QString& path, qint64 spaceNeeded = 0) const;

      QList<SharedDirectory*> sharedEntries;

      FilePool filePool;

      mutable QRecursiveMutex mutex;
   };
}
