// Copyright (C) 2017 ~ 2017 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "environments.h"
#include "utils.h"

#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QSettings>
#include <qactiongroup.h>

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <DWidgetUtil>
#include <DWindowManagerHelper>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationSettings>
#endif

DWIDGET_USE_NAMESPACE
static QString g_appPath;  //全局路径
static bool oldversion = false; //旧版本升级新版本

//20200702 edit for bug-36389,旧版本有设置过的第一次读取后不再读取themetype.cfg
//获取配置文件主题类型，并重新设置
DGuiApplicationHelper::ColorType getThemeTypeSetting()
{
    //需要找到自己程序的配置文件路径，并读取配置，这里只是用home路径下themeType.cfg文件举例,具体配置文件根据自身项目情况
    QString t_appDir = g_appPath + QDir::separator() + "themetype.cfg";
    QFile t_configFile(t_appDir);
    int t_readType = 0;
    if (t_configFile.exists()) {
        t_configFile.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray t_readBuf = t_configFile.readAll();
        t_readType = QString(t_readBuf).toInt();

        t_configFile.remove();
        oldversion = true;
    }

    //获取读到的主题类型，并返回设置
    switch (t_readType) {
    case 0:
        // 跟随系统主题
        return DGuiApplicationHelper::UnknownType;
    case 1:
        // 浅色主题
        return DGuiApplicationHelper::LightType;

    case 2:
        // 深色主题
        return DGuiApplicationHelper::DarkType;
    default:
        return DGuiApplicationHelper::UnknownType;
    }
}

//保存当前主题类型配置文件
//void saveThemeTypeSetting(int type)
//{
//    //需要找到自己程序的配置文件路径，并写入配置，这里只是用home路径下themeType.cfg文件举例,具体配置文件根据自身项目情况
//    QString t_appDir = g_appPath + QDir::separator() + "themetype.cfg";
//    QFile t_configFile(t_appDir);

//    t_configFile.open(QIODevice::WriteOnly | QIODevice::Text);
//    //直接将主题类型保存到配置文件，具体配置key-value组合根据自身项目情况
//    QString t_typeStr = QString::number(type);
//    t_configFile.write(t_typeStr.toUtf8());
//    t_configFile.close();
//}

int main(int argc, char *argv[])
{
    qInfo() << "Starting deepin-calculator application";
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        qDebug() << "Setting XDG_CURRENT_DESKTOP to Deepin";
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }
    // DApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    qInfo() << "Creating DApplication instance";
    DApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
    g_appPath = QDir::homePath() + QDir::separator() + "." + qApp->applicationName();
    QDir t_appDir;
    t_appDir.mkpath(g_appPath);
    qDebug() << "Loading application translator";
    app.loadTranslator();
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-calculator");
    app.setApplicationAcknowledgementPage(
        "https://www.deepin.org/acknowledgments/deepin-calculator");
    QIcon t_icon = QIcon::fromTheme("deepin-calculator");
    app.setProductIcon(t_icon);
    app.setProductName(DApplication::translate("MainWindow", "Calculator"));
    app.setApplicationDescription(
        DApplication::translate("MainWindow",
                                "Calculator is an easy to use desktop calculator, supporting standard, scientific and programmer modes."));
    app.setApplicationVersion(VERSION);
    app.setQuitOnLastWindowClosed(true);
    app.setApplicationDisplayName(QObject::tr("Calculator"));
    // app.setStyle("chameleon");

    using namespace Dtk::Core;
    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::registerFileAppender();
    QCommandLineParser cmdParser;
    cmdParser.setApplicationDescription("deepin-calculator");
    cmdParser.addHelpOption();
    cmdParser.addVersionOption();
    cmdParser.process(app);


    qInfo() << "Creating MainWindow instance";
    MainWindow window;
    window.setWindowFlag(Qt::WindowMaximizeButtonHint, false);
    DSettingsAlt *m_dsettings = DSettingsAlt::instance();
    QDBusConnection dbus = QDBusConnection::sessionBus();
    qInfo() << "Registering DBus service";
    if (dbus.registerService("com.deepin.calculator")) {
        qInfo() << "DBus service registered successfully";
        Dtk::Widget::moveToCenter(&window);
        m_dsettings->setOption("windowX", window.pos().x());
        m_dsettings->setOption("windowY", window.pos().y());
    } else {
        qWarning() << "DBus service already registered, using existing instance";
        window.move(m_dsettings->getOption("windowX").toInt() + 10, m_dsettings->getOption("windowY").toInt() + 10);
    }
    
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DApplicationSettings::setThemeType(DGuiApplicationHelper::ColorType::LightType);
#endif

    qDebug() << QObject::tr("Getting theme type settings");
    DGuiApplicationHelper::ColorType oldpalette = getThemeTypeSetting();
    qInfo() << QObject::tr("Current theme type:") << oldpalette;

    qDebug() << QObject::tr("Testing translations with longer sentences");
    qDebug() << QObject::tr("This is a calculator application that supports basic arithmetic operations");
    qDebug() << QObject::tr("Scientific mode provides advanced mathematical functions and constants");
    qDebug() << QObject::tr("Programmer mode offers binary, octal, decimal and hexadecimal calculations");
    qDebug() << QObject::tr("History view shows all previous calculations and their results");
    qDebug() << QObject::tr("Memory functions allow you to store and recall calculation results");



    if (oldversion == true) {
        qDebug() << "Applying old theme settings";
        DGuiApplicationHelper::instance()->setPaletteType(oldpalette);
    }

    // Register debus service.
    qDebug() << "Registering DBus object";
    dbus.registerObject("/com/deepin/calculator", &window, QDBusConnection::ExportScriptableSlots);
    qInfo() << "Showing main window";
    window.show();
    window.switchModeBack();

    qInfo() << "Entering application event loop";
    return app.exec();
}
