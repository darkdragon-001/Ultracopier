#include "InternetUpdater.h"
#include "EventDispatcher.h"
#include "OptionEngine.h"
#include "cpp11addition.h"

#ifdef ULTRACOPIER_INTERNET_SUPPORT

#include <QNetworkRequest>
#include <QUrl>

#include "PluginsManager.h"

InternetUpdater::InternetUpdater(QObject *parent) :
    QObject(parent)
{
    connect(&newUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    connect(&firstUpdateTimer,&QTimer::timeout,this,&InternetUpdater::downloadFile);
    newUpdateTimer.start(1000*3600);
    firstUpdateTimer.setSingleShot(true);
    firstUpdateTimer.start(1000*60);
}

void InternetUpdater::downloadFile()
{
    if(!stringtobool(OptionEngine::optionEngine->getOptionValue("Ultracopier","checkTheUpdate")))
        return;
    std::string name="Ultracopier";
    std::string ultracopierVersion;
    #ifdef ULTRACOPIER_VERSION_ULTIMATE
    ultracopierVersion=name+" Ultimate/"+ULTRACOPIER_VERSION;
    #else
    ultracopierVersion=name+"/"+ULTRACOPIER_VERSION;
    #endif
    #ifdef ULTRACOPIER_VERSION_PORTABLE
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
             ultracopierVersion+=" portable/all-in-one";
        #else
             ultracopierVersion+=" portable";
        #endif
    #else
        #ifdef ULTRACOPIER_PLUGIN_ALL_IN_ONE
            ultracopierVersion+=" all-in-one";
        #endif
    #endif
    #if defined(Q_OS_WIN32) || defined(Q_OS_MAC)
    ultracopierVersion+=" (OS: "+EventDispatcher::GetOSDisplayString()+")";
    #endif
    ultracopierVersion+=" "+std::string(ULTRACOPIER_PLATFORM_CODE);
    QNetworkRequest networkRequest(QStringLiteral(ULTRACOPIER_UPDATER_URL));
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader,QString::fromStdString(ultracopierVersion));
    networkRequest.setRawHeader("Connection", "Close");
    reply = qnam.get(networkRequest);
    connect(reply, &QNetworkReply::finished, this, &InternetUpdater::httpFinished);
}

void InternetUpdater::httpFinished()
{
    QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if (!reply->isFinished())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"get the new update failed: not finished");
        reply->deleteLater();
        return;
    }
    else if (reply->error())
    {
        newUpdateTimer.stop();
        newUpdateTimer.start(1000*3600*24);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"get the new update failed: "+reply->errorString().toStdString());
        reply->deleteLater();
        return;
    } else if (!redirectionTarget.isNull()) {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"redirection denied to: "+redirectionTarget.toUrl().toString().toStdString());
        reply->deleteLater();
        return;
    }
    QString newVersion=QString::fromUtf8(reply->readAll());
    if(newVersion.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"version string is empty");
        reply->deleteLater();
        return;
    }
    newVersion.remove("\n");
    if(!newVersion.contains(QRegularExpression(QLatin1Literal("^[0-9]+(\\.[0-9]+)+$"))))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"version string don't match: "+newVersion.toStdString());
        reply->deleteLater();
        return;
    }
    if(newVersion==ULTRACOPIER_VERSION)
    {
        reply->deleteLater();
        return;
    }
    if(PluginsManager::compareVersion(newVersion.toStdString(),"<=",ULTRACOPIER_VERSION))
    {
        reply->deleteLater();
        return;
    }
    newUpdateTimer.stop();
    emit newUpdate(newVersion.toStdString());
    reply->deleteLater();
}

#endif
