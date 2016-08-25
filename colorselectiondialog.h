#ifndef COLORSELECTIONDIALOG_H
#define COLORSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidgetItem>

#include "mainwindow.h"
#include "extcolordefs.h"

namespace Ui {
class ColorSelectionDialog;
}

class MainWindow;

class ColorSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ColorSelectionDialog(QWidget *parent = 0);
    ~ColorSelectionDialog();
    int getItemCount();
    QListWidgetItem *getItemAt(int row);

public slots:
    void addBtnClicked();
    void removeSelectedBtnClicked();
    void removeAllBtnClicked();
    void saveBtnClicked();

private:
    Ui::ColorSelectionDialog *ui;
};

#endif // COLORSELECTIONDIALOG_H
