#include "DiskSpace.h"
#include "ui_DiskSpace.h"
#include "StructEnumDefinition_CopyEngine.h"

DiskSpace::DiskSpace(FacilityInterface * facilityEngine,std::vector<Diskspace> list,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DiskSpace)
{
    Qt::WindowFlags flags = windowFlags();
    #ifdef Q_OS_LINUX
    flags=flags & ~Qt::X11BypassWindowManagerHint;
    #endif
    flags=flags | Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);

    ui->setupUi(this);
    ok=false;
    int index=0;
    int size=list.size();
    QString drives;
    while(index<size)
    {
        drives+=tr("Drives %1 have %2 available but need %3")
                .arg(QString::fromStdString(list.at(index).drive))
                .arg(QString::fromStdString(facilityEngine->sizeToString(list.at(index).freeSpace)))
                .arg(QString::fromStdString(facilityEngine->sizeToString(list.at(index).requiredSpace)));
        drives+=QStringLiteral("<br />");
        index++;
    }
    ui->drives->setText(drives);
}

DiskSpace::~DiskSpace()
{
    delete ui;
}

void DiskSpace::on_ok_clicked()
{
    ok=true;
    close();
}

void DiskSpace::on_cancel_clicked()
{
    ok=false;
    close();
}

bool DiskSpace::getAction() const
{
    return ok;
}
