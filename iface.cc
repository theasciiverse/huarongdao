/* $Id: iface.cc,v 1.6 2004/04/25 11:07:12 qhuo Exp $ */

/*  
    hrd -- The puzzle game: HuaRongDao -- http://hrd.sourceforge.net/
    Copyright (C) 2004 by Qingning Huo <qhuo@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "hrd.h"

Interface::Interface(const Board* board)
	: m_error(0), m_board(board), m_steps(0), m_event(0)
{
}

Interface::~Interface()
{
}

int
Interface::error() const
{
	return m_error;
}

void
Interface::set_board(const Board* board)
{
	m_board = board;
}

void
Interface::set_steps(unsigned steps)
{
	m_steps = steps;
}

void
Interface::set_prompt(const string& prompt)
{
	m_prompt = prompt;
}

void
Interface::echo(char c)
{
	m_echo += c;
}

void
Interface::clear_echo(char c)
{
	m_echo.resize(0);
	if (c)
		m_echo += c;
}

int
Interface::get_event()
{
	int rc;
	if (m_event) {
		rc = m_event;
		m_event = 0;
	} else {
		rc = next_event();
	}

	return rc;
}

// XXX: only one event cache.
void
Interface::unget_event(int event)
{
	m_event = event;
}

