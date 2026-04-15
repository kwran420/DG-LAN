#-------------------------------------------------
#
# Project created by QtCreator 2010-10-18T14:48:35
#
#-------------------------------------------------

# Uncomment this line to enable the leak detector.
# DEFINES += ENABLE_NVWA

QT += core gui widgets network xml

# winextras removed: not available in MSYS2 Qt 5.15; replaced with direct Win32 APIs

TARGET = "DG-LAN.GUI"
TEMPLATE = app

win32 {
   RC_FILE = ../Common/version.rc
}

include(../Common/common.pri)
include(../Libs/protobuf.pri)
include(../Protos/Protos.pri)

INCLUDEPATH += . ..

LIBS += -L../Common/RemoteCoreController/output/$$FOLDER \
    -lRemoteCoreController
PRE_TARGETDEPS += ../Common/RemoteCoreController/output/$$FOLDER/libRemoteCoreController.a

LIBS += -L../Common/LogManager/output/$$FOLDER \
    -lLogManager
PRE_TARGETDEPS += ../Common/LogManager/output/$$FOLDER/libLogManager.a

LIBS += -L../Common/output/$$FOLDER \
    -lCommon
PRE_TARGETDEPS += ../Common/output/$$FOLDER/libCommon.a

CONFIG(debug, debug|release) {
   contains(DEFINES, ENABLE_NVWA) {
      DEFINES += _DEBUG_NEW_ERROR_CRASH
      SOURCES += ../Libs/Nvwa/debug_new.cpp
   }
}

win32 {
   LIBS += libole32 -lgdi32 -luser32
   SOURCES += Taskbar/TaskbarImplWin.cpp
   HEADERS += Taskbar/TaskbarImplWin.h
}

SOURCES += main.cpp\
    MainWindow.cpp \
    ../Protos/gui_protocol.pb.cc \
    ../Protos/common.pb.cc \
    ../Protos/gui_settings.pb.cc \
    ../Protos/core_settings.pb.cc \
    Settings/SharedEntryListModel.cpp \
    StatusBar.cpp \
    ScrollingNotification.cpp \
    DialogAbout.cpp \
    DialogUserGuide.cpp \
    Log.cpp \
    CheckBoxList.cpp \
    Browse/BrowseModel.cpp \
    Browse/NetworkFileModel.cpp \
    Downloads/DownloadsFlatModel.cpp \
    Log/LogModel.cpp \
    Search/SearchModel.cpp \
    Settings/RemoteFileDialog.cpp \
    DownloadMenu.cpp \
    D-LAN_GUI.cpp \
    ProgressBar.cpp \
    IconProvider.cpp \
    Utils.cpp \
    Settings/AskNewPasswordDialog.cpp \
    Downloads/DownloadsTreeModel.cpp \
    Downloads/DownloadsModel.cpp \
    BusyIndicator.cpp \
    Log/LogDelegate.cpp \
    Peers/PeersDock.cpp \
    Peers/PeerListModel.cpp \
    Peers/PeerListDelegate.cpp \
    Search/SearchDock.cpp \
    MDI/TabButtons.cpp \
    MDI/MdiArea.cpp \
    Browse/BrowseWidget.cpp \
    Browse/NetworkWidget.cpp \
    WelcomeDialog.cpp \
    UpdateChecker.cpp \
    UpdateDialog.cpp \
    Downloads/DownloadsWidget.cpp \
    Search/SearchWidget.cpp \
    Settings/SettingsWidget.cpp \
    MDI/MdiWidget.cpp \
    ColorBox.cpp \
    Constants.cpp \
    Search/SearchUtils.cpp

HEADERS  += MainWindow.h \
    ../Protos/gui_protocol.pb.h \
    ../Protos/common.pb.h \
    ../Protos/gui_settings.pb.h \
    ../Protos/core_settings.pb.h \
    Settings/SharedEntryListModel.h \
    StatusBar.h \
    ScrollingNotification.h \
    Log.h \
    DialogAbout.h \
    DialogUserGuide.h \
    CheckBoxList.h \
    CheckBoxModel.h \
    IFilter.h \
    Browse/BrowseModel.h \
    Browse/NetworkFileModel.h \
    Downloads/DownloadFilterStatus.h \
    Log/LogModel.h \
    Search/SearchModel.h \
    Settings/RemoteFileDialog.h \
    DownloadMenu.h \
    D-LAN_GUI.h \
    ProgressBar.h \
    IconProvider.h \
    Utils.h \
    Settings/AskNewPasswordDialog.h \
    Downloads/DownloadsFlatModel.h \
    Downloads/DownloadsTreeModel.h \
    Downloads/DownloadsModel.h \
    BusyIndicator.h \
    Taskbar/Taskbar.h \
    Taskbar/ITaskbarImpl.h \
    Taskbar/TaskbarTypes.h \
    Log/LogDelegate.h \
    Peers/PeersDock.h \
    Peers/PeerListModel.h \
    Peers/PeerListDelegate.h \
    Search/SearchDock.h \
    MDI/TabButtons.h \
    MDI/MdiArea.h \
    MDI/MdiWidget.h \
    Browse/BrowseWidget.h \
    Browse/NetworkWidget.h \
    WelcomeDialog.h \
    UpdateChecker.h \
    UpdateDialog.h \
    Downloads/DownloadsWidget.h \
    Search/SearchWidget.h \
    Settings/SettingsWidget.h \
    ColorBox.h \
    Constants.h \
    Search/SearchUtils.h

FORMS    += MainWindow.ui \
    StatusBar.ui \
    DialogAbout.ui \
    DialogUserGuide.ui \
    Settings/RemoteFileDialog.ui \
    Settings/AskNewPasswordDialog.ui \
    Peers/PeersDock.ui \
    Search/SearchDock.ui \
    Browse/BrowseWidget.ui \
    Downloads/DownloadsWidget.ui \
    Search/SearchWidget.ui \
    Settings/SettingsWidget.ui

RESOURCES += \
    ressources.qrc
