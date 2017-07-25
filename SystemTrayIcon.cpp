//
// Created by pikachu on 17-7-20.
//

#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QMessageBox>
#include "SystemTrayIcon.h"
#include "EditOnlinePacUrlDialog.h"
#include "SsValidator.h"

void output(Profile& profile){
    qDebug()<<"profile";
    qDebug()<<"server"<<profile.server<<profile.server_port<<profile.password;
    qDebug()<<"local "<<profile.local_address<<profile.local_port;
    qDebug()<<"method"<<profile.method;
    qDebug()<<"timeout"<<profile.timeout;
}
SystemTrayIcon::SystemTrayIcon(QObject *parent)
        : QSystemTrayIcon(parent),
          networkInter("com.deepin.daemon.Network", "/com/deepin/daemon/Network", QDBusConnection::sessionBus(), this) {
    setIcon(QIcon(":/icons/shadowsocks.ico"));
    menu = new QMenu("menu");
    startSystemAgentAction = new QAction("启动系统代理", this);
    startSystemAgentAction->setCheckable(true);
    menu->addAction(startSystemAgentAction);

    systemAgentModeMenu = new QMenu("系统代理模式", menu);
    QActionGroup* systemAgentModeActionGroup=new QActionGroup(this);
    pacModeAction = new QAction("PAC模式", this);
    globelModeAction = new QAction("全局模式", this);
    systemAgentModeActionGroup->addAction(pacModeAction);
    systemAgentModeActionGroup->addAction(globelModeAction);
    pacModeAction->setCheckable(true);
    globelModeAction->setCheckable(true);
    systemAgentModeMenu->addAction(pacModeAction);
    systemAgentModeMenu->addAction(globelModeAction);
    menu->addMenu(systemAgentModeMenu);

    serverMenu = new QMenu("服务器", menu);
    loadBalancingAction = new QAction("负载均衡", this);
    highAvailabilityAction = new QAction("高可用", this);
    accordingToStatisticsAction = new QAction("根据统计", this);
    serverGroup=new QActionGroup(this);
    editServerAction = new QAction("编辑服务器...", this);
    statisticsConfigurationAction = new QAction("统计配置...", this);
    shareServerConfigurationAction = new QAction("分享服务器配置...", this);
    scanThe2DCodeOnTheScreenAction = new QAction("扫描屏幕上的二维码...", this);
    importTheURLFromTheClipboardAction = new QAction("从剪贴板导入URL...", this);
    menu->addMenu(serverMenu);

    pacMenu = new QMenu("PAC", menu);
    useLocalPacAction = new QAction("使用本地PAC", this);
    useOnlinePacAction = new QAction("使用在线PAC", this);
    useLocalPacAction->setCheckable(true);
    useOnlinePacAction->setCheckable(true);
    QActionGroup* pacGroup=new QActionGroup(this);
    pacGroup->addAction(useLocalPacAction);
    pacGroup->addAction(useOnlinePacAction);
    editLocalPacFileAction = new QAction("编辑本地PAC文件...", this);
    updateLocalPacFromGFWListAction = new QAction("从GFWList更新本地PAC", this);
    editUserRulesForGFWListAction = new QAction("编辑GFWList的用户规则...", this);
    protectLocalPacAction = new QAction("保护本地PAC", this);
    copyLocalPacUrlAction = new QAction("复制本地PAC网址", this);
    editOnlinePacUrlAction = new QAction("编辑在线PAC网址...", this);
    pacMenu->addAction(useLocalPacAction);
    pacMenu->addAction(useOnlinePacAction);
    pacMenu->addSeparator();
    pacMenu->addAction(editLocalPacFileAction);
    pacMenu->addAction(updateLocalPacFromGFWListAction);
    pacMenu->addAction(editUserRulesForGFWListAction);
    pacMenu->addAction(protectLocalPacAction);
    pacMenu->addAction(copyLocalPacUrlAction);
    pacMenu->addAction(editOnlinePacUrlAction);
    menu->addMenu(pacMenu);

    positiveAgentAction = new QAction("正向代理...", this);
    menu->addAction(positiveAgentAction);

    menu->addSeparator();

    bootAction = new QAction("开机启动", this);
    menu->addAction(bootAction);

    allowConnectionsFromTheLANAction = new QAction("允许来自局域网的连接", this);
    menu->addAction(allowConnectionsFromTheLANAction);

    menu->addSeparator();

    editShortcutsAction = new QAction("编辑快捷键...", this);
    menu->addAction(editShortcutsAction);

    helpMenu = new QMenu("帮助", menu);
    showLogAction = new QAction("显示日志", this);
    detailedLogAction = new QAction("详细记录日志", this);
    updateMenu = new QMenu("更新", helpMenu);
    checkForUpdateAction = new QAction("检查更新", this);
    updateMenu->addAction(checkForUpdateAction);
    updateMenu->addSeparator();
    checkForUpdatesAtStartupAction = new QAction("启动时检查更新", this);
    checkTheBetaUpdateAction = new QAction("检查测试版更新", this);
    updateMenu->addAction(checkForUpdatesAtStartupAction);
    updateMenu->addAction(checkTheBetaUpdateAction);
    helpMenu->addAction(showLogAction);
    helpMenu->addAction(detailedLogAction);
    helpMenu->addMenu(updateMenu);
    menu->addMenu(helpMenu);

    aboutAction = new QAction("关于", this);
    menu->addAction(aboutAction);

    exitAction = new QAction("退出", this);
    menu->addAction(exitAction);

    setContextMenu(menu);


    startSystemAgentAction->setChecked(false);
    systemAgentModeMenu->setEnabled(false);

    controller=new Controller(true);
        QObject::connect(controller, &QSS::Controller::debug, [](QString log) {
        qDebug() << "[QSS::Controller::debug]" << log;
    });
    QObject::connect(controller, &QSS::Controller::info, [](QString log) {
        qDebug() << "[QSS::Controller::info]" << log;
    });
//    editServerDialog=new EditServerDialog(profiles);
    initConfig();

    connect(startSystemAgentAction, &QAction::changed, [this]() {

        bool checked=startSystemAgentAction->isChecked();
        if (!checked) {
            qDebug() << "取消系统代理";
            systemAgentModeMenu->setEnabled(false);
            setProxyMethod("none");
            controller->stop();
        } else {
            qDebug() << "启动系统代理";
            systemAgentModeMenu->setEnabled(true);
            if (pacModeAction->isChecked()){
                setAutoProxy();
            } else if(globelModeAction->isChecked()){
                setManualProxy();
            } else{
                pacModeAction->trigger();
            }

            for(auto&it:serverGroup->actions()){
                ServerAction* action= dynamic_cast<ServerAction *>(it);
                controller->setup(action->profile);
                controller->start();
            }
        }

    });
    connect(systemAgentModeActionGroup,&QActionGroup::triggered,[this](QAction *action){
        if(action==pacModeAction){
            setAutoProxy();
        } else{
            setManualProxy();
        }
    });
    connect(serverGroup,&QActionGroup::triggered,this,&SystemTrayIcon::onServerActionTriggered);
    connect(editServerAction,&QAction::triggered,this,&SystemTrayIcon::onEditServerActionTriggered);
    connect(pacGroup,&QActionGroup::triggered,[=](QAction *action){
        if (action==useLocalPacAction){
            pacConfig.is_local=true;
#ifdef QT_DEBUG
            qDebug()<<"use local pac";
#endif
        } else{
#ifdef QT_DEBUG
            qDebug()<<"use online pac";
#endif
            EditOnlinePacUrlDialog dialog(pacConfig.onlineUrl);
            int ret = dialog.exec();
            if(ret==QDialog::Accepted){
                pacConfig.onlineUrl=dialog.getOnlinePacUrl();
                pacConfig.is_local= false;
            } else{
                useLocalPacAction->trigger();
            }
        }
        setAutoProxy();
    });
    connect(shareServerConfigurationAction,&QAction::triggered,[=](){
#ifdef QT_DEBUG
        qDebug()<<"打开分享窗口";
#endif
        shareServerConfigWidget=new ShareServerConfigWidget();
        shareServerConfigWidget->exec();
    });
    connect(scanThe2DCodeOnTheScreenAction,&QAction::triggered,this,&SystemTrayIcon::onScanThe2DCodeOnTheScreenActionTriggered);
    connect(importTheURLFromTheClipboardAction,&QAction::triggered,[=](){
        QString uri=QApplication::clipboard()->text().trimmed();
        qDebug()<<"uri"<<uri;
        if(!SSValidator::validate(uri)){
            QMessageBox::critical(
                    nullptr,
                    tr("从剪贴板导入URL错误"),
                    tr("没有找到包含有效ss uri的字符串"));
        } else{
            Profile profile = QSS::Profile(uri.toUtf8());
            Config config;
            config.profile=profile;
            config.remarks=profile.server;
            configs.append(config);
            ConfigUtil::saveConfig(configs);
//            editServerAction->trigger();
            onEditServerActionTriggered(true);
        }
    });
    connect(exitAction, &QAction::triggered, []() {
        qApp->exit();
    });


    updateServerMenu();
    if(!configs.isEmpty()){
        startSystemAgentAction->trigger();
        pacModeAction->trigger();
    }
}

void SystemTrayIcon::setProxyMethod(QString proxyMethod) {
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(networkInter.SetProxyMethod(proxyMethod), this);
    QObject::connect(w, &QDBusPendingCallWatcher::finished, [=] {
        qDebug()<<"success to set proxy method "<<proxyMethod;
    });
    connect(w, &QDBusPendingCallWatcher::finished, w, &QDBusPendingCallWatcher::deleteLater);
}

void SystemTrayIcon::setManualProxy() {
    setProxyMethod("manual");
    QString type="socks";
//    QString addr="127.0.0.1";
    QString addr=localAddress;
//    QString port="1080";
    QString port=localPort;
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(networkInter.SetProxy(type, addr, port), this);
    QObject::connect(w, &QDBusPendingCallWatcher::finished, [=] {
        qDebug()<<"set proxy"<<type<<addr<<port;
    });
}

void SystemTrayIcon::setAutoProxy() {
    setProxyMethod("auto");
    QString proxy=pacConfig.getProxy();
    QDBusPendingCallWatcher *w = new QDBusPendingCallWatcher(networkInter.SetAutoProxy(proxy), this);

    QObject::connect(w, &QDBusPendingCallWatcher::finished, [=](){
        qDebug()<<"set auto proxy finished"<<proxy;
    });
}

void SystemTrayIcon::updateServerMenu() {
#ifdef QT_DEBUG
    qDebug()<<"刷新服务器列表";
#endif
    serverMenu->clear();
    serverMenu->addAction(loadBalancingAction);
    serverMenu->addAction(highAvailabilityAction);
    serverMenu->addAction(accordingToStatisticsAction);
    serverMenu->addSeparator();
    if(!configs.isEmpty()){
        delete serverGroup;
        QAction* t;
        serverGroup=new QActionGroup(this);
        connect(serverGroup,&QActionGroup::triggered,this,&SystemTrayIcon::onServerActionTriggered);
        for (int i = 0; i < configs.size(); ++i) {
            auto it=configs[i];
#ifdef QT_DEBUG
            qDebug()<<it.remarks;
#endif
            ServerAction* action=new ServerAction(it.remarks,this);
            action->profile=it.profile;
            action->setCheckable(true);
            serverGroup->addAction(action);
            serverMenu->addAction(action);
            if(i==0){
                t=action;
            }
#ifdef QT_DEBUG
            output(action->profile);
#endif
        }
        serverMenu->addSeparator();
        if (t!= nullptr){
            t->trigger();
        }
    } else{
        startSystemAgentAction->setEnabled(false);
    }
    serverMenu->addAction(editServerAction);
    serverMenu->addAction(statisticsConfigurationAction);
    serverMenu->addSeparator();
    serverMenu->addAction(shareServerConfigurationAction);
    serverMenu->addAction(scanThe2DCodeOnTheScreenAction);
    serverMenu->addAction(importTheURLFromTheClipboardAction);

}

void SystemTrayIcon::initConfig() {
    configs=ConfigUtil::readConfig();
    downloadPac();
}

void SystemTrayIcon::onServerActionTriggered(QAction *action) {
    ServerAction* serverAction= dynamic_cast<ServerAction *>(action);
    controller->setup(serverAction->profile);
    localAddress=serverAction->profile.local_address;
    localPort=QString::number(serverAction->profile.local_port);
    if(startSystemAgentAction->isChecked()){
        controller->stop();
        controller->setup(serverAction->profile);
        controller->start();
    }
}

void SystemTrayIcon::downloadPac() {
    pacConfig.onlineUrl="https://raw.githubusercontent.com/PikachuHy/ss/master/autoproxy.pac";
    QFile *pacFile;//文件指针
    QNetworkReply *pacReply;
    QNetworkAccessManager *pacManager;
    //下载文件之前先在本地创建一个文件
    QDir file;//文件夹
    QString fileStr = QObject::tr("%1/.ss/").arg(QDir::homePath());
    QString fileName = fileStr + "/autoproxy.pac";
    //判断文件夹是否存在 不存在创建
    if (!file.exists(fileStr)) {
        file.mkpath(fileStr);
    }
    pacFile = new QFile(fileName);
    //判断文件是否可写入 不可写删除 指针赋值0
    if (!pacFile->open(QIODevice::WriteOnly)) {
        delete pacFile;
        pacFile = 0;
        return;
    }
    //开始请求 下载文件
    QUrl  serviceUrl = QUrl("https://raw.githubusercontent.com/PikachuHy/ss/master/autoproxy.pac");
    pacManager = new QNetworkAccessManager();
//get方式请求 如需加密用post
    pacReply=pacManager->get(QNetworkRequest(serviceUrl));
    QObject::connect(pacReply,&QNetworkReply::readyRead,[=](){
        if(pacFile){
            pacFile->write(pacReply->readAll());
        }

    });//数据写入
    QObject::connect(pacReply,&QNetworkReply::finished,[=](){
        //刷新文件
        pacFile->flush();
        pacFile->close();
        pacFile->deleteLater();
        pacReply->deleteLater();
        pacManager->deleteLater();
        pacConfig.localFilePath=fileName;
        pacConfig.is_local=true;
        if (!configs.isEmpty()){
            useLocalPacAction->trigger();
        }
    });//请求完成
}

void SystemTrayIcon::onEditServerActionTriggered(bool isNew) {
#ifdef QT_DEBUG
    qDebug()<<"启动服务器配置界面";
#endif
    EditServerDialog* w = new EditServerDialog(isNew);
    int ret=w->exec();
    if(ret==QDialog::Accepted){
        configs = ConfigUtil::readConfig();
#ifdef QT_DEBUG
        qDebug()<<"configs.isEmpty()"<<configs.isEmpty()<<"startSystemAgentAction->isEnabled()"<<startSystemAgentAction->isEnabled();
        qDebug()<<"configs.isEmpty() && startSystemAgentAction->isEnabled()"<<(configs.isEmpty() && startSystemAgentAction->isEnabled());
        qDebug()<<"!configs.isEmpty() && !startSystemAgentAction->isEnabled()"<<(!configs.isEmpty() && !startSystemAgentAction->isEnabled());
#endif
        if(configs.isEmpty() && startSystemAgentAction->isEnabled()){
            startSystemAgentAction->setChecked(false);
            startSystemAgentAction->setEnabled(false);
        } else if (!configs.isEmpty() && !startSystemAgentAction->isEnabled()){
            startSystemAgentAction->setEnabled(true);
        }
        updateServerMenu();
//#ifdef QT_DEBUG
//            qDebug()<<"保存配置";
//#endif
//            configs=ConfigUtil::readConfig();
//            updateServerMenu();
//#ifdef QT_DEBUG
//            qDebug()<<configs.isEmpty();
//#endif
//            if(!configs.isEmpty()){
//                startSystemAgentAction->setCheckable(true);
//            } else{
//                if(startSystemAgentAction->isChecked()){
//                    startSystemAgentAction->trigger();
//                }
//                startSystemAgentAction->setCheckable(false);
//            }
        if(startSystemAgentAction->isChecked()){
#ifdef QT_DEBUG
            qDebug()<<"重新启动";
#endif
            controller->stop();
            if (!configs.isEmpty()){
                Profile& profile=configs.first().profile;
                controller->setup(profile);
                localAddress=profile.local_address;
                localPort=QString::number(profile.local_port);
                controller->start();
            }
        }
    }
}

void SystemTrayIcon::onScanThe2DCodeOnTheScreenActionTriggered() {
    QString uri;
    QList<QScreen *> screens = qApp->screens();
    for (QList<QScreen *>::iterator sc = screens.begin();
         sc != screens.end();
         ++sc) {
        QImage raw_sc = (*sc)->grabWindow(qApp->desktop()->winId()).toImage();
        QString result = URIHelper::decodeImage(raw_sc);
        if (!result.isNull()) {
            uri = result;
        }
    }
    qDebug()<<"扫描到二维码"<<uri;
    if(!SSValidator::validate(uri)){
        QMessageBox::critical(
                nullptr,
                tr("未找到二维码"),
                tr("没有找到包含有效ss uri的二维码"));
    } else{
        Profile profile = QSS::Profile(uri.toUtf8());
        Config config;
        config.profile=profile;
        config.remarks=profile.server;
        configs.append(config);
        ConfigUtil::saveConfig(configs);
//            editServerAction->trigger();
        onEditServerActionTriggered(true);
    }

}

ServerAction::ServerAction(const QString &text, QObject *parent) : QAction(text, parent) {

}
