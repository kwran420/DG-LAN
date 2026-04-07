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

#include <priv/Cache/Cache.h>
using namespace FM;

#include <QDir>
#include <QQueue>

#include <Common/Global.h>
#include <Common/Settings.h>
#include <Common/ProtoHelper.h>
#include <Common/Constants.h>

#include <Exceptions.h>
#include <priv/Log.h>
#include <priv/Exceptions.h>
#include <priv/Constants.h>
#include <priv/Cache/SharedEntry.h>
#include <priv/Cache/Directory.h>
#include <priv/Cache/File.h>

/**
  * @class FM::Cache
  *
  * Owns all the shared directories (roots), their content (directories and file) and the chunks.
  * Here are the main capabilities:
  *  - Browse directories and files.
  *  - Create a new file.
  *  - Add or remove a shared directory (root).
  *  - Serialize or deserialize the hashes of the files in a 'Protos::FileCache::Hashes' structure (to be saved/loaded in/from a physical file).
  */

Cache::Cache()
{
   qRegisterMetaType<Entry*>("Entry*");
}

Cache::~Cache()
{
   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
      i.next()->del();
}

/**
  * Call the given lambda for each entries owned by the cache.
  * It can be a shared directory, a sub-directory or a file.
  */
void Cache::forall(std::function<void(Entry*)> fun) const
{
   QQueue<Entry*> entries;
   foreach (SharedDirectory* sd, this->sharedEntries)
      entries.enqueue(static_cast<Entry*>(sd->getRootEntry()));

   while (!entries.isEmpty())
   {
      Entry* current = entries.dequeue();
      fun(current);

      Directory* dir = dynamic_cast<Directory*>(current);
      if (dir)
      {
         foreach (File* file, dir->getFiles())
            fun(file);
         foreach (Directory* subDir, dir->getSubDirs())
            entries.enqueue(subDir);
      }
   }
}

/**
  * Gets the roots directories.
  */
Protos::Common::Entries Cache::getProtoSharedEntries() const
{
   QMutexLocker locker(&this->mutex);

   Protos::Common::Entries result;

   foreach (SharedDirectory* sharedDir, this->sharedEntries)
   {
      Protos::Common::Entry* entry = result.add_entry();
      sharedDir->populateEntry(entry);
   }

   return result;
}

Protos::Common::Entries Cache::getProtoEntries(const Protos::Common::Entry& dir, int maxNbHashesPerEntry) const
{
   QMutexLocker locker(&this->mutex);

   Protos::Common::Entries result;

   if (Directory* directory = this->getDirectory(dir))
   {
      QLinkedList<Directory*> subDirs = directory->getSubDirs();
      QLinkedList<File*> files = directory->getFiles();

      for (auto it = subDirs.begin(); it != subDirs.end(); ++it)
      {
         if (!*it) continue;
         (*it)->populateEntry(result.add_entry());
      }

      for (auto it = files.begin(); it != files.end(); ++it)
      {
         if (!*it) continue;
         if ((*it)->isComplete())
            (*it)->populateEntry(result.add_entry(), false, maxNbHashesPerEntry);
      }
   }

   return result;
}

/**
  * a) Search among their shared directory the one who match the given directory.
  * b) In the shared directory try to find the directory corresponding to 'entry.dir.path'.
  */
Directory* Cache::getDirectory(const Protos::Common::Entry& dir) const
{
   if (!dir.has_shared_entry())
      return nullptr;

   QMutexLocker locker(&this->mutex);

   foreach (SharedDirectory* sharedDir, this->sharedEntries)
   {
      if (sharedDir->getId() == dir.shared_entry().id().hash())
      {
         Directory* rootDir = static_cast<Directory*>(sharedDir->getRootEntry());

         // Empty path means this IS the root shared entry itself.
         if (dir.path().empty())
            return rootDir;

         // Build the folder list from the path, avoiding QDir::cleanPath("")->"."
         QString pathStr = Common::ProtoHelper::getStr(dir, &Protos::Common::Entry::path);
         QString cleaned = QDir::cleanPath(pathStr);
         QStringList folders;
         if (cleaned != ".")
            folders = cleaned.split('/', QString::SkipEmptyParts);
         folders << Common::ProtoHelper::getStr(dir, &Protos::Common::Entry::name);

         Directory* currentDir = rootDir;
         foreach (QString folder, folders)
         {
            currentDir = currentDir->getSubDir(folder);
            if (!currentDir)
               return nullptr;
         }

         return currentDir;
      }
   }

   return nullptr;
}

/**
  * @param path The absolute path to a directory or a file.
  * @return Returns a directory or a file, it can be a shared directory. Returns 'nullptr' if no entry found.
  */
Entry* Cache::getEntry(const QString& path) const
{
   QMutexLocker locker(&this->mutex);

   foreach (SharedDirectory* sharedDir, this->sharedEntries)
   {
      // We remove the end '/'.
      QString currentPath(sharedDir->getFullPath().getPath());
      if (currentPath.length() > 1 && currentPath.endsWith('/'))
         currentPath.remove(currentPath.size() - 1, 1);

      if (path.startsWith(currentPath) && (path.size() == currentPath.size() || path[currentPath.size()] == '/'))
      {
         QString relativePath(path);
         relativePath.remove(0, currentPath.size());
         const QStringList folders = relativePath.split('/', QString::SkipEmptyParts);

         Directory* currentDir = static_cast<Directory*>(sharedDir->getRootEntry());
         for (QStringListIterator i(folders); i.hasNext();)
         {
            QString folder = i.next();
            Directory* dir = currentDir->getSubDir(folder);
            if (!dir)
            {
               if (!i.hasNext())
               {
                  File* file = currentDir->getFile(folder);
                  if (file)
                     return file;
               }
               return nullptr;
            }
            currentDir = dir;
         }

         return currentDir;
      }
   }

   return nullptr;
}

/**
  * Try to find the file from the cache with the provided reference.
  * @return Returns 'nullptr' if the file hasn't be found.
  */
File* Cache::getFile(const Protos::Common::Entry& fileEntry) const
{
   QMutexLocker locker(&this->mutex);

   if (!fileEntry.has_shared_entry())
   {
      L_WARN(QString("Cache::getFile : 'fileEntry' doesn't have the field 'shared_dir' set!"));
      return nullptr;
   }

   foreach (SharedDirectory* sharedDir, this->sharedEntries)
   {
      if (sharedDir->getId() == fileEntry.shared_entry().id().hash())
      {
         const QString& relativePath = Common::ProtoHelper::getStr(fileEntry, &Protos::Common::Entry::path);
         const QStringList folders = relativePath.split('/', QString::SkipEmptyParts);

         Directory* dir = static_cast<Directory*>(sharedDir->getRootEntry());
         QStringListIterator i(folders);
         forever
         {
            if (dir)
            {
               if (!i.hasNext())
               {
                  File* file = dir->getFile(Common::ProtoHelper::getStr(fileEntry, &Protos::Common::Entry::name));

                  if (file)
                     return file;
                  return nullptr;
               }
            }
            else
               return nullptr;

            if (!i.hasNext())
               return nullptr;

            dir = dir->getSubDir(i.next());
         }

         return nullptr;
      }
   }

   return nullptr;
}

/**
  * Creates a new file in the path defined in 'fileEntry' and returns its chunks.
  *
  * @exception NoWriteableDirectoryException
  * @exception InsufficientStorageSpaceException
  * @exception UnableToCreateNewFileException
  * @exception UnableToCreateNewDirException
  */
QList<QSharedPointer<IChunk>> Cache::newFile(Protos::Common::Entry& fileEntry)
{
   QMutexLocker locker(&this->mutex);

   QString dirPath = QDir::cleanPath(Common::ProtoHelper::getStr(fileEntry, &Protos::Common::Entry::path));
   if (dirPath == ".")
      dirPath.clear();
   const qint64 spaceNeeded = fileEntry.size() + SETTINGS.get<quint32>("minimum_free_space");

   // If we know where to put the file.
   Directory* dir = nullptr;
   if (fileEntry.has_shared_entry())
   {
      SharedDirectory* sharedDir = this->getSharedEntry(fileEntry.shared_entry().id().hash());

      if (sharedDir)
      {
         if (Common::Global::availableDiskSpace(sharedDir->getFullPath().getPath()) < spaceNeeded)
            throw InsufficientStorageSpaceException();

         dir = static_cast<Directory*>(sharedDir->getRootEntry())->createSubDirs(dirPath.split('/', QString::SkipEmptyParts), true);
      }
      else
         fileEntry.clear_shared_entry(); // The shared directory is invalid.
   }

   if (!dir)
      dir = this->getWriteableDirectory(dirPath, spaceNeeded);

   if (!dir)
      throw UnableToCreateNewFileException();

   Common::Hashes hashes;
   for (int i = 0; i < fileEntry.chunk_size(); i++)
      hashes << fileEntry.chunk(i).hash();

   const QString& name = Common::ProtoHelper::getStr(fileEntry, &Protos::Common::Entry::name);

   // If a file with the same name already exists we will compare its hashes with the given entry.
   File* file = dir->getFile(name);
   if (file != nullptr)
   {
      bool resetExistingFile = false;
      const QVector<QSharedPointer<Chunk>>& existingChunks = file->getChunks();
      if (existingChunks.size() != fileEntry.chunk_size())
         resetExistingFile = true;
      else
         for (int i = 0; i < existingChunks.size(); i++)
            if (existingChunks[i]->getHash() != Common::Hash(fileEntry.chunk(i).hash()))
            {
               resetExistingFile = true;
               break;
            }

      if (resetExistingFile)
         file->setToUnfinished(fileEntry.size(), hashes);
   }
   else
   {
      file = new File(dir->getRoot(), name, fileEntry.size(), QDateTime::currentDateTime(), dir, hashes, true);
   }

   fileEntry.set_exists(true); // File has been physically created.
   dir->populateEntry(&fileEntry); // We set the shared directory.

   // Is there a better way to up cast? An other method is shown below that uses 'reinterpret_cast'.
   QList<QSharedPointer<IChunk>> ichunks;
   const QVector<QSharedPointer<Chunk>>& chunks = file->getChunks();
   ichunks.reserve(chunks.size());
   for (QVectorIterator<QSharedPointer<Chunk>> i(chunks); i.hasNext();)
      ichunks << i.next();
   return ichunks;

   // This method works but 'reinterpret_cast' is too dangerous. (only if 'File::getChunks()' return a QList).
   // QList<QSharedPointer<Chunk>> chunks = file->getChunks();
   // return *(reinterpret_cast<QList<QSharedPointer<IChunk>>*>(&chunks));
}

/**
  * @exception NoWriteableDirectoryException
  * @exception UnableToCreateNewDirException
  */
void Cache::newDirectory(Protos::Common::Entry& dirEntry)
{
   QMutexLocker locker(&this->mutex);

   QString dirPath = QDir::cleanPath(Common::ProtoHelper::getStr(dirEntry, &Protos::Common::Entry::path));
   if (dirPath == ".")
      dirPath.clear();
   dirPath += '/' + Common::ProtoHelper::getStr(dirEntry, &Protos::Common::Entry::name);

   // If we know where to create the directory.
   Directory* dir = nullptr;
   if (dirEntry.has_shared_entry())
   {
      SharedDirectory* sharedDir = this->getSharedEntry(dirEntry.shared_entry().id().hash());
      if (sharedDir)
         dir = static_cast<Directory*>(sharedDir->getRootEntry())->createSubDirs(dirPath.split('/', QString::SkipEmptyParts), true);
      else
         dirEntry.clear_shared_entry(); // The shared directory is invalid.
   }

   if (!dir)
      dir = this->getWriteableDirectory(dirPath);

   if (!dir)
      throw UnableToCreateNewDirException();
}

QList<Common::SharedEntry> Cache::getSharedEntries() const
{
   QMutexLocker locker(&this->mutex);

   QList<Common::SharedEntry> list;

   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
      list << makeSharedEntry(i.next());

   return list;
}

SharedDirectory* Cache::getSharedEntry(const Common::Hash& ID) const
{
   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* dir = i.next();
      if (dir->getId() == ID)
         return dir;
   }
   return nullptr;
}

SharedEntry* Cache::getSharedEntry(const QString& path) const
{
   QMutexLocker locker(&this->mutex);
   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* dir = i.next();
      if (dir->getFullPath().getPath() == path)
         return dir;
   }
   return nullptr;
}

/**
  * @exception DirsNotFoundException
  */
void Cache::setSharedPaths(const QList<Common::Path>& paths)
{
   QMutexLocker locker(&this->mutex);

   QStringList dirsNotFound;

   int j = 0; // currentDirs.
   for (int i = 0; i < paths.size(); i++) // dirs.
   {
      for (int j2 = j; j2 < this->sharedEntries.size(); j2++)
      {
         const QString dir = Common::Path::cleanDirPath(paths[i].getPath());
         if (dir == this->sharedEntries[j2]->getFullPath().getPath())
         {
            this->sharedEntries.move(j2, j++);
            goto nextDir;
         }
      }
      try
      {
         // paths[i].getPath() not found -> we create a new one.
         if (this->createShareEntry(paths[i].getPath(), Common::Hash(), j))
            j++;
      }
      catch (DirNotFoundException& e)
      {
         dirsNotFound << e.path;
      }
   nextDir:;
   }

   while (j < this->sharedEntries.size())
      this->removeSharedEntry(this->sharedEntries[j]);

   for (int k = 0; k < this->sharedEntries.size(); k++)
      this->sharedEntries[k]->mergeSubSharedEntries();

   if (!dirsNotFound.isEmpty())
      throw DirsNotFoundException(dirsNotFound);
}

/**
  * @exception DirsNotFoundException
  */
QPair<Common::SharedEntry, QString> Cache::addASharedEntry(const QString& absoluteDir)
{
   QMutexLocker locker(&this->mutex);

   QString absoluteDirCleaned = Common::Path::cleanDirPath(absoluteDir);

   // If the given directory is already a shared directory
   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* current = i.next();
      if (absoluteDirCleaned == current->getFullPath().getPath())
         return qMakePair(makeSharedEntry(current), QString("/"));
   }

   // If the given directory is a sub directory to an existing shared directory
   SharedDirectory* superDir = this->getSuperSharedEntry(absoluteDirCleaned);
   if (superDir && absoluteDirCleaned.indexOf(superDir->getFullPath().getPath()) == 0)
   {
      QString relativeDir(absoluteDirCleaned);
      relativeDir.remove(0, superDir->getFullPath().getPath().length());
      relativeDir.prepend('/');
      return qMakePair(makeSharedEntry(superDir), relativeDir);
   }

   // Else we create a new shared directory
   try
   {
      SharedDirectory* dir = this->createShareEntry(absoluteDirCleaned);
      if (dir)
      {
         dir->mergeSubSharedEntries();
         return qMakePair(makeSharedEntry(dir), QString("/"));
      }
      else
         throw UnableToCreateSharedDirectory();
   }
   catch (DirNotFoundException& e)
   {
      throw DirsNotFoundException(QStringList() << e.path);
   }
}

/**
  * Will inform the fileUpdater and delete 'dir'.
  * If 'dir2' is given 'dir' content (sub dirs + files) will be give to 'dir2'.
  * The directory is deleted by fileUpdater.
  */
void Cache::removeSharedEntry(SharedEntry* sharedEntry, Directory* dir2)
{
   QMutexLocker locker(&this->mutex);
   SharedDirectory* dir = dynamic_cast<SharedDirectory*>(sharedEntry);
   if (dir && this->sharedEntries.contains(dir))
   {
      this->sharedEntries.removeOne(dir);
      emit sharedEntryRemoved(dir, dir2);
   }
}

SharedDirectory* Cache::getSuperSharedEntry(const QString& path) const
{
   QMutexLocker locker(&this->mutex);
   const QStringList& folders = path.split('/', QString::SkipEmptyParts);

   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* sharedDir = i.next();
      const QStringList& foldersShared = sharedDir->getFullPath().getPath().split('/', QString::SkipEmptyParts);
      if (folders.size() <= foldersShared.size())
         continue;

      for (int i = 0; i < foldersShared.size(); i++)
         if (folders[i] != foldersShared[i])
            goto nextSharedDir;

      return sharedDir;
      nextSharedDir:;
   }

   return nullptr;
}

QList<SharedEntry*> Cache::getSubSharedEntries(const QString& path) const
{
   QMutexLocker locker(&this->mutex);
   QList<SharedEntry*> ret;

   const QStringList& folders = path.split('/', QString::SkipEmptyParts);

   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* sharedDir = i.next();
      const QStringList& foldersShared = sharedDir->getFullPath().getPath().split('/', QString::SkipEmptyParts);

      if (foldersShared.size() <= folders.size())
         continue;

      for (int i = 0; i < folders.size(); i++)
         if (folders[i] != foldersShared[i])
            goto nextSharedDir;

      ret << sharedDir;

      nextSharedDir:;
   }

   return ret;
}

/**
  * If path matches a shared directory or one of its sub directories then true is returned.
  */
bool Cache::isShared(const QString& path) const
{
   QMutexLocker locker(&this->mutex);
   foreach (SharedDirectory* dir, this->sharedEntries)
      if (dir->getFullPath().getPath() == path)
         return true;
   return false;
}

/**
  * Returns the directory that best matches to the given path.
  * For example, path = /home/peter/linux/distrib/debian/etch
  *  This directory exists in cache : /home/peter/linux/distrib
  *  Thus, this directory 'distrib' will be returned.
  * @param path An absolute path.
  * @return If no directory can be match 0 is returned.
  */
Directory* Cache::getFittestDirectory(const QString& path) const
{
   QMutexLocker locker(&this->mutex);

   foreach (SharedDirectory* sharedDir, this->sharedEntries)
   {
      const QString sharedDirPath = sharedDir->getFullPath().getPath();
      if (path.startsWith(sharedDirPath))
      {
         QString relativePath(path);
         relativePath.remove(0, sharedDirPath.size());
         const QStringList folders = relativePath.split('/', QString::SkipEmptyParts);

         Directory* currentDir = static_cast<Directory*>(sharedDir->getRootEntry());
         foreach (QString folder, folders)
         {
            Directory* nextdir = currentDir->getSubDir(folder);
            if (!nextdir)
               break;
            currentDir = nextdir;
         }
         return currentDir;
      }
   }

   return nullptr;
}

/**
  * Defines the shared directories from the persisted given data.
  * The directories and files are not created here but later by the FileUpdater, see the FileManager ctor.
  */
void Cache::createSharedEntries(const Protos::FileCache::Hashes& hashes)
{
   QStringList paths;
   QList<Common::Hash> ids;

   // Add the shared directories from the file cache.
   for (int i = 0; i < hashes.shareddir_size(); i++)
   {
      const Protos::FileCache::Hashes_SharedDir& dir = hashes.shareddir(i);
      paths << Common::ProtoHelper::getStr(dir, &Protos::FileCache::Hashes_SharedDir::path);
      ids << dir.id().hash();
   }
   this->createSharedEntries(paths, ids);
}

/**
  * Populates the given structure to be persisted later.
  */
void Cache::populateHashes(Protos::FileCache::Hashes& hashes) const
{
   QMutexLocker locker(&this->mutex);

   hashes.set_version(FILE_CACHE_VERSION);
   hashes.set_chunksize(Common::Constants::CHUNK_SIZE);

   for (QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
   {
      SharedDirectory* sharedDir = i.next();
      Protos::FileCache::Hashes_SharedDir* sharedDirMess = hashes.add_shareddir();
      sharedDirMess->mutable_id()->set_hash(sharedDir->getId().getData(), Common::Hash::HASH_SIZE);
      Common::ProtoHelper::setStr(*sharedDirMess, &Protos::FileCache::Hashes_SharedDir::mutable_path, sharedDir->getFullPath().getPath());

      static_cast<Directory*>(sharedDir->getRootEntry())->populateHashesDir(*sharedDirMess->mutable_root());
   }
}

quint64 Cache::getAmount() const
{
   QMutexLocker locker(&this->mutex);

   quint64 amount = 0;
   for(QListIterator<SharedDirectory*> i(this->sharedEntries); i.hasNext();)
      amount += static_cast<Directory*>(i.next()->getRootEntry())->getSize();
   return amount;
}

void Cache::onEntryAdded(Entry* entry)
{
   emit entryAdded(entry);
}

void Cache::onEntryRemoved(Entry* entry)
{
   emit entryRemoved(entry);
}

void Cache::onEntryRenamed(Entry* entry, const QString& oldName)
{
   emit entryRenamed(entry, oldName);
}

void Cache::onEntryResizing(Entry* entry)
{
   emit entryResizing(entry);
}

void Cache::onEntryResized(Entry* entry, qint64 oldSize)
{
   emit entryResized(entry, oldSize);
}

void Cache::onChunkHashKnown(const QSharedPointer<Chunk>& chunk)
{
   emit chunkHashKnown(chunk);
}

void Cache::onChunkRemoved(const QSharedPointer<Chunk>& chunk)
{
   emit chunkRemoved(chunk);
}

void Cache::onScanned(Directory* dir)
{
   emit directoryScanned(dir);
}

void Cache::deleteEntry(Entry* entry)
{
   delete entry;
}

/**
  * Creates a new shared directory.
  * The other shared directories may not be merged with the new one, use 'SharedDirectory::mergeSubSharedEntries' to do that after this call.
  *
  * @exception DirNotFoundException
  */
SharedDirectory* Cache::createShareEntry(const QString path, const Common::Hash& ID, int pos)
{
   try
   {
      SharedDirectory* dir = !ID.isNull() ?
         new SharedDirectory(this, Common::Path(path), ID) :
         new SharedDirectory(this, Common::Path(path));

      L_DEBU(QString("Add a new shared directory : %1").arg(path));

      if (pos == -1 || pos > this->sharedEntries.size())
         this->sharedEntries << dir;
      else
         this->sharedEntries.insert(pos, dir);

      emit newSharedEntry(dir);

      return dir;
   }
   catch (SharedEntryAlreadySharedException&)
   {
      L_DEBU(QString("Directory already shared : %1").arg(path));
   }
   catch (SuperDirectoryExistsException& e)
   {
      L_WARN(QString("There is already a super directory: %1 for this directory : %2").arg(e.superDirectory).arg(e.subDirectory));
   }

   return nullptr;
}

Common::SharedEntry Cache::makeSharedEntry(const SharedEntry* dir)
{
   Common::Path fp = dir->getFullPath();
   qint64 sz = static_cast<Directory*>(const_cast<SharedEntry*>(dir)->getRootEntry())->getSize();
   return Common::SharedEntry { dir->getId(), fp, QString(), sz, Common::Global::availableDiskSpace(fp.getPath()) };
}

/**
  * Create new shared directories.
  *
  * @exception DirsNotFoundException
  */
void Cache::createSharedEntries(const QStringList& dirs, const QList<Common::Hash>& ids)
{
   QMutexLocker locker(&this->mutex);

   QStringList dirsNotFound;

   QListIterator<QString> i(dirs);
   QListIterator<Common::Hash> k(ids);
   while (i.hasNext())
   {
      QString path = i.next();

      try
      {
         SharedDirectory* dir = k.hasNext() ?
            this->createShareEntry(path, k.next()) :
            this->createShareEntry(path);

         if (dir)
            dir->mergeSubSharedEntries();
      }
      catch (DirNotFoundException& e)
      {
         dirsNotFound << e.path;
      }
   }

   if (!dirsNotFound.isEmpty())
      throw DirsNotFoundException(dirsNotFound);
}

/**
  * Returns a directory which matches to the path, it will choose the shared directory which :
  *  - Has at least the needed space.
  *  - Has the most directories in common with 'path'.
  *
  * The missing directories will be automatically created.
  *
  * @param path A relative path to a directory. Must be a cleaned path (QDir::cleanPath).
  * @param spaceNeeded The number of storage space needed, if no directory can be found the exception 'InsufficientStorageSpaceException' is thrown.
  * @return The directory, 0 if unknown error.
  * @exception InsufficientStorageSpaceException (only if 'spaceNeeded' > 0)
  * @exception NoWriteableDirectoryException
  * @exception UnableToCreateNewDirException
  */
Directory* Cache::getWriteableDirectory(const QString& path, qint64 spaceNeeded) const
{
   QMutexLocker locker(&this->mutex);

   QStringList folders = path.split('/', QString::SkipEmptyParts);
   folders.removeAll(".");

   if (this->sharedEntries.isEmpty())
      throw NoWriteableDirectoryException();

   // Search for the fittest shared directory.
   SharedDirectory* currentSharedDir = nullptr;
   int currentNbDirsInCommon = -1;

   foreach (SharedDirectory* dir, this->sharedEntries)
   {
      if (spaceNeeded > 0 && Common::Global::availableDiskSpace(dir->getFullPath().getPath()) < spaceNeeded)
         continue;

      Directory* currentDir = static_cast<Directory*>(dir->getRootEntry());
      int nbDirsInCommon = 0;
      foreach (QString folder, folders)
      {
         currentDir = currentDir->getSubDir(folder);
         if (currentDir)
            nbDirsInCommon += 1;
         else
            break;
      }
      if (nbDirsInCommon > currentNbDirsInCommon)
      {
         currentNbDirsInCommon = nbDirsInCommon;
         currentSharedDir = dir;
      }
   }

   if (!currentSharedDir)
      throw InsufficientStorageSpaceException(); // Not executed if 'spaceNeeded' equals 0.

   // Create the missing directories.
   return static_cast<Directory*>(currentSharedDir->getRootEntry())->createSubDirs(folders, true);
}
