#include "FolderExistsDialog.h"
#include "ui_folderExistsDialog.h"
#include "TransferThread.h"
#include "../../../cpp11addition.h"

#ifdef Q_OS_WIN32
#define CURRENTSEPARATOR "\\"
#else
#define CURRENTSEPARATOR "/"
#endif

#include <QMessageBox>

FolderExistsDialog::FolderExistsDialog(QWidget *parent, INTERNALTYPEPATH source, bool isSame, INTERNALTYPEPATH destination,
                                       std::string firstRenamingRule, std::string otherRenamingRule) :
    QDialog(parent),
    ui(new Ui::folderExistsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    action=FolderExists_Cancel;
    oldName=TransferThread::resolvedName(TransferThread::internalStringTostring(destination));
    ui->lineEditNewName->setText(QString::fromStdString(oldName));
    ui->lineEditNewName->setPlaceholderText(QString::fromStdString(oldName));
#ifdef Q_OS_WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfoW;
    if(GetFileAttributesExW(source.c_str(),GetFileExInfoStandard,&fileInfoW))
    {
        uint64_t mdate=fileInfoW.ftLastWriteTime.dwHighDateTime;
        mdate<<=32;
        mdate|=fileInfoW.ftLastWriteTime.dwLowDateTime;
        uint64_t size=fileInfoW.nFileSizeHigh;
        size<<=32;
        size|=fileInfoW.nFileSizeLow;
#else
    struct stat source_statbuf;
    #ifdef Q_OS_UNIX
    if(lstat(TransferThread::internalStringTostring(source).c_str(), &source_statbuf)==0)
    #else
    if(stat(TransferThread::internalStringTostring(source).c_str(), &source_statbuf)==0)
    #endif
    {
        #ifdef Q_OS_UNIX
            #ifdef Q_OS_MAC
            const uint64_t mdate=source_statbuf.st_mtimespec.tv_sec;
            #else
            const uint64_t mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtim);
            #endif
        #else
        const uint64_t mdate=*reinterpret_cast<int64_t*>(&source_statbuf.st_mtime);
        #endif
#endif
        ui->label_content_source_modified->setText(QDateTime::fromSecsSinceEpoch(mdate).toString());
    }
    else
        ui->label_content_source_modified->hide();
    ui->label_content_source_folder_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(source))));
    std::string folder=TransferThread::internalStringTostring(FSabsolutePath(source));
    if(folder.size()>80)
        folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
    ui->label_content_source_folder->setText(QString::fromStdString(folder));
    if(ui->label_content_source_folder_name->text().isEmpty())
    {
        ui->label_source_folder_name->hide();
        ui->label_content_source_folder_name->hide();
    }
    if(isSame)
    {
        this->destinationInfo=TransferThread::internalStringTostring(source);
        ui->label_source->hide();
        ui->label_destination->hide();
        ui->label_destination_modified->hide();
        ui->label_destination_folder_name->hide();
        ui->label_destination_folder->hide();
        ui->label_content_destination_modified->hide();
        ui->label_content_destination_folder_name->hide();
        ui->label_content_destination_folder->hide();
    }
    else
    {
        this->destinationInfo=TransferThread::internalStringTostring(destination);
        this->setWindowTitle(tr("Folder already exists"));
        struct stat destination_statbuf;
        #ifdef Q_OS_UNIX
        if(lstat(TransferThread::internalStringTostring(destination).c_str(), &destination_statbuf)==0)
        #else
        if(stat(TransferThread::internalStringTostring(destination).c_str(), &destination_statbuf)==0)
        #endif
        {
            #ifdef Q_OS_UNIX
                #ifdef Q_OS_MAC
                const uint64_t mdate=destination_statbuf.st_mtimespec.tv_sec;
                #else
                const uint64_t mdate=*reinterpret_cast<int64_t*>(&destination_statbuf.st_mtim);
                #endif
            #else
            const uint64_t mdate=*reinterpret_cast<int64_t*>(&destination_statbuf.st_mtime);
            #endif
            ui->label_content_destination_modified->setText(QDateTime::fromSecsSinceEpoch(mdate).toString());
        }
        else
            ui->label_content_destination_modified->hide();
        ui->label_content_destination_folder_name->setText(QString::fromStdString(TransferThread::resolvedName(TransferThread::internalStringTostring(destination))));
        std::string folder=TransferThread::internalStringTostring(FSabsolutePath(destination));
        if(folder.size()>80)
            folder=folder.substr(0,38)+"..."+folder.substr(folder.size()-38);
        ui->label_content_destination_folder->setText(QString::fromStdString(folder));
        if(ui->label_content_destination_folder_name->text().isEmpty())
        {
            ui->label_destination_folder_name->hide();
            ui->label_content_destination_folder_name->hide();
        }
    }
    this->firstRenamingRule=firstRenamingRule;
    this->otherRenamingRule=otherRenamingRule;
    on_SuggestNewName_clicked();
}

FolderExistsDialog::~FolderExistsDialog()
{
    delete ui;
}

void FolderExistsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
        break;
        default:
        break;
    }
}

std::string FolderExistsDialog::getNewName()
{
    if(oldName==ui->lineEditNewName->text().toStdString() || ui->checkBoxAlways->isChecked())
        return "";
    else
        return ui->lineEditNewName->text().toStdString();
}

void FolderExistsDialog::on_SuggestNewName_clicked()
{
    struct stat p_statbuf;
    std::string destinationInfo=this->destinationInfo;
    QString absolutePath=QString::fromStdString(FSabsolutePath(destinationInfo));
    QString fileName=QString::fromStdString(TransferThread::resolvedName(destinationInfo));
    QString suffix;
    QString destination;
    QString newFileName;
    //resolv the suffix
    if(fileName.contains(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$"))))
    {
        suffix=fileName;
        suffix.replace(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$")),QStringLiteral("\\2"));
        fileName.replace(QRegularExpression(QStringLiteral("^(.*)(\\.[a-z0-9]+)$")),QStringLiteral("\\1"));
    }
    //resolv the new name
    int num=1;
    do
    {
        if(num==1)
        {
            if(firstRenamingRule.empty())
                newFileName=tr("%name% - copy");
            else
            {
                newFileName=QString::fromStdString(firstRenamingRule);
            }
        }
        else
        {
            if(otherRenamingRule.empty())
                newFileName=tr("%name% - copy (%number%)");
            else
                newFileName=QString::fromStdString(otherRenamingRule);
            newFileName.replace(QStringLiteral("%number%"),QString::number(num));
        }
        newFileName.replace(QStringLiteral("%name%"),fileName);
        newFileName.replace(QStringLiteral("%suffix%"),suffix);
        destination=absolutePath+CURRENTSEPARATOR+newFileName;
        destinationInfo=destination.toStdString();
        num++;
    }
    #ifdef Q_OS_UNIX
    while(lstat(destinationInfo.c_str(), &p_statbuf)==0);
    #else
    while(stat(destinationInfo.c_str(), &p_statbuf)==0);
    #endif
    ui->lineEditNewName->setText(newFileName);
}

void FolderExistsDialog::on_Rename_clicked()
{
    action=FolderExists_Rename;
    this->close();
}

void FolderExistsDialog::on_Skip_clicked()
{
    action=FolderExists_Skip;
    this->close();
}

void FolderExistsDialog::on_Cancel_clicked()
{
    action=FolderExists_Cancel;
    this->close();
}

FolderExistsAction FolderExistsDialog::getAction()
{
    return action;
}

bool FolderExistsDialog::getAlways()
{
    return ui->checkBoxAlways->isChecked();
}

void FolderExistsDialog::on_Merge_clicked()
{
    action=FolderExists_Merge;
    this->close();
}

void FolderExistsDialog::on_lineEditNewName_editingFinished()
{
    updateRenameButton();
}

void FolderExistsDialog::on_lineEditNewName_returnPressed()
{
    updateRenameButton();
    if(ui->Rename->isEnabled())
        on_Rename_clicked();
    else
        QMessageBox::warning(this,tr("Error"),tr("Try rename with using special characters"));
}

void FolderExistsDialog::on_lineEditNewName_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    updateRenameButton();
}

void FolderExistsDialog::updateRenameButton()
{
    ui->Rename->setEnabled(ui->checkBoxAlways->isChecked() || (!ui->lineEditNewName->text().contains(QRegularExpression("[/\\\\\\*]")) && oldName!=ui->lineEditNewName->text().toStdString() && !ui->lineEditNewName->text().isEmpty()));
}
