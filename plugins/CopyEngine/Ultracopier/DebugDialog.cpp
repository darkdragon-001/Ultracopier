/** \file debugDialog.cpp
\brief Define the dialog to have debug information
\author alpha_one_x86 */

#include "DebugDialog.h"

#ifdef ULTRACOPIER_PLUGIN_DEBUG
#ifdef ULTRACOPIER_PLUGIN_DEBUG_WINDOW
#include "ui_debugDialog.h"

DebugDialog::DebugDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::debugDialog)
{
    ui->setupUi(this);
}

DebugDialog::~DebugDialog()
{
    delete ui;
}

void DebugDialog::setTransferList(const std::vector<std::string> &list)
{
    ui->tranferList->clear();
    unsigned int index=0;
    while(index<list.size())
    {
        ui->tranferList->addItem(QString::fromStdString(list.at(index)));
        index++;
    }
}

void DebugDialog::setActiveTransfer(const int &activeTransfer)
{
    ui->spinBoxActiveTransfer->setValue(activeTransfer);
}

void DebugDialog::setInodeUsage(const int &inodeUsage)
{
    ui->spinBoxNumberOfInode->setValue(inodeUsage);
}

void DebugDialog::setTransferThreadList(const std::vector<std::string> &list)
{
    ui->transferThreadList->clear();
    unsigned int index=0;
    while(index<list.size())
    {
        ui->transferThreadList->addItem(QString::fromStdString(list.at(index)));
        index++;
    }
}

#endif
#endif
