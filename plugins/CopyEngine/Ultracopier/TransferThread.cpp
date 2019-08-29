//presume bug linked as multple paralelle inode to resume after "overwrite"
//then do overwrite node function to not re-set the file name

#include "TransferThread.h"
#ifdef Q_OS_WIN32
#include <windows.h>
#endif

#ifdef Q_OS_WIN32
    #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
        #ifndef NOMINMAX
            #define NOMINMAX
        #endif
        #include <windows.h>
    #endif
#endif

#ifdef Q_OS_WIN32
#define CURRENTSEPARATOR "\\"
#else
#define CURRENTSEPARATOR "/"
#endif

#include "../../../cpp11addition.h"

TransferThread::TransferThread() :
    haveStartTime                   (false),
    transfer_stat                   (TransferStat_Idle),
    doRightTransfer                 (false),
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    rsync                           (false),
    #endif
    stopIt                          (false),
    fileExistsAction                (FileExists_NotSet),
    alwaysDoFileExistsAction        (FileExists_NotSet),
    needSkip                        (false),
    needRemove                      (false),
    deletePartiallyTransferredFiles (true),
    writeError                      (false),
    readError                       (false),
    renameTheOriginalDestination    (false),
    havePermission                  (false)
{
    start();
    moveToThread(this);
    readThread.setWriteThread(&writeThread);
    source.setCaching(false);
    destination.setCaching(false);
    renameRegex=std::regex("^(.*)(\\.[a-zA-Z0-9]+)$");
    #ifdef Q_OS_WIN32
        #ifndef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
            regRead=std::regex("^[a-zA-Z]:");
        #endif
    #endif

    minTime=QDateTime(QDate(ULTRACOPIER_PLUGIN_MINIMALYEAR,1,1));
}

TransferThread::~TransferThread()
{
    stopIt=true;
    readThread.exit();
    readThread.wait();
    writeThread.exit();
    writeThread.wait();
    exit();
    //else cash without this disconnect
    //disconnect(&readThread);
    //disconnect(&writeThread);
    wait();
}

void TransferThread::run()
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] start: ")+QString::number((qint64)QThread::currentThreadId())));
    transfer_stat			= TransferStat_Idle;
    stopIt			= false;
    fileExistsAction	= FileExists_NotSet;
    alwaysDoFileExistsAction= FileExists_NotSet;
    //the error push
    connect(&readThread,&ReadThread::error,                     this,					&TransferThread::getReadError,      	Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::error,                   this,					&TransferThread::getWriteError,         Qt::QueuedConnection);
    //the thread change operation
    connect(this,&TransferThread::internalStartPreOperation,	this,					&TransferThread::preOperation,          Qt::QueuedConnection);
    connect(this,&TransferThread::internalStartPostOperation,	this,					&TransferThread::postOperation,         Qt::QueuedConnection);
    //the state change operation
    connect(&readThread,&ReadThread::opened,                    this,					&TransferThread::readIsReady,           Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::opened,                  this,					&TransferThread::writeIsReady,          Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,             this,					&TransferThread::readIsStopped,         Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::writeIsStopped,          this,                   &TransferThread::writeIsStopped,		Qt::QueuedConnection);
    connect(&readThread,&ReadThread::readIsStopped,             &writeThread,			&WriteThread::endIsDetected,            Qt::QueuedConnection);
    connect(&readThread,&ReadThread::closed,                    this,					&TransferThread::readIsClosed,          Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::closed,                  this,					&TransferThread::writeIsClosed,         Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::reopened,                this,					&TransferThread::writeThreadIsReopened,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::checksumFinish,            this,					&TransferThread::readChecksumFinish,	Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::checksumFinish,          this,					&TransferThread::writeChecksumFinish,	Qt::QueuedConnection);
    //error management
    connect(&readThread,&ReadThread::isSeekToZeroAndWait,       this,					&TransferThread::readThreadIsSeekToZeroAndWait,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAtTheLastPosition,this,	&TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(&readThread,&ReadThread::resumeAfterErrorByRestartAll,&writeThread,         &WriteThread::flushAndSeekToZero,               Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::flushedAndSeekedToZero,  this,                   &TransferThread::readThreadResumeAfterError,	Qt::QueuedConnection);
    connect(this,&TransferThread::internalTryStartTheTransfer,	this,					&TransferThread::internalStartTheTransfer,      Qt::QueuedConnection);

    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(&readThread,&ReadThread::debugInformation,          this,                   &TransferThread::debugInformation,  Qt::QueuedConnection);
    connect(&writeThread,&WriteThread::debugInformation,        this,                   &TransferThread::debugInformation,  Qt::QueuedConnection);
    connect(&driveManagement,&DriveManagement::debugInformation,this,                   &TransferThread::debugInformation,	Qt::QueuedConnection);
    #endif

    exec();
}

TransferStat TransferThread::getStat() const
{
    return transfer_stat;
}

void TransferThread::startTheTransfer()
{
    emit internalTryStartTheTransfer();
}

void TransferThread::internalStartTheTransfer()
{
    if(transfer_stat==TransferStat_Idle)
    {
        if(mode!=Ultracopier::Move)
        {
            /// \bug can pass here because in case of direct move on same media, it return to idle stat directly
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at idle"));
        }
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at PostOperation"));
        return;
    }
    if(transfer_stat==TransferStat_Transfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't start transfert at Transfer"));
        return;
    }
    if(canStartTransfer)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] canStartTransfer is already set to true"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] check how start the transfer"));
    canStartTransfer=true;
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start directly the transfer"));
        ifCanStartTransfer();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] start the transfer as delayed"));
}

bool TransferThread::setFiles(const QFileInfo& source, const int64_t &size, const QFileInfo& destination, const Ultracopier::CopyMode &mode)
{
    if(transfer_stat!=TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
        return false;
    }
    //to prevent multiple file alocation into ListThread::doNewActions_inode_manipulation()
    transfer_stat			= TransferStat_PreOperation;
    //emit pushStat(stat,transferId);
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start, source: "+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
    this->source                    = source;
    this->destination               = destination;
    this->mode                      = mode;
    this->size                      = size;
    stopIt                          = false;
    fileExistsAction                = FileExists_NotSet;
    canStartTransfer                = false;
    sended_state_preOperationStopped= false;
    canBeMovedDirectlyVariable      = false;
    canBeCopiedDirectlyVariable     = false;
    fileContentError                = false;
    real_doChecksum                 = false;
    writeError                      = false;
    writeError_source_seeked        = false;
    writeError_destination_reopened = false;
    readError                       = false;
    fileContentError                = false;
    resetExtraVariable();
    emit internalStartPreOperation();
    return true;
}

void TransferThread::setFileExistsAction(const FileExistsAction &action)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source.absoluteFilePath().toStdString()+(", destination: ")+destination.absoluteFilePath().toStdString());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+("] action: ")+std::to_string(action));
    if(action!=FileExists_Rename)
        fileExistsAction	= action;
    else
    {
        //always rename pass here
        fileExistsAction	= action;
        alwaysDoFileExistsAction=action;
    }
    if(action==FileExists_Skip)
    {
        skip();
        return;
    }
    resetExtraVariable();
    emit internalStartPreOperation();
}

void TransferThread::setFileRename(const std::string &nameForRename)
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source.absoluteFilePath().toStdString()+(", destination: ")+destination.absoluteFilePath().toStdString());
        return;
    }
    if(QString::fromStdString(nameForRename).contains(QRegularExpression(QStringLiteral("[/\\\\\\*]"))))
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't use this kind of name, internal error"));
        emit errorOnFile(destination,tr("Try rename with using special characters").toStdString());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] nameForRename: "+nameForRename);
    if(!renameTheOriginalDestination)
        destination.setFile(destination.absolutePath()+CURRENTSEPARATOR+QString::fromStdString(nameForRename));
    else
    {
        QString tempDestination=destination.absoluteFilePath();
        QFile destinationFile(tempDestination);
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"["+std::to_string(id)+"] "+QStringLiteral("rename %1: to: %2").arg(destination.absoluteFilePath()).arg(destination.absolutePath()+CURRENTSEPARATOR+QString::fromStdString(nameForRename)).toStdString());
        if(!destinationFile.rename(destination.absolutePath()+CURRENTSEPARATOR+QString::fromStdString(nameForRename)))
        {
            if(!destinationFile.exists())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("source not exists %1: destination: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()).toStdString());
                emit errorOnFile(destinationFile,tr("File not found").toStdString());
                return;
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("unable to do real move %1: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()).toStdString());
            emit errorOnFile(destinationFile,destinationFile.errorString().toStdString());
            return;
        }
        if(source.absoluteFilePath()==destination.absoluteFilePath())
            source.setFile(destination.absolutePath()+CURRENTSEPARATOR+QString::fromStdString(nameForRename));
        destination.setFile(tempDestination);
        destination.refresh();
    }
    fileExistsAction	= FileExists_NotSet;
    resetExtraVariable();
    emit internalStartPreOperation();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] destination is: "+destination.absoluteFilePath().toStdString());
}

void TransferThread::setAlwaysFileExistsAction(const FileExistsAction &action)
{
    //ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+QStringLiteral("] action to do always: ")+QString::number(action)));
    alwaysDoFileExistsAction=action;
}

void TransferThread::resetExtraVariable()
{
    sended_state_preOperationStopped=false;
    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsReadyVariable         = false;
    writeIsReadyVariable		= false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    needRemove                  = false;
    needSkip                    = false;
    retry                       = false;
    readIsOpenVariable          = false;
    writeIsOpenVariable         = false;
    readIsOpeningVariable       = false;
    writeIsOpeningVariable      = false;
    havePermission              = false;
}

void TransferThread::preOperation()
{
    if(transfer_stat!=TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] already used, source: ")+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
        return;
    }
    haveStartTime=true;
    startTransferTime.restart();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start: source: "+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
    needRemove=false;
    if(isSame())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same "+source.absoluteFilePath().toStdString()+" than "+destination.absoluteFilePath().toStdString());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after is same");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before destination exists");
    if(destinationExists())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] destination exists: "+destination.absoluteFilePath().toStdString());
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] after destination exists");
    /*Why this code?
    if(readError)
    {
        readError=false;
        return;
    }*/
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] before keep date");
    #ifdef Q_OS_WIN32
    doTheDateTransfer=!source.isSymLink();
    #else
    doTheDateTransfer=true;
    #endif
    if(doTheDateTransfer)
    {
        if(source.lastModified()<minTime)
        {
            if(/*true when the destination have been remove but not the symlink:*/source.isSymLink())
                doTheDateTransfer=false;
            else
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"the sources is older to copy the time: "+source.absoluteFilePath().toStdString()+": "+minTime.toString(QStringLiteral("dd.MM.yyyy hh:mm:ss.zzz")).toStdString()+">="+source.lastModified().toString(QStringLiteral("dd.MM.yyyy hh:mm:ss.zzz")).toStdString());
                doTheDateTransfer=false;
                if(keepDate)
                {
                    emit errorOnFile(source,tr("Wrong modification date or unable to get it, you can disable time transfer to do it").toStdString());
                    return;
                }
            }
        }
        else
        {
            doTheDateTransfer=readFileDateTime(source);
            #ifdef Q_OS_MAC
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] read the source time: "+std::to_string(butime.modtime));
            #endif
            if(!doTheDateTransfer)
            {
                //will have the real error at source open
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unable to read the source time: "+source.absoluteFilePath().toStdString());
                if(keepDate)
                {
                    emit errorOnFile(source,tr("Wrong modification date or unable to get it, you can disable time transfer to do it").toStdString());
                    return;
                }
            }
        }
    }
    if(canBeMovedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("need moved directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());
        canBeMovedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    if(canBeCopiedDirectly())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("need copied directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());
        canBeCopiedDirectlyVariable=true;
        readThread.fakeOpen();
        writeThread.fakeOpen();
        return;
    }
    tryOpen();
}

void TransferThread::tryOpen()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source and destination: "+source.absoluteFilePath().toStdString()+" and "+destination.absoluteFilePath().toStdString());
    TransferAlgorithm transferAlgorithm=this->transferAlgorithm;
    if(transferAlgorithm==TransferAlgorithm_Automatic)
    {
        #ifdef Q_OS_LINUX
        if(driveManagement.isSameDrive(destination.absoluteFilePath().toStdString(),source.absoluteFilePath().toStdString()))
        {
            const QByteArray &type=driveManagement.getDriveType(driveManagement.getDrive(source.absoluteFilePath().toStdString()));
            if(type=="nfs" || type=="smb")
                transferAlgorithm=TransferAlgorithm_Parallel;
            else
                transferAlgorithm=TransferAlgorithm_Sequential;
        }
        else
        #endif
            transferAlgorithm=TransferAlgorithm_Parallel;
    }
    if(!readIsOpenVariable)
    {
        if(!readIsOpeningVariable)
        {
            readError=false;
            readThread.open(source.absoluteFilePath(),mode);
            readIsOpeningVariable=true;

            if(doRightTransfer)
                havePermission=readFilePermissions(QFile(source.absoluteFilePath()));
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] readIsOpeningVariable is true when try open");
            emit errorOnFile(source,tr("Internal error: Already opening").toStdString());
            readError=true;
            return;
        }
    }
    if(!writeIsOpenVariable)
    {
        if(!writeIsOpeningVariable)
        {
            if(transferAlgorithm==TransferAlgorithm_Sequential)
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transferAlgorithm==TransferAlgorithm_Sequential");
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transferAlgorithm==TransferAlgorithm_Parallel");
            writeError=false;
            if(transferAlgorithm==TransferAlgorithm_Sequential)
                writeThread.open(destination.absoluteFilePath(),size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)),sequentialBuffer,true);
            else
                writeThread.open(destination.absoluteFilePath(),size,osBuffer && (!osBufferLimited || (osBufferLimited && size<osBufferLimit)),parallelBuffer,false);
            writeIsOpeningVariable=true;
        }
        else
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+"writeIsOpeningVariable is true when try open");
            emit errorOnFile(destination,tr("Internal error: Already opening").toStdString());
            writeError=true;
            return;
        }
    }
}

bool TransferThread::isSame()
{
    //check if source and destination is not the same
    //source.absoluteFilePath()==destination.absoluteFilePath() not work is source don't exists
    if(source.absoluteFilePath()==destination.absoluteFilePath())
    {
        #ifdef ULTRACOPIER_PLUGIN_DEBUG
        if(!source.exists())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+source.absoluteFilePath().toStdString()+" not exists");
        if(!source.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+source.absoluteFilePath().toStdString()+" isSymLink");
        if(!destination.isSymLink())
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start source: "+destination.absoluteFilePath().toStdString()+" isSymLink");
        #endif
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is same but skip");
            transfer_stat=TransferStat_Idle;
            emit postOperationStopped();
            //quit
            return true;
        }
        if(checkAlwaysRename())
            return false;
        emit fileAlreadyExists(source,destination,true);
        return true;
    }
    return false;
}

bool TransferThread::destinationExists()
{
    //check if destination exists
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("overwrite: %1, alwaysDoFileExistsAction: %2, readError: %3, writeError: %4")
                             .arg(fileExistsAction)
                             .arg(alwaysDoFileExistsAction)
                             .arg(readError)
                             .arg(writeError)
                             .toStdString()
                             );
    if(alwaysDoFileExistsAction==FileExists_Overwrite || readError || writeError
            #ifdef ULTRACOPIER_PLUGIN_RSYNC
            || rsync
            #endif
            )
        return false;
    bool destinationExists;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] time to first FS access");
    destination.refresh();
    destinationExists=destination.exists();
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] finish first FS access");
    if(destinationExists)
    {
        if(fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_Skip)
        {
            transfer_stat=TransferStat_Idle;
            emit postOperationStopped();
            //quit
            return true;
        }
        if(checkAlwaysRename())
            return false;
        if(source.exists())
        {
            if(fileExistsAction==FileExists_OverwriteIfNewer || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNewer))
            {
                if(destination.lastModified()<source.lastModified())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
            if(fileExistsAction==FileExists_OverwriteIfOlder || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfOlder))
            {
                if(destination.lastModified()>source.lastModified())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
            if(fileExistsAction==FileExists_OverwriteIfNotSame || (fileExistsAction==FileExists_NotSet && alwaysDoFileExistsAction==FileExists_OverwriteIfNotSame))
            {
                if(destination.lastModified()!=source.lastModified() || destination.size()!=source.size())
                    return false;
                else
                {
                    transfer_stat=TransferStat_Idle;
                    emit postOperationStopped();
                    return true;
                }
            }
        }
        else
        {
            if(fileExistsAction!=FileExists_NotSet)
            {
                transfer_stat=TransferStat_Idle;
                emit postOperationStopped();
                return true;
            }
        }
        if(fileExistsAction==FileExists_NotSet)
        {
            emit fileAlreadyExists(source,destination,false);
            return true;
        }
    }
    return false;
}

std::string TransferThread::resolvedName(const QFileInfo &inode)
{
    QString fileName=inode.fileName();
    if(fileName.isEmpty())
    {
        QDir absoluteDir=inode.absoluteDir();
        fileName=absoluteDir.dirName();
        if(fileName.isEmpty())
        {
            fileName=absoluteDir.cdUp();
            fileName=absoluteDir.dirName();
        }
    }
    #ifdef Q_OS_WIN32
    if(fileName.isEmpty())
    {
        fileName=inode.absolutePath();
        fileName.replace(QRegularExpression(QStringLiteral("^([a-zA-Z]+):.*$")),QStringLiteral("\\1"));
        if(inode.absolutePath().contains(QRegularExpression(QStringLiteral("^[a-zA-Z]+:[/\\\\]?$"))))
            fileName=tr("Drive %1").arg(fileName);
        else
            fileName=tr("Unknown folder");
    }
    #else
    if(fileName.isEmpty())
        fileName=tr("root");
    #endif
    return fileName.toStdString();
}

std::string TransferThread::getSourcePath() const
{
    return source.absoluteFilePath().toStdString();
}

std::string TransferThread::getDestinationPath() const
{
    return destination.absoluteFilePath().toStdString();
}

QFileInfo TransferThread::getSourceInode() const
{
    return source;
}

QFileInfo TransferThread::getDestinationInode() const
{
    return destination;
}

Ultracopier::CopyMode TransferThread::getMode() const
{
    return mode;
}

//return true if has been renamed
bool TransferThread::checkAlwaysRename()
{
    if(alwaysDoFileExistsAction==FileExists_Rename)
    {
        QFileInfo newDestination=destination;
        std::string fileName=resolvedName(newDestination);
        std::string suffix;
        std::string newFileName;
        //resolv the suffix
        if(std::regex_match(fileName,renameRegex))
        {
            suffix=fileName;
            suffix=std::regex_replace(suffix,renameRegex,"$2");
            fileName=std::regex_replace(fileName,renameRegex,"$1");
        }
        //resolv the new name
        int num=1;
        do
        {
            if(num==1)
            {
                if(firstRenamingRule.empty())
                    newFileName=tr("%name% - copy").toStdString();
                else
                    newFileName=firstRenamingRule;
            }
            else
            {
                if(otherRenamingRule.empty())
                    newFileName=tr("%name% - copy (%number%)").toStdString();
                else
                    newFileName=otherRenamingRule;
                stringreplaceAll(newFileName,"%number%",std::to_string(num));
            }
            stringreplaceAll(newFileName,"%name%",fileName);
            stringreplaceAll(newFileName,"%suffix%",suffix);
            newDestination.setFile(newDestination.absolutePath()+CURRENTSEPARATOR+QString::fromStdString(newFileName));
            num++;
        }
        while(newDestination.exists());
        if(!renameTheOriginalDestination)
            destination=newDestination;
        else
        {
            QFile destinationFile(destination.absoluteFilePath());
            if(!destinationFile.rename(newDestination.absoluteFilePath()))
            {
                if(!destinationFile.exists())
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("source not exists %1: destination: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()).toStdString());
                    emit errorOnFile(destinationFile,tr("File not found").toStdString());
                    readError=true;
                    return true;
                }
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("unable to do real move %1: %2, error: %3").arg(destinationFile.fileName()).arg(destinationFile.fileName()).arg(destinationFile.errorString()).toStdString());
                readError=true;
                emit errorOnFile(destinationFile,destinationFile.errorString().toStdString());
                return true;
            }
        }
        return true;
    }
    return false;
}

void TransferThread::tryMoveDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("need moved directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(source.absoluteFilePath());
    QFile destinationFile(destination.absoluteFilePath());
    #ifndef Q_OS_WIN32
    if(destinationFile.exists() || destination.isSymLink())
    {
        if(!sourceFile.exists() && !source.isSymLink())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destinationFile.fileName().toStdString()+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source file doesn't exist").toStdString());
            return;
        }
        else if(!destinationFile.remove())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destinationFile.fileName().toStdString()+", error: "+destinationFile.errorString().toStdString());
            readError=true;
            emit errorOnFile(destination,destinationFile.errorString().toStdString());
            return;
        }
    }
    #endif
    QDir dir(destination.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destination.absolutePath());
        mkpathTransfer->release();
    }
    #ifdef Q_OS_WIN32
    //if(!sourceFile.copy(destinationFile.fileName()))
    if(MoveFileEx(
            reinterpret_cast<const wchar_t*>(sourceFile.fileName().utf16()),
            reinterpret_cast<const wchar_t*>(destinationFile.fileName().utf16()),
            MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING
        )==0)
    #else
    if(!sourceFile.rename(destinationFile.fileName()))
    #endif
    {
        readError=true;
        if(!sourceFile.exists() && !source.isSymLink())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("source not exists %1: destination: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).toStdString());
            emit errorOnFile(sourceFile,tr("File not found").toStdString());
            return;
        }
        else if(!dir.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("destination folder not exists %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).toStdString());
            emit errorOnFile(destination.absolutePath(),tr("Unable to do the folder").toStdString());
            return;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("unable to do real move %1: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).toStdString());
        emit errorOnFile(sourceFile,sourceFile.errorString().toStdString());
        return;
    }
    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

void TransferThread::tryCopyDirectly()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("need copied directly: %1 to %2").arg(source.absoluteFilePath()).arg(destination.absoluteFilePath()).toStdString());

    sended_state_readStopped	= false;
    sended_state_writeStopped	= false;
    writeError                  = false;
    readError                   = false;
    readIsFinishVariable		= false;
    writeIsFinishVariable		= false;
    readIsClosedVariable		= false;
    writeIsClosedVariable		= false;
    //move if on same mount point
    QFile sourceFile(source.absoluteFilePath());
    QFile destinationFile(destination.absoluteFilePath());
    #ifndef Q_OS_WIN32
    if(destinationFile.exists() || destination.isSymLink())
    {
        if(!sourceFile.exists() && !source.isSymLink())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destinationFile.fileName().toStdString()+", source not exists");
            readError=true;
            emit errorOnFile(destination,tr("The source doesn't exist").toStdString());
            return;
        }
        else if(!destinationFile.remove())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+destinationFile.fileName().toStdString()+", error: "+destinationFile.errorString().toStdString());
            readError=true;
            emit errorOnFile(destination,destinationFile.errorString().toStdString());
            return;
        }
    }
    #endif
    QDir dir(destination.absolutePath());
    {
        mkpathTransfer->acquire();
        if(!dir.exists())
            dir.mkpath(destination.absolutePath());
        mkpathTransfer->release();
    }
    /** on windows, symLink is normal file, can be copied
     * on unix not, should be created **/
    #ifdef Q_OS_WIN32
    //if(!sourceFile.copy(destinationFile.fileName()))
    if(CopyFileEx(
                reinterpret_cast<const wchar_t*>(sourceFile.fileName().utf16()),
                reinterpret_cast<const wchar_t*>(destinationFile.fileName().utf16()),
            NULL,
            NULL,
            FALSE,
            0
        )==0)
    #else
    if(!QFile::link(sourceFile.symLinkTarget(),destinationFile.fileName()))
    #endif
    {
        readError=true;
        if(!sourceFile.exists() && !source.isSymLink())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("source not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()).toStdString());
            emit errorOnFile(sourceFile,tr("The source file doesn't exist").toStdString());
            return;
        }
        else if(destinationFile.exists() || destination.isSymLink())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("destination already exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()).toStdString());
            emit errorOnFile(sourceFile,tr("Another file exists at same place").toStdString());
            return;
        }
        else if(!dir.exists())
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("destination folder not exists %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()).toStdString());
            emit errorOnFile(sourceFile,tr("Unable to do the folder").toStdString());
            return;
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] "+QStringLiteral("unable to do sym link copy %1 -> %4: %2, error: %3").arg(sourceFile.fileName()).arg(destinationFile.fileName()).arg(sourceFile.errorString()).arg(sourceFile.symLinkTarget()).toStdString());
        emit errorOnFile(sourceFile,sourceFile.errorString().toStdString());
        return;
    }
    readThread.fakeReadIsStarted();
    writeThread.fakeWriteIsStarted();
    readThread.fakeReadIsStopped();
    writeThread.fakeWriteIsStopped();
}

bool TransferThread::canBeMovedDirectly() const
{
    if(mode!=Ultracopier::Move)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] mode!=Ultracopier::Move");
        return false;
    }
    return source.isSymLink() || driveManagement.isSameDrive(destination.absoluteFilePath().toStdString(),source.absoluteFilePath().toStdString());
}

bool TransferThread::canBeCopiedDirectly() const
{
    return source.isSymLink();
}

void TransferThread::readIsReady()
{
    if(readIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsReadyVariable=true;
    readIsOpenVariable=true;
    readIsClosedVariable=false;
    readIsOpeningVariable=false;
    ifCanStartTransfer();
}

void TransferThread::ifCanStartTransfer()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readIsReadyVariable: "+std::to_string(readIsReadyVariable)+", writeIsReadyVariable: "+std::to_string(writeIsReadyVariable));
    if(readIsReadyVariable && writeIsReadyVariable)
    {
        transfer_stat=TransferStat_WaitForTheTransfer;
        sended_state_readStopped	= false;
        sended_state_writeStopped	= false;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stat=WaitForTheTransfer");
        if(!sended_state_preOperationStopped)
        {
            sended_state_preOperationStopped=true;
            emit preOperationStopped();
        }
        if(canStartTransfer)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] stat=Transfer, "+QStringLiteral("canBeMovedDirectlyVariable: %1, canBeCopiedDirectlyVariable: %2").arg(canBeMovedDirectlyVariable).arg(canBeCopiedDirectlyVariable).toStdString());
            transfer_stat=TransferStat_Transfer;
            if(canBeMovedDirectlyVariable)
                tryMoveDirectly();
            else if(canBeCopiedDirectlyVariable)
                tryCopyDirectly();
            else
            {
                needRemove=deletePartiallyTransferredFiles;
                readThread.startRead();
            }
            emit pushStat(transfer_stat,transferId);
        }
        //else
            //emit pushStat(stat,transferId);
    }
}

void TransferThread::writeIsReady()
{
    if(writeIsReadyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsReadyVariable=true;
    writeIsOpenVariable=true;
    writeIsClosedVariable=false;
    writeIsOpeningVariable=false;
    ifCanStartTransfer();
}


//set the copy info and options before runing
void TransferThread::setRightTransfer(const bool doRightTransfer)
{
    this->doRightTransfer=doRightTransfer;
}

//set keep date
void TransferThread::setKeepDate(const bool keepDate)
{
    this->keepDate=keepDate;
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
//set the current max speed in KB/s
void TransferThread::setMultiForBigSpeed(const int &multiForBigSpeed)
{
    readThread.setMultiForBigSpeed(multiForBigSpeed);
    writeThread.setMultiForBigSpeed(multiForBigSpeed);
}
#endif

//set block size in Bytes
bool TransferThread::setBlockSize(const unsigned int blockSize)
{
    bool read=readThread.setBlockSize(blockSize);
    bool write=writeThread.setBlockSize(blockSize);
    return (read && write);
}

//pause the copy
void TransferThread::pause()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] wrong stat to put in pause");
        return;
    }
    haveStartTime=false;
    readThread.pause();
    writeThread.pause();
}

//resume the copy
void TransferThread::resume()
{
    //only pause/resume during the transfer of file data
    //from transfer_stat!=TransferStat_Idle because it resume at wrong order
    if(transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] wrong stat to put in pause");
        return;
    }
    readThread.resume();
    writeThread.resume();
}

//stop the current copy
void TransferThread::stop()
{
    stopIt=true;
    haveStartTime=false;
    if(transfer_stat==TransferStat_Idle)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        return;
    }
    if(remainSourceOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"remainSourceOpen()");
        readThread.stop();
    }
    if(remainDestinationOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"remainDestinationOpen()");
        writeThread.stop();
    }
    if(!remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"transfer_stat==TransferStat_Idle");
        if(needRemove && source.absoluteFilePath()!=destination.absoluteFilePath())
        {
            if(source.exists())
                QFile(destination.absoluteFilePath()).remove();
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination when the source don't exists"));
        }
        transfer_stat=TransferStat_PostOperation;
        emit internalStartPostOperation();
    }
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("transfer_stat==%1 && remainFileOpen()").arg(transfer_stat).toStdString());
}

bool TransferThread::remainFileOpen() const
{
    return remainSourceOpen() || remainDestinationOpen();
}

bool TransferThread::remainSourceOpen() const
{
    return (readIsOpenVariable || readIsOpeningVariable) && !readIsClosedVariable;
}

bool TransferThread::remainDestinationOpen() const
{
    return (writeIsOpenVariable || writeIsOpeningVariable) && !writeIsClosedVariable;
}

void TransferThread::readIsFinish()
{
    if(readIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] double event dropped"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsFinishVariable=true;
    canStartTransfer=false;
    //check here if need start checksuming or not
    real_doChecksum=doChecksum && (!checksumOnlyOnError || fileContentError) && (!canBeMovedDirectlyVariable && !canBeCopiedDirectlyVariable);
    if(real_doChecksum)
    {
        readIsFinishVariable=false;
        transfer_stat=TransferStat_Checksum;
        sourceChecksum=QByteArray();
        destinationChecksum=QByteArray();
        readThread.startCheckSum();
    }
    else
    {
        transfer_stat=TransferStat_PostTransfer;
        if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then readIsClosed() already call
            readThread.postOperation();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] in skip, don't start postOperation");
    }
    emit pushStat(transfer_stat,transferId);
}

void TransferThread::writeIsFinish()
{
    if(writeIsFinishVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsFinishVariable=true;
    //check here if need start checksuming or not
    if(real_doChecksum)
    {
        writeIsFinishVariable=false;
        transfer_stat=TransferStat_Checksum;
        writeThread.startCheckSum();
    }
    else
    {
        if(!needSkip || (canBeCopiedDirectlyVariable || canBeMovedDirectlyVariable))//if skip, stop call, then writeIsClosed() already call
            writeThread.postOperation();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] in skip, don't start postOperation");
    }
}

void TransferThread::readChecksumFinish(const QByteArray& checksum)
{
    sourceChecksum=checksum;
    compareChecksum();
}

void TransferThread::writeChecksumFinish(const QByteArray& checksum)
{
    destinationChecksum=checksum;
    compareChecksum();
}

void TransferThread::compareChecksum()
{
    if(sourceChecksum.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] the checksum of source is missing");
        return;
    }
    if(destinationChecksum.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] the checksum of destination is missing");
        return;
    }
    if(sourceChecksum==destinationChecksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] the checksum match");
        readThread.postOperation();
        writeThread.postOperation();
        transfer_stat=TransferStat_PostTransfer;
        emit pushStat(transfer_stat,transferId);
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] the checksum not match"));
        //emit error here, and wait to resume
        emit errorOnFile(destination,tr("The checksums do not match").toStdString());
    }
}

void TransferThread::readIsClosed()
{
    if(readIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] double event dropped"));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readIsClosedVariable=true;
    readIsOpeningVariable=false;
    checkIfAllIsClosedAndDoOperations();
}

void TransferThread::writeIsClosed()
{
    if(writeIsClosedVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    writeIsClosedVariable=true;
    writeIsOpeningVariable=false;
    if(stopIt && needRemove && source.absoluteFilePath()!=destination.absoluteFilePath())
    {
        if(source.exists())
            QFile(destination.absoluteFilePath()).remove();
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try destroy the destination when the source don't exists"));
    }
    checkIfAllIsClosedAndDoOperations();
}

// return true if all is closed, and do some operations, don't use into condition to check if is closed!
bool TransferThread::checkIfAllIsClosedAndDoOperations()
{
    if((readError || writeError) && !needSkip && !stopIt)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resolve error before progress");
        return false;
    }
    if(!remainFileOpen())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit internalStartPostOperation() to do the real post operation");
        transfer_stat=TransferStat_PostOperation;
        //emit pushStat(stat,transferId);
        emit internalStartPostOperation();
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] "+QStringLiteral("wait self close: readIsReadyVariable: %1, readIsClosedVariable: %2, writeIsReadyVariable: %3, writeIsClosedVariable: %4")
                     .arg(readIsReadyVariable)
                     .arg(readIsClosedVariable)
                     .arg(writeIsReadyVariable)
                     .arg(writeIsClosedVariable)
                                 .toStdString()
                     );
        return false;
    }
}

/// \todo found way to retry that's
/// \todo the rights copy
void TransferThread::postOperation()
{
    if(transfer_stat!=TransferStat_PostOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+"] need be in transfer, source: "+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString()+", stat:"+std::to_string(transfer_stat));
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    //all except closing
    if((readError || writeError) && !needSkip && !stopIt)//normally useless by checkIfAllIsFinish()
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] resume after error");
        return;
    }

    if(!needSkip && !stopIt)
    {
        if(!canBeCopiedDirectlyVariable && !canBeMovedDirectlyVariable)
        {
            if(writeIsOpenVariable && !writeIsClosedVariable)
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] can't pass in post operation if write is not closed"));
                emit errorOnFile(destination,tr("Internal error: The destination is not closed").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(readThread.getLastGoodPosition()!=writeThread.getLastGoodPosition())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+QString("] readThread.getLastGoodPosition(%1)!=writeThread.getLastGoodPosition(%2)")
                                         .arg(readThread.getLastGoodPosition())
                                         .arg(writeThread.getLastGoodPosition())
                                         .toStdString()
                                         );
                emit errorOnFile(destination,tr("Internal error: The size transfered doesn't match").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            if(!writeThread.bufferIsEmpty())
            {
                writeThread.flushBuffer();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] buffer is not empty"));
                emit errorOnFile(destination,tr("Internal error: The buffer is not empty").toStdString());
                needSkip=false;
                if(deletePartiallyTransferredFiles)
                    needRemove=true;
                writeError=true;
                return;
            }
            //in normal mode, without copy/move syscall
            if(!doFilePostOperation())
                return;
        }

        //remove source in moving mode
        if(mode==Ultracopier::Move && !canBeMovedDirectlyVariable)
        {
            if(destination.exists() && destination.isFile())
            {
                QFile sourceFile(source.absoluteFilePath());
                if(!sourceFile.remove())
                {
                    needSkip=false;
                    emit errorOnFile(source,sourceFile.errorString().toStdString());
                    return;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] try remove source but destination not exists!"));
        }
    }
    else//do difference skip a file and skip this error case
    {
        if(needRemove && destination.exists() && source.exists() && source.absoluteFilePath()!=destination.absoluteFilePath() && destination.isFile())
        {
            QFile destinationFile(destination.absoluteFilePath());
            if(!destinationFile.remove())
            {
                //emit errorOnFile(source,destinationFile.errorString());
                //return;
            }
        }
        else
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] try remove destination but not exists!");
    }
    source.setFile(QStringLiteral(""));
    destination.setFile(QStringLiteral(""));
    //don't need remove because have correctly finish (it's not in: have started)
    needRemove=false;
    needSkip=false;
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit postOperationStopped()");
    transfer_stat=TransferStat_Idle;
    emit postOperationStopped();
}

bool TransferThread::doFilePostOperation()
{
    //do operation needed by copy
    //set the time if no write thread used

    destination.refresh();
    if(!destination.exists() && !destination.isSymLink())
    {
        if(!stopIt)
            if(/*true when the destination have been remove but not the symlink:*/!source.isSymLink())
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date: File not found");
                emit errorOnFile(destination,tr("Unable to change the date").toStdString()+": "+tr("File not found").toStdString());
                return false;
            }
    }
    else
    {
        if(doTheDateTransfer)
        {
            if(!writeFileDateTime(destination))
            {
                if(!destination.isFile())
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date (is not a file)");
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to change the date");
                /* error with virtual folder under windows */
                #ifndef Q_OS_WIN32
                if(keepDate)
                {
                    emit errorOnFile(destination,tr("Unable to change the date").toStdString());
                    return false;
                }
                #endif
            }
            else
            {
                #ifndef Q_OS_WIN32
                destination.refresh();
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] read the destination time: "+destination.lastModified().toString().toStdString());
                if(destination.lastModified()<minTime)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] read the destination time lower than min time: "+destination.lastModified().toString().toStdString());
                    if(keepDate)
                    {
                        emit errorOnFile(destination,tr("Unable to change the date").toStdString());
                        return false;
                    }
                }
                #endif
            }
        }
        if(doRightTransfer)
        {
            //should be never used but...
            /*source.refresh();
            if(source.exists())*/
            if(havePermission)
            {
                QFile destinationFile(destination.absoluteFilePath());
                if(!writeFilePermissions(destinationFile))
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Unable to set the destination file permission");
                    //emit errorOnFile(destination,tr("Unable to set the destination file permission"));
                    //return false;
                }
            }
            else
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] Try doRightTransfer when source not exists");
        }
    }
    if(stopIt)
        return false;

    return true;
}

//////////////////////////////////////////////////////////////////
/////////////////////// Error management /////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::getWriteError()
{
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in write error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError		= true;
    writeError			= true;
    writeIsReadyVariable		= false;
    writeError_source_seeked	= false;
    writeError_destination_reopened	= false;
    writeIsOpeningVariable=false;
    if(!readError)//already display error for the read
        emit errorOnFile(destination,writeThread.errorString());
}

void TransferThread::getReadError()
{
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] already in read error!");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    fileContentError	= true;
    readError		= true;
    //writeIsReadyVariable	= false;//wrong because write can be ready here
    readIsReadyVariable	= false;
    readIsOpeningVariable=false;
    if(!writeError)//already display error for the write
        emit errorOnFile(source,readThread.errorString());
}

//retry after error
void TransferThread::retryAfterError()
{
    /// \warning skip the resetExtraVariable(); to be more exact and resolv some bug
    if(transfer_stat==TransferStat_Idle)
    {
        if(transferId==0)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] seam have bug, source: ")+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
            return;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] restart all, source: "+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString());
        readError=false;
        //writeError=false;
        emit internalStartPreOperation();
        return;
    }
    //opening error
    if(transfer_stat==TransferStat_PreOperation)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] is not idle, source: "+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString()+", stat: "+std::to_string(transfer_stat));
        readError=false;
        //writeError=false;
        emit internalStartPreOperation();
        //tryOpen();-> recheck all, because can be an error into isSame(), rename(), ...
        return;
    }
    //data streaming error
    if(transfer_stat!=TransferStat_PostOperation && transfer_stat!=TransferStat_Transfer && transfer_stat!=TransferStat_PostTransfer && transfer_stat!=TransferStat_Checksum)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"["+std::to_string(id)+("] is not in right stat, source: ")+source.absoluteFilePath().toStdString()+", destination: "+destination.absoluteFilePath().toStdString()+", stat: "+std::to_string(transfer_stat));
        return;
    }
    if(transfer_stat==TransferStat_PostOperation)
    {
        if(readError || writeError)
        {
            readError=false;
            //writeError=false;
            resumeTransferAfterWriteError();
            writeThread.flushBuffer();
            transfer_stat=TransferStat_PreOperation;
            emit internalStartPreOperation();
            return;
        }
        emit internalStartPostOperation();
        return;
    }
    if(canBeMovedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retry the system move");
        tryMoveDirectly();
        return;
    }
    if(canBeCopiedDirectlyVariable)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retry the copy directly");
        tryCopyDirectly();
        return;
    }
    if(transfer_stat==TransferStat_Checksum)
    {
        if(writeError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the write error");
            writeThread.reopen();
        }
        else if(readError)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the read error");
            readThread.reopen();
        }
        else //only checksum difference
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] retry all the transfer");
            canStartTransfer=true;
            ifCanStartTransfer();
        }
        return;
    }
    //can have error on source and destination at the same time
    if(writeError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the write error: "+std::to_string(readError));
        if(readError)
            readThread.reopen();
        else
        {
            readIsClosedVariable=false;
            readThread.seekToZeroAndWait();
        }
        writeThread.reopen();
    }
    if(readError)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start and resume the read error");
        readThread.reopen();
    }
    if(!writeError && !readError)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] unknow error resume");
}

void TransferThread::writeThreadIsReopened()
{
    if(writeError_destination_reopened)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeError_destination_reopened=true;
    if(transfer_stat==TransferStat_Checksum)
    {
        writeThread.startCheckSum();
        return;
    }
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThread::readThreadIsSeekToZeroAndWait()
{
    if(writeError_source_seeked)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeError_source_seeked=true;
    if(writeError_source_seeked && writeError_destination_reopened)
        resumeTransferAfterWriteError();
}

void TransferThread::resumeTransferAfterWriteError()
{
    writeError=false;
/********************************
 if(canStartTransfer)
     readThread.startRead();
useless, because the open destination event
will restart the transfer as normal
*********************************/
/*********************************
if(!canStartTransfer)
    stat=WaitForTheTransfer;
useless because already do at open event
**********************************/
    //if is in wait
    if(!canStartTransfer)
        emit checkIfItCanBeResumed();
}

void TransferThread::readThreadResumeAfterError()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    readError=false;
    writeIsReady();
    readIsReady();
}

//////////////////////////////////////////////////////////////////
///////////////////////// Normal event ///////////////////////////
//////////////////////////////////////////////////////////////////

void TransferThread::readIsStopped()
{
    if(!sended_state_readStopped)
    {
        sended_state_readStopped=true;
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit readIsStopped()");
        emit readStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] drop dual read stopped");
        return;
    }
    readIsFinish();
}

void TransferThread::writeIsStopped()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start");
    if(!sended_state_writeStopped)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] emit writeStopped()");
        sended_state_writeStopped=true;
        emit writeStopped();
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] double event dropped");
        return;
    }
    writeIsFinish();
}

#ifdef ULTRACOPIER_PLUGIN_SPEED_SUPPORT
void TransferThread::timeOfTheBlockCopyFinished()
{
    readThread.timeOfTheBlockCopyFinished();
    writeThread.timeOfTheBlockCopyFinished();
}
#endif

bool TransferThread::setParallelBuffer(const int &parallelBuffer)
{
    if(parallelBuffer<1 || parallelBuffer>ULTRACOPIER_PLUGIN_MAX_PARALLEL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] wrong parallelBuffer: "+std::to_string(parallelBuffer));
        return false;
    }
    else
    {
        this->parallelBuffer=parallelBuffer;
        return true;
    }
}

bool TransferThread::setSequentialBuffer(const int &sequentialBuffer)
{
    if(sequentialBuffer<1 || sequentialBuffer>ULTRACOPIER_PLUGIN_MAX_SEQUENTIAL_NUMBER_OF_BLOCK)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] wrong sequentialBuffer: "+std::to_string(sequentialBuffer));
        return false;
    }
    else
    {
        this->sequentialBuffer=sequentialBuffer;
        return true;
    }
}

void TransferThread::setTransferAlgorithm(const TransferAlgorithm &transferAlgorithm)
{
    this->transferAlgorithm=transferAlgorithm;
    if(transferAlgorithm==TransferAlgorithm_Sequential)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transferAlgorithm==TransferAlgorithm_Sequential");
    else if(transferAlgorithm==TransferAlgorithm_Automatic)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transferAlgorithm==TransferAlgorithm_Automatic");
    else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] transferAlgorithm==TransferAlgorithm_Parallel");
}

//fonction to read the file date time
bool TransferThread::readFileDateTime(const QFileInfo &source)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readFileDateTime("+source.absoluteFilePath().toStdString()+")");
    if(source.lastModified()<minTime)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] the sources is older to copy the time: "+source.absoluteFilePath().toStdString()+": "+source.lastModified().toString().toStdString());
        return false;
    }
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            struct stat info;
            if(stat(source.absoluteFilePath().toLatin1().data(),&info)!=0)
                return false;
            time_t ctime=info.st_ctim.tv_sec;
            time_t actime=info.st_atim.tv_sec;
            time_t modtime=info.st_mtim.tv_sec;
            //this function avalaible on unix and mingw
            butime.actime=actime;
            butime.modtime=modtime;
            Q_UNUSED(ctime);
            return true;
        #else //mainly for mac
            time_t ctime=source.created().toTime_t();
            time_t actime=source.lastRead().toTime_t();
            time_t modtime=source.lastModified().toTime_t();
            //this function avalaible on unix and mingw
            butime.actime=actime;
            butime.modtime=modtime;
            Q_UNUSED(ctime);
            return true;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                struct stat info;
                if(stat(source.toLatin1().data(),&info)!=0)
                    return false;
                time_t ctime=info.st_ctim.tv_sec;
                time_t actime=info.st_atim.tv_sec;
                time_t modtime=info.st_mtim.tv_sec;
                //this function avalaible on unix and mingw
                butime.actime=actime;
                butime.modtime=modtime;
                Q_UNUSED(ctime);
                return true;
            #else
                wchar_t filePath[65535];
                if(std::regex_match(source.absoluteFilePath().toStdString(),regRead))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(source.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                HANDLE hFileSouce = CreateFileW(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if(hFileSouce == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] open failed to read: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                if(!GetFileTime(hFileSouce, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileSouce);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to get the file time");
                    return false;
                }
                this->ftCreateL=ftCreate.dwLowDateTime;
                this->ftCreateH=ftCreate.dwHighDateTime;
                this->ftAccessL=ftAccess.dwLowDateTime;
                this->ftAccessH=ftAccess.dwHighDateTime;
                this->ftWriteL=ftWrite.dwLowDateTime;
                this->ftWriteH=ftWrite.dwHighDateTime;
                CloseHandle(hFileSouce);
                return true;
            #endif
        #else
            return false;
        #endif
    #endif
    return false;
}

bool TransferThread::writeFileDateTime(const QFileInfo &destination)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeFileDateTime("+destination.absoluteFilePath().toStdString()+")");
    /** Why not do it with Qt? Because it not support setModificationTime(), and get the time with Qt, that's mean use local time where in C is UTC time */
    #ifdef Q_OS_UNIX
        #ifdef Q_OS_LINUX
            return utime(destination.absoluteFilePath().toLatin1().data(),&butime)==0;
        #else //mainly for mac
            return utime(destination.absoluteFilePath().toLatin1().data(),&butime)==0;
        #endif
    #else
        #ifdef Q_OS_WIN32
            #ifdef ULTRACOPIER_PLUGIN_SET_TIME_UNIX_WAY
                return utime(destination.toLatin1().data(),&butime)==0;
            #else
                wchar_t filePath[65535];
                if(std::regex_match(destination.absoluteFilePath().toStdString(),regRead))
                    filePath[QDir::toNativeSeparators(QStringLiteral("\\\\?\\")+destination.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                else
                    filePath[QDir::toNativeSeparators(destination.absoluteFilePath()).toWCharArray(filePath)]=L'\0';
                HANDLE hFileDestination = CreateFileW(filePath, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                if(hFileDestination == INVALID_HANDLE_VALUE)
                {
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] open failed to write: "+QString::fromWCharArray(filePath).toStdString()+", error: "+std::to_string(GetLastError()));
                    return false;
                }
                FILETIME ftCreate, ftAccess, ftWrite;
                ftCreate.dwLowDateTime=this->ftCreateL;
                ftCreate.dwHighDateTime=this->ftCreateH;
                ftAccess.dwLowDateTime=this->ftAccessL;
                ftAccess.dwHighDateTime=this->ftAccessH;
                ftWrite.dwLowDateTime=this->ftWriteL;
                ftWrite.dwHighDateTime=this->ftWriteH;
                if(!SetFileTime(hFileDestination, &ftCreate, &ftAccess, &ftWrite))
                {
                    CloseHandle(hFileDestination);
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] unable to set the file time");
                    return false;
                }
                CloseHandle(hFileDestination);
                return true;
            #endif
        #else
            return false;
        #endif
    #endif
    return false;
}

bool TransferThread::readFilePermissions(const QFile &source)
{
    this->permissions=source.permissions();
    return true;
}

bool TransferThread::writeFilePermissions(QFile &destination)
{
    return destination.setPermissions(this->permissions);
}

//skip the copy
void TransferThread::skip()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] start with stat: "+std::to_string(transfer_stat));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] readIsOpeningVariable: "+std::to_string(readIsOpeningVariable)+", readIsOpenVariable: "+std::to_string(readIsOpenVariable)+", readIsReadyVariable: "+std::to_string(readIsReadyVariable)+", readIsFinishVariable: "+std::to_string(readIsFinishVariable)+", readIsClosedVariable: "+std::to_string(readIsClosedVariable));
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] writeIsOpeningVariable: "+std::to_string(writeIsOpeningVariable)+", writeIsOpenVariable: "+std::to_string(writeIsOpenVariable)+", writeIsReadyVariable: "+std::to_string(writeIsReadyVariable)+", writeIsFinishVariable: "+std::to_string(writeIsFinishVariable)+", writeIsClosedVariable: "+std::to_string(writeIsClosedVariable));
    switch(transfer_stat)
    {
    case TransferStat_WaitForTheTransfer:
        //needRemove=true;never put that's here, can product destruction of the file
    case TransferStat_PreOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        needSkip=true;
        //check if all is source and destination is closed
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_Transfer:
    case TransferStat_PostTransfer:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        if(canBeMovedDirectlyVariable || canBeCopiedDirectlyVariable)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] Do the direct FS fake close, canBeMovedDirectlyVariable: "+std::to_string(canBeMovedDirectlyVariable)+", canBeCopiedDirectlyVariable: "+std::to_string(canBeCopiedDirectlyVariable));
            readThread.fakeReadIsStarted();
            writeThread.fakeWriteIsStarted();
            readThread.fakeReadIsStopped();
            writeThread.fakeWriteIsStopped();
            return;
        }
        writeThread.flushBuffer();
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_Checksum:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        if(remainFileOpen())
        {
            if(remainSourceOpen())
                readThread.stop();
            if(remainDestinationOpen())
                writeThread.stop();
        }
        else // wait nothing, just quit
        {
            transfer_stat=TransferStat_PostOperation;
            emit internalStartPostOperation();
        }
        break;
    case TransferStat_PostOperation:
        if(needSkip)
        {
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"["+std::to_string(id)+"] skip already in progress");
            return;
        }
        //needRemove=true;never put that's here, can product destruction of the file
        needSkip=true;
        writeThread.flushBuffer();
        emit internalStartPostOperation();
        break;
    default:
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+"] can skip in this state: "+std::to_string(transfer_stat));
        return;
    }
}

//return info about the copied size
int64_t TransferThread::copiedSize()
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_PostOperation:
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_Checksum:
        return transferSize;
    default:
        return 0;
    }
}

//retry after error
void TransferThread::putAtBottom()
{
    emit tryPutAtBottom();
}

#ifdef ULTRACOPIER_PLUGIN_RSYNC
/// \brief set rsync
void TransferThread::setRsync(const bool rsync)
{
    this->rsync=rsync;
}
#endif

void TransferThread::set_osBufferLimit(const unsigned int &osBufferLimit)
{
    this->osBufferLimit=osBufferLimit;
}

#ifdef ULTRACOPIER_PLUGIN_DEBUG
//to set the id
void TransferThread::setId(int id)
{
    this->id=id;
    readThread.setId(id);
    writeThread.setId(id);
}

char TransferThread::readingLetter() const
{
    switch(readThread.stat)
    {
    case ReadThread::Idle:
        return '_';
    break;
    case ReadThread::InodeOperation:
        return 'I';
    break;
    case ReadThread::Read:
        return 'R';
    break;
    case ReadThread::WaitWritePipe:
        return 'W';
    break;
    case ReadThread::Checksum:
        return 'S';
    break;
    default:
        return '?';
    }
}

char TransferThread::writingLetter() const
{
    switch(writeThread.stat)
    {
    case WriteThread::Idle:
        return '_';
    break;
    case WriteThread::InodeOperation:
        return 'I';
    break;
    case WriteThread::Write:
        return 'W';
    break;
    case WriteThread::Close:
        return 'C';
    break;
    case WriteThread::Read:
        return 'R';
    break;
    case WriteThread::Checksum:
        return 'S';
    break;
    default:
        return '?';
    }
}

#endif

void TransferThread::setMkpathTransfer(QSemaphore *mkpathTransfer)
{
    this->mkpathTransfer=mkpathTransfer;
    writeThread.setMkpathTransfer(mkpathTransfer);
}

void TransferThread::set_doChecksum(bool doChecksum)
{
    this->doChecksum=doChecksum;
}

void TransferThread::set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible)
{
    this->checksumIgnoreIfImpossible=checksumIgnoreIfImpossible;
}

void TransferThread::set_checksumOnlyOnError(bool checksumOnlyOnError)
{
    this->checksumOnlyOnError=checksumOnlyOnError;
}

void TransferThread::set_osBuffer(bool osBuffer)
{
    this->osBuffer=osBuffer;
}

void TransferThread::set_osBufferLimited(bool osBufferLimited)
{
    this->osBufferLimited=osBufferLimited;
}

//not copied size, because that's count to the checksum, ...
uint64_t TransferThread::realByteTransfered() const
{
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
    case TransferStat_Checksum:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_PostTransfer:
        return (readThread.getLastGoodPosition()+writeThread.getLastGoodPosition())/2;
    case TransferStat_PostOperation:
        return transferSize;
    default:
        return 0;
    }
}

//first is read, second is write
std::pair<uint64_t, uint64_t> TransferThread::progression() const
{
    std::pair<uint64_t,uint64_t> returnVar;
    switch(transfer_stat)
    {
    case TransferStat_Transfer:
        returnVar.first=readThread.getLastGoodPosition();
        returnVar.second=writeThread.getLastGoodPosition();
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_Checksum:
        returnVar.first=readThread.getLastGoodPosition();
        returnVar.second=writeThread.getLastGoodPosition();
    break;
    case TransferStat_PostTransfer:
        returnVar.first=transferSize;
        returnVar.second=writeThread.getLastGoodPosition();
        /*if(returnVar.first<returnVar.second)
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"["+std::to_string(id)+QStringLiteral("] read is smaller than write"));*/
    break;
    case TransferStat_PostOperation:
        returnVar.first=transferSize;
        returnVar.second=transferSize;
    break;
    default:
        returnVar.first=0;
        returnVar.second=0;
    }
    return returnVar;
}

void TransferThread::setRenamingRules(const std::string &firstRenamingRule, const std::string &otherRenamingRule)
{
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
}

void TransferThread::setDeletePartiallyTransferredFiles(const bool &deletePartiallyTransferredFiles)
{
    this->deletePartiallyTransferredFiles=deletePartiallyTransferredFiles;
}

void TransferThread::setRenameTheOriginalDestination(const bool &renameTheOriginalDestination)
{
    this->renameTheOriginalDestination=renameTheOriginalDestination;
}

void TransferThread::set_updateMount()
{
    driveManagement.tryUpdate();
}
