/* $Id: game.cc,v 1.12 2004/04/18 20:32:13 qhuo Exp $ */

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

char
dir2char(Board::Dir dir)
{
	switch (dir) {
		case Board::D_LEFT:
			return 'l';
		case Board::D_RIGHT:
			return 'r';
		case Board::D_UP:
			return 'u';
		case Board::D_DOWN:
			return 'd';
		default:
			return 0;
	}
}

Board::Dir
char2dir(int ch)
{
	switch (ch) {
		case 'l':
		case KEY_LEFT:
			return Board::D_LEFT;
		case 'r':
		case KEY_RIGHT:
			return Board::D_RIGHT;
		case 'u':
		case KEY_UP:
			return Board::D_UP;
		case 'd':
		case KEY_DOWN:
			return Board::D_DOWN;
		default:
			return Board::D_NONE;
	}
}

Game::Game(Interface* iface, Board* board)
	: m_iface(iface), m_board(board), m_steps(0)
{
	m_history.reserve(128);
}

Game::~Game()
{
	delete m_board;
	delete m_iface;
}

int
Game::loop()
{
	m_iface->set_prompt("Ready.");
	m_iface->draw();

	for (;;) {
		int ch = m_iface->get_event();

		if (ch == HRD_CTRL_C) {
			m_iface->set_prompt("Please use command \"q\" to exit!");
			m_iface->draw();
		}

		else if (isdigit(ch)) {
			on_select(ch);
			if (m_board->is_solved()) {
				m_iface->set_prompt("Solved! Well done!");
				m_iface->draw();
				(void) m_iface->get_event();
				return 0;
			}
		}

		else if (ch == 'q') {
			m_iface->set_prompt("Quit game? (y/N)");
			m_iface->clear_echo(ch);
			m_iface->draw();
			if ('y' == m_iface->get_event())
				return 0;
			m_iface->set_prompt("Good.");
			m_iface->clear_echo();
			m_iface->draw();
		}
		
		else switch (ch) {
			case '\n':
				on_enter(ch);
				break;
			case 'u':
				on_undo(ch);
				break;
			case 'r':
				on_redo(ch);
				break;
			case 'b':
				on_bsave(ch);
				break;
			case 'l':
				on_bload(ch);
				break;
			default:
				flash();
				break;
		}
	}
}

void
Game::on_select(int ch0)
{
	int i = ch0 - '0';
	if (i < 0 || i > 9)
		return;

	m_board->set_selected(i);

	m_iface->set_prompt("Selected.");
	m_iface->clear_echo(ch0);
	m_iface->draw();

read:
	int ch = m_iface->get_event();
	Board::Dir dir;

	if (ch == 'q' || ch == HRD_CTRL_C) {
		m_iface->set_prompt("Cancelled.");
	} else if (isdigit(ch)) {
		m_iface->unget_event(ch);
		return;
	} else if (ch == '\n') {
		m_iface->set_prompt("Ready.");
	} else if ((dir = char2dir(ch)) != 0) {
		m_iface->echo(dir2char(dir));
		do_move(i, dir);
	} else {
		flash();
		goto read;
	}

	m_board->set_selected(-1);
	m_iface->draw();
}

void
Game::do_move(int index, Board::Dir d)
{
	if (!m_board->can_move(index, d)) {
		m_iface->set_prompt("Cannot move.");
		return;
	}

	Board::Move m(index, d);
	m_board->do_move(m);
	new_move(index, d);
	m_iface->set_steps(m_steps);

	if (m_board->can_move(index) <= 1) {
		m_iface->set_prompt("Moved.");
		return;
	}

	m_iface->set_prompt("Moved. Further move possible.");
	m_iface->draw();

	Board::Dir d2;
	{
again:
		int ch = m_iface->get_event();

		if (ch == 'q' || ch == HRD_CTRL_C || ch == '\n') {
			m_iface->set_prompt("Ready.");
			return;
		} else if (isdigit(ch)) {
			m_iface->unget_event(ch);
			return;
		} else if ((d2 = char2dir(ch)) != 0) {
			m_iface->echo(dir2char(d2));
		} else {
			flash();
			goto again;
		}
	}

	if (!m_board->can_move(index, d2)) {
		m_iface->set_prompt("Cannot move.");
		return;
	}

	Board::Move m2(index, d2);
	m_board->do_move(m2);
	if (old_move(d2)) {
		m_iface->set_prompt("Moved.");
	} else {
		m_iface->set_prompt("Moved back.");
	}
	m_iface->set_steps(m_steps);
	m_iface->draw();
}

void
Game::new_move(int index, Board::Dir dir)
{
	if (m_history.size() > m_steps)
		m_history.resize(m_steps);

	m_history.push_back(Board::Move(index, dir));
	++m_steps;
}

// XXX: old_move() must be called right after new_move()
bool
Game::old_move(Board::Dir dir2)
{
	Board::Move& m = m_history.back();
	if (dir2 == -m.m_dir) {
		m_history.pop_back();
		--m_steps;
		return false;
	} else {
		m.m_dir2 = dir2;
		return true;
	}
}

void
Game::on_enter(int /*c*/)
{
	m_iface->set_prompt("Ready.");
	m_iface->clear_echo();
	m_iface->draw();
}

bool
Game::undo()
{
	if (m_steps > 0) {
		--m_steps;
		return true;
	} else {
		return false;
	}
}

bool
Game::redo()
{
	if (m_steps < m_history.size()) {
		++m_steps;
		return true;
	} else {
		return false;
	}
}

void
Game::on_undo(int c)
{
	if (undo()) {
		m_board->undo_move(m_history[m_steps]);
		m_iface->set_steps(m_steps);
		m_iface->set_prompt("Undone.");
	} else {
		m_iface->set_prompt("Cannot undo.");
	}
	m_iface->clear_echo(c);
	m_iface->redraw();
}

void
Game::on_redo(int c)
{
	if (redo()) {
		m_board->do_move(m_history[m_steps-1]);
		m_iface->set_steps(m_steps);
		m_iface->set_prompt("Redone.");
	} else {
		m_iface->set_prompt("Cannot redo.");
	}
	m_iface->clear_echo(c);
	m_iface->redraw();
}

void
Game::on_bsave(int c)
{
	m_iface->set_prompt("Which slot? (0-9a-zA-Z)");
	m_iface->clear_echo(c);
	m_iface->draw();

	int c2 = m_iface->get_event();
	if (!isalnum(c2)) {
		flash();
		return;
	}

	m_iface->echo(c2);
	if (int error = Bookmark::save((char)c2, m_board->get_pattern(),
				m_history, m_steps)) {
		m_iface->set_prompt(string("Failed: ") + strerror(error) + ".");
	} else {
		m_iface->set_prompt(string("Saved at slot \"") + (char)c2 + "\".");
	}
	m_iface->draw();
}

void
Game::on_bload(int c)
{
	m_iface->set_prompt("Which slot? (0-9a-zA-Z)");
	m_iface->clear_echo(c);
	m_iface->draw();

	int c2 = m_iface->get_event();
	if (!isalnum(c2)) {
		flash();
		return;
	}

	m_iface->echo(c2);

	Board* pb=0;
	vector<Board::Move> mv;

	if (/*int error = */Bookmark::load((char)c2, &pb, &mv)) {
		//m_iface->set_prompt(string("Failed: ") + strerror(error) + ".");
		m_iface->set_prompt("Failed.");
	} else {
		m_history.swap(mv);
		m_steps = m_history.size();
		delete m_board;
		m_board = pb;
		m_iface->set_board(pb);
		m_iface->set_steps(m_steps);
		m_iface->set_prompt(string("Loaded from slot \"") + (char)c2 + "\".");
	}

	m_iface->draw();
}

