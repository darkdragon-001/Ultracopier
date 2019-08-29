/** \file ThemesManager.h
\brief Define the class to manage and load the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef THEMES_MANAGER_H
#define THEMES_MANAGER_H

#include <QString>
#include <QObject>
#include <QIcon>
#include <QList>
#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
#include <QPluginLoader>
#endif

#include "Environment.h"
#include "ResourcesManager.h"
#include "OptionEngine.h"
#include "PluginsManager.h"
#include "LanguagesManager.h"
#include "LocalPluginOptions.h"
#include "FacilityEngine.h"

#include "interface/PluginInterface_Themes.h"

/** \brief Define the class to manage and load the themes

This class provide a core load and manage the themes */
class ThemesManager : public QObject
{
    Q_OBJECT
    //public slots:
        /*/// \brief To change the current themes selected
        bool changeCurrentTheme(QString theNewThemeToLoad);*/
    public:
        /** \brief To get image into the current themes, or default if not found
        \param filePath The file path to search, like toto.png resolved with the root of the current themes
        \see currentStylePath */
        QIcon loadIcon(const std::string &fileName);
        /** \brief To get if one themes instance
        \see Core() */
        PluginInterface_Themes * getThemesInstance();

        static ThemesManager *themesManager;
        /// \brief Create the manager and load the defaults variables
        ThemesManager();
        /// \brief Destroy the themes manager
        ~ThemesManager();
    private:
        /// \brief The default themes path where it has theme's files
        std::string defaultStylePath;
        /// \brief The current themes path loaded by ultracopier
        std::string currentStylePath;
        /// \brief OptionEngineGroupKey then: Group -> Key
        struct PluginsAvailableThemes
        {
            PluginsAvailable plugin;
            PluginInterface_ThemesFactory *factory;
            #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE_DIRECT
            QPluginLoader *pluginLoader;
            #endif
            LocalPluginOptions *options;
        };
        std::vector<PluginsAvailableThemes> pluginList;
        int currentPluginIndex;//can bne -1
        bool stopIt;
    signals:
        /// \brief send this signal when the themes have changed
        void theThemeNeedBeUnloaded() const;
        void theThemeIsReloaded() const;
        void newThemeOptions(const std::string &name,QWidget *,const bool &isLoaded,const bool &havePlugin) const;
        void previouslyPluginAdded(PluginsAvailable) const;
    private slots:
        /// \brief reload the themes
        void onePluginAdded(const PluginsAvailable &plugin);
        #ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
        void onePluginWillBeRemoved(const PluginsAvailable &plugin);
        #endif
        void allPluginIsLoaded();
        void newOptionValue(const std::string &group,const std::string &name,const std::string &value);
        #ifdef ULTRACOPIER_DEBUG
        void debugInformation(const Ultracopier::DebugLevel &level, const std::string& fonction, const std::string& text, const std::string& file, const int& ligne);
        #endif // ULTRACOPIER_DEBUG
};

#endif // THEMES_MANAGER_H
