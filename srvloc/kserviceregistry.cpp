/*
 *  Interface to register SLP services.
 *  Copyright (C) 2002 Tim Jansen <tim@tjansen.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include "kserviceregistry.h"
#include <kdebug.h>

#ifdef HAVE_SLP

void KServiceRegistryRegReport(SLPHandle, 
			       SLPError errcode, 
			       void* cookie) {
	KServiceRegistry *s = (KServiceRegistry*) cookie;
	s->m_cbSuccess = (errcode == SLP_OK);
	if (errcode < 0)
		kdDebug() << "KServiceRegistry: error in callback:" << errcode <<endl;
}


KServiceRegistry::KServiceRegistry(const QString &lang) :
	m_opened(false),
	m_lang(lang) {
}

KServiceRegistry::~KServiceRegistry() {
	if (m_opened)
		SLPClose(m_handle);
}

bool KServiceRegistry::ensureOpen() {
	SLPError e;

	if (m_opened)
		return true;

	e = SLPOpen(m_lang.latin1(), SLP_FALSE, &m_handle);
	if (e != SLP_OK) {
		kdDebug() << "KServiceRegistry: error while opening:" << e <<endl;
		return false;
	}
	m_opened = true;
	return true;
}

bool KServiceRegistry::available() {
	return ensureOpen();
}

bool KServiceRegistry::registerService(const QString &serviceURL, 
				       QString attributes, 
				       unsigned short lifetime) {
	if (!ensureOpen())
		return false;

	m_cbSuccess = true;
	SLPError e = SLPReg(m_handle,
			    serviceURL.latin1(),
			    lifetime > 0 ? lifetime : SLP_LIFETIME_MAXIMUM,
			    0,
			    attributes.isNull() ? "" : attributes.latin1(),
			    SLP_TRUE,
			    KServiceRegistryRegReport,
			    this);
	if (e != SLP_OK) {
		kdDebug() << "KServiceRegistry: error in registerService:" << e <<endl;
		return false;
	}
	return m_cbSuccess;
}

bool KServiceRegistry::registerService(const QString &serviceURL, 
				       QMap<QString,QString> attributes, 
				       unsigned short lifetime) {
	if (!ensureOpen())
		return false;
// TODO: encode strings in map?
	QString s;
	QMap<QString,QString>::iterator it = attributes.begin();
	while (it != attributes.end()) {
		if (!s.isEmpty())
			s += ",";
		s += QString("(%1=%2)").arg(it.key()).arg(it.data());
		it++;
	}
	return registerService(serviceURL, s, lifetime);
}

void KServiceRegistry::unregisterService(const QString &serviceURL) {
	if (!m_opened)
		return;
	SLPDereg(m_handle, serviceURL.latin1(), 
		 KServiceRegistryRegReport,
		 this);		 
}

#else

KServiceRegistry::KServiceRegistry(const QString &lang) :
	m_opened(false),
	m_lang(lang) {
}

KServiceRegistry::~KServiceRegistry() {
}

bool KServiceRegistry::ensureOpen() {
	return false;
}

bool KServiceRegistry::available() {
	return false;
}

bool KServiceRegistry::registerService(const QString &, QString, unsigned short ) {
	return false;
}

bool KServiceRegistry::registerService(const QString &, QMap<QString,QString>, unsigned short) {
	return false;
}

void KServiceRegistry::unregisterService(const QString &) {
}

#endif