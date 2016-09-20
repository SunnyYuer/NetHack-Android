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
#define OBJ(name,desc)  name, desc
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
OBJECT(OBJ("strange object", None),
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, 0),
       0, ILLOBJ_CLASS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),

/* weapons ... */
#define WEAPON(name,desc,kn,mg,bi,prob,wt,                \
               cost,sdam,ldam,hitbon,typ,sub,metal,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, mg, 1, 0, 0, 1, 0, 0, bi, 0, typ, sub, metal),  \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define PROJECTILE(name,desc,kn,prob,wt,                  \
                   cost,sdam,ldam,hitbon,metal,sub,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 1, 1, 0, 0, 1, 0, 0, 0, 0, PIERCE, sub, metal), \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, sdam, ldam, hitbon, 0, wt, color)
#define BOW(name,desc,kn,prob,wt,cost,hitbon,metal,sub,color) \
    OBJECT(OBJ(name,desc),                                          \
           BITS(kn, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, sub, metal),      \
           0, WEAPON_CLASS, prob, 0, wt,                            \
           cost, 2, 2, hitbon, 0, wt, color)

/* Note: for weapons that don't do an even die of damage (ex. 2-7 or 3-18)
   the extra damage is added on in weapon.c, not here! */

#define P PIERCE
#define S SLASH
#define B WHACK

/* missiles; materiel reflects the arrowhead, not the shaft */
PROJECTILE("箭", None,  //"arrow"
           1, 55, 1, 2, 6, 6, 0,        IRON, -P_BOW, HI_METAL),
PROJECTILE("精灵箭", "符文箭",  //"elven arrow", "runed arrow"
           0, 20, 1, 2, 7, 6, 0,        WOOD, -P_BOW, HI_WOOD),
PROJECTILE("兽人箭", "粗糙箭",  //"orcish arrow", "crude arrow"
           0, 20, 1, 2, 5, 6, 0,        IRON, -P_BOW, CLR_BLACK),
PROJECTILE("银箭", None,  //"silver arrow"
           1, 12, 1, 5, 6, 6, 0,        SILVER, -P_BOW, HI_SILVER),
PROJECTILE("矢", "竹箭",  //"ya", "bamboo arrow"
           0, 15, 1, 4, 7, 7, 1,        METAL, -P_BOW, HI_METAL),
PROJECTILE("弩箭", None,  //"crossbow bolt"
           1, 55, 1, 2, 4, 6, 0,        IRON, -P_CROSSBOW, HI_METAL),

/* missiles that don't use a launcher */
WEAPON("飞镖", None,  //"dart"
       1, 1, 0, 60,   1,   2,  3,  2, 0, P,   -P_DART, IRON, HI_METAL),
WEAPON("手里剑", "投掷镖",  //"shuriken", "throwing star"
       0, 1, 0, 35,   1,   5,  8,  6, 2, P,   -P_SHURIKEN, IRON, HI_METAL),
WEAPON("回飞镖", None,  //"boomerang"
       1, 1, 0, 15,   5,  20,  9,  9, 0, 0,   -P_BOOMERANG, WOOD, HI_WOOD),

/* spears [note: javelin used to have a separate skill from spears,
   because the latter are primarily stabbing weapons rather than
   throwing ones; but for playability, they've been merged together
   under spear skill and spears can now be thrown like javelins] */
WEAPON("矛", None,  //"spear"
       1, 1, 0, 50,  30,   3,  6,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("精灵矛", "符文矛",  //"elven spear", "runed spear"
       0, 1, 0, 10,  30,   3,  7,  8, 0, P,   P_SPEAR, WOOD, HI_WOOD),
WEAPON("兽人矛", "粗糙矛",  //"orcish spear", "crude spear"
       0, 1, 0, 13,  30,   3,  5,  8, 0, P,   P_SPEAR, IRON, CLR_BLACK),
WEAPON("矮人矛", "结实矛",  //"dwarvish spear", "stout spear"
       0, 1, 0, 12,  35,   3,  8,  8, 0, P,   P_SPEAR, IRON, HI_METAL),
WEAPON("银矛", None,  //"silver spear"
       1, 1, 0,  2,  36,  40,  6,  8, 0, P,   P_SPEAR, SILVER, HI_SILVER),
WEAPON("标枪", "投掷矛",  //"javelin", "throwing spear"
       0, 1, 0, 10,  20,   3,  6,  6, 0, P,   P_SPEAR, IRON, HI_METAL),

/* spearish; doesn't stack, not intended to be thrown */
WEAPON("三叉矛", None,  //"trident"
       1, 0, 0,  8,  25,   5,  6,  4, 0, P,   P_TRIDENT, IRON, HI_METAL),
        /* +1 small, +2d4 large */

/* blades; all stack */
WEAPON("匕首", None,  //"dagger"
       1, 1, 0, 30,  10,   4,  4,  3, 2, P,   P_DAGGER, IRON, HI_METAL),
WEAPON("精灵匕首", "符文匕首",  //"elven dagger", "runed dagger"
       0, 1, 0, 10,  10,   4,  5,  3, 2, P,   P_DAGGER, WOOD, HI_WOOD),
WEAPON("兽人匕首", "粗糙匕首",  //"orcish dagger", "crude dagger"
       0, 1, 0, 12,  10,   4,  3,  3, 2, P,   P_DAGGER, IRON, CLR_BLACK),
WEAPON("银匕首", None,  //"silver dagger"
       1, 1, 0,  3,  12,  40,  4,  3, 2, P,   P_DAGGER, SILVER, HI_SILVER),
WEAPON("仪式刀", None,  //"athame"
       1, 1, 0,  0,  10,   4,  4,  3, 2, S,   P_DAGGER, IRON, HI_METAL),
WEAPON("手术刀", None,  //"scalpel"
       1, 1, 0,  0,   5,   6,  3,  3, 2, S,   P_KNIFE, METAL, HI_METAL),
WEAPON("小刀", None,  //"knife"
       1, 1, 0, 20,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
WEAPON("小剑", None,  //"stiletto"
       1, 1, 0,  5,   5,   4,  3,  2, 0, P|S, P_KNIFE, IRON, HI_METAL),
/* 3.6: worm teeth and crysknives now stack;
   when a stack of teeth is enchanted at once, they fuse into one crysknife;
   when a stack of crysknives drops, the whole stack reverts to teeth */
WEAPON("蠕虫齿", None,  //"worm tooth"
       1, 1, 0,  0,  20,   2,  2,  2, 0, 0,   P_KNIFE, 0, CLR_WHITE),
WEAPON("迅捷小刀", None,  //"crysknife"
       1, 1, 0,  0,  20, 100, 10, 10, 3, P,   P_KNIFE, MINERAL, CLR_WHITE),

/* axes */
WEAPON("斧头", None,  //"axe"
       1, 0, 0, 40,  60,   8,  6,  4, 0, S,   P_AXE, IRON, HI_METAL),
WEAPON("战斧", "双头斧",       /* "double-bitted"? */  //"battle-axe", "double-headed axe"
       0, 0, 1, 10, 120,  40,  8,  6, 0, S,   P_AXE, IRON, HI_METAL),

/* swords */
WEAPON("短剑", None,  //"short sword"
       1, 0, 0,  8,  30,  10,  6,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("精灵短剑", "符文短剑",  //"elven short sword", "runed short sword"
       0, 0, 0,  2,  30,  10,  8,  8, 0, P,   P_SHORT_SWORD, WOOD, HI_WOOD),
WEAPON("兽人短剑", "粗糙短剑",  //"orcish short sword", "crude short sword"
       0, 0, 0,  3,  30,  10,  5,  8, 0, P,   P_SHORT_SWORD, IRON, CLR_BLACK),
WEAPON("矮人短剑", "宽阔短剑",  //"dwarvish short sword", "broad short sword"
       0, 0, 0,  2,  30,  10,  7,  8, 0, P,   P_SHORT_SWORD, IRON, HI_METAL),
WEAPON("短弯刀", "弯刀",  //"scimitar", "curved sword"
       0, 0, 0, 15,  40,  15,  8,  8, 0, S,   P_SCIMITAR, IRON, HI_METAL),
WEAPON("银剑", None,  //"silver saber"
       1, 0, 0,  6,  40,  75,  8,  8, 0, S,   P_SABER, SILVER, HI_SILVER),
WEAPON("大刀", None,  //"broadsword"
       1, 0, 0,  8,  70,  10,  4,  6, 0, S,   P_BROAD_SWORD, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("精灵大刀", "符文大刀",  //"elven broadsword", "runed broadsword"
       0, 0, 0,  4,  70,  10,  6,  6, 0, S,   P_BROAD_SWORD, WOOD, HI_WOOD),
        /* +d4 small, +1 large */
WEAPON("长剑", None,  //"long sword"
       1, 0, 0, 50,  40,  15,  8, 12, 0, S,   P_LONG_SWORD, IRON, HI_METAL),
WEAPON("双手剑", None,  //"two-handed sword"
       1, 0, 1, 22, 150,  50, 12,  6, 0, S,   P_TWO_HANDED_SWORD,
                                                            IRON, HI_METAL),
        /* +2d6 large */
WEAPON("武士刀", "日本刀",  //"katana", "samurai sword"
       0, 0, 0,  4,  40,  80, 10, 12, 1, S,   P_LONG_SWORD, IRON, HI_METAL),
/* special swords set up for artifacts */
WEAPON("武士剑", "武士长剑",  //"tsurugi", "long samurai sword"
       0, 0, 1,  0,  60, 500, 16,  8, 2, S,   P_TWO_HANDED_SWORD,
                                                            METAL, HI_METAL),
        /* +2d6 large */
WEAPON("符文剑", "符文大刀",  //"runesword", "runed broadsword"
       0, 0, 0,  0,  40, 300,  4,  6, 0, S,   P_BROAD_SWORD, IRON, CLR_BLACK),
        /* +d4 small, +1 large; Stormbringer: +5d2 +d8 from level drain */

/* polearms */
/* spear-type */
WEAPON("戟", "粗俗长柄武器",  //"partisan", "vulgar polearm"
       0, 0, 1,  5,  80,  10,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 large */
WEAPON("三叉戟", "大长柄武器",  //"ranseur", "hilted polearm"
       0, 0, 1,  5,  50,   6,  4,  4, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("大战戟", "长柄叉",  //"spetum", "forked polearm"
       0, 0, 1,  5,  50,   5,  6,  6, 0, P,   P_POLEARMS, IRON, HI_METAL),
        /* +1 small, +d6 large */
WEAPON("剑刃戟", "单刃长柄武器",  //"glaive", "single-edged polearm"
       0, 0, 1,  8,  75,   6,  6, 10, 0, S,   P_POLEARMS, IRON, HI_METAL),
WEAPON("长戟", None,  //"lance"
       1, 0, 0,  4, 180,  10,  6,  8, 0, P,   P_LANCE, IRON, HI_METAL),
        /* +2d10 when jousting with lance as primary weapon */
/* axe-type */
WEAPON("斧枪", "成角的战斧",  //"halberd", "angled poleaxe"
       0, 0, 1,  8, 150,  10, 10,  6, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d6 large */
WEAPON("大战斧", "长战斧",  //"bardiche", "long poleaxe"
       0, 0, 1,  4, 120,   7,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small, +2d4 large */
WEAPON("长斧", "极切肉刀",  //"voulge", "pole cleaver"
       0, 0, 1,  4, 125,   5,  4,  4, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +d4 both */
WEAPON("矮人鹤嘴锄", "宽阔锄头",  //"dwarvish mattock", "broad pick"
       0, 0, 1, 13, 120,  50, 12,  8, -1, B,  P_PICK_AXE, IRON, HI_METAL),
/* curved/hooked */
WEAPON("斩矛", "极镰刀",  //"fauchard", "pole sickle"
       0, 0, 1,  6,  60,   5,  6,  8, 0, P|S, P_POLEARMS, IRON, HI_METAL),
WEAPON("长勾刀", "修枝刀",  //"guisarme", "pruning hook"
       0, 0, 1,  6,  80,   5,  4,  8, 0, S,   P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("倒勾戟", "弯曲长柄武器",  //"bill-guisarme", "hooked polearm"
       0, 0, 1,  4, 120,   7,  4, 10, 0, P|S, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
/* other */
WEAPON("苜蓿锤", "分叉长柄武器",  //"lucern hammer", "pronged polearm"
       0, 0, 1,  5, 150,   7,  4,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),
        /* +1d4 small */
WEAPON("鸦啄战锤", "喙长柄武器",  //"bec de corbin", "beaked polearm"
       0, 0, 1,  4, 100,   8,  8,  6, 0, B|P, P_POLEARMS, IRON, HI_METAL),

/* bludgeons */
WEAPON("权杖", None,  //"mace"
       1, 0, 0, 40,  30,   5,  6,  6, 0, B,   P_MACE, IRON, HI_METAL),
        /* +1 small */
WEAPON("流星锤", None,  //"morning star"
       1, 0, 0, 12, 120,  10,  4,  6, 0, B,   P_MORNING_STAR, IRON, HI_METAL),
        /* +d4 small, +1 large */
WEAPON("战锤", None,  //"war hammer"
       1, 0, 0, 15,  50,   5,  4,  4, 0, B,   P_HAMMER, IRON, HI_METAL),
        /* +1 small */
WEAPON("棍棒", None,  //"club"
       1, 0, 0, 12,  30,   3,  6,  3, 0, B,   P_CLUB, WOOD, HI_WOOD),
WEAPON("橡胶管", None,  //"rubber hose"
       1, 0, 0,  0,  20,   3,  4,  3, 0, B,   P_WHIP, PLASTIC, CLR_BROWN),
WEAPON("铁头木棒", "棒子",  //"quarterstaff", "staff"
       0, 0, 1, 11,  40,   5,  6,  6, 0, B,   P_QUARTERSTAFF, WOOD, HI_WOOD),
/* two-piece */
WEAPON("链棒", "皮带棍棒",  //"aklys", "thonged club"
       0, 0, 0,  8,  15,   4,  6,  3, 0, B,   P_CLUB, IRON, HI_METAL),
WEAPON("连枷", None,  //"flail"
       1, 0, 0, 40,  15,   4,  6,  4, 0, B,   P_FLAIL, IRON, HI_METAL),
        /* +1 small, +1d4 large */

/* misc */
WEAPON("牛鞭", None,  //"bullwhip"
       1, 0, 0,  2,  20,   4,  2,  1, 0, 0,   P_WHIP, LEATHER, CLR_BROWN),

/* bows */
BOW("弓", None,               1, 24, 30, 60, 0, WOOD, P_BOW, HI_WOOD),  //"bow"
BOW("精灵弓", "符文弓",  0, 12, 30, 60, 0, WOOD, P_BOW, HI_WOOD),  //"elven bow", "runed bow"
BOW("兽人弓", "粗糙弓", 0, 12, 30, 60, 0, WOOD, P_BOW, CLR_BLACK),  //"orcish bow", "crude bow"
BOW("弩", "长弓",        0,  0, 30, 60, 0, WOOD, P_BOW, HI_WOOD),  //"yumi", "long bow"
BOW("投石器", None,             1, 40,  3, 20, 0, LEATHER, P_SLING, HI_LEATHER),  //"sling"
BOW("十字弓", None,          1, 45, 50, 40, 0, WOOD, P_CROSSBOW, HI_WOOD),  //"crossbow"

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
#define ARMOR(name,desc,kn,mgc,blk,power,prob,delay,wt,  \
              cost,ac,can,sub,metal,c)                   \
    OBJECT(OBJ(name, desc),                                         \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, blk, 0, 0, sub, metal),  \
           power, ARMOR_CLASS, prob, delay, wt,                     \
           cost, 0, 0, 10 - ac, can, wt, c)
#define HELM(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_HELM, metal, c)
#define CLOAK(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_CLOAK, metal, c)
#define SHIELD(name,desc,kn,mgc,blk,power,prob,delay,wt,cost,ac,can,metal,c) \
    ARMOR(name, desc, kn, mgc, blk, power, prob, delay, wt, \
          cost, ac, can, ARM_SHIELD, metal, c)
#define GLOVES(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_GLOVES, metal, c)
#define BOOTS(name,desc,kn,mgc,power,prob,delay,wt,cost,ac,can,metal,c)  \
    ARMOR(name, desc, kn, mgc, 0, power, prob, delay, wt,  \
          cost, ac, can, ARM_BOOTS, metal, c)

/* helmets */
HELM("精灵皮帽", "皮帽",  //"elven leather helm", "leather hat"
     0, 0,           0,  6, 1,  3,  8,  9, 0, LEATHER, HI_LEATHER),
HELM("兽人头盔", "铁骷髅帽",  //"orcish helm", "iron skull cap"
     0, 0,           0,  6, 1, 30, 10,  9, 0, IRON, CLR_BLACK),
HELM("矮人铁头盔", "安全帽",  //"dwarvish iron helm", "hard hat"
     0, 0,           0,  6, 1, 40, 20,  8, 0, IRON, HI_METAL),
HELM("软呢帽", None,  //"fedora"
     1, 0,           0,  0, 0,  3,  1, 10, 0, CLOTH, CLR_BROWN),
HELM("巫师帽", "圆锥形帽",  //"cornuthaum", "conical hat"
     0, 1, CLAIRVOYANT,  3, 1,  4, 80, 10, 1, CLOTH, CLR_BLUE),
        /* name coined by devteam; confers clairvoyance for wizards,
           blocks clairvoyance if worn by role other than wizard */
HELM("愚人帽", "圆锥形帽",  //"dunce cap", "conical hat"
     0, 1,           0,  3, 1,  4,  1, 10, 0, CLOTH, CLR_BLUE),
HELM("瘪罐", None,  //"dented pot"
     1, 0,           0,  2, 0, 10,  8,  9, 0, IRON, CLR_BLACK),
/* with shuffled appearances... */
HELM("钢盔", "羽饰头盔",  //"helmet", "plumed helmet"
     0, 0,           0, 10, 1, 30, 10,  9, 0, IRON, HI_METAL),
HELM("卓越头盔", "蚀刻的头盔",  //"helm of brilliance", "etched helmet"
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, CLR_GREEN),
HELM("敌对阵营头盔", "羽冠头盔",  //"helm of opposite alignment", "crested helmet"
     0, 1,           0,  6, 1, 50, 50,  9, 0, IRON, HI_METAL),
HELM("感知头盔", "檐帽头盔",  //"helm of telepathy", "visored helmet"
     0, 1,     TELEPAT,  2, 1, 50, 50,  9, 0, IRON, HI_METAL),

/* suits of armor */
/*
 * There is code in polyself.c that assumes (1) and (2).
 * There is code in obj.h, objnam.c, mon.c, read.c that assumes (2).
 *      (1) The dragon scale mails and the dragon scales are together.
 *      (2) That the order of the dragon scale mail and dragon scales
 *          is the the same as order of dragons defined in monst.c.
 */
#define DRGN_ARMR(name,mgc,power,cost,ac,color)  \
    ARMOR(name, None, 1, mgc, 1, power, 0, 5, 40,  \
          cost, ac, 0, ARM_SUIT, DRAGON_HIDE, color)
/* 3.4.1: dragon scale mail reclassified as "magic" since magic is
   needed to create them */
DRGN_ARMR("灰龙鳞甲",    1, ANTIMAGIC,  1200, 1, CLR_GRAY),  //"gray dragon scale mail"
DRGN_ARMR("银龙鳞甲",  1, REFLECTING, 1200, 1, DRAGON_SILVER),  //"silver dragon scale mail"
#if 0 /* DEFERRED */
DRGN_ARMR("闪光龙鳞甲", 1, DISPLACED, 1200, 1, CLR_CYAN),  //"shimmering dragon scale mail"
#endif
DRGN_ARMR("红龙鳞甲",     1, FIRE_RES,    900, 1, CLR_RED),  //"red dragon scale mail"
DRGN_ARMR("白龙鳞甲",   1, COLD_RES,    900, 1, CLR_WHITE),  //"white dragon scale mail"
DRGN_ARMR("橙龙鳞甲",  1, SLEEP_RES,   900, 1, CLR_ORANGE),  //"orange dragon scale mail"
DRGN_ARMR("黑龙鳞甲",   1, DISINT_RES, 1200, 1, CLR_BLACK),  //"black dragon scale mail"
DRGN_ARMR("蓝龙鳞甲",    1, SHOCK_RES,   900, 1, CLR_BLUE),  //"blue dragon scale mail"
DRGN_ARMR("绿龙鳞甲",   1, POISON_RES,  900, 1, CLR_GREEN),  //"green dragon scale mail"
DRGN_ARMR("黄龙鳞甲",  1, ACID_RES,    900, 1, CLR_YELLOW),  //"yellow dragon scale mail"
/* For now, only dragons leave these. */
/* 3.4.1: dragon scales left classified as "non-magic"; they confer
   magical properties but are produced "naturally" */
DRGN_ARMR("灰龙鳞",        0, ANTIMAGIC,   700, 7, CLR_GRAY),  //"gray dragon scales"
DRGN_ARMR("银龙鳞",      0, REFLECTING,  700, 7, DRAGON_SILVER),  //"silver dragon scales"
#if 0 /* DEFERRED */
DRGN_ARMR("闪光龙鳞",  0, DISPLACED,   700, 7, CLR_CYAN),  //"shimmering dragon scales"
#endif
DRGN_ARMR("红龙鳞",         0, FIRE_RES,    500, 7, CLR_RED),  //"red dragon scales"
DRGN_ARMR("白龙鳞",       0, COLD_RES,    500, 7, CLR_WHITE),  //"white dragon scales"
DRGN_ARMR("橙龙鳞",      0, SLEEP_RES,   500, 7, CLR_ORANGE),  //"orange dragon scales"
DRGN_ARMR("黑龙鳞",       0, DISINT_RES,  700, 7, CLR_BLACK),  //"black dragon scales"
DRGN_ARMR("蓝龙鳞",        0, SHOCK_RES,   500, 7, CLR_BLUE),  //"blue dragon scales"
DRGN_ARMR("绿龙鳞",       0, POISON_RES,  500, 7, CLR_GREEN),  //"green dragon scales"
DRGN_ARMR("黄龙鳞",      0, ACID_RES,    500, 7, CLR_YELLOW),  //"yellow dragon scales"
#undef DRGN_ARMR
/* other suits */
ARMOR("板甲", None,  //"plate mail"
      1, 0, 1,  0, 44, 5, 450, 600,  3, 2,  ARM_SUIT, IRON, HI_METAL),
ARMOR("水晶板甲", None,  //"crystal plate mail"
      1, 0, 1,  0, 10, 5, 450, 820,  3, 2,  ARM_SUIT, GLASS, CLR_WHITE),
ARMOR("青铜板甲", None,  //"bronze plate mail"
      1, 0, 1,  0, 25, 5, 450, 400,  4, 1,  ARM_SUIT, COPPER, HI_COPPER),
ARMOR("板条甲", None,  //"splint mail"
      1, 0, 1,  0, 62, 5, 400,  80,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("带链甲", None,  //"banded mail"
      1, 0, 1,  0, 72, 5, 350,  90,  4, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("矮人秘银胶衣", None,  //"dwarvish mithril-coat"
      1, 0, 0,  0, 10, 1, 150, 240,  4, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("精灵秘银胶衣", None,  //"elven mithril-coat"
      1, 0, 0,  0, 15, 1, 150, 240,  5, 2,  ARM_SUIT, MITHRIL, HI_SILVER),
ARMOR("锁子甲", None,  //"chain mail"
      1, 0, 0,  0, 72, 5, 300,  75,  5, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("兽人锁子甲", "粗糙锁子甲",  //"orcish chain mail", "crude chain mail"
      0, 0, 0,  0, 20, 5, 300,  75,  6, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("鳞甲", None,  //"scale mail"
      1, 0, 0,  0, 72, 5, 250,  45,  6, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("嵌皮甲", None,  //"studded leather armor"
      1, 0, 0,  0, 72, 3, 200,  15,  7, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("锁环甲", None,  //"ring mail"
      1, 0, 0,  0, 72, 5, 250, 100,  7, 1,  ARM_SUIT, IRON, HI_METAL),
ARMOR("兽人锁环甲", "粗糙锁环甲",  //"orcish ring mail", "crude ring mail"
      0, 0, 0,  0, 20, 5, 250,  80,  8, 1,  ARM_SUIT, IRON, CLR_BLACK),
ARMOR("皮甲", None,  //"leather armor"
      1, 0, 0,  0, 82, 3, 150,   5,  8, 1,  ARM_SUIT, LEATHER, HI_LEATHER),
ARMOR("皮夹克", None,  //"leather jacket"
      1, 0, 0,  0, 12, 0,  30,  10,  9, 0,  ARM_SUIT, LEATHER, CLR_BLACK),

/* shirts */
ARMOR("夏威夷衬衫", None,  //"Hawaiian shirt"
      1, 0, 0,  0,  8, 0,   5,   3, 10, 0,  ARM_SHIRT, CLOTH, CLR_MAGENTA),
ARMOR("T 恤衫", None,  //"T-shirt"
      1, 0, 0,  0,  2, 0,   5,   2, 10, 0,  ARM_SHIRT, CLOTH, CLR_WHITE),

/* cloaks */
CLOAK("木乃伊绷带", None,  //"mummy wrapping"
      1, 0,          0,  0, 0,  3,  2, 10, 1,  CLOTH, CLR_GRAY),
        /* worn mummy wrapping blocks invisibility */
CLOAK("精灵斗篷", "褪色的斗篷",  //"elven cloak", "faded pall"
      0, 1,    STEALTH,  8, 0, 10, 60,  9, 1,  CLOTH, CLR_BLACK),
CLOAK("兽人斗篷", "粗糙的小斗蓬",  //"orcish cloak", "coarse mantelet"
      0, 0,          0,  8, 0, 10, 40, 10, 1,  CLOTH, CLR_BLACK),
CLOAK("矮人斗篷", "带帽斗篷",  //"dwarvish cloak", "hooded cloak"
      0, 0,          0,  8, 0, 10, 50, 10, 1,  CLOTH, HI_CLOTH),
CLOAK("油布斗篷", "湿滑的斗篷",  //"oilskin cloak", "slippery cloak"
      0, 0,          0,  8, 0, 10, 50,  9, 2,  CLOTH, HI_CLOTH),
CLOAK("长袍", None,  //"robe"
      1, 1,          0,  3, 0, 15, 50,  8, 2,  CLOTH, CLR_RED),
        /* robe was adopted from slash'em, where it's worn as a suit
           rather than as a cloak and there are several variations */
CLOAK("炼金术罩衫", "围裙",  //"alchemy smock", "apron"
      0, 1, POISON_RES,  9, 0, 10, 50,  9, 1,  CLOTH, CLR_WHITE),
CLOAK("皮斗篷", None,  //"leather cloak"
      1, 0,          0,  8, 0, 15, 40,  9, 1,  LEATHER, CLR_BROWN),
/* with shuffled appearances... */
CLOAK("保护斗篷", "破烂的斗篷",  //"cloak of protection", "tattered cape"
      0, 1, PROTECTION,  9, 0, 10, 50,  7, 3,  CLOTH, HI_CLOTH),
        /* cloak of protection is now the only item conferring MC 3 */
CLOAK("隐身斗篷", "夜礼服斗篷",  //"cloak of invisibility", "opera cloak"
      0, 1,      INVIS, 10, 0, 10, 60,  9, 1,  CLOTH, CLR_BRIGHT_MAGENTA),
CLOAK("魔法抵抗斗篷", "装饰性长袍",  //"cloak of magic resistance", "ornamental cope"
      0, 1,  ANTIMAGIC,  2, 0, 10, 60,  9, 1,  CLOTH, CLR_WHITE),
        /*  'cope' is not a spelling mistake... leave it be */
CLOAK("幻影斗篷", "一块布",  //"cloak of displacement", "piece of cloth"
      0, 1,  DISPLACED, 10, 0, 10, 50,  9, 1,  CLOTH, HI_CLOTH),

/* shields */
SHIELD("小盾牌", None,  //"small shield"
       1, 0, 0,          0, 6, 0,  30,  3, 9, 0,  WOOD, HI_WOOD),
SHIELD("精灵盾", "蓝绿盾",  //"elven shield", "blue and green shield"
       0, 0, 0,          0, 2, 0,  40,  7, 8, 0,  WOOD, CLR_GREEN),
SHIELD("强兽人盾", "白色手盾",  //"Uruk-hai shield", "white-handed shield"
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, HI_METAL),
SHIELD("兽人盾", "红眼盾",  //"orcish shield", "red-eyed shield"
       0, 0, 0,          0, 2, 0,  50,  7, 9, 0,  IRON, CLR_RED),
SHIELD("大盾牌", None,  //"large shield"
       1, 0, 1,          0, 7, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("矮人圆盾", "大圆盾",  //"dwarvish roundshield", "large round shield"
       0, 0, 0,          0, 4, 0, 100, 10, 8, 0,  IRON, HI_METAL),
SHIELD("反射之盾", "抛光银盾",  //"shield of reflection", "polished silver shield"
       0, 1, 0, REFLECTING, 3, 0,  50, 50, 8, 0,  SILVER, HI_SILVER),

/* gloves */
/* These have their color but not material shuffled, so the IRON must
 * stay CLR_BROWN (== HI_LEATHER) even though it's normally either
 * HI_METAL or CLR_BLACK.  All have shuffled descriptions.
 */
GLOVES("皮手套", "残破的手套",  //"leather gloves", "old gloves"
       0, 0,        0, 16, 1, 10,  8, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("笨拙手套", "加衬手套",  //"gauntlets of fumbling", "padded gloves"
       0, 1, FUMBLING,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),
GLOVES("力量手套", "骑手手套",  //"gauntlets of power", "riding gloves"
       0, 1,        0,  8, 1, 30, 50, 9, 0,  IRON, CLR_BROWN),
GLOVES("敏捷手套", "击剑手套",  //"gauntlets of dexterity", "fencing gloves"
       0, 1,        0,  8, 1, 10, 50, 9, 0,  LEATHER, HI_LEATHER),

/* boots */
BOOTS("低跟鞋", "步行鞋",  //"low boots", "walking shoes"
      0, 0,          0, 25, 2, 10,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("铁鞋", "硬底鞋",  //"iron shoes", "hard shoes"
      0, 0,          0,  7, 2, 50, 16, 8, 0, IRON, HI_METAL),
BOOTS("高筒靴", "长筒靴",  //"high boots", "jackboots"
      0, 0,          0, 15, 2, 20, 12, 8, 0, LEATHER, HI_LEATHER),
/* with shuffled appearances... */
BOOTS("速度靴", "战斗靴",  //"speed boots", "combat boots"
      0, 1,       FAST, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("水上步靴", "丛林靴",  //"water walking boots", "jungle boots"
      0, 1,   WWALKING, 12, 2, 15, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("跳跃靴", "登山靴",  //"jumping boots", "hiking boots"
      0, 1,    JUMPING, 12, 2, 20, 50, 9, 0, LEATHER, HI_LEATHER),
BOOTS("精灵靴", "泥靴",  //"elven boots", "mud boots"
      0, 1,    STEALTH, 12, 2, 15,  8, 9, 0, LEATHER, HI_LEATHER),
BOOTS("踢靴", "带扣靴",  //"kicking boots", "buckled boots"
      0, 1,          0, 12, 2, 50,  8, 9, 0, IRON, CLR_BROWN),
        /* CLR_BROWN for same reason as gauntlets of power */
BOOTS("笨拙靴", "马靴",  //"fumble boots", "riding boots"
      0, 1,   FUMBLING, 12, 2, 20, 30, 9, 0, LEATHER, HI_LEATHER),
BOOTS("飘浮靴", "雪地靴",  //"levitation boots", "snow boots"
      0, 1, LEVITATION, 12, 2, 15, 30, 9, 0, LEATHER, HI_LEATHER),
#undef HELM
#undef CLOAK
#undef SHIELD
#undef GLOVES
#undef BOOTS
#undef ARMOR

/* rings ... */
#define RING(name,stone,power,cost,mgc,spec,mohs,metal,color) \
    OBJECT(OBJ(name, stone),                                          \
           BITS(0, 0, spec, 0, mgc, spec, 0, 0, 0,                    \
                HARDGEM(mohs), 0, P_NONE, metal),                     \
           power, RING_CLASS, 0, 0, 3, cost, 0, 0, 0, 0, 15, color)
RING("装饰品", "木制",  //"adornment", "wooden"
     ADORNED,                  100, 1, 1, 2, WOOD, HI_WOOD),
RING("增加力量", "花岗石",  //"gain strength", "granite"
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("增加体质", "蛋白石",  //"gain constitution", "opal"
     0,                        150, 1, 1, 7, MINERAL, HI_MINERAL),
RING("增加精确", "黏土",  //"increase accuracy", "clay"
     0,                        150, 1, 1, 4, MINERAL, CLR_RED),
RING("增加伤害", "珊瑚",  //"increase damage", "coral"
     0,                        150, 1, 1, 4, MINERAL, CLR_ORANGE),
RING("保护", "黑玛瑙",  //"protection", "black onyx"
     PROTECTION,               100, 1, 1, 7, MINERAL, CLR_BLACK),
        /* 'PROTECTION' intrinsic enhances MC from worn armor by +1,
           regardless of ring's enchantment; wearing a second ring of
           protection (or even one ring of protection combined with
           cloak of protection) doesn't give a second MC boost */
RING("再生", "月石",  //"regeneration", "moonstone"
     REGENERATION,             200, 1, 0,  6, MINERAL, HI_MINERAL),
RING("搜索", "虎眼石",  //"searching", "tiger eye"
     SEARCHING,                200, 1, 0,  6, GEMSTONE, CLR_BROWN),
RING("潜行", "翡翠",  //"stealth", "jade"
     STEALTH,                  100, 1, 0,  6, GEMSTONE, CLR_GREEN),
RING("维持能力", "青铜",  //"sustain ability", "bronze"
     FIXED_ABIL,               100, 1, 0,  4, COPPER, HI_COPPER),
RING("飘浮", "玛瑙",  //"levitation", "agate"
     LEVITATION,               200, 1, 0,  7, GEMSTONE, CLR_RED),
RING("饥饿", "黄宝石",  //"hunger", "topaz"
     HUNGER,                   100, 1, 0,  8, GEMSTONE, CLR_CYAN),
RING("激怒怪物", "蓝宝石",  //"aggravate monster", "sapphire"
     AGGRAVATE_MONSTER,        150, 1, 0,  9, GEMSTONE, CLR_BLUE),
RING("冲突", "红宝石",  //"conflict", "ruby"
     CONFLICT,                 300, 1, 0,  9, GEMSTONE, CLR_RED),
RING("警报", "钻石",  //"warning", "diamond"
     WARNING,                  100, 1, 0, 10, GEMSTONE, CLR_WHITE),
RING("抗毒", "珍珠",  //"poison resistance", "pearl"
     POISON_RES,               150, 1, 0,  4, BONE, CLR_WHITE),
RING("抗火", "铁",  //"fire resistance", "iron"
     FIRE_RES,                 200, 1, 0,  5, IRON, HI_METAL),
RING("抗寒", "黄铜",  //"cold resistance", "brass"
     COLD_RES,                 150, 1, 0,  4, COPPER, HI_COPPER),
RING("抗电", "铜",  //"shock resistance", "copper"
     SHOCK_RES,                150, 1, 0,  3, COPPER, HI_COPPER),
RING("自由行动", "扭曲的",  //"free action", "twisted"
     FREE_ACTION,              200, 1, 0,  6, IRON, HI_METAL),
RING("慢消化", "钢铁",  //"slow digestion", "steel"
     SLOW_DIGESTION,           200, 1, 0,  8, IRON, HI_METAL),
RING("传送", "银",  //"teleportation", "silver"
     TELEPORT,                 200, 1, 0,  3, SILVER, HI_SILVER),
RING("传送控制", "金",  //"teleport control", "gold"
     TELEPORT_CONTROL,         300, 1, 0,  3, GOLD, HI_GOLD),
RING("变形", "象牙",  //"polymorph", "ivory"
     POLYMORPH,                300, 1, 0,  4, BONE, CLR_WHITE),
RING("变形控制", "祖母绿",  //"polymorph control", "emerald"
     POLYMORPH_CONTROL,        300, 1, 0,  8, GEMSTONE, CLR_BRIGHT_GREEN),
RING("隐身", "金属",  //"invisibility", "wire"
     INVIS,                    150, 1, 0,  5, IRON, HI_METAL),
RING("看见隐形", "订婚",  //"see invisible", "engagement"
     SEE_INVIS,                150, 1, 0,  5, IRON, HI_METAL),
RING("变形保护", "闪耀的",  //"protection from shape changers", "shiny"
     PROT_FROM_SHAPE_CHANGERS, 100, 1, 0,  5, IRON, CLR_BRIGHT_CYAN),
#undef RING

/* amulets ... - THE Amulet comes last because it is special */
#define AMULET(name,desc,power,prob) \
    OBJECT(OBJ(name, desc),                                            \
           BITS(0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, P_NONE, IRON),        \
           power, AMULET_CLASS, prob, 0, 20, 150, 0, 0, 0, 0, 20, HI_METAL)
AMULET("感知护身符",                "圆形", TELEPAT, 175),  //"amulet of ESP",                "circular"
AMULET("复活护身符",       "球形", LIFESAVED, 75),  //"amulet of life saving",       "spherical"
AMULET("窒息护身符",          "椭圆形", STRANGLED, 135),  //"amulet of strangulation",          "oval"
AMULET("深度睡眠护身符",    "三角形", SLEEPY, 135),  //"amulet of restful sleep",    "triangular"
AMULET("毒抗护身符",        "锥状", POISON_RES, 165),  //"amulet versus poison",        "pyramidal"
AMULET("变性护身符",               "方形", 0, 130),  //"amulet of change",               "square"
AMULET("阻止变形护身符",          "凹形", UNCHANGING, 45),  //"amulet of unchanging",          "concave"
AMULET("反射护身符",        "六角形", REFLECTING, 75),  //"amulet of reflection",        "hexagonal"
AMULET("魔法呼吸护身符", "八角形", MAGICAL_BREATHING, 65),  //"amulet of magical breathing", "octagonal"
/* fixed descriptions; description duplication is deliberate;
 * fake one must come before real one because selection for
 * description shuffling stops when a non-magic amulet is encountered
 */
OBJECT(OBJ("岩德护身符的廉价塑料仿制品",  //"cheap plastic imitation of the Amulet of Yendor"
           "岩德护身符"),  //"Amulet of Yendor"
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, PLASTIC),
       0, AMULET_CLASS, 0, 0, 20, 0, 0, 0, 0, 0, 1, HI_METAL),
OBJECT(OBJ("岩德护身符", /* note: description == name */
           "岩德护身符"),
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, MITHRIL),
       0, AMULET_CLASS, 0, 0, 20, 30000, 0, 0, 0, 0, 20, HI_METAL),
#undef AMULET

/* tools ... */
/* tools with weapon characteristics come last */
#define TOOL(name,desc,kn,mrg,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, mrg, chg, 0, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat), \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define CONTAINER(name,desc,kn,mgc,chg,prob,wt,cost,mat,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, chg, 1, mgc, chg, 0, 0, 0, 0, 0, P_NONE, mat),   \
           0, TOOL_CLASS, prob, 0, wt, cost, 0, 0, 0, 0, wt, color)
#define WEPTOOL(name,desc,kn,mgc,bi,prob,wt,cost,sdam,ldam,hitbon,sub,mat,clr)\
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 0, 1, 0, mgc, 1, 0, 0, bi, 0, hitbon, sub, mat),    \
           0, TOOL_CLASS, prob, 0, wt, cost, sdam, ldam, hitbon, 0, wt, clr)
/* containers */
CONTAINER("大箱子",       None, 1, 0, 0, 40, 350,   8, WOOD, HI_WOOD),  //"large box"
CONTAINER("箱子",           None, 1, 0, 0, 35, 600,  16, WOOD, HI_WOOD),  //"chest"
CONTAINER("冰盒子",         None, 1, 0, 0,  5, 900,  42, PLASTIC, CLR_WHITE),  //"ice box"
CONTAINER("布袋",           "袋子", 0, 0, 0, 35,  15,   2, CLOTH, HI_CLOTH),  //"sack"
CONTAINER("防水袋",   "袋子", 0, 0, 0,  5,  15, 100, CLOTH, HI_CLOTH),  //"oilskin sack",   "bag"
CONTAINER("次元袋", "袋子", 0, 1, 0, 20,  15, 100, CLOTH, HI_CLOTH),  //"bag of holding", "bag"
CONTAINER("魔术袋",  "袋子", 0, 1, 1, 20,  15, 100, CLOTH, HI_CLOTH),  //"bag of tricks",  "bag"
#undef CONTAINER

/* lock opening tools */
TOOL("万能钥匙",       "钥匙", 0, 0, 0, 0, 80,  3, 10, IRON, HI_METAL),  //"skeleton key",       "key"
TOOL("开锁器",           None, 1, 0, 0, 0, 60,  4, 20, IRON, HI_METAL),  //"lock pick"
TOOL("信用卡",         None, 1, 0, 0, 0, 15,  1, 10, PLASTIC, CLR_WHITE),  //"credit card"
/* light sources */
TOOL("牛油烛",   "蜡烛", 0, 1, 0, 0, 20,  2, 10, WAX, CLR_WHITE),  //"tallow candle",   "candle"
TOOL("蜡状蜡烛",      "蜡烛", 0, 1, 0, 0,  5,  2, 20, WAX, CLR_WHITE),  //"wax candle",      "candle"
TOOL("黄铜灯笼",       None, 1, 0, 0, 0, 30, 30, 12, COPPER, CLR_YELLOW),  //"brass lantern"
TOOL("油灯",          "灯", 0, 0, 0, 0, 45, 20, 10, COPPER, CLR_YELLOW),  //"oil lamp",          "lamp"
TOOL("神灯",        "灯", 0, 0, 1, 0, 15, 20, 50, COPPER, CLR_YELLOW),  //"magic lamp",        "lamp"
/* other tools */
TOOL("高档相机",    None, 1, 0, 0, 1, 15, 12,200, PLASTIC, CLR_BLACK),  //"expensive camera"
TOOL("反光镜",   "镜子", 0, 0, 0, 0, 45, 13, 10, GLASS, HI_SILVER),  //"mirror",   "looking glass"
TOOL("水晶球", "玻璃球", 0, 0, 1, 1, 15,150, 60, GLASS, HI_GLASS),  //"crystal ball", "glass orb"
TOOL("眼镜",              None, 1, 0, 0, 0,  5,  3, 80, GLASS, HI_GLASS),  //"lenses"
TOOL("眼罩",           None, 1, 0, 0, 0, 50,  2, 20, CLOTH, CLR_BLACK),  //"blindfold"
TOOL("毛巾",               None, 1, 0, 0, 0, 50,  2, 50, CLOTH, CLR_MAGENTA),  //"towel"
TOOL("鞍",              None, 1, 0, 0, 0,  5,200,150, LEATHER, HI_LEATHER),  //"saddle"
TOOL("狗链",               None, 1, 0, 0, 0, 65, 12, 20, LEATHER, HI_LEATHER),  //"leash"
TOOL("听诊器",         None, 1, 0, 0, 0, 25,  4, 75, IRON, HI_METAL),  //"stethoscope"
TOOL("装罐器",         None, 1, 0, 0, 1, 15,100, 30, IRON, HI_METAL),  //"tinning kit"
TOOL("开罐器",          None, 1, 0, 0, 0, 35,  4, 30, IRON, HI_METAL),  //"tin opener"
TOOL("油脂罐",       None, 1, 0, 0, 1, 15, 15, 20, IRON, HI_METAL),  //"can of grease"
TOOL("小雕像",            None, 1, 0, 1, 0, 25, 50, 80, MINERAL, HI_MINERAL),  //"figurine"
        /* monster type specified by obj->corpsenm */
TOOL("魔笔",        None, 1, 0, 1, 1, 15,  2, 50, PLASTIC, CLR_RED),  //"magic marker"
/* traps */
TOOL("地雷",           None, 1, 0, 0, 0, 0, 300,180, IRON, CLR_RED),  //"land mine"
TOOL("捕兽夹",            None, 1, 0, 0, 0, 0, 200, 60, IRON, HI_METAL),  //"beartrap"
/* instruments;
   "If tin whistles are made out of tin, what do they make foghorns out of?" */
TOOL("六孔哨",    "口哨", 0, 0, 0, 0,100, 3, 10, METAL, HI_METAL),  //"tin whistle",    "whistle"
TOOL("魔法口哨",  "口哨", 0, 0, 1, 0, 30, 3, 10, METAL, HI_METAL),  //"magic whistle",  "whistle"
TOOL("木笛",     "长笛", 0, 0, 0, 0,  4, 5, 12, WOOD, HI_WOOD),  //"wooden flute",     "flute"
TOOL("魔笛",      "长笛", 0, 0, 1, 1,  2, 5, 36, WOOD, HI_WOOD),  //"magic flute",      "flute"
TOOL("加工号角",       "号角", 0, 0, 0, 0,  5, 18, 15, BONE, CLR_WHITE),  //"tooled horn",       "horn"
TOOL("冰霜号角",        "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"frost horn",        "horn"
TOOL("火焰号角",         "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"fire horn",         "horn"
TOOL("丰饶之角",    "号角", 0, 0, 1, 1,  2, 18, 50, BONE, CLR_WHITE),  //"horn of plenty",    "horn"
        /* horn, but not an instrument */
TOOL("木竖琴",       "竖琴", 0, 0, 0, 0,  4, 30, 50, WOOD, HI_WOOD),  //"wooden harp",       "harp"
TOOL("魔幻竖琴",        "竖琴", 0, 0, 1, 1,  2, 30, 50, WOOD, HI_WOOD),  //"magic harp",        "harp"
TOOL("铃",                None, 1, 0, 0, 0,  2, 30, 50, COPPER, HI_COPPER),  //"bell"
TOOL("军号",               None, 1, 0, 0, 0,  4, 10, 15, COPPER, HI_COPPER),  //"bugle"
TOOL("皮革鼓",      "鼓", 0, 0, 0, 0,  4, 25, 25, LEATHER, HI_LEATHER),  //"leather drum",      "drum"
TOOL("地震鼓","鼓", 0, 0, 1, 1,  2, 25, 25, LEATHER, HI_LEATHER),  //"drum of earthquake","drum"
/* tools useful as weapons */
WEPTOOL("鹤嘴锄", None,  //"pick-axe"
        1, 0, 0, 20, 100,  50,  6,  3, WHACK,  P_PICK_AXE, IRON, HI_METAL),
WEPTOOL("爪钩", "铁钩",  //"grappling hook", "iron hook"
        0, 0, 0,  5,  30,  50,  2,  6, WHACK,  P_FLAIL,    IRON, HI_METAL),
WEPTOOL("独角兽的角", None,  //"unicorn horn"
        1, 1, 1,  0,  20, 100, 12, 12, PIERCE, P_UNICORN_HORN,
                                                           BONE, CLR_WHITE),
        /* 3.4.1: unicorn horn left classified as "magic" */
/* two unique tools;
 * not artifacts, despite the comment which used to be here
 */
OBJECT(OBJ("祈祷烛台", "烛台"),  //"Candelabrum of Invocation", "candelabrum"
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, GOLD),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 200, HI_GOLD),
OBJECT(OBJ("开启之铃", "银铃"),  //"Bell of Opening", "silver bell"
       BITS(0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, P_NONE, SILVER),
       0, TOOL_CLASS, 0, 0, 10, 5000, 0, 0, 0, 0, 50, HI_SILVER),
#undef TOOL
#undef WEPTOOL

/* Comestibles ... */
#define FOOD(name, prob, delay, wt, unk, tin, nutrition, color)         \
    OBJECT(OBJ(name, None),                                       \
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
FOOD("牛肚",        140,  2, 10, 0, FLESH, 200, CLR_BROWN),  //"tripe ration"
FOOD("尸体",                0,  1,  0, 0, FLESH,   0, CLR_BROWN),  //"corpse"
FOOD("蛋",                  85,  1,  1, 1, FLESH,  80, CLR_WHITE),  //"egg"
FOOD("肉丸",              0,  1,  1, 0, FLESH,   5, CLR_BROWN),  //"meatball"
FOOD("肉棍",            0,  1,  1, 0, FLESH,   5, CLR_BROWN),  //"meat stick"
FOOD("大块肉",    0, 20,400, 0, FLESH,2000, CLR_BROWN),  //"huge chunk of meat"
/* special case because it's not mergable */
OBJECT(OBJ("肉环", None),  //"meat ring"
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FLESH),
       0, FOOD_CLASS, 0, 1, 5, 1, 0, 0, 0, 0, 5, CLR_BROWN),
/* pudding 'corpses' will turn into these and combine;
   must be in same order as the pudding monsters */
FOOD("灰色软泥团",     0,  2, 20, 0, FLESH,  20, CLR_GRAY),  //"glob of gray ooze"
FOOD("棕色布丁团", 0,  2, 20, 0, FLESH,  20, CLR_BROWN),  //"glob of brown pudding"
FOOD("绿色粘液团",   0,  2, 20, 0, FLESH,  20, CLR_GREEN),  //"glob of green slime"
FOOD("黑色布丁团", 0,  2, 20, 0, FLESH,  20, CLR_BLACK),  //"glob of black pudding"

/* fruits & veggies */
FOOD("海藻叶子",            0,  1,  1, 0, VEGGY,  30, CLR_GREEN),  //"kelp frond"
FOOD("桉叶",       3,  1,  1, 0, VEGGY,  30, CLR_GREEN),  //"eucalyptus leaf"
FOOD("苹果",                15,  1,  2, 0, VEGGY,  50, CLR_RED),  //"apple"
FOOD("橙子",               10,  1,  2, 0, VEGGY,  80, CLR_ORANGE),  //"orange"
FOOD("梨",                 10,  1,  2, 0, VEGGY,  50, CLR_BRIGHT_GREEN),  //"pear"
FOOD("甜瓜",                10,  1,  5, 0, VEGGY, 100, CLR_BRIGHT_GREEN),  //"melon"
FOOD("香蕉",               10,  1,  2, 0, VEGGY,  80, CLR_YELLOW),  //"banana"
FOOD("胡萝卜",               15,  1,  2, 0, VEGGY,  50, CLR_ORANGE),  //"carrot"
FOOD("附子草枝",    7,  1,  1, 0, VEGGY,  40, CLR_GREEN),  //"sprig of wolfsbane"
FOOD("蒜瓣",       7,  1,  1, 0, VEGGY,  40, CLR_WHITE),  //"clove of garlic"
/* name of slime mold is changed based on player's OPTION=fruit:something
   and bones data might have differently named ones from prior games */
FOOD("黏液",           75,  1,  5, 0, VEGGY, 250, HI_ORGANIC),  //"slime mold"

/* people food */
FOOD("蜂王浆",   0,  1,  2, 0, VEGGY, 200, CLR_YELLOW),  //"lump of royal jelly"
FOOD("奶油派",            25,  1, 10, 0, VEGGY, 100, CLR_WHITE),  //"cream pie"
FOOD("条形糖果",            13,  1,  2, 0, VEGGY, 100, CLR_BROWN),  //"candy bar"
FOOD("幸运饼干",       55,  1,  1, 0, VEGGY,  40, CLR_YELLOW),  //"fortune cookie"
FOOD("煎饼",              25,  2,  2, 0, VEGGY, 200, CLR_YELLOW),  //"pancake"
FOOD("兰巴斯片",         20,  2,  5, 0, VEGGY, 800, CLR_WHITE),  //"lembas wafer"
FOOD("压缩口粮",          20,  3, 15, 0, VEGGY, 600, HI_ORGANIC),  //"cram ration"
FOOD("口粮",         380,  5, 20, 0, VEGGY, 800, HI_ORGANIC),  //"food ration"
FOOD("K- 口粮",              0,  1, 10, 0, VEGGY, 400, HI_ORGANIC),  //"K-ration"
FOOD("C- 口粮",              0,  1, 10, 0, VEGGY, 300, HI_ORGANIC),  //"C-ration"
/* tins have type specified by obj->spe (+1 for spinach, other implies
   flesh; negative specifies preparation method {homemade,boiled,&c})
   and by obj->corpsenm (type of monster flesh) */
FOOD("罐头",                  75,  0, 10, 1, METAL,   0, HI_METAL),  //"tin"
#undef FOOD

/* potions ... */
#define POTION(name,desc,mgc,power,prob,cost,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, GLASS),      \
           power, POTION_CLASS, prob, 0, 20, cost, 0, 0, 0, 0, 10, color)
POTION("增强能力",           "深红色",  1, 0, 42, 300, CLR_RED),  //"gain ability",           "ruby"
POTION("恢复能力",        "粉红色",  1, 0, 40, 100, CLR_BRIGHT_MAGENTA),  //"restore ability",        "pink"
POTION("混乱",            "橙色",  1, CONFUSION, 42, 100, CLR_ORANGE),  //"confusion",            "orange"
POTION("失明",            "黄色",  1, BLINDED, 40, 150, CLR_YELLOW),  //"blindness",            "yellow"
POTION("麻痹",           "翠绿色",  1, 0, 42, 300, CLR_BRIGHT_GREEN),  //"paralysis",           "emerald"
POTION("加速",            "深绿色",  1, FAST, 42, 200, CLR_GREEN),  //"speed",            "dark green"
POTION("飘浮",             "蓝绿色",  1, LEVITATION, 42, 200, CLR_CYAN),  //"levitation",             "cyan"
POTION("幻觉",      "天蓝色",  1, HALLUC, 40, 100, CLR_CYAN),  //"hallucination",      "sky blue"
POTION("隐身", "亮蓝色",  1, INVIS, 40, 150, CLR_BRIGHT_BLUE),  //"invisibility", "brilliant blue"
POTION("看见隐形",       "洋红色",  1, SEE_INVIS, 42, 50, CLR_MAGENTA),  //"see invisible",       "magenta"
POTION("治愈",          "紫红色",  1, 0, 57, 100, CLR_MAGENTA),  //"healing",          "purple-red"
POTION("强力治愈",          "深褐色",  1, 0, 47, 100, CLR_RED),  //"extra healing",          "puce"
POTION("升级",            "乳白色",  1, 0, 20, 300, CLR_WHITE),  //"gain level",            "milky"
POTION("启蒙",        "涡旋形",  1, 0, 20, 200, CLR_BROWN),  //"enlightenment",        "swirly"
POTION("怪物探测",    "多泡的",  1, 0, 40, 150, CLR_WHITE),  //"monster detection",    "bubbly"
POTION("物品探测",      "冒烟的",  1, 0, 42, 150, CLR_GRAY),  //"object detection",      "smoky"
POTION("获得能量",          "混浊的",  1, 0, 42, 150, CLR_WHITE),  //"gain energy",          "cloudy"
POTION("沉睡",       "沸腾的",  1, 0, 42, 100, CLR_GRAY),  //"sleeping",       "effervescent"
POTION("完全治愈",          "黑色的",  1, 0, 10, 200, CLR_BLACK),  //"full healing",          "black"
POTION("变形",            "金色",  1, 0, 10, 200, CLR_YELLOW),  //"polymorph",            "golden"
POTION("酒",                 "棕色",  0, 0, 42,  50, CLR_BROWN),  //"booze",                 "brown"
POTION("疾病",              "起泡的",  0, 0, 42,  50, CLR_CYAN),  //"sickness",              "fizzy"
POTION("果汁",            "深色的",  0, 0, 42,  50, CLR_BLACK),  //"fruit juice",            "dark"
POTION("酸",                  "白色的",  0, 0, 10, 250, CLR_WHITE),  //"acid",                  "white"
POTION("油",                   "黑暗的",  0, 0, 30, 250, CLR_BROWN),  //"oil",                   "murky"
/* fixed description
 */
POTION("水",                 "清澈的",  0, 0, 92, 100, CLR_CYAN),  //"water",                 "clear"
#undef POTION

/* scrolls ... */
#define SCROLL(name,text,mgc,prob,cost) \
    OBJECT(OBJ(name, text),                                           \
           BITS(0, 1, 0, 0, mgc, 0, 0, 0, 0, 0, 0, P_NONE, PAPER),    \
           0, SCROLL_CLASS, prob, 0, 5, cost, 0, 0, 0, 0, 6, HI_PAPER)
SCROLL("防具附魔",              "ZELGO MER",  1,  63,  80),  //"enchant armor"
SCROLL("防具毁坏",         "JUYED AWK YACC",  1,  45, 100),  //"destroy armor"
SCROLL("混乱怪物",                 "NR 9",  1,  53, 100),  //"confuse monster"
SCROLL("恐吓怪物",   "XIXAXA XOXAXA XUXAXA",  1,  35, 100),  //"scare monster"
SCROLL("解除诅咒",             "PRATYAVAYAH",  1,  65,  80),  //"remove curse"
SCROLL("武器附魔",         "DAIYEN FOOELS",  1,  80,  60),  //"enchant weapon"
SCROLL("制造怪物",       "LEP GEX VEN ZEA",  1,  45, 200),  //"create monster"
SCROLL("驯化",                   "PRIRUTSENIE",  1,  15, 200),  //"taming"
SCROLL("灭绝",                  "ELBIB YLOH",  1,  15, 300),  //"genocide"
SCROLL("光亮",                 "VERR YED HORRE",  1,  90,  50),  //"light"
SCROLL("传送",        "VENZAR BORGAVVE",  1,  55, 100),  //"teleportation"
SCROLL("金钱探测",                 "THARR",  1,  33, 100),  //"gold detection"
SCROLL("食物探测",               "YUM YUM",  1,  25, 100),  //"food detection"
SCROLL("鉴定",                  "KERNOD WEL",  1, 180,  20),  //"identify"
SCROLL("魔法地图",              "ELAM EBOW",  1,  45, 100),  //"magic mapping"
SCROLL("失忆",                   "DUAM XNAHT",  1,  35, 200),  //"amnesia"
SCROLL("火",                  "ANDOVA BEGARIN",  1,  30, 100),  //"fire"
SCROLL("大地",                          "KIRJE",  1,  18, 200),  //"earth"
SCROLL("惩罚",            "VE FORBRYDERNE",  1,  15, 300),  //"punishment"
SCROLL("充能",                "HACKEM MUCHE",  1,  15, 300),  //"charging"
SCROLL("臭云",             "VELOX NEB",  1,  15, 300),  //"stinking cloud"
    /* Extra descriptions, shuffled into use at start of new game.
     * Code in win/share/tilemap.c depends on SCR_STINKING_CLOUD preceding
     * these and on how many of them there are.  If a real scroll gets added
     * after stinking cloud or the number of extra descriptions changes,
     * tilemap.c must be modified to match.
     */
SCROLL(None,      "FOOBIE BLETCH",  1,   0, 100),
SCROLL(None,              "TEMOV",  1,   0, 100),
SCROLL(None,         "GARVEN DEH",  1,   0, 100),
SCROLL(None,            "READ ME",  1,   0, 100),
SCROLL(None,      "ETAOIN SHRDLU",  1,   0, 100),
SCROLL(None,        "LOREM IPSUM",  1,   0, 100),
SCROLL(None,              "FNORD",  1,   0, 100), /* Illuminati */
SCROLL(None,            "KO BATE",  1,   0, 100), /* Kurd Lasswitz */
SCROLL(None,      "ABRA KA DABRA",  1,   0, 100), /* traditional incantation */
SCROLL(None,       "ASHPD SODALG",  1,   0, 100), /* Portal */
SCROLL(None,            "ZLORFIK",  1,   0, 100), /* Zak McKracken */
SCROLL(None,      "GNIK SISI VLE",  1,   0, 100), /* Zak McKracken */
SCROLL(None,    "HAPAX LEGOMENON",  1,   0, 100),
SCROLL(None,  "EIRIS SAZUN IDISI",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None,    "PHOL ENDE WODAN",  1,   0, 100), /* Merseburg Incantations */
SCROLL(None,              "GHOTI",  1,   0, 100), /* pronounced as 'fish',
                                                        George Bernard Shaw */
SCROLL(None, "MAPIRO MAHAMA DIROMAT", 1, 0, 100), /* Wizardry */
SCROLL(None,  "VAS CORP BET MANI",  1,   0, 100), /* Ultima */
SCROLL(None,            "XOR OTA",  1,   0, 100), /* Aarne Haapakoski */
SCROLL(None, "STRC PRST SKRZ KRK",  1,   0, 100), /* Czech and Slovak
                                                        tongue-twister */
    /* These must come last because they have special fixed descriptions.
     */
#ifdef MAIL
SCROLL("邮寄",          "有邮戳的",  0,   0,   0),  //"mail",          "stamped"
#endif
SCROLL("空白", "无标签的",  0,  28,  60),  //"blank paper", "unlabeled"
#undef SCROLL

/* spellbooks ... */
/* expanding beyond 52 spells would require changes in spellcasting
   or imposition of a limit on number of spells hero can know because
   they are currently assigned successive letters, a-zA-Z, when learned */
#define SPELL(name,desc,sub,prob,delay,level,mgc,dir,color)  \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 0, 0, 0, mgc, 0, 0, 0, 0, 0, dir, sub, PAPER),       \
           0, SPBOOK_CLASS, prob, delay, 50, level * 100,               \
           0, 0, 0, level, 20, color)
SPELL("挖掘",             "羊皮纸",  //"dig",             "parchment"
      P_MATTER_SPELL,      20,  6, 5, 1, RAY, HI_PAPER),
SPELL("魔法飞弹",   "牛皮纸",  //"magic missile",   "vellum"
      P_ATTACK_SPELL,      45,  2, 2, 1, RAY, HI_PAPER),
SPELL("火球",        "粗糙的",  //"fireball",        "ragged"
      P_ATTACK_SPELL,      20,  4, 4, 1, RAY, HI_PAPER),
SPELL("冰锥",    "卷边",  //"cone of cold",    "dog eared"
      P_ATTACK_SPELL,      10,  7, 4, 1, RAY, HI_PAPER),
SPELL("沉睡",           "斑驳的",  //"sleep",           "mottled"
      P_ENCHANTMENT_SPELL, 50,  1, 1, 1, RAY, HI_PAPER),
SPELL("死亡一指", "褪色的",  //"finger of death", "stained"
      P_ATTACK_SPELL,       5, 10, 7, 1, RAY, HI_PAPER),
SPELL("光亮",           "布",  //"light",           "cloth"
      P_DIVINATION_SPELL,  45,  1, 1, 1, NODIR, HI_CLOTH),
SPELL("探测怪物", "坚韧的",  //"detect monsters", "leathery"
      P_DIVINATION_SPELL,  43,  1, 1, 1, NODIR, HI_LEATHER),
SPELL("治愈",         "白色的",  //"healing",         "white"
      P_HEALING_SPELL,     40,  2, 1, 1, IMMEDIATE, CLR_WHITE),
SPELL("敲击",           "粉红的",  //"knock",           "pink"
      P_MATTER_SPELL,      35,  1, 1, 1, IMMEDIATE, CLR_BRIGHT_MAGENTA),
SPELL("魔力闪电",      "红色的",  //"force bolt",      "red"
      P_ATTACK_SPELL,      35,  2, 1, 1, IMMEDIATE, CLR_RED),
SPELL("迷惑怪物", "橙色的",  //"confuse monster", "orange"
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_ORANGE),
SPELL("治疗失明",  "黄色的",  //"cure blindness",  "yellow"
      P_HEALING_SPELL,     25,  2, 2, 1, IMMEDIATE, CLR_YELLOW),
SPELL("吸血",      "天鹅绒",  //"drain life",      "velvet"
      P_ATTACK_SPELL,      10,  2, 2, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("减慢怪物",    "浅绿色",  //"slow monster",    "light green"
      P_ENCHANTMENT_SPELL, 30,  2, 2, 1, IMMEDIATE, CLR_BRIGHT_GREEN),
SPELL("巫师锁",     "深绿色",  //"wizard lock",     "dark green"
      P_MATTER_SPELL,      30,  3, 2, 1, IMMEDIATE, CLR_GREEN),
SPELL("制造怪物",  "蓝绿色",  //"create monster",  "turquoise"
      P_CLERIC_SPELL,      35,  3, 2, 1, NODIR, CLR_BRIGHT_CYAN),
SPELL("探测食物",     "青色的",  //"detect food",     "cyan"
      P_DIVINATION_SPELL,  30,  3, 2, 1, NODIR, CLR_CYAN),
SPELL("惊恐术",      "淡蓝色",  //"cause fear",      "light blue"
      P_ENCHANTMENT_SPELL, 25,  3, 3, 1, NODIR, CLR_BRIGHT_BLUE),
SPELL("千里眼",    "深蓝色",  //"clairvoyance",    "dark blue"
      P_DIVINATION_SPELL,  15,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("治疗疾病",   "靛蓝色",  //"cure sickness",   "indigo"
      P_HEALING_SPELL,     32,  3, 3, 1, NODIR, CLR_BLUE),
SPELL("魅惑怪物",   "洋红色",  //"charm monster",   "magenta"
      P_ENCHANTMENT_SPELL, 20,  3, 3, 1, IMMEDIATE, CLR_MAGENTA),
SPELL("自我加速",      "紫色的",  //"haste self",      "purple"
      P_ESCAPE_SPELL,      33,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("探测隐形",   "紫罗兰",  //"detect unseen",   "violet"
      P_DIVINATION_SPELL,  20,  4, 3, 1, NODIR, CLR_MAGENTA),
SPELL("飘浮",      "棕褐色",  //"levitation",      "tan"
      P_ESCAPE_SPELL,      20,  4, 4, 1, NODIR, CLR_BROWN),
SPELL("强力治愈",   "带格子",  //"extra healing",   "plaid"
      P_HEALING_SPELL,     27,  5, 3, 1, IMMEDIATE, CLR_GREEN),
SPELL("恢复能力", "浅棕色",  //"restore ability", "light brown"
      P_HEALING_SPELL,     25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("隐身",    "深棕色",  //"invisibility",    "dark brown"
      P_ESCAPE_SPELL,      25,  5, 4, 1, NODIR, CLR_BROWN),
SPELL("探测宝藏", "灰色的",  //"detect treasure", "gray"
      P_DIVINATION_SPELL,  20,  5, 4, 1, NODIR, CLR_GRAY),
SPELL("解除诅咒",    "皱的",  //"remove curse",    "wrinkled"
      P_CLERIC_SPELL,      25,  5, 3, 1, NODIR, HI_PAPER),
SPELL("魔法地图",   "浅灰色的",  //"magic mapping",   "dusty"
      P_DIVINATION_SPELL,  18,  7, 5, 1, NODIR, HI_PAPER),
SPELL("鉴定",        "青铜色",  //"identify",        "bronze"
      P_DIVINATION_SPELL,  20,  6, 3, 1, NODIR, HI_COPPER),
SPELL("超度",     "紫铜色",  //"turn undead",     "copper"
      P_CLERIC_SPELL,      16,  8, 6, 1, IMMEDIATE, HI_COPPER),
SPELL("变形",       "银色的",  //"polymorph",       "silver"
      P_MATTER_SPELL,      10,  8, 6, 1, IMMEDIATE, HI_SILVER),
SPELL("传送",   "金色的",  //"teleport away",   "gold"
      P_ESCAPE_SPELL,      15,  6, 6, 1, IMMEDIATE, HI_GOLD),
SPELL("生成宠物", "辉煌的",  //"create familiar", "glittering"
      P_CLERIC_SPELL,      10,  7, 6, 1, NODIR, CLR_WHITE),
SPELL("消除",    "闪烁的",  //"cancellation",    "shining"
      P_MATTER_SPELL,      15,  8, 7, 1, IMMEDIATE, CLR_WHITE),
SPELL("保护",      "枯燥的",  //"protection",      "dull"
      P_CLERIC_SPELL,      18,  3, 1, 1, NODIR, HI_PAPER),
SPELL("跳跃",         "薄的",  //"jumping",         "thin"
      P_ESCAPE_SPELL,      20,  3, 1, 1, IMMEDIATE, HI_PAPER),
SPELL("点石成肉",  "厚的",  //"stone to flesh",  "thick"
      P_HEALING_SPELL,     15,  1, 3, 1, IMMEDIATE, HI_PAPER),
#if 0 /* DEFERRED */
/* from slash'em, create a tame critter which explodes when attacking,
   damaging adjacent creatures--friend or foe--and dying in the process */
SPELL("火焰球",    "油画布",  //"flame sphere",    "canvas"
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
SPELL("冷冻球",   "精装本",  //"freeze sphere",   "hardcover"
      P_MATTER_SPELL,      20,  2, 1, 1, NODIR, CLR_BROWN),
#endif
/* books with fixed descriptions
 */
SPELL("白纸", "空白", P_NONE, 18, 0, 0, 0, 0, HI_PAPER),  //"blank paper", "plain"
/* tribute book for 3.6 */
OBJECT(OBJ("小说", "平装本"),  //"novel", "paperback"
       BITS(0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, HI_PAPER),
       0, SPBOOK_CLASS, 0, 0, 0, 20, 0, 0, 0, 1, 20, CLR_BRIGHT_BLUE),
/* a special, one of a kind, spellbook */
OBJECT(OBJ("死亡之书", "纸莎草"),  //"Book of the Dead", "papyrus"
       BITS(0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, P_NONE, PAPER),
       0, SPBOOK_CLASS, 0, 0, 20, 10000, 0, 0, 0, 7, 20, HI_PAPER),
#undef SPELL

/* wands ... */
#define WAND(name,typ,prob,cost,mgc,dir,metal,color) \
    OBJECT(OBJ(name, typ),                                              \
           BITS(0, 0, 1, 0, mgc, 1, 0, 0, 0, 0, dir, P_NONE, metal),    \
           0, WAND_CLASS, prob, 0, 7, cost, 0, 0, 0, 0, 30, color)
WAND("光亮",           "玻璃", 95, 100, 1, NODIR, GLASS, HI_GLASS),  //"light",           "glass"
WAND("暗门探测",  //"secret door detection"
                        "巴沙木", 50, 150, 1, NODIR, WOOD, HI_WOOD),  //"balsa"
WAND("启蒙", "水晶", 15, 150, 1, NODIR, GLASS, HI_GLASS),  //"enlightenment", "crystal"
WAND("制造怪物",  "枫木", 45, 200, 1, NODIR, WOOD, HI_WOOD),  //"create monster",  "maple"
WAND("许愿",          "松木",  5, 500, 1, NODIR, WOOD, HI_WOOD),  //"wishing",          "pine"
WAND("无",           "橡木", 25, 100, 0, IMMEDIATE, WOOD, HI_WOOD),  //"nothing",           "oak"
WAND("敲击",        "乌木", 75, 150, 1, IMMEDIATE, WOOD, HI_WOOD),  //"striking",        "ebony"
WAND("隐身", "大理石", 45, 150, 1, IMMEDIATE, MINERAL, HI_MINERAL),  //"make invisible", "marble"
WAND("减慢怪物",      "锡制", 50, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"slow monster",      "tin"
WAND("加速怪物",   "黄铜", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),  //"speed monster",   "brass"
WAND("超度", "铜制", 50, 150, 1, IMMEDIATE, COPPER, HI_COPPER),  //"undead turning", "copper"
WAND("变形",      "银制", 45, 200, 1, IMMEDIATE, SILVER, HI_SILVER),  //"polymorph",      "silver"
WAND("消除", "白金", 45, 200, 1, IMMEDIATE, PLATINUM, CLR_WHITE),  //"cancellation", "platinum"
WAND("传送", "铱金", 45, 200, 1, IMMEDIATE, METAL,  //"teleportation", "iridium"
                                                             CLR_BRIGHT_CYAN),
WAND("解锁",          "锌制", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"opening",          "zinc"
WAND("上锁",      "铝制", 25, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"locking",      "aluminum"
WAND("侦查",       "铀制", 30, 150, 1, IMMEDIATE, METAL, HI_METAL),  //"probing",       "uranium"
WAND("挖掘",          "铁制", 55, 150, 1, RAY, IRON, HI_METAL),  //"digging",          "iron"
WAND("魔法飞弹",   "钢铁", 50, 150, 1, RAY, IRON, HI_METAL),  //"magic missile",   "steel"
WAND("火焰",        "六角形", 40, 175, 1, RAY, IRON, HI_METAL),  //"fire",        "hexagonal"
WAND("寒冷",            "短", 40, 175, 1, RAY, IRON, HI_METAL),  //"cold",            "short"
WAND("沉睡",           "符文", 50, 175, 1, RAY, IRON, HI_METAL),  //"sleep",           "runed"
WAND("死亡",            "长",  5, 500, 1, RAY, IRON, HI_METAL),  //"death",            "long"
WAND("闪电",      "弧形", 40, 175, 1, RAY, IRON, HI_METAL),  //"lightning",      "curved"
/* extra descriptions, shuffled into use at start of new game */
WAND(None,             "分叉",  0, 150, 1, 0, WOOD, HI_WOOD),  //"forked"
WAND(None,             "尖顶",  0, 150, 1, 0, IRON, HI_METAL),  //"spiked"
WAND(None,            "宝石",  0, 150, 1, 0, IRON, HI_MINERAL),  //"jeweled"
#undef WAND

/* coins ... - so far, gold is all there is */
#define COIN(name,prob,metal,worth) \
    OBJECT(OBJ(name, None),                                        \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, P_NONE, metal),    \
           0, COIN_CLASS, prob, 0, 1, worth, 0, 0, 0, 0, 0, HI_GOLD)
COIN("金币", 1000, GOLD, 1),  //"gold piece"
#undef COIN

/* gems ... - includes stones and rocks but not boulders */
#define GEM(name,desc,prob,wt,gval,nutr,mohs,glass,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(0, 1, 0, 0, 0, 0, 0, 0, 0,                              \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, 1, gval, 3, 3, 0, 0, nutr, color)
#define ROCK(name,desc,kn,prob,wt,gval,sdam,ldam,mgc,nutr,mohs,glass,color) \
    OBJECT(OBJ(name, desc),                                             \
           BITS(kn, 1, 0, 0, mgc, 0, 0, 0, 0,                           \
                HARDGEM(mohs), 0, -P_SLING, glass),                     \
           0, GEM_CLASS, prob, 0, wt, gval, sdam, ldam, 0, 0, nutr, color)
GEM("双锂水晶", "白色的",  2, 1, 4500, 15,  5, GEMSTONE, CLR_WHITE),  //"dilithium crystal", "white"
GEM("钻石",           "白色的",  3, 1, 4000, 15, 10, GEMSTONE, CLR_WHITE),  //"diamond",           "white"
GEM("红宝石",                "红色的",  4, 1, 3500, 15,  9, GEMSTONE, CLR_RED),  //"ruby",                "red"
GEM("红锆石",          "橙色的",  3, 1, 3250, 15,  9, GEMSTONE, CLR_ORANGE),  //"jacinth",          "orange"
GEM("蓝宝石",           "蓝色的",  4, 1, 3000, 15,  9, GEMSTONE, CLR_BLUE),  //"sapphire",           "blue"
GEM("黑蛋白石",        "黑色的",  3, 1, 2500, 15,  8, GEMSTONE, CLR_BLACK),  //"black opal",        "black"
GEM("祖母绿",           "绿色的",  5, 1, 2500, 15,  8, GEMSTONE, CLR_GREEN),  //"emerald",           "green"
GEM("绿松石",         "绿色的",  6, 1, 2000, 15,  6, GEMSTONE, CLR_GREEN),  //"turquoise",         "green"
GEM("黄水晶",          "黄色的",  4, 1, 1500, 15,  6, GEMSTONE, CLR_YELLOW),  //"citrine",          "yellow"
GEM("海蓝宝石",        "绿色的",  6, 1, 1500, 15,  8, GEMSTONE, CLR_GREEN),  //"aquamarine",        "green"
GEM("琥珀",   "杏色的",  8, 1, 1000, 15,  2, GEMSTONE, CLR_BROWN),  //"amber",   "yellowish brown"
GEM("黄宝石",   "杏色的", 10, 1,  900, 15,  8, GEMSTONE, CLR_BROWN),  //"topaz",   "yellowish brown"
GEM("黑玉",               "黑色的",  6, 1,  850, 15,  7, GEMSTONE, CLR_BLACK),  //"jet",               "black"
GEM("蛋白石",              "白色的", 12, 1,  800, 15,  6, GEMSTONE, CLR_WHITE),  //"opal",              "white"
GEM("金绿玉",      "黄色的",  8, 1,  700, 15,  5, GEMSTONE, CLR_YELLOW),  //"chrysoberyl",      "yellow"
GEM("石榴石",              "红色的", 12, 1,  700, 15,  7, GEMSTONE, CLR_RED),  //"garnet",              "red"
GEM("紫水晶",         "紫色的", 14, 1,  600, 15,  7, GEMSTONE, CLR_MAGENTA),  //"amethyst",         "violet"
GEM("碧玉",              "红色的", 15, 1,  500, 15,  7, GEMSTONE, CLR_RED),  //"jasper",              "red"
GEM("萤石",         "紫色的", 15, 1,  400, 15,  4, GEMSTONE, CLR_MAGENTA),  //"fluorite",         "violet"
GEM("黑曜石",          "黑色的",  9, 1,  200, 15,  6, GEMSTONE, CLR_BLACK),  //"obsidian",          "black"
GEM("玛瑙",            "橙色的", 12, 1,  200, 15,  6, GEMSTONE, CLR_ORANGE),  //"agate",            "orange"
GEM("翡翠",              "绿色的", 10, 1,  300, 15,  6, GEMSTONE, CLR_GREEN),  //"jade",              "green"
GEM("毫无价值的一块白色玻璃", "白色的",  //"worthless piece of white glass", "white"
    77, 1, 0, 6, 5, GLASS, CLR_WHITE),
GEM("毫无价值的一块蓝色玻璃", "蓝色的",  //"worthless piece of blue glass", "blue"
    77, 1, 0, 6, 5, GLASS, CLR_BLUE),
GEM("毫无价值的一块红色玻璃", "红色的",  //"worthless piece of red glass", "red"
    77, 1, 0, 6, 5, GLASS, CLR_RED),
GEM("毫无价值的一块杏色玻璃", "杏色的",  //"worthless piece of yellowish brown glass", "yellowish brown"
    77, 1, 0, 6, 5, GLASS, CLR_BROWN),
GEM("毫无价值的一块橙色玻璃", "橙色的",  //"worthless piece of orange glass", "orange"
    76, 1, 0, 6, 5, GLASS, CLR_ORANGE),
GEM("毫无价值的一块黄色玻璃", "黄色的",  //"worthless piece of yellow glass", "yellow"
    77, 1, 0, 6, 5, GLASS, CLR_YELLOW),
GEM("毫无价值的一块黑色玻璃", "黑色的",  //"worthless piece of black glass", "black"
    76, 1, 0, 6, 5, GLASS, CLR_BLACK),
GEM("毫无价值的一块绿色玻璃", "绿色的",  //"worthless piece of green glass", "green"
    77, 1, 0, 6, 5, GLASS, CLR_GREEN),
GEM("毫无价值的一块紫色玻璃", "紫色的",  //"worthless piece of violet glass", "violet"
    77, 1, 0, 6, 5, GLASS, CLR_MAGENTA),

/* Placement note: there is a wishable subrange for
 * "gray stones" in the o_ranges[] array in objnam.c
 * that is currently everything between luckstones and flint
 * (inclusive).
 */
ROCK("幸运石", "灰色的",  0,  10,  10, 60, 3, 3, 1, 10, 7, MINERAL, CLR_GRAY),  //"luckstone", "gray"
ROCK("天然磁石", "灰色的",  0,  10, 500,  1, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),  //"loadstone", "gray"
ROCK("试金石", "灰色的", 0,   8,  10, 45, 3, 3, 1, 10, 6, MINERAL, CLR_GRAY),  //"touchstone", "gray"
ROCK("打火石", "灰色的",      0,  10,  10,  1, 6, 6, 0, 10, 7, MINERAL, CLR_GRAY),  //"flint", "gray"
ROCK("岩石", None,         1, 100,  10,  0, 3, 3, 0, 10, 7, MINERAL, CLR_GRAY),  //"rock"
#undef GEM
#undef ROCK

/* miscellaneous ... */
/* Note: boulders and rocks are not normally created at random; the
 * probabilities only come into effect when you try to polymorph them.
 * Boulders weigh more than MAX_CARR_CAP; statues use corpsenm to take
 * on a specific type and may act as containers (both affect weight).
 */
OBJECT(OBJ("巨石", None),  //"boulder"
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 100, 0, 6000, 0, 20, 20, 0, 0, 2000, HI_MINERAL),
OBJECT(OBJ("雕像", None),  //"statue"
       BITS(1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, P_NONE, MINERAL), 0,
       ROCK_CLASS, 900, 0, 2500, 0, 20, 20, 0, 0, 2500, CLR_WHITE),

OBJECT(OBJ("沉重的铁球", None),  //"heavy iron ball"
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       BALL_CLASS, 1000, 0, 480, 10, 25, 25, 0, 0, 200, HI_METAL),
        /* +d4 when "very heavy" */
OBJECT(OBJ("铁链", None),  //"iron chain"
       BITS(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, WHACK, P_NONE, IRON), 0,
       CHAIN_CLASS, 1000, 0, 120, 0, 4, 4, 0, 0, 200, HI_METAL),
        /* +1 both l & s */

/* Venom is normally a transitory missile (spit by various creatures)
 * but can be wished for in wizard mode so could occur in bones data.
 */
OBJECT(OBJ("致盲毒液", "飞溅的毒液"),  //"blinding venom", "splash of venom"
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 0, 0, 0, 0, 0, HI_ORGANIC),
OBJECT(OBJ("酸性毒液", "飞溅的毒液"),  //"acid venom", "splash of venom"
       BITS(0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, P_NONE, LIQUID), 0,
       VENOM_CLASS, 500, 0, 1, 0, 6, 6, 0, 0, 0, HI_ORGANIC),
        /* +d6 small or large */

/* fencepost, the deadly Array Terminator -- name [1st arg] *must* be NULL */
OBJECT(OBJ(None, None),
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
