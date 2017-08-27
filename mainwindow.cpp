#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    dialog=new QFileDialog(this);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    QStringList filters;
    filters<<"All images (*.jpg *.jpeg *.png *.gif *.bmp)"
           <<"JPEG images (*.jpg *.jpeg)"
           <<"PNG images (*.png)"
           <<"GIF images (*.gif)"
           <<"Bitmaps (*.bmp)";
    dialog->setNameFilters(filters);
    dialog->setDirectory(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    connect(dialog,SIGNAL(fileSelected(QString)),this,SLOT(dialogFileSelected(QString)));
    connect(ui->browseBtn,SIGNAL(clicked(bool)),this,SLOT(browseBtnClicked()));
    connect(ui->loadBtn,SIGNAL(clicked(bool)),this,SLOT(loadBtnClicked()));
    connect(ui->fitToWindowBtn,SIGNAL(clicked(bool)),this,SLOT(fitToWindow()));
    connect(ui->resetZoomBtn,SIGNAL(clicked(bool)),this,SLOT(resetZoom()));
    connect(ui->resetBtn,SIGNAL(clicked(bool)),this,SLOT(resetBtnClicked()));
    connect(ui->saveAsBtn,SIGNAL(clicked(bool)),this,SLOT(saveAsBtnClicked()));
    connect(ui->detectSquaresBtn,SIGNAL(clicked(bool)),this,SLOT(detectSquaresBtnClicked()));
    connect(ui->removeMarkingBoxesBtn,SIGNAL(clicked(bool)),this,SLOT(removeAllGraphicsItemsExceptPixmapItemFromScene()));
    connect(ui->chooseColorsBtn,SIGNAL(clicked(bool)),this,SLOT(chooseColorsBtnClicked()));
    image=0;
    originalImageWidth=-1;
    originalImageHeight=-1;
    originalImageWidthM1=-1;
    originalImageHeightM1=-1;
    originalImageData=0;
    pixelColorMap=0;
    markings=new std::vector<QRect>();
    scene=new QGraphicsScene();
    pixmapItem=new QGraphicsPixmapItem();
    scene->addItem(pixmapItem);
    ui->graphicsView->setScene(scene);
    showLargeImageWarning=true;
    colorSelectionDialog=0;

    // Initialize square colors

    possibleSquareColorBufferSize=6;
    possibleSquareColorCount=6;
    possibleSquareColors=(uint32_t*)malloc(possibleSquareColorBufferSize*sizeof(uint32_t));
    possibleSquareColors[0]=0xFF000000; // Black
    possibleSquareColors[1]=0xFF0000FF; // Blue
    possibleSquareColors[2]=0xFF00DD00; // Light green
    possibleSquareColors[3]=0xFF0094FF; // Cyan
    possibleSquareColors[4]=0xFFC3B400; // Yellow
    possibleSquareColors[5]=0xFFFF0000; // Red
}

MainWindow::~MainWindow()
{
    free(originalImageData);
    free(pixelColorMap);
    free(possibleSquareColors);
    delete colorSelectionDialog;
    delete dialog;
    delete markings;
    delete ui;
}

void MainWindow::browseBtnClicked()
{
    dialog->exec();
}

void MainWindow::loadBtnClicked()
{
    QString path=ui->pathBox->text();
    if(path.length()==0)
    {
        QMessageBox::critical(this,"Error","No file selected.");
        return;
    }
    QFile f(path);
    if(!f.exists())
    {
        QMessageBox::critical(this,"Error","The selected file does not exist.");
        return;
    }
    QImage *newImage=new QImage(path);
    if(newImage->isNull())
    {
        QMessageBox::critical(this,"Error","The selected file has an unsupported format.");
        return;
    }
    delete image;
    image=newImage;
    originalImageWidth=image->width();
    originalImageHeight=image->height();
    if(showLargeImageWarning&&originalImageWidth*originalImageHeight>1000000)
    {
        QMessageBox::warning(this,"Warning","Images larger than 1 million pixels take a very long time to process. Consider downsizing the image.");
        showLargeImageWarning=false; // Do not show the warning again
    }
    removeAllGraphicsItemsExceptPixmapItemFromScene();
    originalImageWidthM1=originalImageWidth-1;
    originalImageHeightM1=originalImageHeight-1;
    originalImageData=qImageToBitmapData(image);
    scene->setSceneRect(0,0,originalImageWidth,originalImageHeight);
    pixmapItem->setPixmap(QPixmap::fromImage(*image));
    ui->graphicsView->viewport()->update();
    fitToWindow();
}

void MainWindow::fitToWindow()
{
    if(image==0||image->isNull())
        return;
    int width=image->width();
    int height=image->height();
    QRect rect=ui->graphicsView->contentsRect();
    int availableWidth=rect.width()-ui->graphicsView->verticalScrollBar()->width();
    int availableHeight=rect.height()-ui->graphicsView->horizontalScrollBar()->height();
    if((width-availableWidth)>(height-availableHeight))
        ui->graphicsView->setZoomFactor((float)((float)availableWidth)/((float)width));
    else if(height>availableHeight)
        ui->graphicsView->setZoomFactor((float)((float)availableHeight)/((float)height));
    else
        ui->graphicsView->setZoomFactor(1.0);
}

void MainWindow::resetZoom()
{
    ui->graphicsView->setZoomFactor(1.0);
}

void MainWindow::dialogFileSelected(QString path)
{
    ui->pathBox->setText(path);
    ui->loadBtn->click();
}

void MainWindow::resetBtnClicked()
{
    if(image==0||image->isNull())
        return;

    removeAllGraphicsItemsExceptPixmapItemFromScene();
    delete image;
    image=new QImage((uchar*)originalImageData,originalImageWidth,originalImageHeight,QImage::Format_ARGB32);
    scene->setSceneRect(0,0,originalImageWidth,originalImageHeight);
    pixmapItem->setPixmap(QPixmap::fromImage(*image));
    ui->graphicsView->viewport()->update();
    fitToWindow();
}

void MainWindow::saveAsBtnClicked()
{
    if(image==0||image->isNull())
        return;

    QString path=QFileDialog::getSaveFileName(this,"Save as...",QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),"PNG image (*.png);;JPG image (*.jpg);;GIF image (*.gif);;Bitmap (*.bmp)");
    if(path=="")
        return;

    if(markingsRemoved)
        image->save(path,0,100);
    else
    {
        QImage img=image->copy(0,0,originalImageWidth,originalImageHeight);
        QPainter p(&img);
        p.setPen(QPen(QColor(0xFFFF00FF)));

        size_t markingCount=markings->size();

        for(size_t i=0;i<markingCount;i++)
            p.drawRect(markings->at(i));

        img.save(path,0,100);
    }
}

void MainWindow::detectSquaresBtnClicked()
{
    if(image==0||image->isNull())
        return;

    if(possibleSquareColorCount==0)
    {
        QMessageBox::critical(this,"Error","The list of colors to look for is empty.");
        return;
    }

    removeAllGraphicsItemsExceptPixmapItemFromScene();
    markingsRemoved=false; // Keep this below the removeAllGraphicsItemsExceptPixmapItemFromScene call!
    size_t imageSizeInBytes=originalImageWidth*originalImageHeight;
    free(pixelColorMap);
    pixelColorMap=(uint32_t*)calloc(imageSizeInBytes,sizeof(uint32_t));

    const decimal_t tolerance=ui->toleranceBox->value();
    const int minPixelCountInSquare=ui->minPixelCountInSquareBox->value();

    for(int y=0;y<originalImageHeight;y++)
    {
        int offset=y*originalImageWidth;
        for(int x=0;x<originalImageWidth;x++)
        {
            int pos=offset+x;
            uint32_t color=originalImageData[pos];
            uint32_t nearestColor;
            decimal_t lowestError=std::numeric_limits<decimal_t>::max();
            // Find nearest color, then decide whether to consider this pixel at all
            for(uint32_t i=0;i<possibleSquareColorCount;i++)
            {
                uint32_t possibleColor=possibleSquareColors[i];
                decimal_t error=getColorError(possibleColor,color);
                if(error<lowestError)
                {
                    lowestError=error;
                    nearestColor=possibleColor;
                }
            }
            if(lowestError>tolerance)
            {
                pixelColorMap[offset+x]=0x00000000;
                continue;
            }
            pixelColorMap[offset+x]=nearestColor;
            continue;
        }
    }

    const uint32_t pixelStep=ui->pixelStepBox->value();
    const decimal_t minWidthHeightRatio=ui->minWidthHeightRatioBox->value();
    const decimal_t maxWidthHeightRatio=ui->maxWidthHeightRatioBox->value();

    std::vector<int> maxAreas;
    int markingCount=0;
    markings->clear();

    for(int y=0;y<originalImageHeight;y+=pixelStep)
    {
        int offset=y*originalImageWidth;
        for(int x=0;x<originalImageWidth;x+=pixelStep)
        {
            int pos=offset+x;
            // Check if pixel already in a marking
            bool proceed=true;
            for(int i=0;i<markingCount;i++)
            {
                QRect m=markings->at(i);
                if(m.contains(x,y))
                {
                    proceed=false;
                    break;
                }
            }
            if(!proceed)
                continue;

            uint32_t color=pixelColorMap[pos];

            if(color==0x00000000)
                continue;

            int leftmostX=getBorderPixel(color,x,y,Direction::Left,true,3);
            int rightmostX=getBorderPixel(color,x,y,Direction::Right,true,3);
            int topmostY=getBorderPixel(color,x,y,Direction::Top,true,3);
            int bottommostY=getBorderPixel(color,x,y,Direction::Bottom,true,3);

            if(leftmostX<0||rightmostX<0||topmostY<0||bottommostY<0)
                continue;

            int width=rightmostX-leftmostX;
            int height=bottommostY-topmostY;

            decimal_t ratio=((decimal_t)width)/((decimal_t)height);
            if(ratio<minWidthHeightRatio||ratio>maxWidthHeightRatio)
                continue;

            int area=width*height;
            if(area<minPixelCountInSquare)
                continue;

            int maxArea=round(0.33*((decimal_t)area));
            QRect newRect(leftmostX,topmostY,width,height);

            for(int i=0;i<markingCount;i++)
            {
                QRect m=markings->at(i);
                QRect intersected=m.intersected(newRect);
                int intersectedArea=intersected.width()*intersected.height();
                if(intersectedArea>maxArea||intersectedArea>maxAreas.at(i))
                {
                    proceed=false;
                    break;
                }
            }
            if(!proceed)
                continue;

            markings->push_back(newRect);
            maxAreas.push_back(maxArea);
            markingCount++;
        }
    }

    for(int i=0;i<markingCount;i++)
    {
        QRectF rect=markings->at(i);
        QGraphicsRectItem *item=new QGraphicsRectItem(rect);
        item->setPen(QPen(QColor(0xFFFF00FF)));
        scene->addItem(item);
    }

    delete image;
    image=new QImage((uchar*)pixelColorMap,originalImageWidth,originalImageHeight,QImage::Format_ARGB32);
    scene->setSceneRect(0,0,originalImageWidth,originalImageHeight);
    pixmapItem->setPixmap(QPixmap::fromImage(*image));
    ui->graphicsView->viewport()->update();
    fitToWindow();
}

void MainWindow::removeAllGraphicsItemsExceptPixmapItemFromScene()
{
    markingsRemoved=true;
    QList<QGraphicsItem*> items=scene->items();
    int itemCount=items.count();
    for(int i=0;i<itemCount;i++)
    {
        QGraphicsItem *item=items.at(i);
        if(item==pixmapItem)
            continue;
        scene->removeItem(item);
        delete item;
    }
    ui->graphicsView->viewport()->update();
}

void MainWindow::chooseColorsBtnClicked()
{
    if(colorSelectionDialog==0)
    {
        colorSelectionDialog=new ColorSelectionDialog(this);
        connect(colorSelectionDialog,SIGNAL(accepted()),this,SLOT(colorSelectionDialogAccepted()));
    }
    colorSelectionDialog->show();
}

void MainWindow::colorSelectionDialogAccepted()
{
    possibleSquareColorCount=colorSelectionDialog->getItemCount();
    if(possibleSquareColorBufferSize<possibleSquareColorCount)
    {
        possibleSquareColorBufferSize=possibleSquareColorCount;
        possibleSquareColors=(uint32_t*)realloc(possibleSquareColors,possibleSquareColorBufferSize*sizeof(uint32_t));
    }
    for(uint32_t i=0;i<possibleSquareColorCount;i++)
    {
        uint32_t color=colorSelectionDialog->getItemAt(i)->data(Qt::UserRole).toUInt();
        possibleSquareColors[i]=color;
    }
}

uint32_t *MainWindow::qImageToBitmapData(QImage *image)
{
    int32_t width=image->width();
    int32_t height=image->height();
    uint32_t *out=(uint32_t*)malloc(width*height*sizeof(uint32_t));
    for(int32_t y=0;y<height;y++)
    {
        int32_t offset=y*width;
        QRgb *scanLine=(QRgb*)image->scanLine(y); // Do not free!
        for(int32_t x=0;x<width;x++)
        {
            QRgb color=scanLine[x];
            uint32_t alpha=qAlpha(color);
            uint32_t red=qRed(color);
            uint32_t green=qGreen(color);
            uint32_t blue=qBlue(color);
            out[offset+x]=(alpha<<24)|(red<<16)|(green<<8)|blue;
        }
        // Do not free "scanLine"!
    }
    return out;
}

bool MainWindow::getColorFromHexString(const char *str, uint32_t &color)
{
    int length=strlen(str);
    if(length<3)
        return false;

    if(str[0]=='#')
    {
        str++;
        length--;
    }
    else if(str[0]=='0'&&(str[1]=='x'||str[1]=='X'))
    {
        str+=2;
        length-=2;
    }

    if(length==8) // 0xAARRGGBB; A=255
    {
        char nStr[9];
        nStr[0]=str[0];
        nStr[1]=str[1];
        nStr[2]=str[2];
        nStr[3]=str[3];
        nStr[4]=str[4];
        nStr[5]=str[5];
        nStr[6]=str[6];
        nStr[7]=str[7];
        nStr[8]=0;
        text_t lengthPlaceholder;
        uint8_t *bytes=(uint8_t*)text::bytesFromHexString(nStr,lengthPlaceholder);
        uint32_t a=(uint32_t)bytes[0];
        uint32_t r=(uint32_t)bytes[1];
        uint32_t g=(uint32_t)bytes[2];
        uint32_t b=(uint32_t)bytes[3];
        color=getColor(a,r,g,b);
        free(bytes);
        return true;
    }
    else if(length==6) // 0xRRGGBB; A=255
    {
        char nStr[9];
        nStr[0]='F';
        nStr[1]='F';
        nStr[2]=str[0];
        nStr[3]=str[1];
        nStr[4]=str[2];
        nStr[5]=str[3];
        nStr[6]=str[4];
        nStr[7]=str[5];
        nStr[8]=0;
        text_t lengthPlaceholder;
        uint8_t *bytes=(uint8_t*)text::bytesFromHexString(nStr,lengthPlaceholder);
        uint32_t a=bytes[0];
        uint32_t r=bytes[1];
        uint32_t g=bytes[2];
        uint32_t b=bytes[3];
        color=getColor(a,r,g,b);
        free(bytes);
        return true;
    }
    else if(length==4) // 0xARGB
    {
        char nStr[9];
        nStr[0]=str[0];
        nStr[1]=str[0];
        nStr[2]=str[1];
        nStr[3]=str[1];
        nStr[4]=str[2];
        nStr[5]=str[2];
        nStr[6]=str[3];
        nStr[7]=str[3];
        nStr[8]=0;
        text_t lengthPlaceholder;
        uint8_t *bytes=(uint8_t*)text::bytesFromHexString(nStr,lengthPlaceholder);
        uint32_t a=bytes[0];
        uint32_t r=bytes[1];
        uint32_t g=bytes[2];
        uint32_t b=bytes[3];
        color=getColor(a,r,g,b);
        free(bytes);
        return true;
    }
    else if(length==3) // 0xRGB; A=255
    {
        char nStr[9];
        nStr[0]='F';
        nStr[1]='F';
        nStr[2]=str[0];
        nStr[3]=str[0];
        nStr[4]=str[1];
        nStr[5]=str[1];
        nStr[6]=str[2];
        nStr[7]=str[2];
        nStr[8]=0;
        text_t lengthPlaceholder;
        uint8_t *bytes=(uint8_t*)text::bytesFromHexString(nStr,lengthPlaceholder);
        uint32_t a=bytes[0];
        uint32_t r=bytes[1];
        uint32_t g=bytes[2];
        uint32_t b=bytes[3];
        color=getColor(a,r,g,b);
        free(bytes);
        return true;
    }
    else // Invalid format
        return false;
}

int MainWindow::getBorderPixel(uint32_t color, int startX, int startY, Direction direction, bool checkNeighbors, int pixelTolerance, int cameFromX, int cameFromY)
{
    if(direction==Direction::Left)
    {
        const int offset=startY*originalImageWidth;
        int pixelsOff=0;
        for(int x=startX-1;x>=0;x--)
        {
            uint32_t thisColor=pixelColorMap[offset+x];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&x>0)
                    {
                        for(int _x=__min(x+pixelTolerance,originalImageWidthM1);_x>=x;_x--)
                        {
                            int nV=-1;
                            if(startY>0&&pixelColorMap[(startY-1)*originalImageWidth+_x]==color&&!(cameFromX==_x&&cameFromY==startY-1))
                            {
                                nV=getBorderPixel(color,_x,startY-1,direction,checkNeighbors,pixelTolerance,x,startY);
                            }
                            if(startY<originalImageHeightM1&&pixelColorMap[(startY+1)*originalImageWidth+_x]==color&&!(cameFromX==_x&&cameFromY==startY+1))
                            {
                                int n=getBorderPixel(color,_x,startY+1,direction,checkNeighbors,pixelTolerance,x,startY);
                                if(nV==-1||n<nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV<x)
                                    return nV;
                                else
                                    return x;
                            }
                        }
                    }
                    return x+pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Right)
    {
        const int offset=startY*originalImageWidth;
        int pixelsOff=0;
        for(int x=startX+1;x<=originalImageWidthM1;x++)
        {
            uint32_t thisColor=pixelColorMap[offset+x];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&x<originalImageWidthM1)
                    {
                        for(int _x=__max(x-pixelTolerance,0);_x<=x;_x++)
                        {
                            int nV=-1;
                            if(startY>0&&pixelColorMap[(startY-1)*originalImageWidth+_x]==color&&!(cameFromX==_x&&cameFromY==startY-1))
                            {
                                nV=getBorderPixel(color,_x,startY-1,direction,checkNeighbors,pixelTolerance,x,startY);
                            }
                            if(startY<originalImageHeightM1&&pixelColorMap[(startY+1)*originalImageWidth+_x]==color&&!(cameFromX==_x&&cameFromY==startY+1))
                            {
                                int n=getBorderPixel(color,_x,startY+1,direction,checkNeighbors,pixelTolerance,x,startY);
                                if(/*nV==-1||*/n>nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV>x)
                                    return nV;
                                else
                                    return x;
                            }
                        }
                    }
                    return x-pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return originalImageWidthM1-pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Top)
    {
        int pixelsOff=0;
        for(int y=startY-1;y>=0;y--)
        {
            uint32_t thisColor=pixelColorMap[y*originalImageWidth+startX];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&y>0)
                    {
                        for(int _y=__min(y+pixelTolerance,originalImageHeightM1);_y>=y;_y--)
                        {
                            int nV=-1;
                            if(startX>0&&pixelColorMap[_y*originalImageWidth+startX-1]==color&&!(cameFromX==startX-1&&cameFromY==_y))
                            {
                                nV=getBorderPixel(color,startX-1,_y,direction,checkNeighbors,pixelTolerance,startX,y);
                            }
                            if(startX<originalImageWidthM1&&pixelColorMap[_y*originalImageWidth+startX+1]==color&&!(cameFromX==startX+1&&cameFromY==_y))
                            {
                                int n=getBorderPixel(color,startX+1,_y,direction,checkNeighbors,pixelTolerance,startX,y);
                                if(nV==-1||n<nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV<y)
                                    return nV;
                                else
                                    return y;
                            }
                        }
                    }
                    return y+pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Bottom)
    {
        int pixelsOff=0;
        for(int y=startY+1;y<=originalImageHeightM1;y++)
        {
            uint32_t thisColor=pixelColorMap[y*originalImageWidth+startX];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&y<originalImageHeightM1)
                    {
                        for(int _y=__max(y-pixelTolerance,0);_y<=y;_y++)
                        {
                            int nV=-1;
                            if(startX>0&&pixelColorMap[_y*originalImageWidth+startX-1]==color&&!(cameFromX==startX-1&&cameFromY==_y))
                            {
                                nV=getBorderPixel(color,startX-1,_y,direction,checkNeighbors,pixelTolerance,startX,y);
                            }
                            if(startX<originalImageWidthM1&&pixelColorMap[_y*originalImageWidth+startX+1]==color&&!(cameFromX==startX+1&&cameFromY==_y))
                            {
                                int n=getBorderPixel(color,startX+1,_y,direction,checkNeighbors,pixelTolerance,startX,y);
                                if(/*nV==-1||*/n>nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV>y)
                                    return nV;
                                else
                                    return y;
                            }
                        }
                    }
                    return y-pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return originalImageHeightM1-pixelsOff; // Return last valid pixel
    }
    return -1;
}

int MainWindow::getBorderPixelUsingColorTolerance(uint32_t color, int startX, int startY, Direction direction, decimal_t colorTolerance, bool checkNeighbors, int pixelTolerance, int cameFromX, int cameFromY)
{
    // Note: uses originalImageData instead of pixelColorMap

    if(direction==Direction::Left)
    {
        const int offset=startY*originalImageWidth;
        int pixelsOff=0;
        for(int x=startX-1;x>=0;x--)
        {
            uint32_t thisColor=originalImageData[offset+x];
            if(getColorError(thisColor,color)>colorTolerance)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&x>0)
                    {
                        for(int _x=__min(x+pixelTolerance,originalImageWidthM1);_x>=x;_x--)
                        {
                            int nV=-1;
                            if(startY>0&&getColorError(originalImageData[(startY-1)*originalImageWidth+_x],color)<=colorTolerance&&!(cameFromX==_x&&cameFromY==startY-1))
                            {
                                nV=getBorderPixelUsingColorTolerance(color,_x,startY-1,direction,colorTolerance,checkNeighbors,pixelTolerance,x,startY);
                            }
                            if(startY<originalImageHeightM1&&getColorError(originalImageData[(startY+1)*originalImageWidth+_x],color)<=colorTolerance&&!(cameFromX==_x&&cameFromY==startY+1))
                            {
                                int n=getBorderPixelUsingColorTolerance(color,_x,startY+1,direction,colorTolerance,checkNeighbors,pixelTolerance,x,startY);
                                if(nV==-1||n<nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV<x)
                                    return nV;
                                else
                                    return x;
                            }
                        }
                    }
                    return x+pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Right)
    {
        const int offset=startY*originalImageWidth;
        int pixelsOff=0;
        for(int x=startX+1;x<=originalImageWidthM1;x++)
        {
            uint32_t thisColor=originalImageData[offset+x];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&x<originalImageWidthM1)
                    {
                        for(int _x=__max(x-pixelTolerance,0);_x<=x;_x++)
                        {
                            int nV=-1;
                            if(startY>0&&getColorError(originalImageData[(startY-1)*originalImageWidth+_x],color)<=colorTolerance&&!(cameFromX==_x&&cameFromY==startY-1))
                            {
                                nV=getBorderPixelUsingColorTolerance(color,_x,startY-1,direction,colorTolerance,checkNeighbors,pixelTolerance,x,startY);
                            }
                            if(startY<originalImageHeightM1&&getColorError(originalImageData[(startY+1)*originalImageWidth+_x],color)<=colorTolerance&&!(cameFromX==_x&&cameFromY==startY+1))
                            {
                                int n=getBorderPixelUsingColorTolerance(color,_x,startY+1,direction,colorTolerance,checkNeighbors,pixelTolerance,x,startY);
                                if(/*nV==-1||*/n>nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV>x)
                                    return nV;
                                else
                                    return x;
                            }
                        }
                    }
                    return x-pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return originalImageWidthM1-pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Top)
    {
        int pixelsOff=0;
        for(int y=startY-1;y>=0;y--)
        {
            uint32_t thisColor=originalImageData[y*originalImageWidth+startX];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&y>0)
                    {
                        for(int _y=__min(y+pixelTolerance,originalImageHeightM1);_y>=y;_y--)
                        {
                            int nV=-1;
                            if(startX>0&&getColorError(originalImageData[_y*originalImageWidth+startX-1],color)<=colorTolerance&&!(cameFromX==startX-1&&cameFromY==_y))
                            {
                                nV=getBorderPixelUsingColorTolerance(color,startX-1,_y,direction,colorTolerance,checkNeighbors,pixelTolerance,startX,y);
                            }
                            if(startX<originalImageWidthM1&&getColorError(originalImageData[_y*originalImageWidth+startX+1],color)<=colorTolerance&&!(cameFromX==startX+1&&cameFromY==_y))
                            {
                                int n=getBorderPixelUsingColorTolerance(color,startX+1,_y,direction,colorTolerance,checkNeighbors,pixelTolerance,startX,y);
                                if(nV==-1||n<nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV<y)
                                    return nV;
                                else
                                    return y;
                            }
                        }
                    }
                    return y+pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return pixelsOff; // Return last valid pixel
    }
    else if(direction==Direction::Bottom)
    {
        int pixelsOff=0;
        for(int y=startY+1;y<=originalImageHeightM1;y++)
        {
            uint32_t thisColor=originalImageData[y*originalImageWidth+startX];
            if(thisColor!=color)
            {
                pixelsOff++;
                if(pixelsOff>pixelTolerance)
                {
                    if(checkNeighbors&&y<originalImageHeightM1)
                    {
                        for(int _y=__max(y-pixelTolerance,0);_y<=y;_y++)
                        {
                            int nV=-1;
                            if(startX>0&&getColorError(originalImageData[_y*originalImageWidth+startX-1],color)<=colorTolerance&&!(cameFromX==startX-1&&cameFromY==_y))
                            {
                                nV=getBorderPixelUsingColorTolerance(color,startX-1,_y,direction,colorTolerance,checkNeighbors,pixelTolerance,startX,y);
                            }
                            if(startX<originalImageWidthM1&&getColorError(originalImageData[_y*originalImageWidth+startX+1],color)<=colorTolerance&&!(cameFromX==startX+1&&cameFromY==_y))
                            {
                                int n=getBorderPixelUsingColorTolerance(color,startX+1,_y,direction,colorTolerance,checkNeighbors,pixelTolerance,startX,y);
                                if(/*nV==-1||*/n>nV)
                                    nV=n;
                            }
                            if(nV!=-1)
                            {
                                // Do not check any further pixels
                                if(nV>y)
                                    return nV;
                                else
                                    return y;
                            }
                        }
                    }
                    return y-pixelsOff;
                }
            }
            else if(pixelsOff>0)
                pixelsOff=0;
        }
        if(pixelsOff>0)
            return originalImageHeightM1-pixelsOff; // Return last valid pixel
    }
    return -1;
}
