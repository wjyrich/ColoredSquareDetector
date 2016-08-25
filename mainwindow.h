#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <stdlib.h>
#include <stdint.h>
#include <limits>

#include <QMainWindow>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QStandardPaths>
#include <QGraphicsPixmapItem>
#include <QStringList>

#include "colorselectiondialog.h"
#include "text.h"
#include "extcolordefs.h"

namespace Ui {
class MainWindow;
}

class ColorSelectionDialog;

typedef double decimal_t;

enum Direction
{
    Left=1,Right=2,Top=4,Bottom=8
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    QFileDialog *dialog;
    QImage *image;
    QGraphicsScene *scene;
    QGraphicsPixmapItem *pixmapItem;
    // Image dimensions do not change; we do not have to store them, but we do so for optimization purposes
    int originalImageWidth;
    int originalImageHeight;
    int originalImageWidthM1;
    int originalImageHeightM1;
    uint32_t *originalImageData;
    uint32_t *pixelColorMap;
    bool showLargeImageWarning;

    ColorSelectionDialog *colorSelectionDialog;

    uint32_t possibleSquareColorCount;
    uint32_t possibleSquareColorBufferSize;
    uint32_t *possibleSquareColors;

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static uint32_t *qImageToBitmapData(QImage *image);
    static bool getColorFromHexString(const char *str, uint32_t &color);
    int getBorderPixel(uint32_t color,int startX,int startY,Direction direction,bool checkNeighbors,int pixelTolerance=0,int cameFromX=-1,int cameFromY=-1);
    // Note: uses originalImageData instead of pixelColorMap
    int getBorderPixelUsingColorTolerance(uint32_t color,int startX,int startY,Direction direction,decimal_t colorTolerance,bool checkNeighbors,int pixelTolerance=0,int cameFromX=-1,int cameFromY=-1);

public slots:
    void browseBtnClicked();
    void loadBtnClicked();
    void fitToWindow();
    void resetZoom();
    void dialogFileSelected(QString path);
    void resetBtnClicked();
    void detectSquaresBtnClicked();
    void removeAllGraphicsItemsExceptPixmapItemFromScene();
    void chooseColorsBtnClicked();
    void colorSelectionDialogAccepted();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
