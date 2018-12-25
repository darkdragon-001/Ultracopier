#include "TransferModel.h"

#define COLUMN_COUNT 3

// Model

TransferModel::TransferModel()
{
    iconStart=QIcon(":/Themes/Windows/resources/player_play.png");
    iconPause=QIcon(":/Themes/Windows/resources/player_pause.png");
    iconStop=QIcon(":/Themes/Windows/resources/checkbox.png");
    currentIndexSearch=0;
    currentFile	= 0;
    totalFile	= 0;
    currentSize	= 0;
    totalSize	= 0;
    haveSearchItem=false;
}

int TransferModel::columnCount( const QModelIndex& parent ) const
{
    return parent == QModelIndex() ? COLUMN_COUNT : 0;
}

QVariant TransferModel::data( const QModelIndex& index, int role ) const
{
    Q_UNUSED(role);
    Q_UNUSED(index);
    return QVariant();
}

int TransferModel::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED(parent);
    return 0;
}

QVariant TransferModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role == Qt::DisplayRole && orientation == Qt::Horizontal && section >= 0 && section < COLUMN_COUNT ) {
        switch ( section ) {
            case 0:
            return facilityEngine->translateText("Source");
            case 1:
            return facilityEngine->translateText("Size");
            case 2:
            return facilityEngine->translateText("Destination");
        }
    }

    return QAbstractTableModel::headerData( section, orientation, role );
}

bool TransferModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    Q_UNUSED(index);
    Q_UNUSED(value);
    Q_UNUSED(role);
    return true;
}

/*
  Return[0]: totalFile
  Return[1]: totalSize
  Return[2]: currentFile
  */
QList<quint64> TransferModel::synchronizeItems(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions)
{
    loop_size=returnActions.size();
    index_for_loop=0;
    emit layoutAboutToBeChanged();
    while(index_for_loop<loop_size)
    {
        const Ultracopier::ReturnActionOnCopyList& action=returnActions.at(index_for_loop);
        switch(action.type)
        {
            case Ultracopier::AddingItem:
            {
                this->totalFile++;
                this->totalSize+=action.addAction.size;
            }
            break;
            case Ultracopier::RemoveItem:
                internalRunningOperation.remove(action.addAction.id);
                startId.remove(action.addAction.id);
                stopId.remove(action.addAction.id);
                this->currentFile++;
            break;
            case Ultracopier::PreOperation:
            {
                ItemOfCopyListWithMoreInformations tempItem;
                tempItem.currentProgression=0;
                tempItem.generalData=action.addAction;
                tempItem.generalData.destinationFullPath.remove(tempItem.generalData.destinationFullPath.size()-tempItem.generalData.destinationFileName.size(),tempItem.generalData.destinationFileName.size());
                tempItem.generalData.sourceFullPath.remove(tempItem.generalData.sourceFullPath.size()-tempItem.generalData.sourceFileName.size(),tempItem.generalData.sourceFileName.size());
                tempItem.actionType=action.type;
                internalRunningOperation[action.addAction.id]=tempItem;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("set for file %1: actionType: PreOperation").arg(action.addAction.id));
            }
            break;
            case Ultracopier::Transfer:
            {
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,QStringLiteral("found entry for file %1: actionType: Transfer").arg(action.addAction.id));
                if(!startId.contains(action.addAction.id))
                    startId << action.addAction.id;
                stopId.remove(action.addAction.id);
                if(internalRunningOperation.contains(action.addAction.id))
                    internalRunningOperation[action.addAction.id].actionType=action.type;
                else
                    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unable to found entry for file %1: actionType: Transfer").arg(action.addAction.id));
            }
            break;
            case Ultracopier::PostOperation:
            {
                if(!stopId.contains(action.addAction.id))
                    stopId << action.addAction.id;
                startId.remove(action.addAction.id);
            }
            break;
            case Ultracopier::CustomOperation:
            {
                bool custom_with_progression=(action.addAction.size==1);
                //without progression
                if(custom_with_progression)
                {
                    if(startId.remove(action.addAction.id))
                        if(!stopId.contains(action.addAction.id))
                            stopId << action.addAction.id;
                }
                //with progression
                else
                {
                    stopId.remove(action.addAction.id);
                    if(!startId.contains(action.addAction.id))
                        startId << action.addAction.id;
                }
                if(internalRunningOperation.contains(action.addAction.id))
                {
                    ItemOfCopyListWithMoreInformations &item=internalRunningOperation[action.addAction.id];
                    item.actionType=action.type;
                    item.custom_with_progression=custom_with_progression;
                    item.currentProgression=0;
                }
            }
            break;
            default:
                //unknow code, ignore it
            break;
        }
        index_for_loop++;
    }
    emit layoutChanged();
    return QList<quint64>() << totalFile << totalSize << currentFile;
}

void TransferModel::setFacilityEngine(FacilityInterface * facilityEngine)
{
    this->facilityEngine=facilityEngine;
}

void TransferModel::setFileProgression(QList<Ultracopier::ProgressionItem> &progressionList)
{
    loop_size=progressionList.size();
    index_for_loop=0;
    while(index_for_loop<loop_size)
    {
        if(internalRunningOperation.contains(progressionList.at(index_for_loop).id))
        {
            internalRunningOperation[progressionList.at(index_for_loop).id].generalData.size=progressionList.at(index_for_loop).total;
            internalRunningOperation[progressionList.at(index_for_loop).id].currentProgression=(progressionList.at(index_for_loop).currentRead+progressionList.at(index_for_loop).currentWrite)/2;
            #ifdef ULTRACOPIER_PLUGIN_DEBUG
            progressionList.removeAt(index_for_loop);
            index_for_loop--;
            loop_size--;
            #endif
        }
        index_for_loop++;
    }
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    if(progressionList.size()>0)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"progression remaning items");
    #endif
}

TransferModel::currentTransfertItem TransferModel::getCurrentTransfertItem()
{
    currentTransfertItem returnItem;
    returnItem.haveItem=startId.size()>0;
    if(returnItem.haveItem)
    {
        if(!internalRunningOperation.contains(*startId.constBegin()))
        {
            returnItem.haveItem=false;
            return returnItem;
        }
        const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation[*startId.constBegin()];
        returnItem.from=itemTransfer.generalData.sourceFullPath;
        returnItem.to=itemTransfer.generalData.destinationFullPath;
        returnItem.current_file=itemTransfer.generalData.destinationFileName;
        returnItem.size=facilityEngine->sizeToString(itemTransfer.generalData.size);
        switch(itemTransfer.actionType)
        {
            case Ultracopier::CustomOperation:
            if(!itemTransfer.custom_with_progression)
                returnItem.progressBar_file=-1;
            else
            {
                if(itemTransfer.generalData.size>0)
                    returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
                else
                    returnItem.progressBar_file=-1;
            }
            break;
            case Ultracopier::Transfer:
            if(itemTransfer.generalData.size>0)
                returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
            else
                returnItem.progressBar_file=0;
            break;
            //should never pass here
            case Ultracopier::PostOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_file=65535;
            break;
            //should never pass here
            case Ultracopier::PreOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_file=0;
            break;
            default:
                returnItem.progressBar_file=0;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                break;
        }
    }
    else
    {
        if(!internalRunningOperation.contains(*stopId.constBegin()))
        {
            returnItem.haveItem=false;
            return returnItem;
        }
        else
            returnItem.haveItem=true;
        const ItemOfCopyListWithMoreInformations &itemTransfer=internalRunningOperation[*stopId.constBegin()];
        returnItem.from=itemTransfer.generalData.sourceFullPath;
        returnItem.to=itemTransfer.generalData.destinationFullPath;
        returnItem.current_file=itemTransfer.generalData.destinationFileName;
        returnItem.size=facilityEngine->sizeToString(itemTransfer.generalData.size);
        switch(itemTransfer.actionType)
        {
            case Ultracopier::CustomOperation:
            if(!itemTransfer.custom_with_progression)
                returnItem.progressBar_file=-1;
            else
            {
                if(itemTransfer.generalData.size>0)
                    returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
                else
                    returnItem.progressBar_file=-1;
            }
            break;
            case Ultracopier::Transfer:
            if(itemTransfer.generalData.size>0)
                returnItem.progressBar_file=((double)itemTransfer.currentProgression/itemTransfer.generalData.size)*65535;
            else
                returnItem.progressBar_file=0;
            break;
            //should never pass here
            case Ultracopier::PostOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_file=65535;
            break;
            //should never pass here
            case Ultracopier::PreOperation:
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("wrong action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                returnItem.progressBar_file=0;
            break;
            default:
                returnItem.progressBar_file=65535;
                ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,QStringLiteral("unknow action type for file %1: actionType: %2").arg(itemTransfer.generalData.id).arg(itemTransfer.actionType));
                break;
        }
    }
    return returnItem;
}
