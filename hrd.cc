/* $Id: hrd.cc,v 1.10 2004/04/15 20:10:23 qhuo Exp $ */

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

int
main(int ac, char** av)
{
	if (int error = Bookmark::init()) {
		errno = error;
		perror("Bookmark::init()");
		exit(1);
	}

	string s = "TQQT.BQQB.TLRT.BPPB.P__P.";
	if (ac > 1)
		s = av[1];
	Board* pb = new Board(s);
	if (int e = pb->error()) {
		fprintf(stderr, "Invalid board: %s: error=%d\n", s.c_str(), e);
		delete pb;
		return e;
	}

	Interface* pi = new NcursesInterface(pb);
	if (int e = pi->error()) {
		fprintf(stderr, "Interface error: %d\n", e);
		delete pi;
		delete pb;
		return e;
	}

	Game game(pi, pb);
	return game.loop();
}

