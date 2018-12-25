/** \file session-loader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86
*/

#include <QFile>
#include <QDir>
#include <QCoreApplication>

#include "sessionLoader.h"
void KDESessionLoader::setEnabled(const bool &enabled)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, enabled: "+QString::number(enabled));
    QFile link(QDir::homePath()+"/.kde4/Autostart/ultracopier.sh");
    if(!enabled)
    {
        if(link.exists() && !link.remove())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to remove from the startup: "+link.errorString());
    }
    else
    {
        if(link.open(QIODevice::WriteOnly))
        {
            link.write(QStringLiteral("#!/bin/bash\n").toLocal8Bit());
            link.write(QString(QCoreApplication::applicationFilePath()).toLocal8Bit());
            link.close();
            if(!link.setPermissions(QFile::ExeOwner|QFile::WriteOwner|QFile::ReadOwner))
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to set permissions: "+link.errorString());
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"unable to open in writing the file: "+link.errorString());
    }
}

bool KDESessionLoader::getEnabled() const
{
    //return the value into the variable
    return QFile::exists(QDir::homePath()+"/.kde4/Autostart/ultracopier.sh");
}

void KDESessionLoader::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
}

/// \brief to get the options widget, NULL if not have
QWidget * KDESessionLoader::options()
{
    return NULL;
}

/// \brief to reload the translation, because the new language have been loaded
void KDESessionLoader::newLanguageLoaded()
{
}
