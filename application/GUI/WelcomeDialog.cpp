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

// ── Helper — small ethernet plug icon for step bullets ──────────────────────
static QPixmap makePlugIcon(int sz)
{
   QPixmap px(sz, sz);
   px.fill(Qt::transparent);
   QPainter p(&px);
   p.setRenderHint(QPainter::Antialiasing);

   const float yc = sz * 0.5f;
   // body
   QLinearGradient bg(0, yc - sz*0.28f, 0, yc + sz*0.28f);
   bg.setColorAt(0, QColor(70, 145, 230));
   bg.setColorAt(1, QColor(22, 68, 155));
   p.setPen(Qt::NoPen);
   p.setBrush(bg);
   p.drawRoundedRect(QRectF(sz*0.15f, yc - sz*0.28f, sz*0.60f, sz*0.56f), 3, 3);
   // face
   p.setBrush(QColor(16, 26, 50));
   p.drawRoundedRect(QRectF(sz*0.68f, yc - sz*0.28f, sz*0.17f, sz*0.56f), 2, 2);
   // pins
   p.setPen(QPen(QColor(255, 215, 50), 1.1f));
   for (int i = 0; i < 5; ++i)
      p.drawLine(QPointF(sz*0.69f + i*sz*0.025f, yc - sz*0.22f),
                 QPointF(sz*0.69f + i*sz*0.025f, yc + sz*0.22f));
   return px;
}

// ── Step row widget ──────────────────────────────────────────────────────────
static QWidget* makeStep(const QString& emoji, const QString& bold, const QString& detail)
{
   QWidget* row = new QWidget();
   QHBoxLayout* h = new QHBoxLayout(row);
   h->setContentsMargins(0, 4, 0, 4);
   h->setSpacing(12);

   QLabel* ico = new QLabel(emoji);
   ico->setFixedWidth(32);
   QFont eF = ico->font();
   eF.setPointSize(20);
   ico->setFont(eF);
   ico->setAlignment(Qt::AlignCenter);
   h->addWidget(ico);

   QLabel* txt = new QLabel(QString("<b>%1</b>  %2").arg(bold).arg(detail));
   txt->setWordWrap(true);
   txt->setStyleSheet("color: #f0f0f0;");
   QFont tF = txt->font();
   tF.setPointSize(10);
   txt->setFont(tF);
   h->addWidget(txt, 1);

   return row;
}

// ── WelcomeDialog ────────────────────────────────────────────────────────────

WelcomeDialog::WelcomeDialog(QWidget* parent)
   : QDialog(parent)
{
   setWindowTitle("Welcome to DG-LAN");
   setFixedSize(480, 500);
   setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

   // Root layout
   QVBoxLayout* root = new QVBoxLayout(this);
   root->setContentsMargins(0, 0, 0, 16);
   root->setSpacing(0);

   // ── Header banner ───────────────────────────────────────────────────────
   QWidget* banner = new QWidget();
   banner->setFixedHeight(120);
   banner->setObjectName("welcomeBanner");
   {
      QVBoxLayout* bv = new QVBoxLayout(banner);
      bv->setContentsMargins(20, 10, 20, 10);
      bv->setSpacing(4);

      // Logo image
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
   blay->setSpacing(4);

   QLabel* intro = new QLabel(
      "DG-LAN shares files over your LAN — no internet needed.\n"
      "Perfect for game nights. Here's all you need to know:");
   intro->setWordWrap(true);
   intro->setStyleSheet("color: #cccccc;");
   QFont iF = intro->font();
   iF.setPointSize(10);
   intro->setFont(iF);
   blay->addWidget(intro);

   // Divider
   QFrame* line = new QFrame();
   line->setFrameShape(QFrame::HLine);
   line->setStyleSheet("color: #334455;");
   blay->addWidget(line);
   blay->addSpacing(4);

   // Steps
   blay->addWidget(makeStep("⚙️",  "Settings",  "Add the folders you want to share."));
   blay->addWidget(makeStep("👥",  "Peers",     "Everyone on the LAN shows up automatically."));
   blay->addWidget(makeStep("🔍",  "Browse",    "Double-click a peer to see their files."));
   blay->addWidget(makeStep("⬇️",  "Download",  "Right-click any file → Download it."));
   blay->addWidget(makeStep("💬",  "Chat",      "Talk to everyone on the Chat tab."));
   blay->addWidget(makeStep("📊",  "Indexing",  "See hashing progress on the Indexing tab."));

   blay->addStretch();

   // Pro tip
   QLabel* tip = new QLabel(
      "💡  Big libraries take a few minutes to hash on first run.\n"
      "   Once done, it's instant. Just leave it running.");
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

   QPushButton* btnGo = new QPushButton("  Let's go!  ");
   btnGo->setDefault(true);
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
      "}");

   foot->addWidget(credits);
   foot->addStretch();
   foot->addWidget(btnGo);
   root->addLayout(foot);

   connect(btnGo, &QPushButton::clicked, this, &WelcomeDialog::accept);
}

void WelcomeDialog::paintEvent(QPaintEvent*)
{
   QPainter p(this);

   // Full dialog background
   QLinearGradient bg(0, 0, 0, height());
   bg.setColorAt(0, QColor(14, 24, 44));
   bg.setColorAt(1, QColor(22, 38, 68));
   p.fillRect(rect(), bg);

   // Header banner gradient (top 120px)
   QLinearGradient hdr(0, 0, width(), 120);
   hdr.setColorAt(0,   QColor(20, 50, 130));
   hdr.setColorAt(0.5, QColor(30, 90, 200));
   hdr.setColorAt(1,   QColor(20, 50, 130));
   p.fillRect(QRect(0, 0, width(), 120), hdr);

   // Subtle separator line under header
   p.setPen(QPen(QColor(80, 140, 255, 60), 1));
   p.drawLine(0, 120, width(), 120);

   // RJ-45 watermark plug in bottom-right corner (decorative)
   p.setOpacity(0.05);
   QFont bigF;
   bigF.setPointSize(160);
   p.setFont(bigF);
   p.setPen(QColor(100, 180, 255));
   p.drawText(QRectF(width() - 210, height() - 220, 210, 220),
              Qt::AlignCenter, "⏻");
   p.setOpacity(1.0);
}
