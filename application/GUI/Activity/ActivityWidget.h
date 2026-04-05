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
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QDateTime>
#include <QSharedPointer>

#include <Common/RemoteCoreController/ICoreConnection.h>

namespace GUI
{
   class ActivityWidget : public QWidget
   {
      Q_OBJECT

   public:
      explicit ActivityWidget(QSharedPointer<RCC::ICoreConnection> coreConnection, QWidget* parent = nullptr);
      ~ActivityWidget();

   protected:
      void changeEvent(QEvent* event);

   private slots:
      void newState(const Protos::GUI::State& state);
      void coreConnected();
      void coreDisconnected(bool forced);
      void clearLog();

   private:
      enum EventType { EV_INFO, EV_PEER_JOIN, EV_PEER_LEAVE, EV_UPLOAD, EV_DOWNLOAD, EV_HASH, EV_WARN };

      void addEvent(EventType type, const QString& message);
      void trimLog();

      QListWidget* listWidget;
      QPushButton* btnClear;
      QLabel*      lblTitle;

      // State tracking — compare successive newState() calls to detect changes
      QHash<QByteArray, QString> prevPeers;       // peer_id bytes -> nick
      int                        prevCacheStatus = -1;
      QSet<quint64>              prevUploadIds;
      QSet<quint64>              prevDownloadIds;

      static const int MAX_EVENTS = 1000;

      QSharedPointer<RCC::ICoreConnection> coreConnection;
   };
}
