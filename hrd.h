/* $Id: hrd.h,v 1.17 2004/04/25 11:08:06 qhuo Exp $ */

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

#ifndef HRD_H
#define HRD_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <locale.h>

#include <string>
#include <sstream>
#include <vector>

#ifdef HAVE_CURSES_H
#include <curses.h>
#endif
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif

using std::string;
using std::ostringstream;
using std::vector;

//! isdigit() and isalnum() for curses key codes: the cursor and function
//! keys are above 255, outside what the <ctype.h> functions accept
inline bool key_isdigit(int c) { return c >= 0 && c < 256 && isdigit(c); }
inline bool key_isalnum(int c) { return c >= 0 && c < 256 && isalnum(c); }

////////////////////////////////////////////////////////////////////////
//
// Board, Pieces and etc.
//
////////////////////////////////////////////////////////////////////////
class Board
{
public:
	enum Error {
		ERROR_NOERROR=0,
		ERROR_UNKNOWN,
		ERROR_LENGTH,
		ERROR_NOTDOT,
		ERROR_YOVERFLOW,
		ERROR_OVERLAP,
		ERROR_CHAR,
		ERROR_NUMBER,
		ERROR_OVERFLOW,
	};

	enum Dir {
		D_NONE=0,
		D_LEFT=1,
		D_UP=2,
		D_DOWN=3,
		D_RIGHT=4,
		D_MAX=5,
	};

	enum {
		NPIECE=10,
	};

	struct Move;

public:
	//! Construct a Board from a text string.
	explicit Board(const string&);
	//! Dtor.
	~Board();

	//! Query the pattern string
	const string get_pattern() const;
	
	//! Test whether the Board state is valid.
	int error() const;
	//! Get the error string
	const string error_string() const;

	//! Test whether the board is solverd.
	bool is_solved() const;

	//! Test whether a piece can move
	int can_move(int);
	//! Test whether an move is possible
	bool can_move(int, Dir);
	//! Do move
	void do_move(const Move&);
	//! Undo the move
	void undo_move(const Move&);

	void get_rowcol(int, int*, int*) const;
	char get_type(int) const;

	void set_selected(int);
	int get_selected() const;

private:
	//! Disable copy-ctor and copy-op
	Board(const Board&);
	Board& operator=(const Board&);
	
private:
	enum {
		WIDTH=4,
		HEIGHT=5,
	};

	class Piece;
	class Piece_QQQQ;
	class Piece_LR;
	class Piece_TB;
	class Piece_P;

private:
	Error init(const string&);
	
private:
	const string m_pattern;

	typedef Piece* PPiece;
	typedef PPiece PPieceTable[WIDTH][HEIGHT];
	PPieceTable m_table;
	PPiece m_piece[NPIECE];
	Error m_error;
	int m_selected;
};

struct Board::Move
{
public:
	int m_index;
	Dir m_dir;
	Dir m_dir2;
public:
	Move()
		: m_index(0), m_dir(D_NONE), m_dir2(D_NONE) {}
	Move(int index, Dir dir, Dir dir2=D_NONE)
		: m_index(index), m_dir(dir), m_dir2(dir2) {}
};

class Board::Piece
{
public:
	Piece(PPieceTable& pt, int col, int row);
	virtual ~Piece() =0;
	Error error() const;
	void get_rowcol(int* row, int* col) const;
	virtual char conf_char() const =0;
	virtual bool can_move(Dir) const =0;
	virtual void do_move(Dir) =0;

protected:
	PPieceTable& m_pt;
	int m_col;
	int m_row;
	Error m_error;
};

class Board::Piece_QQQQ : public Board::Piece
{
public:
	Piece_QQQQ(PPieceTable& pt, int col, int row);
	virtual char conf_char() const { return 'Q'; }
	virtual bool can_move(Dir) const;
	virtual void do_move(Dir);
};

class Board::Piece_LR : public Board::Piece
{
public:
	Piece_LR(PPieceTable& pt, int col, int row);
	virtual char conf_char() const { return 'R'; }
	virtual bool can_move(Dir) const;
	virtual void do_move(Dir);
};

class Board::Piece_TB : public Board::Piece
{
public:
	Piece_TB(PPieceTable& pt, int col, int row);
	virtual char conf_char() const { return 'B'; }
	virtual bool can_move(Dir) const;
	virtual void do_move(Dir);
};

class Board::Piece_P : public Board::Piece
{
public:
	Piece_P(PPieceTable& pt, int col, int row);
	virtual char conf_char() const { return 'P'; }
	virtual bool can_move(Dir) const;
	virtual void do_move(Dir);
};

inline Board::Dir
operator-(Board::Dir dir)
{
	return static_cast<Board::Dir>(Board::D_MAX - dir);
}

char dir2char(Board::Dir);
Board::Dir char2dir(int);

////////////////////////////////////////////////////////////////////////
//
// Interface
//
////////////////////////////////////////////////////////////////////////
class Interface
{
public:
	Interface(const Board*);
	virtual ~Interface() =0;
	virtual int error() const;

	virtual void set_board(const Board*);
	virtual void set_steps(unsigned);
	virtual void set_prompt(const string&);
	virtual void echo(char);
	virtual void clear_echo(char c=0);
	virtual int get_event();
	virtual void unget_event(int);

public:
	virtual int next_event() =0;
	virtual void draw() =0;
	virtual void redraw() =0;

protected:
	int m_error;
	const Board* m_board;
	unsigned int m_steps;
	string m_prompt;
	string m_echo;
	int m_event;
};

class NcursesInterface : public Interface
{
public:
	NcursesInterface(Board* board=0);
	virtual ~NcursesInterface();

	virtual int next_event();
	virtual void draw();
	virtual void redraw();

private:
	enum {
		CL_DEFAULT=0,
		CL_PROMPT,
		CL_ECHO,
		CL_STEP,
		CL_BOARD,
		CL_SELECTED,
	};

private:
	void init();
	void display_board(const Board* pb, int r0, int c0);
	void display_box(int row, int col);
	void display_qqqq(int row, int col, char ch);
	void display_lr(int row, int col, char ch);
	void display_tb(int row, int col, char ch);
	void display_p(int row, int col, char ch);
	
private:
	int m_maxy;
	int m_maxx;
	int m_bdy;
	int m_bdx;

	int m_prompt_size;
	int m_echo_size;
	int m_steps_size;
};

////////////////////////////////////////////////////////////////////////
//
// Game
//
////////////////////////////////////////////////////////////////////////
class Game
{
public:
	Game(Interface* iface, Board* board);
	~Game();
	int loop();

private:
	void on_select(int);
	void on_enter(int);
	void on_undo(int);
	void on_redo(int);
	void on_bsave(int);
	void on_bload(int);

	void do_move(int, Board::Dir);
	void new_move(int, Board::Dir);
	bool old_move(Board::Dir);
	bool undo();
	bool redo();
	
private:
	Interface* m_iface;
	Board* m_board;

	vector<Board::Move> m_history;
	unsigned m_steps;
};

inline int
HRD_CTRL(int ch)
{
	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 1;
	else
		return 0;
}

#define HRD_CTRL_C (HRD_CTRL('C'))
#define HRD_CTRL_L (HRD_CTRL('L'))

////////////////////////////////////////////////////////////////////////
//
// Lock
//
////////////////////////////////////////////////////////////////////////
//! abstract base Lock class
class Lock
{
public:
	enum Mode {
		RD_LOCK=1,
		WR_LOCK=2,
	};
public:
	Lock(int, Mode);
	virtual ~Lock() =0;
	virtual bool islocked() const;
	virtual int error() const;
	virtual int lock() =0;
	virtual int unlock() =0;

protected:
	int m_fd;
	Mode m_mode;
	bool m_locked;
	int m_error;
};

//! concrete Lock class use fcntl()
class RecordLock : public Lock
{
public:
	RecordLock(int, Mode);
	virtual ~RecordLock();
	virtual int lock();
	virtual int unlock();
};

typedef RecordLock HRD_Lock;

////////////////////////////////////////////////////////////////////////
//
// Bookmark
//
////////////////////////////////////////////////////////////////////////
class Bookmark
{
public:
	static int init(const string& fn="");
	static int save(char key, const string& pat,
			const vector<Board::Move>&, size_t);
	static int load(char key, Board**, vector<Board::Move>*);

private:
	static int do_pack(string&, char, const string&,
			const vector<Board::Move>&, size_t);
	static int do_write(int, char*, size_t, char, const string&);
	static int do_read(const char*, size_t, char, Board**, vector<Board::Move>*);

private:
	static string s_fn;
};

#endif

