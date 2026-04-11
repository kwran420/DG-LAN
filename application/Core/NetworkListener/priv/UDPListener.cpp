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

#include <priv/UDPListener.h>
using namespace NL;

#include <limits>

#if defined(Q_OS_LINUX)
   #include <netinet/in.h>
#elif defined(Q_OS_DARWIN)
   #include <sys/types.h>
   #include <sys/socket.h>
#elif defined(Q_OS_WIN32)
   #include <Winsock.h>
#endif

#include <QRandomGenerator64>
#include <QDateTime>          // DG-LAN: heartbeat time logging
#include <QNetworkInterface>  // DG-LAN
#include <QNetworkAddressEntry> // DG-LAN

#include <google/protobuf/message.h>

#include <Common/Settings.h>
#include <Common/Constants.h>
#include <Common/Global.h>
#include <Common/ProtoHelper.h>

#include <Core/PeerManager/IPeer.h>

#include <priv/Log.h>
#include <priv/Utils.h>

/**
  * @class NL::UDPListener
  *
  * The goals of this class are:
  *  - Listen for incoming unicast and multicast datagrams, process them and dispatch the information the correct manager: 'FileManager', 'DownloadManager' or 'PeerManager'.
  *  - Offer methods to send unicast or multicast datagrams.
  *  - Periodically send a 'IMAlive' multicast datagrams.
  *
  * @author mcuony
  * @author gburri
  */

UDPListener::UDPListener(
   QSharedPointer<FM::IFileManager> fileManager,
   QSharedPointer<PM::IPeerManager> peerManager,
   QSharedPointer<UM::IUploadManager> uploadManager,
   QSharedPointer<DM::IDownloadManager> downloadManager,
   quint16 unicastPort
) :
   MAX_UDP_DATAGRAM_PAYLOAD_SIZE(static_cast<int>(SETTINGS.get<quint32>("max_udp_datagram_size"))),
   bodyBuffer(UDPListener::buffer + Common::MessageHeader::HEADER_SIZE),
   UNICAST_PORT(unicastPort),
   MULTICAST_PORT(SETTINGS.get<quint32>("multicast_port")),
   multicastGroup(Utils::getMulticastGroup()),
   fileManager(fileManager),
   peerManager(peerManager),
   uploadManager(uploadManager),
   downloadManager(downloadManager),
   currentIMAliveTag(0),
   nextHashRequestType(FIRST_HASHES),
   loggerIMAlive(LM::Builder::newLogger("NetworkListener (IMAlive)")),
   // DG-LAN: fallback state initialisation
   multicastFailureCount(0),
   broadcastFallbackActive(false),
   subnetScanActive(false),
   subnetScanIndex(0),
   imAliveCounter(0)
{
   this->initMulticastUDPSocket();
   this->initUnicastUDPSocket();

   connect(&this->timerIMAlive, &QTimer::timeout, this, &UDPListener::sendIMAliveMessage);
   this->timerIMAlive.start(static_cast<int>(SETTINGS.get<quint32>("peer_imalive_period")));

   // DG-LAN: subnet scan timer — fires once per probe slot, stopped when not scanning
   connect(&this->timerSubnetScan, &QTimer::timeout, this, &UDPListener::sendNextSubnetScanProbe);

   this->sendIMAliveMessage();
}

/**
  * Send an UDP unicast datagram to the given peer.
  * @return 'false' if the datagram can't be sent.
  */
INetworkListener::SendStatus UDPListener::send(Common::MessageHeader::MessageType type, const google::protobuf::Message& message, const Common::Hash& peerID)
{
   PM::IPeer* peer = this->peerManager->getPeer(peerID);
   if (!peer)
      return INetworkListener::SendStatus::PEER_UNKNOWN;

   int messageSize;
   if (!(messageSize = this->writeMessageToBuffer(type, message)))
      return INetworkListener::SendStatus::MESSAGE_TOO_LARGE;

   L_DEBU(QString("Send unicast UDP to %1, header.getType(): %2, message size: %3 \n%4").
      arg(peer->toStringLog()).
      arg(Common::MessageHeader::messToStr(type)).
      arg(messageSize).
      arg(Common::ProtoHelper::getDebugStr(message))
   );

   if (this->unicastSocket.writeDatagram(this->buffer, messageSize, peer->getIP(), peer->getPort()) == -1)
   {
      L_DEBU(QString("Failed to send unicast packet: %1").arg(this->unicastSocket.errorString()));
      return INetworkListener::SendStatus::UNABLE_TO_SEND;
   }

   return INetworkListener::SendStatus::OK;
}

/**
  * Send an UDP multicast message.
  */
INetworkListener::SendStatus UDPListener::send(Common::MessageHeader::MessageType type, const google::protobuf::Message& message)
{
   int messageSize;
   if (!(messageSize = this->writeMessageToBuffer(type, message)))
      return INetworkListener::SendStatus::MESSAGE_TOO_LARGE;

#if DEBUG
   QString logMess = QString("Send multicast UDP: header.getType() = %1, message size = %2 \n%3").
      arg(Common::MessageHeader::messToStr(type)).
      arg(messageSize).
      arg(Common::ProtoHelper::getDebugStr(message));

   if (type == Common::MessageHeader::CORE_IM_ALIVE)
      LOG_DEBU(this->loggerIMAlive, logMess);
   else
      L_DEBU(logMess);
#endif

   if (this->multicastSocket.writeDatagram(this->buffer, messageSize, this->multicastGroup, MULTICAST_PORT) == -1)
   {
      L_WARN(QString("Failed to send multicast packet: %1").arg(this->unicastSocket.errorString()));

      // DG-LAN: track multicast failures and escalate to broadcast fallback
      const quint32 threshold = SETTINGS.get<quint32>("multicast_failure_threshold");
      if (threshold > 0)
      {
         ++this->multicastFailureCount;
         if (this->multicastFailureCount >= static_cast<int>(threshold) && !this->broadcastFallbackActive)
         {
            L_WARN("DG-LAN: multicast failures exceeded threshold — activating broadcast fallback");
            this->broadcastFallbackActive = true;
         }
      }

      return INetworkListener::SendStatus::UNABLE_TO_SEND;
   }

   // Reset failure counter on success
   this->multicastFailureCount = 0;
   return INetworkListener::SendStatus::OK;
}

void UDPListener::sendIMAliveMessage()
{
   Protos::Core::IMAlive IMAliveMessage;
   IMAliveMessage.set_version(Common::Constants::PROTOCOL_VERSION);
   Common::ProtoHelper::setStr(IMAliveMessage, &Protos::Core::IMAlive::mutable_core_version, Common::Global::getVersionFull());
   IMAliveMessage.set_port(this->UNICAST_PORT);

   const QString& nick = this->peerManager->getSelf()->getNick();
   Common::ProtoHelper::setStr(IMAliveMessage, &Protos::Core::IMAlive::mutable_nick, nick.length() > MAX_NICK_LENGTH ? nick.left(MAX_NICK_LENGTH) : nick);

   IMAliveMessage.set_amount(this->fileManager->getAmount());
   IMAliveMessage.set_download_rate(this->downloadManager->getDownloadRate());
   IMAliveMessage.set_upload_rate(this->uploadManager->getUploadRate());
   IMAliveMessage.set_lan_speed(SETTINGS.get<quint32>("lan_speed"));

   // Propagate master key hash so all peers share the same password.
   const Common::Hash localMasterKey = SETTINGS.get<Common::Hash>("master_key_hash");
   if (!localMasterKey.isNull())
      IMAliveMessage.mutable_master_key_hash()->set_hash(localMasterKey.getData(), Common::Hash::HASH_SIZE);

   IMAliveMessage.set_is_master(!SETTINGS.get<bool>("client_mode"));

   this->currentIMAliveTag = QRandomGenerator64::global()->generate64();
   IMAliveMessage.set_tag(this->currentIMAliveTag);

   // We fill the rest of the message with a maximum of needed hashes.
   static const quint32 MAX_IMALIVE_THROUGHPUT = SETTINGS.get<quint32>("max_imalive_throughput");
   static const int AVERAGE_FIXED_SIZE = 100; // [Byte]. Header size + information in the 'IMAlive' message without the hashes.
   static const quint32 IMALIVE_PERIOD = SETTINGS.get<quint32>("peer_imalive_period") / 1000; // [s]
   static const int FIXED_RATE_PER_PEER = AVERAGE_FIXED_SIZE / IMALIVE_PERIOD; // [Byte/s]
   static const int HASH_SIZE = Common::Hash::HASH_SIZE + 4; // "4" is the overhead added by protobuff for each hash.

   const int numberOfPeers = this->peerManager->getNbOfPeers();
   const int maxNumberOfHashesToSend = numberOfPeers == 0 ? std::numeric_limits<int>::max() : IMALIVE_PERIOD * (MAX_IMALIVE_THROUGHPUT - numberOfPeers * FIXED_RATE_PER_PEER) / (numberOfPeers * HASH_SIZE);

   int numberOfHashesToSend = (this->MAX_UDP_DATAGRAM_PAYLOAD_SIZE - IMAliveMessage.ByteSizeLong() - Common::MessageHeader::HEADER_SIZE) / HASH_SIZE;
   if (numberOfHashesToSend > maxNumberOfHashesToSend)
      numberOfHashesToSend = maxNumberOfHashesToSend;

   // The requested hashes method alternates from the first hashes and the oldest hashes.
   // We are trying to have the knowledge about who has which chunk for the whole download queue (IDownloadManager::getTheOldestUnfinishedChunks(..))
   // and for the chunks we want to download first (IDownloadManager::getTheFirstUnfinishedChunks(..)).
   switch (this->nextHashRequestType)
   {
   case FIRST_HASHES:
      this->currentChunkDownloaders = this->downloadManager->getTheFirstUnfinishedChunks(numberOfHashesToSend);
      this->nextHashRequestType = OLDEST_HASHES;
      break;
   case OLDEST_HASHES:
      this->currentChunkDownloaders = this->downloadManager->getTheOldestUnfinishedChunks(numberOfHashesToSend);
      this->nextHashRequestType = FIRST_HASHES;
      break;
   }

   IMAliveMessage.mutable_chunk()->Reserve(this->currentChunkDownloaders.size());
   for (QListIterator<QSharedPointer<DM::IChunkDownloader>> i(this->currentChunkDownloaders); i.hasNext();)
   {
      QSharedPointer<DM::IChunkDownloader> chunkDownloader = i.next();
      IMAliveMessage.add_chunk()->set_hash(chunkDownloader->getHash().getData(), Common::Hash::HASH_SIZE);

      // If we already have the chunk . . .
      QSharedPointer<FM::IChunk> chunk = this->fileManager->getChunk(chunkDownloader->getHash());
      if (!chunk.isNull() && chunk->isComplete())
         chunkDownloader->addPeer(this->peerManager->getSelf());
      else
         chunkDownloader->rmPeer(this->peerManager->getSelf());
   }

   emit IMAliveMessageToBeSend(IMAliveMessage);

   this->send(Common::MessageHeader::CORE_IM_ALIVE, IMAliveMessage);

   // DG-LAN: periodic heartbeat log to GUI — every 12 sends (~1 min at default 5s period)
   // numberOfPeers counts remote peers only; +1 to include ourselves.
   if (++this->imAliveCounter % 12 == 0)
      L_USER(QString("Network [%1]: heartbeat \u2014 %2 peer(s) online")
         .arg(QDateTime::currentDateTime().toString("HH:mm"))
         .arg(numberOfPeers + 1));

   // DG-LAN: always broadcast on ALL interfaces.
   // Multicast silently fails on ZeroTier (send succeeds but never arrives),
   // so we can't rely on failure counting. Broadcast is cheap and reliable.
   this->sendBroadcastIMAlive();

   // DG-LAN: send unicast heartbeat directly to every known peer.
   // ZeroTier doesn't support multicast or broadcast, so without direct
   // unicast heartbeats, ZT peers would time out after ~15 seconds.
   {
      const int messageSize = static_cast<int>(
         Common::MessageHeader::HEADER_SIZE +
         Common::MessageHeader::readHeader(this->buffer).getSize()
      );
      const QList<PM::IPeer*> peers = this->peerManager->getPeers();
      for (PM::IPeer* peer : peers)
         this->unicastSocket.writeDatagram(this->buffer, messageSize, peer->getIP(), MULTICAST_PORT);
   }

   // DG-LAN: drain gossip candidates from PeerManager and probe each one
   const auto gossipCandidates = this->peerManager->takeGossipCandidates();
   if (!gossipCandidates.isEmpty())
      L_USER(QString("Network: sharing peer info with %1 new contact(s)").arg(gossipCandidates.size()));
   for (const auto& candidate : gossipCandidates)
      this->sendUnicastIMAlive(candidate.first, candidate.second);

   // DG-LAN: periodic subnet scan — every 12 heartbeats (~60 seconds).
   // Unicast scan is the ONLY reliable discovery on ZeroTier.
   if (!this->subnetScanActive && (this->imAliveCounter % 12 == 1))
      this->runSubnetScan();
}

void UDPListener::rebindSockets()
{
   // DG-LAN: reset fallback state on rebind (network interface changed)
   this->multicastFailureCount = 0;
   this->broadcastFallbackActive = false;
   this->subnetScanActive = false;
   this->timerSubnetScan.stop();
   this->subnetScanTargets.clear();
   this->subnetScanIndex = 0;

   this->initMulticastUDPSocket();
   this->initUnicastUDPSocket();
}

// DG-LAN: Send IMAlive via broadcast on ALL network interfaces.
// ZeroTier doesn't support multicast, so broadcast is the primary discovery
// mechanism for virtual networks. We send on every interface to support
// hybrid LAN + ZeroTier setups.
void UDPListener::sendBroadcastIMAlive()
{
   // Reuse the existing buffer — it was just filled by sendIMAliveMessage()
   const int messageSize = static_cast<int>(
      Common::MessageHeader::HEADER_SIZE +
      Common::MessageHeader::readHeader(this->buffer).getSize()
   );

   const QString specificIface = SETTINGS.get<QString>("network_interface_name");
   int sent = 0;

   foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces())
   {
      if (!iface.isValid() ||
          iface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
          !iface.flags().testFlag(QNetworkInterface::IsUp) ||
          !iface.flags().testFlag(QNetworkInterface::IsRunning))
         continue;

      // If a specific interface is configured, only broadcast on that one
      if (!specificIface.isEmpty() && iface.name() != specificIface)
         continue;

      // DG-LAN: skip packet-capture loopback adapters (Npcap/WinPcap)
      if (iface.humanReadableName().contains("Npcap", Qt::CaseInsensitive) ||
          iface.humanReadableName().contains("Loopback", Qt::CaseInsensitive))
         continue;

      foreach (const QNetworkAddressEntry& entry, iface.addressEntries())
      {
         if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
            continue;
         if (entry.broadcast().isNull())
            continue;
         // DG-LAN: skip APIPA/link-local addresses (169.254.x.x)
         if ((entry.ip().toIPv4Address() & 0xFFFF0000) == 0xA9FE0000)
            continue;

         L_DEBU(QString("DG-LAN: Sending broadcast IMAlive to %1 via %2")
            .arg(entry.broadcast().toString()).arg(iface.humanReadableName()));

         if (this->unicastSocket.writeDatagram(this->buffer, messageSize, entry.broadcast(), MULTICAST_PORT) == -1)
            L_WARN(QString("DG-LAN: broadcast on %1 failed: %2")
               .arg(iface.humanReadableName()).arg(this->unicastSocket.errorString()));
         else
            ++sent;

         break; // one broadcast per interface is enough
      }
   }

   if (sent == 0)
      L_WARN("DG-LAN: sendBroadcastIMAlive: no broadcast address available on any interface");
}

// DG-LAN: Fallback level 3 — enumerate all host addresses in ALL subnets
// and prepare a rate-limited probe list. Covers both LAN and ZeroTier.
void UDPListener::runSubnetScan()
{
   if (this->subnetScanActive)
      return;

   this->subnetScanTargets.clear();

   const QString specificIface = SETTINGS.get<QString>("network_interface_name");

   foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces())
   {
      if (!iface.isValid() ||
          iface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
          !iface.flags().testFlag(QNetworkInterface::IsUp) ||
          !iface.flags().testFlag(QNetworkInterface::IsRunning))
         continue;

      if (!specificIface.isEmpty() && iface.name() != specificIface)
         continue;

      // DG-LAN: skip packet-capture loopback adapters (Npcap/WinPcap)
      if (iface.humanReadableName().contains("Npcap", Qt::CaseInsensitive) ||
          iface.humanReadableName().contains("Loopback", Qt::CaseInsensitive))
         continue;

      foreach (const QNetworkAddressEntry& entry, iface.addressEntries())
      {
         if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol)
            continue;

         // DG-LAN: skip APIPA/link-local addresses (169.254.x.x)
         if ((entry.ip().toIPv4Address() & 0xFFFF0000) == 0xA9FE0000)
            continue;

         const quint32 ip = entry.ip().toIPv4Address();
         const quint32 mask = entry.netmask().toIPv4Address();
         if (mask == 0)
            continue;

         const quint32 network = ip & mask;
         const quint32 broadcast = network | (~mask);
         const quint32 numHosts = broadcast - network - 1;

         if (numHosts == 0 || numHosts > 65534) // skip /16+ to avoid flooding
            continue;

         L_USER(QString("Network: scanning %1 host(s) in %2/%3 (%4)")
            .arg(numHosts)
            .arg(QHostAddress(network).toString())
            .arg(QHostAddress(mask).toString())
            .arg(iface.humanReadableName()));

         for (quint32 h = 1; h <= numHosts; ++h)
         {
            QHostAddress target(network + h);
            if (target == entry.ip()) // skip our own address
               continue;
            this->subnetScanTargets.append(target);
         }
         break; // one IPv4 entry per interface is enough
      }
   }

   if (this->subnetScanTargets.isEmpty())
   {
      L_WARN("DG-LAN: runSubnetScan: no hosts to probe");
      return;
   }

   this->subnetScanIndex = 0;
   this->subnetScanActive = true;

   // Rate-limit: spread all probes evenly across one peer_imalive_period window
   const int period = static_cast<int>(SETTINGS.get<quint32>("peer_imalive_period"));
   const int intervalMs = qMax(1, period / this->subnetScanTargets.size());
   L_DEBU(QString("DG-LAN: Subnet scan: %1 probes, interval %2 ms")
      .arg(this->subnetScanTargets.size()).arg(intervalMs));

   this->timerSubnetScan.start(intervalMs);
}

// DG-LAN: Send one probe to the next address in the scan list.
void UDPListener::sendNextSubnetScanProbe()
{
   if (this->subnetScanIndex >= this->subnetScanTargets.size())
   {
      // Scan complete — stop timer, reset for next trigger
      this->timerSubnetScan.stop();
      this->subnetScanActive = false;
      this->subnetScanTargets.clear();
      this->subnetScanIndex = 0;
      L_DEBU("DG-LAN: subnet scan cycle complete");
      return;
   }

   const QHostAddress& target = this->subnetScanTargets[this->subnetScanIndex++];
   this->sendUnicastIMAlive(target, MULTICAST_PORT);
}

// DG-LAN: Send a unicast IMAlive directly to a specific peer address.
// Used by subnet scan and for targeting known hosts / stable peers.
void UDPListener::sendUnicastIMAlive(const QHostAddress& addr, quint16 port)
{
   // Build a minimal IMAlive (no chunk hashes — just enough to be discovered)
   Protos::Core::IMAlive msg;
   msg.set_version(Common::Constants::PROTOCOL_VERSION);
   Common::ProtoHelper::setStr(msg, &Protos::Core::IMAlive::mutable_core_version, Common::Global::getVersionFull());
   msg.set_port(this->UNICAST_PORT);
   const QString& nick = this->peerManager->getSelf()->getNick();
   Common::ProtoHelper::setStr(msg, &Protos::Core::IMAlive::mutable_nick,
      nick.length() > MAX_NICK_LENGTH ? nick.left(MAX_NICK_LENGTH) : nick);
   msg.set_amount(this->fileManager->getAmount());
   msg.set_tag(this->currentIMAliveTag);

   const int messageSize = this->writeMessageToBuffer(Common::MessageHeader::CORE_IM_ALIVE, msg);
   if (!messageSize)
      return;

   // DG-LAN: send to multicast port — the multicast socket is always bound to
   // 0.0.0.0 so it receives on ALL interfaces. The unicast socket may be bound
   // to a specific IP and miss traffic from other interfaces.
   this->unicastSocket.writeDatagram(this->buffer, messageSize, addr, port);
}

void UDPListener::processPendingMulticastDatagrams()
{
   static const int MAX_DATAGRAMS_PER_CALL = 500;
   int processed = 0;
   while (this->multicastSocket.hasPendingDatagrams() && processed++ < MAX_DATAGRAMS_PER_CALL)
   {
      QHostAddress peerAddress;
      bool readError = false;
      const Common::MessageHeader& header = this->readDatagramToBuffer(this->multicastSocket, peerAddress, readError);
      if (header.isNull())
      {
         if (readError)
            break;
         continue;
      }

      try
      {
         const Common::Message& message = Common::Message::readMessageBody(header, this->bodyBuffer);

         switch (header.getType())
         {
         case Common::MessageHeader::CORE_IM_ALIVE:
            {
               const Protos::Core::IMAlive& IMAliveMessage = message.getMessage<Protos::Core::IMAlive>();

               this->peerManager->updatePeer(
                  header.getSenderID(),
                  peerAddress,
                  IMAliveMessage.port(),
                  Common::ProtoHelper::getStr(IMAliveMessage, &Protos::Core::IMAlive::nick),
                  IMAliveMessage.amount(),
                  Common::ProtoHelper::getStr(IMAliveMessage, &Protos::Core::IMAlive::core_version),
                  IMAliveMessage.download_rate(),
                  IMAliveMessage.upload_rate(),
                  IMAliveMessage.version(),
                  IMAliveMessage.lan_speed(),
                  IMAliveMessage.is_master()
               );

               // Adopt the master key hash from a peer if we don't have one yet
               // and haven't deliberately cleared it via reset.
               if (IMAliveMessage.has_master_key_hash() && IMAliveMessage.master_key_hash().hash().size() == Common::Hash::HASH_SIZE)
               {
                  Common::Hash peerKey(IMAliveMessage.master_key_hash().hash().data());
                  Common::Hash localKey = SETTINGS.get<Common::Hash>("master_key_hash");
                  if (!peerKey.isNull() && localKey.isNull() && !SETTINGS.get<bool>("master_key_was_reset"))
                  {
                     SETTINGS.set("master_key_hash", peerKey);
                     SETTINGS.save();
                  }
               }

               if (IMAliveMessage.chunk_size() > 0)
               {
                  QList<Common::Hash> hashes;
                  hashes.reserve(IMAliveMessage.chunk_size());
                  for (int i = 0; i < IMAliveMessage.chunk_size(); i++)
                     hashes << IMAliveMessage.chunk(i).hash();

                  const QBitArray& bitArray = this->fileManager->haveChunks(hashes);

                  if (!bitArray.isNull()) // If we own at least one chunk we reply with a CHUNKS_OWNED message.
                  {
                     Protos::Core::ChunksOwned chunkOwnedMessage;
                     chunkOwnedMessage.set_tag(IMAliveMessage.tag());
                     chunkOwnedMessage.mutable_chunk_state()->Reserve(bitArray.size());
                     for (int i = 0; i < bitArray.size(); i++)
                        chunkOwnedMessage.add_chunk_state(bitArray[i]);
                     this->send(Common::MessageHeader::CORE_CHUNKS_OWNED, chunkOwnedMessage, header.getSenderID());
                  }
               }
            }
            break;

         case Common::MessageHeader::CORE_GOODBYE:
            this->peerManager->removePeer(header.getSenderID(), peerAddress);
            break;

         case Common::MessageHeader::CORE_FIND:
            {
               PM::IPeer* peer = this->peerManager->getPeer(header.getSenderID());

               if (peer && peer->isAvailable())
               {
                  const Protos::Core::Find& findMessage = message.getMessage<Protos::Core::Find>();
                  QList<QString> extensions;
                  extensions.reserve(findMessage.pattern().extension_filter_size());
                  for (int i = 0; i < findMessage.pattern().extension_filter_size(); i++)
                     extensions << Common::ProtoHelper::getRepeatedStr(findMessage.pattern(), &Protos::Common::FindPattern::extension_filter, i);

                  QList<Protos::Common::FindResult> results =
                     this->fileManager->find(
                        Common::ProtoHelper::getStr(findMessage.pattern(), &Protos::Common::FindPattern::pattern),
                        extensions,
                        findMessage.pattern().min_size() == 0 ? std::numeric_limits<qint64>::min() : (qint64)findMessage.pattern().min_size(), // According the protocol.
                        findMessage.pattern().max_size() == 0 ? std::numeric_limits<qint64>::max() : (qint64)findMessage.pattern().max_size(), // According the protocol.
                        findMessage.pattern().category(),
                        SETTINGS.get<quint32>("max_number_of_search_result_to_send"),
                        this->MAX_UDP_DATAGRAM_PAYLOAD_SIZE - Common::MessageHeader::HEADER_SIZE
                     );

                  for (QMutableListIterator<Protos::Common::FindResult> i(results); i.hasNext();)
                  {
                     Protos::Common::FindResult& result = i.next();
                     result.set_tag(findMessage.tag());
                     this->send(Common::MessageHeader::CORE_FIND_RESULT, result, header.getSenderID());
                  }
               }
            }
            break;

         default:; // Ignore other messages.
         }

         emit received(message);
      }
      catch (Common::ReadErrorException&)
      {
         L_WARN(QString("Received corrupted multicast message from peer %1 at %2").arg(header.getSenderID().toStr()).arg(peerAddress.toString()));
      }
   }
}

/**
  * Function called when data is recevied by the socket : The corresponding proto is created and the coresponding event is rised.
  */
void UDPListener::processPendingUnicastDatagrams()
{
   static const int MAX_DATAGRAMS_PER_CALL = 500;
   int processed = 0;
   while (this->unicastSocket.hasPendingDatagrams() && processed++ < MAX_DATAGRAMS_PER_CALL)
   {
      QHostAddress peerAddress;
      bool readError = false;
      const Common::MessageHeader& header = this->readDatagramToBuffer(this->unicastSocket, peerAddress, readError);
      if (header.isNull())
      {
         if (readError)
            break;
         continue;
      }

      try
      {
         const Common::Message& message = Common::Message::readMessageBody(header, this->bodyBuffer);

         // DG-LAN: handle IMAlive received via unicast (subnet scan, broadcast, gossip).
         // This is essential for ZeroTier where multicast doesn't work.
         if (header.getType() == Common::MessageHeader::CORE_IM_ALIVE)
         {
            const Protos::Core::IMAlive& IMAliveMessage = message.getMessage<Protos::Core::IMAlive>();

            this->peerManager->updatePeer(
               header.getSenderID(),
               peerAddress,
               IMAliveMessage.port(),
               Common::ProtoHelper::getStr(IMAliveMessage, &Protos::Core::IMAlive::nick),
               IMAliveMessage.amount(),
               Common::ProtoHelper::getStr(IMAliveMessage, &Protos::Core::IMAlive::core_version),
               IMAliveMessage.download_rate(),
               IMAliveMessage.upload_rate(),
               IMAliveMessage.version(),
               IMAliveMessage.lan_speed(),
               IMAliveMessage.is_master()
            );

            // Adopt the master key hash from a peer if we don't have one yet
            // and haven't deliberately cleared it via reset.
            if (IMAliveMessage.has_master_key_hash() && IMAliveMessage.master_key_hash().hash().size() == Common::Hash::HASH_SIZE)
            {
               Common::Hash peerKey(IMAliveMessage.master_key_hash().hash().data());
               Common::Hash localKey = SETTINGS.get<Common::Hash>("master_key_hash");
               if (!peerKey.isNull() && localKey.isNull() && !SETTINGS.get<bool>("master_key_was_reset"))
               {
                  SETTINGS.set("master_key_hash", peerKey);
                  SETTINGS.save();
               }
            }

            if (IMAliveMessage.chunk_size() > 0)
            {
               QList<Common::Hash> hashes;
               hashes.reserve(IMAliveMessage.chunk_size());
               for (int i = 0; i < IMAliveMessage.chunk_size(); i++)
                  hashes << IMAliveMessage.chunk(i).hash();

               const QBitArray& bitArray = this->fileManager->haveChunks(hashes);

               if (!bitArray.isNull())
               {
                  Protos::Core::ChunksOwned chunkOwnedMessage;
                  chunkOwnedMessage.set_tag(IMAliveMessage.tag());
                  chunkOwnedMessage.mutable_chunk_state()->Reserve(bitArray.size());
                  for (int i = 0; i < bitArray.size(); i++)
                     chunkOwnedMessage.add_chunk_state(bitArray[i]);
                  this->send(Common::MessageHeader::CORE_CHUNKS_OWNED, chunkOwnedMessage, header.getSenderID());
               }
            }

            emit received(message);
            continue;
         }

         PM::IPeer* peer = this->peerManager->getPeer(header.getSenderID());
         if (!peer || !peer->isAvailable())
            continue;

         switch (header.getType())
         {
         case Common::MessageHeader::CORE_CHUNKS_OWNED:
            {
               const Protos::Core::ChunksOwned& chunksOwnedMessage = message.getMessage<Protos::Core::ChunksOwned>();

               if (chunksOwnedMessage.tag() != this->currentIMAliveTag)
               {
                  L_WARN(QString("ChunksOwned message tag mismatch: got %1, expected %2 (timing issue, safe to ignore)").arg(chunksOwnedMessage.tag()).arg(currentIMAliveTag));
                  continue;
               }

               if (chunksOwnedMessage.chunk_state_size() != this->currentChunkDownloaders.size())
               {
                  L_WARN(QString("ChunksOwned message size mismatch: got %1 entries, expected %2").arg(chunksOwnedMessage.chunk_state_size()).arg(this->currentChunkDownloaders.size()));
                  continue;
               }

               for (int i = 0; i < chunksOwnedMessage.chunk_state_size(); i++)
                  if (chunksOwnedMessage.chunk_state(i))
                     this->currentChunkDownloaders[i]->addPeer(peer);
                  else
                     this->currentChunkDownloaders[i]->rmPeer(peer);
            }
            break;

         case Common::MessageHeader::CORE_FIND_RESULT:
            {
               Protos::Common::FindResult findResultMessage = message.getMessage<Protos::Common::FindResult>();
               findResultMessage.mutable_peer_id()->set_hash(header.getSenderID().getData(), Common::Hash::HASH_SIZE);
               emit newFindResultMessage(findResultMessage);
            }
            break;

         default:; // Ignore other messages.
         }

         emit received(message);
      }
      catch (Common::ReadErrorException&)
      {
         L_WARN(QString("Received corrupted unicast message from peer %1 at %2").arg(header.getSenderID().toStr()).arg(peerAddress.toString()));
      }
   }
}

void UDPListener::initMulticastUDPSocket()
{
   this->multicastSocket.close();
   this->multicastSocket.disconnect(this);

   this->multicastGroup = Utils::getMulticastGroup();

   // DG-LAN: bind to AnyIPv4 so we receive multicast from ALL interfaces (hybrid LAN+ZeroTier)
   if (!this->multicastSocket.bind(QHostAddress::AnyIPv4, MULTICAST_PORT))
   {
      L_ERRO("Can't bind the multicast socket");
      return;
   }

   // 'loop' is activated only for tests.
#if DEBUG
   const char loop = 1;
#else
   const char loop = 0;
#endif
   this->multicastSocket.setSocketOption(QAbstractSocket::MulticastLoopbackOption, loop);

   // DG-LAN: apply multicast TTL override if configured, otherwise use the standard setting
   const quint32 ttlOverride = SETTINGS.get<quint32>("multicast_ttl_override");
   const quint32 ttl = (ttlOverride > 0) ? ttlOverride : SETTINGS.get<quint32>("multicast_ttl");
   this->multicastSocket.setSocketOption(QAbstractSocket::MulticastTtlOption, ttl);

   // DG-LAN: join multicast group on ALL non-loopback interfaces for hybrid LAN+ZeroTier discovery.
   // A specific interface name can still be set to restrict to one interface.
   const QString specificIface = SETTINGS.get<QString>("network_interface_name");
   int joinedCount = 0;
   foreach (const QNetworkInterface& iface, QNetworkInterface::allInterfaces())
   {
      if (!iface.isValid() ||
          iface.flags().testFlag(QNetworkInterface::IsLoopBack) ||
          !iface.flags().testFlag(QNetworkInterface::IsUp) ||
          !iface.flags().testFlag(QNetworkInterface::IsRunning) ||
          !iface.flags().testFlag(QNetworkInterface::CanMulticast))
         continue;

      // If a specific interface is configured, only join on that one
      if (!specificIface.isEmpty() && iface.name() != specificIface)
         continue;

      // DG-LAN: skip packet-capture loopback adapters (Npcap/WinPcap)
      if (iface.humanReadableName().contains("Npcap", Qt::CaseInsensitive) ||
          iface.humanReadableName().contains("Loopback", Qt::CaseInsensitive))
         continue;

      // Only join if interface has a real (non-APIPA) IPv4 address
      bool hasIPv4 = false;
      foreach (const QNetworkAddressEntry& entry, iface.addressEntries())
      {
         if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
             (entry.ip().toIPv4Address() & 0xFFFF0000) != 0xA9FE0000) // skip 169.254.x.x
         { hasIPv4 = true; break; }
      }
      if (!hasIPv4)
         continue;

      if (this->multicastSocket.joinMulticastGroup(this->multicastGroup, iface))
      {
         L_USER(QString("Network: joined multicast on %1").arg(iface.humanReadableName()));
         ++joinedCount;
      }
      else
         L_WARN(QString("Network: failed to join multicast on %1: %2").arg(iface.humanReadableName()).arg(this->multicastSocket.errorString()));
   }

   if (joinedCount == 0)
      L_ERRO("Unable to join the multicast group on any interface");

   static const int BUFFER_SIZE_UDP = SETTINGS.get<quint32>("udp_buffer_size");
   this->multicastSocket.setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, BUFFER_SIZE_UDP);
   this->multicastSocket.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, BUFFER_SIZE_UDP);

   connect(&this->multicastSocket, &QUdpSocket::readyRead, this, &UDPListener::processPendingMulticastDatagrams);
}

void UDPListener::initUnicastUDPSocket()
{
   this->unicastSocket.close();
   this->unicastSocket.disconnect(this);

   if (!this->unicastSocket.bind(Utils::getCurrentAddressToListenTo(), UNICAST_PORT, QUdpSocket::ReuseAddressHint))
      L_ERRO("Can't bind the unicast socket");

   static const int BUFFER_SIZE_UDP = SETTINGS.get<quint32>("udp_buffer_size");
   this->unicastSocket.setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, BUFFER_SIZE_UDP);
   this->unicastSocket.setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, BUFFER_SIZE_UDP);

   connect(&this->unicastSocket, &QUdpSocket::readyRead, this, &UDPListener::processPendingUnicastDatagrams);
}

/**
  * Writes a given protobuff message to the buffer (this->buffer) prefixed by a header.
  * @return the total size (header size + message size). Return 0 if the total size is bigger than 'Protos.Core.Settings.max_udp_datagram_size'.
  */
int UDPListener::writeMessageToBuffer(Common::MessageHeader::MessageType type, const google::protobuf::Message& message)
{
   const Common::MessageHeader header(type, message.ByteSizeLong(), this->getOwnID());

   const int nbBytesWritten = Common::Message::writeMessageToBuffer(this->buffer, this->MAX_UDP_DATAGRAM_PAYLOAD_SIZE, header, &message);
   if (!nbBytesWritten)
      L_ERRO(QString("Datagram size too big: %1, max allowed: %2").arg(Common::MessageHeader::HEADER_SIZE + header.getSize()).arg(this->MAX_UDP_DATAGRAM_PAYLOAD_SIZE));

   return nbBytesWritten;
}

/**
  * @return A null header if error.
  */
Common::MessageHeader UDPListener::readDatagramToBuffer(QUdpSocket& socket, QHostAddress& peerAddress, bool& readError)
{
   readError = false;
   quint16 port;
   const qint64 datagramSize = socket.readDatagram(this->buffer, BUFFER_SIZE, &peerAddress, &port);
   if (datagramSize == -1)
   {
      L_DEBU(QString("UDP read failed from %1:%2 — %3 (peer may have disconnected)").arg(peerAddress.toString()).arg(port).arg(socket.errorString()));
      readError = true;
      return Common::MessageHeader();
   }

   Common::MessageHeader header = Common::MessageHeader::readHeader(this->buffer);

   if (header.getSize() > datagramSize - Common::MessageHeader::HEADER_SIZE)
   {
      L_ERRO("The message size (header.size) exceeds the datagram size received");
      header.setNull();
      return header;
   }

   if (header.getSenderID() == this->peerManager->getSelf()->getID())
   {
      // L_WARN("We receive a datagram from ourself, skip"); // Don't care . . .
      header.setNull();
      return header;
   }

   if (header.getType() != Common::MessageHeader::CORE_IM_ALIVE)
   {
      PM::IPeer* peer = this->peerManager->getPeer(header.getSenderID());
      if (!peer)
      {
          L_DEBU(QString("Ignored UDP packet from undiscovered peer at %1").arg(peerAddress.toString()));
         header.setNull();
         return header;
      }

      if (!peer->isAlive())
      {
          L_DEBU(QString("Ignored UDP packet from offline peer at %1").arg(peerAddress.toString()));
         header.setNull();
         return header;
      }

      L_DEBU(QString("Receive a datagram UDP from %1, %2").arg(peer->toStringLog()).arg(header.toStr()));
   }
   else
   {
      L_DEBU(QString("Receive a datagram UDP from %1, %2").arg(header.getSenderID().toStr()).arg(header.toStr()));
   }
   return header;
}

Common::Hash UDPListener::getOwnID() const
{
   return this->peerManager->getSelf()->getID();
}
