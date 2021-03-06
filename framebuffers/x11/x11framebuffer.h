/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
*/
#ifndef KRFB_FRAMEBUFFER_X11_X11FRAMEBUFFER_H
#define KRFB_FRAMEBUFFER_X11_X11FRAMEBUFFER_H

#include <framebuffer.h>
#include <QWidget>

class X11FrameBuffer;
typedef union _XEvent XEvent;

class EvWidget: public QWidget
{
    Q_OBJECT

public:
    EvWidget(X11FrameBuffer *x11fb);

protected:
    bool x11Event(XEvent *event);

private:
    X11FrameBuffer *fb;
    int xdamageBaseEvent;
};

/**
    @author Alessandro Praduroux <pradu@pradu.it>
*/
class X11FrameBuffer : public FrameBuffer
{
    Q_OBJECT
public:
    X11FrameBuffer(WId id, QObject *parent = 0);

    ~X11FrameBuffer();

    QList<QRect> modifiedTiles() override;
    int depth() override;
    int height() override;
    int width() override;
    int paddedWidth() override;
    void getServerFormat(rfbPixelFormat &format) override;
    void startMonitor() override;
    void stopMonitor() override;


    void handleXDamage(XEvent *event);
private:
    void cleanupRects();
    void acquireEvents();

    class P;
    P *const d;
};

#endif
