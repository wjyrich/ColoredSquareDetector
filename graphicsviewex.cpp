#include "graphicsviewex.h"

GraphicsViewEx::GraphicsViewEx(QWidget *parent)
    : QGraphicsView(parent)
{
    setMouseTracking(true);
    setScene(new GraphicsSceneEx(this)); // We need to pass a parent!
    posMap=new QMap<QGraphicsItem*,QPoint>();
    defaultHandlers=true;
    zoomFactor=1.0;
    dragging=false;
    startPoint=QPoint();
}

void GraphicsViewEx::wheelEvent(QWheelEvent *e)
{
    if(!defaultHandlers)
    {
        wheelEx(e);
        return;
    }
    if(dragging)
        dragging=false;
    if(e->delta()>0)
    {
        double newZoomFactor=zoomFactor+(zoomFactor/10);
        scale(newZoomFactor/zoomFactor,newZoomFactor/zoomFactor);
        zoomFactor=newZoomFactor;
    }
    else
    {
        double newZoomFactor=zoomFactor-(zoomFactor/10);
        scale(newZoomFactor/zoomFactor,newZoomFactor/zoomFactor);
        zoomFactor=newZoomFactor;
    }
    wheelEx(e);
}

void GraphicsViewEx::enterEvent(QEvent *e)
{
    QGraphicsView::enterEvent(e);
    mouseEnterEx(e);
}

void GraphicsViewEx::leaveEvent(QEvent *e)
{
    QGraphicsView::leaveEvent(e);
    if(dragging)
        dragging=false;
    mouseLeaveEx(e);
}

void GraphicsViewEx::mouseMoveEvent(QMouseEvent *e)
{
    qDebug()<<"MDown";
    QGraphicsView::mouseMoveEvent(e);
    if(!defaultHandlers)
        goto EmitEvent;
    if(dragging)
    {
        int x=offset.x()+(startPoint.x()-e->x());
        int y=offset.y()+(startPoint.y()-e->y());
        int xDelta=abs(x-offset.x());
        int yDelta=abs(y-offset.y());
        if(x<0)
            x=0;
        if(x>horizontalScrollBar()->maximum())
            x=horizontalScrollBar()->maximum();
        if(y<0)
            y=0;
        if(y>verticalScrollBar()->maximum())
            y=verticalScrollBar()->maximum();

        if(xDelta<10&&yDelta<10)
            goto EmitEvent;
        // Hack.
        horizontalScrollBar()->setValue(x);
        verticalScrollBar()->setValue(y);
    }
    EmitEvent:
    mouseMoveEx(e);
    qDebug()<<"MDown X";
}

void GraphicsViewEx::mousePressEvent(QMouseEvent *e)
{
    //QGraphicsView::mousePressEvent(e); // This causes the GraphicsViewEx to freeze when too many items are in the scene. Disable it, as it is not needed.
    if(zoomFactor*scene()->width()<geometry().width()&&zoomFactor*scene()->height()<geometry().height()) // geometry: boundaries
        goto SkipDragActions;
    dragging=true;
    startPoint=e->pos();
    posMap->clear();
    offset=QPoint(horizontalScrollBar()->value(),verticalScrollBar()->value());
    SkipDragActions:
    mouseDownEx(e);
}

void GraphicsViewEx::mouseReleaseEvent(QMouseEvent *e)
{
    //QGraphicsView::mouseReleaseEvent(e); // See comment above.
    dragging=false;
    mouseUpEx(e);
}

void GraphicsViewEx::mouseDoubleClickEvent(QMouseEvent *e)
{
    QGraphicsView::mouseDoubleClickEvent(e);
    mouseDoubleClickEx(e);
}

void GraphicsViewEx::dropEvent(QDropEvent *e)
{
    dropEx(e);
}

void GraphicsViewEx::setZoomFactor(double newZoomFactor)
{
    scale(newZoomFactor/zoomFactor,newZoomFactor/zoomFactor);
    zoomFactor=newZoomFactor;
}

void GraphicsViewEx::resetZoom()
{
    setZoomFactor(1.0);
}
