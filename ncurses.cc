/* $Id: ncurses.cc,v 1.7 2004/04/15 20:10:23 qhuo Exp $ */

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

NcursesInterface::NcursesInterface(Board* board)
	: Interface(board)
{
	initscr();
	raw();
	keypad(stdscr, TRUE);
	noecho();

	start_color();
	init_pair(CL_PROMPT, COLOR_WHITE, COLOR_BLUE);
	init_pair(CL_ECHO, COLOR_WHITE, COLOR_BLUE);
	init_pair(CL_BOARD, COLOR_BLACK, COLOR_WHITE);
	init_pair(CL_SELECTED, COLOR_RED, COLOR_WHITE);

	curs_set(0);
	erase();

	init();
}

NcursesInterface::~NcursesInterface()
{
	endwin();
	printf("Bye.\n");
}

void
NcursesInterface::init()
{
	getmaxyx(stdscr, m_maxy, m_maxx);
	const int bw = 12 + 2;
	const int bh = 15 + 2;
	m_bdy = (m_maxy - bh)/2;
	m_bdx = (m_maxx - bw)/2;

	m_echo_size = 4;
	m_steps_size = 4;
	m_prompt_size = m_maxx - m_echo_size - m_steps_size;
}

int
NcursesInterface::next_event()
{
	int ch;
	while ((ch = getch()) == HRD_CTRL_L)
		redraw();
	return ch;
}

void
NcursesInterface::draw()
{
	if (m_board) {
		display_board(m_board, m_bdy, m_bdx);
	}

	move(m_maxy-1, 0);
	attrset(COLOR_PAIR(CL_PROMPT));
	printw("%s", m_prompt.c_str());
	for (int i=m_prompt.size(); i<m_prompt_size; ++i)
		addch(' ');

	printw("%s", m_echo.c_str());
	for (int i=m_echo.size(); i<m_echo_size; ++i)
		addch(' ');

	ostringstream os;
	os << (m_steps % 10000);
	string s = os.str();

	while (s.length() < static_cast<size_t>(m_steps_size))
		s = string("0") + s;
	printw("%s", s.c_str());
	
	refresh();
}

void
NcursesInterface::redraw()
{
	init();
	erase();
	draw();
}

void
NcursesInterface::display_board(const Board* pb, int r0, int c0)
{
	attrset(COLOR_PAIR(CL_BOARD));
	display_box(r0, c0);

	++r0; ++c0;

	int i_sel = pb->get_selected();

	for (int i=0; i<Board::NPIECE; ++i) {
		if (i == i_sel)
			attrset(COLOR_PAIR(CL_SELECTED));
		else
			attrset(COLOR_PAIR(CL_BOARD));

		int row, col;
		pb->get_rowcol(i, &row, &col);
		row = r0 + row*3;
		col = c0 + col*3;

		int ch = '0' + i;
		
		switch (pb->get_type(i)) {
			case 'Q':
				display_qqqq(row, col, ch);
				break;
			case 'R':
				display_lr(row, col, ch);
				break;
			case 'B':
				display_tb(row, col, ch);
				break;
			case 'P':
				display_p(row, col, ch);
				break;
		}
	}
}

void
NcursesInterface::display_box(int row, int col)
{
	move(row, col);
	addch(ACS_ULCORNER);
	for (int i=0; i<12; ++i) addch(ACS_HLINE);
	addch(ACS_URCORNER);

	for (int i=0; i<16; ++i) {
		move(row+i+1, col);
		addch(ACS_VLINE);
		for (int j=0; j<12; ++j)
			addch(' ');
		addch(ACS_VLINE);
	}

	move(row+16, col);
	addch(ACS_LLCORNER);
	for (int i=0; i<3; ++i) addch(ACS_HLINE);
	for (int i=0; i<6; ++i) addch(' ');
	for (int i=0; i<3; ++i) addch(ACS_HLINE);
	addch(ACS_LRCORNER);
}

void
NcursesInterface::display_qqqq(int row, int col, char ch)
{
	move(row, col);
	addch(ACS_ULCORNER);
	for (int i=0; i<4; ++i) addch(ACS_HLINE);
	addch(ACS_URCORNER);

	for (int i=0; i<4; ++i) {
		move(row+i+1, col);
		addch(ACS_VLINE);
		for (int j=0; j<4; ++j) addch(' ');
		addch(ACS_VLINE);
	}

	move(row+2, col+2); addch(ch); addch(ch);
	move(row+3, col+2); addch(ch); addch(ch);

	move(row+5, col);
	addch(ACS_LLCORNER);
	for (int i=0; i<4; ++i) addch(ACS_HLINE);
	addch(ACS_LRCORNER);
}

void
NcursesInterface::display_lr(int row, int col, char ch)
{
	move(row, col);
	addch(ACS_ULCORNER);
	for (int i=0; i<4; ++i) addch(ACS_HLINE);
	addch(ACS_URCORNER);

	move(row+1, col);
	addch(ACS_VLINE);
	addch(' '); addch(ch); addch(ch); addch(' ');
	addch(ACS_VLINE);

	move(row+2, col);
	addch(ACS_LLCORNER);
	for (int i=0; i<4; ++i) addch(ACS_HLINE);
	addch(ACS_LRCORNER);
}

void
NcursesInterface::display_tb(int row, int col, char ch)
{
	move(row, col);
	addch(ACS_ULCORNER); addch(ACS_HLINE); addch(ACS_URCORNER);

	for (int i=0; i<4; ++i) {
		move(row+i+1, col);
		addch(ACS_VLINE);
		addch(' ');
		addch(ACS_VLINE);
	}

	move(row+2, col+1); addch(ch);
	move(row+3, col+1); addch(ch);

	move(row+5, col);
	addch(ACS_LLCORNER); addch(ACS_HLINE); addch(ACS_LRCORNER);
}

void
NcursesInterface::display_p(int row, int col, char ch)
{	
	move(row, col);
	addch(ACS_ULCORNER); addch(ACS_HLINE); addch(ACS_URCORNER);

	move(row+1, col);
	addch(ACS_VLINE); addch(ch); addch(ACS_VLINE);

	move(row+2, col);
	addch(ACS_LLCORNER); addch(ACS_HLINE); addch(ACS_LRCORNER);
}

