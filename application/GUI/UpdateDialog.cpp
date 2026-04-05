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

#include <UpdateChecker.h>

// ── UpdateDialog ──────────────────────────────────────────────────────────────

UpdateDialog::UpdateDialog(const QString& latestVersion,
                           const QString& releaseUrl,
                           QWidget* parent)
   : QDialog(parent)
{
   setWindowTitle("DG-LAN — Update Available");
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
   setFixedSize(420, 260);
   setAttribute(Qt::WA_StyledBackground, false);

   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(24, 20, 24, 20);
   root->setSpacing(12);

   // Icon + headline row
   QHBoxLayout* headRow = new QHBoxLayout();
   headRow->setSpacing(12);

   QLabel* icon = new QLabel("🎮");
   QFont if_;
   if_.setPointSize(30);
   icon->setFont(if_);

   QVBoxLayout* headText = new QVBoxLayout();

   QLabel* headline = new QLabel("A new version is available!");
   QFont hf;
   hf.setPointSize(14);
   hf.setBold(true);
   headline->setFont(hf);
   headline->setStyleSheet("color: white;");

   QLabel* sub = new QLabel(
      QString("DG-LAN  <b>%1</b>  is ready to download.").arg(latestVersion));
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

   // Info text
   QLabel* info = new QLabel(
      "Click <b>Download</b> to open the GitHub releases page in your browser.\n"
      "Download the installer (.exe) and run it to update.");
   info->setWordWrap(true);
   info->setStyleSheet("color: #aabbcc;");
   info->setTextFormat(Qt::RichText);
   QFont inf = info->font();
   inf.setPointSize(10);
   info->setFont(inf);
   root->addWidget(info);

   // "Don't check automatically" checkbox
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

   QPushButton* btnLater = new QPushButton("Later");
   btnLater->setFixedWidth(80);
   btnLater->setStyleSheet(
      "QPushButton { background: #2a3a4a; color: #aabbcc; border: 1px solid #445566; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background: #344455; }");
   connect(btnLater, &QPushButton::clicked, this, &QDialog::reject);

   QPushButton* btnDownload = new QPushButton("Download");
   btnDownload->setFixedWidth(100);
   btnDownload->setStyleSheet(
      "QPushButton { background: #1a6bb5; color: white; border: none; border-radius: 4px; padding: 6px 12px; }"
      "QPushButton:hover { background: #2280d0; }");
   connect(btnDownload, &QPushButton::clicked, [releaseUrl, this]() {
      QDesktopServices::openUrl(QUrl(releaseUrl));
      accept();
   });

   btnRow->addWidget(btnLater);
   btnRow->addWidget(btnDownload);
   root->addLayout(btnRow);
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
