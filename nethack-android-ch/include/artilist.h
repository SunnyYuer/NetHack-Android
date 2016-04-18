/* NetHack 3.6  artilist.h      $NHDT-Date: 1433050874 2015/05/31 05:41:14 $  $NHDT-Branch: master $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifdef MAKEDEFS_C
/* in makedefs.c, all we care about is the list of names */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, cost, clr) nam

static const char *artifact_names[] = {
#else
/* in artifact.c, set up the actual artifact list structure */

#define A(nam, typ, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, cost, clr) \
    {                                                                       \
        typ, nam, s1, s2, mt, atk, dfn, cry, inv, al, cl, rac, cost, clr    \
    }

/* clang-format off */
#define     NO_ATTK     {0,0,0,0}               /* no attack */
#define     NO_DFNS     {0,0,0,0}               /* no defense */
#define     NO_CARY     {0,0,0,0}               /* no carry effects */
#define     DFNS(c)     {0,c,0,0}
#define     CARY(c)     {0,c,0,0}
#define     PHYS(a,b)   {0,AD_PHYS,a,b}         /* physical */
#define     DRLI(a,b)   {0,AD_DRLI,a,b}         /* life drain */
#define     COLD(a,b)   {0,AD_COLD,a,b}
#define     FIRE(a,b)   {0,AD_FIRE,a,b}
#define     ELEC(a,b)   {0,AD_ELEC,a,b}         /* electrical shock */
#define     STUN(a,b)   {0,AD_STUN,a,b}         /* magical attack */
/* clang-format on */

STATIC_OVL NEARDATA struct artifact artilist[] = {
#endif /* MAKEDEFS_C */

    /* Artifact cost rationale:
     * 1.  The more useful the artifact, the better its cost.
     * 2.  Quest artifacts are highly valued.
     * 3.  Chaotic artifacts are inflated due to scarcity (and balance).
     */

    /*  dummy element #0, so that all interesting indices are non-zero */
    A("", STRANGE_OBJECT, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE,
      NON_PM, NON_PM, 0L, NO_COLOR),

    A("誓约胜利之剑", LONG_SWORD, (SPFX_NOGEN | SPFX_RESTR | SPFX_SEEK  //Excalibur
                                | SPFX_DEFN | SPFX_INTEL | SPFX_SEARCH),
      0, 0, PHYS(5, 10), DRLI(0, 0), NO_CARY, 0, A_LAWFUL, PM_KNIGHT, NON_PM,
      4000L, NO_COLOR),
    /*
     *      Stormbringer only has a 2 because it can drain a level,
     *      providing 8 more.
     */
    A("兴风者", RUNESWORD,  //Stormbringer
      (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN | SPFX_INTEL | SPFX_DRLI), 0, 0,
      DRLI(5, 2), DRLI(0, 0), NO_CARY, 0, A_CHAOTIC, NON_PM, NON_PM, 8000L,
      NO_COLOR),
    /*
     *      Mjollnir will return to the hand of the wielder when thrown
     *      if the wielder is a Valkyrie wearing Gauntlets of Power.
     */
    A("雷神之锤", WAR_HAMMER, /* Mjo:llnir */  //Mjollnir
      (SPFX_RESTR | SPFX_ATTK), 0, 0, ELEC(5, 24), NO_DFNS, NO_CARY, 0,
      A_NEUTRAL, PM_VALKYRIE, NON_PM, 4000L, NO_COLOR),

    A("撕裂者", BATTLE_AXE, SPFX_RESTR, 0, 0, PHYS(3, 6), NO_DFNS, NO_CARY,  //Cleaver
      0, A_NEUTRAL, PM_BARBARIAN, NON_PM, 1500L, NO_COLOR),

    /*
     *      Grimtooth glows in warning when elves are present, but its
     *      damage bonus applies to all targets rather than just elves
     *      (handled as special case in spec_dbon()).
     */
    A("邪兽之牙", ORCISH_DAGGER, (SPFX_RESTR | SPFX_WARN | SPFX_DFLAG2),  //Grimtooth
      0, M2_ELF, PHYS(2, 6), NO_DFNS,
      NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ORC, 300L, CLR_RED),
    /*
     *      Orcrist and Sting have same alignment as elves.
     *
     *      The combination of SPFX_WARN+SPFX_DFLAG2+M2_value will trigger
     *      EWarn_of_mon for all monsters that have the M2_value flag.
     *      Sting and Orcrist will warn of M2_ORC monsters.
     */
    A("杀兽剑", ELVEN_BROADSWORD, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC,  //Orcrist
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF, 2000L,
      CLR_BRIGHT_BLUE), /* bright blue is actually light blue */

    A("杀兽针", ELVEN_DAGGER, (SPFX_WARN | SPFX_DFLAG2), 0, M2_ORC, PHYS(5, 0),  //Sting
      NO_DFNS, NO_CARY, 0, A_CHAOTIC, NON_PM, PM_ELF, 800L, CLR_BRIGHT_BLUE),
    /*
     *      Magicbane is a bit different!  Its magic fanfare
     *      unbalances victims in addition to doing some damage.
     */
    A("神魔克星", ATHAME, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,  //Magicbane
      STUN(3, 4), DFNS(AD_MAGM), NO_CARY, 0, A_NEUTRAL, PM_WIZARD, NON_PM,
      3500L, NO_COLOR),

    A("冰霜之剑", LONG_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,  //Frost Brand
      COLD(5, 0), COLD(0, 0), NO_CARY, 0, A_NONE, NON_PM, NON_PM, 3000L,
      NO_COLOR),

    A("烈焰之剑", LONG_SWORD, (SPFX_RESTR | SPFX_ATTK | SPFX_DEFN), 0, 0,  //Fire Brand
      FIRE(5, 0), FIRE(0, 0), NO_CARY, 0, A_NONE, NON_PM, NON_PM, 3000L,
      NO_COLOR),

    A("屠龙剑", BROADSWORD, (SPFX_RESTR | SPFX_DCLAS), 0, S_DRAGON,  //Dragonbane
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 500L,
      NO_COLOR),

    A("斩魔之剑", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_DEMON,  //Demonbane
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_LAWFUL, NON_PM, NON_PM, 2500L,
      NO_COLOR),

    A("兽化人之灾", SILVER_SABER, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_WERE,  //Werebane
      PHYS(5, 0), DFNS(AD_WERE), NO_CARY, 0, A_NONE, NON_PM, NON_PM, 1500L,
      NO_COLOR),

    A("夜刃", SILVER_SABER, (SPFX_RESTR | SPFX_HALRES), 0, 0,  //Grayswandir
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_LAWFUL, NON_PM, NON_PM, 8000L,
      NO_COLOR),

    A("巨人杀手", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_GIANT,  //Giantslayer
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM, 200L,
      NO_COLOR),

    A("食人魔粉碎者", WAR_HAMMER, (SPFX_RESTR | SPFX_DCLAS), 0, S_OGRE,  //Ogresmasher
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 200L,
      NO_COLOR),

    A("巨魔杀手", MORNING_STAR, (SPFX_RESTR | SPFX_DCLAS), 0, S_TROLL,  //Trollsbane
      PHYS(5, 0), NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 200L,
      NO_COLOR),
    /*
     *      Two problems:  1) doesn't let trolls regenerate heads,
     *      2) doesn't give unusual message for 2-headed monsters (but
     *      allowing those at all causes more problems than worth the effort).
     */
    A("斩首剑", LONG_SWORD, (SPFX_RESTR | SPFX_BEHEAD), 0, 0,  //Vorpal Blade
      PHYS(5, 1), NO_DFNS, NO_CARY, 0, A_NEUTRAL, NON_PM, NON_PM, 4000L,
      NO_COLOR),
    /*
     *      Ah, never shall I forget the cry,
     *              or the shriek that shrieked he,
     *      As I gnashed my teeth, and from my sheath
     *              I drew my Snickersnee!
     *                      --Koko, Lord high executioner of Titipu
     *                        (From Sir W.S. Gilbert's "The Mikado")
     */
    A("草薙剑", KATANA, SPFX_RESTR, 0, 0, PHYS(0, 8), NO_DFNS, NO_CARY,  //Snickersnee
      0, A_LAWFUL, PM_SAMURAI, NON_PM, 1200L, NO_COLOR),

    A("旭日剑", LONG_SWORD, (SPFX_RESTR | SPFX_DFLAG2), 0, M2_UNDEAD,  //Sunsword
      PHYS(5, 0), DFNS(AD_BLND), NO_CARY, 0, A_LAWFUL, NON_PM, NON_PM, 1500L,
      NO_COLOR),

    /*
     *      The artifacts for the quest dungeon, all self-willed.
     */

    A("探测之球", CRYSTAL_BALL,  //The Orb of Detection
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), (SPFX_ESP | SPFX_HSPDAM), 0,
      NO_ATTK, NO_DFNS, CARY(AD_MAGM), INVIS, A_LAWFUL, PM_ARCHEOLOGIST,
      NON_PM, 2500L, NO_COLOR),

    A("阿里曼之心", LUCKSTONE,  //The Heart of Ahriman
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), SPFX_STLTH, 0,
      /* this stone does double damage if used as a projectile weapon */
      PHYS(5, 0), NO_DFNS, NO_CARY, LEVITATION, A_NEUTRAL, PM_BARBARIAN,
      NON_PM, 2500L, NO_COLOR),

    A("力量权杖", MACE,  //The Sceptre of Might
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DALIGN), 0, 0, PHYS(5, 0),
      DFNS(AD_MAGM), NO_CARY, CONFLICT, A_LAWFUL, PM_CAVEMAN, NON_PM, 2500L,
      NO_COLOR),

#if 0 /* OBSOLETE */
A("韦斯特内西的真知晶球",        CRYSTAL_BALL,  //The Palantir of Westernesse
        (SPFX_NOGEN|SPFX_RESTR|SPFX_INTEL),
                (SPFX_ESP|SPFX_REGEN|SPFX_HSPDAM), 0,
        NO_ATTK,        NO_DFNS,        NO_CARY,
        TAMING,         A_CHAOTIC, NON_PM , PM_ELF, 8000L, NO_COLOR ),
#endif

    A("阿斯克勒庇俄斯之杖", QUARTERSTAFF,  //The Staff of Aesculapius
      (SPFX_NOGEN | SPFX_RESTR | SPFX_ATTK | SPFX_INTEL | SPFX_DRLI
       | SPFX_REGEN),
      0, 0, DRLI(0, 0), DRLI(0, 0), NO_CARY, HEALING, A_NEUTRAL, PM_HEALER,
      NON_PM, 5000L, NO_COLOR),

    A("梅林的魔镜", MIRROR,  //The Magic Mirror of Merlin
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK), SPFX_ESP, 0,
      NO_ATTK, NO_DFNS, CARY(AD_MAGM), 0, A_LAWFUL, PM_KNIGHT, NON_PM, 1500L,
      NO_COLOR),

    A("超世界之眼", LENSES,  //The Eyes of the Overworld
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_XRAY), 0, 0, NO_ATTK,
      DFNS(AD_MAGM), NO_CARY, ENLIGHTENING, A_NEUTRAL, PM_MONK, NON_PM,
      2500L, NO_COLOR),

    A("圣洁的冠冕", HELM_OF_BRILLIANCE,  //The Mitre of Holiness
      (SPFX_NOGEN | SPFX_RESTR | SPFX_DFLAG2 | SPFX_INTEL | SPFX_PROTECT), 0,
      M2_UNDEAD, NO_ATTK, NO_DFNS, CARY(AD_FIRE), ENERGY_BOOST, A_LAWFUL,
      PM_PRIEST, NON_PM, 2000L, NO_COLOR),

    A("戴安娜的长弓", BOW,  //The Longbow of Diana
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_REFLECT), SPFX_ESP, 0,
      PHYS(5, 0), NO_DFNS, NO_CARY, CREATE_AMMO, A_CHAOTIC, PM_RANGER, NON_PM,
      4000L, NO_COLOR),

    A("盗窃专用万能钥匙", SKELETON_KEY,  //The Master Key of Thievery
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_SPEAK),
      (SPFX_WARN | SPFX_TCTRL | SPFX_HPHDAM), 0, NO_ATTK, NO_DFNS, NO_CARY,
      UNTRAP, A_CHAOTIC, PM_ROGUE, NON_PM, 3500L, NO_COLOR),

    A("妖刀村正", TSURUGI,  //The Tsurugi of Muramasa
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_BEHEAD | SPFX_LUCK
       | SPFX_PROTECT),
      0, 0, PHYS(0, 8), NO_DFNS, NO_CARY, 0, A_LAWFUL, PM_SAMURAI, NON_PM,
      4500L, NO_COLOR),

    A("岩德至尊白金卡", CREDIT_CARD,  //The Platinum Yendorian Express Card
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_DEFN),
      (SPFX_ESP | SPFX_HSPDAM), 0, NO_ATTK, NO_DFNS, CARY(AD_MAGM),
      CHARGE_OBJ, A_NEUTRAL, PM_TOURIST, NON_PM, 7000L, NO_COLOR),

    A("命运之球", CRYSTAL_BALL,  //The Orb of Fate
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL | SPFX_LUCK),
      (SPFX_WARN | SPFX_HSPDAM | SPFX_HPHDAM), 0, NO_ATTK, NO_DFNS, NO_CARY,
      LEV_TELE, A_NEUTRAL, PM_VALKYRIE, NON_PM, 3500L, NO_COLOR),

    A("艾奇奥匹亚之眼", AMULET_OF_ESP,  //The Eye of the Aethiopica
      (SPFX_NOGEN | SPFX_RESTR | SPFX_INTEL), (SPFX_EREGEN | SPFX_HSPDAM), 0,
      NO_ATTK, DFNS(AD_MAGM), NO_CARY, CREATE_PORTAL, A_NEUTRAL, PM_WIZARD,
      NON_PM, 4000L, NO_COLOR),

    /*
     *  terminator; otyp must be zero
     */
    A(0, 0, 0, 0, 0, NO_ATTK, NO_DFNS, NO_CARY, 0, A_NONE, NON_PM, NON_PM, 0L,
      0) /* 0 is CLR_BLACK rather than NO_COLOR but it doesn't matter here */

}; /* artilist[] (or artifact_names[]) */

#undef A

#ifndef MAKEDEFS_C
#undef NO_ATTK
#undef NO_DFNS
#undef DFNS
#undef PHYS
#undef DRLI
#undef COLD
#undef FIRE
#undef ELEC
#undef STUN
#endif

/*artilist.h*/
