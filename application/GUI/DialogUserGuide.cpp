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

#include <DialogUserGuide.h>
#include <ui_DialogUserGuide.h>
using namespace GUI;

#include <QPainter>
#include <QLinearGradient>

DialogUserGuide::DialogUserGuide(QWidget* parent) :
   QDialog(parent), ui(new Ui::DialogUserGuide)
{
   this->ui->setupUi(this);
   this->setWindowFlags(this->windowFlags() & (~Qt::WindowContextHelpButtonHint));
   this->setFixedSize(640, 720);

   const QString heading = QStringLiteral(
      "<span style=\"color:#80c8ff; font-weight:bold; font-size:11pt;\">%1</span>");
   const QString subheading = QStringLiteral(
      "<span style=\"color:#80c8ff; font-weight:bold;\">%1</span>");

   QString html;

   // ── Getting Started ──────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Getting Started") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>DG-LAN is a decentralised LAN file sharing tool built for LAN parties. "
      "There is no central server &mdash; every machine on the network discovers "
      "peers automatically via multicast and shares files at full switch speed.</p>"

      "<p><b>First launch:</b> On starting DG-LAN for the first time you will be "
      "prompted to set a <b>nickname</b> and add at least one <b>shared folder</b>. "
      "Your shared folders are the directories whose contents will be visible to "
      "everyone else on the network.</p>");

   // ── Network Panel ────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Network Panel") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>The main window is the <b>Network</b> panel. It has two areas:</p>"
      "<ul>"
      "<li><b>Peer list</b> (left) &mdash; shows every DG-LAN user on the LAN, "
      "along with their sharing amount.</li>"
      "<li><b>File index</b> (centre) &mdash; shows the combined file tree of "
      "all peers (or a single peer if you select one from the list).</li>"
      "</ul>");

   html += QStringLiteral("<p>") + subheading.arg("File columns") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<table cellspacing=\"0\" cellpadding=\"2\">"
      "<tr><td><b>Name</b>&nbsp;&nbsp;</td><td>File or folder name</td></tr>"
      "<tr><td><b>Size</b>&nbsp;&nbsp;</td><td>Total size of the file</td></tr>"
      "<tr><td><b>Status</b>&nbsp;&nbsp;</td><td>Current state (Queued, Downloading, Complete, etc.)</td></tr>"
      "<tr><td><b>#</b>&nbsp;&nbsp;</td><td>Queue position number</td></tr>"
      "<tr><td><b>Progress</b>&nbsp;&nbsp;</td><td>Visual progress bar (0&ndash;100%)</td></tr>"
      "<tr><td><b>DL Speed</b>&nbsp;&nbsp;</td><td>Download speed for this file</td></tr>"
      "<tr><td><b>UL Speed</b>&nbsp;&nbsp;</td><td>Upload speed (how fast you are sending this file to others)</td></tr>"
      "<tr><td><b>Peers</b>&nbsp;&nbsp;</td><td>How many peers have this file available</td></tr>"
      "</table>");

   // ── Downloading Files ────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Downloading Files") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>To download files or folders:</p>"
      "<ol>"
      "<li>Browse or search for the content you want in the file index.</li>"
      "<li>Select one or more items.</li>"
      "<li>Click the <b>Download</b> toolbar button, or right-click and choose "
      "<b>Download selected items</b> from the context menu.</li>"
      "</ol>"
      "<p>Additional context-menu options:</p>"
      "<ul>"
      "<li><b>Download selected items to...</b> &mdash; choose a specific destination folder.</li>"
      "<li><b>Redownload</b> &mdash; re-queue a previously completed or failed file.</li>"
      "</ul>");

   // ── Queue Management ─────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Queue Management") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Active and pending downloads appear in the queue with a position number "
      "in the <b>#</b> column. Use the toolbar buttons to reorder them:</p>"
      "<table cellspacing=\"0\" cellpadding=\"2\">"
      "<tr><td>&#9196;&nbsp;&nbsp;</td><td><b>Move to Top</b> &mdash; jump selected item(s) to position 1</td></tr>"
      "<tr><td>&#9650;&nbsp;&nbsp;</td><td><b>Move Up</b> &mdash; move one position higher</td></tr>"
      "<tr><td>&#9660;&nbsp;&nbsp;</td><td><b>Move Down</b> &mdash; move one position lower</td></tr>"
      "<tr><td>&#9197;&nbsp;&nbsp;</td><td><b>Move to Bottom</b> &mdash; send to the end of the queue</td></tr>"
      "</table>"
      "<p>You can also <b>Delete</b> a queued download to cancel it.</p>");

   // ── Search ───────────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Search") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Type a search term into the <b>Search</b> bar (top of the window) and "
      "press Enter. Results are shown in a new tab. You can open multiple search "
      "tabs simultaneously.</p>"
      "<p>Search matches file and folder names across all connected peers.</p>");

   // ── Settings ─────────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Settings") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Open settings from the menu bar: <b>Settings &gt; Preferences...</b></p>");

   html += QStringLiteral("<p>") + subheading.arg("Nickname") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Your display name on the network. Other users see this in the peer list.</p>");

   html += QStringLiteral("<p>") + subheading.arg("Shared folders") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Manage the directories you share with the network:</p>"
      "<ul>"
      "<li><b>Add</b> &mdash; select a folder to share.</li>"
      "<li><b>Remove</b> &mdash; stop sharing the selected folder.</li>"
      "<li><b>Move Up / Move Down</b> &mdash; reorder folders in the list.</li>"
      "<li><b>Open Folder</b> &mdash; open the selected folder in your file manager.</li>"
      "</ul>");

   // ── Master / Client Mode ─────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Master / Client Mode") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>DG-LAN uses a <b>master/client</b> architecture for the network file index:</p>"
      "<ul>"
      "<li><b>Master mode</b> (default) &mdash; this machine maintains and serves "
      "the combined file index for all peers. At least one machine on the network "
      "must run as master.</li>"
      "<li><b>Client mode</b> &mdash; this machine connects to a master for the "
      "shared index. Enable this via <b>Settings &gt; Preferences &gt; Client mode</b>.</li>"
      "</ul>"
      "<p>If multiple machines are set as master, one is automatically elected to "
      "serve the canonical index.</p>");

   // ── Passwords ────────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Passwords") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Two passwords can be configured in Settings:</p>"
      "<ul>"
      "<li><b>Remote password</b> &mdash; required to connect a remote GUI to "
      "the core. Set or change this in the password section of Settings.</li>"
      "<li><b>Master password</b> &mdash; protects the master node so that only "
      "authorised clients can join the network. Set via <b>Reset Master Key</b> "
      "in Settings.</li>"
      "</ul>"
      "<p>If you enter the wrong master password, a warning dialog will appear.</p>");

   // ── Network Settings ─────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Network Settings") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<ul>"
      "<li><b>Network interface</b> &mdash; bind DG-LAN to a specific NIC "
      "(useful if your machine has multiple network adapters).</li>"
      "<li><b>Multicast TTL</b> &mdash; controls how far multicast discovery "
      "packets travel. The default of 1 keeps traffic on the local subnet.</li>"
      "<li><b>Force IPv4</b> &mdash; disable IPv6 peer discovery if your "
      "network does not support it.</li>"
      "</ul>");

   // ── Integrity Check ──────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Integrity Check") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>Enable <b>File integrity check</b> in Settings to verify downloaded "
      "chunks with a hash. This catches rare transfer corruption at the cost of "
      "a small amount of extra CPU usage.</p>");

   // ── Auto-Updates ─────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Auto-Updates") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>DG-LAN checks GitHub for new releases on startup. When a newer version "
      "is available, a scrolling notification appears at the top of the window. "
      "Click the notification or use <b>Help &gt; Check for Updates...</b> to "
      "open the download page.</p>");

   // ── Log Panel ────────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Log Panel") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<p>The log panel at the bottom of the window shows network events, "
      "connection status, and transfer activity. It can be shown or hidden "
      "via the status bar toggle.</p>");

   // ── Keyboard Shortcuts ───────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Keyboard Shortcuts") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<table cellspacing=\"0\" cellpadding=\"2\">"
      "<tr><td><b>Enter</b>&nbsp;&nbsp;</td><td>Download selected / open folder</td></tr>"
      "<tr><td><b>Delete</b>&nbsp;&nbsp;</td><td>Remove selected download from queue</td></tr>"
      "<tr><td><b>Backspace</b>&nbsp;&nbsp;</td><td>Navigate up one folder level</td></tr>"
      "<tr><td><b>Ctrl+F</b>&nbsp;&nbsp;</td><td>Focus the search bar</td></tr>"
      "</table>");

   // ── Tips ─────────────────────────────────────────────────────────────────
   html += QStringLiteral("<p>") + heading.arg("Tips") + QStringLiteral("</p>");
   html += QStringLiteral(
      "<ul>"
      "<li>Share your game installers <i>before</i> the LAN party starts so "
      "the file index is ready when everyone connects.</li>"
      "<li>Use the peer list to check who is online and how much they are sharing.</li>"
      "<li>If downloads are slow, check that both machines are on the same switch "
      "and not routing through Wi-Fi.</li>"
      "<li>Set a master password if you want to restrict who can join the network.</li>"
      "</ul>");

   this->ui->txtGuide->setHtml(html);
}

DialogUserGuide::~DialogUserGuide()
{
   delete this->ui;
}

void DialogUserGuide::paintEvent(QPaintEvent* event)
{
   Q_UNUSED(event);
   QPainter p(this);
   QLinearGradient gradient(0, 0, 0, height());
   gradient.setColorAt(0, QColor(12, 22, 42));
   gradient.setColorAt(0.6, QColor(18, 38, 72));
   gradient.setColorAt(1, QColor(30, 65, 120));
   p.fillRect(QRect(0, 0, width(), height()), gradient);
}

void DialogUserGuide::changeEvent(QEvent* event)
{
   if (event->type() == QEvent::LanguageChange)
      this->ui->retranslateUi(this);

   QDialog::changeEvent(event);
}
