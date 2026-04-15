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
  
#include <Tests.h>
using namespace DM;

#include <QSignalSpy>

#define private public
#include <priv/DownloadManager.h>
#include <priv/ChunkDownloader.h>
#undef private

#include <QtDebug>
#include <QStringList>

#include <Protos/core_protocol.pb.h>
#include <Protos/core_settings.pb.h>
#include <Protos/common.pb.h>

#include <Common/LogManager/Builder.h>
#include <Common/Global.h>
#include <Common/TransferRateCalculator.h>
#include <Common/ThreadPool.h>

#include <Builder.h>

Q_DECLARE_METATYPE(PM::IPeer*)

namespace
{
   class DummyPeer : public PM::IPeer
   {
   public:
      DummyPeer(
         const Common::Hash& id = Common::Hash(),
         const QString& nick = QString("dummy"),
         bool available = true
      ) :
         id(id),
         nick(nick),
         available(available)
      {
      }

      QString toStringLog() const { return QString("DummyPeer"); }
      Common::Hash getID() const { return this->id; }
      QHostAddress getIP() const { return QHostAddress(); }
      quint16 getPort() const { return 0; }
      QString getNick() const { return this->nick; }
      QString getCoreVersion() const { return QString(); }
      quint64 getSharingAmount() const { return 0; }
      quint32 getDownloadRate() const { return 0; }
      quint32 getUploadRate() const { return 0; }
      quint32 getLanSpeed() const { return 0; }
      bool isMaster() const { return false; }
      quint32 getHttpPort() const { return 0; }
      quint32 getSpeed() { return 0; }
      void setSpeed(quint32 newSpeed) {}
      void block(int duration, const QString& reason = QString()) {}
      bool isAlive() const { return this->available; }
      bool isAvailable() const { return this->available; }
      quint32 getProtocolVersion() const { return 0; }
      QSharedPointer<PM::IGetEntriesResult> getEntries(const Protos::Core::GetEntries& dirs) { return QSharedPointer<PM::IGetEntriesResult>(); }
      QSharedPointer<PM::IGetHashesResult> getHashes(const Protos::Common::Entry& file) { return QSharedPointer<PM::IGetHashesResult>(); }
      QSharedPointer<PM::IGetChunksResult> getChunks(const Protos::Core::GetChunks& chunks) { return QSharedPointer<PM::IGetChunksResult>(); }

   private:
      Common::Hash id;
      QString nick;
      bool available;
   };
}

/**
  * @class Tests
  *
  */

Tests::Tests()
{
}

void Tests::initTestCase()
{
   qRegisterMetaType<PM::IPeer*>("PM::IPeer*");
   qDebug() << Common::Global::getDataFolder(Common::Global::DataFolderType::LOCAL, false);

   LM::Builder::initMsgHandler();
   qDebug() << "===== initTestCase() =====";

   this->fileManager = QSharedPointer<MockFileManager>(new MockFileManager());
   this->peerManager = QSharedPointer<MockPeerManager>(new MockPeerManager());
   this->downloadManager = Builder::newDownloadManager(this->fileManager, this->peerManager);
}

void Tests::peerUnavailableRemovesOccupiedPeers()
{
   qDebug() << "===== peerUnavailableRemovesOccupiedPeers() =====";

   DummyPeer peer;
   DownloadManager* downloadManager = static_cast<DownloadManager*>(this->downloadManager.data());

   QVERIFY(downloadManager->occupiedPeersAskingForHashes.setPeerAsOccupied(&peer));
   QVERIFY(downloadManager->occupiedPeersAskingForEntries.setPeerAsOccupied(&peer));
   QVERIFY(downloadManager->occupiedPeersDownloadingChunk.setPeerAsOccupied(&peer));

   this->peerManager->emitPeerUnavailable(&peer);

   QCOMPARE(downloadManager->occupiedPeersAskingForHashes.nbOccupiedPeers(), 0);
   QCOMPARE(downloadManager->occupiedPeersAskingForEntries.nbOccupiedPeers(), 0);
   QCOMPARE(downloadManager->occupiedPeersDownloadingChunk.nbOccupiedPeers(), 0);
}

void Tests::occupiedPeerRemovalDoesNotEmitNewFreePeer()
{
   qDebug() << "===== occupiedPeerRemovalDoesNotEmitNewFreePeer() =====";

   DummyPeer peer;
   OccupiedPeers occupiedPeers;
   QSignalSpy spy(&occupiedPeers, SIGNAL(newFreePeer(PM::IPeer*)));

   QVERIFY(spy.isValid());
   QVERIFY(occupiedPeers.setPeerAsOccupied(&peer));

   occupiedPeers.removePeer(&peer);

   QCOMPARE(occupiedPeers.nbOccupiedPeers(), 0);
   QCOMPARE(spy.count(), 0);
}

void Tests::occupiedPeersTrackIdentityByPeerID()
{
   qDebug() << "===== occupiedPeersTrackIdentityByPeerID() =====";

   const Common::Hash peerID = Common::Hash::fromStr("3333333333333333333333333333333333333333");
   DummyPeer peerInstanceA(peerID, "peer-a");
   DummyPeer peerInstanceB(peerID, "peer-b");

   OccupiedPeers occupiedPeers;

   QVERIFY(occupiedPeers.setPeerAsOccupied(&peerInstanceA));
   QVERIFY(!occupiedPeers.isPeerFree(&peerInstanceB));

   occupiedPeers.removePeer(&peerInstanceB);

   QCOMPARE(occupiedPeers.nbOccupiedPeers(), 0);
   QVERIFY(occupiedPeers.isPeerFree(&peerInstanceA));
}

void Tests::chunkDownloaderTracksPeerIdentityByPeerID()
{
   qDebug() << "===== chunkDownloaderTracksPeerIdentityByPeerID() =====";

   const Common::Hash chunkHash = Common::Hash::fromStr("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
   const Common::Hash peerID = Common::Hash::fromStr("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");

   LinkedPeers linkedPeers;
   OccupiedPeers occupiedPeers;
   Common::TransferRateCalculator transferRateCalculator;
   Common::ThreadPool threadPool(1);
   QSharedPointer<ChunkDownloader> chunkDownloader =
      (new ChunkDownloader(linkedPeers, occupiedPeers, transferRateCalculator, threadPool, chunkHash))->grabStrongRef();

   DummyPeer peerInstanceA(peerID, "peer-a");
   DummyPeer peerInstanceB(peerID, "peer-b");

   chunkDownloader->addPeer(&peerInstanceA);
   QCOMPARE(chunkDownloader->getPeers().size(), 1);
   QCOMPARE(linkedPeers.getPeerIDs().size(), 1);

   chunkDownloader->addPeer(&peerInstanceB);
   QCOMPARE(chunkDownloader->getPeers().size(), 1);
   QCOMPARE(chunkDownloader->getPeers().first()->getNick(), QString("peer-b"));
   QCOMPARE(linkedPeers.getPeerIDs().size(), 1);

   chunkDownloader->rmPeer(&peerInstanceA);
   QVERIFY(chunkDownloader->getPeers().isEmpty());
   QVERIFY(linkedPeers.getPeerIDs().isEmpty());
}

void Tests::cleanupTestCase()
{
   qDebug() << "===== cleanupTestCase() =====";
}
