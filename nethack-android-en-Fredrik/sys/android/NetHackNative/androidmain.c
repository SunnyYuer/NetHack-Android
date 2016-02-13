/* androidmain.c */

#include "hack.h"
#include "dlb.h"
#include "wintty.h"

#include "comm.h"

#include <string.h>
#include <jni.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

static unsigned char s_CharReceiveBuff[RECEIVEBUFFSZ + 1];
static unsigned char s_SendBuff[SENDBUFFSZ + 1];
static volatile int s_SendCnt;

static int s_CharReceiveCnt;

static pthread_mutex_t s_SendMutex;
static sem_t s_SendNotFullSema;
static sem_t s_CommandPerformedSema;

static int s_SendWaitingForNotFull;
static int s_WaitingForCommandPerformed;

static int s_PlayerPosShouldRecenter = 0;	/* Protected by s_ReceiveMutex */
static int s_PlayerPosX = 0;				/* Protected by s_ReceiveMutex */
static int s_PlayerPosY = 0;				/* Protected by s_ReceiveMutex */
static d_level s_PrevDungeonLevel;

static AndroidGameState s_GameState = kAndroidGameStateInvalid;

enum
{
	kGameStateStackSize = 1
};
static AndroidGameState s_GameStateStack[kGameStateStackSize];
static int s_GameStateStackCount = 0;

enum
{
	kInputEventKeys,
	kInputEventMapTap,
	kInputEventDir
};

/* Note: must match enum MoveDir on the Java side. */
enum MoveDir
{
	kMoveDirNone,
	kMoveDirUpLeft,
	kMoveDirUp,
	kMoveDirUpRight,
	kMoveDirLeft,
	kMoveDirCenter,
	kMoveDirRight,
	kMoveDirDownLeft,
	kMoveDirDown,
	kMoveDirDownRight,

	/* Not an actual direction, but a bit we use in the message to indicate
	   that a context-sensitive interpretation of the direction is allowed. */
	kMoveDirAllowContextSensitive = 0x80
};

static void sPushInputEvent(unsigned char eventtype, uint16_t datalen,
		const void *data)
{
}


enum
{
	kCmdNone,
	kCmdSave,
	kCmdRefresh,
	kCmdSwitchTo128,
	kCmdSwitchToAmiga,
	kCmdSwitchToIBM,
	kCmdTilesEnable,
	kCmdTilesDisable
};

enum
{
	kCharSet128,
	kCharSetIBM,
	kCharSetAmiga
};

static int s_CurrentCharSet = kCharSet128;
static int s_InitCharSet = kCharSet128;

static int s_ReadyForSave = 0;
static int s_Command = kCmdNone;
static int s_Quit = 0;
int g_AndroidPureTTY = 0;

#ifdef ANDROID_GRAPHICS_TILED

/* Whether the user has requested tiled mode or not. Even if we are on the
   Rogue level and thus not actually in tiled mode, this would still be true. */
int g_AndroidTiled = 0;

/* If we think the user (Android UI) is currently drawing a tiled view, this
   is true. The native code is the master here, sending control signals to
   drive the Android UI views to the expected state, and this variable is used
   to decide when to send those signals. */
int g_AndroidTilesEnabledForUser = 0;

#endif	/* ANDROID_GRAPHICS_TILED */

static void NDECL(wd_message);
#ifdef WIZARD
static boolean wiz_error_flag = FALSE;
#endif

#if 0	/* Enable for debug output */

void android_debuglog(const char *fmt, ...)
{
	char buff[1024];
	int r;

	va_list args;
	va_start(args, fmt);
	r = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

	android_puts("\033A3");
	android_puts(buff);
	android_puts("\033A0");
}


void android_debugerr(const char *fmt, ...)
{
	/* TODO: Make this more sophisticated. */

	char buff[1024];
	int r;

	va_list args;
	va_start(args, fmt);
	r = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

	android_puts("\033A3");
	android_puts(buff);
	android_puts("\033A0");
}

#else

void android_debuglog(const char *fmt, ...)
{
}

void android_debugerr(const char *fmt, ...)
{
}

#endif


void error(const char *s, ...)
{
	char buff[1024];

	va_list args;
	va_start(args, s);

/*
	if(settty_needed)
		settty((char *)0);
*/
	vsnprintf(buff, sizeof(buff), s, args);
	android_putchar('\n');
	android_puts(buff);
	android_putchar('\n');

	/* Sleep for a bit so we see something even if the user is typing.*/
	sleep(1);
	getchar();

	exit(1);
}

AndroidGameState android_getgamestate()
{
	return s_GameState;
}


static const char *s_statenames[] =
{
	"Invalid",
	"ExtCmd",
	"GetPos",
	"Init",
	"Menu",
	"MoveLoop",
	"Text",
	"WaitingForResponse"
};


void android_switchgamestate(AndroidGameState s)
{
	if(s != s_GameState)
	{
		const char *statename;
		if(s >= 0 && s < kAndroidNumGameStates)
		{
			statename = s_statenames[s];
		}
		else
		{
			statename = "?";
		}
		android_debuglog("Switching state to %s.", statename);
	}
	s_GameState = s;
}



void android_pushgamestate(AndroidGameState s)
{
	if(s_GameStateStackCount < kGameStateStackSize)
	{
		const char *statename;
		if(s >= 0 && s < kAndroidNumGameStates)
		{
			statename = s_statenames[s];
		}
		else
		{
			statename = "?";
		}

		android_debuglog("Pushing state %s.", statename);
		s_GameStateStack[s_GameStateStackCount++] = s_GameState;

		s_GameState = s;
	}
	else
	{
		android_debugerr("Game state stack full.");
	}
}


void android_popgamestate()
{
	if(s_GameStateStackCount >= 0)
	{
		s_GameState = s_GameStateStack[--s_GameStateStackCount];

		const char *statename;
		if(s_GameState >= 0 && s_GameState < kAndroidNumGameStates)
		{
			statename = s_statenames[s_GameState];
		}
		else
		{
			statename = "?";
		}
		android_debuglog("Popping state back to %s.", statename);
	}
	else
	{
		android_debugerr("Popped from empty stack.");
	}
}


void android_setcharset(int charsetindex)
{
	switch(charsetindex)
	{
		case kCharSetAmiga:
			read_config_file("charset_amiga.cnf");
			s_CurrentCharSet = kCharSetAmiga;
			break;
		case kCharSetIBM:
			read_config_file("charset_ibm.cnf");
			s_CurrentCharSet = kCharSetIBM;
			break;
		case kCharSet128:
			read_config_file("charset_128.cnf");
			s_CurrentCharSet = kCharSet128;
			break;
		default:
			break;
	}
}


struct AndroidUnicodeRemap
{
	char		ascii;
	uint16_t	unicode;
};

static const struct AndroidUnicodeRemap s_IbmGraphicsRemap[] =
{
	{	0xb3, 0x2502	},
	{	0xc4, 0x2500	},
	{	0xda, 0x250c	},
	{	0xbf, 0x2510	},
	{	0xc0, 0x2514	},
	{	0xd9, 0x2518	},
	{	0xc5, 0x253c	},
	{	0xc1, 0x2534	},
	{	0xc2, 0x252c	},
	{	0xb4, 0x2524	},
	{	0xc3, 0x251c	},
/*	{	0xb0, 0x2591	}, 		Missing in Droid monospace font? */
/*	{	0xb0, '#'		},*/
	{	0xb0, 0x7000 + 256	},	/* Use extra char from "Amiga" font. */
	{	0xb1, 0x2592	},
	{	0xf0, 0x2261	},
	{	0xf1, 0x00b1	},
/*	{	0xf4, 0x2320	},		Missing in Droid monospace font? */
/*	{	0xf4, '{'		}, */
	{	0xf4, 0x7000 + 257	},	/* Use extra char from "Amiga" font. */
	{	0xf7, 0x2248	},
	{	0xfa, 0x00b7	},
	{	0xfe, 0x25a0	},

	{	0xba, 0x2551	},
	{	0xcd, 0x2550	},
	{	0xc9, 0x2554	},
	{	0xbb, 0x2557	},
	{	0xc8, 0x255a	},
	{	0xbc, 0x255d	},
	{	0xce, 0x256c	},
	{	0xca, 0x2569	},
	{	0xcb, 0x2566	},
	{	0xb9, 0x2563	},
	{	0xcc, 0x2560	},
/*	{	0x01, 0x263b	},	Missing from font */
	{	0x01, 0x7000 + 258	},
	{	0x04, 0x2666	},	/* Working */
	{	0x05, 0x2663	},	/* Working */
	{	0x0c, 0x2640	},	/* Working */
/*	{	0x0e, 0x266b	},	Missing */
	{	0x0e, 0x7000 + 259	},
/*	{	0x0f, 0x263c	},	Missing */
	{	0x0f, 0x7000 + 260	},
	{	0x18, 0x2191	},	/* Working */
	{	0xad, 0xa1		},	/* Working */
	{	0xb2, 0x2593	},	/* Working */
	{	0xe7, 0x3c4		},	/* Working */



	{	0x00, 0x0000	}
};

static void android_putchar_internal(int c)
{
	while(1)
	{
		pthread_mutex_lock(&s_SendMutex);

		uint16_t unicode = c;

		if(s_CurrentCharSet != kCharSetAmiga)
		{
			if(c >= 128 || c < 32)
			{
				int found = 0;

				/* Here, we map the relevant extended characters from MSDOS
					to their Unicode equivalent. */

				const struct AndroidUnicodeRemap *remapPtr = s_IbmGraphicsRemap;

				unicode = 0xbf;	/* Inverted question mark to indicate unknown */

				/* TODO: Would be nice with binary search here. */
				while(remapPtr->ascii)
				{
					if(remapPtr->ascii == c)
					{
						unicode = remapPtr->unicode;
						found = 1;
						break;
					}
					remapPtr++;
				}

				if(c < 32 && !found)
				{
					unicode = c;	/* Preserve \n, etc. */
				}
			}
		}
		else
		{
			if(c >= 128)
			{
				unicode = 0x7000 + (unsigned int)c;
			}
		}

		/* Store the Unicode in the buffer using UTF8 encoding. Note:
		   we could potentially just store them with 2 byte per character
		   Unicode in the array. */

		if(unicode >= 0x800)
		{
			if(s_SendCnt < SENDBUFFSZ - 2)
			{
				unsigned char c1 = 0xe0 + ((unicode & 0xf000) >> 12);
				unsigned char c2 = 0x80 + ((unicode & 0x0f00) >> 6) + ((unicode & 0x00c0) >> 6);
				unsigned char c3 = 0x80 + (unicode & 0x003f);

				s_SendBuff[s_SendCnt++] = (char)c1;
				s_SendBuff[s_SendCnt++] = (char)c2;
				s_SendBuff[s_SendCnt++] = (char)c3;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}
		}
		else if(unicode >= 0x80)
		{
			if(s_SendCnt < SENDBUFFSZ - 1)
			{
				unsigned char c1 = 0xc0 + ((unicode & 0x0700) >> 6) + ((unicode & 0x00c0) >> 6);
				unsigned char c2 = 0x80 + (unicode & 0x003f);

				s_SendBuff[s_SendCnt++] = (char)c1;
				s_SendBuff[s_SendCnt++] = (char)c2;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}

		}
		else
		{
			if(s_SendCnt < SENDBUFFSZ)
			{
				s_SendBuff[s_SendCnt++] = (char)unicode;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}
		}

		s_SendWaitingForNotFull = 1;

		pthread_mutex_unlock(&s_SendMutex);

		sem_wait(&s_SendNotFullSema);
	}
}


/* TEMP */
void android_putchar_internal2(int c)
{
	while(1)
	{
		pthread_mutex_lock(&s_SendMutex);

		uint16_t unicode = c;

		/* Store the Unicode in the buffer using UTF8 encoding. Note:
		   we could potentially just store them with 2 byte per character
		   Unicode in the array. */

		if(unicode >= 0x800)
		{
			if(s_SendCnt < SENDBUFFSZ - 2)
			{
				unsigned char c1 = 0xe0 + ((unicode & 0xf000) >> 12);
				unsigned char c2 = 0x80 + ((unicode & 0x0f00) >> 6) + ((unicode & 0x00c0) >> 6);
				unsigned char c3 = 0x80 + (unicode & 0x003f);

				s_SendBuff[s_SendCnt++] = (char)c1;
				s_SendBuff[s_SendCnt++] = (char)c2;
				s_SendBuff[s_SendCnt++] = (char)c3;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}
		}
		else if(unicode >= 0x80)
		{
			if(s_SendCnt < SENDBUFFSZ - 1)
			{
				unsigned char c1 = 0xc0 + ((unicode & 0x0700) >> 6) + ((unicode & 0x00c0) >> 6);
				unsigned char c2 = 0x80 + (unicode & 0x003f);

				s_SendBuff[s_SendCnt++] = (char)c1;
				s_SendBuff[s_SendCnt++] = (char)c2;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}

		}
		else
		{
			if(s_SendCnt < SENDBUFFSZ)
			{
				s_SendBuff[s_SendCnt++] = (char)unicode;

				pthread_mutex_unlock(&s_SendMutex);

				return;
			}
		}

		s_SendWaitingForNotFull = 1;

		pthread_mutex_unlock(&s_SendMutex);

		sem_wait(&s_SendNotFullSema);
	}
}


void android_putchar(int c)
{

	android_putchar_internal(c);

}


void android_puts(const char *s)
{
	const char *ptr = s;

	while(*s)
	{
		android_putchar_internal((int)(*s++));
	}
}


int android_printf(const char *fmt, ...)
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



void android_makelock()
{
#ifdef WIZARD
	if(!wizard) {
#endif
		/*
		 * check for multiple games under the same name
		 * (if !locknum) or check max nr of players (otherwise)
		 */
		(void) signal(SIGQUIT,SIG_IGN);
		(void) signal(SIGINT,SIG_IGN);
		if(!locknum)
			Sprintf(lock, "%d%s", (int)getuid(), plname);
#ifdef WIZARD
	} else {
		Sprintf(lock, "%d%s", (int)getuid(), plname);
	}
#endif /* WIZARD */
}


#define AUTOSAVE_FILENAME "android_autosave.txt"


/* The last time we auto-saved (probably due to Activity pause), this
   was the value of the "moves" variable. */
static int s_MovesOnLastAutoSave = -1;

void android_autosave_save()
{
	/* If the move counter hasn't increased since the last time we saved,
	   we assume that there is nothing of importance that needs to be saved.
	   This is done so that when switching between different activities
	   (for example bringing up the preference menu), we don't slow down
	   the UI to save the state each time. */
	if(moves != s_MovesOnLastAutoSave)
	{
		FILE *f = fopen(AUTOSAVE_FILENAME, "w");
		if(f)
		{
			fprintf(f, "%s\n", plname);
			fclose(f);
		}

#ifdef INSURANCE
		save_currentstate();

		s_MovesOnLastAutoSave = moves;
#else
# error "Can't save without INSURANCE."
#endif
	}
}


void android_autosave_restore()
{
	char name[PL_NSIZ + 1];
	int found = 0;

	FILE *f = fopen(AUTOSAVE_FILENAME, "r");
	if(!f)
	{
		return;
	}
	if(fgets(name, PL_NSIZ + 1, f))
	{
		char *ptr = name + strlen(name);
		while(ptr >= name)
		{
			char c = *ptr;
			if(c && c != ' ' && c != '\n' && c != '\t' && c != '\r')
			{
				found = 1;
				ptr[1] = '\0';
				break;
			}
			ptr--;
		}
	}
	fclose(f);

	if(!found)
	{
		return;
	}

	strncpy(plname, name, PL_NSIZ - 1);
	plname[PL_NSIZ - 1] = '\0';

	android_makelock();

	if(TMP_restore_savefile(lock) == 0)
	{
		/* Success */
	}
	else
	{
	}
}


void android_autosave_remove()
{
	remove(AUTOSAVE_FILENAME);
}

/* If we need the status lines to redraw, we may need to clear out the
   previous state like this - if not, characters may not be reprinted if
   android_putstr_status() thinks they are already there.
*/
static void android_clear_winstatus_state()
{
	struct WinDesc *cw = wins[WIN_STATUS];
	int i, j;

	if(cw)
	{
		for(i = 0; i < cw->maxrow; i++)
		{
			if(cw->data[i])
			{
				for(j = 0; j < cw->maxcol; j++)
				{
					cw->data[i][j] = '\0';	/* Should this be ' ' instead? */
				}
			}
		}
	}
}

/*
extern int g_android_prevent_output;
*/
static int s_ShouldRefresh = 0;
static int s_SwitchCharSetCmd = -1;




static void android_push_char(unsigned char c)
{
	if(s_CharReceiveCnt >= RECEIVEBUFFSZ)
	{
		/* Buffer filled up, should be avoided, but if it happens, dropping
		   the new character may be more reasonable than dropping the
		   oldest or doing anything else - trying to wait for consumption
		   with threading and stuff is probably too much risk to get a
		   freeze or something. */

		return;
	}

	s_CharReceiveBuff[s_CharReceiveCnt++] = c;
}


static void android_process_input_event_keys()
{
	while(!android_msgq_is_empty())
	{
		unsigned char c = android_msgq_pop_byte();
		if(c)
		{
			android_push_char(c);
		}
		else
		{
			break;
		}
	}
}


static void android_process_input_event_dir()
{
	/* Look-up table for mapping our enum MoveDir to the direction code
	   that NetHack uses. Perhaps we should change it to just use the same. */
	static signed char s_nhindex[] =
	{
		-1,		/* None */
		1,		/* UpLeft */
		2,		/* Up */
		3,		/* UpRight */
		0,		/* Left */
		-1,		/* Center */
		4,		/* Right */
		7,		/* DownLeft */
		6,		/* Down */
		5		/* DownRight */
	};
	
	/* Get the direction, and map it a NetHack direction code. */
	unsigned char c = android_msgq_pop_byte();

	/* Check if we are in the regular movement loop. If not, we should
	   stay away from trying to generate characters for context-sensitive
	   commands. */
	int allowcontext = (android_getgamestate() == kAndroidGameStateMoveLoop);

	/* Strip and store the bit for if context-sensitive actions are allowed. */
	if(!(c & kMoveDirAllowContextSensitive))
	{
		allowcontext = 0;
	}
	c &= ~kMoveDirAllowContextSensitive;

	int index = -1;
	if(c < 10)
	{
		index = s_nhindex[c];
	}

	/* Variables for a potential destination position. */
	int destx = u.ux, desty = u.uy;

	if(index >= 0)
	{
		if(!allowcontext)
		{
			/* Map the direction to a character, respecting the 'number_pad'
			   option. */
			const char *sdp;
			if(iflags.num_pad)
				sdp = ndir;
			else
				sdp = sdir;
			android_push_char(sdp[index]);
			return;
		}

		/* Compute the desired position after the move. */
		destx += xdir[index];
		desty += ydir[index];
	}
	else if(c == kMoveDirCenter && !allowcontext)
	{
		/* In this case, we are not in the main movement loop, and got the
		   center direction - treat as a period character. */
		android_push_char('.');
		return;
	}

	/* For the remainder of the cases, we will use click_to_cmd(). This handles
	   opening/kicking doors, picking up stuff, moving up/down, etc. */
	if(c != kMoveDirNone)
	{
		/* click_to_cmd() does a better job with context-sensitive stuff
		   when the 'travel' option is on - let's pretend it always is,
		   in this context. */
		boolean oldtravelcmd = iflags.travelcmd;

		const char *cmd = click_to_cmd(destx, desty, CLICK_1);

		/* Restore the 'travel' option. */
		iflags.travelcmd = oldtravelcmd;

		/* Add the characters for the command (probably always just 1, in this
		   particular case. */
		while(*cmd)
		{
			android_push_char(*cmd++);
		}
	}
}


static int android_process_input_event_maptap(int *clickxout, int *clickyout)
{
	int x = android_msgq_pop_byte();
	int y = android_msgq_pop_byte();

	if(clickxout && clickyout)
	{
		*clickxout = x;
		*clickyout = y;

		return 1;
	}
	else
	{
		return 0;
	}
}


void android_process_input(int *chout, int *clickxout, int *clickyout)
{
	if(chout)
	{
		*chout = -1;
	}
	if(clickxout)
	{
		*clickxout = -1;
	}
	if(clickyout)
	{
		*clickyout = -1;
	}

	while(1)
	{
		pthread_mutex_lock(&s_ReceiveMutex);

		/* Request recentering on the player if the dungeon level changed. */
		if(s_PrevDungeonLevel.dnum != u.uz.dnum || s_PrevDungeonLevel.dlevel != u.uz.dlevel)
		{
			s_PrevDungeonLevel.dnum = u.uz.dnum;
			s_PrevDungeonLevel.dlevel = u.uz.dlevel;
			s_PlayerPosShouldRecenter = 1;
		}

		s_PlayerPosX = u.ux;
		s_PlayerPosY = u.uy;

		/* Not sure */
		if(s_Command != kCmdNone)	/* && !g_android_prevent_output)*/
		{
			int cmd = s_Command;
			s_Command = kCmdNone;

			android_msgq_wake_waiting();

			pthread_mutex_unlock(&s_ReceiveMutex);

			int shouldterminate = 0;

			if(cmd == kCmdSave)
			{
#if 0
				if(dosave0())
				{
					shouldterminate = 1;
				}
#endif

				if(s_ReadyForSave)
				{
					android_autosave_save();
				}
			}
			else if(cmd == kCmdRefresh)
			{
				s_ShouldRefresh = 1;
			}
			else if(cmd == kCmdSwitchToAmiga ||
					cmd == kCmdSwitchToIBM ||
					cmd == kCmdSwitchTo128)
			{
				s_SwitchCharSetCmd = cmd;
			}
			else if(cmd == kCmdTilesEnable || cmd == kCmdTilesDisable)
			{
#ifdef ANDROID_GRAPHICS_TILED
				g_AndroidTiled = (cmd == kCmdTilesEnable);
#endif

				/* Not sure about this, we really could use a better way to
				   force a redraw. */
				if(s_CharReceiveCnt < RECEIVEBUFFSZ)
				{
					s_CharReceiveBuff[s_CharReceiveCnt++] = 18;	/* ^R */
				}

			}

			if(s_WaitingForCommandPerformed)
			{
				s_WaitingForCommandPerformed = 0;
				sem_post(&s_CommandPerformedSema);
			}

			if(shouldterminate)
			{
				exit_nhwindows("Be seeing you...");
				terminate(EXIT_SUCCESS);
			}

			continue;
		}

		if(s_SwitchCharSetCmd >= 0)
		{
			if(android_getgamestate() == kAndroidGameStateMoveLoop)
			{
				int cmd = s_SwitchCharSetCmd;
				s_SwitchCharSetCmd = -1;

				pthread_mutex_unlock(&s_ReceiveMutex);

				switch(cmd)
				{
					case kCmdSwitchToAmiga:
						android_setcharset(kCharSetAmiga);
						break;
					case kCmdSwitchToIBM:
						android_setcharset(kCharSetIBM);
						break;
					case kCmdSwitchTo128:
						android_setcharset(kCharSet128);
						break;
					default:
						break;
				}

				if(s_CharReceiveCnt < RECEIVEBUFFSZ)
				{
					s_CharReceiveBuff[s_CharReceiveCnt++] = 18;	/* ^R */
				}

				continue;
			}
		}

		if(s_ShouldRefresh)
		{
			if(android_getgamestate() == kAndroidGameStateMoveLoop)
			{
				s_ShouldRefresh = 0;

				pthread_mutex_unlock(&s_ReceiveMutex);

				if(!g_AndroidPureTTY)
				{
					android_clear_winstatus_state();
				}

				bot();

				curs_on_u();

				continue;
			}
		}

		int processedany = 0;
		int done = 0;

		while(!android_msgq_is_empty())
		{
			unsigned char msgtype = android_msgq_pop_byte();

			switch(msgtype)
			{
				case kInputEventKeys:
					android_process_input_event_keys();
					break;
				case kInputEventMapTap:
					done = android_process_input_event_maptap(clickxout, clickyout);
					break;
				case kInputEventDir:
					android_process_input_event_dir();
					break;
				default:
					exit(1);	/* Probably too aggressive for released builds. */
					break;
			}

			processedany = 1;
		}

		if(processedany)
		{
			android_msgq_wake_waiting();
		}

		if(done)
		{
			pthread_mutex_unlock(&s_ReceiveMutex);
			return;
		}

		if(s_CharReceiveCnt > 0 && chout)
		{
			int ret = s_CharReceiveBuff[0], i;
			/* Hmm, no good! */
			for(i = 0; i < s_CharReceiveCnt - 1; i++)
			{
				s_CharReceiveBuff[i] = s_CharReceiveBuff[i + 1];
			}
			s_CharReceiveCnt--;

			pthread_mutex_unlock(&s_ReceiveMutex);
			*chout = ret;
			return;
		}

		s_ReceiveWaitingForData = 1;

		pthread_mutex_unlock(&s_ReceiveMutex);

		sem_wait(&s_ReceiveWaitingForDataSema);
	}
}


int android_getch(void)
{
	int ch;
	android_process_input(&ch, NULL, NULL);
	return ch;
}



static void sSendCmd(int cmd, int sync)
{
	pthread_mutex_lock(&s_ReceiveMutex);

	while(1)
	{
		if(s_Command == kCmdNone)
		{
			s_Command = cmd;

			if(sync)
			{
				s_WaitingForCommandPerformed = 1;
			}

			break;
		}
		else
		{
			s_ReceiveWaitingForConsumption = 1;

			pthread_mutex_unlock(&s_ReceiveMutex);

			sem_wait(&s_ReceiveWaitingForConsumptionSema);

			pthread_mutex_lock(&s_ReceiveMutex);
		}
	}

	if(s_ReceiveWaitingForData)
	{
		s_ReceiveWaitingForData = 0;
		sem_post(&s_ReceiveWaitingForDataSema);
	}

	pthread_mutex_unlock(&s_ReceiveMutex);

	if(sync)
	{
		sem_wait(&s_CommandPerformedSema);
	}
}


pthread_t g_ThreadHandle;
char *g_NetHackDir = NULL;	// "/data/data/com.nethackff/nethackdir"

void nethack_exit(int result)
{
	android_autosave_remove();

	s_Quit = 1;
	while(1)
	{
		sleep(60);
	}
}


static void *sThreadFunc()
{
	int fd;

	int argc = 1;
	char *argv[] = {	"nethack"  	};

	int i = 0;

	char buff[256];

	android_switchgamestate(kAndroidGameStateInit);

	if(g_NetHackDir)
	{
		chdir(g_NetHackDir);
	}

	if(g_AndroidPureTTY)
	{
		choose_windows("tty");
	}
	else
	{
		choose_windows("android");
	}

	if(!g_AndroidPureTTY)
	{
		/* As far as the Java side is concerned, we actually pretend to
		   use the menu window for the startup screen. This is done to get
		   word wrapping if needed.
		*/
		android_puts("\033A4\033AS");
	}

	initoptions();

	init_nhwindows(&argc,argv);

	android_autosave_restore();

	if(!*plname || !strncmp(plname, "player", 4)
		    || !strncmp(plname, "games", 4)) {
		askname();
	}

	int charset = s_InitCharSet;
	s_InitCharSet = -1;
	android_setcharset(charset);

	android_makelock();

	getlock();

	/* When asked if an old game with the same name should be destroyed,
	   I had some issue where the cursor didn't reset to the beginning of the
	   line. Simply printing an extra line break here probably takes care of
	   that issue. */
	printf("\n");

	dlb_init();	/* must be before newgame() */

	/*
	 * Initialization of the boundaries of the mazes
	 * Both boundaries have to be even.
	 */
	x_maze_max = COLNO-1;
	if (x_maze_max % 2)
		x_maze_max--;
	y_maze_max = ROWNO-1;
	if (y_maze_max % 2)
		y_maze_max--;

	/*
	 *  Initialize the vision system.  This must be before mklev() on a
	 *  new game or before a level restore on a saved game.
	 */
	vision_init();

	display_gamewindows();

	if ((fd = restore_saved_game()) >= 0) {

#ifdef WIZARD
		/* Since wizard is actually flags.debug, restoring might
		 * overwrite it.
		 */
		boolean remember_wiz_mode = wizard;
#endif
		const char *fq_save = fqname(SAVEF, SAVEPREFIX, 1);

		(void) chmod(fq_save,0);	/* disallow parallel restores */
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#ifdef NEWS
		if(iflags.news) {
		    display_file(NEWS, FALSE);
		    iflags.news = FALSE; /* in case dorecover() fails */
		}
#endif
		pline("Restoring save file...");
		mark_synch();	/* flush output */
		if(!dorecover(fd))
			goto not_recovered;
#ifdef WIZARD
		if(!wizard && remember_wiz_mode) wizard = TRUE;
#endif

	if(!g_AndroidPureTTY)
	{
		/* Since we pretend to use a menu window, we need to tell the Java
		   side of the UI to go back to the main view now, and close the
		   menu window. */
		android_puts("\033A4\033AH");
		android_puts("\033A0");
	}


		check_special_room(FALSE);
		wd_message();

		if (discover || wizard) {
			if(yn("Do you want to keep the save file?") == 'n')
			    (void) delete_savefile();
			else {
			    (void) chmod(fq_save,FCMASK); /* back to readable */
			    compress(fq_save);
			}
		}
		flags.move = 0;
	} else {
not_recovered:
		player_selection();

	if(!g_AndroidPureTTY)
	{
		/* Since we pretend to use a menu window, we need to tell the Java
		   side of the UI to go back to the main view now, and close the
		   menu window. */
		android_puts("\033A4\033AH");
		android_puts("\033A0");
	}


		newgame();
		wd_message();

		flags.move = 0;
		set_wear();
		(void) pickup(1);
	}

	s_ReadyForSave = 1;
	android_switchgamestate(kAndroidGameStateMoveLoop);

	pthread_mutex_lock(&s_ReceiveMutex);
	s_PlayerPosShouldRecenter = 1;
	s_PrevDungeonLevel.dnum = u.uz.dnum;
	s_PrevDungeonLevel.dlevel = u.uz.dlevel;
	pthread_mutex_unlock(&s_ReceiveMutex);

	moveloop();

	nethack_exit(EXIT_SUCCESS);

	return(0);
}

static const int kUiModeAndroidTTY = 0;
static const int kUiModePureTTY = 1;
#ifdef ANDROID_GRAPHICS_TILED
static const int kUiModeAndroidTiled = 2;
#endif

int Java_com_nethackff_NetHackJNI_NetHackInit(JNIEnv *env, jobject thiz,
		int uimode, jstring nethackdir)
{
	char *p;
	int x, y;
	const char *nethackdirnative;

	if(g_ThreadHandle)
	{
		return 0;
	}

	nethackdirnative = (*env)->GetStringUTFChars(env, nethackdir, 0);
	if(g_NetHackDir)
	{
		/* This shouldn't happen, but probably best to deal with it in case. */
		free((void*)g_NetHackDir);
		g_NetHackDir = NULL;
	}
	g_NetHackDir = malloc(strlen(nethackdirnative + 1));
	strcpy(g_NetHackDir, nethackdirnative);
	(*env)->ReleaseStringUTFChars(env, nethackdir, nethackdirnative);

	g_AndroidPureTTY = (uimode == kUiModePureTTY);
#ifdef ANDROID_GRAPHICS_TILED
	g_AndroidTiled = (uimode == kUiModeAndroidTiled);
	g_AndroidTilesEnabledForUser = g_AndroidTiled;
#endif
	s_SendWaitingForNotFull = 0;
	s_ReceiveWaitingForData = 0;
	s_ReceiveWaitingForConsumption = 0;
	s_CharReceiveCnt = 0;

	android_msgq_init();

	s_SendCnt = 0;
	s_Quit = 0;	
	s_ReadyForSave = 0;
	s_WaitingForCommandPerformed = 0;

	if(pthread_mutex_init(&s_SendMutex, 0) != 0)
	{
		return 0;
	}

	if(pthread_mutex_init(&s_ReceiveMutex, 0) != 0)
	{
		pthread_mutex_destroy(&s_SendMutex);
		return 0;
	}

	if(sem_init(&s_ReceiveWaitingForDataSema, 0, 0) != 0)
	{
		pthread_mutex_destroy(&s_ReceiveMutex);
		pthread_mutex_destroy(&s_SendMutex);
		return 0;
	}

	if(sem_init(&s_ReceiveWaitingForConsumptionSema, 0, 0) != 0)
	{
		pthread_mutex_destroy(&s_ReceiveMutex);
		pthread_mutex_destroy(&s_SendMutex);
		return 0;
	}

	if(sem_init(&s_SendNotFullSema, 0, 0) != 0)
	{
		pthread_mutex_destroy(&s_ReceiveMutex);
		pthread_mutex_destroy(&s_SendMutex);
		sem_destroy(&s_ReceiveWaitingForConsumptionSema);
		sem_destroy(&s_ReceiveWaitingForDataSema);

		return 0;
	}

	if(sem_init(&s_CommandPerformedSema, 0, 0) != 0)
	{
		pthread_mutex_destroy(&s_ReceiveMutex);
		pthread_mutex_destroy(&s_SendMutex);
		sem_destroy(&s_ReceiveWaitingForConsumptionSema);
		sem_destroy(&s_ReceiveWaitingForDataSema);
		sem_destroy(&s_SendNotFullSema);

		return 0;
	}

	if(pthread_create(&g_ThreadHandle, NULL, sThreadFunc, NULL) != 0)
	{
		pthread_mutex_destroy(&s_ReceiveMutex);
		pthread_mutex_destroy(&s_SendMutex);
		sem_destroy(&s_ReceiveWaitingForConsumptionSema);
		sem_destroy(&s_ReceiveWaitingForDataSema);
		sem_destroy(&s_SendNotFullSema);
		sem_destroy(&s_CommandPerformedSema);

		g_ThreadHandle = 0;
		return 0;	/* Failure. */
	}

	return 1;
}

void Java_com_nethackff_NetHackJNI_NetHackShutdown(JNIEnv *env, jobject thiz)
{
	if(g_ThreadHandle)
	{
#if 0
		pthread_cancel(g_ThreadHandle);	/* Would return 0 on success. */
#endif

		pthread_mutex_destroy(&s_SendMutex);
		pthread_mutex_destroy(&s_ReceiveMutex);
		sem_destroy(&s_ReceiveWaitingForConsumptionSema);
		sem_destroy(&s_ReceiveWaitingForDataSema);
		sem_destroy(&s_SendNotFullSema);
		sem_destroy(&s_CommandPerformedSema);
		g_ThreadHandle = 0;
	}

	if(g_NetHackDir)
	{
		free((void*)g_NetHackDir);
		g_NetHackDir = NULL;
	}
}


int Java_com_nethackff_NetHackJNI_NetHackHasQuit(JNIEnv *env, jobject thiz)
{
	return s_Quit;
}

extern int dosave0();

int Java_com_nethackff_NetHackJNI_NetHackSave(JNIEnv *env, jobject thiz)
{
	if(!s_ReadyForSave)
	{
		return 0;
	}

	sSendCmd(kCmdSave, 1);

	return 1;
}


void Java_com_nethackff_NetHackJNI_NetHackRefreshDisplay(
		JNIEnv *env, jobject thiz)
{
	/* Do we need to do anything to check if we are in a state where
	   this is appropriate? */
#if 0
	doredraw();
    flags.botlx = 1;	/* force a redraw of the bottom line */
	g_android_refresh = 1;
#endif

	sSendCmd(kCmdRefresh, 0);
}


void Java_com_nethackff_NetHackJNI_NetHackSwitchCharSet(
		JNIEnv *env, jobject thiz, int charset)
{
	int cmd = -1;

	if(s_InitCharSet >= 0)
	{
		s_InitCharSet = charset;
		return;
	}

	switch(charset)
	{
		case kCharSet128:
			cmd = kCmdSwitchTo128;
			break;
		case kCharSetIBM:
			cmd = kCmdSwitchToIBM;
			break;
		case kCharSetAmiga:
			cmd = kCmdSwitchToAmiga;
			break;
		default:
			break;
	}

	if(cmd < 0)
	{
		return;
	}

	sSendCmd(cmd, 0);
}


void Java_com_nethackff_NetHackJNI_NetHackSetTilesEnabled(
		JNIEnv *env, jobject thiz, int tilesenabled)
{
	int sync = 1;	/* Maybe less risk for timing-related display bugs this way? */
	if(tilesenabled)
	{
		sSendCmd(kCmdTilesEnable, sync);
	}
	else
	{
		sSendCmd(kCmdTilesDisable, sync);
	}
}

int Java_com_nethackff_NetHackJNI_NetHackGetPlayerPosX(JNIEnv *env,
		jobject thiz)
{
	int ret;

	pthread_mutex_lock(&s_ReceiveMutex);

	ret = s_PlayerPosX;

	pthread_mutex_unlock(&s_ReceiveMutex);

	return ret;
}


int Java_com_nethackff_NetHackJNI_NetHackGetPlayerPosY(JNIEnv *env,
		jobject thiz)
{
	int ret;

	pthread_mutex_lock(&s_ReceiveMutex);

	ret = s_PlayerPosY;

	pthread_mutex_unlock(&s_ReceiveMutex);

	return ret;
}


int Java_com_nethackff_NetHackJNI_NetHackGetPlayerPosShouldRecenter(JNIEnv *env,
		jobject thiz)
{
	int ret;

	pthread_mutex_lock(&s_ReceiveMutex);

	ret = s_PlayerPosShouldRecenter;

	/* Not sure: */
	s_PlayerPosShouldRecenter = 0;

	pthread_mutex_unlock(&s_ReceiveMutex);

	return ret;
}


void Java_com_nethackff_NetHackJNI_NetHackSendDir(JNIEnv *env, jobject thiz,
		int dir, int allowcontext)
{
	android_msgq_begin_message();
	android_msgq_push_byte((unsigned char)kInputEventDir);

	unsigned char c = (unsigned char)dir;
	if(allowcontext)
	{
		c |= kMoveDirAllowContextSensitive;
	}
	android_msgq_push_byte(c);

	android_msgq_end_message();
}


void Java_com_nethackff_NetHackJNI_NetHackTerminalSend(JNIEnv *env, jobject thiz,
		jstring str)
{
	const char *nativestr = (*env)->GetStringUTFChars(env, str, 0);

	android_msgq_begin_message();

	android_msgq_push_byte((unsigned char)kInputEventKeys);

	const char *ptr = nativestr;
	for(; *ptr;)
	{
		unsigned int c = *ptr++;

		/* For the meta keys to work, we need to convert the UTF
		   encoding back to 8 bit chars, which we do here. */
		if((c & 0xe0) == 0xc0)
		{
			unsigned int d = *ptr++;
			c = ((c << 6) & 0xc0) | (d & 0x3f);
		}

		android_msgq_push_byte((unsigned char)c);
	}

	android_msgq_end_message();

	(*env)->ReleaseStringUTFChars(env, str, nativestr);
}


void Java_com_nethackff_NetHackJNI_NetHackMapTap(JNIEnv *env,
		jobject thiz, int x, int y)
{
	android_msgq_begin_message();
	android_msgq_push_byte((char)kInputEventMapTap);
	android_msgq_push_byte((unsigned char)x);
	android_msgq_push_byte((unsigned char)y);
	android_msgq_end_message();
}


jstring Java_com_nethackff_NetHackJNI_NetHackTerminalReceive(JNIEnv *env,
		jobject thiz)
{
	pthread_mutex_lock(&s_SendMutex);

	s_SendBuff[s_SendCnt] = '\0';
	jstring str = (*env)->NewStringUTF(env, s_SendBuff);
	s_SendCnt = 0;

	if(s_SendWaitingForNotFull)
	{
		s_SendWaitingForNotFull = 0;
		sem_post(&s_SendNotFullSema);
	}

	pthread_mutex_unlock(&s_SendMutex);

	return str;
}

static void
wd_message()
{
#ifdef WIZARD
	if (wiz_error_flag) {
		pline("Only user \"%s\" may access debug (wizard) mode.",
# ifndef KR1ED
			WIZARD);
# else
			WIZARD_NAME);
# endif
		pline("Entering discovery mode instead.");
	} else
#endif
	if (discover)
		You("are in non-scoring discovery mode.");
}

/* End of file androidmain.c */
