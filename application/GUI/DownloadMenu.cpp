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

#include <DownloadMenu.h>
using namespace GUI;

#include <algorithm>

#include <QAction>

#include <Utils.h>

/**
  * @class GUI::DownloadMenu
  *
  * Show the list of shared directory as a menu.
  * - The menu can be shown by calling 'show(..)'.
  * - When the user select an action, the signal 'downloadTo(..)' is emitted.
  * - Can be sub-classed to add some entries. In this case 'onShowMenu(..)' must be overridden.
  */

DownloadMenu::DownloadMenu(const SharedEntryListModel& sharedEntryListModel) :
   sharedEntryListModel(sharedEntryListModel)
{
}

void DownloadMenu::show(const QPoint& globalPosition)
{
   QMenu menu;

   const QList<Common::SharedEntry>& sharedDirs = this->sharedEntryListModel.getSharedDirectories();

   if (!sharedDirs.isEmpty())
   {
      QAction* actionDownload = new QAction(
         QIcon(":/icons/ressources/download.png"),
         tr("Download && rehost (auto-select folder)"),
         &menu
      );
      actionDownload->setToolTip(tr("Downloads to the first shared folder with enough space. The file will be automatically shared with other peers."));
      connect(actionDownload, SIGNAL(triggered()), this, SIGNAL(download()));
      menu.addAction(actionDownload);

      if (sharedDirs.size() > 1)
      {
         QMenu* subMenu = menu.addMenu(QIcon(":/icons/ressources/folder.png"), tr("Download && rehost to..."));
         for (QListIterator<Common::SharedEntry> i(sharedDirs); i.hasNext();)
         {
            const auto& sharedDir = i.next();
            QAction* action = new QAction(
               QIcon(":/icons/ressources/download.png"),
               sharedDir.path.getPath(),
               subMenu
            );
            action->setData(QVariant::fromValue(sharedDir));
            connect(action, SIGNAL(triggered()), this, SLOT(actionTriggered()));
            subMenu->addAction(action);
         }
      }
   }
   else
   {
      QAction* noShareWarning = new QAction(
         QIcon(":/icons/ressources/error.png"),
         tr("No shared folders configured — add one in Settings to download && rehost"),
         &menu
      );
      noShareWarning->setEnabled(false);
      menu.addAction(noShareWarning);
   }

   this->onShowMenu(menu);

   menu.exec(globalPosition);
}

void DownloadMenu::actionTriggered()
{
   QAction* action = static_cast<QAction*>(this->sender());

   if (!action->data().isNull())
   {
      Common::SharedEntry sharedDir = action->data().value<Common::SharedEntry>();
      emit downloadTo(sharedDir.path.getPath(), sharedDir.ID);
   }
}
