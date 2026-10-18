/* Stands in for the config.h that configure writes (see config.h.in),
   so asciiverse/build.sh needs no autotools. HAVE_CURSES_H can be
   defined on the command line instead (PDCurses). */
#if !defined(HAVE_CURSES_H) && !defined(HAVE_NCURSES_H)
#define HAVE_NCURSES_H 1
#endif
#define PACKAGE "hrd"
#define PACKAGE_NAME "hrd"
#define PACKAGE_TARNAME "hrd"
#define PACKAGE_VERSION "0.1"
#define PACKAGE_STRING "hrd 0.1"
#define PACKAGE_BUGREPORT ""
#define VERSION "0.1"
