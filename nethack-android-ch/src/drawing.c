/* NetHack 3.6	drawing.c	$NHDT-Date: 1447124657 2015/11/10 03:04:17 $  $NHDT-Branch: master $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) NetHack Development Team 1992.                   */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h"

/* Relevant header information in rm.h, objclass.h, and monsym.h. */

#ifdef C
#undef C
#endif

#ifdef TEXTCOLOR
#define C(n) n
#else
#define C(n)
#endif

struct symsetentry symset[NUM_GRAPHICS];

int currentgraphics = 0;

nhsym showsyms[SYM_MAX] = DUMMY; /* symbols to be displayed */
nhsym l_syms[SYM_MAX] = DUMMY;   /* loaded symbols          */
nhsym r_syms[SYM_MAX] = DUMMY;   /* rogue symbols           */

nhsym warnsyms[WARNCOUNT] = DUMMY; /* the current warning display symbols */
const char invisexplain[] = "记住的, 未看见的, 生物";

/* Default object class symbols.  See objclass.h.
 * {symbol, name, explain}
 *     name:    used in object_detect().
 *     explain: used in do_look().
 */
const struct class_sym def_oc_syms[MAXOCLASSES] = {
    { '\0', "", "" }, /* placeholder for the "random class" */
    { ILLOBJ_SYM, "illegal objects", "strange object" },
    { WEAPON_SYM, "武器", "武器" },
    { ARMOR_SYM, "防具", "一套或一件防具" },
    { RING_SYM, "戒指", "戒指" },
    { AMULET_SYM, "护身符", "护身符" },
    { TOOL_SYM, "工具", "有用的东西( 鹤嘴锄, 钥匙, 灯...)" },
    { FOOD_SYM, "食物", "一块食物" },
    { POTION_SYM, "药水", "药水" },
    { SCROLL_SYM, "卷轴", "卷轴" },
    { SPBOOK_SYM, "魔法书", "魔法书" },
    { WAND_SYM, "魔杖", "魔杖" },
    { GOLD_SYM, "金币", "一堆金币" },
    { GEM_SYM, "石头", "宝石或石头" },
    { ROCK_SYM, "大石头", "巨石或雕像" },
    { BALL_SYM, "铁球", "铁球" },
    { CHAIN_SYM, "铁链", "铁链" },
    { VENOM_SYM, "毒液", "飞溅的毒液" }
};

/* Default monster class symbols.  See monsym.h. */
const struct class_sym def_monsyms[MAXMCLASSES] = {
    { '\0', "", "" },
    { DEF_ANT, "", "蚂蚁或其他昆虫" },
    { DEF_BLOB, "", "液滴" },
    { DEF_COCKATRICE, "", "鸡蛇" },
    { DEF_DOG, "", "狗或其他犬科动物" },
    { DEF_EYE, "", "眼或球" },
    { DEF_FELINE, "", "猫或其他猫科动物" },
    { DEF_GREMLIN, "", "小鬼" },
    { DEF_HUMANOID, "", "类人动物" },
    { DEF_IMP, "", "小恶魔或小魔鬼" },
    { DEF_JELLY, "", "果冻" },
    { DEF_KOBOLD, "", "狗头人" },
    { DEF_LEPRECHAUN, "", "小矮妖" },
    { DEF_MIMIC, "", "拟型怪" },
    { DEF_NYMPH, "", "仙女" },
    { DEF_ORC, "", "兽人" },
    { DEF_PIERCER, "", "锥子" },
    { DEF_QUADRUPED, "", "四足动物" },
    { DEF_RODENT, "", "啮齿动物" },
    { DEF_SPIDER, "", "蛛形纲动物或蜈蚣" },
    { DEF_TRAPPER, "", "捕兽者或潜伏者" },
    { DEF_UNICORN, "", "独角兽或马" },
    { DEF_VORTEX, "", "漩涡" },
    { DEF_WORM, "", "蠕虫" },
    { DEF_XAN, "", "玄蚊或其他神话中的/ 虚构的 昆虫" },
    { DEF_LIGHT, "", "光" },
    { DEF_ZRUTY, "", "山区巨人" },
    { DEF_ANGEL, "", "天使般的人" },
    { DEF_BAT, "", "蝙蝠或鸟类" },
    { DEF_CENTAUR, "", "半人马" },
    { DEF_DRAGON, "", "龙" },
    { DEF_ELEMENTAL, "", "元素" },
    { DEF_FUNGUS, "", "真菌或霉菌" },
    { DEF_GNOME, "", "侏儒" },
    { DEF_GIANT, "", "巨型类人动物" },
    { '\0', "", "隐形的怪物" },
    { DEF_JABBERWOCK, "", "颊脖龙" },
    { DEF_KOP, "", "吉斯通警察" },
    { DEF_LICH, "", "巫妖" },
    { DEF_MUMMY, "", "木乃伊" },
    { DEF_NAGA, "", "纳迦" },
    { DEF_OGRE, "", "食人魔" },
    { DEF_PUDDING, "", "布丁或软泥怪" },
    { DEF_QUANTMECH, "", "量子力学" },
    { DEF_RUSTMONST, "", "锈怪或解魔怪" },
    { DEF_SNAKE, "", "蛇" },
    { DEF_TROLL, "", "巨魔" },
    { DEF_UMBER, "", "土巨怪" },
    { DEF_VAMPIRE, "", "吸血鬼" },
    { DEF_WRAITH, "", "幽灵" },
    { DEF_XORN, "", "索尔石怪" },
    { DEF_YETI, "", "类人猿生物" },
    { DEF_ZOMBIE, "", "僵尸" },
    { DEF_HUMAN, "", "人类或精灵" },
    { DEF_GHOST, "", "鬼魂" },
    { DEF_GOLEM, "", "魔像" },
    { DEF_DEMON, "", "大魔鬼" },
    { DEF_EEL, "", "海怪" },
    { DEF_LIZARD, "", "蜥蜴" },
    { DEF_WORM_TAIL, "", "长蠕虫尾" },
    { DEF_MIMIC_DEF, "", "拟型怪" },
};

const struct symdef def_warnsyms[WARNCOUNT] = {
    /* white warning  */
    { '0', "未知生物使你担忧",    C(CLR_WHITE) },
    /* pink warning   */
    { '1', "未知生物使你担心",  C(CLR_RED) },
    /* red warning    */
    { '2', "未知生物使你焦虑",  C(CLR_RED) },
    /* ruby warning   */
    { '3', "未知生物使你忧虑", C(CLR_RED) },
    /* purple warning */
    { '4', "未知生物使你惊恐",    C(CLR_MAGENTA) },
    /* black warning  */
    { '5', "未知生物使你恐惧",    C(CLR_BRIGHT_MAGENTA) },
};

/*
 *  Default screen symbols with explanations and colors.
 */
const struct symdef defsyms[MAXPCHARS] = {
/* 0*/ { ' ', "房间的黑暗部分", C(NO_COLOR) },  /* stone */
       { '|', "墙壁", C(CLR_GRAY) },                 /* vwall */
       { '-', "墙壁", C(CLR_GRAY) },                 /* hwall */
       { '-', "墙壁", C(CLR_GRAY) },                 /* tlcorn */
       { '-', "墙壁", C(CLR_GRAY) },                 /* trcorn */
       { '-', "墙壁", C(CLR_GRAY) },                 /* blcorn */
       { '-', "墙壁", C(CLR_GRAY) },                 /* brcorn */
       { '-', "墙壁", C(CLR_GRAY) },                 /* crwall */
       { '-', "墙壁", C(CLR_GRAY) },                 /* tuwall */
       { '-', "墙壁", C(CLR_GRAY) },                 /* tdwall */
/*10*/ { '|', "墙壁", C(CLR_GRAY) },                 /* tlwall */
       { '|', "墙壁", C(CLR_GRAY) },                 /* trwall */
       { '.', "门口", C(CLR_GRAY) },              /* ndoor */
       { '-', "打开的门", C(CLR_BROWN) },           /* vodoor */
       { '|', "打开的门", C(CLR_BROWN) },           /* hodoor */
       { '+', "关上的门", C(CLR_BROWN) },         /* vcdoor */
       { '+', "关上的门", C(CLR_BROWN) },         /* hcdoor */
       { '#', "铁栏杆", C(HI_METAL) },            /* bars */
       { '#', "树", C(CLR_GREEN) },                /* tree */
       { '.', "房间的地板", C(CLR_GRAY) },      /* room */
/*20*/ { '.', "房间的黑暗部分", C(CLR_BLACK) }, /* dark room */
       { '#', "走廊", C(CLR_GRAY) },             /* dark corr */
       { '#', "照亮了的走廊", C(CLR_GRAY) },   /* lit corr (see mapglyph.c) */
       { '<', "通往上面的楼梯", C(CLR_GRAY) },         /* upstair */
       { '>', "通往下面的楼梯", C(CLR_GRAY) },       /* dnstair */
       { '<', "通往上面的梯子", C(CLR_BROWN) },           /* upladder */
       { '>', "通往下面的梯子", C(CLR_BROWN) },         /* dnladder */
       { '_', "祭坛", C(CLR_GRAY) },                /* altar */
       { '|', "坟墓", C(CLR_GRAY) },                /* grave */
       { '\\', "华丽的王座", C(HI_GOLD) },       /* throne */
/*30*/ { '#', "水槽", C(CLR_GRAY) },                 /* sink */
       { '{', "喷泉", C(CLR_BLUE) },             /* fountain */
       { '}', "水", C(CLR_BLUE) },                /* pool */
       { '.', "冰", C(CLR_CYAN) },                  /* ice */
       { '}', "熔岩", C(CLR_RED) },           /* lava */
       { '.', "放下的吊桥", C(CLR_BROWN) },  /* vodbridge */
       { '.', "放下的吊桥", C(CLR_BROWN) },  /* hodbridge */
       { '#', "升起的吊桥", C(CLR_BROWN) },   /* vcdbridge */
       { '#', "升起的吊桥", C(CLR_BROWN) },   /* hcdbridge */
       { ' ', "天空", C(CLR_CYAN) },                  /* open air */
/*40*/ { '#', "云", C(CLR_GRAY) },                /* [part of] a cloud */
       { '}', "水", C(CLR_BLUE) },                /* under water */
       { '^', "箭陷阱", C(HI_METAL) },           /* trap */
       { '^', "飞镖陷阱", C(HI_METAL) },            /* trap */
       { '^', "落石陷阱", C(CLR_GRAY) },    /* trap */
       { '^', "尖板", C(CLR_BROWN) },       /* trap */
       { '^', "捕兽夹", C(HI_METAL) },            /* trap */
       { '^', "地雷", C(CLR_RED) },             /* trap */
       { '^', "滚动巨石陷阱", C(CLR_GRAY) }, /* trap */
       { '^', "催眠气体陷阱", C(HI_ZAP) },      /* trap */
/*50*/ { '^', "生锈陷阱", C(CLR_BLUE) },            /* trap */
       { '^', "火焰陷阱", C(CLR_ORANGE) },          /* trap */
       { '^', "坑", C(CLR_BLACK) },                 /* trap */
       { '^', "尖刺坑", C(CLR_BLACK) },          /* trap */
       { '^', "洞", C(CLR_BROWN) },                /* trap */
       { '^', "陷阱门", C(CLR_BROWN) },           /* trap */
       { '^', "传送陷阱", C(CLR_MAGENTA) },  /* trap */
       { '^', "地层传送", C(CLR_MAGENTA) },    /* trap */
       { '^', "魔法门", C(CLR_BRIGHT_MAGENTA) }, /* trap */
       { '"', "网", C(CLR_GRAY) },                    /* web */
/*60*/ { '^', "雕像陷阱", C(CLR_GRAY) },            /* trap */
       { '^', "魔法陷阱", C(HI_ZAP) },               /* trap */
       { '^', "反魔法区域", C(HI_ZAP) },         /* trap */
       { '^', "变形陷阱", C(CLR_BRIGHT_GREEN) }, /* trap */
       { '^', "振动方块", C(CLR_YELLOW) },     /* trap */
       { '|', "墙壁", C(CLR_GRAY) },            /* vbeam */
       { '-', "墙壁", C(CLR_GRAY) },            /* hbeam */
       { '\\', "墙壁", C(CLR_GRAY) },           /* lslant */
       { '/', "墙壁", C(CLR_GRAY) },            /* rslant */
       { '*', "", C(CLR_WHITE) },               /* dig beam */
       { '!', "", C(CLR_WHITE) },               /* camera flash beam */
       { ')', "", C(HI_WOOD) },                 /* boomerang open left */
/*70*/ { '(', "", C(HI_WOOD) },                 /* boomerang open right */
       { '0', "", C(HI_ZAP) },                  /* 4 magic shield symbols */
       { '#', "", C(HI_ZAP) },
       { '@', "", C(HI_ZAP) },
       { '*', "", C(HI_ZAP) },
       { '#', "毒云", C(CLR_BRIGHT_GREEN) },   /* part of a cloud */
       { '?', "有效的位置", C(CLR_BRIGHT_GREEN) }, /*  target position */
       { '/', "", C(CLR_GREEN) },         /* swallow top left      */
       { '-', "", C(CLR_GREEN) },         /* swallow top center    */
       { '\\', "", C(CLR_GREEN) },        /* swallow top right     */
/*80*/ { '|', "", C(CLR_GREEN) },         /* swallow middle left   */
       { '|', "", C(CLR_GREEN) },         /* swallow middle right  */
       { '\\', "", C(CLR_GREEN) },        /* swallow bottom left   */
       { '-', "", C(CLR_GREEN) },         /* swallow bottom center */
       { '/', "", C(CLR_GREEN) },         /* swallow bottom right  */
       { '/', "", C(CLR_ORANGE) },        /* explosion top left     */
       { '-', "", C(CLR_ORANGE) },        /* explosion top center   */
       { '\\', "", C(CLR_ORANGE) },       /* explosion top right    */
       { '|', "", C(CLR_ORANGE) },        /* explosion middle left  */
       { ' ', "", C(CLR_ORANGE) },        /* explosion middle center*/
/*90*/ { '|', "", C(CLR_ORANGE) },        /* explosion middle right */
       { '\\', "", C(CLR_ORANGE) },       /* explosion bottom left  */
       { '-', "", C(CLR_ORANGE) },        /* explosion bottom center*/
       { '/', "", C(CLR_ORANGE) },        /* explosion bottom right */
};

/* default rogue level symbols */
static const uchar def_r_oc_syms[MAXOCLASSES] = {
/* 0*/ '\0', ILLOBJ_SYM, WEAPON_SYM, ']', /* armor */
       RING_SYM,
/* 5*/ ',',                     /* amulet */
       TOOL_SYM, ':',           /* food */
       POTION_SYM, SCROLL_SYM,
/*10*/ SPBOOK_SYM, WAND_SYM,
       GEM_SYM,                /* gold -- yes it's the same as gems */
       GEM_SYM, ROCK_SYM,
/*15*/ BALL_SYM, CHAIN_SYM, VENOM_SYM
};

#undef C

#ifdef TERMLIB
void NDECL((*decgraphics_mode_callback)) = 0; /* set in tty_start_screen() */
#endif /* TERMLIB */

#ifdef PC9800
void NDECL((*ibmgraphics_mode_callback)) = 0; /* set in tty_start_screen() */
void NDECL((*ascgraphics_mode_callback)) = 0; /* set in tty_start_screen() */
#endif

/*
 * Convert the given character to an object class.  If the character is not
 * recognized, then MAXOCLASSES is returned.  Used in detect.c, invent.c,
 * objnam.c, options.c, pickup.c, sp_lev.c, lev_main.c, and tilemap.c.
 */
int
def_char_to_objclass(ch)
char ch;
{
    int i;
    for (i = 1; i < MAXOCLASSES; i++)
        if (ch == def_oc_syms[i].sym)
            break;
    return i;
}

/*
 * Convert a character into a monster class.  This returns the _first_
 * match made.  If there are are no matches, return MAXMCLASSES.
 * Used in detect.c, options.c, read.c, sp_lev.c, and lev_main.c
 */
int
def_char_to_monclass(ch)
char ch;
{
    int i;
    for (i = 1; i < MAXMCLASSES; i++)
        if (ch == def_monsyms[i].sym)
            break;
    return i;
}

/*
 * Explanations of the functions found below:
 *
 * init_symbols()
 *                     Sets the current display symbols, the
 *                     loadable symbols to the default NetHack
 *                     symbols, including the r_syms rogue level
 *                     symbols. This would typically be done
 *                     immediately after execution begins. Any
 *                     previously loaded external symbol sets are
 *                     discarded.
 *
 * switch_symbols(arg)
 *                     Called to swap in new current display symbols
 *                     (showsyms) from either the default symbols,
 *                     or from the loaded symbols.
 *
 *                     If (arg == 0) then showsyms are taken from
 *                     defsyms, def_oc_syms, and def_monsyms.
 *
 *                     If (arg != 0), which is the normal expected
 *                     usage, then showsyms are taken from the
 *                     adjustable display symbols found in l_syms.
 *                     l_syms may have been loaded from an external
 *                     symbol file by config file options or interactively
 *                     in the Options menu.
 *
 * assign_graphics(arg)
 *
 *                     This is used in the game core to toggle in and
 *                     out of other {rogue} level display modes.
 *
 *                     If arg is ROGUESET, this places the rogue level
 *                     symbols from r_syms into showsyms.
 *
 *                     If arg is PRIMARY, this places the symbols
 *                     from l_monsyms into showsyms.
 *
 * update_l_symset()
 *                     Update a member of the loadable (l_*) symbol set.
 *
 * update_r_symset()
 *                     Update a member of the rogue (r_*) symbol set.
 *
 */

void
init_symbols()
{
    init_l_symbols();
    init_showsyms();
    init_r_symbols();
}

void
update_bouldersym()
{
    showsyms[SYM_BOULDER + SYM_OFF_X] = iflags.bouldersym;
    l_syms[SYM_BOULDER + SYM_OFF_X] = iflags.bouldersym;
    r_syms[SYM_BOULDER + SYM_OFF_X] = iflags.bouldersym;
}

void
init_showsyms()
{
    register int i;

    for (i = 0; i < MAXPCHARS; i++)
        showsyms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        showsyms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        showsyms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        showsyms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++) {
        if (i == SYM_BOULDER)
            showsyms[i + SYM_OFF_X] = iflags.bouldersym
                                        ? iflags.bouldersym
                                        : def_oc_syms[ROCK_CLASS].sym;
        else if (i == SYM_INVISIBLE)
            showsyms[i + SYM_OFF_X] = DEF_INVISIBLE;
    }
}

/* initialize defaults for the loadable symset */
void
init_l_symbols()
{
    register int i;

    for (i = 0; i < MAXPCHARS; i++)
        l_syms[i + SYM_OFF_P] = defsyms[i].sym;
    for (i = 0; i < MAXOCLASSES; i++)
        l_syms[i + SYM_OFF_O] = def_oc_syms[i].sym;
    for (i = 0; i < MAXMCLASSES; i++)
        l_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        l_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++) {
        if (i == SYM_BOULDER)
            l_syms[i + SYM_OFF_X] = iflags.bouldersym
                                      ? iflags.bouldersym
                                      : def_oc_syms[ROCK_CLASS].sym;
        else if (i == SYM_INVISIBLE)
            l_syms[i + SYM_OFF_X] = DEF_INVISIBLE;
    }

    clear_symsetentry(PRIMARY, FALSE);
}

void
init_r_symbols()
{
    register int i;

    /* These are defaults that can get overwritten
       later by the roguesymbols option */

    for (i = 0; i < MAXPCHARS; i++)
        r_syms[i + SYM_OFF_P] = defsyms[i].sym;
    r_syms[S_vodoor] = r_syms[S_hodoor] = r_syms[S_ndoor] = '+';
    r_syms[S_upstair] = r_syms[S_dnstair] = '%';

    for (i = 0; i < MAXOCLASSES; i++)
        r_syms[i + SYM_OFF_O] = def_r_oc_syms[i];
    for (i = 0; i < MAXMCLASSES; i++)
        r_syms[i + SYM_OFF_M] = def_monsyms[i].sym;
    for (i = 0; i < WARNCOUNT; i++)
        r_syms[i + SYM_OFF_W] = def_warnsyms[i].sym;
    for (i = 0; i < MAXOTHER; i++) {
        if (i == SYM_BOULDER)
            r_syms[i + SYM_OFF_X] = iflags.bouldersym
                                      ? iflags.bouldersym
                                      : def_oc_syms[ROCK_CLASS].sym;
        else if (i == SYM_INVISIBLE)
            r_syms[i + SYM_OFF_X] = DEF_INVISIBLE;
    }

    clear_symsetentry(ROGUESET, FALSE);
    /* default on Rogue level is no color
     * but some symbol sets can override that
     */
    symset[ROGUESET].nocolor = 1;
}

void
assign_graphics(whichset)
int whichset;
{
    register int i;

    switch (whichset) {
    case ROGUESET:
        /* Adjust graphics display characters on Rogue levels */

        for (i = 0; i < SYM_MAX; i++)
            showsyms[i] = r_syms[i];

#if defined(MSDOS) && defined(USE_TILES)
        if (iflags.grmode)
            tileview(FALSE);
#endif
        currentgraphics = ROGUESET;
        break;

    case PRIMARY:
    default:
        for (i = 0; i < SYM_MAX; i++)
            showsyms[i] = l_syms[i];

#if defined(MSDOS) && defined(USE_TILES)
        if (iflags.grmode)
            tileview(TRUE);
#endif
        currentgraphics = PRIMARY;
        break;
    }
}

void
switch_symbols(nondefault)
int nondefault;
{
    register int i;

    if (nondefault) {
        for (i = 0; i < SYM_MAX; i++)
            showsyms[i] = l_syms[i];
#ifdef PC9800
        if (SYMHANDLING(H_IBM) && ibmgraphics_mode_callback)
            (*ibmgraphics_mode_callback)();
        else if (!symset[currentgraphics].name && ascgraphics_mode_callback)
            (*ascgraphics_mode_callback)();
#endif
#ifdef TERMLIB
        if (SYMHANDLING(H_DEC) && decgraphics_mode_callback)
            (*decgraphics_mode_callback)();
#endif
    } else
        init_symbols();
}

void
update_l_symset(symp, val)
struct symparse *symp;
int val;
{
    l_syms[symp->idx] = val;
}

void
update_r_symset(symp, val)
struct symparse *symp;
int val;
{
    r_syms[symp->idx] = val;
}

void
clear_symsetentry(which_set, name_too)
int which_set;
boolean name_too;
{
    if (symset[which_set].desc)
        free((genericptr_t) symset[which_set].desc);
    symset[which_set].desc = (char *) 0;

    symset[which_set].handling = H_UNK;
    symset[which_set].nocolor = 0;
    /* initialize restriction bits */
    symset[which_set].primary = 0;
    symset[which_set].rogue = 0;

    if (name_too) {
        if (symset[which_set].name)
            free((genericptr_t) symset[which_set].name);
        symset[which_set].name = (char *) 0;
    }
}

/*
 * If you are adding code somewhere to be able to recognize
 * particular types of symset "handling", define a
 * H_XXX macro in include/rm.h and add the name
 * to this array at the matching offset.
 */
const char *known_handling[] = {
    "UNKNOWN", /* H_UNK */
    "IBM",     /* H_IBM */
    "DEC",     /* H_DEC */
    (const char *) 0,
};

/*
 * Accepted keywords for symset restrictions.
 * These can be virtually anything that you want to
 * be able to test in the code someplace.
 * Be sure to:
 *    - add a corresponding Bitfield to the symsetentry struct in rm.h
 *    - initialize the field to zero in parse_sym_line in the SYM_CONTROL
 *      case 0 section of the idx switch. The location is prefaced with
 *      with a comment stating "initialize restriction bits".
 *    - set the value appropriately based on the index of your keyword
 *      under the case 5 sections of the same SYM_CONTROL idx switches.
 *    - add the field to clear_symsetentry()
 */
const char *known_restrictions[] = {
    "primary", "rogue", (const char *) 0,
};

struct symparse loadsyms[] = {
    { SYM_CONTROL, 0, "start" },
    { SYM_CONTROL, 0, "begin" },
    { SYM_CONTROL, 1, "finish" },
    { SYM_CONTROL, 2, "handling" },
    { SYM_CONTROL, 3, "description" },
    { SYM_CONTROL, 4, "color" },
    { SYM_CONTROL, 4, "colour" },
    { SYM_CONTROL, 5, "restrictions" },
    { SYM_PCHAR, S_stone, "S_stone" },
    { SYM_PCHAR, S_vwall, "S_vwall" },
    { SYM_PCHAR, S_hwall, "S_hwall" },
    { SYM_PCHAR, S_tlcorn, "S_tlcorn" },
    { SYM_PCHAR, S_trcorn, "S_trcorn" },
    { SYM_PCHAR, S_blcorn, "S_blcorn" },
    { SYM_PCHAR, S_brcorn, "S_brcorn" },
    { SYM_PCHAR, S_crwall, "S_crwall" },
    { SYM_PCHAR, S_tuwall, "S_tuwall" },
    { SYM_PCHAR, S_tdwall, "S_tdwall" },
    { SYM_PCHAR, S_tlwall, "S_tlwall" },
    { SYM_PCHAR, S_trwall, "S_trwall" },
    { SYM_PCHAR, S_ndoor, "S_ndoor" },
    { SYM_PCHAR, S_vodoor, "S_vodoor" },
    { SYM_PCHAR, S_hodoor, "S_hodoor" },
    { SYM_PCHAR, S_vcdoor, "S_vcdoor" },
    { SYM_PCHAR, S_hcdoor, "S_hcdoor" },
    { SYM_PCHAR, S_bars, "S_bars" },
    { SYM_PCHAR, S_tree, "S_tree" },
    { SYM_PCHAR, S_room, "S_room" },
    { SYM_PCHAR, S_corr, "S_corr" },
    { SYM_PCHAR, S_litcorr, "S_litcorr" },
    { SYM_PCHAR, S_upstair, "S_upstair" },
    { SYM_PCHAR, S_dnstair, "S_dnstair" },
    { SYM_PCHAR, S_upladder, "S_upladder" },
    { SYM_PCHAR, S_dnladder, "S_dnladder" },
    { SYM_PCHAR, S_altar, "S_altar" },
    { SYM_PCHAR, S_grave, "S_grave" },
    { SYM_PCHAR, S_throne, "S_throne" },
    { SYM_PCHAR, S_sink, "S_sink" },
    { SYM_PCHAR, S_fountain, "S_fountain" },
    { SYM_PCHAR, S_pool, "S_pool" },
    { SYM_PCHAR, S_ice, "S_ice" },
    { SYM_PCHAR, S_lava, "S_lava" },
    { SYM_PCHAR, S_vodbridge, "S_vodbridge" },
    { SYM_PCHAR, S_hodbridge, "S_hodbridge" },
    { SYM_PCHAR, S_vcdbridge, "S_vcdbridge" },
    { SYM_PCHAR, S_hcdbridge, "S_hcdbridge" },
    { SYM_PCHAR, S_air, "S_air" },
    { SYM_PCHAR, S_cloud, "S_cloud" },
    { SYM_PCHAR, S_poisoncloud, "S_poisoncloud" },
    { SYM_PCHAR, S_water, "S_water" },
    { SYM_PCHAR, S_arrow_trap, "S_arrow_trap" },
    { SYM_PCHAR, S_dart_trap, "S_dart_trap" },
    { SYM_PCHAR, S_falling_rock_trap, "S_falling_rock_trap" },
    { SYM_PCHAR, S_squeaky_board, "S_squeaky_board" },
    { SYM_PCHAR, S_bear_trap, "S_bear_trap" },
    { SYM_PCHAR, S_land_mine, "S_land_mine" },
    { SYM_PCHAR, S_rolling_boulder_trap, "S_rolling_boulder_trap" },
    { SYM_PCHAR, S_sleeping_gas_trap, "S_sleeping_gas_trap" },
    { SYM_PCHAR, S_rust_trap, "S_rust_trap" },
    { SYM_PCHAR, S_fire_trap, "S_fire_trap" },
    { SYM_PCHAR, S_pit, "S_pit" },
    { SYM_PCHAR, S_spiked_pit, "S_spiked_pit" },
    { SYM_PCHAR, S_hole, "S_hole" },
    { SYM_PCHAR, S_trap_door, "S_trap_door" },
    { SYM_PCHAR, S_teleportation_trap, "S_teleportation_trap" },
    { SYM_PCHAR, S_level_teleporter, "S_level_teleporter" },
    { SYM_PCHAR, S_magic_portal, "S_magic_portal" },
    { SYM_PCHAR, S_web, "S_web" },
    { SYM_PCHAR, S_statue_trap, "S_statue_trap" },
    { SYM_PCHAR, S_magic_trap, "S_magic_trap" },
    { SYM_PCHAR, S_anti_magic_trap, "S_anti_magic_trap" },
    { SYM_PCHAR, S_polymorph_trap, "S_polymorph_trap" },
    { SYM_PCHAR, S_vbeam, "S_vbeam" },
    { SYM_PCHAR, S_hbeam, "S_hbeam" },
    { SYM_PCHAR, S_lslant, "S_lslant" },
    { SYM_PCHAR, S_rslant, "S_rslant" },
    { SYM_PCHAR, S_digbeam, "S_digbeam" },
    { SYM_PCHAR, S_flashbeam, "S_flashbeam" },
    { SYM_PCHAR, S_boomleft, "S_boomleft" },
    { SYM_PCHAR, S_boomright, "S_boomright" },
    { SYM_PCHAR, S_goodpos, "S_goodpos" },
    { SYM_PCHAR, S_ss1, "S_ss1" },
    { SYM_PCHAR, S_ss2, "S_ss2" },
    { SYM_PCHAR, S_ss3, "S_ss3" },
    { SYM_PCHAR, S_ss4, "S_ss4" },
    { SYM_PCHAR, S_sw_tl, "S_sw_tl" },
    { SYM_PCHAR, S_sw_tc, "S_sw_tc" },
    { SYM_PCHAR, S_sw_tr, "S_sw_tr" },
    { SYM_PCHAR, S_sw_ml, "S_sw_ml" },
    { SYM_PCHAR, S_sw_mr, "S_sw_mr" },
    { SYM_PCHAR, S_sw_bl, "S_sw_bl" },
    { SYM_PCHAR, S_sw_bc, "S_sw_bc" },
    { SYM_PCHAR, S_sw_br, "S_sw_br" },
    { SYM_PCHAR, S_explode1, "S_explode1" },
    { SYM_PCHAR, S_explode2, "S_explode2" },
    { SYM_PCHAR, S_explode3, "S_explode3" },
    { SYM_PCHAR, S_explode4, "S_explode4" },
    { SYM_PCHAR, S_explode5, "S_explode5" },
    { SYM_PCHAR, S_explode6, "S_explode6" },
    { SYM_PCHAR, S_explode7, "S_explode7" },
    { SYM_PCHAR, S_explode8, "S_explode8" },
    { SYM_PCHAR, S_explode9, "S_explode9" },
    { SYM_OC, WEAPON_CLASS + SYM_OFF_O, "S_weapon" },
    { SYM_OC, ARMOR_CLASS + SYM_OFF_O, "S_armor" },
    { SYM_OC, ARMOR_CLASS + SYM_OFF_O, "S_armour" },
    { SYM_OC, RING_CLASS + SYM_OFF_O, "S_ring" },
    { SYM_OC, AMULET_CLASS + SYM_OFF_O, "S_amulet" },
    { SYM_OC, TOOL_CLASS + SYM_OFF_O, "S_tool" },
    { SYM_OC, FOOD_CLASS + SYM_OFF_O, "S_food" },
    { SYM_OC, POTION_CLASS + SYM_OFF_O, "S_potion" },
    { SYM_OC, SCROLL_CLASS + SYM_OFF_O, "S_scroll" },
    { SYM_OC, SPBOOK_CLASS + SYM_OFF_O, "S_book" },
    { SYM_OC, WAND_CLASS + SYM_OFF_O, "S_wand" },
    { SYM_OC, COIN_CLASS + SYM_OFF_O, "S_coin" },
    { SYM_OC, GEM_CLASS + SYM_OFF_O, "S_gem" },
    { SYM_OC, ROCK_CLASS + SYM_OFF_O, "S_rock" },
    { SYM_OC, BALL_CLASS + SYM_OFF_O, "S_ball" },
    { SYM_OC, CHAIN_CLASS + SYM_OFF_O, "S_chain" },
    { SYM_OC, VENOM_CLASS + SYM_OFF_O, "S_venom" },
    { SYM_MON, S_ANT + SYM_OFF_M, "S_ant" },
    { SYM_MON, S_BLOB + SYM_OFF_M, "S_blob" },
    { SYM_MON, S_COCKATRICE + SYM_OFF_M, "S_cockatrice" },
    { SYM_MON, S_DOG + SYM_OFF_M, "S_dog" },
    { SYM_MON, S_EYE + SYM_OFF_M, "S_eye" },
    { SYM_MON, S_FELINE + SYM_OFF_M, "S_feline" },
    { SYM_MON, S_GREMLIN + SYM_OFF_M, "S_gremlin" },
    { SYM_MON, S_HUMANOID + SYM_OFF_M, "S_humanoid" },
    { SYM_MON, S_IMP + SYM_OFF_M, "S_imp" },
    { SYM_MON, S_JELLY + SYM_OFF_M, "S_jelly" },
    { SYM_MON, S_KOBOLD + SYM_OFF_M, "S_kobold" },
    { SYM_MON, S_LEPRECHAUN + SYM_OFF_M, "S_leprechaun" },
    { SYM_MON, S_MIMIC + SYM_OFF_M, "S_mimic" },
    { SYM_MON, S_NYMPH + SYM_OFF_M, "S_nymph" },
    { SYM_MON, S_ORC + SYM_OFF_M, "S_orc" },
    { SYM_MON, S_PIERCER + SYM_OFF_M, "S_piercer" },
    { SYM_MON, S_QUADRUPED + SYM_OFF_M, "S_quadruped" },
    { SYM_MON, S_RODENT + SYM_OFF_M, "S_rodent" },
    { SYM_MON, S_SPIDER + SYM_OFF_M, "S_spider" },
    { SYM_MON, S_TRAPPER + SYM_OFF_M, "S_trapper" },
    { SYM_MON, S_UNICORN + SYM_OFF_M, "S_unicorn" },
    { SYM_MON, S_VORTEX + SYM_OFF_M, "S_vortex" },
    { SYM_MON, S_WORM + SYM_OFF_M, "S_worm" },
    { SYM_MON, S_XAN + SYM_OFF_M, "S_xan" },
    { SYM_MON, S_LIGHT + SYM_OFF_M, "S_light" },
    { SYM_MON, S_ZRUTY + SYM_OFF_M, "S_zruty" },
    { SYM_MON, S_ANGEL + SYM_OFF_M, "S_angel" },
    { SYM_MON, S_BAT + SYM_OFF_M, "S_bat" },
    { SYM_MON, S_CENTAUR + SYM_OFF_M, "S_centaur" },
    { SYM_MON, S_DRAGON + SYM_OFF_M, "S_dragon" },
    { SYM_MON, S_ELEMENTAL + SYM_OFF_M, "S_elemental" },
    { SYM_MON, S_FUNGUS + SYM_OFF_M, "S_fungus" },
    { SYM_MON, S_GNOME + SYM_OFF_M, "S_gnome" },
    { SYM_MON, S_GIANT + SYM_OFF_M, "S_giant" },
    { SYM_MON, S_JABBERWOCK + SYM_OFF_M, "S_jabberwock" },
    { SYM_MON, S_KOP + SYM_OFF_M, "S_kop" },
    { SYM_MON, S_LICH + SYM_OFF_M, "S_lich" },
    { SYM_MON, S_MUMMY + SYM_OFF_M, "S_mummy" },
    { SYM_MON, S_NAGA + SYM_OFF_M, "S_naga" },
    { SYM_MON, S_OGRE + SYM_OFF_M, "S_ogre" },
    { SYM_MON, S_PUDDING + SYM_OFF_M, "S_pudding" },
    { SYM_MON, S_QUANTMECH + SYM_OFF_M, "S_quantmech" },
    { SYM_MON, S_RUSTMONST + SYM_OFF_M, "S_rustmonst" },
    { SYM_MON, S_SNAKE + SYM_OFF_M, "S_snake" },
    { SYM_MON, S_TROLL + SYM_OFF_M, "S_troll" },
    { SYM_MON, S_UMBER + SYM_OFF_M, "S_umber" },
    { SYM_MON, S_VAMPIRE + SYM_OFF_M, "S_vampire" },
    { SYM_MON, S_WRAITH + SYM_OFF_M, "S_wraith" },
    { SYM_MON, S_XORN + SYM_OFF_M, "S_xorn" },
    { SYM_MON, S_YETI + SYM_OFF_M, "S_yeti" },
    { SYM_MON, S_ZOMBIE + SYM_OFF_M, "S_zombie" },
    { SYM_MON, S_HUMAN + SYM_OFF_M, "S_human" },
    { SYM_MON, S_GHOST + SYM_OFF_M, "S_ghost" },
    { SYM_MON, S_GOLEM + SYM_OFF_M, "S_golem" },
    { SYM_MON, S_DEMON + SYM_OFF_M, "S_demon" },
    { SYM_MON, S_EEL + SYM_OFF_M, "S_eel" },
    { SYM_MON, S_LIZARD + SYM_OFF_M, "S_lizard" },
    { SYM_MON, S_WORM_TAIL + SYM_OFF_M, "S_worm_tail" },
    { SYM_MON, S_MIMIC_DEF + SYM_OFF_M, "S_mimic_def" },
    { SYM_OTH, SYM_BOULDER + SYM_OFF_X, "S_boulder" },
    { SYM_OTH, SYM_INVISIBLE + SYM_OFF_X, "S_invisible" },
    { 0, 0, (const char *) 0 } /* fence post */
};

/*drawing.c*/
