/**
  * DG-LAN - A decentralized LAN file sharing software.
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

#include <QObject>
#include <QTcpSocket>
#include <QFile>
#include <QSharedPointer>
#include <QMap>
#include <QPair>

#include <Core/FileManager/IFileManager.h>
#include <Core/PeerManager/IPeerManager.h>

#include <priv/Log.h>

namespace HS
{
   class HttpConnection : public QObject
   {
      Q_OBJECT
   public:
      HttpConnection(
         QSharedPointer<FM::IFileManager> fileManager,
         QSharedPointer<PM::IPeerManager> peerManager,
         QTcpSocket* socket
      );

      ~HttpConnection();

   signals:
      void finished(HttpConnection* connection);

   private slots:
      void readRequest();
      void bytesWritten(qint64 bytes);
      void socketDisconnected();

   private:
      static const int READ_BUFFER_SIZE = 65536;

      void processRequest();

      // Route handlers.
      void handleFileRequest(const QString& path);
      void handleApiFiles();
      void handleApiStatus();
      void handleApiHealth();

      // File resolution with path traversal protection.
      QString resolveFilePath(const QString& seHashHex, const QString& relativePath);

      // HTTP helpers.
      void sendResponse(int statusCode, const QByteArray& body, const QString& contentType = "text/plain");
      void sendError(int statusCode, const QString& message);
      void sendRedirect(const QString& url);
      void sendFileHeaders(int statusCode, qint64 contentLength, const QString& contentType,
                           qint64 rangeStart = -1, qint64 rangeEnd = -1, qint64 totalSize = -1);
      void streamNextChunk();
      void finish();

      static QString guessContentType(const QString& filename);
      static QPair<qint64, qint64> parseRangeHeader(const QString& rangeValue, qint64 fileSize);
      static QString computeETag(const QString& filePath, qint64 fileSize, qint64 lastModified);

      LOG_INIT_H("HttpConnection")

      QSharedPointer<FM::IFileManager> fileManager;
      QSharedPointer<PM::IPeerManager> peerManager;
      QTcpSocket* socket;

      // Parsed request state.
      QString method;
      QString requestPath;
      QMap<QString, QString> headers;
      bool headersParsed;

      // File streaming state.
      QFile* currentFile;
      qint64 bytesRemaining;
   };
}
