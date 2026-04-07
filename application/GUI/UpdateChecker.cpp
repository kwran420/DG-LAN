/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#include <UpdateChecker.h>
using namespace GUI;

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>
#include <QVersionNumber>

#include <Common/Version.h>

// ── Configuration ─────────────────────────────────────────────────────────────
static const QString GITHUB_OWNER = "kwran420";
static const QString GITHUB_REPO  = "DG-LAN";

// The current version as a clean semver string (matches release tag format).
// Tags on GitHub should be like "v1.2.0" to match this.
static const QString CURRENT_VERSION = QString("v") + VERSION;

// GitHub releases API endpoint — uses the list endpoint so pre-releases
// are included (the /releases/latest endpoint skips them).
static QString apiUrl()
{
   return QString("https://api.github.com/repos/%1/%2/releases?per_page=10")
      .arg(GITHUB_OWNER, GITHUB_REPO);
}

// ── Settings helpers ──────────────────────────────────────────────────────────
bool UpdateChecker::isAutoCheckEnabled()
{
   QSettings s("DGLan", "DG-LAN");
   return s.value("update/check_on_launch", true).toBool();
}

void UpdateChecker::setAutoCheckEnabled(bool enabled)
{
   QSettings s("DGLan", "DG-LAN");
   s.setValue("update/check_on_launch", enabled);
}

// ── UpdateChecker ─────────────────────────────────────────────────────────────
UpdateChecker::UpdateChecker(QObject* parent)
   : QObject(parent)
   , nam(new QNetworkAccessManager(this))
{
   connect(nam, &QNetworkAccessManager::finished,
           this, &UpdateChecker::onReplyFinished);
}

void UpdateChecker::check()
{
   QUrl url(apiUrl());
   QNetworkRequest req(url);
   // GitHub API requires a User-Agent header.
   req.setRawHeader("User-Agent", "DG-LAN-UpdateChecker/1.0");
   // Accept JSON.
   req.setRawHeader("Accept", "application/vnd.github+json");
   req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                    QNetworkRequest::NoLessSafeRedirectPolicy);
   nam->get(req);
}

void UpdateChecker::onReplyFinished(QNetworkReply* reply)
{
   reply->deleteLater();

   if (reply->error() != QNetworkReply::NoError)
   {
      emit checkFailed(reply->errorString());
      return;
   }

   const QByteArray data = reply->readAll();
   QJsonParseError parseErr;
   const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);

   // The endpoint returns an array of releases.
   if (doc.isNull() || !doc.isArray())
   {
      emit checkFailed(QString("Failed to parse GitHub response: %1").arg(parseErr.errorString()));
      return;
   }

   auto strip = [](const QString& v) -> QString {
      return v.startsWith('v', Qt::CaseInsensitive) ? v.mid(1) : v;
   };

   const QVersionNumber current = QVersionNumber::fromString(strip(CURRENT_VERSION));

   // Walk the releases and pick the highest version (skipping drafts).
   QVersionNumber bestVersion;
   QString bestTag, bestUrl, bestDownloadUrl;

   const QJsonArray releases = doc.array();
   for (const auto& r : releases)
   {
      const QJsonObject obj = r.toObject();
      if (obj.value("draft").toBool())
         continue;

      const QString tag = obj.value("tag_name").toString().trimmed();
      if (tag.isEmpty())
         continue;

      const QVersionNumber ver = QVersionNumber::fromString(strip(tag));
      if (ver <= bestVersion)
         continue;

      // Find the .exe installer asset in this release.
      QString downloadUrl;
      const QJsonArray assets = obj.value("assets").toArray();
      for (const auto& a : assets)
      {
         const QJsonObject asset = a.toObject();
         const QString name = asset.value("name").toString();
         if (name.endsWith(".exe", Qt::CaseInsensitive))
         {
            downloadUrl = asset.value("browser_download_url").toString();
            break;
         }
      }

      bestVersion     = ver;
      bestTag         = tag;
      bestUrl         = obj.value("html_url").toString().trimmed();
      bestDownloadUrl = downloadUrl;
   }

   if (bestTag.isEmpty())
   {
      emit checkFailed("No release tag found in GitHub response.");
      return;
   }

   if (bestVersion > current)
      emit updateAvailable(bestTag, bestUrl, bestDownloadUrl);
   else
      emit upToDate(CURRENT_VERSION);
}
