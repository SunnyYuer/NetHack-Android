/* winandroid.c */

#include "hack.h"
#include "dlb.h"		/* dlb_fopen() */
#include "tcap.h"		/* nh_CD */
#include "wintty.h"

#include "func_tab.h"	/* extcmdlist */
#include <jni.h>

void android_player_selection();
winid android_create_nhwindow(int type);
void android_clear_nhwindow(winid window);
void android_display_nhwindow(winid window, BOOLEAN_P blocking);
void android_destroy_nhwindow(winid window);
/*
E void FDECL(android_display_nhwindow, (winid, BOOLEAN_P));
*/
void android_curs(winid window, int x, int y);
void android_putstr(winid window, int attr, const char *str);
/*void android_display_file(const char *fname, boolean complain);*/
void FDECL(android_display_file, (const char *, BOOLEAN_P complain));
int android_select_menu(winid window, int how, menu_item **menu_list);
int android_nh_poskey(int *x, int *y, int *mod);
int android_doprev_message();
char android_yn_function(const char *query, const char *resp, CHAR_P def);
/*E char FDECL(android_yn_function, (const char *, const char *, CHAR_P));*/
void android_getlin(const char *query, char *bufp);
int android_get_ext_cmd();

/* These are defined in 'wintty.c', wrappers to sneakily access static
	functions in that file. Long term, it would probably be best to
	make the Android TTY code independent of the regular TTY graphics
	code, if it can be done without too much code duplication. */
void android_free_window_info(struct WinDesc *wd, BOOLEAN_P b);
void android_process_menu_window(winid wid, struct WinDesc *wd);
void android_process_text_window(winid wid, struct WinDesc *wd);

#ifdef ANDROID_GRAPHICS_TILED
void FDECL(android_tiled_print_glyph, (winid,XCHAR_P,XCHAR_P,int));
/*void android_tiled_print_glyph(winid window, xchar x, xchar y, int glyph);*/
#endif


/* TODO: Put in header file (for androidmain.c, right now) */
void android_process_input(int *chout, int *clickxout, int *clickyout);


struct window_procs android_procs = {
    "android",
#ifdef MSDOS
    WC_TILED_MAP|WC_ASCII_MAP|
#endif
#if defined(WIN32CON)
    WC_MOUSE_SUPPORT|
#endif
    WC_COLOR|WC_HILITE_PET|WC_INVERSE|WC_EIGHT_BIT_IN,
    0L,
    tty_init_nhwindows,
/*	android_player_selection,*/
	tty_player_selection,
    tty_askname,
    tty_get_nh_event,
    tty_exit_nhwindows,
    tty_suspend_nhwindows,
    tty_resume_nhwindows,
	android_create_nhwindow,
	android_clear_nhwindow,
    android_display_nhwindow,
	android_destroy_nhwindow,
	android_curs,
	android_putstr,
/*	tty_display_file,*/
	android_display_file,
    tty_start_menu,
    tty_add_menu,
    tty_end_menu,
/*	tty_select_menu,*/
	android_select_menu,
    tty_message_menu,
    tty_update_inventory,
    tty_mark_synch,
    tty_wait_synch,
#ifdef CLIPPING
    tty_cliparound,
#endif
#ifdef POSITIONBAR
    tty_update_positionbar,
#endif
#ifdef ANDROID_GRAPHICS_TILED
    android_tiled_print_glyph,
#else
    tty_print_glyph,
#endif
    tty_raw_print,
    tty_raw_print_bold,
    tty_nhgetch,
    android_nh_poskey,
    tty_nhbell,
    android_doprev_message,
    android_yn_function,
    android_getlin,
    android_get_ext_cmd,
    tty_number_pad,
    tty_delay_output,
#ifdef CHANGE_COLOR	/* the Mac uses a palette device */
    tty_change_color,
#ifdef MAC
    tty_change_background,
    set_tty_font_name,
#endif
    tty_get_color_string,
#endif

    /* other defs that really should go away (they're tty specific) */
    tty_start_screen,
    tty_end_screen,
    genl_outrip,
#if defined(WIN32CON)
    nttty_preference_update,
#else
    genl_preference_update,
#endif
};

#ifdef ANDROID_GRAPHICS_TILED

extern int g_AndroidTiled;
extern int g_AndroidTilesEnabledForUser;

#endif	/* ANDROID_GRAPHICS_TILED */

static char winpanicstr[] = "Bad window id %d";

void android_wininit_data(int *argcp, char **argv)	/* should we have these params? */
{
	win_tty_init();
}

#ifdef ANDROID_GRAPHICS_TILED

void android_putchar_internal2(int c);

static void android_put_utf8(int unicode)
{
#if 0
	char buff[4];

	if(unicode >= 0x800)
	{
		unsigned char c1 = 0xe0 + ((unicode & 0xf000) >> 12);
		unsigned char c2 = 0x80 + ((unicode & 0x0f00) >> 6) + ((unicode & 0x00c0) >> 6);
		unsigned char c3 = 0x80 + (unicode & 0x003f);

		buff[0] = c1;
		buff[1] = c2;
		buff[2] = c3;
		buff[3] = '\0';
		android_puts(buff);
	}
	else if(unicode >= 0x80)
	{
		unsigned char c1 = 0xc0 + ((unicode & 0x0700) >> 6) + ((unicode & 0x00c0) >> 6);
		unsigned char c2 = 0x80 + (unicode & 0x003f);

		buff[0] = c1;
		buff[1] = c2;
		buff[2] = '\0';
		android_puts(buff);
	}
	else
	{
		buff[0] = (char)unicode;
		buff[1] = '\0';
		android_puts(buff);
	}
#endif
	android_putchar_internal2(unicode);
}


/* from tile.c */
extern short glyph2tile[];
extern int total_tiles_used;


/*
void FDECL(tty_print_glyph, (winid,XCHAR_P,XCHAR_P,int));
void android_tiled_print_glyph(winid window, xchar x, xchar y, int glyph)
*/
void android_tiled_print_glyph(window, x, y, glyph)
    winid window;
    xchar x, y;
    int glyph;
{
	/* This is a bit arbitrary, but basically whenever we are about to
	   print a glyph, we check to see if we are in the desired tiling mode. */

	/* We want tiles on if the user has requested so, and we are not on
	   the rogue level. */	
	int shouldtilesbeenabled = g_AndroidTiled
#ifdef REINCARNATION
			 && !Is_rogue_level(&u.uz)
#endif
			;

	/* Check to see if our desired state is any different than what the user
	   should already have been set to. */
	if(shouldtilesbeenabled != g_AndroidTilesEnabledForUser)
	{
		if(shouldtilesbeenabled)
		{
			/* Escape sequence to enter tiled view. */
			android_puts("\033Ar");
		}
		else
		{
			/* Escape sequence to leave tiled view, for a character view. */
			android_puts("\033AR");
		}

		/* Remember what we told the user. */
		g_AndroidTilesEnabledForUser = shouldtilesbeenabled;
	}

	/* Now, if the user is not currently set to be in tiled view mode,
	   we just call into the TTY code to print the correct colored
	   character. */
	if(!g_AndroidTilesEnabledForUser)
	{
		tty_print_glyph(window, x, y, glyph);
		return;
	}

	int tile = glyph2tile[glyph];

	android_puts("\033A5");

	--x;	/* column 0 is never used */
/*
    x += cw->offx;
    y += cw->offy;
*/
	cmov(x, y);

    /* map glyph to character and color */
/*
    mapglyph(glyph, &ch, &color, &special, x, y);
	android_putchar(ch);
*/

// TODO: UTF8 here?
//	android_putchar(tile);

	android_put_utf8(tile + 0x100);
//	android_put_utf8(tile);

	android_puts("\033A0");
}

#endif	/* ANDROID_GRAPHICS_TILED */

#if 0

void android_askname()
{
	plname[0] = 'a';
	plname[1] = 'n';
	plname[2] = 'd';
	plname[3] = 'r';
	plname[4] = 'o';
	plname[5] = 'i';
	plname[6] = 'd';
	plname[7] = '\0';
}

#endif

static int s_MessageNumColumns = 80;
static int s_StatusNumColumns = 80;
static int s_NumMsgLines = 1;

void Java_com_nethackff_NetHackJNI_NetHackSetScreenDim(
		JNIEnv *env, jobject thiz, int msgwidth, int nummsglines,
		int statuswidth)
{
	s_MessageNumColumns = msgwidth;
	s_StatusNumColumns = statuswidth;
	s_NumMsgLines = nummsglines;
}


static int s_MsgCol = 0;
static int s_MsgRow = 0;

#if 0

/* HACK */
void tty_putsym(winid window, int x, int y, char ch);

static int s_TextCol = 0;

static void android_text_print_word(const char *wordstart, const char *wordend)
{
	char buff[256];

	const int wordlen = wordend - wordstart;
	if(s_TextCol > 0)
	{
		int maxcol = s_ScreenNumColumns;

		if(s_TextCol + wordlen + 1 > maxcol)
		{
			/* The word doesn't fit, advance to the next line, unless
			   we just wrapped around anyway. */
			if(s_TextCol != s_ScreenNumColumns)
			{
				android_puts("\n");
			}
			s_TextCol = 0;
		}
		else
		{
			android_puts(" ");
			s_TextCol++;
		}
	}

	strncpy(buff, wordstart, sizeof(buff) - 1);
	if(wordlen < sizeof(buff))
	{
		buff[wordlen] = '\0';
	}
	else
	{
		buff[sizeof(buff) - 1] = '\0';
	}
	android_puts(buff);
	s_TextCol += strlen(buff);
	while(s_TextCol >= s_ScreenNumColumns)
	{
		s_TextCol -= s_ScreenNumColumns;
	}
}


void android_output_text_wrapped(winid win, const char *str)
{
	const char *ptr;
	int i;
	const char *wordstart = NULL;
	int foundend = 0;
	int continued = 0;

	while(!foundend)
	{
		ptr = str;
		wordstart = NULL;
		while(1)
		{
			char c = *ptr++;
			if(c == ' ' || !c)
			{
				if(wordstart)
				{
					android_text_print_word(wordstart, ptr - 1);
				}
				wordstart = NULL;

				if(!c)
				{
					foundend = 1;
					break;
				}
			}
			else if(!wordstart)
			{
				wordstart = ptr - 1;
			}
		}
	}
}


/* Adapted from tty_player_selection() - seems like a sad amount of code
   to duplicate, but appears to have been done for other ports (Amiga,
   at least). */

void android_player_selection()
{
	int i, k, n;
	char pick4u = 'n', thisch, lastch = 0;
	char pbuf[QBUFSZ], plbuf[QBUFSZ];
	winid win;
	anything any;
	menu_item *selected = 0;

	/* prevent an unnecessary prompt */
	rigid_role_checks();

	/* Should we randomly pick for the player? */
	if (!flags.randomall &&
		(flags.initrole == ROLE_NONE || flags.initrace == ROLE_NONE ||
		 flags.initgend == ROLE_NONE || flags.initalign == ROLE_NONE)) {
		int echoline;
		char *prompt = build_plselection_prompt(pbuf, QBUFSZ, flags.initrole,
				flags.initrace, flags.initgend, flags.initalign);

		tty_putstr(BASE_WINDOW, 0, "");
		echoline = wins[BASE_WINDOW]->cury;
/*
		tty_putstr(BASE_WINDOW, 0, prompt);
*/

		android_output_text_wrapped(BASE_WINDOW, prompt);

		do {
		pick4u = lowc(readchar());
		if (index(quitchars, pick4u)) pick4u = 'y';
		} while(!index(ynqchars, pick4u));
		if ((int)strlen(prompt) + 1 < CO) {
		/* Echo choice and move back down line */
		tty_putsym(BASE_WINDOW, (int)strlen(prompt)+1, echoline, pick4u);
		tty_putstr(BASE_WINDOW, 0, "");
		} else
		/* Otherwise it's hard to tell where to echo, and things are
		 * wrapping a bit messily anyway, so (try to) make sure the next
		 * question shows up well and doesn't get wrapped at the
		 * bottom of the window.
		 */
		tty_clear_nhwindow(BASE_WINDOW);
		
		if (pick4u != 'y' && pick4u != 'n') {
give_up:	/* Quit */
		if (selected) free((genericptr_t) selected);
/* TEMP */
#if 0
		bail((char *)0);
#endif
		/*NOTREACHED*/
		return;
		}
	}

	(void)	root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);

	/* Select a role, if necessary */
	/* we'll try to be compatible with pre-selected race/gender/alignment,
	 * but may not succeed */
	if (flags.initrole < 0) {
		char rolenamebuf[QBUFSZ];
		/* Process the choice */
		if (pick4u == 'y' || flags.initrole == ROLE_RANDOM || flags.randomall) {
		/* Pick a random role */
		flags.initrole = pick_role(flags.initrace, flags.initgend,
						flags.initalign, PICK_RANDOM);
		if (flags.initrole < 0) {
			tty_putstr(BASE_WINDOW, 0, "Incompatible role!");
			flags.initrole = randrole();
		}
		} else {
			tty_clear_nhwindow(BASE_WINDOW);
		tty_putstr(BASE_WINDOW, 0, "Choosing Character's Role");
		/* Prompt for a role */
		win = create_nhwindow(NHW_MENU);
		start_menu(win);
		any.a_void = 0;			/* zero out all bits */
		for (i = 0; roles[i].name.m; i++) {
			if (ok_role(i, flags.initrace, flags.initgend,
							flags.initalign)) {
			any.a_int = i+1;	/* must be non-zero */
			thisch = lowc(roles[i].name.m[0]);
			if (thisch == lastch) thisch = highc(thisch);
			if (flags.initgend != ROLE_NONE && flags.initgend != ROLE_RANDOM) {
				if (flags.initgend == 1	 && roles[i].name.f)
					Strcpy(rolenamebuf, roles[i].name.f);
				else
					Strcpy(rolenamebuf, roles[i].name.m);
			} else {
				if (roles[i].name.f) {
					Strcpy(rolenamebuf, roles[i].name.m);
					Strcat(rolenamebuf, "/");
					Strcat(rolenamebuf, roles[i].name.f);
				} else 
					Strcpy(rolenamebuf, roles[i].name.m);
			}	
			add_menu(win, NO_GLYPH, &any, thisch,
				0, ATR_NONE, an(rolenamebuf), MENU_UNSELECTED);
			lastch = thisch;
			}
		}

		any.a_int = pick_role(flags.initrace, flags.initgend,
					flags.initalign, PICK_RANDOM)+1;
		if (any.a_int == 0) /* must be non-zero */
			any.a_int = randrole()+1;
		add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
				"Random", MENU_UNSELECTED);
		any.a_int = i+1;	/* must be non-zero */
		add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
				"Quit", MENU_UNSELECTED);
		Sprintf(pbuf, "Pick a role for your %s", plbuf);
		end_menu(win, pbuf);

		n = select_menu(win, PICK_ONE, &selected);
		destroy_nhwindow(win);

		/* Process the choice */
		if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

		flags.initrole = selected[0].item.a_int - 1;
		free((genericptr_t) selected),	selected = 0;
		}
		(void)	root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}
	
	/* Select a race, if necessary */
	/* force compatibility with role, try for compatibility with
	 * pre-selected gender/alignment */
	if (flags.initrace < 0 || !validrace(flags.initrole, flags.initrace)) {
		/* pre-selected race not valid */
		if (pick4u == 'y' || flags.initrace == ROLE_RANDOM || flags.randomall) {
		flags.initrace = pick_race(flags.initrole, flags.initgend,
							flags.initalign, PICK_RANDOM);
		if (flags.initrace < 0) {
			tty_putstr(BASE_WINDOW, 0, "Incompatible race!");
			flags.initrace = randrace(flags.initrole);
		}
		} else {	/* pick4u == 'n' */
		/* Count the number of valid races */
		n = 0;	/* number valid */
		k = 0;	/* valid race */
		for (i = 0; races[i].noun; i++) {
			if (ok_race(flags.initrole, i, flags.initgend,
							flags.initalign)) {
			n++;
			k = i;
			}
		}
		if (n == 0) {
			for (i = 0; races[i].noun; i++) {
			if (validrace(flags.initrole, i)) {
				n++;
				k = i;
			}
			}
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
			tty_clear_nhwindow(BASE_WINDOW);
			tty_putstr(BASE_WINDOW, 0, "Choosing Race");
			win = create_nhwindow(NHW_MENU);
			start_menu(win);
			any.a_void = 0;			/* zero out all bits */
			for (i = 0; races[i].noun; i++)
			if (ok_race(flags.initrole, i, flags.initgend,
							flags.initalign)) {
				any.a_int = i+1;	/* must be non-zero */
				add_menu(win, NO_GLYPH, &any, races[i].noun[0],
				0, ATR_NONE, races[i].noun, MENU_UNSELECTED);
			}
			any.a_int = pick_race(flags.initrole, flags.initgend,
					flags.initalign, PICK_RANDOM)+1;
			if (any.a_int == 0) /* must be non-zero */
			any.a_int = randrace(flags.initrole)+1;
			add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
					"Random", MENU_UNSELECTED);
			any.a_int = i+1;	/* must be non-zero */
			add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
					"Quit", MENU_UNSELECTED);
			Sprintf(pbuf, "Pick the race of your %s", plbuf);
			end_menu(win, pbuf);
			n = select_menu(win, PICK_ONE, &selected);
			destroy_nhwindow(win);
			if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

			k = selected[0].item.a_int - 1;
			free((genericptr_t) selected),	selected = 0;
		}
		flags.initrace = k;
		}
		(void)	root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}

	/* Select a gender, if necessary */
	/* force compatibility with role/race, try for compatibility with
	 * pre-selected alignment */
	if (flags.initgend < 0 || !validgend(flags.initrole, flags.initrace,
						flags.initgend)) {
		/* pre-selected gender not valid */
		if (pick4u == 'y' || flags.initgend == ROLE_RANDOM || flags.randomall) {
		flags.initgend = pick_gend(flags.initrole, flags.initrace,
						flags.initalign, PICK_RANDOM);
		if (flags.initgend < 0) {
			tty_putstr(BASE_WINDOW, 0, "Incompatible gender!");
			flags.initgend = randgend(flags.initrole, flags.initrace);
		}
		} else {	/* pick4u == 'n' */
		/* Count the number of valid genders */
		n = 0;	/* number valid */
		k = 0;	/* valid gender */
		for (i = 0; i < ROLE_GENDERS; i++) {
			if (ok_gend(flags.initrole, flags.initrace, i,
							flags.initalign)) {
			n++;
			k = i;
			}
		}
		if (n == 0) {
			for (i = 0; i < ROLE_GENDERS; i++) {
			if (validgend(flags.initrole, flags.initrace, i)) {
				n++;
				k = i;
			}
			}
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
			tty_clear_nhwindow(BASE_WINDOW);
			tty_putstr(BASE_WINDOW, 0, "Choosing Gender");
			win = create_nhwindow(NHW_MENU);
			start_menu(win);
			any.a_void = 0;			/* zero out all bits */
			for (i = 0; i < ROLE_GENDERS; i++)
			if (ok_gend(flags.initrole, flags.initrace, i,
								flags.initalign)) {
				any.a_int = i+1;
				add_menu(win, NO_GLYPH, &any, genders[i].adj[0],
				0, ATR_NONE, genders[i].adj, MENU_UNSELECTED);
			}
			any.a_int = pick_gend(flags.initrole, flags.initrace,
						flags.initalign, PICK_RANDOM)+1;
			if (any.a_int == 0) /* must be non-zero */
			any.a_int = randgend(flags.initrole, flags.initrace)+1;
			add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
					"Random", MENU_UNSELECTED);
			any.a_int = i+1;	/* must be non-zero */
			add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
					"Quit", MENU_UNSELECTED);
			Sprintf(pbuf, "Pick the gender of your %s", plbuf);
			end_menu(win, pbuf);
			n = select_menu(win, PICK_ONE, &selected);
			destroy_nhwindow(win);
			if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

			k = selected[0].item.a_int - 1;
			free((genericptr_t) selected),	selected = 0;
		}
		flags.initgend = k;
		}
		(void)	root_plselection_prompt(plbuf, QBUFSZ - 1,
			flags.initrole, flags.initrace, flags.initgend, flags.initalign);
	}

	/* Select an alignment, if necessary */
	/* force compatibility with role/race/gender */
	if (flags.initalign < 0 || !validalign(flags.initrole, flags.initrace,
							flags.initalign)) {
		/* pre-selected alignment not valid */
		if (pick4u == 'y' || flags.initalign == ROLE_RANDOM || flags.randomall) {
		flags.initalign = pick_align(flags.initrole, flags.initrace,
							flags.initgend, PICK_RANDOM);
		if (flags.initalign < 0) {
			tty_putstr(BASE_WINDOW, 0, "Incompatible alignment!");
			flags.initalign = randalign(flags.initrole, flags.initrace);
		}
		} else {	/* pick4u == 'n' */
		/* Count the number of valid alignments */
		n = 0;	/* number valid */
		k = 0;	/* valid alignment */
		for (i = 0; i < ROLE_ALIGNS; i++) {
			if (ok_align(flags.initrole, flags.initrace, flags.initgend,
							i)) {
			n++;
			k = i;
			}
		}
		if (n == 0) {
			for (i = 0; i < ROLE_ALIGNS; i++) {
			if (validalign(flags.initrole, flags.initrace, i)) {
				n++;
				k = i;
			}
			}
		}

		/* Permit the user to pick, if there is more than one */
		if (n > 1) {
			tty_clear_nhwindow(BASE_WINDOW);
			tty_putstr(BASE_WINDOW, 0, "Choosing Alignment");
			win = create_nhwindow(NHW_MENU);
			start_menu(win);
			any.a_void = 0;			/* zero out all bits */
			for (i = 0; i < ROLE_ALIGNS; i++)
			if (ok_align(flags.initrole, flags.initrace,
							flags.initgend, i)) {
				any.a_int = i+1;
				add_menu(win, NO_GLYPH, &any, aligns[i].adj[0],
				 0, ATR_NONE, aligns[i].adj, MENU_UNSELECTED);
			}
			any.a_int = pick_align(flags.initrole, flags.initrace,
						flags.initgend, PICK_RANDOM)+1;
			if (any.a_int == 0) /* must be non-zero */
			any.a_int = randalign(flags.initrole, flags.initrace)+1;
			add_menu(win, NO_GLYPH, &any , '*', 0, ATR_NONE,
					"Random", MENU_UNSELECTED);
			any.a_int = i+1;	/* must be non-zero */
			add_menu(win, NO_GLYPH, &any , 'q', 0, ATR_NONE,
					"Quit", MENU_UNSELECTED);
			Sprintf(pbuf, "Pick the alignment of your %s", plbuf);
			end_menu(win, pbuf);
			n = select_menu(win, PICK_ONE, &selected);
			destroy_nhwindow(win);
			if (n != 1 || selected[0].item.a_int == any.a_int)
			goto give_up;		/* Selected quit */

			k = selected[0].item.a_int - 1;
			free((genericptr_t) selected),	selected = 0;
		}
		flags.initalign = k;
		}
	}
	/* Success! */
	tty_display_nhwindow(BASE_WINDOW, FALSE);
}

#endif

/*
extern struct WinDesc *wins[MAXWIN];
*/
winid android_create_nhwindow(int type)
{
	winid newid = tty_create_nhwindow(type);

	if(newid >= 0 && newid < MAXWIN)
	{
		struct WinDesc *newwin = wins[newid];
		if(newwin)
		{
			if(newwin->type == NHW_STATUS)
			{
				newwin->offy = 0;
			}
		}
	}

	return newid;
}

extern int g_AndroidPureTTY;

void android_clear_nhwindow(winid window)
{
	struct WinDesc *cw = wins[window];

	if(cw && cw->type == NHW_MESSAGE)
	{
		android_puts("\033A1");
		android_puts("\033[H\033[J");

		s_MsgCol = 0;
		s_MsgRow = 0;
		android_puts("\033A0");
		return;
	}
	else if(cw && cw->type == NHW_STATUS)
	{
		android_puts("\033A2");
		android_puts("\033[H\033[J");

		android_puts("\033A0");
		return;
	}
#ifdef ANDROID_GRAPHICS_TILED
	else if(cw && cw->type == NHW_MAP && g_AndroidTiled && g_AndroidTilesEnabledForUser)
	{
		android_puts("\033A5");
		android_puts("\033[H\033[J");
		android_puts("\033A0");
		return;
	}
#endif

	tty_clear_nhwindow(window);
}

#if 0
static AndroidGameState s_PreMenuGameState = kAndroidGameStateInvalid;
#endif

/*
void android_display_nhwindow(winid window, boolean blocking)
*/
void android_display_nhwindow(winid window, BOOLEAN_P blocking)
{
    register struct WinDesc *cw = 0;

    if(window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
	{
		panic(winpanicstr,  window);
	}
    if(cw->flags & WIN_CANCELLED)
	{
		return;
	}

	if(cw->type == NHW_MENU || cw->type == NHW_TEXT)
	{
#if 0
if(s_PreMenuGameState == kAndroidGameStateInvalid)
{
		s_PreMenuGameState = android_getgamestate();
}
#endif
		if(!cw->active)
		{
			if(cw->type == NHW_MENU)
			{
				android_pushgamestate(kAndroidGameStateMenu);
			}
			else
			{
				android_pushgamestate(kAndroidGameStateText);
			}
		}

		android_puts("\033A4\033AS");

		cw->active = 1;
		cw->offx = 0;
	    cw->offy = 0;
		clear_screen();

		if (cw->data || !cw->maxrow)
			android_process_text_window(window, cw);
		else
			android_process_menu_window(window, cw);
		return;
	}

	tty_display_nhwindow(window, blocking);
}


static void android_dismiss_nhwindow(winid window)
{
	struct WinDesc *cw = 0;

    if(window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
	{
		panic(winpanicstr,  window);
	}

	switch(cw->type)
	{
		case NHW_MESSAGE:
			if (ttyDisplay->toplin)
				tty_display_nhwindow(WIN_MESSAGE, TRUE);
		/*FALLTHRU*/
		case NHW_STATUS:
		case NHW_BASE:
		case NHW_MAP:
			/*
			 * these should only get dismissed when the game is going away
			 * or suspending
			 */
			tty_curs(BASE_WINDOW, 1, (int)ttyDisplay->rows-1);
			cw->active = 0;
			break;
	    case NHW_MENU:
	    case NHW_TEXT:
			if(cw->active)
			{
				android_puts("\033A4\033AH");

				if (iflags.window_inited)
				{
					/* otherwise dismissing the text endwin after other windows
					 * are dismissed tries to redraw the map and panics.  since
					 * the whole reason for dismissing the other windows was to
					 * leave the ending window on the screen, we don't want to
					 * erase it anyway.
					 */
/*
					erase_menu_or_text(window, cw, FALSE);
*/
				    clear_screen();
			    }
			    cw->active = 0;

				android_puts("\033A0");

				android_popgamestate();
			}

#if 0
			if(s_PreMenuGameState != kAndroidGameStateInvalid)
			{
				android_setgamestate(s_PreMenuGameState);
				s_PreMenuGameState = kAndroidGameStateInvalid;
			}
#endif
			break;
    }
    cw->flags = 0;
}


void android_destroy_nhwindow(winid window)
{
    register struct WinDesc *cw = 0;

    if(window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
	panic(winpanicstr,  window);

    if(cw->active)
	{
		android_dismiss_nhwindow(window);
	}
    if(cw->type == NHW_MESSAGE)
	{
		iflags.window_inited = 0;
		if(cw->type == NHW_MAP)
			clear_screen();
	}

	android_free_window_info(cw, TRUE);
	free((genericptr_t)cw);
	wins[window] = 0;
}

void android_curs_status(winid window, int x, int y)
{
	/* Adapted from tty_curs(). */

    struct WinDesc *cw = 0;
    int cx = ttyDisplay->curx;
    int cy = ttyDisplay->cury;

    if(window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0)
	{
		panic(winpanicstr,  window);
	}

    ttyDisplay->lastwin = window;

    cw->curx = --x;	/* column 0 is never used */
    cw->cury = y;

    x += cw->offx;
    y += cw->offy;

	cmov(x, y);

    ttyDisplay->curx = x;
    ttyDisplay->cury = y;
}

void android_curs(winid window, int x, int y)
{
	struct WinDesc *cw = wins[window];
	if(cw && cw->type == NHW_MESSAGE)
	{
		/* HACK */
		int oldco = CO;
		CO = s_MessageNumColumns;

		android_puts("\033A1");
		tty_curs(window, x, y);
		android_puts("\033A0");

		CO = oldco;
		return;
	}
	else if(cw && cw->type == NHW_STATUS)
	{
		android_puts("\033A2");

		android_curs_status(window, x, y);

		android_puts("\033A0");
		return;
	}
	else if(cw && cw->type == NHW_MAP)
	{
		if(g_AndroidTilesEnabledForUser)
		{
			android_puts("\033A5");
			--x;	/* column 0 is never used */
			cmov(x, y);
			android_puts("\033A0");

			return;
		}
	}
	tty_curs(window, x, y);
}

#if 0

#endif

static void android_putstr_status(struct WinDesc *cw, const char *str)
{
	/* Adapted from tty_putstr(). */

	int i, j, n0;
	char *ob;
	const char *nb;

	android_puts("\033A2");

	ob = &cw->data[cw->cury][j = cw->curx];
	if(flags.botlx)
	{
		*ob = 0;
	}

	if(!cw->cury && (int)strlen(str) >= s_StatusNumColumns)
	{
	    /* the characters before "St:" are unnecessary */
	    nb = index(str, ':');
	    if(nb && nb > str+2)
			str = nb - 2;
	}
	nb = str;
	for(i = cw->curx + 1, n0 = cw->cols; i < n0; i++, nb++)
	{
	    if(!*nb)
		{
			if(*ob || flags.botlx)
			{
			    /* last char printed may be in middle of line */
				tty_curs(WIN_STATUS, i, cw->cury);
			    cl_end();
			}
			break;
	    }
	    if(*ob != *nb)
		{
			/* Adapted from tty_putsym() */

			tty_curs(WIN_STATUS, i, cw->cury);
			putchar(*nb);

			ttyDisplay->curx++;

			cw->curx++;
		}
	    if(*ob)
			ob++;
	}

	/* Keeping track of the old data here is important to support
	   colors in the status window - it works in a way that's a bit
	   funky, selecting the color first, and then reprinting the whole
	   line, relying on that this function only prints the characters
	   that have changed, using the new color. */
	strncpy(&cw->data[cw->cury][j], str, cw->cols - j - 1);
	cw->data[cw->cury][cw->cols-1] = '\0'; /* null terminate */
	cw->cury = (cw->cury+1) % 2;
	cw->curx = 0;

	android_puts("\033A0");
}


static void android_update_topl_word(const char *wordstart,
		const char *wordend)
{
	char buff[256];

	const int wordlen = wordend - wordstart;
	if(s_MsgCol > 0)
	{
		int maxcol = s_MessageNumColumns;

		if(s_MsgCol + wordlen + 1 > maxcol)
		{
			/* The word doesn't fit, advance to the next line, unless
			   we just wrapped around anyway. */
			if(s_MsgCol != s_MessageNumColumns)
			{
				android_puts("\n");
			}
			s_MsgCol = 0;
			s_MsgRow++;
		}
		else
		{
			android_puts(" ");
			s_MsgCol++;
		}
	}

	strncpy(buff, wordstart, sizeof(buff) - 1);
	if(wordlen < sizeof(buff))
	{
		buff[wordlen] = '\0';
	}
	else
	{
		buff[sizeof(buff) - 1] = '\0';
	}
	android_puts(buff);
	s_MsgCol += strlen(buff);
	while(s_MsgCol >= s_MessageNumColumns)
	{
		s_MsgCol -= s_MessageNumColumns;
		s_MsgRow++;
	}

	if(s_MsgRow >= s_NumMsgLines)
	{
		s_MsgRow = s_NumMsgLines - 1;
	}
}

/* Hopefully temporary */
int g_android_prevent_output = 0;

static void android_update_topl(const char *str)
{
	int i;
	const char *ptr;
	const char *wordstart = NULL;
	int foundend = 0;
	int continued = 0;
	int numspacesskipped = 0;

while(!foundend)
{
	ptr = str;
	if(s_MsgRow < s_NumMsgLines - 1 || continued)
	{
		wordstart = NULL;
		numspacesskipped = 0;
		while(1)
		{
			char c = *ptr++;
			if(c == ' ' || !c)
			{
				if(wordstart)
				{
					if(continued && (s_MsgRow == s_NumMsgLines - 1))
					{
#if 0
						if(s_MsgCol + ptr - 1 - wordstart >= s_ScreenNumColumns - 8)
#endif
						if(s_MsgCol + ptr - wordstart >= s_MessageNumColumns - 8)
						{
							ptr = wordstart;
							break;
						}
					}
					android_update_topl_word(wordstart, ptr - 1);
/* NOT SURE */
continued = 1;
					if(s_MsgRow >= s_NumMsgLines - 1 && !continued)
					{
						break;
					}
				}
				wordstart = NULL;

				if(!c)
				{
					/* Make sure to output a space at the end, if we found any
					   after the last word. This is needed to make the cursor
					   appear like it does in the pure TTY version, after a
					   question of some sort.
					   Note: not sure if it would ever be appropriate to output
					   more than one space here - for now, we only do one, as
					   perhaps there is a greater risk of something going wrong
					   otherwise. */
					if(numspacesskipped && s_MsgCol < s_MessageNumColumns - 1)
					{
						android_puts(" ");
						s_MsgCol++;
						numspacesskipped = 0;
					}

					foundend = 1;
					break;
				}
				else
				{
					numspacesskipped++;
				}
			}
			else if(!wordstart)
			{
				wordstart = ptr - 1;
			}
		}
	}

	if(!foundend)
	{
		str = ptr;

		const int len = strlen(str);
		int lastcol = s_MsgCol + len - 1;
		if(s_MsgCol)
		{
			lastcol++;
		}
		if(lastcol < s_MessageNumColumns - 8)	/* Room for --More-- */
		{
			if(s_MsgCol)
			{
				android_puts(" ");
				s_MsgCol++;
			}
			android_puts(str);
			s_MsgCol += len;
			foundend = 1;
		}
		else
		{
			android_puts("--More--");

			s_MsgCol += 8;

			g_android_prevent_output++;
			xwaitforspace("\033 ");
			g_android_prevent_output--;

			/* Not sure exactly why, but this seems to be necessary to prevent
			   some cases where the main (map) view could be cleared out. Since
			   this whole function is about the message line, we should be able
			   to safely send the code for selecting that here. */
			android_puts("\033A1");

			android_puts("\033[H\033[J");

			s_MsgCol = 0;
			s_MsgRow = 0;
			continued = 1;
		}
	}
}


#if 0
	register const char *bp = str;
	register char *tl, *otl;
	register int n0;
	int notdied = 1;

	/* If there is room on the line, print message on same line */
	/* But messages like "You die..." deserve their own line */
	n0 = strlen(bp);
	if ((ttyDisplay->toplin == 1 || (cw->flags & WIN_STOP)) &&
	    cw->cury == 0 &&
	    n0 + (int)strlen(toplines) + 3 < CO-8 &&  /* room for --More-- */
	    (notdied = strncmp(bp, "You die", 7))) {
		Strcat(toplines, "  ");
		Strcat(toplines, bp);
		cw->curx += 2;
		if(!(cw->flags & WIN_STOP))
		    android_addtopl(bp);
		return;
	} else if (!(cw->flags & WIN_STOP)) {
	    if(ttyDisplay->toplin == 1) more();
	    else if(cw->cury) {	/* for when flags.toplin == 2 && cury > 1 */
		docorner(1, cw->cury+1); /* reset cury = 0 if redraw screen */
		cw->curx = cw->cury = 0;/* from home--cls() & docorner(1,n) */
	    }
	}
#if 0
	remember_topl();
#endif
	(void) strncpy(toplines, bp, TBUFSZ);
	toplines[TBUFSZ - 1] = 0;

	for(tl = toplines; n0 >= CO; ){
	    otl = tl;
	    for(tl+=CO-1; tl != otl && !isspace(*tl); --tl) ;
	    if(tl == otl) {
		/* Eek!  A huge token.  Try splitting after it. */
		tl = index(otl, ' ');
		if (!tl) break;    /* No choice but to spit it out whole. */
	    }
	    *tl++ = '\n';
	    n0 = strlen(tl);
	}
	if(!notdied) cw->flags &= ~WIN_STOP;
#if 0
	if(!(cw->flags & WIN_STOP)) android_redotoplin(toplines);
#endif

#endif
}

void android_putstr_message(struct WinDesc *cw, const char *str)
{
	android_puts("\033A1");

#if 0
	android_puts(str);
#endif
	android_update_topl(str);

	android_puts("\033A0");

}

#if 0

void android_putstr_menu(struct WinDesc *cw, const char *str)
{
	android_puts("\033A4");

	android_puts(str);

	android_puts("\033A0");

}

#endif

void android_putstr_text(winid window, int attr,
		struct WinDesc *cw, const char *str)
{
	int i, n0;
	char *ob;

	android_puts("\033A4");

	android_puts(str);

	if(cw->type == NHW_TEXT && cw->cury == ttyDisplay->rows-1) {
	    /* not a menu, so save memory and output 1 page at a time */
	    cw->maxcol = ttyDisplay->cols; /* force full-screen mode */
		android_display_nhwindow(window, TRUE);
	    for(i=0; i<cw->maxrow; i++)
		if(cw->data[i]){
		    free((genericptr_t)cw->data[i]);
		    cw->data[i] = 0;
		}
	    cw->maxrow = cw->cury = 0;
	}
	/* always grows one at a time, but alloc 12 at a time */
	if(cw->cury >= cw->rows) {
	    char **tmp;

	    cw->rows += 12;
	    tmp = (char **) alloc(sizeof(char *) * (unsigned)cw->rows);
	    for(i=0; i<cw->maxrow; i++)
		tmp[i] = cw->data[i];
	    if(cw->data)
		free((genericptr_t)cw->data);
	    cw->data = tmp;

	    for(i=cw->maxrow; i<cw->rows; i++)
		cw->data[i] = 0;
	}
	if(cw->data[cw->cury])
	    free((genericptr_t)cw->data[cw->cury]);
	n0 = strlen(str) + 1;
	ob = cw->data[cw->cury] = (char *)alloc((unsigned)n0 + 1);
	*ob++ = (char)(attr + 1);	/* avoid nuls, for convenience */
	Strcpy(ob, str);

	if(n0 > cw->maxcol)
	    cw->maxcol = n0;
	if(++cw->cury > cw->maxrow)
	    cw->maxrow = cw->cury;
	if(n0 > CO) {
	    /* attempt to break the line */
	    for(i = CO-1; i && str[i] != ' ' && str[i] != '\n';)
		i--;
	    if(i) {
		cw->data[cw->cury-1][++i] = '\0';
		tty_putstr(window, attr, &str[i]);
	    }
	}

	android_puts("\033A0");

}


static void android_remember_topl(const char *str)
{
	/* This was adapted from the remember_topl() function in win/tty/topl.c
	   (which is static, and thus not usable from here without changing code
	   outside sys/android/. */

	register struct WinDesc *cw = wins[WIN_MESSAGE];
	int idx = cw->maxrow;
	unsigned len = strlen(toplines) + 1;

    if (len > (unsigned)cw->datlen[idx])
	{
		if (cw->data[idx])
		{
			free(cw->data[idx]);
		}
		len += (8 - (len & 7));		/* pad up to next multiple of 8 */
		cw->data[idx] = (char *)alloc(len);
		cw->datlen[idx] = (short)len;
    }
	Strcpy(cw->data[idx], toplines);
	cw->maxcol = cw->maxrow = (idx + 1) % cw->rows;

	strncpy(toplines, str, TBUFSZ);
	toplines[TBUFSZ - 1] = 0;
}


void android_putstr(winid window, int attr, const char *str)
{
	struct WinDesc *cw = wins[window];

#if 0
android_debuglog("msg %d: '%s'", cw ? cw->type : -1, str);
#endif

#if 0
	if(cw && cw->type == NHW_MESSAGE)
{
android_puts("\033A3");
char buff3[256];
sprintf(buff3, "[win=%d]", (int)cw->type);
android_puts(buff3);
android_puts(str);
android_puts("\033A0");
}
#endif

	if(cw && cw->type == NHW_MESSAGE)
	{
		/* Add to message history. */
		android_remember_topl(str);

		android_putstr_message(cw, str);

		return;
	}
	else if(cw && cw->type == NHW_STATUS)
	{
		android_putstr_status(cw, str);
		return;
	}
#if 1
	else if(cw && cw->type == NHW_TEXT)
	{
		android_puts("\033A4");
#if 0
		tty_putstr(window, attr, str);
#endif
		android_putstr_text(window, attr, cw, str);

		android_puts("\033A0");
		return;
	}
#endif
#if 0
	else if(cw && cw->type == NHW_MENU)
	{
android_debuglog("GOT MENU");
		android_putstr_menu(cw, str);
		return;
	}
#endif
	tty_putstr(window, attr, str);
}


void android_display_file(const char *fname, BOOLEAN_P complain)
{
	/* Adapted from tty_display_file(). We can't easily use that directly,
	   as it calls the other tty_...() functions directly. */

	dlb *f;
	char buf[BUFSZ];
	char *cr;

	tty_clear_nhwindow(WIN_MESSAGE);
	f = dlb_fopen(fname, "r");
	if(!f)
	{
		if(complain)
		{
			home();
			mark_synch();
			raw_print("");
			perror(fname);
			wait_synch();
			pline("Cannot open \"%s\".", fname);
	    }
		else if(u.ux)
		{
			docrt();
		}
	}
	else
	{
		winid datawin = create_nhwindow(NHW_TEXT);
		boolean empty = TRUE;

		if(complain
#ifndef NO_TERMS
			&& nh_CD
#endif
		)
		{
			/* attempt to scroll text below map window if there's room */
			wins[datawin]->offy = wins[WIN_STATUS]->offy+3;
			if((int) wins[datawin]->offy + 12 > (int) ttyDisplay->rows)
				wins[datawin]->offy = 0;
		}
		while(dlb_fgets(buf, BUFSZ, f))
		{
			if((cr = index(buf, '\n')) != 0)
				*cr = 0;
			if(index(buf, '\t') != 0)
				tabexpand(buf);
			empty = FALSE;
			putstr(datawin, 0, buf);
			if(wins[datawin]->flags & WIN_CANCELLED)
			    break;
	    }
	    if(!empty)
			display_nhwindow(datawin, FALSE);
		destroy_nhwindow(datawin);
	    dlb_fclose(f);
    }
}


int android_select_menu(winid window, int how, menu_item **menu_list)
{
	/* Adapted from tty_select_menu(). */

	struct WinDesc *cw = 0;
	tty_menu_item *curr;
	menu_item *mi;
	int n, cancelled;

#if 0
	android_puts("\033A4\033AS");
#endif
	if(window == WIN_ERR || (cw = wins[window]) == (struct WinDesc *) 0
			|| cw->type != NHW_MENU)
	{
		panic(winpanicstr,  window);
	}

	*menu_list = (menu_item *) 0;
	cw->how = (short) how;
	morc = 0;

	android_display_nhwindow(window, TRUE);
	cancelled = !!(cw->flags & WIN_CANCELLED);
#if 0
	tty_dismiss_nhwindow(window);	/* does not destroy window data */
#endif
	android_dismiss_nhwindow(window);	/* does not destroy window data */

 	if(cancelled)
	{
		n = -1;
    }
	else
	{
		for (n = 0, curr = cw->mlist; curr; curr = curr->next)
		{
			if(curr->selected)
			{
				n++;
			}
		}
    }

	if(n > 0)
	{
		*menu_list = (menu_item *) alloc(n * sizeof(menu_item));
		for(mi = *menu_list, curr = cw->mlist; curr; curr = curr->next)
		{
			if(curr->selected)
			{
				mi->item = curr->identifier;
				mi->count = curr->count;
				mi++;
	    	}
    	}
	}

	android_puts("\033AH\033A0");

	return n;
}

typedef boolean FDECL((*getlin_hook_proc), (char *));
extern char erase_char, kill_char;	/* from appropriate tty.c file */
STATIC_DCL boolean FDECL(ext_cmd_getlin_hook, (char *));
extern int NDECL(extcmd_via_menu);	/* cmd.c */

STATIC_OVL void
android_hooked_tty_getlin(query, bufp, hook)
const char *query;
register char *bufp;
getlin_hook_proc hook;
{
	register char *obufp = bufp;
	register int c;
	struct WinDesc *cw = wins[WIN_MESSAGE];
	boolean doprev = 0;

	if(ttyDisplay->toplin == 1 && !(cw->flags & WIN_STOP)) more();
	cw->flags &= ~WIN_STOP;
	ttyDisplay->toplin = 3; /* special prompt state */
	ttyDisplay->inread++;
#if 0
	pline("%s ", query);
#endif
	android_printf("%s ", query);
	*obufp = 0;
	for(;;) {
#if 0
		(void) fflush(stdout);
#endif
		Sprintf(toplines, "%s ", query);
		Strcat(toplines, obufp);
		if((c = Getchar()) == EOF) {
			break;
		}
		if(c == '\033') {
			*obufp = c;
			obufp[1] = 0;
			break;
		}
		if (ttyDisplay->intr) {
		    ttyDisplay->intr--;
		    *bufp = 0;
		}
		if(c == '\020') { /* ctrl-P */
		    if (iflags.prevmsg_window != 's') {
			int sav = ttyDisplay->inread;
			ttyDisplay->inread = 0;
			(void) tty_doprev_message();
			ttyDisplay->inread = sav;
			tty_clear_nhwindow(WIN_MESSAGE);
			cw->maxcol = cw->maxrow;
			addtopl(query);
			addtopl(" ");
			*bufp = 0;
			addtopl(obufp);
		    } else {
			if (!doprev)
			    (void) tty_doprev_message();/* need two initially */
			(void) tty_doprev_message();
			doprev = 1;
			continue;
		    }
		} else if (doprev && iflags.prevmsg_window == 's') {
		    tty_clear_nhwindow(WIN_MESSAGE);
		    cw->maxcol = cw->maxrow;
		    doprev = 0;
		    addtopl(query);
		    addtopl(" ");
		    *bufp = 0;
		    addtopl(obufp);
		}
		if(c == erase_char || c == '\b') {
			if(bufp != obufp) {
				char *i;
				bufp--;
				android_puts("\b");
				for (i = bufp; *i; ++i) android_puts(" ");
				for (; i > bufp; --i) android_puts("\b");
				*bufp = 0;
			} else	tty_nhbell();
		} else if(c == '\n') {
			break;
		} else if(' ' <= (unsigned char) c && c != '\177' &&
			    (bufp-obufp < BUFSZ-1 && bufp-obufp < COLNO)) {
				/* avoid isprint() - some people don't have it
				   ' ' is not always a printing char */
			char *i = eos(bufp);
			*bufp = c;
			bufp[1] = 0;
			android_puts(bufp);
			bufp++;
			if (hook && (*hook)(obufp)) {
			    android_puts(bufp);
			    /* pointer and cursor left where they were */
			    for (i = bufp; *i; ++i) android_puts("\b");
			} else if (i > bufp) {
			    char *s = i;

			    /* erase rest of prior guess */
			    for (; i > bufp; --i) android_puts(" ");
			    for (; s > bufp; --s) android_puts("\b");
			}
		} else if(c == kill_char || c == '\177') { /* Robert Viduya */
				/* this test last - @ might be the kill_char */
			for (; *bufp; ++bufp) android_puts(" ");
			for (; bufp != obufp; --bufp) android_puts("\b \b");
			*bufp = 0;
		} else
			tty_nhbell();
	}
	ttyDisplay->toplin = 2;		/* nonempty, no --More-- required */
	ttyDisplay->inread--;
	clear_nhwindow(WIN_MESSAGE);	/* clean up after ourselves */
}


static void sToggleCursor(void)
{
	android_puts("\033AC");
}

#ifndef C	/* this matches src/cmd.c */
#define C(c)	(0x1f & (c))
#endif

static void android_redotoplin(const char *str)
{
	android_puts("\033A1");

	/* This is what the regular TTY redotoplin() function does -
	   not sure if we need to do any more of that stuff. */
#if 0
	int otoplin = ttyDisplay->toplin;
	home();
	if(*str & 0x80) {
		/* kludge for the / command, the only time we ever want a */
		/* graphics character on the top line */
		g_putch((int)*str++);
		ttyDisplay->curx++;
	}
	end_glyphout();	/* in case message printed during graphics output */
	putsyms(str);
	cl_end();
	ttyDisplay->toplin = 1;
	if(ttyDisplay->cury && otoplin != 3)
		more();
#endif

	android_puts("\033[H\033[J");

	s_MsgCol = 0;
	s_MsgRow = 0;

	android_update_topl(str);

	android_puts("\033A0");
}


int android_nh_poskey(int *x, int *y, int *mod)
{
    (void) fflush(stdout);
    /* Note: if raw_print() and wait_synch() get called to report terminal
     * initialization problems, then wins[] and ttyDisplay might not be
     * available yet.  Such problems will probably be fatal before we get
     * here, but validate those pointers just in case...
     */
    if (WIN_MESSAGE != WIN_ERR && wins[WIN_MESSAGE])
	    wins[WIN_MESSAGE]->flags &= ~WIN_STOP;

	int ch, clickx, clicky;

	android_process_input(&ch, &clickx, &clicky);

	if(clickx >= 0)
	{
		/* Not entirely sure about the +1 here, but I think I've seen this
		   elsewhere in the NH code - column 0 doesn't seem to be used. */
		*x = clickx + 1;
		*y = clicky;
		*mod = CLICK_1;
		return 0;
	}


	/* See tty_nhgetch() - not sure how much of this we really need to do. */
    if (!ch) ch = '\033'; /* map NUL to ESC since nethack doesn't expect NUL */
    if (ttyDisplay && ttyDisplay->toplin == 1)
	ttyDisplay->toplin = 2;
    return ch;
}


int android_doprev_message()
{
	/* This is an unfortunate copy of tty_doprev_message, with the only
	   difference being that we call android_redotoplin() instead of
	   redotoplin() in 'win/tty/topl.c', to print in the right window. */

    register struct WinDesc *cw = wins[WIN_MESSAGE];

    winid prevmsg_win;
    int i;
    if ((iflags.prevmsg_window != 's') && !ttyDisplay->inread) { /* not single */
        if(iflags.prevmsg_window == 'f') { /* full */
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            cw->maxcol = cw->maxrow;
            i = cw->maxcol;
            do {
                if(cw->data[i] && strcmp(cw->data[i], "") )
                    putstr(prevmsg_win, 0, cw->data[i]);
                i = (i + 1) % cw->rows;
            } while (i != cw->maxcol);
            putstr(prevmsg_win, 0, toplines);
            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
        } else if (iflags.prevmsg_window == 'c') {		/* combination */
            do {
                morc = 0;
                if (cw->maxcol == cw->maxrow) {
                    ttyDisplay->dismiss_more = C('p');	/* <ctrl/P> allowed at --More-- */
                    android_redotoplin(toplines);
                    cw->maxcol--;
                    if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else if (cw->maxcol == (cw->maxrow - 1)){
                    ttyDisplay->dismiss_more = C('p');	/* <ctrl/P> allowed at --More-- */
                    android_redotoplin(cw->data[cw->maxcol]);
                    cw->maxcol--;
                    if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                    if (!cw->data[cw->maxcol])
                        cw->maxcol = cw->maxrow;
                } else {
                    prevmsg_win = create_nhwindow(NHW_MENU);
                    putstr(prevmsg_win, 0, "Message History");
                    putstr(prevmsg_win, 0, "");
                    cw->maxcol = cw->maxrow;
                    i = cw->maxcol;
                    do {
                        if(cw->data[i] && strcmp(cw->data[i], "") )
                            putstr(prevmsg_win, 0, cw->data[i]);
                        i = (i + 1) % cw->rows;
                    } while (i != cw->maxcol);
                    putstr(prevmsg_win, 0, toplines);
                    display_nhwindow(prevmsg_win, TRUE);
                    destroy_nhwindow(prevmsg_win);
                }

            } while (morc == C('p'));
            ttyDisplay->dismiss_more = 0;
        } else { /* reversed */
            morc = 0;
            prevmsg_win = create_nhwindow(NHW_MENU);
            putstr(prevmsg_win, 0, "Message History");
            putstr(prevmsg_win, 0, "");
            putstr(prevmsg_win, 0, toplines);
            cw->maxcol=cw->maxrow-1;
            if(cw->maxcol < 0) cw->maxcol = cw->rows-1;
            do {
                putstr(prevmsg_win, 0, cw->data[cw->maxcol]);
                cw->maxcol--;
                if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
                if (!cw->data[cw->maxcol])
                    cw->maxcol = cw->maxrow;
            } while (cw->maxcol != cw->maxrow);

            display_nhwindow(prevmsg_win, TRUE);
            destroy_nhwindow(prevmsg_win);
            cw->maxcol = cw->maxrow;
            ttyDisplay->dismiss_more = 0;
        }
    } else if(iflags.prevmsg_window == 's') { /* single */
        ttyDisplay->dismiss_more = C('p');  /* <ctrl/P> allowed at --More-- */
        do {
            morc = 0;
            if (cw->maxcol == cw->maxrow)
                android_redotoplin(toplines);
            else if (cw->data[cw->maxcol])
                android_redotoplin(cw->data[cw->maxcol]);
            cw->maxcol--;
            if (cw->maxcol < 0) cw->maxcol = cw->rows-1;
            if (!cw->data[cw->maxcol])
                cw->maxcol = cw->maxrow;
        } while (morc == C('p'));
        ttyDisplay->dismiss_more = 0;
    }
    return 0;
}


char android_yn_function(const char *query, const char *resp, CHAR_P def)
{
	char ret;

	android_pushgamestate(kAndroidGameStateWaitingForResponse);

	android_puts("\033A1");
	sToggleCursor();		/* Toggle cursor on. */
	android_puts("\033A0");

	ret = tty_yn_function(query, resp, def);

	android_puts("\033A1");
	sToggleCursor();		/* Toggle cursor off again. */
	android_puts("\033A0");

	android_popgamestate();

	return ret;
}



void android_getlin(const char *query, char *bufp)
{
	android_pushgamestate(kAndroidGameStateWaitingForResponse);

	android_puts("\033A1");
	sToggleCursor();		/* Toggle cursor on. */
/*
	android_puts("\033A0");
*/
	android_hooked_tty_getlin(query, bufp, (getlin_hook_proc)0);

	android_puts("\033A1");
	sToggleCursor();		/* Toggle cursor off again. */
	android_puts("\033A0");

	android_popgamestate();
}



/*
 * Read in an extended command, doing command line completion.  We
 * stop when we have found enough characters to make a unique command.
 */
int android_get_ext_cmd()
{
	int ret;

	android_puts("\033A1");

#if 1
	int i;
	char buf[BUFSZ];

	if (iflags.extmenu) return extcmd_via_menu();
	/* maybe a runtime option? */
	/* hooked_tty_getlin("#", buf, flags.cmd_comp ? ext_cmd_getlin_hook : (getlin_hook_proc) 0); */

	android_pushgamestate(kAndroidGameStateExtCmd);

	sToggleCursor();		/* Toggle cursor on. */

#ifdef REDO
	android_hooked_tty_getlin("#", buf, in_doagain ? (getlin_hook_proc)0
		: ext_cmd_getlin_hook);
#else
	android_hooked_tty_getlin("#", buf, ext_cmd_getlin_hook);
#endif

	android_puts("\033A1");
	sToggleCursor();		/* Toggle cursor off again. */

	android_popgamestate();

	(void) mungspaces(buf);
	if (buf[0] == 0 || buf[0] == '\033') return -1;

	for (i = 0; extcmdlist[i].ef_txt != (char *)0; i++)
		if (!strcmpi(buf, extcmdlist[i].ef_txt)) break;

#ifdef REDO
	if (!in_doagain) {
	    int j;
	    for (j = 0; buf[j]; j++)
		savech(buf[j]);
	    savech('\n');
	}
#endif

	if (extcmdlist[i].ef_txt == (char *)0) {
		pline("%s: unknown extended command.", buf);
		i = -1;
	}
#endif

	android_puts("\033A0");

	return i;
}


/*
 * Implement extended command completion by using this hook into
 * tty_getlin.  Check the characters already typed, if they uniquely
 * identify an extended command, expand the string to the whole
 * command.
 *
 * Return TRUE if we've extended the string at base.  Otherwise return FALSE.
 * Assumptions:
 *
 *	+ we don't change the characters that are already in base
 *	+ base has enough room to hold our string
 */
STATIC_OVL boolean
ext_cmd_getlin_hook(base)
	char *base;
{
	int oindex, com_index;

	com_index = -1;
	for (oindex = 0; extcmdlist[oindex].ef_txt != (char *)0; oindex++) {
		if (!strncmpi(base, extcmdlist[oindex].ef_txt, strlen(base))) {
			if (com_index == -1)	/* no matches yet */
			    com_index = oindex;
			else			/* more than 1 match */
			    return FALSE;
		}
	}
	if (com_index >= 0) {
		Strcpy(base, extcmdlist[com_index].ef_txt);
		return TRUE;
	}

	return FALSE;	/* didn't match anything */
}

/* End of file winandroid.c */
