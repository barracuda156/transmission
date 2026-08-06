# Legacy Qt 4 client, forked from the 2.94-era Qt client and adapted to
# the current libtransmission API. Exists for platforms whose newest
# usable Qt is 4.8 (e.g. Mac OS X 10.5/10.6 PowerPC), where the Qt 5/6
# client in qt/ cannot build. Built only when ENABLE_QT_LEGACY is ON;
# mutually exclusive with ENABLE_QT since both produce transmission-qt.
project(trqt-legacy)

# CMake 4 removed FindQt4 and its helper modules; copies from
# CMake 3.28.6 are vendored under cmake/.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Qt4 4.8.0 REQUIRED QtCore QtGui QtNetwork)

# QtDBus provides single-instance interop and desktop notifications on
# platforms that run a session bus; optional everywhere.
set(ENABLE_QT_LEGACY_DBUS_INTEROP OFF)
if(QT_QTDBUS_FOUND)
    set(ENABLE_QT_LEGACY_DBUS_INTEROP ON)
endif()

set(${PROJECT_NAME}_SOURCES
    AboutDialog.cc
    AddData.cc
    Application.cc
    ColumnResizer.cc
    ComInteropHelper.cc
    DBusInteropHelper.cc
    DetailsDialog.cc
    FaviconCache.cc
    FileTreeDelegate.cc
    FileTreeItem.cc
    FileTreeModel.cc
    FileTreeView.cc
    FilterBar.cc
    FilterBarComboBox.cc
    FilterBarComboBoxDelegate.cc
    FilterBarLineEdit.cc
    Filters.cc
    Formatter.cc
    FreeSpaceLabel.cc
    IconToolButton.cc
    InteropHelper.cc
    InteropObject.cc
    LicenseDialog.cc
    MainWindow.cc
    MakeDialog.cc
    OptionsDialog.cc
    PathButton.cc
    Prefs.cc
    PrefsDialog.cc
    RelocateDialog.cc
    RpcClient.cc
    Session.cc
    SessionDialog.cc
    SqueezeLabel.cc
    StatsDialog.cc
    Torrent.cc
    TorrentDelegate.cc
    TorrentDelegateMin.cc
    TorrentFilter.cc
    TorrentModel.cc
    TorrentView.cc
    TrackerDelegate.cc
    TrackerModel.cc
    TrackerModelFilter.cc
    Utils.cc
    WatchDir.cc)

# COM interop is Windows/ActiveQt-only and is not supported by this client.
set_source_files_properties(ComInteropHelper.cc PROPERTIES HEADER_FILE_ONLY ON)
if(NOT ENABLE_QT_LEGACY_DBUS_INTEROP)
    set_source_files_properties(DBusInteropHelper.cc PROPERTIES HEADER_FILE_ONLY ON)
endif()

set(${PROJECT_NAME}_MOC_HEADERS
    AboutDialog.h
    Application.h
    ColumnResizer.h
    DetailsDialog.h
    FaviconCache.h
    FileTreeDelegate.h
    FileTreeModel.h
    FileTreeView.h
    FilterBar.h
    FilterBarComboBox.h
    FilterBarComboBoxDelegate.h
    FilterBarLineEdit.h
    FreeSpaceLabel.h
    IconToolButton.h
    InteropObject.h
    LicenseDialog.h
    MainWindow.h
    MakeDialog.h
    OptionsDialog.h
    PathButton.h
    Prefs.h
    PrefsDialog.h
    RelocateDialog.h
    RpcClient.h
    Session.h
    SessionDialog.h
    SqueezeLabel.h
    StatsDialog.h
    Torrent.h
    TorrentDelegate.h
    TorrentDelegateMin.h
    TorrentFilter.h
    TorrentModel.h
    TorrentView.h
    TrackerDelegate.h
    TrackerModel.h
    TrackerModelFilter.h
    WatchDir.h)

qt4_wrap_cpp(${PROJECT_NAME}_MOC_SOURCES ${${PROJECT_NAME}_MOC_HEADERS})

# MakeDialog.cc declares MakeProgressDialog and includes its own moc output.
qt4_generate_moc(MakeDialog.cc ${CMAKE_CURRENT_BINARY_DIR}/MakeDialog.moc)

qt4_wrap_ui(${PROJECT_NAME}_UI_SOURCES
    AboutDialog.ui
    DetailsDialog.ui
    LicenseDialog.ui
    MainWindow.ui
    MakeDialog.ui
    MakeProgressDialog.ui
    OptionsDialog.ui
    PrefsDialog.ui
    RelocateDialog.ui
    SessionDialog.ui
    StatsDialog.ui)

set(${PROJECT_NAME}_QRC_FILES application.qrc)
if(WIN32 OR APPLE)
    list(APPEND ${PROJECT_NAME}_QRC_FILES icons/Faenza/Faenza.qrc)
endif()

qt4_add_resources(${PROJECT_NAME}_QRC_SOURCES ${${PROJECT_NAME}_QRC_FILES})

set(${PROJECT_NAME}_LINGUAS
    de
    en
    es
    eu
    fr
    hu
    id
    it_IT
    kk
    ko
    lt
    pl_PL
    pt_BR
    ru
    uk
    zh_CN)

if(ENABLE_NLS)
    set(${PROJECT_NAME}_ENABLED_LINGUAS ${${PROJECT_NAME}_LINGUAS})
else()
    set(${PROJECT_NAME}_ENABLED_LINGUAS)
endif()

set(${PROJECT_NAME}_TS_FILES)
foreach(LANG ${${PROJECT_NAME}_ENABLED_LINGUAS})
    list(APPEND ${PROJECT_NAME}_TS_FILES translations/transmission_${LANG}.ts)
endforeach()

if(${PROJECT_NAME}_TS_FILES)
    qt4_add_translation(${PROJECT_NAME}_QM_FILES ${${PROJECT_NAME}_TS_FILES})
endif()

add_executable(${TR_NAME}-qt WIN32
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_MOC_SOURCES}
    ${${PROJECT_NAME}_UI_SOURCES}
    ${${PROJECT_NAME}_QRC_SOURCES}
    ${CMAKE_CURRENT_BINARY_DIR}/MakeDialog.moc
    ${${PROJECT_NAME}_QM_FILES})

target_include_directories(${TR_NAME}-qt
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${TR_NAME}-qt
    ${TR_NAME}
    Qt4::QtGui
    Qt4::QtNetwork
    $<$<BOOL:${ENABLE_QT_LEGACY_DBUS_INTEROP}>:Qt4::QtDBus>
    Qt4::QtCore)

target_compile_definitions(${TR_NAME}-qt
    PRIVATE
        "TRANSLATIONS_DIR=\"${CMAKE_INSTALL_FULL_DATADIR}/${TR_NAME}/translations\""
        QT_NO_CAST_FROM_ASCII
        $<$<BOOL:${ENABLE_QT_LEGACY_DBUS_INTEROP}>:ENABLE_DBUS_INTEROP>)

install(TARGETS ${TR_NAME}-qt DESTINATION ${CMAKE_INSTALL_BINDIR})

if(INSTALL_DOC)
    install(FILES ${TR_NAME}-qt.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
endif()

install(FILES transmission-qt.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

if(ENABLE_NLS)
    install(FILES ${${PROJECT_NAME}_QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/${TR_NAME}/translations)
endif()
