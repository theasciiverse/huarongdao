/* $Id: board.cc,v 1.9 2004/04/15 20:10:23 qhuo Exp $ */

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

////////////////////////////////////////////////////////////////////////
//
// Board
//
////////////////////////////////////////////////////////////////////////
Board::Board(const string& s)
	: m_pattern(s), m_error(ERROR_UNKNOWN), m_selected(-1)
{
	for (int i=0; i<NPIECE; ++i)
		m_piece[i]=0;
	for (int i=0; i<WIDTH; ++i)
		for (int j=0; j<HEIGHT; ++j)
			m_table[i][j]=0;
	m_error = init(s);
}
	
Board::~Board()
{
	for (int i=0; i<NPIECE; ++i)
		delete m_piece[i];
}

const string
Board::get_pattern() const
{
	return m_pattern;
}

void
Board::set_selected(int i)
{
	m_selected = i;
}

int
Board::get_selected() const
{
	return m_selected;
}

int
Board::error() const
{
	return m_error;
}

Board::Error
Board::init(const string& s)
{
	if (s.length() != (WIDTH+1) * HEIGHT)
		return ERROR_LENGTH;

	int x=0, y=0, next=0;
	int qc=0, lc=0, tc=0, pc=0;
	for (unsigned i=0; i<s.length(); ++i) {
		if ((i+1) % (WIDTH+1) == 0) {
			if (s[i] != '.')
				return ERROR_NOTDOT;
			else
				continue;
		}
		
		if (y >= HEIGHT)
			return ERROR_YOVERFLOW;

		Piece* pp;

		if ((pp = m_table[x][y]) != 0) {
			if (pp->conf_char() == s[i])
				goto NEXT_BOX;
			else
				return ERROR_OVERLAP;
		}
		
		switch (s[i]) {
		default:
			return ERROR_CHAR;
		case '_':
			break;
		case 'Q':
			pp = new Piece_QQQQ(m_table, x, y);
			++qc;
			goto QLTP;
		case 'L':
			pp = new Piece_LR(m_table, x, y);
			++lc;
			goto QLTP;
		case 'T':
			pp = new Piece_TB(m_table, x, y);
			++tc;
			goto QLTP;
		case 'P':
			pp = new Piece_P(m_table, x, y);
			++pc;
			goto QLTP;
		QLTP:
			if (Error error = pp->error())
				return error;
			m_piece[next] = pp;
			if (++next > NPIECE)
				return ERROR_OVERFLOW;
			break;
		}
NEXT_BOX:
		if (++x >= WIDTH) {
			x = 0;
			++y;
		}
	}

	if (qc==1 && lc+tc==5 && pc==4)
		return ERROR_NOERROR;
	else
		return ERROR_NUMBER;
}

bool
Board::is_solved() const
{
	return m_table[1][4] && m_table[1][4]->conf_char() == 'Q'
		&& m_table[2][4] == m_table[1][4];
}

int
Board::can_move(int index)
{
	if (Piece* pp = m_piece[index]) {
		return static_cast<int>(pp->can_move(D_LEFT)) + 
			static_cast<int>(pp->can_move(D_RIGHT)) +
			static_cast<int>(pp->can_move(D_UP)) +
			static_cast<int>(pp->can_move(D_DOWN));
	} else {
		return false;
	}
}

bool
Board::can_move(int index, Dir dir)
{
	if (Piece* pp = m_piece[index])
		return pp->can_move(dir);
	else
		return false;
}

// XXX: no check is done here, 
// caller must make sure can_make() returns true.
void
Board::do_move(const Move& m)
{
	m_piece[m.m_index]->do_move(m.m_dir);
	if (m.m_dir2)
		m_piece[m.m_index]->do_move(m.m_dir2);
}

// XXX: as above, caller must make sure the call will succeed.
void
Board::undo_move(const Move& m)
{
	if (m.m_dir2)
		m_piece[m.m_index]->do_move(-m.m_dir2);
	m_piece[m.m_index]->do_move(-m.m_dir);
}

void
Board::get_rowcol(int index, int *row, int *col) const
{
	const Piece* pp = m_piece[index];
	pp->get_rowcol(row, col);
}

char
Board::get_type(int index) const
{
	const Piece* pp = m_piece[index];
	return pp->conf_char();
}


////////////////////////////////////////////////////////////////////////
//
// Board::Piece
//
////////////////////////////////////////////////////////////////////////
Board::Piece::Piece(PPieceTable& pt, int col, int row)
	: m_pt(pt), m_col(col), m_row(row), m_error(ERROR_NOERROR)
{
}

Board::Piece::~Piece()
{
}

Board::Error
Board::Piece::error() const
{
	return m_error;
}

void
Board::Piece::get_rowcol(int *row, int *col) const
{
	*row = m_row;
	*col = m_col;
}

////////////////////////////////////////////////////////////////////////
//
// Board::Piece subclasses
//
////////////////////////////////////////////////////////////////////////
Board::Piece_QQQQ::Piece_QQQQ(PPieceTable& pt, int c, int r)
	: Piece(pt, c, r)
{
	//printf("placing QQQQ at (r=%d,c=%d)\n", r, c);
	if (c+1 >= WIDTH && r+1 >= HEIGHT) {
		m_error = ERROR_OVERFLOW;
	} else if (pt[c][r] || pt[c][r+1] ||
			pt[c+1][r] || pt[c+1][r+1]) {
		m_error = ERROR_OVERLAP;
	} else {
		pt[c][r] = pt[c][r+1] = this;
		pt[c+1][r] = pt[c+1][r+1] = this;
	}
}

Board::Piece_LR::Piece_LR(PPieceTable& pt, int c, int r)
	: Piece(pt, c, r)
{
	//printf("placing LR at (r=%d,c=%d)\n", r, c);
	if (c+1 >= WIDTH) {
		m_error = ERROR_OVERFLOW;
	} else if (pt[c][r] || pt[c+1][r]) {
		m_error = ERROR_OVERLAP;
	} else {
		pt[c][r] = pt[c+1][r] = this;
	}
}

Board::Piece_TB::Piece_TB(PPieceTable& pt, int c, int r)
	: Piece(pt, c, r)
{
	//printf("placing TB at (r=%d,c=%d)\n", r, c);
	if (r+1 >= HEIGHT) {
		m_error = ERROR_OVERFLOW;
	} else if (pt[c][r] || pt[c][r+1]) {
		m_error = ERROR_OVERLAP;
	} else {
		pt[c][r] = pt[c][r+1] = this;
	}
}

Board::Piece_P::Piece_P(PPieceTable& pt, int c, int r)
	: Piece(pt, c, r)
{
	//printf("placing P at (r=%d,c=%d)\n", r, c);
	if (pt[c][r]) {
		m_error = ERROR_OVERLAP;
	} else {
		pt[c][r] = this;
	}
}

bool
Board::Piece_QQQQ::can_move(Dir dir) const
{
	bool rc = false;

	switch (dir) {
	case D_LEFT: 
		rc = m_col>0 && !m_pt[m_col-1][m_row] && !m_pt[m_col-1][m_row+1];
		break;
	case D_RIGHT: 
		rc = m_col+2<WIDTH && !m_pt[m_col+2][m_row] && !m_pt[m_col+2][m_row+1];
		break;
	case D_UP: 
		rc = m_row>0 && !m_pt[m_col][m_row-1] && !m_pt[m_col+1][m_row-1];
		break;
	case D_DOWN: 
		rc = m_row+2<HEIGHT && !m_pt[m_col][m_row+2] && !m_pt[m_col+1][m_row+2];
		break;
	default:
		break;
	}

	return rc;
}

bool
Board::Piece_LR::can_move(Dir dir) const
{
	bool rc = false;

	switch (dir) {
	case D_LEFT: 
		rc = m_col>0 && !m_pt[m_col-1][m_row];
		break;
	case D_RIGHT: 
		rc = m_col+2<WIDTH && !m_pt[m_col+2][m_row];
		break;
	case D_UP: 
		rc = m_row>0 && !m_pt[m_col][m_row-1] && !m_pt[m_col+1][m_row-1];
		break;
	case D_DOWN: 
		rc = m_row+1<HEIGHT && !m_pt[m_col][m_row+1] && !m_pt[m_col+1][m_row+1];
		break;
	default:
		break;
	}

	return rc;
}

bool
Board::Piece_TB::can_move(Dir dir) const
{
	bool rc = false;

	switch (dir) {
	case D_LEFT: 
		rc = m_col>0 && !m_pt[m_col-1][m_row] && !m_pt[m_col-1][m_row+1];
		break;
	case D_RIGHT: 
		rc = m_col+1<WIDTH && !m_pt[m_col+1][m_row] && !m_pt[m_col+1][m_row+1];
		break;
	case D_UP: 
		rc = m_row>0 && !m_pt[m_col][m_row-1];
		break;
	case D_DOWN: 
		rc = m_row+2<HEIGHT && !m_pt[m_col][m_row+2];
		break;
	default:
		break;
	}

	return rc;
}

bool
Board::Piece_P::can_move(Dir dir) const
{
	bool rc = false;

	switch (dir) {
	case D_LEFT: 
		rc = m_col>0 && !m_pt[m_col-1][m_row];
		break;
	case D_RIGHT: 
		rc = m_col+1<WIDTH && !m_pt[m_col+1][m_row];
		break;
	case D_UP: 
		rc = m_row>0 && !m_pt[m_col][m_row-1];
		break;
	case D_DOWN: 
		rc = m_row+1<HEIGHT && !m_pt[m_col][m_row+1];
		break;
	default:
		break;
	}

	return rc;
}

void
Board::Piece_QQQQ::do_move(Dir dir)
{
	switch (dir) {
	case D_LEFT:
		m_pt[m_col-1][m_row] = m_pt[m_col-1][m_row+1] = this;
		m_pt[m_col+1][m_row] = m_pt[m_col+1][m_row+1] = 0;
		--m_col;
		break;
	case D_RIGHT:
		m_pt[m_col+2][m_row] = m_pt[m_col+2][m_row+1] = this;
		m_pt[m_col][m_row] = m_pt[m_col][m_row+1] = 0;
		++m_col;
		break;
	case D_UP:
		m_pt[m_col][m_row-1] = m_pt[m_col+1][m_row-1] = this;
		m_pt[m_col][m_row+1] = m_pt[m_col+1][m_row+1] = 0;
		--m_row;
		break;
	case D_DOWN:
		m_pt[m_col][m_row+2] = m_pt[m_col+1][m_row+2] = this;
		m_pt[m_col][m_row] = m_pt[m_col+1][m_row] = 0;
		++m_row;
		break;
	default:
		break;
	}
}

void
Board::Piece_LR::do_move(Dir dir)
{
	switch (dir) {
	case D_LEFT:
		m_pt[m_col-1][m_row] = this;
		m_pt[m_col+1][m_row] = 0;
		--m_col;
		break;
	case D_RIGHT:
		m_pt[m_col+2][m_row] = this;
		m_pt[m_col][m_row] = 0;
		++m_col;
		break;
	case D_UP:
		m_pt[m_col][m_row-1] = m_pt[m_col+1][m_row-1] = this;
		m_pt[m_col][m_row] = m_pt[m_col+1][m_row] = 0;
		--m_row;
		break;
	case D_DOWN:
		m_pt[m_col][m_row+1] = m_pt[m_col+1][m_row+1] = this;
		m_pt[m_col][m_row] = m_pt[m_col+1][m_row] = 0;
		++m_row;
		break;
	default:
		break;
	}
}

void
Board::Piece_TB::do_move(Dir dir)
{
	switch (dir) {
	case D_LEFT:
		m_pt[m_col-1][m_row] = m_pt[m_col-1][m_row+1] = this;
		m_pt[m_col][m_row] = m_pt[m_col][m_row+1] = 0;
		--m_col;
		break;
	case D_RIGHT:
		m_pt[m_col+1][m_row] = m_pt[m_col+1][m_row+1] = this;
		m_pt[m_col][m_row] = m_pt[m_col][m_row+1] = 0;
		++m_col;
		break;
	case D_UP:
		m_pt[m_col][m_row-1] = this;
		m_pt[m_col][m_row+1] = 0;
		--m_row;
		break;
	case D_DOWN:
		m_pt[m_col][m_row+2] = this;
		m_pt[m_col][m_row] = 0;
		++m_row;
		break;
	default:
		break;
	}
}

void
Board::Piece_P::do_move(Dir dir)
{
	switch (dir) {
	case D_LEFT:
		m_pt[m_col-1][m_row] = this;
		m_pt[m_col][m_row] = 0;
		--m_col;
		break;
	case D_RIGHT:
		m_pt[m_col+1][m_row] = this;
		m_pt[m_col][m_row] = 0;
		++m_col;
		break;
	case D_UP:
		m_pt[m_col][m_row-1] = this;
		m_pt[m_col][m_row] = 0;
		--m_row;
		break;
	case D_DOWN:
		m_pt[m_col][m_row+1] = this;
		m_pt[m_col][m_row] = 0;
		++m_row;
		break;
	default:
		break;
	}
}


