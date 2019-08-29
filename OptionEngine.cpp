/** \file OptionEngine.cpp
\brief Define the class of the event dispatcher
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QFileInfo>
#include <QDir>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QMessageBox>

#include "OptionEngine.h"

/// \todo async the options write

/// \brief Initiate the option, load from backend
OptionEngine::OptionEngine()
{
    //locate the settings
    QString settingsFilePath=QString::fromStdString(ResourcesManager::resourcesManager->getWritablePath());
    if(QFile::exists(settingsFilePath+"/Ultracopier.conf"))
    {
        if(settingsFilePath!="")
            settings = new QSettings(settingsFilePath+QStringLiteral("Ultracopier.conf"),QSettings::IniFormat);
        else
            settings = NULL;
    }
    else
        settings = new QSettings(QStringLiteral("Ultracopier"),QStringLiteral("Ultracopier"));
    if(settings!=NULL)
    {
        //do some write test
        if(settings->status()!=QSettings::NoError)
        {
            delete settings;
            settings=NULL;
        }
        else if(!settings->isWritable())
        {
            delete settings;
            settings=NULL;
        }
        else
        {
            settings->setValue(QStringLiteral("test"),QStringLiteral("test"));
            if(settings->status()!=QSettings::NoError)
            {
                delete settings;
                settings=NULL;
            }
            else
            {
                settings->remove(QStringLiteral("test"));
                if(settings->status()!=QSettings::NoError)
                {
                    delete settings;
                    settings=NULL;
                }
            }
        }
    }
    //set the backend
    if(settings==NULL)
    {
        #ifdef ULTRACOPIER_VERSION_PORTABLE
        ResourcesManager::resourcesManager->disableWritablePath();
        #endif // ULTRACOPIER_VERSION_PORTABLE
        currentBackend=Memory;
    }
    else
        currentBackend=File;
    connect(this,&OptionEngine::resetOptions,this,&OptionEngine::internal_resetToDefaultValue);
}

/// \brief Destroy the option
OptionEngine::~OptionEngine()
{
}

/// \brief To add option group to options
bool OptionEngine::addOptionGroup(const std::string &groupName,const std::vector<std::pair<std::string, std::string> > &KeysList)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start(\""+groupName+"\",[...])");
    //search if previous with the same name exists
    if(GroupKeysList.find(groupName)!=GroupKeysList.cend())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"group already used previously");
        return false;
    }
    //if the backend is file, enter into the group
    if(currentBackend==File)
        settings->beginGroup(QString::fromStdString(groupName));
    //browse all key, and append it to the key
    unsigned int index=0;
    //QList<OptionEngineGroupKey> KeyListTemp;
    while(index<KeysList.size())
    {
        OptionEngineGroupKey theCurrentKey;
        const std::pair<std::string, std::string> &key=KeysList.at(index);
        theCurrentKey.defaultValue=key.second;
        //if memory backend, load the default value into the current value
        if(currentBackend==Memory)
            theCurrentKey.currentValue=theCurrentKey.defaultValue;
        else
        {
            if(settings->contains(QString::fromStdString(key.first)))//if file backend, load the default value from the file
            {
                theCurrentKey.currentValue=settings->value(QString::fromStdString(key.first)).toString().toStdString();
                #ifdef ULTRACOPIER_DEBUG
                if(theCurrentKey.currentValue!=theCurrentKey.defaultValue)
                {
                    if(groupName=="Ultracopier" && key.first=="key")
                    {
                    }
                    else
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"The current key: "+groupName+", group: "+key.first+", have value: "+theCurrentKey.currentValue);
                }
                #endif
            }
            else //or if not found load the default value and set into the file
            {
                theCurrentKey.currentValue=theCurrentKey.defaultValue;
                //to switch default value if is unchanged
                //settings->setValue(key.first,theCurrentKey.defaultValue);
            }
            if(settings->status()!=QSettings::NoError)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Have writing error, switch to memory only options");
                #ifdef ULTRACOPIER_VERSION_PORTABLE
                ResourcesManager::resourcesManager->disableWritablePath();
                #endif // ULTRACOPIER_VERSION_PORTABLE
                currentBackend=Memory;
            }
        }
        GroupKeysList[groupName][key.first]=theCurrentKey;
        index++;
    }
    //if the backend is file, leave into the group
    if(currentBackend==File)
        settings->endGroup();
    return true;
}

/// \brief To remove option group to options, remove the widget need be do into the calling object
bool OptionEngine::removeOptionGroup(const std::string &groupName)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, groupName: "+groupName);
    if(GroupKeysList.erase(groupName)!=1)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName);
    return false;
}

/// \brief To get option value
std::string OptionEngine::getOptionValue(const std::string &groupName,const std::string &variableName) const
{
    if(GroupKeysList.find(groupName)!=GroupKeysList.cend())
    {
        const std::unordered_map<std::string,OptionEngineGroupKey> &optionEngineGroupKey=GroupKeysList.at(groupName);
        if(optionEngineGroupKey.find(variableName)!=optionEngineGroupKey.cend())
            return optionEngineGroupKey.at(variableName).currentValue;
        QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2")
                              .arg(QString::fromStdString(groupName))
                              .arg(QString::fromStdString(variableName))
                              );
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName+", variableName: "+variableName);
        return std::string();
    }
    QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(QString::fromStdString(groupName)).arg(QString::fromStdString(variableName)));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,QString("The variable was not found: %1 %2")
                             .arg(QString::fromStdString(groupName))
                             .arg(QString::fromStdString(variableName))
                             .toStdString()
                             );
    //return default value
    return std::string();
}

/// \brief To set option value
void OptionEngine::setOptionValue(const std::string &groupName,const std::string &variableName,const std::string &value)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"groupName: \""+groupName+"\", variableName: \""+variableName+"\", value: \""+value+"\"");

    if(GroupKeysList.find(groupName)!=GroupKeysList.cend())
    {
        const std::unordered_map<std::string,OptionEngineGroupKey> &group=GroupKeysList.at(groupName);
        if(group.find(variableName)!=group.cend())
        {
            //prevent re-write the same value into the variable
            if(group.at(variableName).currentValue==value)
                return;
            //write ONLY the new value
            GroupKeysList[groupName][variableName].currentValue=value;
            if(currentBackend==File)
            {
                settings->beginGroup(QString::fromStdString(groupName));
                settings->setValue(QString::fromStdString(variableName),QString::fromStdString(value));
                settings->endGroup();
                if(settings->status()!=QSettings::NoError)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Have writing error, switch to memory only options");
                    #ifdef ULTRACOPIER_VERSION_PORTABLE
                    ResourcesManager::resourcesManager->disableWritablePath();
                    #endif // ULTRACOPIER_VERSION_PORTABLE
                    currentBackend=Memory;
                }
            }
            emit newOptionValue(groupName,variableName,value);
            return;
        }
        QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(QString::fromStdString(groupName)).arg(QString::fromStdString(variableName)));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"value not found, internal bug, groupName: "+groupName+", variableName: "+variableName);
        return;
    }
    QMessageBox::critical(NULL,"Internal error",tr("The variable was not found: %1 %2").arg(QString::fromStdString(groupName)).arg(QString::fromStdString(variableName)));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"The variable was not found: "+groupName+" "+variableName);
}

//the reset of right value of widget need be do into the calling object
void OptionEngine::internal_resetToDefaultValue()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start");

    for(auto& n:GroupKeysList)
    {
        const std::string &firstKey=n.first;
        for(auto& m:n.second)
        {
            const std::string &secondKey=m.first;
            OptionEngineGroupKey &o=m.second;
            if(o.currentValue!=o.defaultValue)
            {
                o.currentValue=o.defaultValue;

                if(currentBackend==File)
                {
                    settings->beginGroup(QString::fromStdString(firstKey));
                    settings->remove(QString::fromStdString(secondKey));
                    settings->endGroup();
                    if(settings->status()!=QSettings::NoError)
                    {
                        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Have writing error, switch to memory only options");
                        #ifdef ULTRACOPIER_VERSION_PORTABLE
                        ResourcesManager::resourcesManager->disableWritablePath();
                        #endif // ULTRACOPIER_VERSION_PORTABLE
                        currentBackend=Memory;
                    }
                }
                emit newOptionValue(firstKey,secondKey,o.currentValue);
            }
        }
    }
}

void OptionEngine::queryResetOptions()
{
    emit resetOptions();
}
