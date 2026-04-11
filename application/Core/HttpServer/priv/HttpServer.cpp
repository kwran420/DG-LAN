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

#include <priv/HttpServer.h>
using namespace HS;

#include <Common/Settings.h>

LOG_INIT_CPP(HttpServer)

HttpServer::HttpServer(
   QSharedPointer<FM::IFileManager> fileManager,
   QSharedPointer<PM::IPeerManager> peerManager
) :
   fileManager(fileManager),
   peerManager(peerManager)
{
   const quint32 PORT = SETTINGS.get<quint32>("http_server_port");

   if (!this->tcpServer.listen(QHostAddress::Any, PORT))
   {
      L_ERRO(QString("HttpServer: unable to listen on port %1: %2").arg(PORT).arg(this->tcpServer.errorString()));
      return;
   }

   connect(&this->tcpServer, &QTcpServer::newConnection, this, &HttpServer::newConnection);

   L_USER(QString("HttpServer listening on port %1").arg(PORT));
}

HttpServer::~HttpServer()
{
   this->tcpServer.close();

   for (HttpConnection* connection : this->connections)
   {
      connection->disconnect(this);
      delete connection;
   }

   L_DEBU("HttpServer deleted");
}

void HttpServer::newConnection()
{
   QTcpSocket* socket = this->tcpServer.nextPendingConnection();
   if (!socket)
      return;

   const quint32 maxConnections = SETTINGS.get<quint32>("http_max_connections");
   if (static_cast<quint32>(this->connections.size()) >= maxConnections)
   {
      L_WARN("HttpServer: rejecting connection, max connections reached");
      socket->write("HTTP/1.1 503 Service Unavailable\r\nConnection: close\r\n\r\n");
      socket->disconnectFromHost();
      socket->deleteLater();
      return;
   }

   HttpConnection* connection = new HttpConnection(this->fileManager, this->peerManager, socket);
   connect(connection, &HttpConnection::finished, this, &HttpServer::connectionFinished);
   this->connections.append(connection);
}

void HttpServer::connectionFinished(HttpConnection* connection)
{
   this->connections.removeOne(connection);
   connection->deleteLater();
}
