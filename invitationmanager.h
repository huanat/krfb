/* This file is part of the KDE project
   Copyright (C) 2007 Alessandro Praduroux <pradu@pradu.it>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version 2
   of the License.
*/
#ifndef INVITATIONMANAGER_H
#define INVITATIONMANAGER_H

#include <QObject>
#include <QList>
#include "invitation.h"


class InvitationManagerPrivate;
/**
	@author Alessandro Praduroux <pradu@pradu.it>
*/
class InvitationManager : public QObject
{
Q_OBJECT
friend class InvitationManagerPrivate;
public:
    static InvitationManager *self();

    ~InvitationManager();

    Invitation addInvitation();

    int activeInvitations();

    void removeInvitation(const Invitation &inv);
    void removeAllInvitations();

    const QList<Invitation> &invitations();

signals:
    void invitationNumChanged(int);

public Q_SLOTS:

    void loadInvitations();
    void saveInvitations();

private:

    void invalidateOldInvitations();
    InvitationManager();
    static InvitationManager *_self;

    QList<Invitation> invitationList;


};

#endif