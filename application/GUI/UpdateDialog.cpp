/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#include <UpdateDialog.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QNetworkRequest>
#ifdef Q_OS_WIN
#   include <windows.h>
#else
#   include <QProcess>
#endif

#include <UpdateChecker.h>

// ── UpdateDialog ──────────────────────────────────────────────────────────────

UpdateDialog::UpdateDialog(const QString& latestVersion,
                           const QString& releaseUrl,
                           const QString& downloadUrl,
                           QWidget* parent,
                           bool forced)
   : QDialog(parent)
   , m_dlNam(new QNetworkAccessManager(this))
   , m_downloadUrl(downloadUrl)
   , m_releaseUrl(releaseUrl)
   , m_tempFile(QDir::temp().filePath("DG-LAN-Update-Setup.exe"))
{
   setWindowTitle(forced ? "DG-LAN — Update Required" : "DG-LAN — Update Available");
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
   if (forced)
      setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
   setFixedSize(420, 290);
   setAttribute(Qt::WA_StyledBackground, false);

   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(24, 20, 24, 20);
   root->setSpacing(12);

   // Icon + headline row
   QHBoxLayout* headRow = new QHBoxLayout();
   headRow->setSpacing(12);

   QLabel* icon = new QLabel("\U0001f3ae");
   QFont if_;
   if_.setPointSize(30);
   icon->setFont(if_);

   QVBoxLayout* headText = new QVBoxLayout();

   QLabel* headline = new QLabel(forced ? "Update required to stay on the network!" : "A new version is available!");
   QFont hf;
   hf.setPointSize(14);
   hf.setBold(true);
   headline->setFont(hf);
   headline->setStyleSheet("color: white;");

   QLabel* sub = new QLabel(
      QString("DG-LAN  <b>%1</b>  is ready to install.").arg(latestVersion));
   sub->setStyleSheet("color: #99bbdd;");
   sub->setTextFormat(Qt::RichText);

   headText->addWidget(headline);
   headText->addWidget(sub);

   headRow->addWidget(icon);
   headRow->addLayout(headText, 1);
   root->addLayout(headRow);

   // Divider
   QFrame* line = new QFrame();
   line->setFrameShape(QFrame::HLine);
   line->setStyleSheet("color: #334466;");
   root->addWidget(line);

   // Status label
   m_status = new QLabel(
      downloadUrl.isEmpty()
         ? "Click <b>Open Release Page</b> to download manually from GitHub."
         : "Click <b>Download &amp; Install</b> to update automatically.");
   m_status->setWordWrap(true);
   m_status->setStyleSheet("color: #aabbcc;");
   m_status->setTextFormat(Qt::RichText);
   QFont sf = m_status->font();
   sf.setPointSize(10);
   m_status->setFont(sf);
   root->addWidget(m_status);

   // Progress bar — hidden until download starts
   m_progress = new QProgressBar();
   m_progress->setRange(0, 100);
   m_progress->setValue(0);
   m_progress->setVisible(false);
   m_progress->setStyleSheet(
      "QProgressBar { background: #1a2a3a; border: 1px solid #334466; border-radius: 3px;"
      "               text-align: center; color: white; }"
      "QProgressBar::chunk { background: #1a6bb5; border-radius: 2px; }");
   root->addWidget(m_progress);

   // "Check automatically" checkbox
   QCheckBox* chkAuto = new QCheckBox("Check for updates automatically on launch");
   chkAuto->setChecked(UpdateChecker::isAutoCheckEnabled());
   chkAuto->setStyleSheet("color: #8899aa;");
   connect(chkAuto, &QCheckBox::toggled, [](bool checked) {
      UpdateChecker::setAutoCheckEnabled(checked);
   });
   root->addWidget(chkAuto);

   root->addStretch();

   // Button row
   QHBoxLayout* btnRow = new QHBoxLayout();
   btnRow->setSpacing(8);
   btnRow->addStretch();

   m_btnLater = new QPushButton(forced ? "Quit" : "Later");
   m_btnLater->setFixedWidth(80);
   m_btnLater->setStyleSheet(
      "QPushButton { background: #2a3a4a; color: #aabbcc; border: 1px solid #445566; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background: #344455; }");
   if (forced)
      connect(m_btnLater, &QPushButton::clicked, []() { QCoreApplication::quit(); });
   else
      connect(m_btnLater, &QPushButton::clicked, this, &QDialog::reject);

   const bool hasDirect = !downloadUrl.isEmpty();
   m_btnAction = new QPushButton(hasDirect ? "Download && Install" : "Open Release Page");
   m_btnAction->setFixedWidth(160);
   m_btnAction->setStyleSheet(
      "QPushButton { background: #1a6bb5; color: white; border: none; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background: #2280d0; }"
      "QPushButton:disabled { background: #2a3a4a; color: #667788; }");

   if (hasDirect)
      connect(m_btnAction, &QPushButton::clicked, this, &UpdateDialog::startDownload);
   else
      connect(m_btnAction, &QPushButton::clicked, [releaseUrl, this]() {
         QDesktopServices::openUrl(QUrl(releaseUrl));
         accept();
      });

   btnRow->addWidget(m_btnLater);
   btnRow->addWidget(m_btnAction);
   root->addLayout(btnRow);
}

void UpdateDialog::startDownload()
{
   m_btnAction->setEnabled(false);
   m_btnAction->setText("Downloading...");
   m_btnLater->setEnabled(false);
   m_status->setText("Connecting to GitHub...");
   m_progress->setRange(0, 0);  // indeterminate until we know the total
   m_progress->setVisible(true);

   QUrl reqUrl(m_downloadUrl);
   QNetworkRequest req(reqUrl);
   req.setRawHeader("User-Agent", "DG-LAN-UpdateChecker/1.0");
   req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                    QNetworkRequest::NoLessSafeRedirectPolicy);

   m_reply = m_dlNam->get(req);
   connect(m_reply, &QNetworkReply::downloadProgress,
           this, &UpdateDialog::onProgress);
   connect(m_reply, &QNetworkReply::finished,
           this, &UpdateDialog::onDownloadDone);
}

void UpdateDialog::onProgress(qint64 received, qint64 total)
{
   if (total > 0)
   {
      m_progress->setRange(0, 100);
      m_progress->setValue(static_cast<int>(received * 100 / total));
      m_status->setText(QString("Downloading... %1 / %2 MB")
         .arg(received / 1'000'000.0, 0, 'f', 1)
         .arg(total    / 1'000'000.0, 0, 'f', 1));
   }
}

void UpdateDialog::onDownloadDone()
{
   m_reply->deleteLater();

   if (m_reply->error() != QNetworkReply::NoError)
   {
      m_status->setText("Download failed \u2014 opening release page in browser...");
      m_progress->setVisible(false);
      m_btnLater->setEnabled(true);
      m_btnAction->setText("Open Release Page");
      m_btnAction->setEnabled(true);
      disconnect(m_btnAction, nullptr, this, nullptr);
      connect(m_btnAction, &QPushButton::clicked, [this]() {
         QDesktopServices::openUrl(QUrl(m_releaseUrl));
         accept();
      });
      return;
   }

   // Save installer to temp directory
   QFile f(m_tempFile);
   if (!f.open(QIODevice::WriteOnly))
   {
      m_status->setText("Could not write to temp directory \u2014 opening release page...");
      QDesktopServices::openUrl(QUrl(m_releaseUrl));
      accept();
      return;
   }
   f.write(m_reply->readAll());
   f.close();

   m_progress->setRange(0, 100);
   m_progress->setValue(100);
   m_status->setText("Download complete. Launching installer...");
   QCoreApplication::processEvents();

   // Launch installer \u2014 ShellExecute handles UAC elevation from installer manifest
#ifdef Q_OS_WIN
   const QString native = QDir::toNativeSeparators(m_tempFile);
   ShellExecuteW(
      nullptr, L"open",
      reinterpret_cast<LPCWSTR>(native.utf16()),
      L"/VERYSILENT /NORESTART",
      nullptr, SW_SHOWNORMAL);
#else
   QProcess::startDetached(m_tempFile, {"/VERYSILENT", "/NORESTART"});
#endif

   QCoreApplication::quit();
}

void UpdateDialog::paintEvent(QPaintEvent*)
{
   QPainter p(this);
   QLinearGradient g(0, 0, 0, height());
   g.setColorAt(0, QColor(12, 22, 42));
   g.setColorAt(1, QColor(25, 50, 90));
   p.fillRect(rect(), g);
}

// ── UpToDateDialog ────────────────────────────────────────────────────────────

UpToDateDialog::UpToDateDialog(const QString& currentVersion, QWidget* parent)
   : QDialog(parent)
{
   setWindowTitle("DG-LAN — Up to Date");
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
   setFixedSize(340, 160);

   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(24, 20, 24, 20);
   root->setSpacing(10);

   QLabel* icon = new QLabel("✔  You're up to date!");
   QFont f;
   f.setPointSize(13);
   f.setBold(true);
   icon->setFont(f);
   icon->setStyleSheet("color: #44cc88;");
   root->addWidget(icon);

   QLabel* ver = new QLabel(QString("DG-LAN %1 is the latest version.").arg(currentVersion));
   ver->setStyleSheet("color: #aabbcc;");
   root->addWidget(ver);

   root->addStretch();

   QPushButton* btnOk = new QPushButton("OK");
   btnOk->setFixedWidth(80);
   btnOk->setStyleSheet(
      "QPushButton { background: #1a6bb5; color: white; border: none; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background: #2280d0; }");
   connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

   QHBoxLayout* btnRow = new QHBoxLayout();
   btnRow->addStretch();
   btnRow->addWidget(btnOk);
   root->addLayout(btnRow);
}
