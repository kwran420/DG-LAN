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

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTableWidget>
#include <QDateTime>
#include <QSharedPointer>

#include <Common/RemoteCoreController/ICoreConnection.h>

namespace GUI
{
   class HashingProgressWidget : public QWidget
   {
      Q_OBJECT

   public:
      explicit HashingProgressWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, QWidget* parent = nullptr);
      ~HashingProgressWidget();

   protected:
      void changeEvent(QEvent* event);

   private slots:
      void newState(const Protos::GUI::State& state);
      void coreConnected();
      void coreDisconnected(bool forced);

   private:
      void updateStatusLabel(Protos::GUI::State::Stats::CacheStatus status, int progress);
      void updateSharedDirs(const Protos::GUI::State& state);

      // Status section
      QLabel*       lblStatusIcon;
      QLabel*       lblStatusText;
      QProgressBar* progressBar;
      QLabel*       lblProgressPct;
      QLabel*       lblSinceTime;

      // Shared directories table (columns: Path, Size, Free Space)
      QTableWidget* tblDirs;

      QSharedPointer<RCC::ICoreConnection> coreConnection;

      // Track when current operation started
      QDateTime operationStartTime;
      Protos::GUI::State::Stats::CacheStatus lastStatus = Protos::GUI::State::Stats::UNKNOWN;
   };
}
