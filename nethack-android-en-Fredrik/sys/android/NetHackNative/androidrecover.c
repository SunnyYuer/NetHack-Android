/* androidrecover.c */

#include "androidrecover.h"
#include "hack.h"

#include <stdarg.h>
#include <stdio.h>

/* TEMP */
#if 1
#define SAVESIZE	(PL_NSIZ + 13)	/* save/99999player.e */
char savename[SAVESIZE]; /* holds relative path of save file from playground */
int FDECL(TMP_restore_savefile, (char *));
int TMP_main(int argc, char *argv[]);
#endif

/*------------------------------------------------------------------*/
/* TEMP */

/*	SCCS Id: @(#)recover.c	3.4	1999/10/23	*/
/*	Copyright (c) Janet Walz, 1992.				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 *  Utility for reconstructing NetHack save file from a set of individual
 *  level files.  Requires that the `checkpoint' option be enabled at the
 *  time NetHack creates those level files.
 */
#include "config.h"
#if !defined(O_WRONLY) && !defined(LSC) && !defined(AZTEC_C)
#include <fcntl.h>
#endif
#ifdef WIN32
#include <errno.h>
#include "win32api.h"
#endif

#ifdef VMS
extern int FDECL(vms_creat, (const char *,unsigned));
extern int FDECL(vms_open, (const char *,int,unsigned));
#endif	/* VMS */

#if 1
int FDECL(TMP_restore_savefile, (char *));
void FDECL(TMP_set_levelfile_name, (int));
int FDECL(TMP_open_levelfile, (int));
int NDECL(TMP_create_savefile);
void FDECL(TMP_copy_bytes, (int,int));
#endif

#ifndef WIN_CE
#define Fprintf	(void)fprintf1
#else
#define Fprintf	(void)nhce_message
static void nhce_message(FILE*, const char*, ...);
#endif

/* TEMP */
int fprintf1(FILE *stream, const char *fmt, ...)
{
	char buff[1024];
	int r;

	va_list args;
	va_start(args, fmt);
	r = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

	android_puts(buff);

	return r;
}

#define Close	(void)close


#if defined(EXEPATH)
char *FDECL(exepath, (char *));
#endif

#if defined(__BORLANDC__) && !defined(_WIN32)
extern unsigned _stklen = STKSIZ;
#endif
#if 0
char savename[SAVESIZE]; /* holds relative path of save file from playground */
#endif

int
TMP_main(argc, argv)
int argc;
char *argv[];
{
	int argno;

	const char *dir = (char *)0;
#ifdef AMIGA
	char *startdir = (char *)0;
#endif


	if (!dir) dir = getenv("NETHACKDIR");
	if (!dir) dir = getenv("HACKDIR");
#if defined(EXEPATH)
	if (!dir) dir = exepath(argv[0]);
#endif
	if (argc == 1 || (argc == 2 && !strcmp(argv[1], "-"))) {
	    Fprintf(stderr,
		"Usage: %s [ -d directory ] base1 [ base2 ... ]\n", argv[0]);
#if defined(WIN32) || defined(MSDOS)
	    if (dir) {
	    	Fprintf(stderr, "\t(Unless you override it with -d, recover will look \n");
	    	Fprintf(stderr, "\t in the %s directory on your system)\n", dir);
	    }
#endif
	    exit(EXIT_FAILURE);
	}

	argno = 1;
	if (!strncmp(argv[argno], "-d", 2)) {
		dir = argv[argno]+2;
		if (*dir == '=' || *dir == ':') dir++;
		if (!*dir && argc > argno) {
			argno++;
			dir = argv[argno];
		}
		if (!*dir) {
		    Fprintf(stderr,
			"%s: flag -d must be followed by a directory name.\n",
			argv[0]);
		    exit(EXIT_FAILURE);
		}
		argno++;
	}
#if defined(SECURE) && !defined(VMS)
	if (dir
# ifdef HACKDIR
		&& strcmp(dir, HACKDIR)
# endif
		) {
		(void) setgid(getgid());
		(void) setuid(getuid());
	}
#endif	/* SECURE && !VMS */

#ifdef HACKDIR
	if (!dir) dir = HACKDIR;
#endif

#ifdef AMIGA
	startdir = getcwd(0,255);
#endif
	if (dir && chdir((char *) dir) < 0) {
		Fprintf(stderr, "%s: cannot chdir to %s.\n", argv[0], dir);
		exit(EXIT_FAILURE);
	}

	while (argc > argno) {
		if (TMP_restore_savefile(argv[argno]) == 0)
		{
#if 0
		    Fprintf(stderr, "recovered \"%s\" to %s\n",
			    argv[argno], savename);
#endif
		}
		argno++;
	}
#ifdef AMIGA
	if (startdir) (void)chdir(startdir);
#endif
	exit(EXIT_SUCCESS);
	/*NOTREACHED*/
	return 0;
}

static char TMP_lock[256];

void
TMP_set_levelfile_name(lev)
int lev;
{
	char *tf;

	tf = rindex(TMP_lock, '.');
	if (!tf) tf = TMP_lock + strlen(TMP_lock);
	(void) sprintf(tf, ".%d", lev);
#ifdef VMS
	(void) strcat(tf, ";1");
#endif
}

int
TMP_open_levelfile(lev)
int lev;
{
	int fd;

	TMP_set_levelfile_name(lev);
#if defined(MICRO) || defined(WIN32) || defined(MSDOS)
	fd = open(TMP_lock, O_RDONLY | O_BINARY);
#else
	fd = open(TMP_lock, O_RDONLY, 0);
#endif
	return fd;
}

int
TMP_create_savefile()
{
	int fd;

#if defined(MICRO) || defined(WIN32) || defined(MSDOS)
	fd = open(savename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, FCMASK);
#else
	fd = creat(savename, FCMASK);
#endif
	return fd;
}

void
copy_bytes(ifd, ofd)
int ifd, ofd;
{
	char buf[BUFSIZ];
	int nfrom, nto;

	do {
		nfrom = read(ifd, buf, BUFSIZ);
		nto = write(ofd, buf, nfrom);
		if (nto != nfrom) {
			Fprintf(stderr, "file copy failed!\n");
			exit(EXIT_FAILURE);
		}
	} while (nfrom == BUFSIZ);
}

int
TMP_restore_savefile(basename)
char *basename;
{
	int gfd, lfd, sfd;
	int lev, savelev, hpid;
	xchar levc;
	struct version_info version_data;

	/* level 0 file contains:
	 *	pid of creating process (ignored here)
	 *	level number for current level of save file
	 *	name of save file nethack would have created
	 *	and game state
	 */
	(void) strcpy(TMP_lock, basename);
	gfd = TMP_open_levelfile(0);
	if (gfd < 0) {
#if defined(WIN32) && !defined(WIN_CE)
 	    if(errno == EACCES) {
	  	Fprintf(stderr,
			"\nThere are files from a game in progress under your name.");
		Fprintf(stderr,"\nThe files are locked or inaccessible.");
		Fprintf(stderr,"\nPerhaps the other game is still running?\n");
	    } else
	  	Fprintf(stderr,
			"\nTrouble accessing level 0 (errno = %d).\n", errno);
#endif
	    Fprintf(stderr, "Cannot open level 0 for %s.\n", basename);
	    return(-1);
	}
	if (read(gfd, (genericptr_t) &hpid, sizeof hpid) != sizeof hpid) {
	    Fprintf(stderr, "%s\n%s%s%s\n",
	     "Checkpoint data incompletely written or subsequently clobbered;",
		    "recovery for \"", basename, "\" impossible.");
	    Close(gfd);
	    return(-1);
	}
	if (read(gfd, (genericptr_t) &savelev, sizeof(savelev))
							!= sizeof(savelev)) {
	    Fprintf(stderr,
	    "Checkpointing was not in effect for %s -- recovery impossible.\n",
		    basename);
	    Close(gfd);
	    return(-1);
	}
	if ((read(gfd, (genericptr_t) savename, sizeof savename)
		!= sizeof savename) ||
	    (read(gfd, (genericptr_t) &version_data, sizeof version_data)
		!= sizeof version_data)) {
	    Fprintf(stderr, "Error reading %s -- can't recover.\n", TMP_lock);
	    Close(gfd);
	    return(-1);
	}

	/* save file should contain:
	 *	version info
	 *	current level (including pets)
	 *	(non-level-based) game state
	 *	other levels
	 */
	sfd = TMP_create_savefile();
	if (sfd < 0) {
	    Fprintf(stderr, "Cannot create savefile %s.\n", savename);
	    Close(gfd);
	    return(-1);
	}

	lfd = TMP_open_levelfile(savelev);
	if (lfd < 0) {
	    Fprintf(stderr, "Cannot open level of save for %s.\n", basename);
	    Close(gfd);
	    Close(sfd);
	    return(-1);
	}

	if (write(sfd, (genericptr_t) &version_data, sizeof version_data)
		!= sizeof version_data) {
	    Fprintf(stderr, "Error writing %s; recovery failed.\n", savename);
	    Close(gfd);
	    Close(sfd);
	    return(-1);
	}

	copy_bytes(lfd, sfd);
	Close(lfd);
	(void) unlink(TMP_lock);

	copy_bytes(gfd, sfd);
	Close(gfd);
	TMP_set_levelfile_name(0);
	(void) unlink(TMP_lock);

	for (lev = 1; lev < 256; lev++) {
		/* level numbers are kept in xchars in save.c, so the
		 * maximum level number (for the endlevel) must be < 256
		 */
		if (lev != savelev) {
			lfd = TMP_open_levelfile(lev);
			if (lfd >= 0) {
				/* any or all of these may not exist */
				levc = (xchar) lev;
				write(sfd, (genericptr_t) &levc, sizeof(levc));
				copy_bytes(lfd, sfd);
				Close(lfd);
				(void) unlink(TMP_lock);
			}
		}
	}

	Close(sfd);

	return(0);
}

#ifdef EXEPATH
# ifdef __DJGPP__
#define PATH_SEPARATOR '/'
# else
#define PATH_SEPARATOR '\\'
# endif

#define EXEPATHBUFSZ 256
char exepathbuf[EXEPATHBUFSZ];

char *exepath(str)
char *str;
{
	char *tmp, *tmp2;
	int bsize;

	if (!str) return (char *)0;
	bsize = EXEPATHBUFSZ;
	tmp = exepathbuf;
#if !defined(WIN32)
	strcpy (tmp, str);
#else
# if defined(WIN_CE)
	{
	  TCHAR wbuf[EXEPATHBUFSZ];
	  GetModuleFileName((HANDLE)0, wbuf, EXEPATHBUFSZ);
	  NH_W2A(wbuf, tmp, bsize);
	}
# else
	*(tmp + GetModuleFileName((HANDLE)0, tmp, bsize)) = '\0';
# endif
#endif
	tmp2 = strrchr(tmp, PATH_SEPARATOR);
	if (tmp2) *tmp2 = '\0';
	return tmp;
}
#endif /* EXEPATH */

#ifdef AMIGA
#include "date.h"
const char amiga_version_string[] = AMIGA_VERSION_STRING;
#endif

#ifdef WIN_CE
void nhce_message(FILE* f, const char* str, ...)
{
    va_list ap;
	TCHAR wbuf[NHSTR_BUFSIZE];
	char buf[NHSTR_BUFSIZE];

    va_start(ap, str);
	vsprintf(buf, str, ap);
    va_end(ap);

	MessageBox(NULL, NH_A2W(buf, wbuf, NHSTR_BUFSIZE), TEXT("Recover"), MB_OK);
}
#endif

/*recover.c*/


/* End of file androidrecover.c */
