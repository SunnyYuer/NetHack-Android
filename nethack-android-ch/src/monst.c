/* NetHack 3.6	monst.c	$NHDT-Date: 1445556875 2015/10/22 23:34:35 $  $NHDT-Branch: master $:$NHDT-Revision: 1.53 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "config.h"
#include "permonst.h"
#include "monsym.h"

#define NO_ATTK    \
    {              \
        0, 0, 0, 0 \
    }

#define WT_ELF 800
#define WT_DRAGON 4500

#ifdef C
#undef C
#endif
#ifdef TEXTCOLOR
#include "color.h"
#define C(color) color
#define HI_DOMESTIC CLR_WHITE /* use for player + friendlies */
#define HI_LORD CLR_MAGENTA
#else
#define C(color)
#endif

void NDECL(monst_init);
/*
 *	Entry Format:		(from permonst.h)
 *
 *	name, symbol (S_* defines),
 *	difficulty level, move rate, armor class, magic resistance,
 *	alignment, creation/geno flags (G_* defines),
 *	6 * attack structs ( type , damage-type, # dice, # sides ),
 *	weight (WT_* defines), nutritional value, extension length,
 *	sounds made (MS_* defines), physical size (MZ_* defines),
 *	resistances, resistances conferred (both MR_* defines),
 *	3 * flag bitmaps (M1_*, M2_*, and M3_* defines respectively)
 *	symbol color (C(x) macro)
 */
#define MON(nam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, col) \
    {                                                                      \
        nam, sym, lvl, gen, atk, siz, mr1, mr2, flg1, flg2, flg3, C(col)   \
    }
/* LVL() and SIZ() collect several fields to cut down on # of args for MON()
 */
#define LVL(lvl, mov, ac, mr, aln) lvl, mov, ac, mr, aln
#define SIZ(wt, nut, snd, siz) wt, nut, snd, siz
/* ATTK() and A() are to avoid braces and commas within args to MON() */
#define ATTK(at, ad, n, d) \
    {                      \
        at, ad, n, d       \
    }
#define A(a1, a2, a3, a4, a5, a6) \
    {                             \
        a1, a2, a3, a4, a5, a6    \
    }

/*
 *	Rule #1:	monsters of a given class are contiguous in the
 *			mons[] array.
 *
 *	Rule #2:	monsters of a given class are presented in ascending
 *			order of strength.
 *
 *	Rule #3:	monster frequency is included in the geno mask;
 *			the frequency can be from 0 to 7.  0's will also
 *			be skipped during generation.
 *
 *	Rule #4:	monster subclasses (e.g. giants) should be kept
 *			together, unless it violates Rule 2.  NOGEN monsters
 *			won't violate Rule 2.
 *
 * Guidelines for color assignment:
 *
 *	* Use the same color for all `growth stages' of a monster (ex.
 *	  little dog/big dog, baby naga/full-grown naga.
 *
 *	* Use colors given in names wherever possible. If the class has `real'
 *	  members with strong color associations, use those.
 *
 *	* Favor `cool' colors for cold-resistant monsters, `warm' ones for
 *	  fire-resistant ones.
 *
 *	* Try to reserve purple (magenta) for powerful `ruler' monsters (queen
 *	  bee, kobold lord, &c.).
 *
 *	* Subject to all these constraints, try to use color to make as many
 *	  distinctions as the / command (that is, within a monster letter
 *	  distinct names should map to distinct colors).
 *
 * The aim in assigning colors is to be consistent enough so a player can
 * become `intuitive' about them, deducing some or all of these rules
 * unconsciously. Use your common sense.
 */

#ifndef SPLITMON_2
NEARDATA struct permonst mons[] = {
    /*
     * ants
     */
    MON("巨型蚂蚁", S_ANT, LVL(2, 18, 3, 0, 0), (G_GENO | G_SGROUP | 3),  //giant ant
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_BROWN),
    MON("杀人蜂", S_ANT, LVL(1, 18, -1, 0, 0), (G_GENO | G_LGROUP | 2),  //killer bee
        A(ATTK(AT_STNG, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_POIS, M2_HOSTILE | M2_FEMALE, 0,
        CLR_YELLOW),
    MON("兵蚁", S_ANT, LVL(3, 18, 3, 0, 0), (G_GENO | G_SGROUP | 2),  //soldier ant
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_DRST, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(20, 5, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BLUE),
    MON("火蚁", S_ANT, LVL(3, 18, 3, 10, 0), (G_GENO | G_SGROUP | 1),  //fire ant
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BITE, AD_FIRE, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 10, MS_SILENT, MZ_TINY), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    MON("巨型甲虫", S_ANT, LVL(5, 6, 4, 0, 0), (G_GENO | 3),  //giant beetle
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_BLACK),
    MON("蜂王", S_ANT, LVL(9, 24, -4, 0, 0), (G_GENO | G_NOGEN),  //queen bee
        A(ATTK(AT_STNG, AD_DRST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1, 5, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_FLY | M1_NOHANDS | M1_OVIPAROUS | M1_POIS,
        M2_HOSTILE | M2_FEMALE | M2_PRINCE, 0, HI_LORD),
    /*
     * blobs
     */
    MON("酸滴", S_BLOB, LVL(1, 3, 8, 0, 0), (G_GENO | 2),  //acid blob
        A(ATTK(AT_NONE, AD_ACID, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 10, MS_SILENT, MZ_TINY),
        MR_SLEEP | MR_POISON | MR_ACID | MR_STONE, MR_STONE,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID,
        M2_WANDER | M2_NEUTER, 0, CLR_GREEN),
    MON("颤抖的斑点", S_BLOB, LVL(5, 1, 8, 0, 0), (G_GENO | 2),  //quivering blob
        A(ATTK(AT_TUCH, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, MR_POISON,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
        M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_WHITE),
    MON("黏胶立方怪", S_BLOB, LVL(6, 6, 8, 0, 0), (G_GENO | 2),  //gelatinous cube
        A(ATTK(AT_TUCH, AD_PLYS, 2, 4), ATTK(AT_NONE, AD_PLYS, 1, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 150, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_ACID
            | MR_STONE,
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_OMNIVORE
            | M1_ACID,
        M2_WANDER | M2_HOSTILE | M2_NEUTER, 0, CLR_CYAN),
    /*
     * cockatrice
     */
    MON("小鸡蛇", S_COCKATRICE, LVL(4, 4, 8, 30, 0),  //chickatrice
        (G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), ATTK(AT_TUCH, AD_STON, 0, 0),
          ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_HISS, MZ_TINY), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE, M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("鸡蛇", S_COCKATRICE, LVL(5, 6, 6, 30, 0), (G_GENO | 5),  //cockatrice
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), ATTK(AT_TUCH, AD_STON, 0, 0),
          ATTK(AT_NONE, AD_STON, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_YELLOW),
    MON("蛇鸡兽", S_COCKATRICE, LVL(6, 6, 6, 30, 0), (G_GENO | 1),  //pyrolisk
        A(ATTK(AT_GAZE, AD_FIRE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_HISS, MZ_SMALL), MR_POISON | MR_FIRE,
        MR_POISON | MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE | M1_OVIPAROUS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    /*
     * dogs & other canines
     */
    MON("豺狼", S_DOG, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 3),  //jackal
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("狐狸", S_DOG, LVL(0, 15, 7, 0, 0), (G_GENO | 1),  //fox
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_RED),
    MON("土狼", S_DOG, LVL(1, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1),  //coyote
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("豺狼人", S_DOG, LVL(2, 12, 7, 10, -7), (G_NOGEN | G_NOCORPSE),  //werejackal
        A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 250, MS_BARK, MZ_SMALL), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("小狗", S_DOG, LVL(2, 18, 6, 0, 0), (G_GENO | 1),  //little dog
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 150, MS_BARK, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("澳洲野狗", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1),  //dingo
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_YELLOW),
    MON("狗", S_DOG, LVL(4, 16, 5, 0, 0), (G_GENO | 1),  //dog
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 200, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("大狗", S_DOG, LVL(6, 15, 4, 0, 0), (G_GENO | 1),  //large dog
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 250, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("狼", S_DOG, LVL(5, 12, 4, 0, 0), (G_GENO | G_SGROUP | 2),  //wolf
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("狼人", S_DOG, LVL(5, 12, 4, 20, -7), (G_NOGEN | G_NOCORPSE),  //werewolf
        A(ATTK(AT_BITE, AD_WERE, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_BARK, MZ_MEDIUM), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("冬狼崽", S_DOG, LVL(5, 12, 4, 0, -5),  //winter wolf cub
        (G_NOHELL | G_GENO | G_SGROUP | 2),
        A(ATTK(AT_BITE, AD_PHYS, 1, 8), ATTK(AT_BREA, AD_COLD, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(250, 200, MS_BARK, MZ_SMALL), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_CYAN),
    MON("座狼", S_DOG, LVL(7, 12, 4, 0, -5), (G_GENO | G_SGROUP | 2),  //warg
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 350, MS_BARK, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("冬狼", S_DOG, LVL(7, 12, 4, 20, 0), (G_NOHELL | G_GENO | 1),  //winter wolf
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_COLD, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(700, 300, MS_BARK, MZ_LARGE), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG, 0,
        CLR_CYAN),
    MON("地狱小猎犬", S_DOG, LVL(7, 12, 4, 20, -5),  //hell hound pup
        (G_HELL | G_GENO | G_SGROUP | 1),
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 200, MS_BARK, MZ_SMALL), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_RED),
    MON("地狱猎犬", S_DOG, LVL(12, 14, 2, 20, 0), (G_HELL | G_GENO | 1),  //hell hound
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BREA, AD_FIRE, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_BARK, MZ_MEDIUM), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_RED),
#ifdef CHARON
    MON("刻耳柏洛斯", S_DOG, LVL(12, 10, 2, 20, -7),  //Cerberus
        (G_NOGEN | G_UNIQ | G_HELL),
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_BITE, AD_PHYS, 3, 6),
          ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 350, MS_BARK, MZ_LARGE), MR_FIRE, MR_FIRE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_MALE,
        M3_INFRAVISIBLE, CLR_RED),
#endif
    /*
     * eyes
     */
    MON("气体孢子", S_EYE, LVL(1, 3, 10, 0, 0), (G_NOCORPSE | G_GENO | 1),  //gas spore
        A(ATTK(AT_BOOM, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("浮眼", S_EYE, LVL(2, 1, 9, 10, 0), (G_GENO | 5),  //floating eye
        A(ATTK(AT_NONE, AD_PLYS, 0, 70), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), 0, 0,
        M1_FLY | M1_AMPHIBIOUS | M1_NOLIMBS | M1_NOHEAD | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE),
    MON("冻结球", S_EYE, LVL(6, 13, 4, 0, 0),  //freezing sphere
        (G_NOCORPSE | G_NOHELL | G_GENO | 2),
        A(ATTK(AT_EXPL, AD_COLD, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, MR_COLD,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_WHITE),
    MON("火焰球", S_EYE, LVL(6, 13, 4, 0, 0),  //flaming sphere
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_FIRE, 4, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_FIRE, MR_FIRE,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED),
    MON("电球", S_EYE, LVL(6, 13, 4, 0, 0),  //shocking sphere
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_ELEC, 4, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_ELEC, MR_ELEC,
        M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, HI_ZAP),
#if 0 /* not yet implemented */
    MON("beholder", S_EYE,
	LVL(6, 3, 4, 0, -10), (G_GENO | 2),
	A(ATTK(AT_GAZE, AD_SLOW, 0, 0), ATTK(AT_GAZE, AD_SLEE, 2,25),
	  ATTK(AT_GAZE, AD_DISN, 0, 0), ATTK(AT_GAZE, AD_STON, 0, 0),
	  ATTK(AT_GAZE, AD_CNCL, 2, 4), ATTK(AT_BITE, AD_PHYS, 2, 4)),
	SIZ(10, 10, MS_SILENT, MZ_SMALL), MR_COLD, 0,
	M1_FLY | M1_BREATHLESS | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS,
	M2_NOPOLY | M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BROWN),
#endif
    /*
     * felines
     */
    MON("小猫", S_FELINE, LVL(2, 18, 6, 0, 0), (G_GENO | 1),  //kitten
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 150, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("家猫", S_FELINE, LVL(4, 16, 5, 0, 0), (G_GENO | 1),  //housecat
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_DOMESTIC, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("捷豹", S_FELINE, LVL(4, 15, 6, 0, 0), (G_GENO | 2),  //jaguar
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("猞猁", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1),  //lynx
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_CYAN),
    MON("美洲豹", S_FELINE, LVL(5, 15, 6, 0, 0), (G_GENO | 1),  //panther
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BLACK),
    MON("大猫", S_FELINE, LVL(6, 15, 4, 0, 0), (G_GENO | 1),  //large cat
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(250, 250, MS_MEW, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_STRONG | M2_DOMESTIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("老虎", S_FELINE, LVL(6, 12, 6, 0, 0), (G_GENO | 2),  //tiger
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_BITE, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_YELLOW),
    /*
     * gremlins and gargoyles
     */
    MON("小鬼", S_GREMLIN, LVL(5, 12, 2, 25, -9), (G_GENO | 2),  //gremlin
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_CURS, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(100, 20, MS_LAUGH, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_HUMANOID | M1_POIS, M2_STALK, M3_INFRAVISIBLE,
        CLR_GREEN),
    MON("石像鬼", S_GREMLIN, LVL(6, 10, -4, 0, -9), (G_GENO | 2),  //gargoyle
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6),
          ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 200, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE,
        M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS, M2_HOSTILE | M2_STRONG,
        0, CLR_BROWN),
    MON("飞翼石像鬼", S_GREMLIN, LVL(9, 15, -2, 0, -12), (G_GENO | 1),  //winged gargoyle
        A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6),
          ATTK(AT_BITE, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 300, MS_GRUNT, MZ_HUMAN), MR_STONE, MR_STONE,
        M1_FLY | M1_HUMANOID | M1_THICK_HIDE | M1_BREATHLESS | M1_OVIPAROUS,
        M2_LORD | M2_HOSTILE | M2_STRONG | M2_MAGIC, 0, HI_LORD),
    /*
     * humanoids
     */
    MON("霍比特人", S_HUMANOID, LVL(1, 9, 10, 0, 6), (G_GENO | 2),  //hobbit
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 200, MS_HUMANOID, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("矮人", S_HUMANOID, LVL(2, 6, 10, 10, 4), (G_GENO | 3),  //dwarf
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_DWARF | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("熊地精", S_HUMANOID, LVL(3, 9, 5, 0, -6), (G_GENO | 1),  //bugbear
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1250, 250, MS_GROWL, MZ_LARGE), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("矮人领主", S_HUMANOID, LVL(4, 6, 10, 10, 5), (G_GENO | 2),  //dwarf lord
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_DWARF | M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("矮人王", S_HUMANOID, LVL(6, 6, 10, 20, 6), (G_GENO | 1),  //dwarf king
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 300, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_DWARF | M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("夺心魔", S_HUMANOID, LVL(9, 12, 5, 90, -8), (G_GENO | 1),  //mind flayer
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1), NO_ATTK,
          NO_ATTK),
        SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE,
        M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    MON("夺心魔大师", S_HUMANOID, LVL(13, 12, 0, 90, -8),  //master mind flayer
        (G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1),
          ATTK(AT_TENT, AD_DRIN, 2, 1), ATTK(AT_TENT, AD_DRIN, 2, 1)),
        SIZ(1450, 400, MS_HISS, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_FLY | M1_SEE_INVIS | M1_OMNIVORE,
        M2_HOSTILE | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    /*
     * imps & other minor demons/devils
     */
    MON("灵魂", S_IMP, LVL(1, 3, 7, 0, -7),  //manes
        (G_GENO | G_LGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_SLEEP | MR_POISON, 0, M1_POIS,
        M2_HOSTILE | M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("雏形人", S_IMP, LVL(2, 12, 6, 10, -7), (G_GENO | 2),  //homunculus
        A(ATTK(AT_BITE, AD_SLEE, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 100, MS_SILENT, MZ_TINY), MR_SLEEP | MR_POISON,
        MR_SLEEP | MR_POISON, M1_FLY | M1_POIS, M2_STALK,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("小恶魔", S_IMP, LVL(3, 12, 2, 20, -7), (G_GENO | 1),  //imp
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 10, MS_CUSS, MZ_TINY), 0, 0, M1_REGEN, M2_WANDER | M2_STALK,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("劣魔", S_IMP, LVL(3, 3, 7, 0, -7),  //lemure
        (G_HELL | G_GENO | G_LGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 100, MS_SILENT, MZ_MEDIUM), MR_SLEEP | MR_POISON, MR_SLEEP,
        M1_POIS | M1_REGEN, M2_HOSTILE | M2_WANDER | M2_STALK | M2_NEUTER,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("夸塞魔", S_IMP, LVL(3, 15, 2, 20, -7), (G_GENO | 2),  //quasit
        A(ATTK(AT_CLAW, AD_DRDX, 1, 2), ATTK(AT_CLAW, AD_DRDX, 1, 2),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON, M1_REGEN,
        M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("天狗", S_IMP, LVL(6, 13, 5, 30, 7), (G_GENO | 3),  //tengu
        A(ATTK(AT_BITE, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 200, MS_SQAWK, MZ_SMALL), MR_POISON, MR_POISON,
        M1_TPORT | M1_TPORT_CNTRL, M2_STALK, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_CYAN),
    /*
     * jellies
     */
    MON("蓝色果冻", S_JELLY, LVL(4, 0, 8, 10, 0), (G_GENO | 2),  //blue jelly
        A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_COLD | MR_POISON,
        MR_COLD | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLUE),
    MON("珍珠果冻", S_JELLY, LVL(5, 0, 8, 10, 0), (G_GENO | 1),  //spotted jelly
        A(ATTK(AT_NONE, AD_ACID, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("赭冻怪", S_JELLY, LVL(6, 3, 8, 20, 0), (G_GENO | 2),  //ochre jelly
        A(ATTK(AT_ENGL, AD_ACID, 3, 6), ATTK(AT_NONE, AD_ACID, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(50, 20, MS_SILENT, MZ_MEDIUM), MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    /*
     * kobolds
     */
    MON("狗头人", S_KOBOLD, LVL(0, 6, 10, 0, -2), (G_GENO | 1),  //kobold
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 100, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("大狗头人", S_KOBOLD, LVL(1, 6, 10, 0, -3), (G_GENO | 1),  //large kobold
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("狗头人领主", S_KOBOLD, LVL(2, 6, 10, 0, -4), (G_GENO | 1),  //kobold lord
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 200, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE,
        M2_HOSTILE | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("狗头人萨满", S_KOBOLD, LVL(2, 6, 6, 10, -4), (G_GENO | 1),  //kobold shaman
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(450, 150, MS_ORC, MZ_SMALL), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_OMNIVORE, M2_HOSTILE | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    /*
     * leprechauns
     */
    MON("小矮妖", S_LEPRECHAUN, LVL(5, 15, 8, 20, 0), (G_GENO | 4),  //leprechaun
        A(ATTK(AT_CLAW, AD_SGLD, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 30, MS_LAUGH, MZ_TINY), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_GREEDY, M3_INFRAVISIBLE, CLR_GREEN),
    /*
     * mimics
     */
    MON("小拟型怪", S_MIMIC, LVL(7, 3, 7, 0, 0), (G_GENO | 2),  //small mimic
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 200, MS_SILENT, MZ_MEDIUM), MR_ACID, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL | M1_NOEYES
            | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("大拟型怪", S_MIMIC, LVL(8, 3, 7, 10, 0), (G_GENO | 1),  //large mimic
        A(ATTK(AT_CLAW, AD_STCK, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 400, MS_SILENT, MZ_LARGE), MR_ACID, 0,
        M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL
            | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_RED),
    MON("巨型拟型怪", S_MIMIC, LVL(9, 3, 7, 20, 0), (G_GENO | 1),  //giant mimic
        A(ATTK(AT_CLAW, AD_STCK, 3, 6), ATTK(AT_CLAW, AD_STCK, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 500, MS_SILENT, MZ_LARGE), MR_ACID, 0,
        M1_CLING | M1_BREATHLESS | M1_AMORPHOUS | M1_HIDE | M1_ANIMAL
            | M1_NOEYES | M1_NOHEAD | M1_NOLIMBS | M1_THICK_HIDE
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, HI_LORD),
    /*
     * nymphs
     */
    MON("木仙女", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),  //wood nymph
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_GREEN),
    MON("水仙女", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),  //water nymph
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_TPORT | M1_SWIM, M2_HOSTILE | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("山仙女", S_NYMPH, LVL(3, 12, 9, 20, 0), (G_GENO | 2),  //mountain nymph
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_CLAW, AD_SEDU, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SEDUCE, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_TPORT,
        M2_HOSTILE | M2_FEMALE | M2_COLLECT, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * orcs
     */
    MON("地精", S_ORC, LVL(0, 6, 10, 0, -3), (G_GENO | 2),  //goblin
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("大地精", S_ORC, LVL(1, 9, 10, 0, -4), (G_GENO | 2),  //hobgoblin
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    /* plain "orc" for zombie corpses only; not created at random
     */
    MON("兽人", S_ORC, LVL(1, 9, 10, 0, -3), (G_GENO | G_NOGEN | G_LGROUP),  //orc
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 150, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("希尔兽人", S_ORC, LVL(2, 9, 10, 0, -4), (G_GENO | G_LGROUP | 2),  //hill orc
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    MON("魔多兽人", S_ORC, LVL(3, 5, 10, 0, -5), (G_GENO | G_LGROUP | 1),  //Mordor orc
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1200, 200, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("强兽人", S_ORC, LVL(3, 7, 10, 0, -4), (G_GENO | G_LGROUP | 1),  //Uruk-hai
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1300, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLACK),
    MON("兽人萨满", S_ORC, LVL(3, 9, 5, 10, -5), (G_GENO | 1),  //orc shaman
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1000, 300, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    MON("兽人队长", S_ORC, LVL(5, 5, 10, 0, -5), (G_GENO | 1),  //orc-captain
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1350, 350, MS_ORC, MZ_HUMAN), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_ORC | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * piercers
     */
    MON("岩石锥子", S_PIERCER, LVL(3, 1, 3, 0, 0), (G_GENO | 4),  //rock piercer
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_SMALL), 0, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("铁锥子", S_PIERCER, LVL(5, 1, 0, 0, 0), (G_GENO | 2),  //iron piercer
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 300, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_CYAN),
    MON("玻璃锥子", S_PIERCER, LVL(7, 1, 0, 0, 0), (G_GENO | 1),  //glass piercer
        A(ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 300, MS_SILENT, MZ_MEDIUM), MR_ACID, 0,
        M1_CLING | M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_WHITE),
    /*
     * quadrupeds
     */
    MON("洛斯兽", S_QUADRUPED, LVL(2, 9, 7, 0, 0), (G_GENO | G_SGROUP | 4),  //rothe
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 100, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("猛犸", S_QUADRUPED, LVL(5, 9, 0, 0, -2), (G_GENO | 1),  //mumak
        A(ATTK(AT_BUTT, AD_PHYS, 4, 12), ATTK(AT_BITE, AD_PHYS, 2, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2500, 500, MS_ROAR, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("狼狗", S_QUADRUPED, LVL(6, 18, 4, 10, 0), (G_GENO | 2),  //leocrotta
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_BITE, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 500, MS_IMITATE, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_RED),
    MON("狮头象", S_QUADRUPED, LVL(8, 3, 2, 10, 0), (G_GENO | 1),  //wumpus
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 500, MS_BURBLE, MZ_LARGE), 0, 0,
        M1_CLING | M1_ANIMAL | M1_NOHANDS | M1_OMNIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_CYAN),
    MON("雷兽", S_QUADRUPED, LVL(12, 12, 6, 0, 0), (G_GENO | 2),  //titanothere
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2650, 650, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("俾路支兽", S_QUADRUPED, LVL(14, 12, 5, 0, 0), (G_GENO | 2),  //baluchitherium
        A(ATTK(AT_CLAW, AD_PHYS, 5, 4), ATTK(AT_CLAW, AD_PHYS, 5, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_GRAY),
    MON("乳齿象", S_QUADRUPED, LVL(20, 12, 5, 0, 0), (G_GENO | 1),  //mastodon
        A(ATTK(AT_BUTT, AD_PHYS, 4, 8), ATTK(AT_BUTT, AD_PHYS, 4, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(3800, 800, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS | M1_HERBIVORE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * rodents
     */
    MON("褐鼠", S_RODENT, LVL(0, 12, 7, 0, 0), (G_GENO | G_SGROUP | 1),  //sewer rat
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 12, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("巨鼠", S_RODENT, LVL(1, 10, 7, 0, 0), (G_GENO | G_SGROUP | 2),  //giant rat
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("狂鼠", S_RODENT, LVL(2, 12, 6, 0, 0), (G_GENO | 1),  //rabid rat
        A(ATTK(AT_BITE, AD_DRCO, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 5, MS_SQEEK, MZ_TINY), MR_POISON, 0,
        M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_CARNIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("鼠人", S_RODENT, LVL(2, 12, 6, 10, -7), (G_NOGEN | G_NOCORPSE),  //wererat
        A(ATTK(AT_BITE, AD_WERE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(40, 30, MS_SQEEK, MZ_TINY), MR_POISON, 0,
        M1_NOHANDS | M1_POIS | M1_REGEN | M1_CARNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    MON("岩石鼹鼠", S_RODENT, LVL(3, 3, 0, 20, 0), (G_GENO | 2),  //rock mole
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_TUNNEL | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE,
        M2_HOSTILE | M2_GREEDY | M2_JEWELS | M2_COLLECT, M3_INFRAVISIBLE,
        CLR_GRAY),
    MON("土拨鼠", S_RODENT, LVL(3, 3, 0, 20, 0), (G_NOGEN | G_GENO),  //woodchuck
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_TUNNEL /*LOGGING*/ | M1_ANIMAL | M1_NOHANDS | M1_SWIM
            | M1_HERBIVORE,
        /* In reality, they tunnel instead of cutting lumber.  Oh, well. */
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * spiders & scorpions (keep webmaker() in sync if new critters are added)
     */
    MON("洞穴蜘蛛", S_SPIDER, LVL(1, 12, 3, 0, 0), (G_GENO | G_SGROUP | 2),  //cave spider
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("蜈蚣", S_SPIDER, LVL(2, 4, 3, 0, 0), (G_GENO | 1),  //centipede
        A(ATTK(AT_BITE, AD_DRST, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 50, MS_SILENT, MZ_TINY), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_YELLOW),
    MON("巨型蜘蛛", S_SPIDER, LVL(5, 15, 4, 0, 0), (G_GENO | 1),  //giant spider
        A(ATTK(AT_BITE, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_LARGE), MR_POISON, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_MAGENTA),
    MON("蝎子", S_SPIDER, LVL(5, 15, 3, 0, 0), (G_GENO | 2),  //scorpion
        A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2),
          ATTK(AT_STNG, AD_DRST, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(50, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_CONCEAL | M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS
            | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_RED),
    /*
     * trappers, lurkers, &c
     */
    MON("潜伏者", S_TRAPPER, LVL(10, 3, 3, 0, 0), (G_GENO | 2),  //lurker above
        A(ATTK(AT_ENGL, AD_DGST, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0,
        M1_HIDE | M1_FLY | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GRAY),
    MON("捕兽者", S_TRAPPER, LVL(12, 3, 3, 0, 0), (G_GENO | 2),  //trapper
        A(ATTK(AT_ENGL, AD_DGST, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(800, 350, MS_SILENT, MZ_HUGE), 0, 0,
        M1_HIDE | M1_ANIMAL | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STALK | M2_STRONG, 0, CLR_GREEN),
    /*
     * unicorns and horses
     */
    MON("小马", S_UNICORN, LVL(3, 16, 6, 0, 0), (G_GENO | 2),  //pony
        A(ATTK(AT_KICK, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 250, MS_NEIGH, MZ_MEDIUM), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    MON("白色独角兽", S_UNICORN, LVL(4, 24, 2, 70, 7), (G_GENO | 2),  //white unicorn
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("灰色独角兽", S_UNICORN, LVL(4, 24, 2, 70, 0), (G_GENO | 1),  //gray unicorn
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("黑色独角兽", S_UNICORN, LVL(4, 24, 2, 70, -7), (G_GENO | 1),  //black unicorn
        A(ATTK(AT_BUTT, AD_PHYS, 1, 12), ATTK(AT_KICK, AD_PHYS, 1, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1300, 300, MS_NEIGH, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOHANDS | M1_HERBIVORE, M2_WANDER | M2_STRONG | M2_JEWELS,
        M3_INFRAVISIBLE, CLR_BLACK),
    MON("马", S_UNICORN, LVL(5, 20, 5, 0, 0), (G_GENO | 2),  //horse
        A(ATTK(AT_KICK, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 300, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    MON("战马", S_UNICORN, LVL(7, 24, 4, 0, 0), (G_GENO | 2),  //warhorse
        A(ATTK(AT_KICK, AD_PHYS, 1, 10), ATTK(AT_BITE, AD_PHYS, 1, 4),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 350, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_HERBIVORE,
        M2_WANDER | M2_STRONG | M2_DOMESTIC, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * vortices
     */
    MON("雾云", S_VORTEX, LVL(3, 1, 0, 0, 0), (G_GENO | G_NOCORPSE | 2),  //fog cloud
        A(ATTK(AT_ENGL, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_AMORPHOUS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("尘埃旋涡", S_VORTEX, LVL(4, 20, 2, 30, 0),  //dust vortex
        (G_GENO | G_NOCORPSE | 2), A(ATTK(AT_ENGL, AD_BLND, 2, 8), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("冰旋涡", S_VORTEX, LVL(5, 20, 2, 30, 0),  //ice vortex
        (G_NOHELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_COLD, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_CYAN),
    MON("能量旋涡", S_VORTEX, LVL(6, 20, 2, 30, 0),  //energy vortex
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_ELEC, 1, 6), ATTK(AT_ENGL, AD_DREN, 4, 6),
          ATTK(AT_NONE, AD_ELEC, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_ELEC | MR_SLEEP | MR_DISINT | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, 0, HI_ZAP),
    MON("蒸汽旋涡", S_VORTEX, LVL(7, 22, 2, 30, 0),  //steam vortex
        (G_HELL | G_GENO | G_NOCORPSE | 2),
        A(ATTK(AT_ENGL, AD_FIRE, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_BLUE),
    MON("火旋涡", S_VORTEX, LVL(8, 22, 2, 30, 0),  //fire vortex
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_ENGL, AD_FIRE, 1, 10), ATTK(AT_NONE, AD_FIRE, 0, 4),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_UNSOLID,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    /*
     * worms
     */
    MON("幼长蠕虫", S_WORM, LVL(5, 3, 5, 0, 0), G_GENO,  //baby long worm
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("幼紫蠕虫", S_WORM, LVL(8, 3, 5, 0, 0), G_GENO,  //baby purple worm
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(600, 250, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_CARNIVORE, M2_HOSTILE, 0,
        CLR_MAGENTA),
    MON("长蠕虫", S_WORM, LVL(9, 3, 5, 10, 0), (G_GENO | 2),  //long worm
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_SILENT, MZ_GIGANTIC), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE
            | M1_NOTAKE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_BROWN),
    MON("紫蠕虫", S_WORM, LVL(15, 9, 6, 20, 0), (G_GENO | 2),  //purple worm
        A(ATTK(AT_BITE, AD_PHYS, 2, 8), ATTK(AT_ENGL, AD_DGST, 1, 10),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2700, 700, MS_SILENT, MZ_GIGANTIC), 0, 0,
        M1_ANIMAL | M1_SLITHY | M1_NOLIMBS | M1_OVIPAROUS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, 0, CLR_MAGENTA),
    /*
     * xan, &c
     */
    MON("电子虫", S_XAN, LVL(0, 12, 9, 0, 0),  //grid bug
        (G_GENO | G_SGROUP | G_NOCORPSE | 3),
        A(ATTK(AT_BITE, AD_ELEC, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(15, 10, MS_BUZZ, MZ_TINY), MR_ELEC | MR_POISON, 0, M1_ANIMAL,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_MAGENTA),
    MON("玄蚊", S_XAN, LVL(7, 18, -4, 0, 0), (G_GENO | 3),  //xan
        A(ATTK(AT_STNG, AD_LEGS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(300, 300, MS_BUZZ, MZ_TINY), MR_POISON, MR_POISON,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_RED),
    /*
     * lights
     */
    MON("黄光", S_LIGHT, LVL(3, 15, 0, 0, 0),  //yellow light
        (G_NOCORPSE | G_GENO | 4), A(ATTK(AT_EXPL, AD_BLND, 10, 20), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_SMALL),
        MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON
            | MR_ACID | MR_STONE,
        0, M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS
               | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    MON("黑光", S_LIGHT, LVL(5, 15, 0, 0, 0),  //black light
        (G_NOCORPSE | G_GENO | 2), A(ATTK(AT_EXPL, AD_HALU, 10, 12), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_SMALL),
        MR_FIRE | MR_COLD | MR_ELEC | MR_DISINT | MR_SLEEP | MR_POISON
            | MR_ACID | MR_STONE,
        0,
        M1_FLY | M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS
            | M1_NOHEAD | M1_MINDLESS | M1_UNSOLID | M1_SEE_INVIS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK),
    /*
     * zruty
     */
    MON("山区巨人", S_ZRUTY, LVL(9, 8, 3, 0, 0), (G_GENO | 2),  //zruty
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_BITE, AD_PHYS, 3, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 600, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * Angels and other lawful minions
     */
    MON("羽蛇", S_ANGEL, LVL(8, 10, 5, 30, 7),  //couatl
        (G_NOHELL | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_BITE, AD_PHYS, 1, 3),
          ATTK(AT_HUGS, AD_WRAP, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 400, MS_HISS, MZ_LARGE), MR_POISON, 0,
        M1_FLY | M1_NOHANDS | M1_SLITHY | M1_POIS,
        M2_MINION | M2_STALK | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("亚历克斯", S_ANGEL, LVL(10, 8, 0, 30, 7), (G_NOHELL | G_NOCORPSE | 1),  //Aleax
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN),
        MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS,
        M2_MINION | M2_STALK | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    /* Angels start with the emin extension attached, and usually have
       the isminion flag set; however, non-minion Angels can be tamed
       and will switch to edog (guardian Angel is handled specially and
       always sticks with emin) */
    MON("天使", S_ANGEL, LVL(14, 10, -4, 55, 12),  //Angel
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN),
        MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_HUMANOID | M1_SEE_INVIS,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("麒麟", S_ANGEL, LVL(16, 18, -5, 90, 15),  //ki-rin
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_KICK, AD_PHYS, 2, 4), ATTK(AT_KICK, AD_PHYS, 2, 4),
          ATTK(AT_BUTT, AD_PHYS, 3, 6), ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEIGH, MZ_LARGE), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_SEE_INVIS,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_GOLD),
    MON("执政官", S_ANGEL, LVL(19, 16, -6, 80, 15),  //Archon
        (G_NOHELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4),
          ATTK(AT_GAZE, AD_BLND, 2, 6), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_HUMANOID | M1_SEE_INVIS | M1_REGEN,
        M2_NOPOLY | M2_MINION | M2_STALK | M2_STRONG | M2_NASTY | M2_LORD
            | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Bats
     */
    MON("蝙蝠", S_BAT, LVL(0, 22, 8, 0, 0), (G_GENO | G_SGROUP | 1),  //bat
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 20, MS_SQEEK, MZ_TINY), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_WANDER,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("巨蝙蝠", S_BAT, LVL(2, 22, 7, 0, 0), (G_GENO | 2),  //giant bat
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SQEEK, MZ_SMALL), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_RED),
    MON("乌鸦", S_BAT, LVL(4, 20, 6, 0, 0), (G_GENO | 2),  //raven
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_BLND, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(40, 20, MS_SQAWK, MZ_SMALL), 0, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_WANDER | M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK),
    MON("吸血蝙蝠", S_BAT, LVL(5, 20, 6, 0, 0), (G_GENO | 2),  //vampire bat
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRST, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(30, 20, MS_SQEEK, MZ_SMALL), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_ANIMAL | M1_NOHANDS | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * Centaurs
     */
    MON("平原半人马", S_CENTAUR, LVL(4, 18, 4, 0, 0), (G_GENO | 1),  //plains centaur
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2500, 500, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("森林半人马", S_CENTAUR, LVL(5, 18, 3, 10, -1), (G_GENO | 1),  //forest centaur
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2550, 600, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    MON("山半人马", S_CENTAUR, LVL(6, 20, 2, 10, -3), (G_GENO | 1),  //mountain centaur
        A(ATTK(AT_WEAP, AD_PHYS, 1, 10), ATTK(AT_KICK, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2550, 500, MS_HUMANOID, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_CYAN),
    /*
     * Dragons
     */
    /* The order of the dragons is VERY IMPORTANT.  Quite a few
     * pieces of code depend on gray being first and yellow being last.
     * The code also depends on the *order* being the same as that for
     * dragon scale mail and dragon scales in objects.c.  Baby dragons
     * cannot confer intrinsics, to avoid polyself/egg abuse.
     *
     * As reptiles, dragons are cold-blooded and thus aren't seen
     * with infravision.  Red dragons are the exception.
     */
    MON("幼灰龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby gray dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GRAY),
    MON("幼银龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby silver dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, DRAGON_SILVER),
#if 0 /* DEFERRED */
    MON("baby shimmering dragon", S_DRAGON,
	LVL(12, 9, 2, 10, 0), G_GENO,
	A(ATTK(AT_BITE, AD_PHYS, 2, 6),
	  NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(1500, 500, MS_ROAR, MZ_HUGE), 0, 0,
	M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
	M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_CYAN),
#endif
    MON("幼红龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby red dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_FIRE, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, M3_INFRAVISIBLE,
        CLR_RED),
    MON("幼白龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby white dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_COLD, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_WHITE),
    MON("幼橙龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby orange dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_SLEEP, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_ORANGE),
    MON("幼黑龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby black dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_DISINT, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLACK),
    MON("幼蓝龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby blue dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ELEC, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_BLUE),
    MON("幼绿龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby green dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_POISON, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_GREEN),
    MON("幼黄龙", S_DRAGON, LVL(12, 9, 2, 10, 0), G_GENO,  //baby yellow dragon
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_ROAR, MZ_HUGE), MR_ACID | MR_STONE, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_ACID,
        M2_HOSTILE | M2_STRONG | M2_GREEDY | M2_JEWELS, 0, CLR_YELLOW),
    MON("灰龙", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1),  //gray dragon
        A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_GRAY),
    MON("银龙", S_DRAGON, LVL(15, 9, -1, 20, 4), (G_GENO | 1),  //silver dragon
        A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, 0,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, DRAGON_SILVER),
#if 0 /* DEFERRED */
    MON("shimmering dragon", S_DRAGON,
	LVL(15, 9, -1, 20, 4), (G_GENO | 1),
	A(ATTK(AT_BREA, AD_MAGM, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
	  ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
	  NO_ATTK, NO_ATTK),
	SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), 0, 0,
	M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
	  | M1_CARNIVORE,
	M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
	0, CLR_CYAN),
#endif
    MON("红龙", S_DRAGON, LVL(15, 9, -1, 20, -4), (G_GENO | 1),  //red dragon
        A(ATTK(AT_BREA, AD_FIRE, 6, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_FIRE, MR_FIRE,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_INFRAVISIBLE, CLR_RED),
    MON("白龙", S_DRAGON, LVL(15, 9, -1, 20, -5), (G_GENO | 1),  //white dragon
        A(ATTK(AT_BREA, AD_COLD, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_COLD, MR_COLD,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_WHITE),
    MON("橙龙", S_DRAGON, LVL(15, 9, -1, 20, 5), (G_GENO | 1),  //orange dragon
        A(ATTK(AT_BREA, AD_SLEE, 4, 25), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_SLEEP, MR_SLEEP,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_ORANGE),
    MON("黑龙", S_DRAGON, LVL(15, 9, -1, 20, -6), (G_GENO | 1),  //black dragon
        A(ATTK(AT_BREA, AD_DISN, 4, 10), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_DISINT, MR_DISINT,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_BLACK),
    MON("蓝龙", S_DRAGON, LVL(15, 9, -1, 20, -7), (G_GENO | 1),  //blue dragon
        A(ATTK(AT_BREA, AD_ELEC, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ELEC, MR_ELEC,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_BLUE),
    MON("绿龙", S_DRAGON, LVL(15, 9, -1, 20, 6), (G_GENO | 1),  //green dragon
        A(ATTK(AT_BREA, AD_DRST, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_POISON, MR_POISON,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS | M1_OVIPAROUS
            | M1_CARNIVORE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_GREEN),
    MON("黄龙", S_DRAGON, LVL(15, 9, -1, 20, 7), (G_GENO | 1),  //yellow dragon
        A(ATTK(AT_BREA, AD_ACID, 4, 6), ATTK(AT_BITE, AD_PHYS, 3, 8),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_DRAGON, 1500, MS_ROAR, MZ_GIGANTIC), MR_ACID | MR_STONE,
        MR_STONE, M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_SEE_INVIS
                      | M1_OVIPAROUS | M1_CARNIVORE | M1_ACID,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        0, CLR_YELLOW),
    /*
     * Elementals
     */
    MON("潜行者", S_ELEMENTAL, LVL(8, 12, 3, 0, 0), (G_GENO | 3),  //stalker
        A(ATTK(AT_CLAW, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 400, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_FLY | M1_SEE_INVIS,
        M2_WANDER | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION,
        CLR_WHITE),
    MON("空气元素", S_ELEMENTAL, LVL(8, 36, 2, 30, 0), (G_NOCORPSE | 1),  //air elemental
        A(ATTK(AT_ENGL, AD_PHYS, 1, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_FLY,
        M2_STRONG | M2_NEUTER, 0, CLR_CYAN),
    MON("火元素", S_ELEMENTAL, LVL(8, 12, 2, 30, 0), (G_NOCORPSE | 1),  //fire elemental
        A(ATTK(AT_CLAW, AD_FIRE, 3, 6), ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUGE), MR_FIRE | MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_FLY | M1_NOTAKE,
        M2_STRONG | M2_NEUTER, M3_INFRAVISIBLE, CLR_YELLOW),
    MON("土元素", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1),  //earth elemental
        A(ATTK(AT_CLAW, AD_PHYS, 4, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 0, MS_SILENT, MZ_HUGE),
        MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_WALLWALK | M1_THICK_HIDE,
        M2_STRONG | M2_NEUTER, 0, CLR_BROWN),
    MON("水元素", S_ELEMENTAL, LVL(8, 6, 2, 30, 0), (G_NOCORPSE | 1),  //water elemental
        A(ATTK(AT_CLAW, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2500, 0, MS_SILENT, MZ_HUGE), MR_POISON | MR_STONE, 0,
        M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS | M1_BREATHLESS
            | M1_UNSOLID | M1_AMPHIBIOUS | M1_SWIM,
        M2_STRONG | M2_NEUTER, 0, CLR_BLUE),
    /*
     * Fungi
     */
    MON("地衣", S_FUNGUS, LVL(0, 1, 9, 0, 0), (G_GENO | 4),  //lichen
        A(ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(20, 200, MS_SILENT, MZ_SMALL), 0, 0,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BRIGHT_GREEN),
    MON("棕霉菌", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),  //brown mold
        A(ATTK(AT_NONE, AD_COLD, 0, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_COLD | MR_POISON,
        MR_COLD | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS
                                 | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("黄霉菌", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 2),  //yellow mold
        A(ATTK(AT_NONE, AD_STUN, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_POIS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_YELLOW),
    MON("绿霉菌", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),  //green mold
        A(ATTK(AT_NONE, AD_ACID, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_ACID | MR_STONE, MR_STONE,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_ACID | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("红霉菌", S_FUNGUS, LVL(1, 0, 9, 0, 0), (G_GENO | 1),  //red mold
        A(ATTK(AT_NONE, AD_FIRE, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 30, MS_SILENT, MZ_SMALL), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON, M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS
                                 | M1_NOHEAD | M1_MINDLESS | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, M3_INFRAVISIBLE, CLR_RED),
    MON("尖叫蕈", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 1),  //shrieker
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SHRIEK, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA),
    MON("紫真菌", S_FUNGUS, LVL(3, 1, 7, 0, 0), (G_GENO | 2),  //violet fungus
        A(ATTK(AT_TUCH, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_STCK, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_BREATHLESS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD | M1_MINDLESS
            | M1_NOTAKE,
        M2_HOSTILE | M2_NEUTER, 0, CLR_MAGENTA),
    /*
     * Gnomes
     */
    MON("侏儒", S_GNOME, LVL(1, 6, 10, 4, 0), (G_GENO | G_SGROUP | 1),  //gnome
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(650, 100, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GNOME | M2_COLLECT, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    MON("侏儒领主", S_GNOME, LVL(3, 8, 10, 4, 0), (G_GENO | 2),  //gnome lord
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("侏儒巫师", S_GNOME, LVL(3, 10, 4, 10, 0), (G_GENO | 1),  //gnomish wizard
        A(ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(700, 120, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_MAGIC, M3_INFRAVISIBLE | M3_INFRAVISION, HI_ZAP),
    MON("侏儒王", S_GNOME, LVL(5, 10, 10, 20, 0), (G_GENO | 1),  //gnome king
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(750, 150, MS_ORC, MZ_SMALL), 0, 0, M1_HUMANOID | M1_OMNIVORE,
        M2_GNOME | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
#ifdef SPLITMON_1
};
#endif
#endif /* !SPLITMON_2 */

/* horrible kludge alert:
 * This is a compiler-specific kludge to allow the compilation of monst.o in
 * two pieces, by defining first SPLITMON_1 and then SPLITMON_2. The
 * resulting assembler files (monst1.s and monst2.s) are then run through
 * sed to change local symbols, concatenated together, and assembled to
 * produce monst.o. THIS ONLY WORKS WITH THE ATARI GCC, and should only
 * be done if you don't have enough memory to compile monst.o the "normal"
 * way.  --ERS
 */

#ifndef SPLITMON_1
#ifdef SPLITMON_2
struct permonst _mons2[] = {
#endif
    /*
     * giant Humanoids
     */
    MON("巨人", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_NOGEN | 1),  //giant
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("石头巨人", S_GIANT, LVL(6, 6, 0, 0, 2), (G_GENO | G_SGROUP | 1),  //stone giant
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("希尔巨人", S_GIANT, LVL(8, 10, 6, 0, -2), (G_GENO | G_SGROUP | 1),  //hill giant
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2200, 700, MS_BOAST, MZ_HUGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT
            | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN),
    MON("火巨人", S_GIANT, LVL(9, 12, 4, 5, 2), (G_GENO | G_SGROUP | 1),  //fire giant
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_FIRE, MR_FIRE,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_YELLOW),
    MON("霜巨人", S_GIANT, LVL(10, 12, 3, 10, -3),  //frost giant
        (G_NOHELL | G_GENO | G_SGROUP | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_COLD, MR_COLD,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("双头巨人", S_GIANT, LVL(10, 12, 3, 0, 0), (G_GENO | 1),  //ettin
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_WEAP, AD_PHYS, 3, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 500, MS_GRUNT, MZ_HUGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("风暴巨人", S_GIANT, LVL(16, 12, 3, 10, -3),  //storm giant
        (G_GENO | G_SGROUP | 1), A(ATTK(AT_WEAP, AD_PHYS, 2, 12), NO_ATTK,
                                   NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2250, 750, MS_BOAST, MZ_HUGE), MR_ELEC, MR_ELEC,
        M1_HUMANOID | M1_CARNIVORE, M2_GIANT | M2_STRONG | M2_ROCKTHROW
                                        | M2_NASTY | M2_COLLECT | M2_JEWELS,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
    MON("提坦", S_GIANT, LVL(16, 18, -3, 70, 9), (1),  //titan
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2300, 900, MS_SPELL, MZ_HUGE), 0, 0,
        M1_FLY | M1_HUMANOID | M1_OMNIVORE,
        M2_STRONG | M2_ROCKTHROW | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_MAGENTA),
    MON("弥诺陶洛斯", S_GIANT, LVL(15, 15, 6, 0, 0), (G_GENO | G_NOGEN),  //minotaur
        A(ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10),
          ATTK(AT_BUTT, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 700, MS_SILENT, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    /* 'I' is a visual marker for all invisible monsters and must be unused */
    /*
     * Jabberwock
     */
    /* the illustration from _Through_the_Looking_Glass_
       depicts hands as well as wings */
    MON("颊脖龙", S_JABBERWOCK, LVL(15, 12, -2, 50, 0), (G_GENO | 1),  //jabberwock
        A(ATTK(AT_BITE, AD_PHYS, 2, 10), ATTK(AT_BITE, AD_PHYS, 2, 10),
          ATTK(AT_CLAW, AD_PHYS, 2, 10), ATTK(AT_CLAW, AD_PHYS, 2, 10),
          NO_ATTK, NO_ATTK),
        SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_FLY | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE,
        CLR_ORANGE),
#if 0 /* DEFERRED */
    MON("vorpal jabberwock", S_JABBERWOCK,
	LVL(20, 12, -2, 50, 0), (G_GENO | 1),
	A(ATTK(AT_BITE, AD_PHYS, 3, 10), ATTK(AT_BITE, AD_PHYS, 3, 10),
	  ATTK(AT_CLAW, AD_PHYS, 3, 10), ATTK(AT_CLAW, AD_PHYS, 3, 10),
	  NO_ATTK, NO_ATTK),
	SIZ(1300, 600, MS_BURBLE, MZ_LARGE), 0, 0,
	M1_ANIMAL | M1_FLY | M1_CARNIVORE,
	M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT, M3_INFRAVISIBLE,
        HI_LORD),
#endif
    /*
     * Kops
     */
    MON("吉斯通警察", S_KOP, LVL(1, 6, 10, 10, 9),  //Keystone Kop
        (G_GENO | G_LGROUP | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("警察中士", S_KOP, LVL(2, 8, 10, 10, 10),  //Kop Sergeant
        (G_GENO | G_SGROUP | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("警察中尉", S_KOP, LVL(3, 10, 10, 20, 11), (G_GENO | G_NOGEN),  //Kop Lieutenant
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_CYAN),
    MON("警察上尉", S_KOP, LVL(4, 12, 10, 20, 12), (G_GENO | G_NOGEN),  //Kop Kaptain
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_ARREST, MZ_HUMAN), 0, 0, M1_HUMANOID,
        M2_HUMAN | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_LORD),
    /*
     * Liches
     */
    MON("巫妖", S_LICH, LVL(11, 6, 0, 30, -9), (G_GENO | G_NOCORPSE | 1),  //lich
        A(ATTK(AT_TUCH, AD_COLD, 1, 10), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_BROWN),
    MON("半神巫妖", S_LICH, LVL(14, 9, -2, 60, -12),  //demilich
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 3, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        MR_COLD, M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_INFRAVISION, CLR_RED),
    MON("巫妖大师", S_LICH, LVL(17, 9, -4, 90, -15),  //master lich
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 3, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_POISON, MR_FIRE | MR_COLD,
        M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION,
        HI_LORD),
    MON("大巫妖", S_LICH, LVL(25, 9, -6, 90, -15),  //arch-lich
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_COLD, 5, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 100, MS_MUMBLE, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_ELEC | MR_POISON, MR_FIRE | MR_COLD,
        M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_HOSTILE | M2_MAGIC, M3_WANTSBOOK | M3_INFRAVISION,
        HI_LORD),
    /*
     * Mummies
     */
    MON("狗头人木乃伊", S_MUMMY, LVL(3, 8, 6, 20, -2),  //kobold mummy
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN),
    MON("侏儒木乃伊", S_MUMMY, LVL(4, 10, 6, 20, -3),  //gnome mummy
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_GNOME, M3_INFRAVISION, CLR_RED),
    MON("兽人木乃伊", S_MUMMY, LVL(5, 10, 5, 20, -4),  //orc mummy
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_ORC | M2_GREEDY | M2_JEWELS,
        M3_INFRAVISION, CLR_GRAY),
    MON("矮人木乃伊", S_MUMMY, LVL(5, 10, 5, 20, -4),  //dwarf mummy
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_DWARF | M2_GREEDY | M2_JEWELS,
        M3_INFRAVISION, CLR_RED),
    MON("精灵木乃伊", S_MUMMY, LVL(6, 12, 4, 30, -5),  //elf mummy
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_ELF, M3_INFRAVISION, CLR_GREEN),
    MON("人类木乃伊", S_MUMMY, LVL(6, 12, 4, 30, -5),  //human mummy
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE, M3_INFRAVISION, CLR_GRAY),
    MON("双头木乃伊", S_MUMMY, LVL(7, 12, 4, 30, -6),  //ettin mummy
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_BLUE),
    MON("巨人木乃伊", S_MUMMY, LVL(8, 14, 3, 30, -7),  //giant mummy
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_HOSTILE | M2_GIANT | M2_STRONG | M2_JEWELS,
        M3_INFRAVISION, CLR_CYAN),
    /*
     * Nagas
     */
    MON("红幼纳迦", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,  //red naga hatchling
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    MON("黑幼纳迦", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,  //black naga hatchling
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON | MR_ACID | MR_STONE,
        MR_POISON | MR_STONE, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_ACID
                                  | M1_NOTAKE | M1_CARNIVORE,
        M2_STRONG, 0, CLR_BLACK),
    MON("金幼纳迦", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,  //golden naga hatchling
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, HI_GOLD),
    MON("幼纳迦守卫", S_NAGA, LVL(3, 10, 6, 0, 0), G_GENO,  //guardian naga hatchling
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 100, MS_MUMBLE, MZ_LARGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, CLR_GREEN),
    MON("红纳迦", S_NAGA, LVL(6, 12, 4, 0, -4), (G_GENO | 1),  //red naga
        A(ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_BREA, AD_FIRE, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON, M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE
                                 | M1_OVIPAROUS | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    MON("黑纳迦", S_NAGA, LVL(8, 14, 2, 10, 4), (G_GENO | 1),  //black naga
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_SPIT, AD_ACID, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON | MR_ACID | MR_STONE,
        MR_POISON | MR_STONE,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_ACID
            | M1_NOTAKE | M1_CARNIVORE,
        M2_STRONG, 0, CLR_BLACK),
    MON("金纳迦", S_NAGA, LVL(10, 14, 2, 70, 5), (G_GENO | 1),  //golden naga
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), ATTK(AT_MAGC, AD_SPEL, 4, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_NOTAKE
            | M1_OMNIVORE,
        M2_STRONG, 0, HI_GOLD),
    MON("纳迦守卫", S_NAGA, LVL(12, 16, 0, 50, 7), (G_GENO | 1),  //guardian naga
        A(ATTK(AT_BITE, AD_PLYS, 1, 6), ATTK(AT_SPIT, AD_DRST, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2600, 400, MS_MUMBLE, MZ_HUGE), MR_POISON, MR_POISON,
        M1_NOLIMBS | M1_SLITHY | M1_THICK_HIDE | M1_OVIPAROUS | M1_POIS
            | M1_NOTAKE | M1_OMNIVORE,
        M2_STRONG, 0, CLR_GREEN),
    /*
     * Ogres
     */
    MON("食人魔", S_OGRE, LVL(5, 10, 5, 0, -3), (G_SGROUP | G_GENO | 1),  //ogre
        A(ATTK(AT_WEAP, AD_PHYS, 2, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1600, 500, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("食人魔领主", S_OGRE, LVL(7, 12, 3, 30, -5), (G_GENO | 2),  //ogre lord
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1700, 700, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_LORD | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("食人魔王", S_OGRE, LVL(9, 14, 4, 60, -7), (G_GENO | 2),  //ogre king
        A(ATTK(AT_WEAP, AD_PHYS, 3, 5), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1700, 750, MS_GRUNT, MZ_LARGE), 0, 0, M1_HUMANOID | M1_CARNIVORE,
        M2_STRONG | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Puddings
     *
     * must be in the same order as the pudding globs in objects.c
     */
    MON("灰泥怪", S_PUDDING, LVL(3, 1, 8, 0, 0), (G_GENO | G_NOCORPSE | 2),  //gray ooze
        A(ATTK(AT_BITE, AD_RUST, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 250, MS_SILENT, MZ_MEDIUM),
        MR_FIRE | MR_COLD | MR_POISON | MR_ACID | MR_STONE,
        MR_FIRE | MR_COLD | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GRAY),
    MON("棕色布丁", S_PUDDING, LVL(5, 3, 8, 0, 0),  //brown pudding
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_BITE, AD_DCAY, 0, 0), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(500, 250, MS_SILENT, MZ_MEDIUM),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE,
        MR_COLD | MR_ELEC | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BROWN),
    MON("绿色黏液", S_PUDDING, LVL(6, 6, 6, 0, 0),  //green slime
        (G_HELL | G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_TUCH, AD_SLIM, 1, 4), ATTK(AT_NONE, AD_SLIM, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 150, MS_SILENT, MZ_LARGE),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE, 0,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID | M1_POIS,
        M2_HOSTILE | M2_NEUTER, 0, CLR_GREEN),
    MON("黑色布丁", S_PUDDING, LVL(10, 6, 6, 0, 0),  //black pudding
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_BITE, AD_CORR, 3, 8), ATTK(AT_NONE, AD_CORR, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(900, 250, MS_SILENT, MZ_LARGE),
        MR_COLD | MR_ELEC | MR_POISON | MR_ACID | MR_STONE,
        MR_COLD | MR_ELEC | MR_POISON,
        M1_BREATHLESS | M1_AMORPHOUS | M1_NOEYES | M1_NOLIMBS | M1_NOHEAD
            | M1_MINDLESS | M1_OMNIVORE | M1_ACID,
        M2_HOSTILE | M2_NEUTER, 0, CLR_BLACK),
    /*
     * Quantum mechanics
     */
    MON("量子力学", S_QUANTMECH, LVL(7, 12, 3, 10, 0), (G_GENO | 3),  //quantum mechanic
        A(ATTK(AT_CLAW, AD_TLPT, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 20, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE | M1_POIS | M1_TPORT, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_CYAN),
    /*
     * Rust monster or disenchanter
     */
    MON("锈怪", S_RUSTMONST, LVL(5, 18, 2, 0, 0), (G_GENO | 2),  //rust monster
        A(ATTK(AT_TUCH, AD_RUST, 0, 0), ATTK(AT_TUCH, AD_RUST, 0, 0),
          ATTK(AT_NONE, AD_RUST, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 250, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_SWIM | M1_ANIMAL | M1_NOHANDS | M1_METALLIVORE, M2_HOSTILE,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("解魔怪", S_RUSTMONST, LVL(12, 12, -10, 0, -3),  //disenchanter
        (G_HELL | G_GENO | 2),
        A(ATTK(AT_CLAW, AD_ENCH, 4, 4), ATTK(AT_NONE, AD_ENCH, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(750, 200, MS_GROWL, MZ_LARGE), 0, 0, M1_ANIMAL | M1_CARNIVORE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BLUE),
    /*
     * Snakes
     */
    MON("束带蛇", S_SNAKE, LVL(1, 8, 8, 0, 0), (G_LGROUP | G_GENO | 1),  //garter snake
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(50, 60, MS_HISS, MZ_TINY), 0, 0,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY
            | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE,
        0, 0, CLR_GREEN),
    MON("蛇", S_SNAKE, LVL(4, 15, 3, 0, 0), (G_GENO | 2),  //snake
        A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_OVIPAROUS | M1_CARNIVORE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("水蝮蛇", S_SNAKE, LVL(4, 15, 3, 0, 0),  //water moccasin
        (G_GENO | G_NOGEN | G_LGROUP),
        A(ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(150, 80, MS_HISS, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_RED),
    MON("巨蟒", S_SNAKE, LVL(6, 3, 5, 0, 0), (G_GENO | 1),  //python
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), ATTK(AT_TUCH, AD_PHYS, 0, 0),
          ATTK(AT_HUGS, AD_WRAP, 1, 4), ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK),
        SIZ(250, 100, MS_HISS, MZ_LARGE), 0, 0,
        M1_SWIM | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_CARNIVORE
            | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE | M2_STRONG, M3_INFRAVISION, CLR_MAGENTA),
    MON("响尾蛇", S_SNAKE, LVL(6, 15, 2, 0, 0), (G_GENO | 1),  //pit viper
        A(ATTK(AT_BITE, AD_DRST, 1, 4), ATTK(AT_BITE, AD_DRST, 1, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 60, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISION, CLR_BLUE),
    MON("眼镜蛇", S_SNAKE, LVL(6, 18, 2, 0, 0), (G_GENO | 1),  //cobra
        A(ATTK(AT_BITE, AD_DRST, 2, 4), ATTK(AT_SPIT, AD_BLND, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(250, 100, MS_HISS, MZ_MEDIUM), MR_POISON, MR_POISON,
        M1_SWIM | M1_CONCEAL | M1_NOLIMBS | M1_ANIMAL | M1_SLITHY | M1_POIS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_BLUE),
    /*
     * Trolls
     */
    MON("巨魔", S_TROLL, LVL(7, 12, 4, 0, -3), (G_GENO | 2),  //troll
        A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 4, 2),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 350, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BROWN),
    MON("冰巨魔", S_TROLL, LVL(9, 10, 2, 20, -3), (G_NOHELL | G_GENO | 1),  //ice troll
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_COLD, 2, 6),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1000, 300, MS_GRUNT, MZ_LARGE), MR_COLD, MR_COLD,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_WHITE),
    MON("岩石巨魔", S_TROLL, LVL(9, 12, 0, 0, -3), (G_GENO | 1),  //rock troll
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 300, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_CYAN),
    MON("水巨魔", S_TROLL, LVL(11, 14, 4, 40, -3), (G_NOGEN | G_GENO),  //water troll
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 350, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE | M1_SWIM,
        M2_STRONG | M2_STALK | M2_HOSTILE, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BLUE),
    MON("欧罗海", S_TROLL, LVL(13, 12, -4, 0, -7), (G_GENO | 1),  //Olog-hai
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 2, 8),
          ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 400, MS_GRUNT, MZ_LARGE), 0, 0,
        M1_HUMANOID | M1_REGEN | M1_CARNIVORE,
        M2_STRONG | M2_STALK | M2_HOSTILE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /*
     * Umber hulk
     */
    MON("土巨怪", S_UMBER, LVL(9, 6, 2, 25, 0), (G_GENO | 2),  //umber hulk
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 5), ATTK(AT_GAZE, AD_CONF, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(1200, 500, MS_SILENT, MZ_LARGE), 0, 0, M1_TUNNEL | M1_CARNIVORE,
        M2_STRONG, M3_INFRAVISIBLE, CLR_BROWN),
    /*
     * Vampires
     */
    MON("吸血鬼", S_VAMPIRE, LVL(10, 12, 1, 25, -8),  //vampire
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_BITE, AD_DRLI, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, CLR_RED),
    MON("吸血鬼领主", S_VAMPIRE, LVL(12, 14, 0, 50, -9),  //vampire lord
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_BITE, AD_DRLI, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD
            | M2_MALE | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, CLR_BLUE),
#if 0 /* DEFERRED */
    MON("vampire mage", S_VAMPIRE,
	LVL(20, 14, -4, 50, -9), (G_GENO | G_NOCORPSE | 1),
	A(ATTK(AT_CLAW, AD_DRLI, 2, 8), ATTK(AT_BITE, AD_DRLI, 1, 8),
	  ATTK(AT_MAGC, AD_SPEL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
	M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
	M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_LORD
          | M2_MALE | M2_MAGIC | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, HI_ZAP),
#endif
    MON("穿刺者弗拉德", S_VAMPIRE, LVL(14, 18, -3, 80, -10),  //Vlad the Impaler
        (G_NOGEN | G_NOCORPSE | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 10), ATTK(AT_BITE, AD_DRLI, 1, 10),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_VAMPIRE, MZ_HUMAN), MR_SLEEP | MR_POISON, 0,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_POIS | M1_REGEN,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG
            | M2_NASTY | M2_PRINCE | M2_MALE | M2_SHAPESHIFTER,
        M3_WAITFORU | M3_WANTSCAND | M3_INFRAVISIBLE, HI_LORD),
    /*
     * Wraiths
     */
    MON("古墓尸妖", S_WRAITH, LVL(3, 12, 5, 5, -3),  //barrow wight
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_DRLI, 0, 0), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1200, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_COLLECT, 0, CLR_GRAY),
    MON("幽灵", S_WRAITH, LVL(6, 12, 4, 15, -6), (G_GENO | 2),  //wraith
        A(ATTK(AT_TUCH, AD_DRLI, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(0, 0, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_FLY | M1_HUMANOID | M1_UNSOLID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, 0, CLR_BLACK),
    MON("戒灵", S_WRAITH, LVL(13, 12, 0, 25, -17),  //Nazgul
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_DRLI, 1, 4), ATTK(AT_BREA, AD_SLEE, 2, 25),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_SPELL, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_HUMANOID,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_STRONG | M2_HOSTILE | M2_MALE
            | M2_COLLECT,
        0, HI_LORD),
    /*
     * Xorn
     */
    MON("索尔石怪", S_XORN, LVL(8, 9, -2, 20, 0), (G_GENO | 1),  //xorn
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_BITE, AD_PHYS, 4, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1200, 700, MS_ROAR, MZ_MEDIUM), MR_FIRE | MR_COLD | MR_STONE,
        MR_STONE,
        M1_BREATHLESS | M1_WALLWALK | M1_THICK_HIDE | M1_METALLIVORE,
        M2_HOSTILE | M2_STRONG, 0, CLR_BROWN),
    /*
     * Apelike beasts
     */
    MON("猴子", S_YETI, LVL(2, 12, 6, 0, 0), (G_GENO | 1),  //monkey
        A(ATTK(AT_CLAW, AD_SITM, 0, 0), ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(100, 50, MS_GROWL, MZ_SMALL), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, 0, M3_INFRAVISIBLE, CLR_GRAY),
    MON("猿", S_YETI, LVL(4, 12, 6, 0, 0), (G_GENO | G_SGROUP | 2),  //ape
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1100, 500, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_STRONG, M3_INFRAVISIBLE,
        CLR_BROWN),
    MON("枭熊", S_YETI, LVL(5, 12, 5, 0, 0), (G_GENO | 3),  //owlbear
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 700, MS_ROAR, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE,
        M2_HOSTILE | M2_STRONG | M2_NASTY, M3_INFRAVISIBLE, CLR_BROWN),
    MON("雪人", S_YETI, LVL(5, 15, 6, 0, 0), (G_GENO | 2),  //yeti
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1600, 700, MS_GROWL, MZ_LARGE), MR_COLD, MR_COLD,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("食肉猿", S_YETI, LVL(6, 12, 6, 0, 0), (G_GENO | 1),  //carnivorous ape
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_HUGS, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1250, 550, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_CARNIVORE, M2_HOSTILE | M2_STRONG,
        M3_INFRAVISIBLE, CLR_BLACK),
    MON("北美野人", S_YETI, LVL(7, 15, 6, 0, 2), (G_GENO | 1),  //sasquatch
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6),
          ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1550, 750, MS_GROWL, MZ_LARGE), 0, 0,
        M1_ANIMAL | M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE, M2_STRONG,
        M3_INFRAVISIBLE, CLR_GRAY),
    /*
     * Zombies
     */
    MON("狗头人僵尸", S_ZOMBIE, LVL(0, 6, 10, 0, -2),  //kobold zombie
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 4), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, CLR_BROWN),
    MON("侏儒僵尸", S_ZOMBIE, LVL(1, 6, 10, 0, -2),  //gnome zombie
        (G_GENO | G_NOCORPSE | 1), A(ATTK(AT_CLAW, AD_PHYS, 1, 5), NO_ATTK,
                                     NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(650, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GNOME, M3_INFRAVISION,
        CLR_BROWN),
    MON("兽人僵尸", S_ZOMBIE, LVL(2, 6, 9, 0, -3),  //orc zombie
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(850, 75, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ORC, M3_INFRAVISION, CLR_GRAY),
    MON("矮人僵尸", S_ZOMBIE, LVL(2, 6, 9, 0, -3),  //dwarf zombie
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 150, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_DWARF, M3_INFRAVISION,
        CLR_RED),
    MON("精灵僵尸", S_ZOMBIE, LVL(3, 6, 9, 0, -3),  //elf zombie
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 7), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 175, MS_SILENT, MZ_HUMAN), MR_COLD | MR_SLEEP | MR_POISON,
        0, M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_ELF, M3_INFRAVISION,
        CLR_GREEN),
    MON("人类僵尸", S_ZOMBIE, LVL(4, 6, 8, 0, -3),  //human zombie
        (G_GENO | G_SGROUP | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 200, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION, HI_DOMESTIC),
    MON("双头僵尸", S_ZOMBIE, LVL(6, 8, 6, 0, -4),  //ettin zombie
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 10), ATTK(AT_CLAW, AD_PHYS, 1, 10),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1700, 250, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_STRONG, M3_INFRAVISION,
        CLR_BLUE),
    MON("食尸鬼", S_ZOMBIE, LVL(3, 6, 10, 0, -2), (G_GENO | G_NOCORPSE | 1),  //ghoul
        A(ATTK(AT_CLAW, AD_PLYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 50, MS_SILENT, MZ_SMALL), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_POIS | M1_OMNIVORE,
        M2_UNDEAD | M2_WANDER | M2_HOSTILE, M3_INFRAVISION, CLR_BLACK),
    MON("巨人僵尸", S_ZOMBIE, LVL(8, 8, 6, 0, -4),  //giant zombie
        (G_GENO | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2050, 375, MS_SILENT, MZ_HUGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID,
        M2_UNDEAD | M2_STALK | M2_HOSTILE | M2_GIANT | M2_STRONG,
        M3_INFRAVISION, CLR_CYAN),
    MON("骷髅", S_ZOMBIE, LVL(12, 8, 4, 0, 0), (G_NOCORPSE | G_NOGEN),  //skeleton
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(300, 5, MS_BONES, MZ_HUMAN),
        MR_COLD | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_UNDEAD | M2_WANDER | M2_HOSTILE | M2_STRONG | M2_COLLECT
            | M2_NASTY,
        M3_INFRAVISION, CLR_WHITE),
    /*
     * golems
     */
    MON("稻草魔像", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1),  //straw golem
        A(ATTK(AT_CLAW, AD_PHYS, 1, 2), ATTK(AT_CLAW, AD_PHYS, 1, 2), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        CLR_YELLOW),
    MON("纸魔像", S_GOLEM, LVL(3, 12, 10, 0, 0), (G_NOCORPSE | 1),  //paper golem
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(400, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        HI_PAPER),
    MON("绳子魔像", S_GOLEM, LVL(4, 9, 8, 0, 0), (G_NOCORPSE | 1),  //rope golem
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_HUGS, AD_PHYS, 6, 1), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        CLR_BROWN),
    MON("金魔像", S_GOLEM, LVL(5, 9, 6, 0, 0), (G_NOCORPSE | 1),  //gold golem
        A(ATTK(AT_CLAW, AD_PHYS, 2, 3), ATTK(AT_CLAW, AD_PHYS, 2, 3), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(450, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_NEUTER, 0, HI_GOLD),
    MON("皮革魔像", S_GOLEM, LVL(6, 6, 6, 0, 0), (G_NOCORPSE | 1),  //leather golem
        A(ATTK(AT_CLAW, AD_PHYS, 1, 6), ATTK(AT_CLAW, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_NEUTER, 0,
        HI_LEATHER),
    MON("木魔像", S_GOLEM, LVL(7, 3, 4, 0, 0), (G_NOCORPSE | 1),  //wood golem
        A(ATTK(AT_CLAW, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(900, 0, MS_SILENT, MZ_LARGE), MR_COLD | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_NEUTER, 0, HI_WOOD),
    MON("肉魔像", S_GOLEM, LVL(9, 8, 9, 30, 0), (1),  //flesh golem
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1400, 600, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON,
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID, M2_HOSTILE | M2_STRONG, 0,
        CLR_RED),
    MON("土魔像", S_GOLEM, LVL(11, 7, 7, 40, 0), (G_NOCORPSE | 1),  //clay golem
        A(ATTK(AT_CLAW, AD_PHYS, 3, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1550, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_BROWN),
    MON("石魔像", S_GOLEM, LVL(14, 6, 5, 50, 0), (G_NOCORPSE | 1),  //stone golem
        A(ATTK(AT_CLAW, AD_PHYS, 3, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1900, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_GRAY),
    MON("玻璃魔像", S_GOLEM, LVL(16, 6, 1, 50, 0), (G_NOCORPSE | 1),  //glass golem
        A(ATTK(AT_CLAW, AD_PHYS, 2, 8), ATTK(AT_CLAW, AD_PHYS, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 0, MS_SILENT, MZ_LARGE), MR_SLEEP | MR_POISON | MR_ACID, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE,
        M2_HOSTILE | M2_STRONG, 0, CLR_CYAN),
    MON("铁魔像", S_GOLEM, LVL(18, 6, 3, 60, 0), (G_NOCORPSE | 1),  //iron golem
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_BREA, AD_DRST, 4, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2000, 0, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_BREATHLESS | M1_MINDLESS | M1_HUMANOID | M1_THICK_HIDE | M1_POIS,
        M2_HOSTILE | M2_STRONG | M2_COLLECT, 0, HI_METAL),
    /*
     * humans, including elves and were-critters
     */
    MON("人", S_HUMAN, LVL(0, 12, 10, 0, 0), G_NOGEN, /* for corpses */  //human
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("鼠人", S_HUMAN, LVL(2, 12, 10, 10, -7), (1),  //wererat
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BROWN),
    MON("豺狼人", S_HUMAN, LVL(2, 12, 10, 10, -7), (1),  //werejackal
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_RED),
    MON("狼人", S_HUMAN, LVL(5, 12, 10, 20, -7), (1),  //werewolf
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_WERE, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_REGEN | M1_OMNIVORE,
        M2_NOPOLY | M2_WERE | M2_HOSTILE | M2_HUMAN | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_ORANGE),
    MON("精灵", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN, /* for corpses */  //elf
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_ELF | M2_STRONG | M2_COLLECT,
        M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("伍德兰精灵", S_HUMAN, LVL(4, 12, 10, 10, -5),  //Woodland-elf
        (G_GENO | G_SGROUP | 2), A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
                                   NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GREEN),
    MON("绿精灵", S_HUMAN, LVL(5, 12, 10, 10, -6), (G_GENO | G_SGROUP | 2),  //Green-elf
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_GREEN),
    MON("灰精灵", S_HUMAN, LVL(6, 12, 10, 10, -7), (G_GENO | G_SGROUP | 2),  //Grey-elf
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS, M2_ELF | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("精灵领主", S_HUMAN, LVL(8, 12, 10, 20, -9), (G_GENO | G_SGROUP | 2),  //elf-lord
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_ELF | M2_STRONG | M2_LORD | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BRIGHT_BLUE),
    MON("精灵王", S_HUMAN, LVL(9, 12, 10, 25, -10), (G_GENO | 1),  //Elvenking
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_ELF, 350, MS_HUMANOID, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS,
        M2_ELF | M2_STRONG | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("变形怪", S_HUMAN, LVL(9, 12, 5, 20, 0), (G_GENO | 1),  //doppelganger
        A(ATTK(AT_WEAP, AD_PHYS, 1, 12), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_IMITATE, MZ_HUMAN), MR_SLEEP, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT
            | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("店主", S_HUMAN, LVL(12, 18, 0, 50, 0), G_NOGEN,  //shopkeeper
        A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SELL, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("警卫", S_HUMAN, LVL(12, 12, 10, 40, 10), G_NOGEN,  //guard
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_MERC | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    MON("囚犯", S_HUMAN, LVL(12, 12, 10, 0, 0),  //prisoner
        G_NOGEN, /* for special levels */
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE | M3_CLOSE, HI_DOMESTIC),
    MON("神谕", S_HUMAN, LVL(12, 0, 0, 50, 0), (G_NOGEN | G_UNIQ),  //Oracle
        A(ATTK(AT_NONE, AD_MAGM, 0, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_ORACLE, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_FEMALE, M3_INFRAVISIBLE,
        HI_ZAP),
    /* aligned priests always have the epri extension attached;
       individual instantiations should always have either ispriest
       or isminion set */
    MON("虔诚的牧师", S_HUMAN, LVL(12, 12, 10, 50, 0), G_NOGEN,  //aligned priest
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 1, 4),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN), MR_ELEC, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_LORD | M2_PEACEFUL | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_WHITE),
    /* high priests always have epri and always have ispriest set */
    MON("高级祭司", S_HUMAN, LVL(25, 15, 7, 70, 0), (G_NOGEN | G_UNIQ),  //high priest
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_PRIEST, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_MINION | M2_PRINCE | M2_NASTY | M2_COLLECT
            | M2_MAGIC,
        M3_INFRAVISIBLE, CLR_WHITE),
    MON("士兵", S_HUMAN, LVL(6, 10, 10, 0, -2), (G_SGROUP | G_GENO | 1),  //soldier
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("中士", S_HUMAN, LVL(8, 10, 10, 5, -3), (G_SGROUP | G_GENO | 1),  //sergeant
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_RED),
    MON("护士", S_HUMAN, LVL(11, 6, 0, 0, 0), (G_GENO | 3),  //nurse
        A(ATTK(AT_CLAW, AD_HEAL, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NURSE, MZ_HUMAN), MR_POISON, MR_POISON,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_HOSTILE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("中尉", S_HUMAN, LVL(10, 10, 10, 15, -4), (G_GENO | 1),  //lieutenant
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    MON("上尉", S_HUMAN, LVL(12, 10, 10, 15, -5), (G_GENO | 1),  //captain
        A(ATTK(AT_WEAP, AD_PHYS, 4, 4), ATTK(AT_WEAP, AD_PHYS, 4, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_BLUE),
    /* Keep these separate - some of the mkroom code assumes that
     * all the soldiers are contiguous.
     */
    MON("警卫员", S_HUMAN, LVL(6, 10, 10, 0, -2),  //watchman
        (G_SGROUP | G_NOGEN | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GRAY),
    MON("警卫员队长", S_HUMAN, LVL(10, 10, 10, 15, -4),  //watch captain
        (G_NOGEN | G_GENO | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_WEAP, AD_PHYS, 3, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SOLDIER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_MERC | M2_STALK
                                       | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_GREEN),
    /* Unique humans not tied to quests.
     */
    MON("美杜莎", S_HUMAN, LVL(20, 12, 2, 50, -15), (G_NOGEN | G_UNIQ),  //Medusa
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_GAZE, AD_STON, 0, 0), ATTK(AT_BITE, AD_DRST, 1, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HISS, MZ_LARGE), MR_POISON | MR_STONE,
        MR_POISON | MR_STONE, M1_FLY | M1_SWIM | M1_AMPHIBIOUS | M1_HUMANOID
                                  | M1_POIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG | M2_PNAME | M2_FEMALE,
        M3_WAITFORU | M3_INFRAVISIBLE, CLR_BRIGHT_GREEN),
    MON("岩德巫师", S_HUMAN, LVL(30, 12, -8, 100, A_NONE),  //Wizard of Yendor
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_SAMU, 2, 12), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_HUMAN), MR_FIRE | MR_POISON,
        MR_FIRE | MR_POISON,
        M1_FLY | M1_BREATHLESS | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS
            | M1_TPORT | M1_TPORT_CNTRL | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_PRINCE
            | M2_MALE | M2_MAGIC,
        M3_COVETOUS | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    MON("克罗伊斯", S_HUMAN, LVL(20, 15, 0, 40, 15), (G_UNIQ | G_NOGEN),  //Croesus
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARD, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_PNAME | M2_PRINCE | M2_MALE | M2_GREEDY | M2_JEWELS
            | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_LORD),
#ifdef CHARON
    MON("卡隆", S_HUMAN, LVL(76, 18, -5, 120, 0),  //Charon
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_TUCH, AD_PLYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_FERRY, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_POISON | MR_STONE, 0,
        M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_PNAME | M2_MALE | M2_GREEDY
            | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_WHITE),
#endif
    /*
     * ghosts
     */
    MON("鬼魂", S_GHOST, LVL(10, 3, -5, 50, -5), (G_NOCORPSE | G_NOGEN),  //ghost
        A(ATTK(AT_TUCH, AD_PHYS, 1, 1), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_SILENT, MZ_HUMAN),
        MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID,
        M2_NOPOLY | M2_UNDEAD | M2_STALK | M2_HOSTILE, M3_INFRAVISION,
        CLR_GRAY),
    MON("魂灵", S_GHOST, LVL(12, 10, 10, 0, 0), (G_NOCORPSE | G_NOGEN),  //shade
        A(ATTK(AT_TUCH, AD_PLYS, 2, 6), ATTK(AT_TUCH, AD_SLOW, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 0, MS_WAIL, MZ_HUMAN),
        MR_COLD | MR_DISINT | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_BREATHLESS | M1_WALLWALK | M1_HUMANOID | M1_UNSOLID
            | M1_SEE_INVIS,
        M2_NOPOLY | M2_UNDEAD | M2_WANDER | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISION, CLR_BLACK),
    /*
     * (major) demons
     */
    MON("水妖", S_DEMON, LVL(8, 12, -4, 30, -7),  //water demon
        (G_NOCORPSE | G_NOGEN),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_DJINNI, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS | M1_SWIM,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BLUE),
/* standard demons & devils
 */
#define SEDUCTION_ATTACKS_YES                                     \
    A(ATTK(AT_BITE, AD_SSEX, 0, 0), ATTK(AT_CLAW, AD_PHYS, 1, 3), \
      ATTK(AT_CLAW, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK)
#define SEDUCTION_ATTACKS_NO                                      \
    A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3), \
      ATTK(AT_BITE, AD_DRLI, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK)
    MON("魅魔", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1),  //succubus
        SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN),
        MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("有角的魔鬼", S_DEMON, LVL(6, 9, -5, 50, 11),  //horned devil
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 3), ATTK(AT_STNG, AD_PHYS, 1, 3), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_BROWN),
    MON("梦魇", S_DEMON, LVL(6, 12, 0, 70, -9), (G_NOCORPSE | 1),  //incubus
        SEDUCTION_ATTACKS_YES, SIZ(WT_HUMAN, 400, MS_SEDUCE, MZ_HUMAN),
        MR_FIRE | MR_POISON, 0, M1_HUMANOID | M1_FLY | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_MALE,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    /* Used by AD&D for a type of demon, originally one of the Furies */
    /* and spelled this way */
    MON("伊里逆丝", S_DEMON, LVL(7, 12, 2, 30, 10),  //erinys
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("哈玛魔", S_DEMON, LVL(8, 12, 0, 35, 8),  //barbed devil
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_STNG, AD_PHYS, 3, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_POIS | M1_THICK_HIDE, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("六臂蛇魔", S_DEMON, LVL(7, 12, -6, 80, -12),  //marilith
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_WEAP, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4)),
        SIZ(WT_HUMAN, 400, MS_CUSS, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_SLITHY | M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("弗洛魔", S_DEMON, LVL(8, 12, 0, 50, -9),  //vrock
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_CLAW, AD_PHYS, 1, 8),
          ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("狂战魔", S_DEMON, LVL(9, 6, -2, 55, -10),  //hezrou
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 3), ATTK(AT_CLAW, AD_PHYS, 1, 3),
          ATTK(AT_BITE, AD_PHYS, 4, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("骨魔", S_DEMON, LVL(9, 15, -1, 40, -9),  //bone devil
        (G_HELL | G_NOCORPSE | G_SGROUP | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 4), ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("冰魔", S_DEMON, LVL(11, 6, -4, 55, -12),  //ice devil
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_STNG, AD_COLD, 3, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE),
        MR_FIRE | MR_COLD | MR_POISON, 0, M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_WHITE),
    MON("判魂魔", S_DEMON, LVL(11, 9, -1, 65, -11),  //nalfeshnee
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_CLAW, AD_PHYS, 1, 4), ATTK(AT_CLAW, AD_PHYS, 1, 4),
          ATTK(AT_BITE, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SPELL, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_HUMANOID | M1_POIS, M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("深渊恶魔", S_DEMON, LVL(13, 6, -3, 65, -13),  //pit fiend
        (G_HELL | G_NOCORPSE | 2),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 2), ATTK(AT_WEAP, AD_PHYS, 4, 2),
          ATTK(AT_HUGS, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GROWL, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    MON("桑德斯廷", S_DEMON, LVL(13, 12, 4, 60, -5),  //sandestin
        (G_HELL | G_NOCORPSE | 1),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 400, MS_CUSS, MZ_HUMAN), MR_STONE, 0, M1_HUMANOID,
        M2_NOPOLY | M2_STALK | M2_STRONG | M2_COLLECT | M2_SHAPESHIFTER,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_GRAY),
    MON("炎魔", S_DEMON, LVL(16, 5, -2, 75, -14), (G_HELL | G_NOCORPSE | 1),  //balrog
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY | M2_COLLECT,
        M3_INFRAVISIBLE | M3_INFRAVISION, CLR_RED),
    /* Named demon lords & princes plus Arch-Devils.
     * (their order matters; see minion.c)
     */
    MON("朱比烈斯", S_DEMON, LVL(50, 3, -7, 65, -15),  //Juiblex
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_ENGL, AD_DISE, 4, 10), ATTK(AT_SPIT, AD_ACID, 3, 6),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 0, MS_GURGLE, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_ACID | MR_STONE, 0,
        M1_AMPHIBIOUS | M1_AMORPHOUS | M1_NOHEAD | M1_FLY | M1_SEE_INVIS
            | M1_ACID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_LORD | M2_MALE,
        M3_WAITFORU | M3_WANTSAMUL | M3_INFRAVISION, CLR_BRIGHT_GREEN),
    MON("伊诺胡", S_DEMON, LVL(56, 18, -5, 80, -15),  //Yeenoghu
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_WEAP, AD_CONF, 2, 8),
          ATTK(AT_CLAW, AD_PLYS, 1, 6), ATTK(AT_MAGC, AD_MAGM, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(900, 500, MS_ORC, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_LORD | M2_MALE | M2_COLLECT,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("奥迦斯", S_DEMON, LVL(66, 9, -6, 85, -20),  //Orcus
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 4),
          ATTK(AT_CLAW, AD_PHYS, 3, 4), ATTK(AT_MAGC, AD_SPEL, 8, 6),
          ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK),
        SIZ(1500, 500, MS_ORC, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_WAITFORU | M3_WANTSBOOK | M3_WANTSAMUL | M3_INFRAVISIBLE
            | M3_INFRAVISION,
        HI_LORD),
    MON("吉里昂", S_DEMON, LVL(72, 3, -3, 75, 15),  //Geryon
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 3, 6), ATTK(AT_CLAW, AD_PHYS, 3, 6),
          ATTK(AT_STNG, AD_DRST, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS | M1_SLITHY,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("迪斯帕特", S_DEMON, LVL(78, 15, -2, 80, 15),  //Dispater
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 4, 6), ATTK(AT_MAGC, AD_SPEL, 6, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUMAN), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS | M1_HUMANOID,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE | M2_COLLECT,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    MON("巴力西卜", S_DEMON, LVL(89, 9, -5, 85, 20),  //Baalzebub
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_BITE, AD_DRST, 2, 6), ATTK(AT_GAZE, AD_STUN, 2, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_LARGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION,
        HI_LORD),
    MON("阿斯莫德", S_DEMON, LVL(105, 12, -7, 90, 20),  //Asmodeus
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_CLAW, AD_PHYS, 4, 4), ATTK(AT_MAGC, AD_COLD, 6, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1500, 500, MS_BRIBE, MZ_HUGE), MR_FIRE | MR_COLD | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_HUMANOID | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG
            | M2_NASTY | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_WAITFORU | M3_INFRAVISIBLE | M3_INFRAVISION,
        HI_LORD),
    MON("魔神", S_DEMON, LVL(106, 15, -8, 95, -20),  //Demogorgon
        (G_HELL | G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_MAGC, AD_SPEL, 8, 6), ATTK(AT_STNG, AD_DRLI, 1, 4),
          ATTK(AT_CLAW, AD_DISE, 1, 6), ATTK(AT_CLAW, AD_DISE, 1, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1500, 500, MS_GROWL, MZ_HUGE), MR_FIRE | MR_POISON, 0,
        M1_FLY | M1_SEE_INVIS | M1_NOHANDS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_PNAME | M2_NASTY
            | M2_PRINCE | M2_MALE,
        M3_WANTSAMUL | M3_INFRAVISIBLE | M3_INFRAVISION, HI_LORD),
    /* Riders -- the Four Horsemen of the Apocalypse ("War" == player)
     */
    MON("死亡", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),  //Death
        A(ATTK(AT_TUCH, AD_DETH, 8, 8), ATTK(AT_TUCH, AD_DETH, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
    MON("瘟疫", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),  //Pestilence
        A(ATTK(AT_TUCH, AD_PEST, 8, 8), ATTK(AT_TUCH, AD_PEST, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
    MON("饥荒", S_DEMON, LVL(30, 12, -5, 100, 0), (G_UNIQ | G_NOGEN),  //Famine
        A(ATTK(AT_TUCH, AD_FAMN, 8, 8), ATTK(AT_TUCH, AD_FAMN, 8, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 1, MS_RIDER, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_HUMANOID | M1_REGEN | M1_SEE_INVIS | M1_TPORT_CNTRL,
        M2_NOPOLY | M2_STALK | M2_HOSTILE | M2_PNAME | M2_STRONG | M2_NASTY,
        M3_INFRAVISIBLE | M3_INFRAVISION | M3_DISPLACES, HI_LORD),
/* other demons
 */
#ifdef MAIL
    MON("mail daemon", S_DEMON, LVL(56, 24, 10, 127, 0),
        (G_NOGEN | G_NOCORPSE),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(600, 300, MS_SILENT, MZ_HUMAN),
        MR_FIRE | MR_COLD | MR_ELEC | MR_SLEEP | MR_POISON | MR_STONE, 0,
        M1_FLY | M1_SWIM | M1_BREATHLESS | M1_SEE_INVIS | M1_HUMANOID
            | M1_POIS,
        M2_NOPOLY | M2_STALK | M2_PEACEFUL, M3_INFRAVISIBLE | M3_INFRAVISION,
        CLR_BRIGHT_BLUE),
#endif
    MON("灯神", S_DEMON, LVL(7, 12, 4, 30, 0), (G_NOGEN | G_NOCORPSE),  //djinni
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(1500, 400, MS_DJINNI, MZ_HUMAN), MR_POISON | MR_STONE, 0,
        M1_HUMANOID | M1_FLY | M1_POIS, M2_NOPOLY | M2_STALK | M2_COLLECT,
        M3_INFRAVISIBLE, CLR_YELLOW),
    /*
     * sea monsters
     */
    MON("水母", S_EEL, LVL(3, 3, 6, 0, 0), (G_GENO | G_NOGEN),  //jellyfish
        A(ATTK(AT_STNG, AD_DRST, 3, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(80, 20, MS_SILENT, MZ_SMALL), MR_POISON, MR_POISON,
        M1_SWIM | M1_AMPHIBIOUS | M1_SLITHY | M1_NOLIMBS | M1_NOHEAD
            | M1_NOTAKE | M1_POIS,
        M2_HOSTILE, 0, CLR_BLUE),
    MON("水虎鱼", S_EEL, LVL(5, 12, 4, 0, 0), (G_GENO | G_NOGEN | G_SGROUP),  //piranha
        A(ATTK(AT_BITE, AD_PHYS, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(60, 30, MS_SILENT, MZ_SMALL), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_RED),
    MON("鲨鱼", S_EEL, LVL(7, 12, 2, 0, 0), (G_GENO | G_NOGEN),  //shark
        A(ATTK(AT_BITE, AD_PHYS, 5, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(500, 350, MS_SILENT, MZ_LARGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_THICK_HIDE | M1_NOTAKE,
        M2_HOSTILE, 0, CLR_GRAY),
    MON("巨型鳗鱼", S_EEL, LVL(5, 9, -1, 0, 0), (G_GENO | G_NOGEN),  //giant eel
        A(ATTK(AT_BITE, AD_PHYS, 3, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 250, MS_SILENT, MZ_HUGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_CYAN),
    MON("电鳗", S_EEL, LVL(7, 10, -3, 0, 0), (G_GENO | G_NOGEN),  //electric eel
        A(ATTK(AT_BITE, AD_ELEC, 4, 6), ATTK(AT_TUCH, AD_WRAP, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(200, 250, MS_SILENT, MZ_HUGE), MR_ELEC, MR_ELEC,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_SLITHY | M1_NOLIMBS
            | M1_CARNIVORE | M1_OVIPAROUS | M1_NOTAKE,
        M2_HOSTILE, M3_INFRAVISIBLE, CLR_BRIGHT_BLUE),
    MON("海妖", S_EEL, LVL(20, 3, 6, 0, -3), (G_GENO | G_NOGEN),  //kraken
        A(ATTK(AT_CLAW, AD_PHYS, 2, 4), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_HUGS, AD_WRAP, 2, 6), ATTK(AT_BITE, AD_PHYS, 5, 4), NO_ATTK,
          NO_ATTK),
        SIZ(1800, 1000, MS_SILENT, MZ_HUGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_STRONG, M3_INFRAVISIBLE, CLR_RED),
    /*
     * lizards, &c
     */
    MON("蝾螈", S_LIZARD, LVL(0, 6, 8, 0, 0), (G_GENO | 5),  //newt
        A(ATTK(AT_BITE, AD_PHYS, 1, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 20, MS_SILENT, MZ_TINY), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_YELLOW),
    MON("壁虎", S_LIZARD, LVL(1, 6, 8, 0, 0), (G_GENO | 5),  //gecko
        A(ATTK(AT_BITE, AD_PHYS, 1, 3), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 20, MS_SQEEK, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN),
    MON("鬣蜥", S_LIZARD, LVL(2, 6, 7, 0, 0), (G_GENO | 5),  //iguana
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(30, 30, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_BROWN),
    MON("幼鳄鱼", S_LIZARD, LVL(3, 6, 7, 0, 0), G_GENO,  //baby crocodile
        A(ATTK(AT_BITE, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(200, 200, MS_SILENT, MZ_MEDIUM), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_HOSTILE, 0, CLR_BROWN),
    MON("蜥蜴", S_LIZARD, LVL(5, 6, 6, 10, 0), (G_GENO | 5),  //lizard
        A(ATTK(AT_BITE, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(10, 40, MS_SILENT, MZ_TINY), MR_STONE, MR_STONE,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE, M2_HOSTILE, 0, CLR_GREEN),
    MON("变色龙", S_LIZARD, LVL(6, 5, 6, 10, 0), (G_GENO | 2),  //chameleon
        A(ATTK(AT_BITE, AD_PHYS, 4, 2), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(100, 100, MS_SILENT, MZ_TINY), 0, 0,
        M1_ANIMAL | M1_NOHANDS | M1_CARNIVORE,
        M2_NOPOLY | M2_HOSTILE | M2_SHAPESHIFTER, 0, CLR_BROWN),
    MON("鳄鱼", S_LIZARD, LVL(6, 9, 5, 0, 0), (G_GENO | 1),  //crocodile
        A(ATTK(AT_BITE, AD_PHYS, 4, 2), ATTK(AT_CLAW, AD_PHYS, 1, 12),
          NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_SILENT, MZ_LARGE), 0, 0,
        M1_SWIM | M1_AMPHIBIOUS | M1_ANIMAL | M1_THICK_HIDE | M1_NOHANDS
            | M1_OVIPAROUS | M1_CARNIVORE,
        M2_STRONG | M2_HOSTILE, 0, CLR_BROWN),
    MON("火蜥蜴", S_LIZARD, LVL(8, 12, -1, 0, -9), (G_HELL | 1),  //salamander
        A(ATTK(AT_WEAP, AD_PHYS, 2, 8), ATTK(AT_TUCH, AD_FIRE, 1, 6),
          ATTK(AT_HUGS, AD_PHYS, 2, 6), ATTK(AT_HUGS, AD_FIRE, 3, 6), NO_ATTK,
          NO_ATTK),
        SIZ(1500, 400, MS_MUMBLE, MZ_HUMAN), MR_SLEEP | MR_FIRE, MR_FIRE,
        M1_HUMANOID | M1_SLITHY | M1_THICK_HIDE | M1_POIS,
        M2_STALK | M2_HOSTILE | M2_COLLECT | M2_MAGIC, M3_INFRAVISIBLE,
        CLR_ORANGE),

    /*
     * dummy monster needed for visual interface
     */
    /* (marking it unique prevents figurines)
     */
    MON("长蠕虫尾", S_WORM_TAIL, LVL(0, 0, 0, 0, 0),  //long worm tail
        (G_NOGEN | G_NOCORPSE | G_UNIQ),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, 0, 0), 0, 0, 0L, M2_NOPOLY, 0, CLR_BROWN),

    /* Note:
     * Worm tail must be between the normal monsters and the special
     * quest & pseudo-character ones because an optimization in the
     * random monster selection code assumes everything beyond here
     * has the G_NOGEN and M2_NOPOLY attributes.
     */

    /*
     * character classes
     */
    MON("考古学家", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,  //archeologist
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_TUNNEL | M1_NEEDPICK | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("野蛮人", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,  //barbarian
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("穴居人", S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN,  //caveman
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("女穴居人", S_HUMAN, LVL(10, 12, 10, 0, 1), G_NOGEN,  //cavewoman
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("医生", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,  //healer
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("骑士", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,  //knight
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("僧侣", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,  //monk
        A(ATTK(AT_CLAW, AD_PHYS, 1, 8), ATTK(AT_KICK, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MALE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("牧师", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,  //priest
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("女牧师", S_HUMAN, LVL(10, 12, 10, 2, 0), G_NOGEN,  //priestess
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("游侠", S_HUMAN, LVL(10, 12, 10, 2, -3), G_NOGEN,  //ranger
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("盗贼", S_HUMAN, LVL(10, 12, 10, 1, -3), G_NOGEN,  //rogue
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_GREEDY | M2_JEWELS | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("武士", S_HUMAN, LVL(10, 12, 10, 1, 3), G_NOGEN,  //samurai
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("游客", S_HUMAN, LVL(10, 12, 10, 1, 0), G_NOGEN,  //tourist
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT, M3_INFRAVISIBLE,
        HI_DOMESTIC),
    MON("女武神", S_HUMAN, LVL(10, 12, 10, 1, -1), G_NOGEN,  //valkyrie
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), MR_COLD, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_FEMALE | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("巫师", S_HUMAN, LVL(10, 12, 10, 3, 0), G_NOGEN,  //wizard
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    /*
     * quest leaders
     */
    MON("卡那封勋爵", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),  //Lord Carnarvon
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("珀利阿斯", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ),  //Pelias
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("萨满卡诺夫", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),  //Shaman Karnov
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
#if 0 /* OBSOLETE */
	/* Two for elves - one of each sex.
	 */
    MON("Earendil", S_HUMAN,
	LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ),
	A(ATTK(AT_WEAP, AD_PHYS, 1, 8),
	  NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
	M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
	M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG
          | M2_MALE | M2_COLLECT | M2_MAGIC,
	M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
    MON("Elwing", S_HUMAN,
	LVL(20, 12, 0, 50, -20), (G_NOGEN | G_UNIQ),
	A(ATTK(AT_WEAP, AD_PHYS, 1, 8),
	  NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(WT_ELF, 350, MS_LEADER, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
	M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
	M2_NOPOLY | M2_ELF | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG
          | M2_FEMALE | M2_COLLECT | M2_MAGIC,
	M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
#endif
    MON("希波克拉底", S_HUMAN, LVL(20, 12, 0, 40, 0), (G_NOGEN | G_UNIQ),  //Hippocrates
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("亚瑟王", S_HUMAN, LVL(20, 12, 0, 40, 20), (G_NOGEN | G_UNIQ),  //King Arthur
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("宗师", S_HUMAN, LVL(25, 12, 0, 70, 0), (G_NOGEN | G_UNIQ),  //Grand Master
        A(ATTK(AT_CLAW, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_NASTY
            | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_BLACK),
    MON("大祭司", S_HUMAN, LVL(25, 12, 7, 70, 0), (G_NOGEN | G_UNIQ),  //Arch Priest
        A(ATTK(AT_WEAP, AD_PHYS, 4, 10), ATTK(AT_KICK, AD_PHYS, 2, 8),
          ATTK(AT_MAGC, AD_CLRC, 2, 8), ATTK(AT_MAGC, AD_CLRC, 2, 8), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN),
        MR_FIRE | MR_ELEC | MR_SLEEP | MR_POISON, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_COLLECT
            | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_WHITE),
    MON("俄里翁", S_HUMAN, LVL(20, 12, 0, 30, 0), (G_NOGEN | G_UNIQ),  //Orion
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(2200, 700, MS_LEADER, MZ_HUGE), 0, 0,
        M1_HUMANOID | M1_OMNIVORE | M1_SEE_INVIS | M1_SWIM | M1_AMPHIBIOUS,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISION | M3_INFRAVISIBLE, HI_LORD),
    /* Note: Master of Thieves is also the Tourist's nemesis.
     */
    MON("盗贼大师", S_HUMAN, LVL(20, 12, 0, 30, -20),  //Master of Thieves
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_MALE | M2_GREEDY
            | M2_JEWELS | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("萨托领主", S_HUMAN, LVL(20, 12, 0, 30, 20), (G_NOGEN | G_UNIQ),  //Lord Sato
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("双花", S_HUMAN, LVL(20, 12, 10, 20, 0), (G_NOGEN | G_UNIQ),  //Twoflower
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_PEACEFUL | M2_STRONG | M2_MALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("诺恩", S_HUMAN, LVL(20, 12, 0, 80, 0), (G_NOGEN | G_UNIQ),  //Norn
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1800, 550, MS_LEADER, MZ_HUGE), MR_COLD, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_FEMALE
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, HI_LORD),
    MON("绿衣娜菲利特", S_HUMAN, LVL(20, 12, 0, 60, 0),  //Neferet the Green
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 2, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_LEADER, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_FEMALE | M2_PNAME | M2_PEACEFUL | M2_STRONG
            | M2_COLLECT | M2_MAGIC,
        M3_CLOSE | M3_INFRAVISIBLE, CLR_GREEN),
    /*
     * quest nemeses
     */
    MON("修堤库特里的奴才", S_DEMON, LVL(16, 12, -2, 75, -14),  //Minion of Huhetotl
        (G_NOCORPSE | G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_STALK | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_RED),
    MON("图特阿蒙", S_HUMAN, LVL(16, 12, 0, 10, -14),  //Thoth Amon
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_STRONG | M2_MALE | M2_STALK
            | M2_HOSTILE | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /* Multi-headed, possessing the breath attacks of all the other dragons
     * (selected at random when attacking).
     */
    MON("彩色龙", S_DRAGON, LVL(16, 12, 0, 30, -14),  //Chromatic Dragon
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_BREA, AD_RBRE, 6, 8), ATTK(AT_MAGC, AD_SPEL, 0, 0),
          ATTK(AT_CLAW, AD_SAMU, 2, 8), ATTK(AT_BITE, AD_PHYS, 4, 8),
          ATTK(AT_BITE, AD_PHYS, 4, 8), ATTK(AT_STNG, AD_PHYS, 1, 6)),
        SIZ(WT_DRAGON, 1700, MS_NEMESIS, MZ_GIGANTIC),
        MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON
            | MR_ACID | MR_STONE,
        MR_FIRE | MR_COLD | MR_SLEEP | MR_DISINT | MR_ELEC | MR_POISON
            | MR_STONE,
        M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_HOSTILE | M2_FEMALE | M2_STALK | M2_STRONG | M2_NASTY
            | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
#if 0 /* OBSOLETE */
    MON("Goblin King", S_ORC,
	LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ),
	A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
	  ATTK(AT_CLAW, AD_SAMU, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), 0, 0,
	M1_HUMANOID | M1_OMNIVORE,
	M2_NOPOLY | M2_ORC | M2_HOSTILE | M2_STRONG | M2_STALK | M2_NASTY
          | M2_MALE | M2_GREEDY | M2_JEWELS | M2_COLLECT | M2_MAGIC,
	M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        HI_LORD),
#endif
    MON("独眼巨人", S_GIANT, LVL(18, 12, 0, 0, -15), (G_NOGEN | G_UNIQ),  //Cyclops
        A(ATTK(AT_WEAP, AD_PHYS, 4, 8), ATTK(AT_WEAP, AD_PHYS, 4, 8),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(1900, 700, MS_NEMESIS, MZ_HUGE), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GIANT | M2_STRONG | M2_ROCKTHROW | M2_STALK
            | M2_HOSTILE | M2_NASTY | M2_MALE | M2_JEWELS | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_GRAY),
    MON("恶龙", S_DRAGON, LVL(15, 12, -1, 20, -14), (G_NOGEN | G_UNIQ),  //Ixoth
        A(ATTK(AT_BREA, AD_FIRE, 8, 6), ATTK(AT_BITE, AD_PHYS, 4, 8),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_PHYS, 2, 4),
          ATTK(AT_CLAW, AD_SAMU, 2, 4), NO_ATTK),
        SIZ(WT_DRAGON, 1600, MS_NEMESIS, MZ_GIGANTIC), MR_FIRE | MR_STONE,
        MR_FIRE,
        M1_FLY | M1_THICK_HIDE | M1_NOHANDS | M1_CARNIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_NASTY
            | M2_STALK | M2_GREEDY | M2_JEWELS | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_RED),
    MON("凯恩大师", S_HUMAN, LVL(25, 12, -10, 10, -20), (G_NOGEN | G_UNIQ),  //Master Kaen
        A(ATTK(AT_CLAW, AD_PHYS, 16, 2), ATTK(AT_CLAW, AD_PHYS, 16, 2),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), ATTK(AT_CLAW, AD_SAMU, 1, 4), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE,
        MR_POISON, M1_HUMANOID | M1_HERBIVORE | M1_SEE_INVIS,
        M2_NOPOLY | M2_HUMAN | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG
            | M2_NASTY | M2_STALK | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    MON("纳宗魔", S_DEMON, LVL(16, 12, -2, 85, -127),  //Nalzok
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 8, 4), ATTK(AT_WEAP, AD_PHYS, 4, 6),
          ATTK(AT_MAGC, AD_SPEL, 0, 0), ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_LARGE),
        MR_FIRE | MR_POISON | MR_STONE, 0, M1_FLY | M1_SEE_INVIS | M1_POIS,
        M2_NOPOLY | M2_DEMON | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG
            | M2_STALK | M2_NASTY | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        CLR_RED),
    MON("蝎弩", S_SPIDER, LVL(15, 12, 10, 0, -15), (G_NOGEN | G_UNIQ),  //Scorpius
        A(ATTK(AT_CLAW, AD_PHYS, 2, 6), ATTK(AT_CLAW, AD_SAMU, 2, 6),
          ATTK(AT_STNG, AD_DISE, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(750, 350, MS_NEMESIS, MZ_HUMAN), MR_POISON | MR_STONE, MR_POISON,
        M1_ANIMAL | M1_NOHANDS | M1_OVIPAROUS | M1_POIS | M1_CARNIVORE,
        M2_NOPOLY | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK
            | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU, HI_LORD),
    MON("刺客大师", S_HUMAN, LVL(15, 12, 0, 30, 18),  //Master Assassin
        (G_NOGEN | G_UNIQ),
        A(ATTK(AT_WEAP, AD_DRST, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 8),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_MALE | M2_HOSTILE | M2_STALK
            | M2_NASTY | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /* A renegade daimyo who led a 13 year civil war against the shogun
     * of his time.
     */
    MON("足利尊氏", S_HUMAN, LVL(15, 12, 0, 40, -13),  //Ashikaga Takauji
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 2, 6), ATTK(AT_WEAP, AD_PHYS, 2, 6),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PNAME | M2_HOSTILE | M2_STRONG | M2_STALK
            | M2_NASTY | M2_MALE | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, HI_LORD),
    /*
     * Note: the Master of Thieves was defined above.
     */
    MON("叙尔特领主", S_GIANT, LVL(15, 12, 2, 50, 12), (G_NOGEN | G_UNIQ),  //Lord Surtur
        A(ATTK(AT_WEAP, AD_PHYS, 2, 10), ATTK(AT_WEAP, AD_PHYS, 2, 10),
          ATTK(AT_CLAW, AD_SAMU, 2, 6), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(2250, 850, MS_NEMESIS, MZ_HUGE), MR_FIRE | MR_STONE, MR_FIRE,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_GIANT | M2_MALE | M2_PNAME | M2_HOSTILE | M2_STALK
            | M2_STRONG | M2_NASTY | M2_ROCKTHROW | M2_JEWELS | M2_COLLECT,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISION | M3_INFRAVISIBLE,
        HI_LORD),
    MON("黑暗魔君", S_HUMAN, LVL(15, 12, 0, 80, -10),  //Dark One
        (G_NOGEN | G_UNIQ | G_NOCORPSE),
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6),
          ATTK(AT_CLAW, AD_SAMU, 1, 4), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_NEMESIS, MZ_HUMAN), MR_STONE, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_STRONG | M2_HOSTILE | M2_STALK | M2_NASTY
            | M2_COLLECT | M2_MAGIC,
        M3_WANTSARTI | M3_WAITFORU | M3_INFRAVISIBLE, CLR_BLACK),
    /*
     * quest "guardians"
     */
    MON("学者", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,  //student
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_TUNNEL | M1_NEEDPICK | M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("酋长", S_HUMAN, LVL(5, 12, 10, 10, 0), G_NOGEN,  //chieftain
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("尼安德特人", S_HUMAN, LVL(5, 12, 10, 10, 1), G_NOGEN,  //neanderthal
        A(ATTK(AT_WEAP, AD_PHYS, 2, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
#if 0 /* OBSOLETE */
    MON("High-elf", S_HUMAN,
	LVL(5, 12, 10, 10, -7), G_NOGEN,
	A(ATTK(AT_WEAP, AD_PHYS, 2, 4), ATTK(AT_MAGC, AD_CLRC, 0, 0),
	  NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
	SIZ(WT_ELF, 350, MS_GUARDIAN, MZ_HUMAN), MR_SLEEP, MR_SLEEP,
	M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
	M2_NOPOLY | M2_ELF | M2_PEACEFUL | M2_COLLECT,
	M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
#endif
    MON("护理者", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,  //attendant
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), MR_POISON, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("实习骑士", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,  //page
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("方丈", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,  //abbot
        A(ATTK(AT_CLAW, AD_PHYS, 8, 2), ATTK(AT_KICK, AD_STUN, 3, 2),
          ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_HERBIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("侍祭", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,  //acolyte
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_CLRC, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("猎人", S_HUMAN, LVL(5, 12, 10, 10, -7), G_NOGEN,  //hunter
        A(ATTK(AT_WEAP, AD_PHYS, 1, 4), NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK,
          NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_SEE_INVIS | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISION | M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("刺客", S_HUMAN, LVL(5, 12, 10, 10, -3), G_NOGEN,  //thug
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_WEAP, AD_PHYS, 1, 6), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_GREEDY | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("忍者", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,  //ninja
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_HUMANOID, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_HOSTILE | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("禅师", S_HUMAN, LVL(5, 12, 10, 10, 3), G_NOGEN,  //roshi
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE,
        M2_NOPOLY | M2_HUMAN | M2_PEACEFUL | M2_STRONG | M2_COLLECT,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("导游", S_HUMAN, LVL(5, 12, 10, 20, 0), G_NOGEN,  //guide
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("战士", S_HUMAN, LVL(5, 12, 10, 10, -1), G_NOGEN,  //warrior
        A(ATTK(AT_WEAP, AD_PHYS, 1, 8), ATTK(AT_WEAP, AD_PHYS, 1, 8), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_FEMALE,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    MON("魔法学徒", S_HUMAN, LVL(5, 12, 10, 30, 0), G_NOGEN,  //apprentice
        A(ATTK(AT_WEAP, AD_PHYS, 1, 6), ATTK(AT_MAGC, AD_SPEL, 0, 0), NO_ATTK,
          NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(WT_HUMAN, 400, MS_GUARDIAN, MZ_HUMAN), 0, 0,
        M1_HUMANOID | M1_OMNIVORE, M2_NOPOLY | M2_HUMAN | M2_PEACEFUL
                                       | M2_STRONG | M2_COLLECT | M2_MAGIC,
        M3_INFRAVISIBLE, HI_DOMESTIC),
    /*
     * array terminator
     */
    MON("", 0, LVL(0, 0, 0, 0, 0), (0),
        A(NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK, NO_ATTK),
        SIZ(0, 0, 0, 0), 0, 0, 0L, 0L, 0, 0)
};
#endif /* !SPLITMON_1 */

#ifndef SPLITMON_1
/* dummy routine used to force linkage */
void
monst_init()
{
    return;
}

struct attack sa_yes[NATTK] = SEDUCTION_ATTACKS_YES;
struct attack sa_no[NATTK] = SEDUCTION_ATTACKS_NO;

#endif

/*monst.c*/
