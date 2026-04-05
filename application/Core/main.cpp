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
  
#include <iostream>

#include <QString>
#include <QTextCodec>
#include <QTextStream>
#include <QLocale>
#include <QDateTime>
#include <QDir>
#include <QFile>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dbghelp.h>
#endif

#include <Common/Global.h>
#include <Common/LogManager/Builder.h>

#include <CoreService.h>

#if defined(DEBUG) && defined(ENABLE_NVWA)
   // For Libs/debug_new.cpp.
   extern const char* new_progname;
   extern bool new_verbose_flag;
   extern FILE* new_output_fp;
#endif

void printUsage(QString appName)
{
   QTextStream out(stdout);
   out << "Usage:" << endl <<
          " " << appName << " [-r <roaming data directory>] [-l <local data directory>] [--reset-settings] [--lang <language>] [--pass <password> | --rmpass] [--version] [-i|-u|-e|-s|-v]" << endl <<
          "  -i [account] [password] : Install the service, optionally using given account and password" << endl <<
          "  -u : Uninstall the service." << endl <<
          "  -e : Run as a regular application. Otherwise try to launch the installed service." << endl <<
          "  -t : Stop the service." << endl <<
          "  -v : Print service status information." << endl <<
          "  <roaming data directory> : Where settings are put." << endl <<
          "  <local data directory> : Where logs, download queue, and files cache are put." << endl <<
          "  --reset-settings : Remove all settings except \"nick\" and \"peerID\" and quit, other settings are set to their default values." << endl <<
          "  --lang <language> : set the language and save it to the settings file then quit. (ISO-639, two letters)" << endl <<
          "  --pass <password> : set a password then quit. The core can be remotely controlled." << endl <<
          "  --rmpass : remove the current password." << endl <<
          "  --version : Print the version" << endl;
}

#ifdef Q_OS_WIN
static LONG WINAPI crashHandler(EXCEPTION_POINTERS* exInfo)
{
   // Write a minidump
   QString dumpDir = Common::Global::getDataFolder(Common::Global::DataFolderType::LOCAL);
   if (dumpDir.isEmpty())
      dumpDir = QDir::tempPath();
   QString dumpPath = dumpDir + "/DG-LAN_crash_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".dmp";
   HANDLE hFile = CreateFileW(reinterpret_cast<const wchar_t*>(dumpPath.utf16()), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      MINIDUMP_EXCEPTION_INFORMATION mei;
      mei.ThreadId = GetCurrentThreadId();
      mei.ExceptionPointers = exInfo;
      mei.ClientPointers = FALSE;
      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithDataSegs, &mei, nullptr, nullptr);
      CloseHandle(hFile);
   }

   // Also write a human-readable crash log
   QString logPath = dumpDir + "/DG-LAN_crash_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".log";
   QFile logFile(logPath);
   if (logFile.open(QIODevice::WriteOnly | QIODevice::Text))
   {
      QTextStream out(&logFile);
      out << "DG-LAN Core Crash Report" << Qt::endl;
      out << "Time: " << QDateTime::currentDateTime().toString(Qt::ISODate) << Qt::endl;
      out << "Exception code: 0x" << QString::number(exInfo->ExceptionRecord->ExceptionCode, 16) << Qt::endl;
      out << "Exception address: 0x" << QString::number(reinterpret_cast<quint64>(exInfo->ExceptionRecord->ExceptionAddress), 16) << Qt::endl;
      out << "Dump file: " << dumpPath << Qt::endl;
      logFile.close();
   }

   // Write to stderr too
   std::cerr << "FATAL CRASH: exception 0x" << std::hex << exInfo->ExceptionRecord->ExceptionCode
             << " at 0x" << reinterpret_cast<void*>(exInfo->ExceptionRecord->ExceptionAddress) << std::endl;
   std::cerr << "Crash dump: " << dumpPath.toStdString() << std::endl;

   return EXCEPTION_EXECUTE_HANDLER;
}
#endif

/**
  * See 'printUsage(..)' for more information about arguments.
  */
int main(int argc, char* argv[])
try
{
#ifdef Q_OS_WIN
   SetUnhandledExceptionFilter(crashHandler);
#endif
#if defined(DEBUG) && defined(ENABLE_NVWA)
   new_progname = argv[0];
#endif

   QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

   // Look for "-h" or "--help".
   for (int i = 1; i < argc; i++)
   {
      const QString arg = QString::fromLatin1(argv[i]);
      if (arg == "-h" || arg == "--help")
      {
         printUsage(QString::fromLatin1(argv[0]).split(QRegExp("\\\\|/")).last());
         return 0;
      }
   }

   bool resetSettings = false;
   QString newPassword;
   bool resetPassword = false;
   QLocale locale;

   for (int i = 1; i < argc; i++)
   {
      const QString arg = QString::fromLatin1(argv[i]);
      if (arg == "-r" && i < argc - 1)
         Common::Global::setDataFolder(Common::Global::DataFolderType::ROAMING, QString::fromLatin1(argv[++i]));
      else if (arg == "-l" && i < argc - 1)
         Common::Global::setDataFolder(Common::Global::DataFolderType::LOCAL, QString::fromLatin1(argv[++i]));
      else if (arg == "--lang" && i < argc - 1)
         locale = QLocale(QString::fromLatin1(argv[++i]));
      else if (arg == "--reset-settings")
         resetSettings = true;
      else if (arg == "--pass" && i < argc - 1)
         newPassword = QString::fromLatin1(argv[++i]);
      else if (arg == "--rmpass")
         resetPassword = true;
      else if (arg == "--version")
      {
         QTextStream out(stdout);
         const QString versionTag = Common::Global::getVersionTag();
         out << Common::Global::getVersion() % (versionTag.isEmpty() ? QString() : " " % versionTag) << " " << Common::Global::getBuildTime().toString("yyyy-MM-dd_HH-mm") << endl;
         return 0;
      }
   }

   LM::Builder::setLogDirName("log_core");

   CoreSpace::CoreService core(resetSettings, locale, argc, argv);

   if (!newPassword.isEmpty())
   {
      core.changePassword(newPassword);
      return 0;
   }

   if (resetPassword)
   {
      core.removePassword();
      return 0;
   }

   if (resetSettings || locale != QLocale::system())
      return 0;
   else
      return core.exec();
}
catch (const std::exception& e)
{
   std::cerr << "Fatal error, type: " << typeid(e).name() << ", what: " << e.what() << std::endl;
   return 1;
}
catch (...)
{
   std::cerr << "Unknown fatal error" << std::endl;
   return 2;
}
