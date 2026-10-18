/* $Id: bookmark.cc,v 1.5 2004/04/16 22:50:20 qhuo Exp $ */

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
// Lock
//
////////////////////////////////////////////////////////////////////////
Lock::Lock(int fd, Mode mode)
	: m_fd(fd), m_mode(mode), m_locked(false), m_error(0)
{
}

Lock::~Lock()
{
}

bool
Lock::islocked() const
{
	return m_locked;
}

int
Lock::error() const
{
	return m_error;
}

////////////////////////////////////////////////////////////////////////
//
// RecordLock
//
////////////////////////////////////////////////////////////////////////
RecordLock::RecordLock(int fd, Mode mode)
	: Lock(fd, mode)
{
	lock();
}

RecordLock::~RecordLock()
{
	if (m_locked)
		unlock();
}

int
RecordLock::lock()
{
	m_error = 0;

	struct flock fl;
	fl.l_type = ((m_mode == RD_LOCK) ? F_RDLCK : F_WRLCK);
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	if (fcntl(m_fd, F_SETLK, &fl) != -1)
		m_locked = true;
	else
		m_error = errno;

	return m_error;
}

int
RecordLock::unlock()
{
	m_error = 0;
	
	struct flock fl;
	fl.l_type = F_UNLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	if (fcntl(m_fd, F_SETLK, &fl) != -1)
		m_locked = false;
	else
		m_error = errno;

	return m_error;
}

string Bookmark::s_fn;

/*!
 * return 0 for success, errno on error
 */
int
Bookmark::init(const string& fn0)
{
	if (! fn0.empty()) {
		if (access(fn0.c_str(), R_OK|W_OK) == 0) {
			s_fn = fn0;
			return 0;
		} else {
			return errno;
		}
	}

	/* $XDG_DATA_HOME/hrd, or ~/.local/share/hrd; an empty or relative
	 * XDG_DATA_HOME counts as unset (XDG spec) */
	const char* xdg = getenv("XDG_DATA_HOME");
	const char* home = getenv("HOME");
	string dir;
	if (xdg && xdg[0] == '/')
		dir = string(xdg) + "/hrd";
	else if (home && home[0])
		dir = string(home) + "/.local/share/hrd";
	else
		return 0;	/* nowhere to keep bookmarks: play without them */

	/* the ~/.hrd of older versions stays in use while the new folder
	 * does not exist */
	if (home && home[0] && access(dir.c_str(), F_OK) < 0) {
		const string old = string(home) + "/.hrd";
		if (access((old + "/bookmark").c_str(), F_OK) == 0)
			dir = old;
	}

	const string fn = dir + "/bookmark";

	/* the folder and any missing parents */
	for (size_t i = 1; i <= dir.size(); ++i) {
		if (i == dir.size() || dir[i] == '/') {
			const string part = dir.substr(0, i);
			if (access(part.c_str(), F_OK) < 0
					&& mkdir(part.c_str(), 0777) < 0)
				return errno;
		}
	}
	if (access(fn.c_str(), R_OK|W_OK) < 0) {
		int fd;
		if ((fd = open(fn.c_str(), O_RDWR|O_CREAT, 0666)) < 0 
				|| close(fd) < 0)
			return errno;
	}

	s_fn = fn;
	return 0;
}

int
Bookmark::save(char key, const string& pat,
		const vector<Board::Move>& mv, size_t n)
{
	if (s_fn.empty())
		return EINVAL;

	int error=0;

	string str;
	if ((error = do_pack(str, key, pat, mv, n)))
		return error;

	int fd = open(s_fn.c_str(), O_RDWR);
	if (fd < 0)
		return errno;

	HRD_Lock lock(fd, Lock::WR_LOCK);
	if (!lock.islocked()) {
		(void) close(fd);
		return lock.error();
	}

	char *p0=0;
	size_t map_length=0;
	
	struct stat st;
	if (fstat(fd, &st) < 0) {
		error = errno;
		goto out;
	}

	map_length = st.st_size + str.length();

	if (ftruncate(fd, map_length) < 0) {
		error = errno;
		goto out;
	}

	if ((p0 = (char*)mmap(0, map_length, PROT_READ|PROT_WRITE, 
				MAP_SHARED, fd, 0)) == MAP_FAILED) {
		error = errno;
		p0 = 0;
		goto out;
	}
	
	error = do_write(fd, p0, st.st_size, key, str);

out:
	if (p0 && munmap(p0, map_length) < 0 && !error)
		error = errno;

	if (lock.unlock() && !error)
		error = lock.error();

	if (close(fd) < 0 && !error)
		error = errno;
	
	return error;
}

int
Bookmark::do_pack(string& str, char key, const string& pat,
		const vector<Board::Move>& mv, size_t n)
{
	if (n > mv.size())
		return EINVAL;

	ostringstream os;
	os << key << ":" << pat << ":";
	for (size_t i=0; i<n && i<mv.size(); ++i) {
		os << mv[i].m_index << dir2char(mv[i].m_dir);
		if (mv[i].m_dir2)
			os << dir2char(mv[i].m_dir2);
		os << ',';
	}
	os << "\n";

	str = os.str();
	return 0;
}

int
Bookmark::do_write(int fd, char* p0, size_t fsize, char key, const string& s)
{
	char *p = p0, *pe = p0 + fsize;
	for (; p<pe; ++p) {
		if (*p == key || !(p = (char*)memchr(p, '\n', pe-p)))
			break;
	}
	
	int old_key_size=0;
	
	if (p && p < pe) {
		char* p2 = (char*)memchr(p, '\n', pe-p);
		if (!p2)
			p2 = pe;
		else
			++p2;

		old_key_size = p2 - p;

		if (p2 < pe) {
			memmove(p, p2, pe-p2);
			p += (pe - p2);
		}
	} else {
		p = pe;
	}

	strcpy(p, s.c_str());

	if (old_key_size && ftruncate(fd, fsize + s.length() - old_key_size) < 0)
		return errno;
	
	return 0;
}

int
Bookmark::load(char key, Board** ppb, vector<Board::Move>* pmv)
{
	if (s_fn.empty() || !ppb || !pmv)
		return EINVAL;

	int fd = open(s_fn.c_str(), O_RDWR);
	if (fd < 0)
		return errno;

	HRD_Lock lock(fd, Lock::RD_LOCK);
	if (!lock.islocked()) {
		(void) close(fd);
		return lock.error();
	}

	char *p=0;
	int error=0;

	struct stat st;
	if (fstat(fd, &st) < 0) {
		error = errno;
		goto out;
	}

	if (st.st_size == 0) {
		error = ENOENT;		/* no bookmarks saved yet */
		goto out;
	}

	if ((p = (char*)mmap(0, st.st_size, PROT_READ, 
				MAP_SHARED, fd, 0)) == MAP_FAILED) {
		error = errno;
		p = 0;
		goto out;
	}

	error = do_read(p, st.st_size, key, ppb, pmv);

out:
	if (p && munmap(p, st.st_size) < 0 && !error)
		error = errno;

	if (lock.unlock() && !error)
		error = lock.error();

	if (close(fd) < 0 && !error)
		error = errno;
	
	if (error && *ppb) {
		delete *ppb;
		*ppb = 0;
	}
	
	return error;
}

int
Bookmark::do_read(const char* p0, size_t fsize, char key,
		Board** ppb, vector<Board::Move>* pmv)
{
	const char *p = p0, *pe = p0 + fsize;

	for (; p<pe; ++p) {
		if (*p == key || !(p = (const char*)memchr(p, '\n', pe-p)))
			break;
	}

	if (!p || p>=pe)
		return EINVAL;

	if (!(pe = (const char*)memchr(p, '\n', pe-p)))
		return EINVAL;

	const string s(p, pe);

	if (s.length() < 28 || s[1] != ':' || s[27] != ':')
		return EINVAL;

	const string pattern = s.substr(2, 25);
	Board* pb = new Board(pattern);
	if (pb->error()) {
		delete pb;
		return EINVAL;
	}

	size_t ib=28, ie;
	for (; ((ie=s.find(',',ib))!=string::npos); ib=ie+1) {
		int index;
		Board::Dir d1, d2=Board::D_NONE;

		switch (ie - ib) {
			case 3:
				if (!(d2 = char2dir(s[ib+2])))
					goto fail;
				// FALL THROUGH
			case 2:
				break;
			default:
				goto fail;
		}

		if (!(d1 = char2dir(s[ib+1])))
			goto fail;
		if (!isdigit(s[ib]))
			goto fail;

		index = s[ib]-'0';

		Board::Move m1(index, d1), m(index, d1, d2);

		if (!pb->can_move(index, d1))
			goto fail;

		pb->do_move(m1);

		if (d2) {
			if (!pb->can_move(index, d2))
				goto fail;
			pb->undo_move(m1);
			pb->do_move(m);
		}

		pmv->push_back(m);
	}

	*ppb = pb;
	return 0;

fail:
	delete pb;
	return EINVAL;
}

