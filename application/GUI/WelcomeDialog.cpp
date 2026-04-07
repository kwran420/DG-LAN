/**
  * DG-LAN - A decentralized LAN file sharing tool for gamers.
  * DG-LAN contributors: Kieran Hollis & Matthew Dix (2026)
  * Based on D-LAN by Greg Burri — GPLv3
  */

#include <WelcomeDialog.h>
using namespace GUI;

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPainter>
#include <QLinearGradient>
#include <QFont>
#include <QSettings>
#include <QPixmap>
#include <QFileDialog>
#include <QDir>

// ── Persistent flag ──────────────────────────────────────────────────────────

bool WelcomeDialog::shouldShow()
{
   QSettings s("DGLan", "DG-LAN");
   return !s.value("welcome_shown", false).toBool();
}

void WelcomeDialog::markShown()
{
   QSettings s("DGLan", "DG-LAN");
   s.setValue("welcome_shown", true);
}

QString WelcomeDialog::selectedDirectory() const
{
   return this->chosenDir;
}

// ── WelcomeDialog ────────────────────────────────────────────────────────────

WelcomeDialog::WelcomeDialog(QWidget* parent)
   : QDialog(parent)
{
   setWindowTitle("Welcome to DG-LAN");
   setFixedSize(520, 580);
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(0, 0, 0, 16);
   root->setSpacing(0);

   // ── Header banner ───────────────────────────────────────────────────────
   QWidget* banner = new QWidget();
   banner->setFixedHeight(120);
   {
      QVBoxLayout* bv = new QVBoxLayout(banner);
      bv->setContentsMargins(20, 10, 20, 10);
      bv->setSpacing(4);

      QLabel* logoImg = new QLabel();
      QPixmap src(":/icons/logo.png");
      if (!src.isNull())
         logoImg->setPixmap(src.scaled(300, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      logoImg->setAlignment(Qt::AlignHCenter);

      QLabel* tag = new QLabel("Plug in, game on!");
      QFont tg;
      tg.setFamily("Arial");
      tg.setPointSize(13);
      tg.setItalic(true);
      tag->setFont(tg);
      tag->setStyleSheet("color: #80c8ff;");
      tag->setAlignment(Qt::AlignHCenter);

      bv->addWidget(logoImg);
      bv->addWidget(tag);
   }
   root->addWidget(banner);

   // ── Body ────────────────────────────────────────────────────────────────
   QWidget* body = new QWidget();
   QVBoxLayout* blay = new QVBoxLayout(body);
   blay->setContentsMargins(24, 16, 24, 16);
   blay->setSpacing(8);

   QLabel* intro = new QLabel(
      "DG-LAN shares files over your LAN — no internet needed.\n"
      "Let's set up your shared folder to get started.");
   intro->setWordWrap(true);
   intro->setStyleSheet("color: #cccccc;");
   QFont iF = intro->font();
   iF.setPointSize(10);
   intro->setFont(iF);
   blay->addWidget(intro);

   // Divider
   QFrame* line1 = new QFrame();
   line1->setFrameShape(QFrame::HLine);
   line1->setStyleSheet("color: #334455;");
   blay->addWidget(line1);

   // ── Shared folder setup section ─────────────────────────────────────────
   QLabel* setupTitle = new QLabel("Step 1: Choose your shared folder");
   QFont stF = setupTitle->font();
   stF.setPointSize(12);
   stF.setBold(true);
   setupTitle->setFont(stF);
   setupTitle->setStyleSheet("color: #ffffff;");
   blay->addWidget(setupTitle);

   QLabel* setupDesc = new QLabel(
      "Pick a folder to share with other players on your LAN.\n"
      "Create a dedicated folder (e.g. \"LAN Share\") for best results.");
   setupDesc->setWordWrap(true);
   setupDesc->setStyleSheet("color: #bbbbbb;");
   QFont sdF = setupDesc->font();
   sdF.setPointSize(9);
   setupDesc->setFont(sdF);
   blay->addWidget(setupDesc);

   // ── BIG RED WARNING ─────────────────────────────────────────────────────
   QLabel* warning = new QLabel(
      "⚠  WARNING: Everything in your shared folder will be visible\n"
      "to ALL peers on the network!\n\n"
      "DO NOT share folders containing personal files such as\n"
      "Downloads, Documents, Desktop, or your entire user profile.\n"
      "Only share folders you have specifically set up for LAN sharing.");
   warning->setWordWrap(true);
   QFont wF = warning->font();
   wF.setPointSize(10);
   wF.setBold(true);
   warning->setFont(wF);
   warning->setStyleSheet(
      "color: #ff3333;"
      "background: rgba(180, 0, 0, 50);"
      "border: 2px solid #cc0000;"
      "border-radius: 6px;"
      "padding: 12px;");
   blay->addWidget(warning);

   // ── Folder picker row ───────────────────────────────────────────────────
   QHBoxLayout* pickerRow = new QHBoxLayout();
   pickerRow->setSpacing(8);

   QLabel* pathLabel = new QLabel("No folder selected");
   pathLabel->setStyleSheet(
      "color: #999999;"
      "background: rgba(0,0,0,60);"
      "border: 1px solid #444466;"
      "border-radius: 4px;"
      "padding: 6px 10px;");
   QFont plF = pathLabel->font();
   plF.setPointSize(9);
   pathLabel->setFont(plF);
   pathLabel->setMinimumHeight(30);

   QPushButton* btnBrowse = new QPushButton("  Choose Folder...  ");
   btnBrowse->setStyleSheet(
      "QPushButton {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #3a8fe4, stop:1 #2060b0);"
      "  color: white;"
      "  border: 1px solid #1050b0;"
      "  border-radius: 5px;"
      "  padding: 6px 16px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #4a9ff4, stop:1 #3070c0);"
      "}");

   pickerRow->addWidget(pathLabel, 1);
   pickerRow->addWidget(btnBrowse);
   blay->addLayout(pickerRow);

   blay->addStretch();

   // ── Quick tips ──────────────────────────────────────────────────────────
   QLabel* tip = new QLabel(
      "💡  After setup: peers appear automatically, browse their files,\n"
      "   right-click to download. Big libraries take a few minutes to hash\n"
      "   on first run — once done, it's instant.");
   tip->setWordWrap(true);
   tip->setStyleSheet(
      "color: #ffe480;"
      "background: rgba(80,60,0,80);"
      "border: 1px solid rgba(200,160,0,60);"
      "border-radius: 6px;"
      "padding: 8px;");
   QFont tipF = tip->font();
   tipF.setPointSize(9);
   tip->setFont(tipF);
   blay->addWidget(tip);

   root->addWidget(body, 1);

   // ── Footer ──────────────────────────────────────────────────────────────
   QHBoxLayout* foot = new QHBoxLayout();
   foot->setContentsMargins(24, 0, 24, 0);

   QLabel* credits = new QLabel("Made with ♥ by Kieran Hollis & Matthew Dix — 2026");
   credits->setStyleSheet("color: #556677;");
   QFont cF = credits->font();
   cF.setPointSize(8);
   credits->setFont(cF);

   QPushButton* btnSkip = new QPushButton("  Skip  ");
   btnSkip->setStyleSheet(
      "QPushButton {"
      "  background: transparent;"
      "  color: #7788aa;"
      "  border: 1px solid #445566;"
      "  border-radius: 5px;"
      "  padding: 6px 16px;"
      "}"
      "QPushButton:hover {"
      "  color: #aabbcc;"
      "  border-color: #667788;"
      "}");

   QPushButton* btnGo = new QPushButton("  Let's go!  ");
   btnGo->setDefault(true);
   btnGo->setEnabled(false);
   btnGo->setStyleSheet(
      "QPushButton {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #2a7fd4, stop:1 #1a50a0);"
      "  color: white;"
      "  border: 1px solid #1040a0;"
      "  border-radius: 5px;"
      "  padding: 6px 20px;"
      "  font-weight: bold;"
      "}"
      "QPushButton:hover {"
      "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
      "    stop:0 #4090e4, stop:1 #2060b8);"
      "}"
      "QPushButton:disabled {"
      "  background: #334455;"
      "  color: #667788;"
      "  border-color: #445566;"
      "}");

   foot->addWidget(credits);
   foot->addStretch();
   foot->addWidget(btnSkip);
   foot->addWidget(btnGo);
   root->addLayout(foot);

   // ── Connections ─────────────────────────────────────────────────────────
   connect(btnBrowse, &QPushButton::clicked, this, [this, pathLabel, btnGo]() {
      QString dir = QFileDialog::getExistingDirectory(this, "Choose shared folder",
         QDir::homePath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
      if (!dir.isEmpty())
      {
         this->chosenDir = dir;
         pathLabel->setText(dir);
         pathLabel->setStyleSheet(
            "color: #66ff66;"
            "background: rgba(0,80,0,60);"
            "border: 1px solid #228822;"
            "border-radius: 4px;"
            "padding: 6px 10px;");
         btnGo->setEnabled(true);
      }
   });

   connect(btnSkip, &QPushButton::clicked, this, &WelcomeDialog::accept);
   connect(btnGo, &QPushButton::clicked, this, &WelcomeDialog::accept);
}

void WelcomeDialog::paintEvent(QPaintEvent*)
{
   QPainter p(this);

   QLinearGradient bg(0, 0, 0, height());
   bg.setColorAt(0, QColor(14, 24, 44));
   bg.setColorAt(1, QColor(22, 38, 68));
   p.fillRect(rect(), bg);

   QLinearGradient hdr(0, 0, width(), 120);
   hdr.setColorAt(0,   QColor(20, 50, 130));
   hdr.setColorAt(0.5, QColor(30, 90, 200));
   hdr.setColorAt(1,   QColor(20, 50, 130));
   p.fillRect(QRect(0, 0, width(), 120), hdr);

   p.setPen(QPen(QColor(80, 140, 255, 60), 1));
   p.drawLine(0, 120, width(), 120);

   p.setOpacity(0.05);
   QFont bigF;
   bigF.setPointSize(160);
   p.setFont(bigF);
   p.setPen(QColor(100, 180, 255));
   p.drawText(QRectF(width() - 210, height() - 220, 210, 220),
              Qt::AlignCenter, "⏻");
   p.setOpacity(1.0);
}
