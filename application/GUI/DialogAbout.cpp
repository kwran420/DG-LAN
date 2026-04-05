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
   this->setFixedSize(500, 400);

   // Load and display logo
   QPixmap logo = drawLogoPixmap(this->width(), 110);
   this->ui->lblLogo->setPixmap(logo);
   this->ui->lblLogo->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

   const QDateTime buildTime = QDateTime::fromString(BUILD_TIME, "yyyy-MM-dd_hh-mm");
   const QLocale locale = SETTINGS.get<QLocale>("language");

   this->ui->lblTitle->setText(
      QString("DG-LAN  %1 %2").arg(VERSION).arg(VERSION_TAG));

   this->ui->lblBuiltOn->setText(
      QString("Built on %1").arg(locale.toString(buildTime)));

   this->ui->lblFromRevision->setText(
      QString("<html><head/><body><p>Based on D-LAN — revision "
              "<a href=\"https://github.com/Ummon/D-LAN/commit/%1\">"
              "<span style=\"color: #80c8ff;\">%1</span></a></p></body></html>")
      .arg(GIT_VERSION));

   this->ui->lblCopyright->setText(
      this->ui->lblCopyright->text().arg(buildTime.date().year()));

   const QString& compilerName    = Common::Global::getCompilerName();
   const QString& compilerVersion = Common::Global::getCompilerVersion();
   if (compilerName.isEmpty())
      this->ui->lblCompiler->setText(QString("Built with Qt %1").arg(QT_VERSION_STR));
   else
      this->ui->lblCompiler->setText(
         QString("Built with %1 %2 — Qt %3")
         .arg(compilerName).arg(compilerVersion).arg(QT_VERSION_STR));

#ifdef DEBUG
   this->ui->lblTitle->setText(this->ui->lblTitle->text() + " (DEBUG)");
#endif
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
