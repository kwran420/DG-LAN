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

#include <DialogAbout.h>
#include <ui_DialogAbout.h>
using namespace GUI;

#include <QPainter>
#include <QDateTime>
#include <QLocale>
#include <QLinearGradient>

#include <Common/Version.h>
#include <Common/Global.h>
#include <Common/Settings.h>

// ─── Logo from resource ───────────────────────────────────────────────────────
// static
QPixmap DialogAbout::drawLogoPixmap(int w, int h)
{
   QPixmap src(":/icons/logo.png");
   if (src.isNull())
      return QPixmap();
   return src.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// ─────────────────────────────────────────────────────────────────────────────

DialogAbout::DialogAbout(QWidget *parent) :
   QDialog(parent), ui(new Ui::DialogAbout)
{
   this->ui->setupUi(this);
   this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
   this->setFixedSize(560, 620);

   // Load and display logo
   QPixmap logo = drawLogoPixmap(this->width(), 110);
   this->ui->lblLogo->setPixmap(logo);
   this->ui->lblLogo->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

   const QDateTime buildTime = QDateTime::fromString(BUILD_TIME, "yyyy-MM-dd_hh-mm");
   const QLocale locale = SETTINGS.get<QLocale>("language");

   QString titleText = QString("DG-LAN  %1 %2").arg(VERSION).arg(VERSION_TAG);
#ifdef DEBUG
   titleText += " (DEBUG)";
#endif
   this->ui->lblTitle->setText(titleText);

   // Build info
   const QString& compilerName    = Common::Global::getCompilerName();
   const QString& compilerVersion = Common::Global::getCompilerVersion();
   QString compilerStr = compilerName.isEmpty()
      ? QString("Qt %1").arg(QT_VERSION_STR)
      : QString("%1 %2, Qt %3").arg(compilerName, compilerVersion, QT_VERSION_STR);

   const QString link = QStringLiteral("<a style=\"color:#80c8ff;\" href=\"%1\">%2</a>");

   QString body;
   body += QStringLiteral(
      "<p>DG-LAN is a decentralised, zero-config file sharing tool purpose-built "
      "for LAN parties. Drop in, share your game library, grab what everyone else "
      "is hosting &mdash; all at full gigabit switch speeds with no internet required.</p>");

   // ── History ──
   body += QStringLiteral(
      "<p style=\"color:#80c8ff; font-weight:bold;\">History</p>"
      "<p>DG-LAN began life as <b>D-LAN</b>, an open-source project created by "
      "Greg Burri in 2010. D-LAN was designed from the ground up for high-speed "
      "LAN file sharing using a custom protocol, decentralised peer discovery, "
      "and chunk-level transfers &mdash; far beyond what generic tools like "
      "Windows file sharing or FTP could offer in a LAN party environment.</p>"
      "<p>The original D-LAN served the community well for years, but development "
      "went dormant around 2012. In 2026, the team at "
      "<b>Darwin Gamers</b> picked up the torch. We had been running D-LAN at our "
      "LAN events for years and knew both its strengths and its rough edges. "
      "DG-LAN is our continuation of that work &mdash; modernised, extended, and "
      "actively maintained.</p>");

   // ── What's new ──
   body += QStringLiteral(
      "<p style=\"color:#80c8ff; font-weight:bold;\">What DG-LAN adds</p>"
      "<ul style=\"margin-top:0; margin-bottom:0;\">"
      "<li>Master/client network architecture with password-protected master mode</li>"
      "<li>Unified network file index with live download progress</li>"
      "<li>Download queue management with drag-and-drop reordering</li>"
      "<li>Built-in auto-updater (checks GitHub releases)</li>"
      "<li>Configurable multicast TTL and network interface binding</li>"
      "<li>Modern MSYS2/MinGW64 build chain with Protobuf&nbsp;3</li>"
      "</ul>");

   // ── Maintainers ──
   body += QStringLiteral(
      "<p style=\"color:#80c8ff; font-weight:bold;\">Maintained by</p>"
      "<p><b>Matthew Dix</b> &amp; <b>Kieran Hollis</b><br/>");
   body += link.arg("https://darwingamers.org", "darwingamers.org");
   body += QStringLiteral("</p>");

   // ── Links ──
   body += QStringLiteral(
      "<p style=\"color:#80c8ff; font-weight:bold;\">Links</p>"
      "<table cellspacing=\"0\" cellpadding=\"1\">");
   body += QStringLiteral("<tr><td>GitHub:&nbsp;&nbsp;</td><td>")
      + link.arg("https://github.com/kwran420/DG-LAN", "github.com/kwran420/DG-LAN")
      + QStringLiteral("</td></tr>");
   body += QStringLiteral("<tr><td>Releases:&nbsp;&nbsp;</td><td>")
      + link.arg("https://github.com/kwran420/DG-LAN/releases", "Latest releases")
      + QStringLiteral("</td></tr>");
   body += QStringLiteral("<tr><td>Original D-LAN:&nbsp;&nbsp;</td><td>")
      + link.arg("https://github.com/Ummon/D-LAN", "github.com/Ummon/D-LAN")
      + QStringLiteral("</td></tr>");
   body += QStringLiteral("</table>");

   // ── Build details ──
   body += QStringLiteral(
      "<p style=\"color:#80c8ff; font-weight:bold;\">Build</p>"
      "<table cellspacing=\"0\" cellpadding=\"1\">");
   body += QString("<tr><td>Built:&nbsp;&nbsp;</td><td>%1</td></tr>").arg(locale.toString(buildTime));
   body += QString("<tr><td>Compiler:&nbsp;&nbsp;</td><td>%1</td></tr>").arg(compilerStr);
   body += QString("<tr><td>Commit:&nbsp;&nbsp;</td><td>%1</td></tr>")
      .arg(link.arg(
         QString("https://github.com/kwran420/DG-LAN/commit/%1").arg(GIT_VERSION),
         QString(GIT_VERSION)));
   body += QStringLiteral("</table>");

   // ── License ──
   body += QString(
      "<p style=\"color:#aaa; font-size:9pt;\">Original D-LAN &copy; 2010&ndash;%1 Greg Burri. "
      "DG-LAN &copy; 2026 Darwin Gamers. Distributed under the "
      "GNU General Public License v3.</p>")
      .arg(buildTime.date().year());

   this->ui->lblBody->setText(body);
}

DialogAbout::~DialogAbout()
{
   delete this->ui;
}

/**
  * Dark gradient background — deep navy to blue.
  */
void DialogAbout::paintEvent(QPaintEvent* event)
{
   QPainter p(this);
   QLinearGradient gradient(0, 0, 0, height());
   gradient.setColorAt(0, QColor(12, 22, 42));
   gradient.setColorAt(0.6, QColor(18, 38, 72));
   gradient.setColorAt(1, QColor(30, 65, 120));
   p.fillRect(QRect(0, 0, width(), height()), gradient);

   // Subtle horizontal separator below the logo area
   p.setPen(QPen(QColor(60, 110, 200, 80), 1));
   p.drawLine(0, 112, width(), 112);
}

void DialogAbout::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
      this->ui->retranslateUi(this);

   QDialog::changeEvent(event);
}
