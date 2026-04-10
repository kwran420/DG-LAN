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

#include <Utils.h>
using namespace GUI;

#include <QListView>
#include <QTreeView>
#include <QStringBuilder>
#include <QCoreApplication>
#include <QFileDialog>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QProcess>
#include <QGridLayout>
#include <QLabel>

#include <Settings/RemoteFileDialog.h>
#include <Constants.h>

/**
  * Ask the user to choose one or more directories/files.
  * Note: currently only browses local directories, not remote Core directories.
  */
QStringList Utils::askForDirectoriesOrFiles(QSharedPointer<RCC::ICoreConnection> coreConnection)
{
   if (coreConnection->isLocal())
   {
      QFileDialog fileDialog(0, "Choose one or more directories");
      fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
      fileDialog.setFileMode(QFileDialog::Directory);

      // Allow selecting multiple directories at once.
      QListView* l = fileDialog.findChild<QListView*>("listView");
      if (l)
         l->setSelectionMode(QAbstractItemView::ExtendedSelection);
      QTreeView* t = fileDialog.findChild<QTreeView*>();
      if (t)
         t->setSelectionMode(QAbstractItemView::ExtendedSelection);

      if (fileDialog.exec())
      {
         return fileDialog.selectedFiles();
      }
      return QStringList();
   }
   else
   {
      RemoteFileDialog fileDialog;
      fileDialog.setWindowTitle("Remote directory");
      fileDialog.setText("Remote directory to share: ");
      if (fileDialog.exec())
      {
         return QStringList() << fileDialog.getFolder();
      }
      return QStringList();
   }
}

QStringList Utils::askForDirectoriesToDownloadTo(QSharedPointer<RCC::ICoreConnection> coreConnection)
{
   return QStringList();
}

QString Utils::emoticonsDirectoryPath()
{
   QString defaultPath = QCoreApplication::applicationDirPath() % "/" % Constants::EMOTICONS_DIRECTORY;
#if DEBUG
   if (!QDir(defaultPath).exists())
      return QCoreApplication::applicationDirPath() % "/../../ressources/emoticons";
#endif
   return defaultPath;
}

void Utils::openLocations(const QStringList& paths)
{
   foreach (QString path, paths)
      Utils::openLocation(path);
}

/**
  * Open the location of the path, launch a system file browser to the given directory path. If the path is a file then it will open it's containing directory and select it.
  *
  * An other on Windows is to use 'SHOpenFolderAndSelectItems(..)'.
  */
void Utils::openLocation(const QString& path)
{
#ifdef Q_OS_WIN32
   QProcess explorer;
   if (!QFileInfo(path).isDir())
      explorer.setArguments(QStringList() << "/select,");
   explorer.setNativeArguments("\"" + QDir::toNativeSeparators(path) + "\"");
   explorer.setProgram("explorer");
   explorer.start();
   explorer.waitForFinished(5000);
#else
   QFileInfo fileInfo(path);
   const QString dirPath = fileInfo.isDir() ? path : fileInfo.absolutePath();
   QDesktopServices::openUrl(QUrl("file:///" + dirPath, QUrl::TolerantMode));
#endif
}

void Utils::openFile(const QString& path)
{
   QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
}
