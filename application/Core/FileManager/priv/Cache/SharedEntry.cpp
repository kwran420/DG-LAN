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

#include <priv/Cache/SharedEntry.h>
using namespace FM;

#include <QDir>
#include <QFile>

#include <Common/ProtoHelper.h>
#include <Common/Global.h>
#include <Common/Path.h>

#include <Exceptions.h>
#include <priv/Log.h>
#include <priv/Exceptions.h>
#include <priv/Cache/Cache.h>
#include <priv/Cache/Directory.h>
#include <priv/Cache/File.h>

/**
  * If an existing shared entry is a sub directory or sub file then it will be merged with the new one.
  * @exception SuperDirectoryExistsException Thrown when a super shared directory already exists.
  * @exception SharedEntryAlreadySharedException
  * @exception FileNotFoundException
  * @exception DirNotFoundException
  */
SharedEntry::SharedEntry(Cache* cache, const Common::Path& path, const Common::Hash& id) :
   cache(cache), path(pathWithoutEntryName(path)), id(id.isNull() ? Common::Hash::rand() : id), userName(entryName(path))
{
   const QString& pathStr = path.getPath();

   // Avoid two same directories.
   if (this->cache->isShared(pathStr))
      throw SharedEntryAlreadySharedException();

   // First of all check if the entry physically exists.
   if (path.isFile() && !QFile(pathStr).exists())
      throw FileNotFoundException(pathStr);

   if (!path.isFile() && !QDir(pathStr).exists())
      throw DirNotFoundException(pathStr);

   if (SharedDirectory* dir = this->cache->getSuperSharedEntry(pathStr))
      throw SuperDirectoryExistsException(dir->getFullPath().getPath(), pathStr);
}

/**
  * A factory to create a shared entry (file or directory) depending on the given path.
  */
SharedEntry* SharedEntry::create(Cache* cache, const QString& pathStr, const Common::Hash& id)
{
   Common::Path path(pathStr);
   if (path.isFile())
      return new SharedFile(cache, path, id);
   else
      return new SharedDirectory(cache, path, id);
}

SharedEntry::~SharedEntry()
{
   L_DEBU(QString("SharedEntry deleted: %1").arg(this->getUserName()));
}

void SharedEntry::populateEntry(Protos::Common::Entry* entry) const
{
   this->getRootEntry()->populateEntry(entry, true);
   entry->set_path(""); // Don't expose abs path to peers.
}

void SharedEntry::del(bool invokeDelete)
{
   this->getRootEntry()->del(invokeDelete);
}

void SharedEntry::moveInto(Directory* directory)
{
   // A directory can't be moved into its own tree.
   if (this->getRootEntry()->getRoot() == this)
      return;

   this->getCache()->removeSharedEntry(dynamic_cast<SharedDirectory*>(this), directory->createSubDir(this->getRootEntry()->getName()));
}

void SharedEntry::moveInto(const QString& path)
{
   this->path = Common::Path(path);
}

Cache* SharedEntry::getCache() const
{
   return this->cache;
}

Common::Path SharedEntry::getPath() const
{
   return this->path;
}

Common::Hash SharedEntry::getId() const
{
   return this->id;
}

QString SharedEntry::getUserName() const
{
   return this->userName;
}

/**
  * Extract the entry name from the full path.
  * 'C:/User/Paul/Movies/' -> 'Movies'
  * 'C:/User/Paul/Movies/movie.avi' -> 'movie.avi'
  * '/' -> '/'
  * 'C:/' -> 'C:/'
  */
QString SharedEntry::entryName(const Common::Path& path)
{
   if (path.isFile())
      return path.getFilename();

   if (path.getDirs().isEmpty())
      return path.getRoot();
   else
      return path.getDirs().last();
}

Common::Path SharedEntry::pathWithoutEntryName(const Common::Path& path)
{
   if (path.isFile())
      return path.removeFilename();
   else
      return path.removeLastDir();
}

/////

SharedDirectory::SharedDirectory(Cache* cache, const Common::Path& path, const Common::Hash& id) :
   SharedEntry(cache, path, id),
   directory(new Directory(this, entryName(path), nullptr, false))
{
}

void SharedDirectory::mergeSubSharedEntries()
{
   foreach (SharedEntry* subEntry, this->getCache()->getSubSharedEntries(this->getFullPath().getPath()))
   {
      const QStringList& parentFolders = this->getFullPath().getDirs();
      const QStringList& childFolders = subEntry->getFullPath().getDirs();
      Directory* current = this->directory;
      for (int i = parentFolders.size(); i < childFolders.size(); i++)
         current = current->createSubDir(childFolders[i]);

      this->getCache()->removeSharedEntry(dynamic_cast<SharedDirectory*>(subEntry), current);
   }
}

Entry* SharedDirectory::getRootEntry() const
{
   return this->directory;
}

Common::Path SharedDirectory::getFullPath() const
{
   return this->path.appendDir(this->directory->getName());
}

/////

SharedFile::SharedFile(Cache* cache, const Common::Path& path, const Common::Hash& id) :
   SharedEntry(cache, path, id),
   file(new File(this, entryName(path), 0, QDateTime(), nullptr, Common::Hashes(), false))
{
}

void SharedFile::mergeSubSharedEntries()
{
   // We can't merge another shared entry into a file.
}

Entry* SharedFile::getRootEntry() const
{
   return this->file;
}

Common::Path SharedFile::getFullPath() const
{
   return this->path.setFilename(this->file->getName());
}
