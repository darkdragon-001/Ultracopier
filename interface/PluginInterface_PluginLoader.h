/** \file PluginInterface_PluginLoader.h
\brief Define the interface of the plugin of type: plugin loader
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef PLUGININTERFACE_PLUGINLOADER_H
#define PLUGININTERFACE_PLUGINLOADER_H

#include <string>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

/** \brief To define the interface between Ultracopier and the plugin loader
 * */
class PluginInterface_PluginLoader : public QObject
{
    Q_OBJECT
    public:
        /// \brief try enable/disable the catching
        virtual void setEnabled(const bool &enabled) = 0;
        /// \brief to set resources, writePath can be empty if read only mode
        virtual void setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,const bool &portableVersion) = 0;
        /// \brief to get the options widget, NULL if not have
        virtual QWidget * options() = 0;
    public slots:
        /// \brief to reload the translation, because the new language have been loaded
        virtual void newLanguageLoaded() = 0;
    // signal to implement
    signals:
        void newState(const Ultracopier::CatchState &catchstate) const;
        /// \brief To debug source
        void debugInformation(const Ultracopier::DebugLevel &level,const std::string &fonction,const std::string &text,const std::string &file,const int &ligne) const;
};

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
Q_DECLARE_INTERFACE(PluginInterface_PluginLoader,"first-world.info.ultracopier.PluginInterface.PluginLoader/1.2.4.0");
#endif

#endif // PLUGININTERFACE_PLUGINLOADER_H
