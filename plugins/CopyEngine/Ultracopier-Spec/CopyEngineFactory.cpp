/** \file factory.cpp
\brief Define the factory to create new instance
\author alpha_one_x86 */

#include <QFileDialog>
#include <QList>
#include <QDebug>
#include <cmath>
#include <QStorageInfo>

#include "../../../cpp11addition.h"
#include "CopyEngineFactory.h"
#include "TransferThread.h"

// The cmath header from MSVC does not contain round()
#if (defined(_WIN64) || defined(_WIN32)) && defined(_MSC_VER)
inline double round(double d) {
    return floor( d + 0.5 );
}
#endif

CopyEngineFactory::CopyEngineFactory() :
    ui(new Ui::copyEngineOptions())
{
    qRegisterMetaType<FolderExistsAction>("FolderExistsAction");
    qRegisterMetaType<FileExistsAction>("FileExistsAction");
    qRegisterMetaType<std::vector<Filters_rules> >("std::vector<Filters_rules>");
    qRegisterMetaType<TransferStat>("TransferStat");
    qRegisterMetaType<ActionType>("ActionType");
    qRegisterMetaType<ErrorType>("ErrorType");
    qRegisterMetaType<Diskspace>("Diskspace");
    qRegisterMetaType<std::vector<Diskspace> >("std::vector<Diskspace>");
    qRegisterMetaType<Ultracopier::CopyMode>("Ultracopier::CopyMode");
    qRegisterMetaType<TransferThread::dirent_uc>("TransferThread::dirent_uc");
    #ifdef WIDESTRING
    qRegisterMetaType<std::wstring>("std::wstring");
    qRegisterMetaType<wchar_t *>("wchar_t *");
    #endif

    tempWidget=new QWidget();
    ui->setupUi(tempWidget);
    ui->toolBox->setCurrentIndex(0);
    errorFound=false;
    optionsEngine=NULL;
    filters=new Filters(tempWidget);
    renamingRules=new RenamingRules(tempWidget);

    connect(ui->doRightTransfer,            &QCheckBox::toggled,                                            this,&CopyEngineFactory::setDoRightTransfer);
    connect(ui->keepDate,                   &QCheckBox::toggled,                                            this,&CopyEngineFactory::setKeepDate);
    connect(ui->inodeThreads,               static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),	this,&CopyEngineFactory::on_inodeThreads_editingFinished);
    connect(ui->comboBoxFolderError,        static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFolderError);
    connect(ui->comboBoxFolderCollision,	static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFolderCollision);
    connect(ui->comboBoxFileError,          static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFileError);
    connect(ui->comboBoxFileCollision,      static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),		this,&CopyEngineFactory::setFileCollision);
    connect(ui->checkBoxDestinationFolderExists,	&QCheckBox::toggled,		this,&CopyEngineFactory::setCheckDestinationFolder);
    connect(ui->mkpath,                     &QCheckBox::toggled,		this,&CopyEngineFactory::setMkFullPath);
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    connect(ui->rsync,                      &QCheckBox::toggled,                this,&CopyEngineFactory::setRsync);
    #endif
    connect(ui->inodeThreads,               &QSpinBox::editingFinished,         this,&CopyEngineFactory::on_inodeThreads_editingFinished);
    connect(ui->moveTheWholeFolder,         &QCheckBox::toggled,                this,&CopyEngineFactory::moveTheWholeFolder);
    connect(ui->followTheStrictOrder,       &QCheckBox::toggled,                this,&CopyEngineFactory::followTheStrictOrder);
    connect(ui->deletePartiallyTransferredFiles,&QCheckBox::toggled,            this,&CopyEngineFactory::deletePartiallyTransferredFiles);
    connect(ui->renameTheOriginalDestination,&QCheckBox::toggled,               this,&CopyEngineFactory::renameTheOriginalDestination);
    connect(ui->checkDiskSpace,             &QCheckBox::toggled,                this,&CopyEngineFactory::checkDiskSpace);
    connect(ui->defaultDestinationFolderBrowse,&QPushButton::clicked,           this,&CopyEngineFactory::defaultDestinationFolderBrowse);
    connect(ui->defaultDestinationFolder,&QLineEdit::editingFinished,           this,&CopyEngineFactory::defaultDestinationFolder);

    connect(filters,&Filters::sendNewFilters,this,&CopyEngineFactory::sendNewFilters);
    connect(ui->filters,&QPushButton::clicked,this,&CopyEngineFactory::showFilterDialog);
    connect(renamingRules,&RenamingRules::sendNewRenamingRules,this,&CopyEngineFactory::sendNewRenamingRules);
    connect(ui->renamingRules,&QPushButton::clicked,this,&CopyEngineFactory::showRenamingRules);

    lunchInitFunction.setInterval(0);
    lunchInitFunction.setSingleShot(true);
    connect(&lunchInitFunction,&QTimer::timeout,this,&CopyEngineFactory::init,Qt::QueuedConnection);
    lunchInitFunction.start();
}

CopyEngineFactory::~CopyEngineFactory()
{
    delete renamingRules;
    delete filters;
    delete ui;
}

void CopyEngineFactory::init()
{
}

PluginInterface_CopyEngine * CopyEngineFactory::getInstance()
{
    CopyEngine *realObject=new CopyEngine(facilityEngine);
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    connect(realObject,&CopyEngine::debugInformation,this,&CopyEngineFactory::debugInformation);
    #endif
    realObject->connectTheSignalsSlots();
    PluginInterface_CopyEngine * newTransferEngine=realObject;
    connect(this,&CopyEngineFactory::reloadLanguage,realObject,&CopyEngine::newLanguageLoaded);
    realObject->setRightTransfer(ui->doRightTransfer->isChecked());
    realObject->setKeepDate(ui->keepDate->isChecked());
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    realObject->setRsync(ui->rsync->isChecked());
    #endif
    realObject->setFolderCollision(ui->comboBoxFolderCollision->currentIndex());
    realObject->setFolderError(ui->comboBoxFolderError->currentIndex());
    realObject->setFileCollision(ui->comboBoxFileCollision->currentIndex());
    realObject->setFileError(ui->comboBoxFileError->currentIndex());
    realObject->setCheckDestinationFolderExists(ui->checkBoxDestinationFolderExists->isChecked());
    realObject->setMkFullPath(ui->mkpath->isChecked());
    realObject->set_setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    realObject->setRenamingRules(firstRenamingRule,otherRenamingRule);
    realObject->setMoveTheWholeFolder(ui->moveTheWholeFolder->isChecked());
    realObject->setFollowTheStrictOrder(ui->followTheStrictOrder->isChecked());
    realObject->setDeletePartiallyTransferredFiles(ui->deletePartiallyTransferredFiles->isChecked());
    realObject->setInodeThreads(ui->inodeThreads->value());
    realObject->setRenameTheOriginalDestination(ui->renameTheOriginalDestination->isChecked());
    realObject->setCheckDiskSpace(ui->checkDiskSpace->isChecked());
    realObject->setDefaultDestinationFolder(ui->defaultDestinationFolder->text().toStdString());
    return newTransferEngine;
}

void CopyEngineFactory::setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,
                                     FacilityInterface * facilityInterface,const bool &portableVersion)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, writePath: "+writePath+", pluginPath:"+pluginPath);
    this->facilityEngine=facilityInterface;
    Q_UNUSED(portableVersion);
    #ifndef ULTRACOPIER_PLUGIN_DEBUG
        Q_UNUSED(writePath);
        Q_UNUSED(pluginPath);
    #endif
    #if ! defined (Q_CC_GNU)
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("Unable to change date time of files, only gcc is supported"));
    #endif
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,COMPILERINFO);
    #if defined (UNICODE)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"UNICODE set");
    #endif
    #if defined (_FILE_OFFSET_BITS)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"_FILE_OFFSET_BITS set to "+std::to_string(_FILE_OFFSET_BITS));
    #endif
    #if defined (WIDESTRING)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"WIDESTRING set");
    #endif
    #if defined (_LARGE_FILE_SOURCE)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"_LARGE_FILE_SOURCE set to "+std::to_string(_LARGE_FILE_SOURCE));
    #endif
    #if defined (ULTRACOPIER_PLUGIN_CHECKLISTTYPE)
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE set");
    #else
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"CHECK LIST TYPE not set");
    #endif
    if(options!=NULL)
    {
        //load the options
        std::vector<std::pair<std::string, std::string> > KeysList;
        KeysList.push_back(std::pair<std::string, std::string>("doRightTransfer","true"));
        #ifndef Q_OS_LINUX
        KeysList.push_back(std::pair<std::string, std::string>("keepDate","false"));
        #else
        KeysList.push_back(std::pair<std::string, std::string>("keepDate","true"));
        #endif
        KeysList.push_back(std::pair<std::string, std::string>("blockSize",std::to_string(ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE)));
        uint32_t sequentialBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_SEQUENTIAL_NUMBER_OF_BLOCK;
        uint32_t parallelBuffer=ULTRACOPIER_PLUGIN_DEFAULT_BLOCK_SIZE*ULTRACOPIER_PLUGIN_DEFAULT_PARALLEL_NUMBER_OF_BLOCK;
        //to prevent swap and other bad effect, only under windows and unix for now
        #if defined(Q_OS_WIN32) or (defined(Q_OS_LINUX) and defined(_SC_PHYS_PAGES))
        size_t max_memory=getTotalSystemMemory()/1024;
        if(max_memory>0)
        {
            if(max_memory>2147483648)
                max_memory=2147483648;
            if(sequentialBuffer>(max_memory/10))
               sequentialBuffer=max_memory/10;
            if(parallelBuffer>(max_memory/100))
               parallelBuffer=max_memory/100;
        }
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,QStringLiteral("detected memory: %1MB").arg(max_memory/1024).toStdString());
        #endif
        KeysList.push_back(std::pair<std::string, std::string>("sequentialBuffer",std::to_string(sequentialBuffer)));
        KeysList.push_back(std::pair<std::string, std::string>("parallelBuffer",std::to_string(parallelBuffer)));
        KeysList.push_back(std::pair<std::string, std::string>("parallelizeIfSmallerThan",std::to_string(128)));//128KB, better for modern hardware: Multiple queue en linux, SSD, ...
        KeysList.push_back(std::pair<std::string, std::string>("autoStart","true"));
        #ifdef ULTRACOPIER_PLUGIN_RSYNC
        KeysList.push_back(std::pair<std::string, std::string>("rsync","true"));
        #endif
        KeysList.push_back(std::pair<std::string, std::string>("folderError",std::to_string(0)));
        KeysList.push_back(std::pair<std::string, std::string>("folderCollision",std::to_string(0)));
        KeysList.push_back(std::pair<std::string, std::string>("fileError",std::to_string(2)));
        KeysList.push_back(std::pair<std::string, std::string>("fileCollision",std::to_string(0)));
        KeysList.push_back(std::pair<std::string, std::string>("transferAlgorithm",std::to_string(0)));
        KeysList.push_back(std::pair<std::string, std::string>("checkDestinationFolder","true"));
        KeysList.push_back(std::pair<std::string, std::string>("mkpath","true"));
        KeysList.push_back(std::pair<std::string, std::string>("includeStrings",""));
        KeysList.push_back(std::pair<std::string, std::string>("includeOptions",""));
        KeysList.push_back(std::pair<std::string, std::string>("excludeStrings",""));
        KeysList.push_back(std::pair<std::string, std::string>("excludeOptions",""));
        KeysList.push_back(std::pair<std::string, std::string>("doChecksum","false"));
        KeysList.push_back(std::pair<std::string, std::string>("checksumIgnoreIfImpossible","true"));
        KeysList.push_back(std::pair<std::string, std::string>("checksumOnlyOnError","true"));
        KeysList.push_back(std::pair<std::string, std::string>("osBuffer","false"));
        KeysList.push_back(std::pair<std::string, std::string>("firstRenamingRule",""));
        KeysList.push_back(std::pair<std::string, std::string>("otherRenamingRule",""));
        KeysList.push_back(std::pair<std::string, std::string>("osBufferLimited","false"));
        KeysList.push_back(std::pair<std::string, std::string>("osBufferLimit",std::to_string(512)));
        KeysList.push_back(std::pair<std::string, std::string>("deletePartiallyTransferredFiles","true"));
        KeysList.push_back(std::pair<std::string, std::string>("moveTheWholeFolder","true"));
        KeysList.push_back(std::pair<std::string, std::string>("followTheStrictOrder","false"));
        KeysList.push_back(std::pair<std::string, std::string>("renameTheOriginalDestination","false"));
        KeysList.push_back(std::pair<std::string, std::string>("checkDiskSpace","true"));
        KeysList.push_back(std::pair<std::string, std::string>("defaultDestinationFolder",""));
        KeysList.push_back(std::pair<std::string, std::string>("inodeThreads",std::to_string(16)));
        options->addOptionGroup(KeysList);

        optionsEngine=options;
        resetOptions();

        std::string firstRenamingRule=options->getOptionValue("firstRenamingRule");
        if(firstRenamingRule.find("%name%")==std::string::npos || firstRenamingRule.find("%suffix%")==std::string::npos)
            options->setOptionValue("firstRenamingRule","");
        std::string otherRenamingRule=options->getOptionValue("otherRenamingRule");
        if(otherRenamingRule.find("%name%")==std::string::npos || otherRenamingRule.find("%suffix%")==std::string::npos
                 || otherRenamingRule.find("%number%")==std::string::npos)
            options->setOptionValue("otherRenamingRule","");
    }
}

std::vector<std::string> CopyEngineFactory::supportedProtocolsForTheSource() const
{
    std::vector<std::string> l;
    l.push_back("file");
    return l;
}

std::vector<std::string> CopyEngineFactory::supportedProtocolsForTheDestination() const
{
    std::vector<std::string> l;
    l.push_back("file");
    return l;
}

Ultracopier::CopyType CopyEngineFactory::getCopyType()
{
    return Ultracopier::FileAndFolder;
}

Ultracopier::TransferListOperation CopyEngineFactory::getTransferListOperation()
{
    return Ultracopier::TransferListOperation_ImportExport;
}

bool CopyEngineFactory::canDoOnlyCopy() const
{
    return false;
}

/// \brief to get if have pause
bool CopyEngineFactory::havePause()
{
    return false;
}

void CopyEngineFactory::resetOptions()
{
    auto options=optionsEngine;
    optionsEngine=NULL;
    #if ! defined (Q_CC_GNU)
    ui->keepDate->setEnabled(false);
    ui->keepDate->setToolTip(QStringLiteral("Not supported with this compiler"));
    #endif
    ui->doRightTransfer->setChecked(stringtobool(options->getOptionValue("doRightTransfer")));
    ui->keepDate->setChecked(stringtobool(options->getOptionValue("keepDate")));
    #ifdef ULTRACOPIER_PLUGIN_RSYNC
    ui->rsync->setChecked(stringtobool(options->getOptionValue("rsync")));
    #else
    ui->label_rsync->setVisible(false);
    ui->rsync->setVisible(false);
    #endif
    ui->comboBoxFolderError->setCurrentIndex(stringtouint32(options->getOptionValue("folderError")));
    ui->comboBoxFolderCollision->setCurrentIndex(stringtouint32(options->getOptionValue("folderCollision")));
    ui->comboBoxFileError->setCurrentIndex(stringtouint32(options->getOptionValue("fileError")));
    ui->comboBoxFileCollision->setCurrentIndex(stringtouint32(options->getOptionValue("fileCollision")));
    ui->checkBoxDestinationFolderExists->setChecked(stringtobool(options->getOptionValue("checkDestinationFolder")));
    ui->mkpath->setChecked(stringtobool(options->getOptionValue("mkpath")));
    ui->deletePartiallyTransferredFiles->setChecked(stringtobool(options->getOptionValue("deletePartiallyTransferredFiles")));
    ui->moveTheWholeFolder->setChecked(stringtobool(options->getOptionValue("moveTheWholeFolder")));
    ui->followTheStrictOrder->setChecked(stringtobool(options->getOptionValue("followTheStrictOrder")));
    ui->inodeThreads->setValue(stringtouint32(options->getOptionValue("inodeThreads")));
    ui->renameTheOriginalDestination->setChecked(stringtobool(options->getOptionValue("renameTheOriginalDestination")));
    ui->checkDiskSpace->setChecked(stringtobool(options->getOptionValue("checkDiskSpace")));
    ui->defaultDestinationFolder->setText(QString::fromStdString(options->getOptionValue("defaultDestinationFolder")));

    //ui->autoStart->setChecked(options->getOptionValue("autoStart").toBool());//moved from options(), wrong previous place
    includeStrings=stringtostringlist(options->getOptionValue("includeStrings"));
    includeOptions=stringtostringlist(options->getOptionValue("includeOptions"));
    excludeStrings=stringtostringlist(options->getOptionValue("excludeStrings"));
    excludeOptions=stringtostringlist(options->getOptionValue("excludeOptions"));
    filters->setFilters(includeStrings,includeOptions,excludeStrings,excludeOptions);
    firstRenamingRule=options->getOptionValue("firstRenamingRule");
    otherRenamingRule=options->getOptionValue("otherRenamingRule");
    renamingRules->setRenamingRules(firstRenamingRule,otherRenamingRule);

    optionsEngine=options;
}

QWidget * CopyEngineFactory::options()
{
    return tempWidget;
}

void CopyEngineFactory::setDoRightTransfer(bool doRightTransfer)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("doRightTransfer",booltostring(doRightTransfer));
}

void CopyEngineFactory::setKeepDate(bool keepDate)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("keepDate",booltostring(keepDate));
}

void CopyEngineFactory::setFolderCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("folderCollision",std::to_string(index));
}

void CopyEngineFactory::setFolderError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("folderError",std::to_string(index));
}

void CopyEngineFactory::setCheckDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkDestinationFolder",booltostring(ui->checkBoxDestinationFolderExists->isChecked()));
}

void CopyEngineFactory::setMkFullPath()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("mkpath",booltostring(ui->mkpath->isChecked()));
}

void CopyEngineFactory::newLanguageLoaded()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"start, retranslate the widget options");
    OptionInterface * optionsEngine=this->optionsEngine;
    this->optionsEngine=NULL;
    ui->retranslateUi(tempWidget);
    ui->comboBoxFolderError->setItemText(0,tr("Ask"));
    ui->comboBoxFolderError->setItemText(1,tr("Skip"));

    ui->comboBoxFolderCollision->setItemText(0,tr("Ask"));
    ui->comboBoxFolderCollision->setItemText(1,tr("Merge"));
    ui->comboBoxFolderCollision->setItemText(2,tr("Skip"));
    ui->comboBoxFolderCollision->setItemText(3,tr("Rename"));

    ui->comboBoxFileError->setItemText(0,tr("Ask"));
    ui->comboBoxFileError->setItemText(1,tr("Skip"));
    ui->comboBoxFileError->setItemText(2,tr("Put at the end"));

    ui->comboBoxFileCollision->setItemText(0,tr("Ask"));
    ui->comboBoxFileCollision->setItemText(1,tr("Skip"));
    ui->comboBoxFileCollision->setItemText(2,tr("Overwrite"));
    ui->comboBoxFileCollision->setItemText(3,tr("Overwrite if different"));
    ui->comboBoxFileCollision->setItemText(4,tr("Overwrite if newer"));
    ui->comboBoxFileCollision->setItemText(5,tr("Overwrite if older"));
    ui->comboBoxFileCollision->setItemText(6,tr("Rename"));

    if(optionsEngine!=NULL)
    {
        filters->newLanguageLoaded();
        renamingRules->newLanguageLoaded();
    }
    emit reloadLanguage();
    this->optionsEngine=optionsEngine;
}

void CopyEngineFactory::showFilterDialog()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded. Unable to access the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    filters->exec();
}

void CopyEngineFactory::sendNewFilters(const std::vector<std::string> &includeStrings,const std::vector<std::string> &includeOptions,const std::vector<std::string> &excludeStrings,const std::vector<std::string> &excludeOptions)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->includeStrings=includeStrings;
    this->includeOptions=includeOptions;
    this->excludeStrings=excludeStrings;
    this->excludeOptions=excludeOptions;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("includeStrings",stringlisttostring(includeStrings));
        optionsEngine->setOptionValue("includeOptions",stringlisttostring(includeOptions));
        optionsEngine->setOptionValue("excludeStrings",stringlisttostring(excludeStrings));
        optionsEngine->setOptionValue("excludeOptions",stringlisttostring(excludeOptions));
    }
}

void CopyEngineFactory::sendNewRenamingRules(const std::string &firstRenamingRule,const std::string &otherRenamingRule)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"new filter");
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    if(optionsEngine!=NULL)
    {
        optionsEngine->setOptionValue("firstRenamingRule",firstRenamingRule);
        optionsEngine->setOptionValue("otherRenamingRule",otherRenamingRule);
    }
}

void CopyEngineFactory::showRenamingRules()
{
    if(optionsEngine==NULL)
    {
        QMessageBox::critical(NULL,tr("Options error"),tr("Options engine is not loaded, can't access to the filters"));
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"options not loaded");
        return;
    }
    renamingRules->exec();
}

void CopyEngineFactory::setFileCollision(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action index: "+std::to_string(index));
    if(optionsEngine==NULL)
        return;
    switch(index)
    {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            optionsEngine->setOptionValue("fileCollision",std::to_string(index));
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
        break;
    }
}

void CopyEngineFactory::setFileError(int index)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"action index: "+std::to_string(index));
    if(optionsEngine==NULL)
        return;
    switch(index)
    {
        case 0:
        case 1:
        case 2:
            optionsEngine->setOptionValue("fileError",std::to_string(index));
        break;
        default:
            ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Warning,"Error, unknow index, ignored");
        break;
    }
}

void CopyEngineFactory::deletePartiallyTransferredFiles(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("deletePartiallyTransferredFiles",booltostring(checked));
}

void CopyEngineFactory::renameTheOriginalDestination(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("renameTheOriginalDestination",booltostring(checked));
}

void CopyEngineFactory::checkDiskSpace(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("checkDiskSpace",booltostring(checked));
}

void CopyEngineFactory::defaultDestinationFolderBrowse()
{
    QString destination = QFileDialog::getExistingDirectory(ui->defaultDestinationFolder,
                                                            QString::fromStdString(facilityEngine->translateText("Select destination directory")),
                                                            "",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(destination.isEmpty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Information,"Canceled by the user");
        return;
    }
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    ui->defaultDestinationFolder->setText(destination);
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("defaultDestinationFolder",destination.toStdString());
}

void CopyEngineFactory::defaultDestinationFolder()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("defaultDestinationFolder",ui->defaultDestinationFolder->text().toStdString());
}

void CopyEngineFactory::followTheStrictOrder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("followTheStrictOrder",booltostring(checked));
}

void CopyEngineFactory::moveTheWholeFolder(bool checked)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("moveTheWholeFolder",booltostring(checked));
}

void CopyEngineFactory::on_inodeThreads_editingFinished()
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the spinbox have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("inodeThreads",std::to_string(ui->inodeThreads->value()));
}

#ifdef Q_OS_WIN32
size_t CopyEngineFactory::getTotalSystemMemory()
{
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
}
#endif

#ifdef Q_OS_LINUX
size_t CopyEngineFactory::getTotalSystemMemory()
{
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}
#endif

#ifdef ULTRACOPIER_PLUGIN_RSYNC
void CopyEngineFactory::setRsync(bool rsync)
{
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"the value have changed");
    if(optionsEngine!=NULL)
        optionsEngine->setOptionValue("rsync",std::to_string(rsync));
}
#endif
