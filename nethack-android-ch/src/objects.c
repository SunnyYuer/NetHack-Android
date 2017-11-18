/* NetHack 3.6	objects.c	$NHDT-Date: 1447313395 2015/11/12 07:29:55 $  $NHDT-Branch: master $:$NHDT-Revision: 1.49 $ */
/* Copyright (c) Mike Threepoint, 1989.                           */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The data in this file is processed twice, to construct two arrays.
 * On the first pass, only object name and object description matter.
 * On the second pass, all object-class fields except those two matter.
 * 2nd pass is a recursive inclusion of this file, not a 2nd compilation.
 * The name/description array is also used by makedefs and lev_comp.
 *
 * #ifndef OBJECTS_PASS_2_
 * # define OBJECT(name,desc,foo,bar,glorkum) name,desc
 * struct objdescr obj_descr[] =
 * #else
 * # define OBJECT(name,desc,foo,bar,glorkum) foo,bar,glorkum
 * struct objclass objects[] =
 * #endif
 * {
 *   { OBJECT("strange object",NULL, 1,2,3) },
 *   { OBJECT("arrow","pointy stick", 4,5,6) },
 *   ...
 *   { OBJECT(NULL,NULL, 0,0,0) }
 * };
 * #define OBJECTS_PASS_2_
 * #include "objects.c"
 */

/* *INDENT-OFF* */
/* clang-format off */

#ifndef OBJECTS_PASS_2_
/* first pass */
struct monst { struct monst *dummy; };  /* lint: struct obj's union */
#include "config.h"
#include "obj.h"
#include "objclass.h"
#include "prop.h"
#include "skills.h"

#else /* !OBJECTS_PASS_2_ */
/* second pass */
#include "color.h"
#define COLOR_FIELD(X) X,
#endif /* !OBJECTS_PASS_2_ */

/* objects have symbols: ) [ = " ( % ! ? + / $ * ` 0 _ . */

/*
 *      Note:  OBJ() and BITS() macros are used to avoid exceeding argument
 *      limits imposed by some compilers.  The ctnr field of BITS currently
 *      does not map into struct objclass, and is ignored in the expansion.
 *      The 0 in the expansion corresponds to oc_pre_discovered, which is
 *      set at run-time during role-specific character initialization.
 */

#ifndef OBJECTS_PASS_2_
/* first pass -- object descriptive text */
#define OBJ(ename,name,desc)  ename, name, desc
#define OBJECT(obj,bits,prp,sym,prob,dly,wt, \
               cost,sdam,ldam,oc1,oc2,nut,color)  { obj }
#define None (char *) 0 /* less visual distraction for 'no description' */

NEARDATA struct objdescr obj_descr[] =
#else
/* second pass -- object definitions */
#define BITS(nmkn,mrg,uskn,ctnr,mgc,chrg,uniq,nwsh,big,tuf,dir,sub,mtrl) \
  nmkn,mrg,uskn,0,mgc,chrg,uniq,nwsh,big,tuf,dir,mtrl,sub /*SCO cpp fodder*/
#define OBJECT(obj,bits,prp,sym,prob,dly,wt,cost,sdam,ldam,oc1,oc2,nut,color) \
  { 0, 0, (char *) 0, bits, prp, sym, dly, COLOR_FIELD(color) prob, wt, \
    cost, sdam, ldam, oc1, oc2, nut }
#ifndef lint
#define HARDGEM(n) (n >= 8)
#else
#define HARDGEM(n) (0)
#endif

NEARDATA struct objclass objects[] =
#endif
{
/* dummy object[0] -- description [2nd arg] *must* be NULL */
OBJECT(OBJ("strange object", "奇怪的东西", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0),
       0, ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),

/* weapons ... */
#define WEAPON(ename,name,desc,kn,mg,bi,prob,wt,                \
               cost,sdam,ldam,hitbon,typ,sub,metal,color) \
    OBJECT(OBJ(ename,name,desc),                                          \
           BITS(kn, mg, 1, 0, 0, 1, 0, 0, bi, 0, typ, sub, metal),  \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define PROJECTILE(ename,name,desc,kn,prob,wt,                  \
                   cost,sdam,ldam,hitbon,metal,sub,color) \
    OBJECT(OBJ(ename,name,desc),                                          \
           BITS(kn, 1, 1, 0, 0, 1, 0, 0, 0, 0, PIERCE, sub, metal), \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define BOW(ename,name,desc,kn,prob,wt,cost,hitbon,metal,sub,color) \
    OBJECT(OBJ(ename,name,desc),                                          \
           BITS(kn, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, sub, metal),      \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, 2, 2, hitbon, 0, wt, color)

/* Note: for weapons that don't do an even die of damage (ex. 2-7 or 3-18)
   the extra damage is added on in weapon.c, not here! */

#define P PIERCE
#define S SLASH
#define B WHACK

/* missiles; materiel reflects the arrowhead, not the shaft */
PROJECTILE("arrow", "箭", None,
           1, 55, 1, 2, 6, 6, 0,        IRON, -P_BOW, HI_METAL),
PROJECTILE("elven arrow", "精灵箭", "符文箭",  //"runed arrow"
           0, 20, 1, 2, 7, 6, 0,        WOOD, -P_BOW, HI_WOOD),
PROJECTILE("orcish arrow", "兽人箭", "粗糙箭",  //"crude arrow"
           0, 20, 1, 2, 5, 6, 0,        IRON, -P_BOW, CLR_BLACK),
PROJECTILE("silver arrow", "银箭", None,
           1, 12, 1, 5, 6, 6, 0,        SILVER, -P_BOW, HI_SILVER),
PROJECTILE("ya", "矢", "竹箭",  //"bamboo arrow"
           0, 15, 1, 4, 7, 7, 1,        METAL, -P_BOW, HI_METAL),
PROJECTILE("crossbow bolt", "弩箭", None,
           1, 55, 1, 2, 4, 6, 0,        IRON, -P_CROSSBOW, HI_METAL),

/* missiles that don't use a launcher */
WEAPON("dart", "飞镖", None,
       1, 1, 0, 60,   1,   2,  3,  2, 0, P,   -P_DART, IRON, HI_METAL),
WEAPON("shuriken", "手里剑", "投掷镖",  //"throwing star"
       0, 1, 0, 35,   1,   5,  8,  6, 2, P,   -P_SHURIKEN, IRON, HI_METAL),
WEAPON("boomerang", "回飞镖", None,
       1, 1, 0, 15,   5,  20,  9,  9, 0, 0,   -P_BOOMERANG, WOOD, HI_WOOD),

/* spears [note: javelin used to have a separate skill from spears,
   because the latter are primarily stabbing weapons rather than
   throwing ones; but for playability, they've been merged together
   under spear skill and spears can now be thrown like javelins] */
WEAPON("spear", "矛", None,
       1, 1, 0, 50,  30,   3,  6,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("elven spear", "精灵矛", "符文矛",  //"runed spear"
       0, 1, 0, 10,  30,   3,  7,  8, 0, P,   P_SPEAR, WOOD, HI_WOOD),
WEAPON("orcish spear", "兽人矛", "粗糙矛",  //"crude spear"
       0, 1, 0, 13,  30,   3,  5,  8, 0, P,   P_SPEAR, IRON, CLR_BLACK),
WEAPON("dwarvish spear", "矮人矛", "结实矛",  //"stout spear"
       0, 1, 0, 12,  35,   3,  8,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("silver spear", "银矛", None,
       1, 1, 0,  2,  36,  40,  6,  8, 0, P,   P_SPEAR, SILVER, HI_SILVER),
WEAPON("javelin", "标枪", "投掷矛",  //"throwing spear"
       0, 1, 0, 10,  20,   3,  6,  6, 0, P,   P_SPEAR, IRON, HI_METAL),

/* spearish; doesn't stack, not intended to be thrown */
WEAPON("trident", "三叉矛", None,
       1, 0, 0,  8,  25,   5,  6,  4, 0, P,   P_TRIDENT, IRON, HI_METAL),
        /* +1 small, +2d4 large */

/* blades; all stack */
WEAPON("dagger", "匕首", None,
       1, 1, 0, 30,  10,   4,  4,  3, 2, P,   P_DAGGER, IRON, HI_METAL),
WEAPON("elven dagger", "精灵匕首", "符文匕首",  //"runed dagger"
       0, 1, 0, 10,  10,   4,  5,  3, 2, P,   P_DAGGER, WOOD, HI_WOOD),
WEAPON("orcish dagger", "兽人匕首", "粗糙匕首",  //"crude dagger"
       0, 1, 0, 12,  10,   4,  3,  3, 2, P,   P_DAGGER, IRON, CLR_BLACK),
WEAPON("silver dagger", "银匕首", None,
       1, 1, 0,  3,  12,  40,  4,  3, 2, P,   P_DAGGER, SILVER, HI_SILVER),
WEAPON("athame", "仪式刀", None,
       1, 1, 0,  0,  10,   4,  4,  3, 2, S,   P_DAGGER, IRON, HI_METAL),
WEAPON("scalpel", "手术刀", None,
       1, 1, 0,  0,   5,   6,  3,  3, 2, S,   P_KNIFE, METAL, HI_METAL),
WEAPON("knife", "小刀", None,
       1, 1, 0, 20,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
WEAPON("stiletto", "小剑", None,
       1, 1, 0,  5,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
/* 3.6: worm teeth and crysknives now stack;
   when a stack of teeth is enchanted at once, they fuse into one crysknife;
   when a stack of crysknives drops, the whole stack reverts to teeth */
WEAPON("worm tooth", "蠕虫齿", None,
       1, 1, 0,  0,  20,   2,  2,  2, 0, 0,   P_KNIFE, 0, CLR_WHITE),
WEAPON("crysknife", "迅捷小刀", None,
       1, 1, 0,  0,  20, 100, 10, 10, 3, P,   P_KNIFE, MINERAL, CLR_WHITE),

/* axes */
WEAPON("axe", "斧头", None,
       1, 0, 0, 40,  60,   8,  6,  4, 0, S,   P_AXE, IRON, HI_METAL),
WEAPON("battle-axe", "战斧", "双头斧",       /* "double-bitted"? */  //"double-headed axe"
       0, 0, 1, 10, 120,  40,  8,  6, 0, S,   P_AXE, IRON, HI_METAL),

/* swords */
WEAPON("short sword", "短剑", None,
       1, 0, 0,  8,  30,  10,  6,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("elven short sword", "精灵短剑", "符文短剑",  //"runed short sword"
       0, 0, 0,  2,  30,  10,  8,  8, 0, P,   P_SHORT_SWORD, WOOD, HI_WOOD),
WEAPON("orcish short sword", "兽人短剑", "粗糙短剑",  //"crude short sword"
       0, 0, 0,  3,  30,  10,  5,  8, 0, P,   P_SHORT_SWORD, IRON, CLR_BLACK),
WEAPON("dwarvish short sword", "矮人短剑", "宽阔短剑",  //"broad short sword"
       0, 0, 0,  2,  30,  10,  7,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("scimitar", "短弯刀", "弯刀",  //"curved sword"
       0, 0, 0, 15,  40,  15,  8,  8, 0, S,   P_SCIMITAR, IRON, HI_METAL),
WEAPON("silver saber", "银剑", None,
       1, 0, 0,  6,  40,  75,  8,  8, 0, S,   P_SABER, SILVER, HI_SILVER),
WEAPON("broadsword", "阔剑", None,
       1, 0, 0,  8,  70,  10,  4,  6, 0, S,   P_BROAD_SWORD, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("elven broadsword", "精灵阔剑", "符文阔剑",  //"runed broadsword"
       0, 0, 0,  4,  70,  10,  6,  6, 0, S,   P_BROAD_SWORD, WOOD, HI_WOOD),
        /* +d4 small, +1 large */
WEAPON("long sword", "长剑", None,
       1, 0, 0, 50,  40,  15,  8, 12, 0, S,   P_LONG_SWORD, IRON, HI_METAL),
WEAPON("two-handed sword", "双手剑", None,
       1, 0, 1, 22, 150,  50, 12,  6, 0, S,   P_TWO_HANDED_SWORD,
                                                            IRON, HI_METAL),
        /* +2d6 large */
WEAPON("katana", "武士刀", "日本刀",  //"samurai sword"
       0, 0, 0,  4,  40,  80, 10, 12, 1, S,   P_LONG_SWORD, IRON, HI_METAL),
/* special swords set up for artifacts */
WEAPON("tsurugi", "武士剑", "武士长剑",  //"long samurai sword"
       0, 0, 1,  0,  60, 500, 16,  8, 2, S,   P_TWO_HANDED_SWORD,
                                                            METAL, HI_METAL),
        /* +2d6 large */
WEAPON("runesword", "符文剑", "符文阔剑",  //"runed broadsword"
       0, 0, 0,  0,  40, 300,  4,  6, 0, S,   P_BROAD_SWORD, IRON, CLR_BLACK),
        /* +d4 small, +1 large; Stormbringer: +5d2 +d8 from level drain */

/* polearms */
/* spear-type */
WEAPON("partisan", "戟", "粗俗长柄武器",  //"vulgar polearm"
       0, 0, 1,  5,  80,  10,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 large */
WEAPON("ranseur", "三叉戟", "大长柄武器",  //"hilted polearm"
       0, 0, 1,  5,  50,   6,  4,  4, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("spetum", "大战戟", "长柄叉",  //"forked polearm"
       0, 0, 1,  5,  50,   5,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 small, +d6 large */
WEAPON("glaive", "剑刃戟", "单刃长柄武器",  //"single-edged polearm"
       0, 0, 1,  8,  75,   6,  6, 10, 0, S,   P_POLEARMS, IRON, HI_METAL),
WEAPON("lance", "长戟", None,
       1, 0, 0,  4, 180,  10,  6,  8, 0, P,   P_LANCE, IRON, HI_METAL),
        /* +2d10 when jousting with lance as primary weapon */
/* axe-type */
WEAPON("halberd", "斧枪", "成角的战斧",  //"angled poleaxe"
       0, 0, 1,  8, 150,  10, 10,  6, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d6 large */
WEAPON("bardiche", "大战斧", "长战斧",  //"long poleaxe"
       0, 0, 1,  4, 120,   7,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small, +2d4 large */
WEAPON("voulge", "长斧", "极切肉刀",  //"pole cleaver"
       0, 0, 1,  4, 125,   5,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("dwarvish mattock", "矮人鹤嘴锄", "宽阔锄头",  //"broad pick"
       0, 0, 1, 13, 120,  50, 12,  8, -1, B,  P_PICK_AXE, IRON, HI_METAL),
/* curved/hooked */
WEAPON("fauchard", "斩矛", "极镰刀",  //"pole sickle"
       0, 0, 1,  6,  60,   5,  6,  8, 0, P|S, P_POLEARMS, IRON, HI_METAL),
WEAPON("guisarme", "长勾刀", "修枝刀",  //"pruning hook"
       0, 0, 1,  6,  80,   5,  4,  8, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("bill-guisarme", "倒勾戟", "弯曲长柄武器",  //"hooked polearm"
       0, 0, 1,  4, 120,   7,  4, 10, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
/* other */
WEAPON("lucern hammer", "苜蓿锤", "分叉长柄武器",  //"pronged polearm"
       0, 0, 1,  5, 150,   7,  4,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("bec de corbin", "鸦啄战锤", "喙长柄武器",  //"beaked polearm"
       0, 0, 1,  4, 100,   8,  8,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),

/* bludgeons */
WEAPON("mace", "权杖", None,
       1, 0, 0, 40,  30,   5,  6,  6, 0, B,   P_MACE, IRON, HI_METAL),
        /* +1 small */
WEAPON("morning star", "流星锤", None,
       1, 0, 0, 12, 120,  10,  4,  6, 0, B,   P_MORNING_STAR, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("war hammer", "战锤", None,
       1, 0, 0, 15,  50,   5,  4,  4, 0, B,   P_HAMMER, IRON, HI_METAL),
        /* +1 small */
WEAPON("club", "棍棒", None,
       1, 0, 0, 12,  30,   3,  6,  3, 0, B,   P_CLUB, WOOD, HI_WOOD),
WEAPON("rubber hose", "橡胶管", None,
       1, 0, 0,  0,  20,   3,  4,  3, 0, B,   P_WHIP, PLASTIC, CLR_BROWN),
WEAPON("quarterstaff", "铁头木棒", "棒子",  //"staff"
       0, 0, 1, 11,  40,   5,  6,  6, 0, B,   P_QUARTERSTAFF, WOOD, HI_WOOD),
/* two-piece */
WEAPON("aklys", "链棒", "皮带棍棒",  //"thonged club"
       0, 0, 0,  8,  15,   4,  6,  3, 0, B,   P_CLUB, IRON, HI_METAL),
WEAPON("flail", "连枷", None,
       1, 0, 0, 40,  15,   4,  6,  4, 0, B,   P_FLAIL, IRON, HI_METAL),
        /* +1 small, +1d4 large */

/* misc */
WEAPON("bullwhip", "牛鞭", None,
       1, 0, 0,  2,  20,   4,  2,  1, 0, 0,   P_WHIP, LEATHER, CLR_BROWN),

/* bows */
BOW("bow", "弓", None,               1, 24, 30, 60, 0, WOOD, P_BOW, HI_WOOD),
BOW("elven bow", "精灵弓", "符文弓",  0, 12, 30, 60, 0, WOOD, P_BOW, HI_WOOD),  //"runed bow"
BOW("orcish bow", "兽人弓", "粗糙弓", 0, 12, 30, 60, 0, WOOD, P_BOW, CLR_BLACK),  //"crude bow"
BOW("yumi", "弩", "长弓",        0,  0, 30, 60, 0, WOOD, P_BOW, HI_WOOD),  //"long bow"
BOW("sling", "投石器", None,             1, 40,  3, 20, 0, LEATHER, P_SLING, HI_LEATHER),
BOW("crossbow", "十字弓", None,          1, 45, 50, 40, 0, WOOD, P_CROSSBOW, HI_WOOD),

#undef P
#undef S
#undef B

#undef WEAPON
#undef PROJECTILE
#undef BOW

/* armor ... */
        /* IRON denotes ferrous metals, including steel.
         * Only IRON weapons and armor can rust.
         * Only COPPER (including brass) corrodes.
         * Some creatures are vulnerable to SILVER.
         */
#define ARMOR(ename,name,desc,kn,mgc,blk,power,prob,delay,wt,  \
              cost,ac,can,sub,metal,c)                   \
    OBJECT(OBJ(ename, name, desc),                                         \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, blk, 0, 0, sub, metal),  \
           power, ARMOR_CLASS, prob, delay, wt,                     \
           cost, 0, 0, 10 - ac, can, wt, c)
#define HELM(ename,name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(ename, name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_HELM, metal, c)
#define CLOAK(ename,name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(ename, name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_CLOAK, metal, c)
#define SHIELD(ename,name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,metal,c) \
    ARMOR(ename, name, desc, kn, mgc, blk, power, prob, delay, wt, \
          cost, ac, can, ARM_SHIELD, metal, c)
#define GLOVES(ename,name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(ename, name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_GLOVES, metal, c)
#define BOOTS(ename,name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(ename, name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_BOOTS, metal, c)

/* helmets */
HELM("elven leather helm", "精灵皮帽", "皮帽",  //"leather hat"
     0, 0,           0,  6, 1,  3,  8,  9, 0, LEATHER, HI_LEATHER),
HELM("orcish helm", "兽人头盔", "铁骷髅帽",  //"iron skull cap"
     0, 0,           0,  6, 1, 30, 10,  9, 0, IRON, CLR_BLACK),
HELM("dwarvish iron helm", "矮人铁头盔", "安全帽",  //"hard hat"
     0, 0,           0,  6, 1, 40, 20,  8, 0, IRON, HI_METAL),
HELM("fedora", "软呢帽", None,
     1, 0,           0,  0, 0,  3,  1, 10, 0, CLOTH, CLR_BROWN),
HELM("cornuthaum", "巫师帽", "圆锥形帽",  //"conical hat"
     0, 1, CLAIRVOYANT,  3, 1,  4, 80, 10, 1, CLOTH, CLR_BLUE),
        /* name coined by devteam; confers clairvoyance for wizards,
           blocks clairvoyance if worn by role other than wizard */
HELM("dunce cap", "愚人帽", "圆锥形帽",  //"conical hat"
     0, 1,           0,  3, 1,  4,  1, 10, 0, CLOTH, CLR_BLUE),
HELM("dented pot", "瘪罐", None,
     1, 0,           0,  2, 0, 10,  8,  9, 0, IRON, CLR_BLACK),
/* with shuffled appearances... */
HELM("helmet", "钢盔", "羽饰头盔",  //"plumed helmet"
     0, 0,           0, 10, 1, 30, 10,  9, 0, IRON, HI_METAL),
HELM("helm of brilliance", "卓越头盔", "蚀刻的头盔",  //"etched helmet"
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, CLR_GREEN),
HELM("helm of opposite alignment", "敌对阵营头盔", "羽冠头盔",  //"crested helmet"
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, HI_METAL),
HELM("helm of telepathy", "感知头盔", "檐帽头盔",  //"visored helmet"
     0, 1,     TELEPAT,  2, 1, 50, 50,  9, 0, IRON, HI_METAL),

/* suits of armor */
/*
 * There is code in polyself.c that assumes (1) and (2).
 * There is code in obj.h, objnam.c, mon.c, read.c that assumes (2).
 *      (1) The dragon scale mails and the dragon scales are together.
 *      (2) That the order of the dragon scale mail and dragon scales
 *          is the the same as order of dragons defined in monst.c.
 */
#define DRGN_ARMR(ename,name,mgc,power,cost,ac,color)  \
    ARMOR(ename, name, None, 1, mgc, 1, power, 0, 5, 40,  \
          cost, ac, 0, ARM_SUIT, DRAGON_HIDE, color)
/* 3.4.1: dragon scale mail reclassified as "magic" since magic is
   needed to create them */
DRGN_ARMR("gray dragon scale mail", "灰龙鳞甲",    1, ANTIMAGIC,  1200, 1, CLR_GRAY),
DRGN_ARMR("silver dragon scale mail", "银龙鳞甲",  1, REFLECTING, 1200, 1, DRAGON_SILVER),
#if 0 /* DEFERRED */
DRGN_ARMR("shimmering dragon scale mail", 1, DISPLACED, 1200, 1, CLR_CYAN),
#endif
DRGN_ARMR("red dragon scale mail", "红龙鳞甲",     1, FIRE_RES,    900, 1, CLR_RED),
DRGN_ARMR("white dragon scale mail", "白龙鳞甲",   1, COLD_RES,    900, 1, CLR_WHITE),
DRGN_ARMR("orange dragon scale mail", "橙龙鳞甲",  1, SLEEP_RES,   900, 1, CLR_ORANGE),
DRGN_ARMR("black dragon scale mail", "黑龙鳞甲",   1, DISINT_RES, 1200, 1, CLR_BLACK),
DRGN_ARMR("blue dragon scale mail", "蓝龙鳞甲",    1, SHOCK_RES,   900, 1, CLR_BLUE),
DRGN_ARMR("green dragon scale mail", "绿龙鳞甲",   1, POISON_RES,  900, 1, CLR_GREEN),
DRGN_ARMR("yellow dragon scale mail", "黄龙鳞甲",  1, ACID_RES,    900, 1, CLR_YELLOW),
/* For now, only dragons leave these. */
/* 3.4.1: dragon scales left classified as "non-magic"; they confer
   magical properties but are produced "naturally" */
DRGN_ARMR("gray dragon scales", "灰龙鳞",        0, ANTIMAGIC,   700, 7, CLR_GRAY),
DRGN_ARMR("silver dragon scales", "银龙鳞",      0, REFLECTING,  700, 7, DRAGON_SILVER),
#if 0 /* DEFERRED */
DRGN_ARMR("闪光龙鳞",  0, DISPLACED,   700, 7, CLR_CYAN),  //"shimmering dragon scales"
#endif
DRGN_ARMR("red dragon scales", "红龙鳞",         0, FIRE_RES,    500, 7, CLR_RED),
DRGN_ARMR("white dragon scales", "白龙鳞",       0, COLD_RES,    500, 7, CLR_WHITE),
DRGN_ARMR("orange dragon scales", "橙龙鳞",      0, SLEEP_RES,   500, 7, CLR_ORANGE),
DRGN_ARMR("black dragon scales", "黑龙鳞",       0, DISINT_RES,  700, 7, CLR_BLACK),
DRGN_ARMR("blue dragon scales", "蓝龙鳞",        0, SHOCK_RES,   500, 7, CLR_BLUE),
DRGN_ARMR("green dragon scales", "绿龙鳞",       0, POISON_RES,  500, 7, CLR_GREEN),
DRGN_ARMR("yellow dragon scales", "黄龙鳞",      0, ACID_RES,    500, 7, CLR_YELLOW),
#undef DRGN_ARMR
/* other suits */
ARMOR("plate mail", "板甲", None,
      1, 0, 1,  0, 44, 5, 450, 600,  3, 2,  ARM_SUIT, IRON, HI_METAL),
ARMOR("crystal plate mail", "水晶板甲", None,
      1, 0, 1,  0, 10, 5, 450, 820,  3, 2,  ARM_SUIT, GLASS, CLR_WHITE),
ARMOR("bronze plate mail", "青铜板甲", None,
      1, 0, 1,  0, 25, 5, 450, 400,  4, 1,  ARM_SUIT, COPPER, HI_COPPER),
ARMOR("splint mail", "板条甲", None,
      1, 0, 1,  0, 62, 5, 400,  80,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("banded mail", "带链甲", None,
      1, 0, 1,  0, 72, 5, 350,  90,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("dwarvish mithril-coat", "矮人秘银胶衣", None,
      1, 0, 0,  0, 10, 1, 150, 240,  4, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("elven mithril-coat", "精灵秘银胶衣", None,
      1, 0, 0,  0, 15, 1, 150, 240,  5, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("chain mail", "锁子甲", None,
      1, 0, 0,  0, 72, 5, 300,  75,  5, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("orcish chain mail", "兽人锁子甲", "粗糙锁子甲",  //"crude chain mail"
      0, 0, 0,  0, 20, 5, 300,  75,  6, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("scale mail", "鳞甲", None,
      1, 0, 0,  0, 72, 5, 250,  45,  6, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("studded leather armor", "嵌皮甲", None,
      1, 0, 0,  0, 72, 3, 200,  15,  7, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("ring mail", "锁环甲", None,
      1, 0, 0,  0, 72, 5, 250, 100,  7, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("orcish ring mail", "兽人锁环甲", "粗糙锁环甲",  //"crude ring mail"
      0, 0, 0,  0, 20, 5, 250,  80,  8, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("leather armor", "皮甲", None,
      1, 0, 0,  0, 82, 3, 150,   5,  8, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("leather jacket", "皮夹克", None,
      1, 0, 0,  0, 12, 0,  30,  10,  9, 0,  ARM_SUIT, LEATHER, CLR_BLACK),

/* shirts */
ARMOR("Hawaiian shirt", "夏威夷衬衫", None,
      1, 0, 0,  0,  8, 0,   5,   3, 10, 0,  ARM_SHIRT, CLOTH, CLR_MAGENTA),
ARMOR("T-shirt", "T 恤衫", None,
      1, 0, 0,  0,  2, 0,   5,   2, 10, 0,  ARM_SHIRT, CLOTH, CLR_WHITE),

/* cloaks */
CLOAK("mummy wrapping", "木乃伊绷带", None,
      1, 0,          0,  0, 0,  3,  2, 10, 1,  CLOTH, CLR_GRAY),
        /* worn mummy wrapping blocks invisibility */
CLOAK("elven cloak", "精灵斗篷", "褪色的斗篷",  //"faded pall"
      0, 1,    STEALTH,  8, 0, 10, 60,  9, 1,  CLOTH, CLR_BLACK),
CLOAK("orcish cloak", "兽人斗篷", "粗糙的小斗蓬",  //"coarse mantelet"
      0, 0,          0,  8, 0, 10, 40, 10, 1,  CLOTH, CLR_BLACK),
CLOAK("dwarvish cloak", "矮人斗篷", "带帽斗篷",  //"hooded cloak"
      0, 0,          0,  8, 0, 10, 50, 10, 1,  CLOTH, HI_CLOTH),
CLOAK("oilskin cloak", "油布斗篷", "湿滑的斗篷",  //"slippery cloak"
      0, 0,          0,  8, 0, 10, 50,  9, 2,  CLOTH, HI_CLOTH),
CLOAK("robe", "长袍", None,
      1, 1,          0,  3, 0, 15, 50,  8, 2,  CLOTH, CLR_RED),
        /* robe was adopted from slash'em, where it's worn as a suit
           rather than as a cloak and there are several variations */
CLOAK("alchemy smock", "炼金术罩衫", "围裙",  //"apron"
      0, 1, POISON_RES,  9, 0, 10, 50,  9, 1,  CLOTH, CLR_WHITE),
CLOAK("leather cloak", "皮斗篷", None,
      1, 0,          0,  8, 0, 15, 40,  9, 1,  LEATHER, CLR_BROWN),
/* with shuffled appearances... */
CLOAK("cloak of protection", "保护斗篷", "破烂的斗篷",  //"tattered cape"
      0, 1, PROTECTION,  9, 0, 10, 50,  7, 3,  CLOTH, HI_CLOTH),
        /* cloak of protection is now the only item conferring MC 3 */
CLOAK("cloak of invisibility", "隐身斗篷", "夜礼服斗篷",  //"opera cloak"
      0, 1,      INVIS, 10, 0, 10, 60,  9, 1,  CLOTH, CLR_BRIGHT_MAGENTA),
CLOAK("cloak of magic resistance", "魔法抵抗斗篷", "装饰性长袍",  //"ornamental cope"
      0, 1,  ANTIMAGIC,  2, 0, 10, 60,  9, 1,  CLOTH, CLR_WHITE),
        /*  'cope' is not a spelling mistake... leave it be */
CLOAK("cloak of displacement", "幻影斗篷", "一块布",  //"piece of cloth"
      0, 1,  DISPLACED, 10, 0, 10, 50,  9, 1,  CLOTH, HI_CLOTH),

/* shields */
SHIELD("small shield", "小盾牌", None,
       1, 0, 0,          0, 6, 0,  30,  3, 9, 0,  WOOD, HI_WOOD),
SHIELD("elven shield", "精灵盾", "蓝绿盾",  //"blue and green shield"
       0, 0, 0,          0, 2, 0,  40,  7, 8, 0,  WOOD, CLR_GREEN),
SHIELD("Uruk-hai shield", "强兽人盾", "白色手盾",  //"white-handed shield"
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, HI_METAL),
SHIELD("orcish shield", "兽人盾", "红眼盾",  //"red-eyed shield"
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, CLR_RED),
SHIELD("large shield", "大盾牌", None,
       1, 0, 1,          0, 7, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("dwarvish roundshield", "矮人圆盾", "大圆盾",  //"large round shield"
       0, 0, 0,          0, 4, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("shield of reflection", "反射之盾", "抛光银盾",  //"polished silver shield"
       0, 1, 0, REFLECTING, 3, 0,  50, 50, 8, 0,  SILVER, HI_SILVER),

/* gloves */
/* These have their color but not material shuffled, so the IRON must
 * stay CLR_BROWN (== HI_LEATHER) even though it's normally either
 * HI_METAL or CLR_BLACK.  All have shuffled descriptions.
 */
GLOVES("leather gloves", "皮手套", "残破的手套",  //"old gloves"
       0, 0,        0, 16, 1, 10,  8, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("gauntlets of fumbling", "笨拙手套", "加衬手套",  //"padded gloves"
       0, 1, FUMBLING,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("gauntlets of power", "力量手套", "骑手手套",  //"riding gloves"
       0, 1,        0,  8, 1, 30, 50, 9, 0,  IRON, CLR_BROWN),
GLOVES("gauntlets of dexterity", "敏捷手套", "击剑手套",  //"fencing gloves"
       0, 1,        0,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),

/* boots */
BOOTS("low boots", "低跟鞋", "步行鞋",  //"walking shoes"
      0, 0,          0, 25, 2, 10,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("iron shoes", "铁鞋", "硬底鞋",  //"hard shoes"
      0, 0,          0,  7, 2, 50, 16, 8, 0, IRON, HI_METAL),
BOOTS("high boots", "高筒靴", "长筒靴",  //"jackboots"
      0, 0,          0, 15, 2, 20, 12, 8, 0, LEATHER, HI_LEATHER),
/* with shuffled appearances... */
BOOTS("speed boots", "速度靴", "战斗靴",  //"combat boots"
      0, 1,       FAST, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("water walking boots", "水上步靴", "丛林靴",  //"jungle boots"
      0, 1,   WWALKING, 12, 2, 15, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("jumping boots", "跳跃靴", "登山靴",  //"hiking boots"
      0, 1,    JUMPING, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("elven boots", "精灵靴", "泥靴",  //"mud boots"
      0, 1,    STEALTH, 12, 2, 15,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("kicking boots", "踢靴", "带扣靴",  //"buckled boots"
      0, 1,          0, 12, 2, 50,  8, 9, 0, IRON, CLR_BROWN),
        /* CLR_BROWN for same reason as gauntlets of power */
BOOTS("fumble boots", "笨拙靴", "马靴",  //"riding boots"
      0, 1,   FUMBLING, 12, 2, 20, 30, 9, 0, LEATHER, HI_LEATHER),
BOOTS("levitation boots", "飘浮靴", "雪地靴",  //"snow boots"
      0, 1, LEVITATION, 12, 2, 15, 30, 9, 0, LEATHER, HI_LEATHER),
#undef HELM
#undef CLOAK
#undef SHIELD
#undef GLOVES
#undef BOOTS
#undef ARMOR

/* rings ... */
#define RING(ename,name,stone,power,cost,mgc,spec,mohs,metal,color) \
    OBJECT(OBJ(ename, name, stone),                                          \
           BITS(0, 0, spec, 0, mgc, spec, 0, 0, 0,                    \
                HARDGEM(mohs), 0, P_NONE, metal),                     \
           power, RING_CLASS, 0, 0, 3, cost, 0, 0, 0, 0, 15, color)
RING("adornment", "装饰品", "木制",  //"wooden"
     ADORNED,                  100, 1, 1, 2, WOOD, HI_WOOD),
RING("gain strength", "增加力量", "花岗石",  //"granite"
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("gain constitution", "增加体质", "蛋白石",  //"opal"
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("increase accuracy", "增加精确", "黏土",  //"clay"
     0,                        150, 1, 1, 4, MINERAL, CLR_RED),
RING("increase damage", "增加伤害", "珊瑚",  //"coral"
     0,                        150, 1, 1, 4, MINERAL, CLR_ORANGE),
RING("protection", "保护", "黑玛瑙",  //"black onyx"
     PROTECTION,               100, 1, 1, 7, MINERAL, CLR_BLACK),
        /* 'PROTECTION' intrinsic enhances MC from worn armor by +1,
           regardless of ring's enchantment; wearing a second ring of
           protection (or even one ring of protection combined with
           cloak of protection) doesn't give a second MC boost */
RING("regeneration", "再生", "月石",  //"moonstone"
     REGENERATION,             200, 1, 0,  6, MINERAL, HI_MINERAL),
RING("searching", "搜索", "虎眼石",  //"tiger eye"
     SEARCHING,                200, 1, 0,  6, GEMSTONE, CLR_BROWN),
RING("stealth", "潜行", "翡翠",  //"jade"
     STEALTH,                  100, 1, 0,  6, GEMSTONE, CLR_GREEN),
RING("sustain ability", "维持能力", "青铜",  //"bronze"
     FIXED_ABIL,               100, 1, 0,  4, COPPER, HI_COPPER),
RING("levitation", "飘浮", "玛瑙",  //"agate"
     LEVITATION,               200, 1, 0,  7, GEMSTONE, CLR_RED),
RING("hunger", "饥饿", "黄宝石",  //"topaz"
     HUNGER,                   100, 1, 0,  8, GEMSTONE, CLR_CYAN),
RING("aggravate monster", "激怒怪物", "蓝宝石",  //"sapphire"
     AGGRAVATE_MONSTER,        150, 1, 0,  9, GEMSTONE, CLR_BLUE),
RING("conflict", "冲突", "红宝石",  //"ruby"
     CONFLICT,                 300, 1, 0,  9, GEMSTONE, CLR_RED),
RING("warning", "警报", "钻石",  //"diamond"
     WARNING,                  100, 1, 0, 10, GEMSTONE, CLR_WHITE),
RING("poison resistance", "抗毒", "珍珠",  //"pearl"
     POISON_RES,               150, 1, 0,  4, BONE, CLR_WHITE),
RING("fire resistance", "抗火", "铁",  //"iron"
     FIRE_RES,                 200, 1, 0,  5, IRON, HI_METAL),
RING("cold resistance", "抗寒", "黄铜",  //"brass"
     COLD_RES,                 150, 1, 0,  4, COPPER, HI_COPPER),
RING("shock resistance", "抗电", "铜",  //"copper"
     SHOCK_RES,                150, 1, 0,  3, COPPER, HI_COPPER),
RING("free action", "自由行动", "扭曲的",  //"twisted"
     FREE_ACTION,              200, 1, 0,  6, IRON, HI_METAL),
RING("slow digestion", "慢消化", "钢铁",  //"steel"
     SLOW_DIGESTION,           200, 1, 0,  8, IRON, HI_METAL),
RING("teleportation", "传送", "银",  //"silver"
     TELEPORT,                 200, 1, 0,  3, SILVER, HI_SILVER),
RING("teleport control", "传送控制", "金",  //"gold"
     TELEPORT_CONTROL,         300, 1, 0,  3, GOLD, HI_GOLD),
RING("polymorph", "变形", "象牙",  //"ivory"
     POLYMORPH,                300, 1, 0,  4, BONE, CLR_WHITE),
RING("polymorph control", "变形控制", "祖母绿",  //"emerald"
     POLYMORPH_CONTROL,        300, 1, 0,  8, GEMSTONE, CLR_BRIGHT_GREEN),
RING("invisibility", "隐身", "金属",  //"wire"
     INVIS,                    150, 1, 0,  5, IRON, HI_METAL),
RING("see invisible", "看见隐形", "订婚",  //"engagement"
     SEE_INVIS,                150, 1, 0,  5, IRON, HI_METAL),
RING("protection from shape changers", "变形保护", "闪耀的",  //"shiny"
     PROT_FROM_SHAPE_CHANGERS, 100, 1, 0,  5, IRON, CLR_BRIGHT_CYAN),
#undef RING

/* amulets ... - THE Amulet comes last because it is special */
#define AMULET(ename,name,desc,power,prob) \
    OBJECT(OBJ(ename, name, desc),                                            \
           BITS(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, P_NONE, IRON),        \
           power, AMULET_CLASS, prob, 0, 20, 150, 0, 0, 0, 0, 20, HI_METAL)
AMULET("amulet of ESP", "感知护身符",                "圆形", TELEPAT, 175),  //"circular"
AMULET("amulet of life saving", "复活护身符",       "球形", LIFESAVED, 75),  //"spherical"
AMULET("amulet of strangulation", "窒息护身符",          "椭圆形", STRANGLED, 135),  //"oval"
AMULET("amulet of restful sleep", "深度睡眠护身符",    "三角形", SLEEPY, 135),  //"triangular"
AMULET("amulet versus poison", "毒抗护身符",        "锥状", POISON_RES, 165),  //"pyramidal"
AMULET("amulet of change", "变性护身符",               "方形", 0, 130),  //"square"
AMULET("amulet of unchanging", "阻止变形护身符",          "凹形", UNCHANGING, 45),  //"concave"
AMULET("amulet of reflection", "反射护身符",        "六角形", REFLECTING, 75),  //"hexagonal"
AMULET("amulet of magical breathing", "魔法呼吸护身符", "八角形", MAGICAL_BREATHING, 65),  //"octagonal"
/* fixed descriptions; description duplication is deliberate;
 * fake one must come before real one because selection for
 * description shuffling stops when a non-magic amulet is encountered
 */
OBJECT(OBJ("cheap plastic imitation of the Amulet of Yendor", "岩德护身符的廉价塑料仿制品",
           "岩德护身符"),  //"Amulet of Yendor"
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, PLASTIC),
       0, AMULET_CLASS, 0, 0, 20, 0, 0, 0, 0, 0, 1, HI_METAL),
OBJECT(OBJ("Amulet of Yendor", "岩德护身符", /* note: description == name */
           "岩德护身符"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, MITHRIL),
       0, AMULET_CLASS, 0, 0, 20, 30000, 0, 0, 0, 0, 20, HI_METAL),
#undef AMULET

/* tools ... */
/* tools with weapon characteristics come last */
#define TOOL(ename,name,desc,kn,mrg,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(kn, mrg, chg, 0, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat), \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define CONTAINER(ename,name,desc,kn,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(kn, 0, chg, 1, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat),   \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define WEPTOOL(ename,name,desc,kn,mgc,bi,prob,wt,cost,sdam,ldam,hitbon,sub,mat,clr)\
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, bi, 0, hitbon, sub, mat),    \
           0, TOOL_CLASS, prob, 0, wt, cost, sdam, ldam, hitbon, 0, wt, clr)
/* containers */
CONTAINER("large box", "大箱子",       None, 1, 0, 0, 40, 350,   8, WOOD, HI_WOOD),
CONTAINER("chest", "箱子",           None, 1, 0, 0, 35, 600,  16, WOOD, HI_WOOD),
CONTAINER("ice box", "冰盒子",         None, 1, 0, 0,  5, 900,  42, PLASTIC, CLR_WHITE),
CONTAINER("sack", "布袋",           "袋子", 0, 0, 0, 35,  15,   2, CLOTH, HI_CLOTH),  //"bag"
CONTAINER("oilskin sack", "防水袋",   "袋子", 0, 0, 0,  5,  15, 100, CLOTH, HI_CLOTH),  //"bag"
CONTAINER("bag of holding", "次元袋", "袋子", 0, 1, 0, 20,  15, 100, CLOTH, HI_CLOTH),  //"bag"
CONTAINER("bag of tricks", "魔术袋",  "袋子", 0, 1, 1, 20,  15, 100, CLOTH, HI_CLOTH),  //"bag"
#undef CONTAINER

/* lock opening tools */
TOOL("skeleton key", "万能钥匙",       "钥匙", 0, 0, 0, 0, 80,  3, 10, IRON, HI_METAL),  //"key"
TOOL("lock pick", "开锁器",           None, 1, 0, 0, 0, 60,  4, 20, IRON, HI_METAL),
TOOL("credit card", "信用卡",         None, 1, 0, 0, 0, 15,  1, 10, PLASTIC, CLR_WHITE),
/* light sources */
TOOL("tallow candle", "牛油烛",   "蜡烛", 0, 1, 0, 0, 20,  2, 10, WAX, CLR_WHITE),  //"candle"
TOOL("wax candle", "蜡状蜡烛",      "蜡烛", 0, 1, 0, 0,  5,  2, 20, WAX, CLR_WHITE),  //"candle"
TOOL("brass lantern", "黄铜灯笼",       None, 1, 0, 0, 0, 30, 30, 12, COPPER, CLR_YELLOW),
TOOL("oil lamp", "油灯",          "灯", 0, 0, 0, 0, 45, 20, 10, COPPER, CLR_YELLOW),  //"lamp"
TOOL("magic lamp", "神灯",        "灯", 0, 0, 1, 0, 15, 20, 50, COPPER, CLR_YELLOW),  //"lamp"
/* other tools */
TOOL("expensive camera", "高档相机",    None, 1, 0, 0, 1, 15, 12,200, PLASTIC, CLR_BLACK),
TOOL("mirror", "反光镜",   "镜子", 0, 0, 0, 0, 45, 13, 10, GLASS, HI_SILVER),  //"looking glass"
TOOL("crystal ball", "水晶球", "玻璃球", 0, 0, 1, 1, 15,150, 60, GLASS, HI_GLASS),  //"glass orb"
TOOL("lenses", "眼镜",              None, 1, 0, 0, 0,  5,  3, 80, GLASS, HI_GLASS),
TOOL("blindfold", "眼罩",           None, 1, 0, 0, 0, 50,  2, 20, CLOTH, CLR_BLACK),
TOOL("towel", "毛巾",               None, 1, 0, 0, 0, 50,  2, 50, CLOTH, CLR_MAGENTA),
TOOL("saddle", "鞍",              None, 1, 0, 0, 0,  5,200,150, LEATHER, HI_LEATHER),
TOOL("leash", "狗链",               None, 1, 0, 0, 0, 65, 12, 20, LEATHER, HI_LEATHER),
TOOL("stethoscope", "听诊器",         None, 1, 0, 0, 0, 25,  4, 75, IRON, HI_METAL),
TOOL("tinning kit", "装罐器",         None, 1, 0, 0, 1, 15,100, 30, IRON, HI_METAL),
TOOL("tin opener", "开罐器",          None, 1, 0, 0, 0, 35,  4, 30, IRON, HI_METAL),
TOOL("can of grease", "油脂罐",       None, 1, 0, 0, 1, 15, 15, 20, IRON, HI_METAL),
TOOL("figurine", "小雕像",            None, 1, 0, 1, 0, 25, 50, 80, MINERAL, HI_MINERAL),
        /* monster type specified by obj->corpsenm */
TOOL("magic marker", "魔笔",        None, 1, 0, 1, 1, 15,  2, 50, PLASTIC, CLR_RED),
/* traps */
TOOL("land mine", "地雷",           None, 1, 0, 0, 0, 0, 300,180, IRON, CLR_RED),
TOOL("beartrap", "捕兽夹",            None, 1, 0, 0, 0, 0, 200, 60, IRON, HI_METAL),
/* instruments;
   "If tin whistles are made out of tin, what do they make foghorns out of?" */
TOOL("tin whistle", "六孔哨",    "口哨", 0, 0, 0, 0,100, 3, 10, METAL, HI_METAL),  //"whistle"
TOOL("magic whistle", "魔法口哨",  "口哨", 0, 0, 1, 0, 30, 3, 10, METAL, HI_METAL),  //"whistle"
TOOL("wooden flute", "木笛",     "长笛", 0, 0, 0, 0,  4, 5, 12, WOOD, HI_WOOD),  //"flute"
TOOL("magic flute", "魔笛",      "长笛", 0, 0, 1, 1,  2, 5, 36, WOOD, HI_WOOD),  //"flute"
TOOL("tooled horn", "加工号角",       "号角", 0, 0, 0, 0,  5, 18, 15, BONE, CLR_WHITE),  //"horn"
TOOL("frost horn", "冰霜号角",        "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"horn"
TOOL("fire horn", "火焰号角",         "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"horn"
TOOL("horn of plenty", "丰饶之角",    "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"horn"
        /* horn, but not an instrument */
TOOL("wooden harp", "木竖琴",       "竖琴", 0, 0, 0, 0,  4, 30, 50, WOOD, HI_WOOD),  //"harp"
TOOL("magic harp", "魔幻竖琴",        "竖琴", 0, 0, 1, 1,  2, 30, 50, WOOD, HI_WOOD),  //"harp"
TOOL("bell", "铃",                None, 1, 0, 0, 0,  2, 30, 50, COPPER, HI_COPPER),
TOOL("bugle", "军号",               None, 1, 0, 0, 0,  4, 10, 15, COPPER, HI_COPPER),
TOOL("leather drum", "皮革鼓",      "鼓", 0, 0, 0, 0,  4, 25, 25, LEATHER, HI_LEATHER),  //"drum"
TOOL("drum of earthquake","地震鼓","鼓", 0, 0, 1, 1,  2, 25, 25, LEATHER, HI_LEATHER),  //"drum"
/* tools useful as weapons */
WEPTOOL("pick-axe", "鹤嘴锄", None,
        1, 0, 0, 20, 100,  50,  6,  3, WHACK,  P_PICK_AXE, IRON, HI_METAL),
WEPTOOL("grappling hook", "爪钩", "铁钩",  //"iron hook"
        0, 0, 0,  5,  30,  50,  2,  6, WHACK,  P_FLAIL,    IRON, HI_METAL),
WEPTOOL("unicorn horn", "独角兽的角", None,
        1, 1, 1,  0,  20, 100, 12, 12, PIERCE, P_UNICORN_HORN,
                                                           BONE, CLR_WHITE),
        /* 3.4.1: unicorn horn left classified as "magic" */
/* two unique tools;
 * not artifacts, despite the comment which used to be here
 */
OBJECT(OBJ("Candelabrum of Invocation", "祈祷烛台", "烛台"),  //"candelabrum"
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, GOLD),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 200, HI_GOLD),
OBJECT(OBJ("Bell of Opening", "开启之铃", "银铃"),  //"silver bell"
       BITS(0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, P_NONE, SILVER),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 50, HI_SILVER),
#undef TOOL
#undef WEPTOOL

/* Comestibles ... */
#define FOOD(ename, name, prob, delay, wt, unk, tin, nutrition, color)         \
    OBJECT(OBJ(ename, name, None),                                       \
           BITS(1, 1, unk, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, tin), 0,     \
           FOOD_CLASS, prob, delay, wt, nutrition / 20 + 5, 0, 0, 0, 0, \
           nutrition, color)
/* All types of food (except tins & corpses) must have a delay of at least 1.
 * Delay on corpses is computed and is weight dependant.
 * Domestic pets prefer tripe rations above all others.
 * Fortune cookies can be read, using them up without ingesting them.
 * Carrots improve your vision.
 * +0 tins contain monster meat.
 * +1 tins (of spinach) make you stronger (like Popeye).
 * Meatballs/sticks/rings are only created from objects via stone to flesh.
 */
/* meat */
FOOD("tripe ration", "牛肚",        140,  2, 10, 0, FLESH, 200, CLR_BROWN),
FOOD("corpse", "尸体",                0,  1,  0, 0, FLESH,   0, CLR_BROWN),
FOOD("egg", "蛋",                  85,  1,  1, 1, FLESH,  80, CLR_WHITE),
FOOD("meatball", "肉丸",              0,  1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("meat stick", "肉棍",            0,  1,  1, 0, FLESH,   5, CLR_BROWN),
FOOD("huge chunk of meat", "大块肉",    0, 20,400, 0, FLESH,2000, CLR_BROWN),
/* special case because it's not mergable */
OBJECT(OBJ("meat ring", "肉环", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FLESH),
       0, FOOD_CLASS, 0, 1, 5, 1, 0, 0, 0, 0, 5, CLR_BROWN),
/* pudding 'corpses' will turn into these and combine;
   must be in same order as the pudding monsters */
FOOD("glob of gray ooze", "灰色软泥团",     0,  2, 20, 0, FLESH,  20, CLR_GRAY),
FOOD("glob of brown pudding", "棕色布丁团", 0,  2, 20, 0, FLESH,  20, CLR_BROWN),
FOOD("glob of green slime", "绿色粘液团",   0,  2, 20, 0, FLESH,  20, CLR_GREEN),
FOOD("glob of black pudding", "黑色布丁团", 0,  2, 20, 0, FLESH,  20, CLR_BLACK),

/* fruits & veggies */
FOOD("kelp frond", "海藻叶子",            0,  1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("eucalyptus leaf", "桉叶",       3,  1,  1, 0, VEGGY,  30, CLR_GREEN),
FOOD("apple", "苹果",                15,  1,  2, 0, VEGGY,  50, CLR_RED),
FOOD("orange", "橙子",               10,  1,  2, 0, VEGGY,  80, CLR_ORANGE),
FOOD("pear", "梨",                 10,  1,  2, 0, VEGGY,  50, CLR_BRIGHT_GREEN),
FOOD("melon", "甜瓜",                10,  1,  5, 0, VEGGY, 100, CLR_BRIGHT_GREEN),
FOOD("banana", "香蕉",               10,  1,  2, 0, VEGGY,  80, CLR_YELLOW),
FOOD("carrot", "胡萝卜",               15,  1,  2, 0, VEGGY,  50, CLR_ORANGE),
FOOD("sprig of wolfsbane", "附子草枝",    7,  1,  1, 0, VEGGY,  40, CLR_GREEN),
FOOD("clove of garlic", "蒜瓣",       7,  1,  1, 0, VEGGY,  40, CLR_WHITE),
/* name of slime mold is changed based on player's OPTION=fruit:something
   and bones data might have differently named ones from prior games */
FOOD("slime mold", "黏液",           75,  1,  5, 0, VEGGY, 250, HI_ORGANIC),

/* people food */
FOOD("lump of royal jelly", "蜂王浆",   0,  1,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("cream pie", "奶油派",            25,  1, 10, 0, VEGGY, 100, CLR_WHITE),
FOOD("candy bar", "条形糖果",            13,  1,  2, 0, VEGGY, 100, CLR_BROWN),
FOOD("fortune cookie", "幸运饼干",       55,  1,  1, 0, VEGGY,  40, CLR_YELLOW),
FOOD("pancake", "煎饼",              25,  2,  2, 0, VEGGY, 200, CLR_YELLOW),
FOOD("lembas wafer", "兰巴斯片",         20,  2,  5, 0, VEGGY, 800, CLR_WHITE),
FOOD("cram ration", "压缩口粮",          20,  3, 15, 0, VEGGY, 600, HI_ORGANIC),
FOOD("food ration", "口粮",         380,  5, 20, 0, VEGGY, 800, HI_ORGANIC),
FOOD("K-ration", "K- 口粮",              0,  1, 10, 0, VEGGY, 400, HI_ORGANIC),
FOOD("C-ration", "C- 口粮",              0,  1, 10, 0, VEGGY, 300, HI_ORGANIC),
/* tins have type specified by obj->spe (+1 for spinach, other implies
   flesh; negative specifies preparation method {homemade,boiled,&c})
   and by obj->corpsenm (type of monster flesh) */
FOOD("tin", "罐头",                  75,  0, 10, 1, METAL,   0, HI_METAL),
#undef FOOD

/* potions ... */
#define POTION(ename,name,desc,mgc,power,prob,cost,color) \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, GLASS),      \
           power, POTION_CLASS, prob, 0, 20, cost, 0, 0, 0, 0, 10, color)
POTION("gain ability", "增强能力",           "深红色",  1, 0, 42, 300, CLR_RED),  //"ruby"
POTION("restore ability", "恢复能力",        "粉红色",  1, 0, 40, 100, CLR_BRIGHT_MAGENTA),  //"pink"
POTION("confusion", "混乱",            "橙色",  1, CONFUSION, 42, 100, CLR_ORANGE),  //"orange"
POTION("blindness", "失明",            "黄色",  1, BLINDED, 40, 150, CLR_YELLOW),  //"yellow"
POTION("paralysis", "麻痹",           "翠绿色",  1, 0, 42, 300, CLR_BRIGHT_GREEN),  //"emerald"
POTION("speed", "加速",            "深绿色",  1, FAST, 42, 200, CLR_GREEN),  //"dark green"
POTION("levitation", "飘浮",             "蓝绿色",  1, LEVITATION, 42, 200, CLR_CYAN),  //"cyan"
POTION("hallucination", "幻觉",      "天蓝色",  1, HALLUC, 40, 100, CLR_CYAN),  //"sky blue"
POTION("invisibility", "隐身", "亮蓝色",  1, INVIS, 40, 150, CLR_BRIGHT_BLUE),  //"brilliant blue"
POTION("see invisible", "看见隐形",       "洋红色",  1, SEE_INVIS, 42, 50, CLR_MAGENTA),  //"magenta"
POTION("healing", "治愈",          "紫红色",  1, 0, 57, 100, CLR_MAGENTA),  //"purple-red"
POTION("extra healing", "强力治愈",          "深褐色",  1, 0, 47, 100, CLR_RED),  //"puce"
POTION("gain level", "升级",            "乳白色",  1, 0, 20, 300, CLR_WHITE),  //"milky"
POTION("enlightenment", "启蒙",        "涡旋形",  1, 0, 20, 200, CLR_BROWN),  //"swirly"
POTION("monster detection", "怪物探测",    "多泡的",  1, 0, 40, 150, CLR_WHITE),  //"bubbly"
POTION("object detection", "物品探测",      "冒烟的",  1, 0, 42, 150, CLR_GRAY),  //"smoky"
POTION("gain energy", "获得能量",          "混浊的",  1, 0, 42, 150, CLR_WHITE),  //"cloudy"
POTION("sleeping", "沉睡",       "沸腾的",  1, 0, 42, 100, CLR_GRAY),  //"effervescent"
POTION("full healing", "完全治愈",          "黑色的",  1, 0, 10, 200, CLR_BLACK),  //"black"
POTION("polymorph", "变形",            "金色",  1, 0, 10, 200, CLR_YELLOW),  //"golden"
POTION("booze", "酒",                 "棕色",  0, 0, 42,  50, CLR_BROWN),  //"brown"
POTION("sickness", "疾病",              "起泡的",  0, 0, 42,  50, CLR_CYAN),  //"fizzy"
POTION("fruit juice", "果汁",            "深色的",  0, 0, 42,  50, CLR_BLACK),  //"dark"
POTION("acid", "酸",                  "白色的",  0, 0, 10, 250, CLR_WHITE),  //"white"
POTION("oil", "油",                   "黑暗的",  0, 0, 30, 250, CLR_BROWN),  //"murky"
/* fixed description
 */
POTION("water", "水",                 "清澈的",  0, 0, 92, 100, CLR_CYAN),  //"clear"
#undef POTION

/* scrolls ... */
#define SCROLL(ename,name,text,mgc,prob,cost) \
    OBJECT(OBJ(ename, name, text),                                           \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),    \
           0, SCROLL_CLASS, prob, 0, 5, cost, 0, 0, 0, 0, 6, HI_PAPER)
SCROLL("enchant armor", "防具附魔",              "ZELGO MER",  1,  63,  80),
SCROLL("destroy armor", "防具毁坏",         "JUYED AWK YACC",  1,  45, 100),
SCROLL("confuse monster", "混乱怪物",                 "NR 9",  1,  53, 100),
SCROLL("scare monster", "恐吓怪物",   "XIXAXA XOXAXA XUXAXA",  1,  35, 100),
SCROLL("remove curse", "解除诅咒",             "PRATYAVAYAH",  1,  65,  80),
SCROLL("enchant weapon", "武器附魔",         "DAIYEN FOOELS",  1,  80,  60),
SCROLL("create monster", "制造怪物",       "LEP GEX VEN ZEA",  1,  45, 200),
SCROLL("taming", "驯化",                   "PRIRUTSENIE",  1,  15, 200),
SCROLL("genocide", "灭绝",                  "ELBIB YLOH",  1,  15, 300),
SCROLL("light", "光亮",                 "VERR YED HORRE",  1,  90,  50),
SCROLL("teleportation", "传送",        "VENZAR BORGAVVE",  1,  55, 100),
SCROLL("gold detection", "金钱探测",                 "THARR",  1,  33, 100),
SCROLL("food detection", "食物探测",               "YUM YUM",  1,  25, 100),
SCROLL("identify", "鉴定",                  "KERNOD WEL",  1, 180,  20),
SCROLL("magic mapping", "魔法地图",              "ELAM EBOW",  1,  45, 100),
SCROLL("amnesia", "失忆",                   "DUAM XNAHT",  1,  35, 200),
SCROLL("fire", "火",                  "ANDOVA BEGARIN",  1,  30, 100),
SCROLL("earth", "大地",                          "KIRJE",  1,  18, 200),
SCROLL("punishment", "惩罚",            "VE FORBRYDERNE",  1,  15, 300),
SCROLL("charging", "充能",                "HACKEM MUCHE",  1,  15, 300),
SCROLL("stinking cloud", "臭云",             "VELOX NEB",  1,  15, 300),
    /* Extra descriptions, shuffled into use at start of new game.
     * Code in win/share/tilemap.c depends on SCR_STINKING_CLOUD preceding
     * these and on how many of them there are.  If a real scroll gets added
     * after stinking cloud or the number of extra descriptions changes,
     * tilemap.c must be modified to match.
     */
SCROLL(None, None,      "FOOBIE BLETCH",  1,   0, 100),
SCROLL(None, None,              "TEMOV",  1,   0, 100),
SCROLL(None, None,         "GARVEN DEH",  1,   0, 100),
SCROLL(None, None,            "READ ME",  1,   0, 100),
SCROLL(None, None,      "ETAOIN SHRDLU",  1,   0, 100),
SCROLL(None, None,        "LOREM IPSUM",  1,   0, 100),
SCROLL(None, None,              "FNORD",  1,   0, 100), /* Illuminati */
SCROLL(None, None,            "KO BATE",  1,   0, 100), /* Kurd Lasswitz */
SCROLL(None, None,      "ABRA KA DABRA",  1,   0, 100), /* traditional incantation */
SCROLL(None, None,       "ASHPD SODALG",  1,   0, 100), /* Portal */
SCROLL(None, None,            "ZLORFIK",  1,   0, 100), /* Zak McKracken */
SCROLL(None, None,      "GNIK SISI VLE",  1,   0, 100), /* Zak McKracken */
SCROLL(None, None,    "HAPAX LEGOMENON",  1,   0, 100),
SCROLL(None, None,  "EIRIS SAZUN IDISI",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None, None,    "PHOL ENDE WODAN",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None, None,              "GHOTI",  1,   0, 100), /* pronounced as 'fish',
                                                        George Bernard Shaw */
SCROLL(None, None, "MAPIRO MAHAMA DIROMAT", 1, 0, 100), /* Wizardry */
SCROLL(None, None,  "VAS CORP BET MANI",  1,   0, 100), /* Ultima */
SCROLL(None, None,            "XOR OTA",  1,   0, 100), /* Aarne Haapakoski */
SCROLL(None, None, "STRC PRST SKRZ KRK",  1,   0, 100), /* Czech and Slovak
                                                        tongue-twister */
    /* These must come last because they have special fixed descriptions.
     */
#ifdef MAIL
SCROLL("mail", "邮件",          "有邮戳的",  0,   0,   0),  //"stamped"
#endif
SCROLL("blank paper", "空白", "无标签的",  0,  28,  60),  //"unlabeled"
#undef SCROLL

/* spellbooks ... */
/* expanding beyond 52 spells would require changes in spellcasting
   or imposition of a limit on number of spells hero can know because
   they are currently assigned successive letters, a-zA-Z, when learned */
#define SPELL(ename,name,desc,sub,prob,delay,level,mgc,dir,color)  \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(0, 0, 0, 0, mgc, 0, 0, 0, 0, 0, dir, sub, PAPER),       \
           0, SPBOOK_CLASS, prob, delay, 50, level * 100,               \
           0, 0, 0, level, 20, color)
SPELL("dig", "挖掘",             "羊皮纸",  //"parchment"
      P_MATTER_SPELL,      20,  6, 5, 1, RAY, HI_PAPER),
SPELL("magic missile", "魔法飞弹",   "牛皮纸",  //"vellum"
      P_ATTACK_SPELL,      45,  2, 2, 1, RAY, HI_PAPER),
SPELL("fireball", "火球",        "粗糙的",  //"ragged"
      P_ATTACK_SPELL,      20,  4, 4, 1, RAY, HI_PAPER),
SPELL("cone of cold", "冰锥",    "卷边",  //"dog eared"
      P_ATTACK_SPELL,      10,  7, 4, 1, RAY, HI_PAPER),
SPELL("sleep", "沉睡",           "斑驳的",  //"mottled"
      P_ENCHANTMENT_SPELL, 50,  1, 1, 1, RAY, HI_PAPER),
SPELL("finger of death", "死亡一指", "褪色的",  //"stained"
      P_ATTACK_SPELL,       5, 10, 7, 1, RAY, HI_PAPER),
SPELL("light", "光亮",           "布",  //"cloth"
      P_DIVINATION_SPELL,  45,  1, 1, 1, NODIR, HI_CLOTH),
SPELL("detect monsters", "探测怪物", "坚韧的",  //"leathery"
      P_DIVINATION_SPELL,  43,  1, 1, 1, NODIR, HI_LEATHER),
SPELL("healing", "治愈",         "白色的",  //"white"
      P_HEALING_SPELL,     40,  2, 1, 1, IMMEDIATE, CLR_WHITE),
SPELL("knock", "敲击",           "粉红的",  //"pink"
      P_MATTER_SPELL,      35,  1, 1, 1, IMMEDIATE, CLR_BRIGHT_MAGENTA),
SPELL("force bolt", "力冲击",      "红色的",  //"red"
      P_ATTACK_SPELL,      35,  2, 1, 1, IMMEDIATE, CLR_RED),
SPELL("confuse monster", "混乱怪物", "橙色的",  //"orange"
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_ORANGE),
SPELL("cure blindness", "治疗失明",  "黄色的",  //"yellow"
      P_HEALING_SPELL,     25,  2, 2, 1, IMMEDIATE, CLR_YELLOW),
SPELL("drain life", "吸血",      "天鹅绒",  //"velvet"
      P_ATTACK_SPELL,      10,  2, 2, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("slow monster", "减慢怪物",    "浅绿色",  //"light green"
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_BRIGHT_GREEN),
SPELL("wizard lock", "巫师锁",     "深绿色",  //"dark green"
      P_MATTER_SPELL,      30,  3, 2, 1, IMMEDIATE, CLR_GREEN),
SPELL("create monster", "制造怪物",  "蓝绿色",  //"turquoise"
      P_CLERIC_SPELL,      35,  3, 2, 1, NODIR, CLR_BRIGHT_CYAN),
SPELL("detect food", "探测食物",     "青色的",  //"cyan"
      P_DIVINATION_SPELL,  30,  3, 2, 1, NODIR, CLR_CYAN),
SPELL("cause fear", "造成恐惧",      "淡蓝色",  //"light blue"
      P_ENCHANTMENT_SPELL, 25,  3, 3, 1, NODIR, CLR_BRIGHT_BLUE),
SPELL("clairvoyance", "千里眼",    "深蓝色",  //"dark blue"
      P_DIVINATION_SPELL,  15,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("cure sickness", "治疗疾病",   "靛蓝色",  //"indigo"
      P_HEALING_SPELL,     32,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("charm monster", "魅惑怪物",   "洋红色",  //"magenta"
      P_ENCHANTMENT_SPELL, 20,  3, 3, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("haste self", "自我加速",      "紫色的",  //"purple"
      P_ESCAPE_SPELL,      33,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("detect unseen", "探测隐形",   "紫罗兰",  //"violet"
      P_DIVINATION_SPELL,  20,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("levitation", "飘浮",      "棕褐色",  //"tan"
      P_ESCAPE_SPELL,      20,  4, 4, 1, NODIR, CLR_BROWN),
SPELL("extra healing", "强力治愈",   "带格子",  //"plaid"
      P_HEALING_SPELL,     27,  5, 3, 1, IMMEDIATE, CLR_GREEN),
SPELL("restore ability", "恢复能力", "浅棕色",  //"light brown"
      P_HEALING_SPELL,     25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("invisibility", "隐身",    "深棕色",  //"dark brown"
      P_ESCAPE_SPELL,      25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("detect treasure", "探测宝藏", "灰色的",  //"gray"
      P_DIVINATION_SPELL,  20,  5, 4, 1, NODIR, CLR_GRAY),
SPELL("remove curse", "解除诅咒",    "皱的",  //"wrinkled"
      P_CLERIC_SPELL,      25,  5, 3, 1, NODIR, HI_PAPER),
SPELL("magic mapping", "魔法地图",   "浅灰色的",  //"dusty"
      P_DIVINATION_SPELL,  18,  7, 5, 1, NODIR, HI_PAPER),
SPELL("identify", "鉴定",        "青铜色",  //"bronze"
      P_DIVINATION_SPELL,  20,  6, 3, 1, NODIR, HI_COPPER),
SPELL("turn undead", "超度",     "紫铜色",  //"copper"
      P_CLERIC_SPELL,      16,  8, 6, 1, IMMEDIATE, HI_COPPER),
SPELL("polymorph", "变形",       "银色的",  //"silver"
      P_MATTER_SPELL,      10,  8, 6, 1, IMMEDIATE, HI_SILVER),
SPELL("teleport away", "传送",   "金色的",  //"gold"
      P_ESCAPE_SPELL,      15,  6, 6, 1, IMMEDIATE, HI_GOLD),
SPELL("create familiar", "生成宠物", "辉煌的",  //"glittering"
      P_CLERIC_SPELL,      10,  7, 6, 1, NODIR, CLR_WHITE),
SPELL("cancellation", "消除",    "闪烁的",  //"shining"
      P_MATTER_SPELL,      15,  8, 7, 1, IMMEDIATE, CLR_WHITE),
SPELL("protection", "保护",      "枯燥的",  //"dull"
      P_CLERIC_SPELL,      18,  3, 1, 1, NODIR, HI_PAPER),
SPELL("jumping", "跳跃",         "薄的",  //"thin"
      P_ESCAPE_SPELL,      20,  3, 1, 1, IMMEDIATE, HI_PAPER),
SPELL("stone to flesh", "点石成肉",  "厚的",  //"thick"
      P_HEALING_SPELL,     15,  1, 3, 1, IMMEDIATE, HI_PAPER),
#if 0 /* DEFERRED */
/* from slash'em, create a tame critter which explodes when attacking,
   damaging adjacent creatures--friend or foe--and dying in the process */
SPELL("flame sphere", "火焰球",    "油画布",  //"canvas"
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
SPELL("freeze sphere", "冷冻球",   "精装本",  //"hardcover"
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
#endif
/* books with fixed descriptions
 */
SPELL("blank paper", "白纸", "空白", P_NONE, 18, 0, 0, 0, 0, HI_PAPER),  //"plain"
/* tribute book for 3.6 */
OBJECT(OBJ("novel", "小说", "平装本"),  //"paperback"
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, HI_PAPER),
       0, SPBOOK_CLASS, 0, 0, 0, 20, 0, 0, 0, 1, 20, CLR_BRIGHT_BLUE),
/* a special, one of a kind, spellbook */
OBJECT(OBJ("Book of the Dead", "死亡之书", "纸莎草"),  //"papyrus"
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 0, 0, 20, 10000, 0, 0, 0, 7, 20, HI_PAPER),
#undef SPELL

/* wands ... */
#define WAND(ename,name,typ,prob,cost,mgc,dir,metal,color) \
    OBJECT(OBJ(ename, name, typ),                                              \
           BITS(0, 0, 1, 0, mgc, 1, 0, 0, 0, 0, dir, P_NONE, metal),    \
           0, WAND_CLASS, prob, 0, 7, cost, 0, 0, 0, 0, 30, color)
WAND("light", "光亮",           "玻璃", 95, 100, 1, NODIR, GLASS, HI_GLASS),  //"glass"
WAND("secret door detection", "暗门探测",
                        "巴沙木", 50, 150, 1, NODIR, WOOD, HI_WOOD),  //"balsa"
WAND("enlightenment", "启蒙", "水晶", 15, 150, 1, NODIR, GLASS, HI_GLASS),  //"crystal"
WAND("create monster", "制造怪物",  "枫木", 45, 200, 1, NODIR, WOOD, HI_WOOD),  //"maple"
WAND("wishing", "许愿",          "松木",  5, 500, 1, NODIR, WOOD, HI_WOOD),  //"pine"
WAND("nothing", "无",           "橡木", 25, 100, 0, IMMEDIATE, WOOD, HI_WOOD),  //"oak"
WAND("striking", "冲击",        "乌木", 75, 150, 1, IMMEDIATE, WOOD, HI_WOOD),  //"ebony"
WAND("make invisible", "隐身", "大理石", 45, 150, 1, IMMEDIATE, MINERAL, HI_MINERAL),  //"marble"
WAND("slow monster", "减慢怪物",      "锡制", 50, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"tin"
WAND("speed monster", "加速怪物",   "黄铜", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),  //"brass"
WAND("undead turning", "超度", "铜制", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),  //"copper"
WAND("polymorph", "变形",      "银制", 45, 200, 1, IMMEDIATE, SILVER, HI_SILVER),  //"silver"
WAND("cancellation", "消除", "白金", 45, 200, 1, IMMEDIATE, PLATINUM, CLR_WHITE),  //"platinum"
WAND("teleportation", "传送", "铱金", 45, 200, 1, IMMEDIATE, METAL,  //"iridium"
                                                             CLR_BRIGHT_CYAN),
WAND("opening", "解锁",          "锌制", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"zinc"
WAND("locking", "上锁",      "铝制", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"aluminum"
WAND("probing", "侦查",       "铀制", 30, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"uranium"
WAND("digging", "挖掘",          "铁制", 55, 150, 1, RAY, IRON, HI_METAL),  //"iron"
WAND("magic missile", "魔法飞弹",   "钢铁", 50, 150, 1, RAY, IRON, HI_METAL),  //"steel"
WAND("fire", "火焰",        "六角形", 40, 175, 1, RAY, IRON, HI_METAL),  //"hexagonal"
WAND("cold", "寒冷",            "短", 40, 175, 1, RAY, IRON, HI_METAL),  //"short"
WAND("sleep", "沉睡",           "符文", 50, 175, 1, RAY, IRON, HI_METAL),  //"runed"
WAND("death", "死亡",            "长",  5, 500, 1, RAY, IRON, HI_METAL),  //"long"
WAND("lightning", "闪电",      "弧形", 40, 175, 1, RAY, IRON, HI_METAL),  //"curved"
/* extra descriptions, shuffled into use at start of new game */
WAND(None, None,             "分叉",  0, 150, 1, 0, WOOD, HI_WOOD),  //"forked"
WAND(None, None,             "尖顶",  0, 150, 1, 0, IRON, HI_METAL),  //"spiked"
WAND(None, None,            "宝石",  0, 150, 1, 0, IRON, HI_MINERAL),  //"jeweled"
#undef WAND

/* coins ... - so far, gold is all there is */
#define COIN(ename,name,prob,metal,worth) \
    OBJECT(OBJ(ename, name, None),                                        \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, metal),    \
           0, COIN_CLASS, prob, 0, 1, worth, 0, 0, 0, 0, 0, HI_GOLD)
COIN("gold piece", "金币", 1000, GOLD, 1),
#undef COIN

/* gems ... - includes stones and rocks but not boulders */
#define GEM(ename,name,desc,prob,wt,gval,nutr,mohs,glass,color) \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0,                              \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, 1, gval, 3, 3, 0, 0, nutr, color)
#define ROCK(ename,name,desc,kn,prob,wt,gval,sdam,ldam,mgc,nutr,mohs,glass,color) \
    OBJECT(OBJ(ename, name, desc),                                             \
           BITS(kn, 1, 0, 0, mgc, 0, 0, 0, 0,                           \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, wt, gval, sdam, ldam, 0, 0, nutr, color)
GEM("dilithium crystal", "双锂水晶", "白色的",  2, 1, 4500, 15,  5, GEMSTONE, CLR_WHITE),  //"white"
GEM("diamond", "钻石",           "白色的",  3, 1, 4000, 15, 10, GEMSTONE, CLR_WHITE),  //"white"
GEM("ruby", "红宝石",                "红色的",  4, 1, 3500, 15,  9, GEMSTONE, CLR_RED),  //"red"
GEM("jacinth", "红锆石",          "橙色的",  3, 1, 3250, 15,  9, GEMSTONE, CLR_ORANGE),  //"orange"
GEM("sapphire", "蓝宝石",           "蓝色的",  4, 1, 3000, 15,  9, GEMSTONE, CLR_BLUE),  //"blue"
GEM("black opal", "黑蛋白石",        "黑色的",  3, 1, 2500, 15,  8, GEMSTONE, CLR_BLACK),  //"black"
GEM("emerald", "祖母绿",           "绿色的",  5, 1, 2500, 15,  8, GEMSTONE, CLR_GREEN),  //"green"
GEM("turquoise", "绿松石",         "绿色的",  6, 1, 2000, 15,  6, GEMSTONE, CLR_GREEN),  //"green"
GEM("citrine", "黄水晶",          "黄色的",  4, 1, 1500, 15,  6, GEMSTONE, CLR_YELLOW),  //"yellow"
GEM("aquamarine", "海蓝宝石",        "绿色的",  6, 1, 1500, 15,  8, GEMSTONE, CLR_GREEN),  //"green"
GEM("amber", "琥珀",   "杏色的",  8, 1, 1000, 15,  2, GEMSTONE, CLR_BROWN),  //"yellowish brown"
GEM("topaz", "黄宝石",   "杏色的", 10, 1,  900, 15,  8, GEMSTONE, CLR_BROWN),  //"yellowish brown"
GEM("jet", "黑玉",               "黑色的",  6, 1,  850, 15,  7, GEMSTONE, CLR_BLACK),  //"black"
GEM("opal", "蛋白石",              "白色的", 12, 1,  800, 15,  6, GEMSTONE, CLR_WHITE),  //"white"
GEM("chrysoberyl", "金绿玉",      "黄色的",  8, 1,  700, 15,  5, GEMSTONE, CLR_YELLOW),  //"yellow"
GEM("garnet", "石榴石",              "红色的", 12, 1,  700, 15,  7, GEMSTONE, CLR_RED),  //"red"
GEM("amethyst", "紫水晶",         "紫色的", 14, 1,  600, 15,  7, GEMSTONE, CLR_MAGENTA),  //"violet"
GEM("jasper", "碧玉",              "红色的", 15, 1,  500, 15,  7, GEMSTONE, CLR_RED),  //"red"
GEM("fluorite", "萤石",         "紫色的", 15, 1,  400, 15,  4, GEMSTONE, CLR_MAGENTA),  //"violet"
GEM("obsidian", "黑曜石",          "黑色的",  9, 1,  200, 15,  6, GEMSTONE, CLR_BLACK),  //"black"
GEM("agate", "玛瑙",            "橙色的", 12, 1,  200, 15,  6, GEMSTONE, CLR_ORANGE),  //"orange"
GEM("jade", "翡翠",              "绿色的", 10, 1,  300, 15,  6, GEMSTONE, CLR_GREEN),  //"green"
GEM("worthless piece of white glass", "毫无价值的一块白色玻璃", "白色的",  //"white"
    77, 1, 0, 6, 5, GLASS, CLR_WHITE),
GEM("worthless piece of blue glass", "毫无价值的一块蓝色玻璃", "蓝色的",  //"blue"
    77, 1, 0, 6, 5, GLASS, CLR_BLUE),
GEM("worthless piece of red glass", "毫无价值的一块红色玻璃", "红色的",  //"red"
    77, 1, 0, 6, 5, GLASS, CLR_RED),
GEM("worthless piece of yellowish brown glass", "毫无价值的一块杏色玻璃", "杏色的",  //"yellowish brown"
    77, 1, 0, 6, 5, GLASS, CLR_BROWN),
GEM("worthless piece of orange glass", "毫无价值的一块橙色玻璃", "橙色的",  //"orange"
    76, 1, 0, 6, 5, GLASS, CLR_ORANGE),
GEM("worthless piece of yellow glass", "毫无价值的一块黄色玻璃", "黄色的",  //"yellow"
    77, 1, 0, 6, 5, GLASS, CLR_YELLOW),
GEM("worthless piece of black glass", "毫无价值的一块黑色玻璃", "黑色的",  //"black"
    76, 1, 0, 6, 5, GLASS, CLR_BLACK),
GEM("worthless piece of green glass", "毫无价值的一块绿色玻璃", "绿色的",  //"green"
    77, 1, 0, 6, 5, GLASS, CLR_GREEN),
GEM("worthless piece of violet glass", "毫无价值的一块紫色玻璃", "紫色的",  //"violet"
    77, 1, 0, 6, 5, GLASS, CLR_MAGENTA),

/* Placement note: there is a wishable subrange for
 * "gray stones" in the o_ranges[] array in objnam.c
 * that is currently everything between luckstones and flint
 * (inclusive).
 */
ROCK("luckstone", "幸运石", "灰色的",  0,  10,  10, 60, 3, 3, 1, 10, 7, MINERAL, CLR_GRAY),  //"gray"
ROCK("loadstone", "天然磁石", "灰色的",  0,  10, 500,  1, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),  //"gray"
ROCK("touchstone", "试金石", "灰色的", 0,   8,  10, 45, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),  //"gray"
ROCK("flint", "打火石", "灰色的",      0,  10,  10,  1, 6, 6, 0, 10, 7, MINERAL, CLR_GRAY),  //"gray"
ROCK("rock", "岩石", None,         1, 100,  10,  0, 3, 3, 0, 10, 7, MINERAL, CLR_GRAY),
#undef GEM
#undef ROCK

/* miscellaneous ... */
/* Note: boulders and rocks are not normally created at random; the
 * probabilities only come into effect when you try to polymorph them.
 * Boulders weigh more than MAX_CARR_CAP; statues use corpsenm to take
 * on a specific type and may act as containers (both affect weight).
 */
OBJECT(OBJ("boulder", "巨石", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 100, 0, 6000, 0, 20, 20, 0, 0, 2000, HI_MINERAL),
OBJECT(OBJ("statue", "雕像", None),
       BITS(1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 900, 0, 2500, 0, 20, 20, 0, 0, 2500, CLR_WHITE),

OBJECT(OBJ("heavy iron ball", "沉重的铁球", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       BALL_CLASS, 1000, 0, 480, 10, 25, 25, 0, 0, 200, HI_METAL),
        /* +d4 when "very heavy" */
OBJECT(OBJ("iron chain", "铁链", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       CHAIN_CLASS, 1000, 0, 120, 0, 4, 4, 0, 0, 200, HI_METAL),
        /* +1 both l & s */

/* Venom is normally a transitory missile (spit by various creatures)
 * but can be wished for in wizard mode so could occur in bones data.
 */
OBJECT(OBJ("blinding venom", "致盲毒液", "飞溅的毒液"),  //"splash of venom"
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 0, 0, 0, 0, 0, HI_ORGANIC),
OBJECT(OBJ("acid venom", "酸性毒液", "飞溅的毒液"),  //"splash of venom"
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 6, 6, 0, 0, 0, HI_ORGANIC),
        /* +d6 small or large */

/* fencepost, the deadly Array Terminator -- name [1st arg] *must* be NULL */
OBJECT(OBJ(None, None, None),
       BITS(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0), 0,
       ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
}; /* objects[] */

#ifndef OBJECTS_PASS_2_

/* perform recursive compilation for second structure */
#undef OBJ
#undef OBJECT
#define OBJECTS_PASS_2_
#include "objects.c"

/* clang-format on */
/* *INDENT-ON* */

void NDECL(objects_init);

/* dummy routine used to force linkage */
void
objects_init()
{
    return;
}

#endif /* !OBJECTS_PASS_2_ */

/*objects.c*/
