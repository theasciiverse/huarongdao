#
# $Id: Makefile.am,v 1.8 2004/04/25 11:08:06 qhuo Exp $
#

AM_CXXFLAGS = -Wall -W

bin_PROGRAMS = hrd
hrd_SOURCES = hrd.h hrd.cc board.cc iface.cc ncurses.cc game.cc bookmark.cc
hrd_LDFLAGS = $(CURSES_LIB)

man_MANS = hrd.6
EXTRA_DIST = $(man_MANS)
