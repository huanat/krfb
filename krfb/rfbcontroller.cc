/***************************************************************************
                              rfbcontroller.cpp
                             -------------------
    begin                : Sun Dec 9 2001
    copyright            : (C) 2001 by Tim Jansen
    email                : tim@tjansen.de
 ***************************************************************************/

/***************************************************************************
 * Contains portions & concept from rfb's x0rfbserver.cc
 * Copyright (C) 2000 heXoNet Support GmbH, D-66424 Homburg.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "rfbcontroller.h"

#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>

#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qwindowdefs.h>
#include <qtimer.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qglobal.h>

RFBController::RFBController(Configuration *c) :
	configuration(c),
	socket(0),
	connection(0),
	idleUpdateScheduled(false)
{
	connect(dialog.acceptConnectionButton, SIGNAL(clicked()),
		SLOT(dialogAccepted()));
	connect(dialog.refuseConnectionButton, SIGNAL(clicked()),
		SLOT(dialogRefused()));

	startServer();
}

RFBController::~RFBController() {
	if (serversocket) 
		delete serversocket;
	if (socket)
		delete socket;
	if (connection) 
		delete connection;
}

void RFBController::startServer() {
	serversocket = new KServerSocket(configuration->port(), false);
	connect(serversocket, SIGNAL(accepted(KSocket*)), SLOT(accepted(KSocket*)));
	if (!serversocket->bindAndListen()) {
		delete serversocket;
		serversocket = 0;
		KMessageBox::error(0, 
				   i18n("KRfb Server cannot run, port %1 is already in use. ")
				     .arg(configuration->port()),
				   i18n("KRfb Error"));
	}
}

RFBState RFBController::state() {
	if (!serversocket)
		return RFB_ERROR;
	if (!socket)
		return RFB_WAITING;
	if (!connection)
		return RFB_CONNECTING;
	return RFB_CONNECTED;
}

void RFBController::rebind() {
	delete serversocket;
	startServer();
}

/* called when KServerSocket accepted a connection.
   Refuses the connection if there is already a connection.
 */
void RFBController::accepted(KSocket *s) {
	int sockFd = s->socket();
	if (sockFd < 0)
		kdError() << "Got negative socket (error)" << endl;

	if (socket) {
		kdWarning() << "refuse 2nd connection" << endl;
		delete s;
		return;
	}

	int one = 1;
	setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, 
		   (char *)&one, sizeof(one));
	fcntl(sockFd, F_SETFL, O_NONBLOCK);
	socket = s;

	if (configuration->askOnConnect()) {
		dialog.allowRemoteControlCB->setChecked(
			configuration->allowDesktopControl());
		dialog.show();
	}
	else {
		acceptConnection(configuration->allowDesktopControl());
	}
}

void RFBController::acceptConnection(bool allowDesktopControl) {
	KSocket *s = socket;
	connect(s, SIGNAL(readEvent(KSocket*)), SLOT(socketReadable()));
	connect(s, SIGNAL(writeEvent(KSocket*)), SLOT(socketWritable()));
	connect(s, SIGNAL(closeEvent(KSocket*)), SLOT(closeSession()));
	s->enableRead(true);
	s->enableWrite(true);
	connection = new RFBConnection(qt_xdisplay(), s->socket(), 
				       configuration->password(),
				       allowDesktopControl,
				       false);

	emit sessionEstablished();
}

void RFBController::idleSlot() {
	idleUpdateScheduled = false;
	if (connection) {
                connection->scanUpdates();
		connection->sendIncrementalFramebufferUpdate();
		checkWriteBuffer();
	}
}

void RFBController::checkWriteBuffer() {
	BufferedConnection *bc = connection->bufferedConnection;	
	bool bufferEmpty = (bc->senderBuffer.end - bc->senderBuffer.pos) == 0;
       	socket->enableWrite(!bufferEmpty);
	if (bufferEmpty && !idleUpdateScheduled && connection) {
		QTimer::singleShot(0, this, SLOT(idleSlot()));
		idleUpdateScheduled = true;
	}
}

void RFBController::socketReadable() {
	ASSERT(socket);
	int fd = socket->socket();
	BufferedConnection *bc = connection->bufferedConnection;
	int count = read(fd,
			 bc->receiverBuffer.data,
                         bc->receiverBuffer.size);
	if (count >= 0)
		bc->receiverBuffer.end += count;
	else {
		closeSession();
		KMessageBox::error(0, 
				   i18n("An error occurred while reading from the remote client. The connection will be terminated."),
				   i18n("KRfb Error"));
	}
	while (connection->currentState && bc->hasReceiverBufferData()) {
		connection->update();
		checkWriteBuffer();
	}
	bc->receiverBuffer.pos = 0;
	bc->receiverBuffer.end = 0;

	if (!connection->currentState)
		closeSession();
}

void RFBController::socketWritable() {
	ASSERT(socket);
	int fd = socket->socket();
	BufferedConnection *bc = connection->bufferedConnection;
	ASSERT((bc->senderBuffer.end - bc->senderBuffer.pos) > 0);
	int count = write(fd,
			  bc->senderBuffer.data + bc->senderBuffer.pos,
			  bc->senderBuffer.end - bc->senderBuffer.pos);
	if (count >= 0) {
		bc->senderBuffer.pos += count;
	} else {
		KMessageBox::error(0, 
				   i18n("An error occurred while writing to the remote client. The connection will be terminated."),
				   i18n("KRfb Error"));
	}

	checkWriteBuffer();
}

void RFBController::closeSession() {
	if (!socket)
		return;
	if (connection) {
		delete connection;
		connection = 0;
		emit sessionFinished();
	}
	closeSocket();
}

void RFBController::dialogAccepted() {
	ASSERT(socket);
	ASSERT(!connection);

	dialog.hide();
	acceptConnection(dialog.allowRemoteControlCB->isChecked());
}

void RFBController::dialogRefused() {
	ASSERT(socket);
	ASSERT(!connection);
	
	closeSocket();
	dialog.hide();
	emit sessionRefused();
}

void RFBController::closeSocket() {
	delete socket;
	socket = 0;
}

#include "rfbcontroller.moc"