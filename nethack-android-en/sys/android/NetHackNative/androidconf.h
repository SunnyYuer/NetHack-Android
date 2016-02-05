#ifndef ANDROIDCONF_H
#define ANDROIDCONF_H

#define TEXTCOLOR		/* Use colored monsters and objects */

#define ASCIIGRAPH		/* Enable IBMgraphics option */

#define SELF_RECOVER	/* NetHack itself can recover games */

// TODO
#if 0
#define HACKFONT		/* Use special hack.font */
#define SHELL			/* Have a shell escape command (!) */
#define MAIL			/* Get mail at unexpected occasions */
#define DEFAULT_ICON "NetHack:default.icon"	/* private icon */
#define AMIFLUSH		/* toss typeahead (select flush in .cnf) */
/* #define OPT_DISPMAP		/* enable fast_map option */

#endif

#if 0

#ifndef MICRO_H
#include "micro.h"
#endif

/* From pcconf.h: */
#define PATHLEN		64	/* maximum pathlength */
#define FILENAME	80	/* maximum filename length (conservative) */

#ifndef SYSTEM_H
#include "system.h"		/* For SIG_RET_TYPE, etc */
#endif

#endif

/*#undef	UNIX*/		/* Not sure... seems to not work with MICRO otherwise */
#undef	TERMLIB

#ifdef TTY_GRAPHICS
# define ANSI_DEFAULT
#endif

#undef COMPRESS
#undef COMPRESS_EXTENSION
#define COMPRESS "/system/bin/gzip"		/* gzip compression */
#define COMPRESS_EXTENSION ".gz" 		/* normal gzip extension */

extern void android_putchar(int c);
extern void android_puts(const char *s);
extern int android_printf(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

extern int android_getch(void);

/* Is there a better way to remap these? I sure hope so. */
#undef putchar
#define putchar android_putchar
#undef puts
#define puts android_puts
#undef getchar
#define getchar android_getch
#undef printf
#define printf android_printf

/* This is for __attribte__ ((format...)) under GCC, I think, which
   otherwise gets confused and produces a bunch of
   "'android_printf' is an unrecognized format function type"
   warnings.
*/
#undef PRINTF_F
#define PRINTF_F(a, b)

typedef enum
{
	kAndroidGameStateInvalid,

	/* Note: if making changes here, keep s_statenames[] in 'androidmain.c'
	   up to date. */

	kAndroidGameStateExtCmd,
	kAndroidGameStateGetPos,
	kAndroidGameStateInit,
	kAndroidGameStateMenu,
	kAndroidGameStateMoveLoop,
	kAndroidGameStateText,
	kAndroidGameStateWaitingForResponse,

	kAndroidNumGameStates
} AndroidGameState;

extern AndroidGameState android_getgamestate(void);
extern void android_switchgamestate(AndroidGameState s);
extern void android_pushgamestate(AndroidGameState s);
extern void android_popgamestate();

#endif /* ANDROIDCONF_H */

/* End of file androidconf.h */
