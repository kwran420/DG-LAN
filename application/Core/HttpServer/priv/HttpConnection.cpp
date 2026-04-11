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

#include <priv/HttpConnection.h>
using namespace HS;

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QDateTime>

#include <Common/Settings.h>
#include <Common/Global.h>
#include <Core/PeerManager/IPeer.h>

LOG_INIT_CPP(HttpConnection)

HttpConnection::HttpConnection(
   QSharedPointer<FM::IFileManager> fileManager,
   QSharedPointer<PM::IPeerManager> peerManager,
   QTcpSocket* socket
) :
   fileManager(fileManager),
   peerManager(peerManager),
   socket(socket),
   headersParsed(false),
   currentFile(nullptr),
   bytesRemaining(0)
{
   this->socket->setParent(this);
   connect(this->socket, &QTcpSocket::readyRead, this, &HttpConnection::readRequest);
   connect(this->socket, &QTcpSocket::disconnected, this, &HttpConnection::socketDisconnected);
}

HttpConnection::~HttpConnection()
{
   delete this->currentFile;
}

void HttpConnection::readRequest()
{
   if (this->headersParsed)
      return;

   // Limit total header size to 8 KiB to prevent memory abuse.
   static const int MAX_HEADER_SIZE = 8192;
   if (this->socket->bytesAvailable() > MAX_HEADER_SIZE)
   {
      sendError(413, "Request headers too large");
      return;
   }

   // Check if we have a complete set of headers (ends with \r\n\r\n).
   if (!this->socket->peek(this->socket->bytesAvailable()).contains("\r\n\r\n"))
      return;

   const QByteArray rawRequest = this->socket->readAll();
   const int headerEnd = rawRequest.indexOf("\r\n\r\n");
   const QByteArray headerBlock = rawRequest.left(headerEnd);
   const QList<QByteArray> lines = headerBlock.split('\n');

   if (lines.isEmpty())
   {
      sendError(400, "Empty request");
      return;
   }

   // Parse request line: "GET /path HTTP/1.1"
   const QByteArray requestLine = lines[0].trimmed();
   const QList<QByteArray> parts = requestLine.split(' ');
   if (parts.size() < 2)
   {
      sendError(400, "Malformed request line");
      return;
   }

   this->method = QString::fromLatin1(parts[0]);
   this->requestPath = QUrl::fromPercentEncoding(parts[1]);

   // Parse headers.
   for (int i = 1; i < lines.size(); ++i)
   {
      const QByteArray line = lines[i].trimmed();
      const int colonPos = line.indexOf(':');
      if (colonPos > 0)
      {
         const QString key = QString::fromLatin1(line.left(colonPos)).trimmed().toLower();
         const QString value = QString::fromLatin1(line.mid(colonPos + 1)).trimmed();
         this->headers.insert(key, value);
      }
   }

   this->headersParsed = true;
   processRequest();
}

void HttpConnection::processRequest()
{
   // Handle CORS preflight.
   if (this->method == "OPTIONS")
   {
      QByteArray response = "HTTP/1.1 204 No Content\r\n"
         "Access-Control-Allow-Origin: *\r\n"
         "Access-Control-Allow-Methods: GET, HEAD, OPTIONS\r\n"
         "Access-Control-Allow-Headers: Range, If-None-Match\r\n"
         "Access-Control-Max-Age: 86400\r\n"
         "Connection: close\r\n"
         "\r\n";
      this->socket->write(response);
      finish();
      return;
   }

   if (this->method != "GET" && this->method != "HEAD")
   {
      sendError(405, "Method not allowed");
      return;
   }

   // Route the request.
   if (this->requestPath.startsWith("/files/"))
   {
      const QString filePart = this->requestPath.mid(7); // After "/files/"
      handleFileRequest(filePart);
   }
   else if (this->requestPath == "/api/v1/files" || this->requestPath == "/api/v1/files/")
   {
      handleApiFiles();
   }
   else if (this->requestPath == "/api/v1/status" || this->requestPath == "/api/v1/status/")
   {
      handleApiStatus();
   }
   else if (this->requestPath == "/api/v1/health" || this->requestPath == "/api/v1/health/")
   {
      handleApiHealth();
   }
   else
   {
      sendError(404, "Not found");
   }
}

void HttpConnection::handleFileRequest(const QString& path)
{
   // Path format: {shared_entry_hash}/{relative/path/to/file}
   const int slashPos = path.indexOf('/');
   if (slashPos < 1)
   {
      sendError(400, "Invalid path: expected /files/{hash}/{path}");
      return;
   }

   const QString seHashHex = path.left(slashPos);
   const QString relativePath = path.mid(slashPos + 1);

   if (relativePath.isEmpty())
   {
      sendError(400, "Missing file path");
      return;
   }

   // Resolve the file with path traversal protection.
   const QString absolutePath = resolveFilePath(seHashHex, relativePath);
   if (absolutePath.isEmpty())
   {
      // File not found locally. Try peer redirect.
      const QList<PM::IPeer*> peers = this->peerManager->getPeers();

      for (PM::IPeer* peer : peers)
      {
         if (!peer->isMaster())
            continue;

         const quint32 peerHttpPort = peer->getHttpPort();
         if (peerHttpPort == 0)
            continue;

         const QHostAddress peerIp = peer->getIP();
         const QString redirectUrl = QString("http://%1:%2/files/%3/%4")
            .arg(peerIp.toString())
            .arg(peerHttpPort)
            .arg(seHashHex, relativePath);

         sendRedirect(redirectUrl);
         return;
      }

      sendError(404, "File not found");
      return;
   }

   // Check ETag for conditional requests.
   const QFileInfo fi(absolutePath);
   const qint64 fileSize = fi.size();
   const qint64 lastModified = fi.lastModified().toSecsSinceEpoch();
   const QString etag = computeETag(absolutePath, fileSize, lastModified);

   if (this->headers.value("if-none-match") == etag)
   {
      QByteArray response = "HTTP/1.1 304 Not Modified\r\n"
         "ETag: " + etag.toUtf8() + "\r\n"
         "Access-Control-Allow-Origin: *\r\n"
         "Connection: close\r\n"
         "\r\n";
      this->socket->write(response);
      finish();
      return;
   }

   const QString contentType = guessContentType(absolutePath);

   // Parse Range header.
   qint64 rangeStart = 0;
   qint64 rangeEnd = fileSize - 1;
   int statusCode = 200;

   if (this->headers.contains("range"))
   {
      const QPair<qint64, qint64> range = parseRangeHeader(this->headers.value("range"), fileSize);
      if (range.first < 0)
      {
         sendError(416, "Range not satisfiable");
         return;
      }
      rangeStart = range.first;
      rangeEnd = range.second;
      statusCode = 206;
   }

   // Open the file.
   this->currentFile = new QFile(absolutePath);
   if (!this->currentFile->open(QIODevice::ReadOnly))
   {
      delete this->currentFile;
      this->currentFile = nullptr;
      sendError(500, "Cannot open file");
      return;
   }

   this->currentFile->seek(rangeStart);
   this->bytesRemaining = rangeEnd - rangeStart + 1;

   // HEAD requests: send headers only.
   if (this->method == "HEAD")
   {
      sendFileHeaders(statusCode, this->bytesRemaining, contentType, rangeStart, rangeEnd, fileSize);
      finish();
      return;
   }

   // Send headers then stream body.
   sendFileHeaders(statusCode, this->bytesRemaining, contentType, rangeStart, rangeEnd, fileSize);
   connect(this->socket, &QTcpSocket::bytesWritten, this, &HttpConnection::bytesWritten);
   streamNextChunk();
}

void HttpConnection::handleApiFiles()
{
   const Protos::Common::Entries rootEntries = this->fileManager->getEntries();

   QJsonArray filesArray;

   for (int i = 0; i < rootEntries.entry_size(); ++i)
   {
      const auto& entry = rootEntries.entry(i);
      const QString name = QString::fromStdString(entry.name());
      const std::string& seHashRaw = entry.shared_entry().id().hash();
      const QString seHashHex = QByteArray(seHashRaw.data(), static_cast<int>(seHashRaw.size())).toHex();

      QJsonObject fileObj;
      fileObj["name"] = name;
      fileObj["size"] = static_cast<qint64>(entry.size());
      fileObj["is_directory"] = entry.type() == Protos::Common::Entry::DIR;
      fileObj["shared_entry_hash"] = seHashHex;
      fileObj["http_url"] = QString("/files/%1/").arg(seHashHex);

      // Build peer URLs for all alive master peers.
      QJsonArray peerUrls;
      const QList<PM::IPeer*> peers = this->peerManager->getPeers();
      for (PM::IPeer* peer : peers)
      {
         if (peer->isMaster() && peer->getHttpPort() > 0)
         {
            peerUrls.append(QString("http://%1:%2/files/%3/")
               .arg(peer->getIP().toString())
               .arg(peer->getHttpPort())
               .arg(seHashHex));
         }
      }
      fileObj["peer_urls"] = peerUrls;
      filesArray.append(fileObj);
   }

   QJsonObject root;
   root["files"] = filesArray;
   root["peer_id"] = SETTINGS.get<Common::Hash>("peer_id").toStr();

   sendResponse(200, QJsonDocument(root).toJson(QJsonDocument::Compact), "application/json");
}

void HttpConnection::handleApiStatus()
{
   const QList<PM::IPeer*> peers = this->peerManager->getPeers();

   QJsonArray peersArray;
   for (PM::IPeer* peer : peers)
   {
      QJsonObject peerObj;
      peerObj["nick"] = peer->getNick();
      peerObj["ip"] = peer->getIP().toString();
      peerObj["sharing_amount"] = static_cast<qint64>(peer->getSharingAmount());
      peerObj["download_rate"] = static_cast<qint64>(peer->getDownloadRate());
      peerObj["upload_rate"] = static_cast<qint64>(peer->getUploadRate());
      peerObj["is_master"] = peer->isMaster();
      peersArray.append(peerObj);
   }

   QJsonObject root;
   root["version"] = Common::Global::getVersionFull();
   root["peer_count"] = static_cast<int>(peers.size());
   root["peers"] = peersArray;

   sendResponse(200, QJsonDocument(root).toJson(QJsonDocument::Compact), "application/json");
}

void HttpConnection::handleApiHealth()
{
   QJsonObject root;
   root["status"] = "ok";
   root["version"] = Common::Global::getVersionFull();
   sendResponse(200, QJsonDocument(root).toJson(QJsonDocument::Compact), "application/json");
}

QString HttpConnection::resolveFilePath(const QString& seHashHex, const QString& relativePath)
{
   // Validate hash format: 56 hex characters (SHA3-224).
   static const QRegularExpression hashRegex("^[0-9a-fA-F]{1,64}$");
   if (!hashRegex.match(seHashHex).hasMatch())
      return QString();

   // Reject null bytes.
   if (relativePath.contains(QChar('\0')) || seHashHex.contains(QChar('\0')))
      return QString();

   // Decode the hash.
   const QByteArray hashBytes = QByteArray::fromHex(seHashHex.toLatin1());
   const Common::Hash seId(hashBytes);

   // Look up the shared entry path.
   const QString sePath = this->fileManager->getSharedEntry(seId);
   if (sePath.isEmpty())
      return QString();

   // Clean and join the path.
   const QString joined = QDir::cleanPath(sePath + "/" + relativePath);

   // Canonicalize to resolve symlinks and ".." components.
   const QFileInfo fi(joined);
   const QString canonical = fi.canonicalFilePath();
   if (canonical.isEmpty())
      return QString();

   // Path traversal check: canonical must start with the shared entry path.
   const QString canonicalSe = QFileInfo(sePath).canonicalFilePath();
   if (canonicalSe.isEmpty() || !canonical.startsWith(canonicalSe))
      return QString();

   // Must be a regular file.
   if (!fi.isFile())
      return QString();

   return canonical;
}

void HttpConnection::sendResponse(int statusCode, const QByteArray& body, const QString& contentType)
{
   const char* statusText = "OK";
   switch (statusCode)
   {
      case 200: statusText = "OK"; break;
      case 204: statusText = "No Content"; break;
      case 206: statusText = "Partial Content"; break;
      case 301: statusText = "Moved Permanently"; break;
      case 302: statusText = "Found"; break;
      case 304: statusText = "Not Modified"; break;
      case 400: statusText = "Bad Request"; break;
      case 403: statusText = "Forbidden"; break;
      case 404: statusText = "Not Found"; break;
      case 405: statusText = "Method Not Allowed"; break;
      case 413: statusText = "Payload Too Large"; break;
      case 416: statusText = "Range Not Satisfiable"; break;
      case 500: statusText = "Internal Server Error"; break;
      case 503: statusText = "Service Unavailable"; break;
   }

   QByteArray response;
   response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
   response.append(QString("Content-Type: %1\r\n").arg(contentType).toUtf8());
   response.append(QString("Content-Length: %1\r\n").arg(body.size()).toUtf8());
   response.append("Access-Control-Allow-Origin: *\r\n");
   response.append("Access-Control-Expose-Headers: Content-Length, Content-Range, ETag\r\n");
   response.append("Connection: close\r\n");
   response.append("\r\n");
   response.append(body);

   this->socket->write(response);
   finish();
}

void HttpConnection::sendError(int statusCode, const QString& message)
{
   QJsonObject error;
   error["error"] = QJsonObject{{"code", statusCode}, {"message", message}};
   sendResponse(statusCode, QJsonDocument(error).toJson(QJsonDocument::Compact), "application/json");
}

void HttpConnection::sendRedirect(const QString& url)
{
   QByteArray response;
   response.append("HTTP/1.1 302 Found\r\n");
   response.append(QString("Location: %1\r\n").arg(url).toUtf8());
   response.append("Access-Control-Allow-Origin: *\r\n");
   response.append("Connection: close\r\n");
   response.append("\r\n");

   this->socket->write(response);
   finish();
}

void HttpConnection::sendFileHeaders(int statusCode, qint64 contentLength, const QString& contentType,
                                     qint64 rangeStart, qint64 rangeEnd, qint64 totalSize)
{
   const char* statusText = (statusCode == 206) ? "Partial Content" : "OK";

   QByteArray response;
   response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
   response.append(QString("Content-Type: %1\r\n").arg(contentType).toUtf8());
   response.append(QString("Content-Length: %1\r\n").arg(contentLength).toUtf8());
   response.append("Accept-Ranges: bytes\r\n");
   response.append("Access-Control-Allow-Origin: *\r\n");
   response.append("Access-Control-Expose-Headers: Content-Length, Content-Range, ETag, Accept-Ranges\r\n");
   response.append("Connection: close\r\n");

   if (statusCode == 206)
   {
      response.append(QString("Content-Range: bytes %1-%2/%3\r\n")
         .arg(rangeStart).arg(rangeEnd).arg(totalSize).toUtf8());
   }

   // ETag.
   if (this->currentFile)
   {
      const QFileInfo fi(this->currentFile->fileName());
      response.append(QString("ETag: %1\r\n")
         .arg(computeETag(fi.filePath(), fi.size(), fi.lastModified().toSecsSinceEpoch())).toUtf8());
   }

   // Content-Disposition for downloads.
   if (this->currentFile)
   {
      const QFileInfo fi(this->currentFile->fileName());
      response.append(QString("Content-Disposition: inline; filename=\"%1\"\r\n")
         .arg(fi.fileName()).toUtf8());
   }

   response.append("\r\n");
   this->socket->write(response);
}

void HttpConnection::streamNextChunk()
{
   if (!this->currentFile || this->bytesRemaining <= 0)
   {
      finish();
      return;
   }

   // Don't overwhelm the socket buffer.
   if (this->socket->bytesToWrite() > READ_BUFFER_SIZE * 4)
      return;

   const qint64 toRead = qMin(static_cast<qint64>(READ_BUFFER_SIZE), this->bytesRemaining);
   const QByteArray data = this->currentFile->read(toRead);

   if (data.isEmpty())
   {
      finish();
      return;
   }

   this->socket->write(data);
   this->bytesRemaining -= data.size();

   if (this->bytesRemaining <= 0)
      finish();
}

void HttpConnection::bytesWritten(qint64)
{
   if (this->bytesRemaining > 0)
      streamNextChunk();
}

void HttpConnection::socketDisconnected()
{
   emit finished(this);
}

void HttpConnection::finish()
{
   if (this->currentFile)
   {
      this->currentFile->close();
      delete this->currentFile;
      this->currentFile = nullptr;
   }
   this->socket->disconnectFromHost();
}

QString HttpConnection::guessContentType(const QString& filename)
{
   const QString ext = QFileInfo(filename).suffix().toLower();

   // Video.
   if (ext == "mp4" || ext == "m4v") return "video/mp4";
   if (ext == "mkv") return "video/x-matroska";
   if (ext == "webm") return "video/webm";
   if (ext == "avi") return "video/x-msvideo";
   if (ext == "mov") return "video/quicktime";

   // Audio.
   if (ext == "mp3") return "audio/mpeg";
   if (ext == "flac") return "audio/flac";
   if (ext == "ogg") return "audio/ogg";
   if (ext == "wav") return "audio/wav";
   if (ext == "m4a") return "audio/mp4";

   // Images.
   if (ext == "png") return "image/png";
   if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
   if (ext == "gif") return "image/gif";
   if (ext == "webp") return "image/webp";
   if (ext == "svg") return "image/svg+xml";
   if (ext == "ico") return "image/x-icon";

   // Archives.
   if (ext == "zip") return "application/zip";
   if (ext == "7z") return "application/x-7z-compressed";
   if (ext == "rar") return "application/x-rar-compressed";
   if (ext == "tar") return "application/x-tar";
   if (ext == "gz") return "application/gzip";
   if (ext == "iso") return "application/x-iso9660-image";

   // Documents.
   if (ext == "pdf") return "application/pdf";
   if (ext == "txt" || ext == "log" || ext == "md") return "text/plain; charset=utf-8";
   if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
   if (ext == "css") return "text/css; charset=utf-8";
   if (ext == "js") return "application/javascript";
   if (ext == "json") return "application/json";
   if (ext == "xml") return "application/xml";

   // Executables / Installers.
   if (ext == "exe" || ext == "msi") return "application/octet-stream";

   return "application/octet-stream";
}

QPair<qint64, qint64> HttpConnection::parseRangeHeader(const QString& rangeValue, qint64 fileSize)
{
   // Format: "bytes=start-end" or "bytes=start-" or "bytes=-suffix"
   static const QRegularExpression rangeRegex("^bytes=(\\d*)-(\\d*)$");
   const auto match = rangeRegex.match(rangeValue.trimmed());

   if (!match.hasMatch())
      return qMakePair(static_cast<qint64>(-1), static_cast<qint64>(-1));

   const QString startStr = match.captured(1);
   const QString endStr = match.captured(2);

   qint64 start, end;

   if (startStr.isEmpty() && endStr.isEmpty())
      return qMakePair(static_cast<qint64>(-1), static_cast<qint64>(-1));

   if (startStr.isEmpty())
   {
      // Suffix range: bytes=-500 means last 500 bytes.
      const qint64 suffix = endStr.toLongLong();
      if (suffix <= 0 || suffix > fileSize)
         return qMakePair(static_cast<qint64>(-1), static_cast<qint64>(-1));
      start = fileSize - suffix;
      end = fileSize - 1;
   }
   else if (endStr.isEmpty())
   {
      // Open-ended: bytes=500-
      start = startStr.toLongLong();
      end = fileSize - 1;
   }
   else
   {
      start = startStr.toLongLong();
      end = endStr.toLongLong();
   }

   if (start < 0 || start >= fileSize || end < start || end >= fileSize)
      return qMakePair(static_cast<qint64>(-1), static_cast<qint64>(-1));

   return qMakePair(start, end);
}

QString HttpConnection::computeETag(const QString& filePath, qint64 fileSize, qint64 lastModified)
{
   QCryptographicHash hash(QCryptographicHash::Md5);
   hash.addData(filePath.toUtf8());
   hash.addData(QByteArray::number(fileSize));
   hash.addData(QByteArray::number(lastModified));
   return QString("\"" + QString::fromLatin1(hash.result().toHex().left(16)) + "\"");
}
