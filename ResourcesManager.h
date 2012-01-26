/** \file ResourcesManager.h
\brief Define the class to manage and load the resources linked with the themes
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef RESOURCES_MANAGER_H
#define RESOURCES_MANAGER_H

#include <QStringList>
#include <QString>
#include <QObject>

#include "Environment.h"
#include "Singleton.h"

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class ResourcesManager : public QObject, public Singleton<ResourcesManager>
{
	Q_OBJECT
	friend class Singleton<ResourcesManager>;
	private:
		/// \brief Create the manager and load the default variable
		ResourcesManager();
		/// \brief Destroy the resource manager
		~ResourcesManager();
	public:
		/** \brief Get folder presence and the path
		\return Empty QString if not found */
		QString getFolderReadPath(QString path);
		/** \brief Get folder presence, the path and check in the folder and sub-folder the file presence
		\return Empty QString if not found */
		QString getFolderReadPathMultiple(QString path,QStringList fileToCheck);
		bool checkFolderContent(QString path,QStringList fileToCheck);
		/// \brief add / or \ in function of the platform at the end of path if both / and \ are not found
		static QString AddSlashIfNeeded(QString path);
		/// \brief get the writable path
		QString getWritablePath();
		/// \brief disable the writable path, if ultracopier is unable to write into
		bool disableWritablePath();
		/// \brief get the read path
		QStringList getReadPath();
		/// \brief remove folder
		static bool removeFolder(QString dir);
	private:
		/// \brief List of the path to read only access
		QStringList searchPath;
		/// \brief The writable path, empty if not found
		QString writablePath;
		//temp variable
		int index,loop_size;
};

#endif // RESOURCES_MANAGER_H
