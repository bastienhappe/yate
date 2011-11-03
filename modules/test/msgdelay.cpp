/*
 * msgdelay.cpp
 * This file is part of the YATE Project http://YATE.null.ro
 *
 * An arbitrary message delayer
 *
 * Yet Another Telephony Engine - a fully featured software PBX and IVR
 * Copyright (C) 2011 Null Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <yatengine.h>

using namespace TelEngine;
namespace { // anonymous

class DelayHandler : public MessageHandler
{
public:
    DelayHandler(int prio) : MessageHandler(0,prio) { }
    virtual bool received(Message &msg);
};

class MsgDelay : public Plugin
{
public:
    MsgDelay();
    virtual ~MsgDelay();
    virtual void initialize();
    bool unload();
private:
    DelayHandler* m_handler;
};

INIT_PLUGIN(MsgDelay);

UNLOAD_PLUGIN(unloadNow)
{
    if (unloadNow)
        return __plugin.unload();
    return true;
}


bool DelayHandler::received(Message &msg)
{
    NamedString* p = msg.getParam(YSTRING("message_delay"));
    if (!p)
	return false;
    int ms = p->toInteger();
    // make sure we don't get here again
    msg.clearParam(p);
    if (ms > 0) {
	// delay maximum 10s
	if (ms > 10000)
	    ms = 10000;
	Debug(DebugAll,"Delaying '%s' by %d ms in thread '%s'",msg.safe(),ms,Thread::currentName());
	Thread::msleep(ms);
    }
    return false;
};


MsgDelay::MsgDelay()
    : Plugin("msgdelay","misc"),
      m_handler(0)
{
    Output("Loaded module MsgDelay");
}

MsgDelay::~MsgDelay()
{
    Output("Unloading module MsgDelay");
}

bool MsgDelay::unload()
{
    if (m_handler) {
	Engine::uninstall(m_handler);
	TelEngine::destruct(m_handler);
    }
    return true;
}

void MsgDelay::initialize()
{
    if (!m_handler) {
	int prio = Engine::config().getIntValue("general","msgdelay",50);
	if (prio > 0) {
	    Output("Initializing module MsgDelay priority %d",prio);
	    m_handler = new DelayHandler(prio);
	    Engine::install(m_handler);
	}
    }
}

}; // anonymous namespace

/* vi: set ts=8 sw=4 sts=4 noet: */