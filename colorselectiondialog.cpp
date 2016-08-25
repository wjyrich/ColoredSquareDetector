#include "colorselectiondialog.h"
#include "ui_colorselectiondialog.h"

ColorSelectionDialog::ColorSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ColorSelectionDialog)
{
    ui->setupUi(this);
    setWindowFlags((windowFlags()&(~(Qt::WindowContextHelpButtonHint)))|Qt::MSWindowsFixedSizeDialogHint);

    connect(ui->addBtn,SIGNAL(clicked(bool)),this,SLOT(addBtnClicked()));
    connect(ui->removeSelectedBtn,SIGNAL(clicked(bool)),this,SLOT(removeSelectedBtnClicked()));
    connect(ui->removeAllBtn,SIGNAL(clicked(bool)),this,SLOT(removeAllBtnClicked()));
    connect(ui->saveBtn,SIGNAL(clicked(bool)),this,SLOT(saveBtnClicked()));

    if(parent!=0)
    {
        MainWindow *wnd=(MainWindow*)parent;
        for(uint32_t i=0;i<wnd->possibleSquareColorCount;i++)
        {
            uint32_t color=wnd->possibleSquareColors[i];
            QString colorStr="#"+QString::number((uint)color,16).toUpper();
            QListWidgetItem *item=new QListWidgetItem(colorStr);
            item->setData(Qt::UserRole,color); // Store color for later retrieval
            ui->colorList->addItem(item);
        }
    }
}

ColorSelectionDialog::~ColorSelectionDialog()
{
    delete ui;
}

int ColorSelectionDialog::getItemCount()
{
    return ui->colorList->count();
}

QListWidgetItem *ColorSelectionDialog::getItemAt(int row)
{
    return ui->colorList->item(row);
}

void ColorSelectionDialog::addBtnClicked()
{
    uint32_t color;
    QString enteredText=ui->colorBox->text();
    if(enteredText.length()==0)
    {
        QMessageBox::critical(this,"Error","Please enter a color.");
        return;
    }
    char *str=strdup(enteredText.toStdString().c_str());
    if(!MainWindow::getColorFromHexString(str,color))
    {
        QMessageBox::critical(this,"Error","Please enter a valid color.");
        free(str);
        return;
    }
    free(str);

    // Check if similar/identical color already present

    int itemCount=ui->colorList->count();
    for(int i=0;i<itemCount;i++)
    {
        QListWidgetItem *item=ui->colorList->item(i);
        uint32_t thisColor=item->data(Qt::UserRole).toUInt(); // Read color from item
        if(thisColor==color)
        {
            QMessageBox::critical(this,"Error","This color is already listed.");
            return;
        }
        else if(getColorError(thisColor,color)<0.1)
        {
            QMessageBox::warning(this,"Warning","There are now two very similar colors in the list. They may be mistaken for each other by the image analyzer.");
            break;
        }
    }
    QString colorStr="#"+QString::number((uint)color,16).toUpper();
    QListWidgetItem *newItem=new QListWidgetItem(colorStr);
    newItem->setData(Qt::UserRole,QVariant(color)); // Store color for later retrieval
    ui->colorList->addItem(newItem);
}

void ColorSelectionDialog::removeSelectedBtnClicked()
{
    QList<QListWidgetItem*> items=ui->colorList->selectedItems();
    if(items.count()==0)
        return;
    ui->colorList->clearSelection();
    delete ui->colorList->takeItem(ui->colorList->row(items.at(0)));
    ui->colorList->repaint();
}

void ColorSelectionDialog::removeAllBtnClicked()
{
    QList<QListWidgetItem*> items=ui->colorList->selectedItems();
    ui->colorList->clearSelection();
    int count=items.count();
    for(int i=0;i<count;i++)
        delete ui->colorList->takeItem(i);
    ui->colorList->repaint();
}

void ColorSelectionDialog::saveBtnClicked()
{
    this->accept();
    this->close();
}
