/* NetHack 3.6	pray.c	$NHDT-Date: 1446854232 2015/11/06 23:57:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.87 $ */
/* Copyright (c) Benson I. Margulies, Mike Stephenson, Steve Linhart, 1989. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(prayer_done);
STATIC_DCL struct obj *NDECL(worst_cursed_item);
STATIC_DCL int NDECL(in_trouble);
STATIC_DCL void FDECL(fix_worst_trouble, (int));
STATIC_DCL void FDECL(angrygods, (ALIGNTYP_P));
STATIC_DCL void FDECL(at_your_feet, (const char *));
STATIC_DCL void NDECL(gcrownu);
STATIC_DCL void FDECL(pleased, (ALIGNTYP_P));
STATIC_DCL void FDECL(godvoice, (ALIGNTYP_P, const char *));
STATIC_DCL void FDECL(god_zaps_you, (ALIGNTYP_P));
STATIC_DCL void FDECL(fry_by_god, (ALIGNTYP_P, BOOLEAN_P));
STATIC_DCL void FDECL(gods_angry, (ALIGNTYP_P));
STATIC_DCL void FDECL(gods_upset, (ALIGNTYP_P));
STATIC_DCL void FDECL(consume_offering, (struct obj *));
STATIC_DCL boolean FDECL(water_prayer, (BOOLEAN_P));
STATIC_DCL boolean FDECL(blocked_boulder, (int, int));

/* simplify a few tests */
#define Cursed_obj(obj, typ) ((obj) && (obj)->otyp == (typ) && (obj)->cursed)

/*
 * Logic behind deities and altars and such:
 * + prayers are made to your god if not on an altar, and to the altar's god
 *   if you are on an altar
 * + If possible, your god answers all prayers, which is why bad things happen
 *   if you try to pray on another god's altar
 * + sacrifices work basically the same way, but the other god may decide to
 *   accept your allegiance, after which they are your god.  If rejected,
 *   your god takes over with your punishment.
 * + if you're in Gehennom, all messages come from Moloch
 */

/*
 *      Moloch, who dwells in Gehennom, is the "renegade" cruel god
 *      responsible for the theft of the Amulet from Marduk, the Creator.
 *      Moloch is unaligned.
 */
static const char *Moloch = "摩洛";

static const char *godvoices[] = {
    "低沉", "怒喝", "响起", "洪亮",
};

/* values calculated when prayer starts, and used when completed */
static aligntyp p_aligntyp;
static int p_trouble;
static int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

#define PIOUS 20
#define DEVOUT 14
#define FERVENT 9
#define STRIDENT 4

/*
 * The actual trouble priority is determined by the order of the
 * checks performed in in_trouble() rather than by these numeric
 * values, so keep that code and these values synchronized in
 * order to have the values be meaningful.
 */

#define TROUBLE_STONED 14
#define TROUBLE_SLIMED 13
#define TROUBLE_STRANGLED 12
#define TROUBLE_LAVA 11
#define TROUBLE_SICK 10
#define TROUBLE_STARVING 9
#define TROUBLE_REGION 8 /* stinking cloud */
#define TROUBLE_HIT 7
#define TROUBLE_LYCANTHROPE 6
#define TROUBLE_COLLAPSING 5
#define TROUBLE_STUCK_IN_WALL 4
#define TROUBLE_CURSED_LEVITATION 3
#define TROUBLE_UNUSEABLE_HANDS 2
#define TROUBLE_CURSED_BLINDFOLD 1

#define TROUBLE_PUNISHED (-1)
#define TROUBLE_FUMBLING (-2)
#define TROUBLE_CURSED_ITEMS (-3)
#define TROUBLE_SADDLE (-4)
#define TROUBLE_BLIND (-5)
#define TROUBLE_POISONED (-6)
#define TROUBLE_WOUNDED_LEGS (-7)
#define TROUBLE_HUNGRY (-8)
#define TROUBLE_STUNNED (-9)
#define TROUBLE_CONFUSED (-10)
#define TROUBLE_HALLUCINATION (-11)


#define ugod_is_angry() (u.ualign.record < 0)
#define on_altar() IS_ALTAR(levl[u.ux][u.uy].typ)
#define on_shrine() ((levl[u.ux][u.uy].altarmask & AM_SHRINE) != 0)
#define a_align(x, y) ((aligntyp) Amask2align(levl[x][y].altarmask & AM_MASK))

/* critically low hit points if hp <= 5 or hp <= maxhp/N for some N */
boolean
critically_low_hp(only_if_injured)
boolean only_if_injured; /* determines whether maxhp <= 5 matters */
{
    int divisor, hplim, curhp = Upolyd ? u.mh : u.uhp,
                        maxhp = Upolyd ? u.mhmax : u.uhpmax;

    if (only_if_injured && !(curhp < maxhp))
        return FALSE;
    /* if maxhp is extremely high, use lower threshold for the division test
       (golden glow cuts off at 11+5*lvl, nurse interaction at 25*lvl; this
       ought to use monster hit dice--and a smaller multiplier--rather than
       ulevel when polymorphed, but polyself doesn't maintain that) */
    hplim = 15 * u.ulevel;
    if (maxhp > hplim)
        maxhp = hplim;
    /* 7 used to be the unconditional divisor */
    switch (xlev_to_rank(u.ulevel)) { /* maps 1..30 into 0..8 */
    case 0:
    case 1:
        divisor = 5;
        break; /* explvl 1 to 5 */
    case 2:
    case 3:
        divisor = 6;
        break; /* explvl 6 to 13 */
    case 4:
    case 5:
        divisor = 7;
        break; /* explvl 14 to 21 */
    case 6:
    case 7:
        divisor = 8;
        break; /* explvl 22 to 29 */
    default:
        divisor = 9;
        break; /* explvl 30+ */
    }
    /* 5 is a magic number in TROUBLE_HIT handling below */
    return (boolean) (curhp <= 5 || curhp * divisor <= maxhp);
}

/*
 * Return 0 if nothing particular seems wrong, positive numbers for
 * serious trouble, and negative numbers for comparative annoyances.
 * This returns the worst problem. There may be others, and the gods
 * may fix more than one.
 *
 * This could get as bizarre as noting surrounding opponents, (or
 * hostile dogs), but that's really hard.
 *
 * We could force rehumanize of polyselfed people, but we can't tell
 * unintentional shape changes from the other kind. Oh well.
 * 3.4.2: make an exception if polymorphed into a form which lacks
 * hands; that's a case where the ramifications override this doubt.
 */
STATIC_OVL int
in_trouble()
{
    struct obj *otmp;
    int i, j, count = 0;

    /*
     * major troubles
     */
    if (Stoned)
        return TROUBLE_STONED;
    if (Slimed)
        return TROUBLE_SLIMED;
    if (Strangled)
        return TROUBLE_STRANGLED;
    if (u.utrap && u.utraptype == TT_LAVA)
        return TROUBLE_LAVA;
    if (Sick)
        return TROUBLE_SICK;
    if (u.uhs >= WEAK)
        return TROUBLE_STARVING;
    if (region_danger())
        return TROUBLE_REGION;
    if (critically_low_hp(FALSE))
        return TROUBLE_HIT;
    if (u.ulycn >= LOW_PM)
        return TROUBLE_LYCANTHROPE;
    if (near_capacity() >= EXT_ENCUMBER && AMAX(A_STR) - ABASE(A_STR) > 3)
        return TROUBLE_COLLAPSING;

    for (i = -1; i <= 1; i++)
        for (j = -1; j <= 1; j++) {
            if (!i && !j)
                continue;
            if (!isok(u.ux + i, u.uy + j)
                || IS_ROCK(levl[u.ux + i][u.uy + j].typ)
                || (blocked_boulder(i, j) && !throws_rocks(youmonst.data)))
                count++;
        }
    if (count == 8 && !Passes_walls)
        return TROUBLE_STUCK_IN_WALL;

    if (Cursed_obj(uarmf, LEVITATION_BOOTS)
        || stuck_ring(uleft, RIN_LEVITATION)
        || stuck_ring(uright, RIN_LEVITATION))
        return TROUBLE_CURSED_LEVITATION;
    if (nohands(youmonst.data) || !freehand()) {
        /* for bag/box access [cf use_container()]...
           make sure it's a case that we know how to handle;
           otherwise "fix all troubles" would get stuck in a loop */
        if (welded(uwep))
            return TROUBLE_UNUSEABLE_HANDS;
        if (Upolyd && nohands(youmonst.data)
            && (!Unchanging || ((otmp = unchanger()) != 0 && otmp->cursed)))
            return TROUBLE_UNUSEABLE_HANDS;
    }
    if (Blindfolded && ublindf->cursed)
        return TROUBLE_CURSED_BLINDFOLD;

    /*
     * minor troubles
     */
    if (Punished || (u.utrap && u.utraptype == TT_BURIEDBALL))
        return TROUBLE_PUNISHED;
    if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING)
        || Cursed_obj(uarmf, FUMBLE_BOOTS))
        return TROUBLE_FUMBLING;
    if (worst_cursed_item())
        return TROUBLE_CURSED_ITEMS;
    if (u.usteed) { /* can't voluntarily dismount from a cursed saddle */
        otmp = which_armor(u.usteed, W_SADDLE);
        if (Cursed_obj(otmp, SADDLE))
            return TROUBLE_SADDLE;
    }

    if (Blinded > 1 && haseyes(youmonst.data)
        && (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND)))
        return TROUBLE_BLIND;
    for (i = 0; i < A_MAX; i++)
        if (ABASE(i) < AMAX(i))
            return TROUBLE_POISONED;
    if (Wounded_legs && !u.usteed)
        return TROUBLE_WOUNDED_LEGS;
    if (u.uhs >= HUNGRY)
        return TROUBLE_HUNGRY;
    if (HStun & TIMEOUT)
        return TROUBLE_STUNNED;
    if (HConfusion & TIMEOUT)
        return TROUBLE_CONFUSED;
    if (HHallucination & TIMEOUT)
        return TROUBLE_HALLUCINATION;
    return 0;
}

/* select an item for TROUBLE_CURSED_ITEMS */
STATIC_OVL struct obj *
worst_cursed_item()
{
    register struct obj *otmp;

    /* if strained or worse, check for loadstone first */
    if (near_capacity() >= HVY_ENCUMBER) {
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (Cursed_obj(otmp, LOADSTONE))
                return otmp;
    }
    /* weapon takes precedence if it is interfering
       with taking off a ring or putting on a shield */
    if (welded(uwep) && (uright || bimanual(uwep))) { /* weapon */
        otmp = uwep;
        /* gloves come next, due to rings */
    } else if (uarmg && uarmg->cursed) { /* gloves */
        otmp = uarmg;
        /* then shield due to two handed weapons and spells */
    } else if (uarms && uarms->cursed) { /* shield */
        otmp = uarms;
        /* then cloak due to body armor */
    } else if (uarmc && uarmc->cursed) { /* cloak */
        otmp = uarmc;
    } else if (uarm && uarm->cursed) { /* suit */
        otmp = uarm;
    } else if (uarmh && uarmh->cursed) { /* helmet */
        otmp = uarmh;
    } else if (uarmf && uarmf->cursed) { /* boots */
        otmp = uarmf;
    } else if (uarmu && uarmu->cursed) { /* shirt */
        otmp = uarmu;
    } else if (uamul && uamul->cursed) { /* amulet */
        otmp = uamul;
    } else if (uleft && uleft->cursed) { /* left ring */
        otmp = uleft;
    } else if (uright && uright->cursed) { /* right ring */
        otmp = uright;
    } else if (ublindf && ublindf->cursed) { /* eyewear */
        otmp = ublindf;                      /* must be non-blinding lenses */
        /* if weapon wasn't handled above, do it now */
    } else if (welded(uwep)) { /* weapon */
        otmp = uwep;
        /* active secondary weapon even though it isn't welded */
    } else if (uswapwep && uswapwep->cursed && u.twoweap) {
        otmp = uswapwep;
        /* all worn items ought to be handled by now */
    } else {
        for (otmp = invent; otmp; otmp = otmp->nobj) {
            if (!otmp->cursed)
                continue;
            if (otmp->otyp == LOADSTONE || confers_luck(otmp))
                break;
        }
    }
    return otmp;
}

STATIC_OVL void
fix_worst_trouble(trouble)
int trouble;
{
    int i;
    struct obj *otmp = 0;
    const char *what = (const char *) 0;
    static NEARDATA const char leftglow[] = "你左边的戒指发出柔和的光芒",
                               rightglow[] = "你右边的戒指发出柔和的光芒";

    switch (trouble) {
    case TROUBLE_STONED:
        make_stoned(0L, "你感觉身体更柔软了.", 0, (char *) 0);
        break;
    case TROUBLE_SLIMED:
        make_slimed(0L, "黏液消失了.");
        break;
    case TROUBLE_STRANGLED:
        if (uamul && uamul->otyp == AMULET_OF_STRANGULATION) {
            Your("护身符消失了!");
            useup(uamul);
        }
        You("又能呼吸了.");
        Strangled = 0;
        context.botl = 1;
        break;
    case TROUBLE_LAVA:
        You("回到了结实的地上.");
        /* teleport should always succeed, but if not,
         * just untrap them.
         */
        if (!safe_teleds(FALSE))
            u.utrap = 0;
        break;
    case TROUBLE_STARVING:
        losestr(-1);
        /*FALLTHRU*/
    case TROUBLE_HUNGRY:
        Your("%s 感觉饱了.", body_part(STOMACH));
        init_uhunger();
        context.botl = 1;
        break;
    case TROUBLE_SICK:
        You_feel("好些了.");
        make_sick(0L, (char *) 0, FALSE, SICK_ALL);
        break;
    case TROUBLE_REGION:
        /* stinking cloud, with hero vulnerable to HP loss */
        region_safety();
        break;
    case TROUBLE_HIT:
        /* "fix all troubles" will keep trying if hero has
           5 or less hit points, so make sure they're always
           boosted to be more than that */
        You_feel("好多了.");
        if (Upolyd) {
            u.mhmax += rnd(5);
            if (u.mhmax <= 5)
                u.mhmax = 5 + 1;
            u.mh = u.mhmax;
        }
        if (u.uhpmax < u.ulevel * 5 + 11)
            u.uhpmax += rnd(5);
        if (u.uhpmax <= 5)
            u.uhpmax = 5 + 1;
        u.uhp = u.uhpmax;
        context.botl = 1;
        break;
    case TROUBLE_COLLAPSING:
        /* override Fixed_abil; uncurse that if feasible */
        You_feel("强壮%s了.",
                 (AMAX(A_STR) - ABASE(A_STR) > 6) ? "多 " : "些");
        ABASE(A_STR) = AMAX(A_STR);
        context.botl = 1;
        if (Fixed_abil) {
            if ((otmp = stuck_ring(uleft, RIN_SUSTAIN_ABILITY)) != 0) {
                if (otmp == uleft)
                    what = leftglow;
            } else if ((otmp = stuck_ring(uright, RIN_SUSTAIN_ABILITY))
                       != 0) {
                if (otmp == uright)
                    what = rightglow;
            }
            if (otmp)
                goto decurse;
        }
        break;
    case TROUBLE_STUCK_IN_WALL:
        Your("周围改变了.");
        /* no control, but works on no-teleport levels */
        (void) safe_teleds(FALSE);
        break;
    case TROUBLE_CURSED_LEVITATION:
        if (Cursed_obj(uarmf, LEVITATION_BOOTS)) {
            otmp = uarmf;
        } else if ((otmp = stuck_ring(uleft, RIN_LEVITATION)) != 0) {
            if (otmp == uleft)
                what = leftglow;
        } else if ((otmp = stuck_ring(uright, RIN_LEVITATION)) != 0) {
            if (otmp == uright)
                what = rightglow;
        }
        goto decurse;
    case TROUBLE_UNUSEABLE_HANDS:
        if (welded(uwep)) {
            otmp = uwep;
            goto decurse;
        }
        if (Upolyd && nohands(youmonst.data)) {
            if (!Unchanging) {
                Your("样子变得不确定了.");
                rehumanize(); /* "You return to {normal} form." */
            } else if ((otmp = unchanger()) != 0 && otmp->cursed) {
                /* otmp is an amulet of unchanging */
                goto decurse;
            }
        }
        if (nohands(youmonst.data) || !freehand())
            impossible("fix_worst_trouble: couldn't cure hands.");
        break;
    case TROUBLE_CURSED_BLINDFOLD:
        otmp = ublindf;
        goto decurse;
    case TROUBLE_LYCANTHROPE:
        you_unwere(TRUE);
        break;
    /*
     */
    case TROUBLE_PUNISHED:
        Your("铁链消失了.");
        if (u.utrap && u.utraptype == TT_BURIEDBALL)
            buried_ball_to_freedom();
        else
            unpunish();
        break;
    case TROUBLE_FUMBLING:
        if (Cursed_obj(uarmg, GAUNTLETS_OF_FUMBLING))
            otmp = uarmg;
        else if (Cursed_obj(uarmf, FUMBLE_BOOTS))
            otmp = uarmf;
        goto decurse;
        /*NOTREACHED*/
        break;
    case TROUBLE_CURSED_ITEMS:
        otmp = worst_cursed_item();
        if (otmp == uright)
            what = rightglow;
        else if (otmp == uleft)
            what = leftglow;
    decurse:
        if (!otmp) {
            impossible("fix_worst_trouble: nothing to uncurse.");
            return;
        }
        if (!Blind || (otmp == ublindf && Blindfolded_only)) {
            pline("%s %s光芒.",
                  what ? what : (const char *) Yobjnam2(otmp, "发出柔和的"),
                  hcolor(NH_AMBER));
            iflags.last_msg = PLNMSG_OBJ_GLOWS;
            otmp->bknown = TRUE;
        }
        uncurse(otmp);
        update_inventory();
        break;
    case TROUBLE_POISONED:
        /* override Fixed_abil; ignore items which confer that */
        if (Hallucination)
            pline("在你的坦克里有一只老虎.");
        else
            You_feel("身体又健康了.");
        for (i = 0; i < A_MAX; i++) {
            if (ABASE(i) < AMAX(i)) {
                ABASE(i) = AMAX(i);
                context.botl = 1;
            }
        }
        (void) encumber_msg();
        break;
    case TROUBLE_BLIND: {
        const char *eyes = body_part(EYE);

        if (eyecount(youmonst.data) != 1)
            eyes = makeplural(eyes);
        Your("%s %s 好些了.", eyes, vtense(eyes, "感觉"));
        u.ucreamed = 0;
        make_blinded(0L, FALSE);
        break;
    }
    case TROUBLE_WOUNDED_LEGS:
        heal_legs();
        break;
    case TROUBLE_STUNNED:
        make_stunned(0L, TRUE);
        break;
    case TROUBLE_CONFUSED:
        make_confused(0L, TRUE);
        break;
    case TROUBLE_HALLUCINATION:
        pline("看起来你又回到了堪萨斯州.");
        (void) make_hallucinated(0L, FALSE, 0L);
        break;
    case TROUBLE_SADDLE:
        otmp = which_armor(u.usteed, W_SADDLE);
        if (!Blind) {
            pline("%s %s光芒.", Yobjnam2(otmp, "发出柔和的"), hcolor(NH_AMBER));
            otmp->bknown = TRUE;
        }
        uncurse(otmp);
        break;
    }
}

/* "I am sometimes shocked by...  the nuns who never take a bath without
 * wearing a bathrobe all the time.  When asked why, since no man can see
 * them,
 * they reply 'Oh, but you forget the good God'.  Apparently they conceive of
 * the Deity as a Peeping Tom, whose omnipotence enables Him to see through
 * bathroom walls, but who is foiled by bathrobes." --Bertrand Russell, 1943
 * Divine wrath, dungeon walls, and armor follow the same principle.
 */
STATIC_OVL void
god_zaps_you(resp_god)
aligntyp resp_god;
{
    if (u.uswallow) {
        pline(
          "突然一道闪电从天上落到你身上!");
        pline("它击中了 %s!", mon_nam(u.ustuck));
        if (!resists_elec(u.ustuck)) {
            pline("%s 被电成灰烬!", Monnam(u.ustuck));
            /* Yup, you get experience.  It takes guts to successfully
             * pull off this trick on your god, anyway.
             */
            xkilled(u.ustuck, 0);
        } else
            pline("%s 看起来没有受影响.", Monnam(u.ustuck));
    } else {
        pline("突然, 一道闪电击中你!");
        if (Reflecting) {
            shieldeff(u.ux, u.uy);
            if (Blind)
                pline("出于某些原因你不受影响.");
            else
                (void) ureflects("但%s 被你的%s反射开了.", "它");
        } else if (Shock_resistance) {
            shieldeff(u.ux, u.uy);
            pline("但是看起来没有影响你.");
        } else
            fry_by_god(resp_god, FALSE);
    }

    pline("%s 还没停手...", align_gname(resp_god));
    if (u.uswallow) {
        pline("一道大角度的分解光束瞄准你攻击了%s!",
              mon_nam(u.ustuck));
        if (!resists_disint(u.ustuck)) {
            pline("%s 被分解为一堆灰尘!", Monnam(u.ustuck));
            xkilled(u.ustuck, 2); /* no corpse */
        } else
            pline("%s 看起来没有受影响.", Monnam(u.ustuck));
    } else {
        pline("一道大角度的分解光束击中了你!");

        /* disintegrate shield and body armor before disintegrating
         * the impudent mortal, like black dragon breath -3.
         */
        if (uarms && !(EReflecting & W_ARMS)
            && !(EDisint_resistance & W_ARMS))
            (void) destroy_arm(uarms);
        if (uarmc && !(EReflecting & W_ARMC)
            && !(EDisint_resistance & W_ARMC))
            (void) destroy_arm(uarmc);
        if (uarm && !(EReflecting & W_ARM) && !(EDisint_resistance & W_ARM)
            && !uarmc)
            (void) destroy_arm(uarm);
        if (uarmu && !uarm && !uarmc)
            (void) destroy_arm(uarmu);
        if (!Disint_resistance)
            fry_by_god(resp_god, TRUE);
        else {
            You("在其%s 光芒中沐浴了一分钟...", NH_BLACK);
            godvoice(resp_god, "我不敢相信!");
        }
        if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) {
            /* one more try for high altars */
            verbalize("汝无法从吾愤怒中逃脱, 凡人!");
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            summon_minion(resp_god, FALSE);
            verbalize("消灭%s, 我的仆人们!", uhim());
        }
    }
}

STATIC_OVL void
fry_by_god(resp_god, via_disintegration)
aligntyp resp_god;
boolean via_disintegration;
{
    You("%s!", !via_disintegration ? "被电成灰烬"
                                   : "被分解为一堆灰尘");
    killer.format = KILLED_BY;
    Sprintf(killer.name, "%s的愤怒", align_gname(resp_god));
    done(DIED);
}

STATIC_OVL void
angrygods(resp_god)
aligntyp resp_god;
{
    int maxanger;

    if (Inhell)
        resp_god = A_NONE;
    u.ublessed = 0;

    /* changed from tmp = u.ugangr + abs (u.uluck) -- rph */
    /* added test for alignment diff -dlc */
    if (resp_god != u.ualign.type)
        maxanger = u.ualign.record / 2 + (Luck > 0 ? -Luck / 3 : -Luck);
    else
        maxanger = 3 * u.ugangr + ((Luck > 0 || u.ualign.record >= STRIDENT)
                                   ? -Luck / 3
                                   : -Luck);
    if (maxanger < 1)
        maxanger = 1; /* possible if bad align & good luck */
    else if (maxanger > 15)
        maxanger = 15; /* be reasonable */

    switch (rn2(maxanger)) {
    case 0:
    case 1:
        You_feel("%s 是%s.", align_gname(resp_god),
                 Hallucination ? "沮丧的" : "不高兴的");
        break;
    case 2:
    case 3:
        godvoice(resp_god, (char *) 0);
        pline("\" 汝 %s, %s.\"",
              (ugod_is_angry() && resp_god == u.ualign.type)
                  ? "迷失了道路"
                  : "甚傲慢",
              youmonst.data->mlet == S_HUMAN ? "凡人" : "畜生");
        verbalize("汝须重新修习!");
        (void) adjattrib(A_WIS, -1, FALSE);
        losexp((char *) 0);
        break;
    case 6:
        if (!Punished) {
            gods_angry(resp_god);
            punish((struct obj *) 0);
            break;
        } /* else fall thru */
    case 4:
    case 5:
        gods_angry(resp_god);
        if (!Blind && !Antimagic)
            pline("%s 光芒围绕着你.", hcolor(NH_BLACK));
        rndcurse();
        break;
    case 7:
    case 8:
        godvoice(resp_god, (char *) 0);
        verbalize("汝敢%s 吾?",
                  (on_altar() && (a_align(u.ux, u.uy) != resp_god))
                      ? "轻蔑"
                      : "号令");
        pline("\" 那么死吧, %s!\"",
              youmonst.data->mlet == S_HUMAN ? "凡人" : "畜生");
        summon_minion(resp_god, FALSE);
        break;

    default:
        gods_angry(resp_god);
        god_zaps_you(resp_god);
        break;
    }
    u.ublesscnt = rnz(300);
    return;
}

/* helper to print "str appears at your feet", or appropriate */
static void
at_your_feet(str)
const char *str;
{
    if (Blind)
        str = Something;
    if (u.uswallow) {
        /* barrier between you and the floor */
        pline("%s %s 进了%s %s里.", str, vtense(str, "掉"),
              s_suffix(mon_nam(u.ustuck)), mbodypart(u.ustuck, STOMACH));
    } else {
        pline("%s %s你的%s%s!", str,
              Blind ? "到达" : vtense(str, "出现在"),
              makeplural(body_part(FOOT)), Levitation ? "下" : "上");
    }
}

STATIC_OVL void
gcrownu()
{
    struct obj *obj;
    boolean already_exists, in_hand;
    short class_gift;
    int sp_no;
#define ok_wep(o) ((o) && ((o)->oclass == WEAPON_CLASS || is_weptool(o)))

    HSee_invisible |= FROMOUTSIDE;
    HFire_resistance |= FROMOUTSIDE;
    HCold_resistance |= FROMOUTSIDE;
    HShock_resistance |= FROMOUTSIDE;
    HSleep_resistance |= FROMOUTSIDE;
    HPoison_resistance |= FROMOUTSIDE;
    godvoice(u.ualign.type, (char *) 0);

    obj = ok_wep(uwep) ? uwep : 0;
    already_exists = in_hand = FALSE; /* lint suppression */
    switch (u.ualign.type) {
    case A_LAWFUL:
        u.uevent.uhand_of_elbereth = 1;
        verbalize("我赐予你荣誉...  伊尔碧绿丝(Elbereth) 之手!");
        break;
    case A_NEUTRAL:
        u.uevent.uhand_of_elbereth = 2;
        in_hand = (uwep && uwep->oartifact == ART_VORPAL_BLADE);
        already_exists =
            exist_artifact(LONG_SWORD, artiname(ART_VORPAL_BLADE));
        verbalize("你将成为我的平衡使者!");
        break;
    case A_CHAOTIC:
        u.uevent.uhand_of_elbereth = 3;
        in_hand = (uwep && uwep->oartifact == ART_STORMBRINGER);
        already_exists =
            exist_artifact(RUNESWORD, artiname(ART_STORMBRINGER));
        verbalize("你被选中来要为我的荣耀而%s!",
                  already_exists && !in_hand ? "杀生" : "偷取灵魂");
        break;
    }

    class_gift = STRANGE_OBJECT;
    /* 3.3.[01] had this in the A_NEUTRAL case below,
       preventing chaotic wizards from receiving a spellbook */
    if (Role_if(PM_WIZARD)
        && (!uwep || (uwep->oartifact != ART_VORPAL_BLADE
                      && uwep->oartifact != ART_STORMBRINGER))
        && !carrying(SPE_FINGER_OF_DEATH)) {
        class_gift = SPE_FINGER_OF_DEATH;
    make_splbk:
        obj = mksobj(class_gift, TRUE, FALSE);
        bless(obj);
        obj->bknown = TRUE;
        at_your_feet("一本魔法书");
        dropy(obj);
        u.ugifts++;
        /* when getting a new book for known spell, enhance
           currently wielded weapon rather than the book */
        for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
            if (spl_book[sp_no].sp_id == class_gift) {
                if (ok_wep(uwep))
                    obj = uwep; /* to be blessed,&c */
                break;
            }
    } else if (Role_if(PM_MONK) && (!uwep || !uwep->oartifact)
               && !carrying(SPE_RESTORE_ABILITY)) {
        /* monks rarely wield a weapon */
        class_gift = SPE_RESTORE_ABILITY;
        goto make_splbk;
    }

    switch (u.ualign.type) {
    case A_LAWFUL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && obj->otyp == LONG_SWORD && !obj->oartifact) {
            if (!Blind)
                Your("剑明亮地发光了片刻.");
            obj = oname(obj, artiname(ART_EXCALIBUR));
            if (obj && obj->oartifact == ART_EXCALIBUR)
                u.ugifts++;
        }
        /* acquire Excalibur's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_EXCALIBUR)
            discover_artifact(ART_EXCALIBUR);
        break;
    case A_NEUTRAL:
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            Your("%s 出鞘虎视眈眈!", xname(obj));
            obj->dknown = TRUE;
        } else if (!already_exists) {
            obj = mksobj(LONG_SWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_VORPAL_BLADE));
            obj->spe = 1;
            at_your_feet("一把剑");
            dropy(obj);
            u.ugifts++;
        }
        /* acquire Vorpal Blade's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_LONG_SWORD);
        if (obj && obj->oartifact == ART_VORPAL_BLADE)
            discover_artifact(ART_VORPAL_BLADE);
        break;
    case A_CHAOTIC: {
        char swordbuf[BUFSZ];

        Sprintf(swordbuf, "%s 剑", hcolor(NH_BLACK));
        if (class_gift != STRANGE_OBJECT) {
            ; /* already got bonus above */
        } else if (obj && in_hand) {
            Your("%s 预示地发出低沉的声音!", swordbuf);
            obj->dknown = TRUE;
        } else if (!already_exists) {
            obj = mksobj(RUNESWORD, FALSE, FALSE);
            obj = oname(obj, artiname(ART_STORMBRINGER));
            obj->spe = 1;
            at_your_feet(swordbuf);
            dropy(obj);
            u.ugifts++;
        }
        /* acquire Stormbringer's skill regardless of weapon or gift */
        unrestrict_weapon_skill(P_BROAD_SWORD);
        if (obj && obj->oartifact == ART_STORMBRINGER)
            discover_artifact(ART_STORMBRINGER);
        break;
    }
    default:
        obj = 0; /* lint */
        break;
    }

    /* enhance weapon regardless of alignment or artifact status */
    if (ok_wep(obj)) {
        bless(obj);
        obj->oeroded = obj->oeroded2 = 0;
        obj->oerodeproof = TRUE;
        obj->bknown = obj->rknown = TRUE;
        if (obj->spe < 1)
            obj->spe = 1;
        /* acquire skill in this weapon */
        unrestrict_weapon_skill(weapon_type(obj));
    } else if (class_gift == STRANGE_OBJECT) {
        /* opportunity knocked, but there was nobody home... */
        You_feel("无价值的.");
    }
    update_inventory();

    /* lastly, confer an extra skill slot/credit beyond the
       up-to-29 you can get from gaining experience levels */
    add_weapon_skill(1);
    return;
}

STATIC_OVL void
pleased(g_align)
aligntyp g_align;
{
    /* don't use p_trouble, worst trouble may get fixed while praying */
    int trouble = in_trouble(); /* what's your worst difficulty? */
    int pat_on_head = 0, kick_on_butt;

    You_feel("%s 是%s.", align_gname(g_align),
             (u.ualign.record >= DEVOUT)
                 ? Hallucination ? "得意洋洋的" : "很高兴的"
                 : (u.ualign.record >= STRIDENT)
                       ? Hallucination ? "易激动的" : "高兴的"
                       : Hallucination ? "吃饱的" : "满意的");

    /* not your deity */
    if (on_altar() && p_aligntyp != u.ualign.type) {
        adjalign(-1);
        return;
    } else if (u.ualign.record < 2 && trouble <= 0)
        adjalign(1);

    /*
     * Depending on your luck & align level, the god you prayed to will:
     *  - fix your worst problem if it's major;
     *  - fix all your major problems;
     *  - fix your worst problem if it's minor;
     *  - fix all of your problems;
     *  - do you a gratuitous favor.
     *
     * If you make it to the the last category, you roll randomly again
     * to see what they do for you.
     *
     * If your luck is at least 0, then you are guaranteed rescued from
     * your worst major problem.
     */
    if (!trouble && u.ualign.record >= DEVOUT) {
        /* if hero was in trouble, but got better, no special favor */
        if (p_trouble == 0)
            pat_on_head = 1;
    } else {
        int action, prayer_luck;
        int tryct = 0;

        /* Negative luck is normally impossible here (can_pray() forces
           prayer failure in that situation), but it's possible for
           Luck to drop during the period of prayer occupation and
           become negative by the time we get here.  [Reported case
           was lawful character whose stinking cloud caused a delayed
           killing of a peaceful human, triggering the "murderer"
           penalty while successful prayer was in progress.  It could
           also happen due to inconvenient timing on Friday 13th, but
           the magnitude there (-1) isn't big enough to cause trouble.]
           We don't bother remembering start-of-prayer luck, just make
           sure it's at least -1 so that Luck+2 is big enough to avoid
           a divide by zero crash when generating a random number.  */
        prayer_luck = max(Luck, -1); /* => (prayer_luck + 2 > 0) */
        action = rn1(prayer_luck + (on_altar() ? 3 + on_shrine() : 2), 1);
        if (!on_altar())
            action = min(action, 3);
        if (u.ualign.record < STRIDENT)
            action = (u.ualign.record > 0 || !rnl(2)) ? 1 : 0;

        switch (min(action, 5)) {
        case 5:
            pat_on_head = 1;
        case 4:
            do
                fix_worst_trouble(trouble);
            while ((trouble = in_trouble()) != 0);
            break;

        case 3:
            fix_worst_trouble(trouble);
        case 2:
            /* arbitrary number of tries */
            while ((trouble = in_trouble()) > 0 && (++tryct < 10))
                fix_worst_trouble(trouble);
            break;

        case 1:
            if (trouble > 0)
                fix_worst_trouble(trouble);
        case 0:
            break; /* your god blows you off, too bad */
        }
    }

    /* note: can't get pat_on_head unless all troubles have just been
       fixed or there were no troubles to begin with; hallucination
       won't be in effect so special handling for it is superfluous */
    if (pat_on_head)
        switch (rn2((Luck + 6) >> 1)) {
        case 0:
            break;
        case 1:
            if (uwep && (welded(uwep) || uwep->oclass == WEAPON_CLASS
                         || is_weptool(uwep))) {
                char repair_buf[BUFSZ];

                *repair_buf = '\0';
                if (uwep->oeroded || uwep->oeroded2)
                    Sprintf(repair_buf, " 然后现在%s像新的一样好",
                            otense(uwep, "就"));

                if (uwep->cursed) {
                    if (!Blind) {
                        pline("%s %s光芒%s.", Yobjnam2(uwep, "发出柔和的"),
                              hcolor(NH_AMBER), repair_buf);
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
                        You_feel("%s的力量超越了%s.", u_gname(),
                                 yname(uwep));
                    uncurse(uwep);
                    uwep->bknown = TRUE;
                    *repair_buf = '\0';
                } else if (!uwep->blessed) {
                    if (!Blind) {
                        pline("%s 带有%s 光环%s.",
                              Yobjnam2(uwep, "发出柔和的光芒"),
                              hcolor(NH_LIGHT_BLUE), repair_buf);
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                    } else
                        You_feel("%s的祝福超越了%s.", u_gname(),
                                 yname(uwep));
                    bless(uwep);
                    uwep->bknown = TRUE;
                    *repair_buf = '\0';
                }

                /* fix any rust/burn/rot damage, but don't protect
                   against future damage */
                if (uwep->oeroded || uwep->oeroded2) {
                    uwep->oeroded = uwep->oeroded2 = 0;
                    /* only give this message if we didn't just bless
                       or uncurse (which has already given a message) */
                    if (*repair_buf)
                        pline("%s 就像新的一样!",
                              Yobjnam2(uwep, Blind ? "感觉" : "看起来"));
                }
                update_inventory();
            }
            break;
        case 3:
            /* takes 2 hints to get the music to enter the stronghold;
               skip if you've solved it via mastermind or destroyed the
               drawbridge (both set uopened_dbridge) or if you've already
               travelled past the Valley of the Dead (gehennom_entered) */
            if (!u.uevent.uopened_dbridge && !u.uevent.gehennom_entered) {
                if (u.uevent.uheard_tune < 1) {
                    godvoice(g_align, (char *) 0);
                    verbalize("听, %s!", youmonst.data->mlet == S_HUMAN
                                               ? "凡人"
                                               : "生物");
                    verbalize(
                       "要进入城堡, 你必须演奏出正确的曲调!");
                    u.uevent.uheard_tune++;
                    break;
                } else if (u.uevent.uheard_tune < 2) {
                    You_hear("一种神圣的音乐...");
                    pline("它听起来像:  \"%s\".", tune);
                    u.uevent.uheard_tune++;
                    break;
                }
            }
        /* Otherwise, falls into next case */
        case 2:
            if (!Blind)
                You("被%s光芒所环绕.", an(hcolor(NH_GOLDEN)));
            /* if any levels have been lost (and not yet regained),
               treat this effect like blessed full healing */
            if (u.ulevel < u.ulevelmax) {
                u.ulevelmax -= 1; /* see potion.c */
                pluslvl(FALSE);
            } else {
                u.uhpmax += 5;
                if (Upolyd)
                    u.mhmax += 5;
            }
            u.uhp = u.uhpmax;
            if (Upolyd)
                u.mh = u.mhmax;
            ABASE(A_STR) = AMAX(A_STR);
            if (u.uhunger < 900)
                init_uhunger();
            if (u.uluck < 0)
                u.uluck = 0;
            make_blinded(0L, TRUE);
            context.botl = 1;
            break;
        case 4: {
            register struct obj *otmp;
            int any = 0;

            if (Blind)
                You_feel("到%s的力量.", u_gname());
            else
                You("被%s 光环所环绕.", an(hcolor(NH_LIGHT_BLUE)));
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                if (otmp->cursed) {
                    if (!Blind) {
                        pline("%s %s光芒.", Yobjnam2(otmp, "发出柔和的"),
                              hcolor(NH_AMBER));
                        iflags.last_msg = PLNMSG_OBJ_GLOWS;
                        otmp->bknown = TRUE;
                        ++any;
                    }
                    uncurse(otmp);
                }
            }
            if (any)
                update_inventory();
            break;
        }
        case 5: {
            static NEARDATA const char msg[] =
                "\"因此我赐予你%s的礼物!\"";

            godvoice(u.ualign.type,
                     "你的进步让我很欣慰,");
            if (!(HTelepat & INTRINSIC)) {
                HTelepat |= FROMOUTSIDE;
                pline(msg, "感知能力");
                if (Blind)
                    see_monsters();
            } else if (!(HFast & INTRINSIC)) {
                HFast |= FROMOUTSIDE;
                pline(msg, "快速");
            } else if (!(HStealth & INTRINSIC)) {
                HStealth |= FROMOUTSIDE;
                pline(msg, "潜行");
            } else {
                if (!(HProtection & INTRINSIC)) {
                    HProtection |= FROMOUTSIDE;
                    if (!u.ublessed)
                        u.ublessed = rn1(3, 2);
                } else
                    u.ublessed++;
                pline(msg, "我的保护");
            }
            verbalize("以我的名义明智地使用它!");
            break;
        }
        case 7:
        case 8:
            if (u.ualign.record >= PIOUS && !u.uevent.uhand_of_elbereth) {
                gcrownu();
                break;
            } /* else FALLTHRU */
        case 6: {
            struct obj *otmp;
            int sp_no, trycnt = u.ulevel + 1;

            /* not yet known spells given preference over already known ones
             */
            /* Also, try to grant a spell for which there is a skill slot */
            otmp = mkobj(SPBOOK_CLASS, TRUE);
            while (--trycnt > 0) {
                if (otmp->otyp != SPE_BLANK_PAPER) {
                    for (sp_no = 0; sp_no < MAXSPELL; sp_no++)
                        if (spl_book[sp_no].sp_id == otmp->otyp)
                            break;
                    if (sp_no == MAXSPELL
                        && !P_RESTRICTED(spell_skilltype(otmp->otyp)))
                        break; /* usable, but not yet known */
                } else {
                    if (!objects[SPE_BLANK_PAPER].oc_name_known
                        || carrying(MAGIC_MARKER))
                        break;
                }
                otmp->otyp = rnd_class(bases[SPBOOK_CLASS], SPE_BLANK_PAPER);
            }
            bless(otmp);
            at_your_feet("一本魔法书");
            place_object(otmp, u.ux, u.uy);
            newsym(u.ux, u.uy);
            break;
        }
        default:
            impossible("Confused deity!");
            break;
        }

    u.ublesscnt = rnz(350);
    kick_on_butt = u.uevent.udemigod ? 1 : 0;
    if (u.uevent.uhand_of_elbereth)
        kick_on_butt++;
    if (kick_on_butt)
        u.ublesscnt += kick_on_butt * rnz(1000);

    return;
}

/* either blesses or curses water on the altar,
 * returns true if it found any water here.
 */
STATIC_OVL boolean
water_prayer(bless_water)
boolean bless_water;
{
    register struct obj *otmp;
    register long changed = 0;
    boolean other = FALSE, bc_known = !(Blind || Hallucination);

    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        /* turn water into (un)holy water */
        if (otmp->otyp == POT_WATER
            && (bless_water ? !otmp->blessed : !otmp->cursed)) {
            otmp->blessed = bless_water;
            otmp->cursed = !bless_water;
            otmp->bknown = bc_known;
            changed += otmp->quan;
        } else if (otmp->oclass == POTION_CLASS)
            other = TRUE;
    }
    if (!Blind && changed) {
        pline("%s 药水%s 在祭坛上发出%s %s光芒了片刻.",
              ((other && changed > 1L) ? "一些"
                                       : (other ? "一瓶" : "")),
              ((other || changed > 1L) ? "" : ""), (changed > 1L ? "" : ""),
              (bless_water ? hcolor(NH_LIGHT_BLUE) : hcolor(NH_BLACK)));
    }
    return (boolean) (changed > 0L);
}

STATIC_OVL void
godvoice(g_align, words)
aligntyp g_align;
const char *words;
{
    const char *quot = "";

    if (words)
        quot = "\"";
    else
        words = "";

    pline_The("%s的声音%s: %s %s%s", align_gname(g_align),
              godvoices[rn2(SIZE(godvoices))], quot, words, quot);
}

STATIC_OVL void
gods_angry(g_align)
aligntyp g_align;
{
    godvoice(g_align, "汝触怒了吾.");
}

/* The g_align god is upset with you. */
STATIC_OVL void
gods_upset(g_align)
aligntyp g_align;
{
    if (g_align == u.ualign.type)
        u.ugangr++;
    else if (u.ugangr)
        u.ugangr--;
    angrygods(g_align);
}

STATIC_OVL void
consume_offering(otmp)
register struct obj *otmp;
{
    if (Hallucination)
        switch (rn2(3)) {
        case 0:
            Your("祭品长出了翅膀和螺旋桨并怒吼着离去了!");
            break;
        case 1:
            Your("祭品开始膨胀, 并越来越大, 发出爆裂声!");
            break;
        case 2:
            Your(
     "祭品瓦解为一团舞云并逐渐消失了!");
            break;
        }
    else if (Blind && u.ualign.type == A_LAWFUL)
        Your("祭品消失了!");
    else
        Your("祭品在%s中消耗掉了!",
             u.ualign.type == A_LAWFUL ? "闪光" : "爆裂的火焰");
    if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
    exercise(A_WIS, TRUE);
}

int
dosacrifice()
{
    static NEARDATA const char cloud_of_smoke[] =
        "一团%s烟围绕着你...";
    register struct obj *otmp;
    int value = 0, pm;
    boolean highaltar;
    aligntyp altaralign = a_align(u.ux, u.uy);

    if (!on_altar() || u.uswallow) {
        You("没有站在祭坛上.");
        return 0;
    }
    highaltar = ((Is_astralevel(&u.uz) || Is_sanctum(&u.uz))
                 && (levl[u.ux][u.uy].altarmask & AM_SHRINE));

    otmp = floorfood("献祭", 1);  //sacrifice
    if (!otmp)
        return 0;
    /*
     * Was based on nutritional value and aging behavior (< 50 moves).
     * Sacrificing a food ration got you max luck instantly, making the
     * gods as easy to please as an angry dog!
     *
     * Now only accepts corpses, based on the game's evaluation of their
     * toughness.  Human and pet sacrifice, as well as sacrificing unicorns
     * of your alignment, is strongly discouraged.
     */
#define MAXVALUE 24 /* Highest corpse value (besides Wiz) */

    if (otmp->otyp == CORPSE) {
        register struct permonst *ptr = &mons[otmp->corpsenm];
        struct monst *mtmp;
        extern const int monstr[];

        /* KMH, conduct */
        u.uconduct.gnostic++;

        /* you're handling this corpse, even if it was killed upon the altar
         */
        feel_cockatrice(otmp, TRUE);
        if (rider_corpse_revival(otmp, FALSE))
            return 1;

        if (otmp->corpsenm == PM_ACID_BLOB
            || (monstermoves <= peek_at_iced_corpse_age(otmp) + 50)) {
            value = monstr[otmp->corpsenm] + 1;
            if (otmp->oeaten)
                value = eaten_stat(value, otmp);
        }

        if (your_race(ptr)) {
            if (is_demon(youmonst.data)) {
                You("发现这个想法很令人满意.");
                exercise(A_WIS, TRUE);
            } else if (u.ualign.type != A_CHAOTIC) {
                pline("你会为这种无耻的罪行而后悔!");
                exercise(A_WIS, FALSE);
            }

            if (highaltar
                && (altaralign != A_CHAOTIC || u.ualign.type != A_CHAOTIC)) {
                goto desecrate_high_altar;
            } else if (altaralign != A_CHAOTIC && altaralign != A_NONE) {
                /* curse the lawful/neutral altar */
                pline_The("祭坛被沾染上%s 的血液.", urace.adj);
                levl[u.ux][u.uy].altarmask = AM_CHAOTIC;
                angry_priest();
            } else {
                struct monst *dmon;
                const char *demonless_msg;

                /* Human sacrifice on a chaotic or unaligned altar */
                /* is equivalent to demon summoning */
                if (altaralign == A_CHAOTIC && u.ualign.type != A_CHAOTIC) {
                    pline(
                    "血液淹没了祭坛, 祭坛消失在%s 云里!",
                          hcolor(NH_BLACK));
                    levl[u.ux][u.uy].typ = ROOM;
                    levl[u.ux][u.uy].altarmask = 0;
                    newsym(u.ux, u.uy);
                    angry_priest();
                    demonless_msg = "云消散了";
                } else {
                    /* either you're chaotic or altar is Moloch's or both */
                    pline_The("血液覆盖了祭坛!");
                    change_luck(altaralign == A_NONE ? -2 : 2);
                    demonless_msg = "血液凝固了";
                }
                if ((pm = dlord(altaralign)) != NON_PM
                    && (dmon = makemon(&mons[pm], u.ux, u.uy, NO_MM_FLAGS))
                           != 0) {
                    char dbuf[BUFSZ];

                    Strcpy(dbuf, a_monnam(dmon));
                    if (!strcmpi(dbuf, "它"))
                        Strcpy(dbuf, "可怕的什么东西");
                    else
                        dmon->mstrategy &= ~STRAT_APPEARMSG;
                    You("召唤了%s!", dbuf);
                    if (sgn(u.ualign.type) == sgn(dmon->data->maligntyp))
                        dmon->mpeaceful = TRUE;
                    You("被惊吓住, 无法移动了.");
                    nomul(-3);
                    multi_reason = "被一个恶魔吓坏了";
                    nomovemsg = 0;
                } else
                    pline_The("%s.", demonless_msg);
            }

            if (u.ualign.type != A_CHAOTIC) {
                adjalign(-5);
                u.ugangr += 3;
                (void) adjattrib(A_WIS, -1, TRUE);
                if (!Inhell)
                    angrygods(u.ualign.type);
                change_luck(-5);
            } else
                adjalign(5);
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            return 1;
        } else if (has_omonst(otmp)
                   && (mtmp = get_mtraits(otmp, FALSE)) != 0
                   && mtmp->mtame) {
                /* mtmp is a temporary pointer to a tame monster's attributes,
                 * not a real monster */
            pline("所以这就是你如何回报忠诚?");
            adjalign(-3);
            value = -1;
            HAggravate_monster |= FROMOUTSIDE;
        } else if (is_undead(ptr)) { /* Not demons--no demon corpses */
            if (u.ualign.type != A_CHAOTIC)
                value += 1;
        } else if (is_unicorn(ptr)) {
            int unicalign = sgn(ptr->maligntyp);

            if (unicalign == altaralign) {
                /* When same as altar, always a very bad action.
                 */
                pline("这样的行为对%s是一种侮辱!",
                      (unicalign == A_CHAOTIC) ? "混沌"
                         : unicalign ? "秩序" : "平衡");
                (void) adjattrib(A_WIS, -1, TRUE);
                value = -5;
            } else if (u.ualign.type == altaralign) {
                /* When different from altar, and altar is same as yours,
                 * it's a very good action.
                 */
                if (u.ualign.record < ALIGNLIM)
                    You_feel("合适的 %s.", align_str(u.ualign.type));
                else
                    You_feel("你彻底地在正确的道路上.");
                adjalign(5);
                value += 3;
            } else if (unicalign == u.ualign.type) {
                /* When sacrificing unicorn of your alignment to altar not of
                 * your alignment, your god gets angry and it's a conversion.
                 */
                u.ualign.record = -1;
                value = 1;
            } else {
                /* Otherwise, unicorn's alignment is different from yours
                 * and different from the altar's.  It's an ordinary (well,
                 * with a bonus) sacrifice on a cross-aligned altar.
                 */
                value += 3;
            }
        }
    } /* corpse */

    if (otmp->otyp == AMULET_OF_YENDOR) {
        if (!highaltar) {
        too_soon:
            if (altaralign == A_NONE && Inhell)
                /* hero has left Moloch's Sanctum so is in the process
                   of getting away with the Amulet (outside of Gehennom,
                   fall through to the "ashamed" feedback) */
                gods_upset(A_NONE);
            else
                You_feel("%s.",
                         Hallucination
                            ? "思乡的"
                            /* if on track, give a big hint */
                            : (altaralign == u.ualign.type)
                               ? "到返回表面的冲动"
                               /* else headed towards celestial disgrace */
                               : "羞愧的");
            return 1;
        } else {
            /* The final Test.  Did you win? */
            if (uamul == otmp)
                Amulet_off();
            u.uevent.ascended = 1;
            if (carried(otmp))
                useup(otmp); /* well, it's gone now */
            else
                useupf(otmp, 1L);
            You("把岩德护身符献给%s...", a_gname());
            if (altaralign == A_NONE) {
                /* Moloch's high altar */
                if (u.ualign.record > -99)
                    u.ualign.record = -99;
                /*[apparently shrug/snarl can be sensed without being seen]*/
                pline("%s 耸了耸肩并保持着对%s的统治,", Moloch,
                      u_gname());
                pline("然后残忍地扼杀了你的生命.");
                Sprintf(killer.name, "%s 冷漠", s_suffix(Moloch));
                killer.format = KILLED_BY;
                done(DIED);
                /* life-saved (or declined to die in wizard/explore mode) */
                pline("%s 怒吼并再来了一次...", Moloch);
                fry_by_god(A_NONE, TRUE); /* wrath of Moloch */
                /* declined to die in wizard or explore mode */
                pline(cloud_of_smoke, hcolor(NH_BLACK));
                done(ESCAPED);
            } else if (u.ualign.type != altaralign) {
                /* And the opposing team picks you up and
                   carries you off on their shoulders */
                adjalign(-99);
                pline("%s 接受了你的礼物, 然后获得了对%s的统治...",
                      a_gname(), u_gname());
                pline("%s 是暴怒的...", u_gname());
                pline("幸运的是, %s 准许你活着...", a_gname());
                pline(cloud_of_smoke, hcolor(NH_ORANGE));
                done(ESCAPED);
            } else { /* super big win */
                adjalign(10);
                u.uachieve.ascended = 1;
                pline(
               "一个看不见的唱诗班在歌唱, 你沐浴在光辉之中...");
                godvoice(altaralign, "祝贺, 凡人!");
                display_nhwindow(WIN_MESSAGE, FALSE);
                verbalize(
          "为汝之功之报, 吾赐尔不朽之礼!");
                You("升为%s半神的地位...",
                    flags.female ? "女" : "");
                done(ASCENDED);
            }
        }
    } /* real Amulet */

    if (otmp->otyp == FAKE_AMULET_OF_YENDOR) {
        if (!highaltar && !otmp->known)
            goto too_soon;
        You_hear("附近的雷声.");
        if (!otmp->known) {
            You("认识到你造成了一个%s.",
                Hallucination ? "疏忽" : "错误");
            otmp->known = TRUE;
            change_luck(-1);
            return 1;
        } else {
            /* don't you dare try to fool the gods */
            if (Deaf)
                pline("噢, 不."); /* didn't hear thunderclap */
            change_luck(-3);
            adjalign(-1);
            u.ugangr += 3;
            value = -3;
        }
    } /* fake Amulet */

    if (value == 0) {
        pline1(nothing_happens);
        return 1;
    }

    if (altaralign != u.ualign.type && highaltar) {
    desecrate_high_altar:
        /*
         * REAL BAD NEWS!!! High altars cannot be converted.  Even an attempt
         * gets the god who owns it truly pissed off.
         */
        You_feel("你周围的气氛变得紧张...");
        pline("突然, 你意识到%s 在注意你...", a_gname());
        godvoice(altaralign,
                 "所以, 凡人!  你敢亵渎我的圣殿!");
        /* Throw everything we have at the player */
        god_zaps_you(altaralign);
    } else if (value
               < 0) { /* I don't think the gods are gonna like this... */
        gods_upset(altaralign);
    } else {
        int saved_anger = u.ugangr;
        int saved_cnt = u.ublesscnt;
        int saved_luck = u.uluck;

        /* Sacrificing at an altar of a different alignment */
        if (u.ualign.type != altaralign) {
            /* Is this a conversion ? */
            /* An unaligned altar in Gehennom will always elicit rejection. */
            if (ugod_is_angry() || (altaralign == A_NONE && Inhell)) {
                if (u.ualignbase[A_CURRENT] == u.ualignbase[A_ORIGINAL]
                    && altaralign != A_NONE) {
                    You("有一种强烈的感觉%s生气了...",
                        u_gname());
                    consume_offering(otmp);
                    pline("%s 接受了你的忠诚.", a_gname());

                    uchangealign(altaralign, 0);
                    /* Beware, Conversion is costly */
                    change_luck(-3);
                    u.ublesscnt += 300;
                } else {
                    u.ugangr += 3;
                    adjalign(-5);
                    pline("%s 拒绝了你的祭品!", a_gname());
                    godvoice(altaralign, "受苦吧, 异教徒!");
                    change_luck(-5);
                    (void) adjattrib(A_WIS, -2, TRUE);
                    if (!Inhell)
                        angrygods(u.ualign.type);
                }
                return 1;
            } else {
                consume_offering(otmp);
                You("感觉到%s 和%s之间的冲突.", u_gname(),
                    a_gname());
                if (rn2(8 + u.ulevel) > 5) {
                    struct monst *pri;
                    You_feel("%s的力量在增加.", u_gname());
                    exercise(A_WIS, TRUE);
                    change_luck(1);
                    /* Yes, this is supposed to be &=, not |= */
                    levl[u.ux][u.uy].altarmask &= AM_SHRINE;
                    /* the following accommodates stupid compilers */
                    levl[u.ux][u.uy].altarmask =
                        levl[u.ux][u.uy].altarmask
                        | (Align2amask(u.ualign.type));
                    if (!Blind)
                        pline_The("祭坛发出%s光芒.",
                                  hcolor((u.ualign.type == A_LAWFUL)
                                            ? NH_WHITE
                                            : u.ualign.type
                                               ? NH_BLACK
                                               : (const char *) "灰色的"));

                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (3 * ALIGNLIM) / 4)
                        summon_minion(altaralign, TRUE);
                    /* anger priest; test handles bones files */
                    if ((pri = findpriest(temple_occupied(u.urooms)))
                        && !p_coaligned(pri))
                        angry_priest();
                } else {
                    pline("不幸运, 你感觉%s的力量在减少.",
                          u_gname());
                    change_luck(-1);
                    exercise(A_WIS, FALSE);
                    if (rnl(u.ulevel) > 6 && u.ualign.record > 0
                        && rnd(u.ualign.record) > (7 * ALIGNLIM) / 8)
                        summon_minion(altaralign, TRUE);
                }
                return 1;
            }
        }

        consume_offering(otmp);
        /* OK, you get brownie points. */
        if (u.ugangr) {
            u.ugangr -= ((value * (u.ualign.type == A_CHAOTIC ? 2 : 3))
                         / MAXVALUE);
            if (u.ugangr < 0)
                u.ugangr = 0;
            if (u.ugangr != saved_anger) {
                if (u.ugangr) {
                    pline("%s 似乎%s.", u_gname(),
                          Hallucination ? "状态很好" : "稍微宽慰了");

                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
                    pline("%s 似乎%s.", u_gname(),
                          Hallucination ? "广大无边 (not a new fact)"
                                        : "宽慰了");

                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            } else { /* not satisfied yet */
                if (Hallucination)
                    pline_The("神似乎很苛刻.");
                else
                    You("有一种不适当的感觉.");
            }
        } else if (ugod_is_angry()) {
            if (value > MAXVALUE)
                value = MAXVALUE;
            if (value > -u.ualign.record)
                value = -u.ualign.record;
            adjalign(value);
            You_feel("不完全地被赦免了.");
        } else if (u.ublesscnt > 0) {
            u.ublesscnt -= ((value * (u.ualign.type == A_CHAOTIC ? 500 : 300))
                            / MAXVALUE);
            if (u.ublesscnt < 0)
                u.ublesscnt = 0;
            if (u.ublesscnt != saved_cnt) {
                if (u.ublesscnt) {
                    if (Hallucination)
                        You("认识到神们不像你和我.");
                    else
                        You("有一种希望的感觉.");
                    if ((int) u.uluck < 0)
                        change_luck(1);
                } else {
                    if (Hallucination)
                        pline("总的来说, 有一种炸洋葱的味道.");
                    else
                        You("有一种和解的感觉.");
                    if ((int) u.uluck < 0)
                        u.uluck = 0;
                }
            }
        } else {
            int nartifacts = nartifact_exist();

            /* you were already in pretty good standing */
            /* The player can gain an artifact */
            /* The chance goes down as the number of artifacts goes up */
            if (u.ulevel > 2 && u.uluck >= 0
                && !rn2(10 + (2 * u.ugifts * nartifacts))) {
                otmp = mk_artifact((struct obj *) 0, a_align(u.ux, u.uy));
                if (otmp) {
                    if (otmp->spe < 0)
                        otmp->spe = 0;
                    if (otmp->cursed)
                        uncurse(otmp);
                    otmp->oerodeproof = TRUE;
                    at_your_feet("一个东西");
                    dropy(otmp);
                    godvoice(u.ualign.type, "明智地使用我的礼物!");
                    u.ugifts++;
                    u.ublesscnt = rnz(300 + (50 * nartifacts));
                    exercise(A_WIS, TRUE);
                    /* make sure we can use this weapon */
                    unrestrict_weapon_skill(weapon_type(otmp));
                    if (!Hallucination && !Blind) {
                        otmp->dknown = 1;
                        makeknown(otmp->otyp);
                        discover_artifact(otmp->oartifact);
                    }
                    return 1;
                }
            }
            change_luck((value * LUCKMAX) / (MAXVALUE * 2));
            if ((int) u.uluck < 0)
                u.uluck = 0;
            if (u.uluck != saved_luck) {
                if (Blind)
                    You("觉得%s擦过了你的%s.", something,
                        body_part(FOOT));
                else
                    You(Hallucination
                    ? "看见马唐草在你的%s上.  地牢中一个有趣的事物."
                            : "瞥见四叶草在你的%s上.",
                        makeplural(body_part(FOOT)));
            }
        }
    }
    return 1;
}

/* determine prayer results in advance; also used for enlightenment */
boolean
can_pray(praying)
boolean praying; /* false means no messages should be given */
{
    int alignment;

    p_aligntyp = on_altar() ? a_align(u.ux, u.uy) : u.ualign.type;
    p_trouble = in_trouble();

    if (is_demon(youmonst.data) && (p_aligntyp != A_CHAOTIC)) {
        if (praying)
            pline_The("向一个%s神祈祷的想法会让你厌恶.",
                      p_aligntyp ? "秩序" : "中立");
        return FALSE;
    }

    if (praying)
        You("开始向%s 祈祷.", align_gname(p_aligntyp));

    if (u.ualign.type && u.ualign.type == -p_aligntyp)
        alignment = -u.ualign.record; /* Opposite alignment altar */
    else if (u.ualign.type != p_aligntyp)
        alignment = u.ualign.record / 2; /* Different alignment altar */
    else
        alignment = u.ualign.record;

    if ((p_trouble > 0) ? (u.ublesscnt > 200)      /* big trouble */
           : (p_trouble < 0) ? (u.ublesscnt > 100) /* minor difficulties */
              : (u.ublesscnt > 0))                 /* not in trouble */
        p_type = 0;                     /* too soon... */
    else if ((int) Luck < 0 || u.ugangr || alignment < 0)
        p_type = 1; /* too naughty... */
    else /* alignment >= 0 */ {
        if (on_altar() && u.ualign.type != p_aligntyp)
            p_type = 2;
        else
            p_type = 3;
    }

    if (is_undead(youmonst.data) && !Inhell
        && (p_aligntyp == A_LAWFUL || (p_aligntyp == A_NEUTRAL && !rn2(10))))
        p_type = -1;
    /* Note:  when !praying, the random factor for neutrals makes the
       return value a non-deterministic approximation for enlightenment.
       This case should be uncommon enough to live with... */

    return !praying ? (boolean) (p_type == 3 && !Inhell) : TRUE;
}

/* #pray commmand */
int
dopray()
{
    /* Confirm accidental slips of Alt-P */
    if (ParanoidPray && yn("你确定要祈祷吗?") != 'y')
        return 0;

    u.uconduct.gnostic++;

    /* set up p_type and p_alignment */
    if (!can_pray(TRUE))
        return 0;

    if (wizard && p_type >= 0) {
        if (yn("让神为高兴状态?") == 'y') {
            u.ublesscnt = 0;
            if (u.uluck < 0)
                u.uluck = 0;
            if (u.ualign.record <= 0)
                u.ualign.record = 1;
            u.ugangr = 0;
            if (p_type < 2)
                p_type = 3;
        }
    }
    nomul(-3);
    multi_reason = "祈祷";
    nomovemsg = "你完成了你的祈祷.";
    afternmv = prayer_done;

    if (p_type == 3 && !Inhell) {
        /* if you've been true to your god you can't die while you pray */
        if (!Blind)
            You("被微弱的光所环绕.");
        u.uinvulnerable = TRUE;
    }

    return 1;
}

STATIC_PTR int
prayer_done() /* M. Stephenson (1.0.3b) */
{
    aligntyp alignment = p_aligntyp;

    u.uinvulnerable = FALSE;
    if (p_type == -1) {
        godvoice(alignment,
                 (alignment == A_LAWFUL)
                    ? "卑鄙的生物, 汝敢号令吾?"
                    : "不要再这样走下去了, 是对自然的歪曲!");
        You_feel("你像是四分五裂了.");
        /* KMH -- Gods have mastery over unchanging */
        rehumanize();
        /* no Half_physical_damage adjustment here */
        losehp(rnd(20), "残余的超度效果", KILLED_BY_AN);
        exercise(A_CON, FALSE);
        return 1;
    }
    if (Inhell) {
        pline("自从你来到了葛汉诺姆, %s 不会再帮助你.",
              align_gname(alignment));
        /* haltingly aligned is least likely to anger */
        if (u.ualign.record <= 0 || rnl(u.ualign.record))
            angrygods(u.ualign.type);
        return 0;
    }

    if (p_type == 0) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        u.ublesscnt += rnz(250);
        change_luck(-3);
        gods_upset(u.ualign.type);
    } else if (p_type == 1) {
        if (on_altar() && u.ualign.type != alignment)
            (void) water_prayer(FALSE);
        angrygods(u.ualign.type); /* naughty */
    } else if (p_type == 2) {
        if (water_prayer(FALSE)) {
            /* attempted water prayer on a non-coaligned altar */
            u.ublesscnt += rnz(250);
            change_luck(-3);
            gods_upset(u.ualign.type);
        } else
            pleased(alignment);
    } else {
        /* coaligned */
        if (on_altar())
            (void) water_prayer(TRUE);
        pleased(alignment); /* nice */
    }
    return 1;
}

/* #turn command */
int
doturn()
{
    /* Knights & Priest(esse)s only please */
    struct monst *mtmp, *mtmp2;
    int once, range, xlev;

    if (!Role_if(PM_PRIEST) && !Role_if(PM_KNIGHT)) {
        /* Try to use the "turn undead" spell.
         *
         * This used to be based on whether hero knows the name of the
         * turn undead spellbook, but it's possible to know--and be able
         * to cast--the spell while having lost the book ID to amnesia.
         * (It also used to tell spelleffects() to cast at self?)
         */
        int sp_no;

        for (sp_no = 0; sp_no < MAXSPELL; ++sp_no) {
            if (spl_book[sp_no].sp_id == NO_SPELL)
                break;
            else if (spl_book[sp_no].sp_id == SPE_TURN_UNDEAD)
                return spelleffects(sp_no, FALSE);
        }
        You("不知道如何超度!");
        return 0;
    }
    u.uconduct.gnostic++;

    if ((u.ualign.type != A_CHAOTIC
         && (is_demon(youmonst.data) || is_undead(youmonst.data)))
        || u.ugangr > 6) { /* "Die, mortal!" */
        pline("不知何故, %s 似乎无视了你.", u_gname());
        aggravate();
        exercise(A_WIS, FALSE);
        return 0;
    }
    if (Inhell) {
        pline("自从你来到了葛汉诺姆, %s 不会再帮助你.", u_gname());
        aggravate();
        return 0;
    }
    pline("你咏唱一首奥术式来呼唤 %s.", u_gname());
    exercise(A_WIS, TRUE);

    /* note: does not perform unturn_dead() on victims' inventories */
    range = BOLT_LIM + (u.ulevel / 5); /* 5 to 11 */
    range *= range;
    once = 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp2) {
        mtmp2 = mtmp->nmon;

        if (DEADMONSTER(mtmp))
            continue;
        if (!cansee(mtmp->mx, mtmp->my) || distu(mtmp->mx, mtmp->my) > range)
            continue;

        if (!mtmp->mpeaceful
            && (is_undead(mtmp->data) || is_vampshifter(mtmp)
                || (is_demon(mtmp->data) && (u.ulevel > (MAXULEV / 2))))) {
            mtmp->msleeping = 0;
            if (Confusion) {
                if (!once++)
                    pline("不幸的是, 你的声音结结巴巴.");
                mtmp->mflee = 0;
                mtmp->mfrozen = 0;
                mtmp->mcanmove = 1;
            } else if (!resist(mtmp, '\0', 0, TELL)) {
                xlev = 6;
                switch (mtmp->data->mlet) {
                /* this is intentional, lichs are tougher
                   than zombies. */
                case S_LICH:
                    xlev += 2; /*FALLTHRU*/
                case S_GHOST:
                    xlev += 2; /*FALLTHRU*/
                case S_VAMPIRE:
                    xlev += 2; /*FALLTHRU*/
                case S_WRAITH:
                    xlev += 2; /*FALLTHRU*/
                case S_MUMMY:
                    xlev += 2; /*FALLTHRU*/
                case S_ZOMBIE:
                    if (u.ulevel >= xlev && !resist(mtmp, '\0', 0, NOTELL)) {
                        if (u.ualign.type == A_CHAOTIC) {
                            mtmp->mpeaceful = 1;
                            set_malign(mtmp);
                        } else { /* damn them */
                            killed(mtmp);
                        }
                        break;
                    } /* else flee */
                /*FALLTHRU*/
                default:
                    monflee(mtmp, 0, FALSE, TRUE);
                    break;
                }
            }
        }
    }
    nomul(-5);
    multi_reason = "试图超度怪物";
    nomovemsg = You_can_move_again;
    return 1;
}

const char *
a_gname()
{
    return a_gname_at(u.ux, u.uy);
}

/* returns the name of an altar's deity */
const char *
a_gname_at(x, y)
xchar x, y;
{
    if (!IS_ALTAR(levl[x][y].typ))
        return (char *) 0;

    return align_gname(a_align(x, y));
}

/* returns the name of the hero's deity */
const char *
u_gname()
{
    return align_gname(u.ualign.type);
}

const char *
align_gname(alignment)
aligntyp alignment;
{
    const char *gnam;

    switch (alignment) {
    case A_NONE:
        gnam = Moloch;
        break;
    case A_LAWFUL:
        gnam = urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = urole.cgod;
        break;
    default:
        impossible("unknown alignment.");
        gnam = "someone";
        break;
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

static const char *hallu_gods[] = {
    "飞行的意大利面条怪物", /* Church of the FSM */
    "厄里斯",                         /* Discordianism */
    "火星人",                 /* every science fiction ever */
    "Xom",                          /* Crawl */
    "和天龙",                 /* ADOM */
    "岩德中央银行",   /* economics */
    "牙仙子",                  /* real world(?) */
    "奥姆",                           /* Discworld */
    "约格莫夫",                     /* Magic: the Gathering */
    "魔苟斯",                      /* LoTR */
    "克苏鲁",                      /* Lovecraft */
    "the Ori",                      /* Stargate */
    "命运女神",                      /* why not? */
    "你的朋友电脑",     /* Paranoia */
};

/* hallucination handling for priest/minion names: select a random god
   iff character is hallucinating */
const char *
halu_gname(alignment)
aligntyp alignment;
{
    const char *gnam = NULL;
    int which;

    if (!Hallucination)
        return align_gname(alignment);

    /* The priest may not have initialized god names. If this is the
     * case, and we roll priest, we need to try again. */
    do
        which = randrole();
    while (!roles[which].lgod);

    switch (rn2(9)) {
    case 0:
    case 1:
        gnam = roles[which].lgod;
        break;
    case 2:
    case 3:
        gnam = roles[which].ngod;
        break;
    case 4:
    case 5:
        gnam = roles[which].cgod;
        break;
    case 6:
    case 7:
        gnam = hallu_gods[rn2(sizeof hallu_gods / sizeof *hallu_gods)];
        break;
    case 8:
        gnam = Moloch;
        break;
    default:
        impossible("rn2 broken in halu_gname?!?");
    }
    if (!gnam) {
        impossible("No random god name?");
        gnam = "your Friend the Computer"; /* Paranoia */
    }
    if (*gnam == '_')
        ++gnam;
    return gnam;
}

/* deity's title */
const char *
align_gtitle(alignment)
aligntyp alignment;
{
    const char *gnam, *result = "神";

    switch (alignment) {
    case A_LAWFUL:
        gnam = urole.lgod;
        break;
    case A_NEUTRAL:
        gnam = urole.ngod;
        break;
    case A_CHAOTIC:
        gnam = urole.cgod;
        break;
    default:
        gnam = 0;
        break;
    }
    if (gnam && *gnam == '_')
        result = "女神";
    return result;
}

void
altar_wrath(x, y)
register int x, y;
{
    aligntyp altaralign = a_align(x, y);

    if (!strcmp(align_gname(altaralign), u_gname())) {
        godvoice(altaralign, "你竟敢亵渎我的祭坛!");
        (void) adjattrib(A_WIS, -1, FALSE);
    } else {
        pline("一个声音( 莫非是%s?) 低语:", align_gname(altaralign));
        verbalize("你要付出代价, 异教徒!");
        change_luck(-1);
    }
}

/* assumes isok() at one space away, but not necessarily at two */
STATIC_OVL boolean
blocked_boulder(dx, dy)
int dx, dy;
{
    register struct obj *otmp;
    long count = 0L;

    for (otmp = level.objects[u.ux + dx][u.uy + dy]; otmp;
         otmp = otmp->nexthere) {
        if (otmp->otyp == BOULDER)
            count += otmp->quan;
    }

    switch (count) {
    case 0:
        /* no boulders--not blocked */
        return FALSE;
    case 1:
        /* possibly blocked depending on if it's pushable */
        break;
    default:
        /* more than one boulder--blocked after they push the top one;
           don't force them to push it first to find out */
        return TRUE;
    }

    if (!isok(u.ux + 2 * dx, u.uy + 2 * dy))
        return TRUE;
    if (IS_ROCK(levl[u.ux + 2 * dx][u.uy + 2 * dy].typ))
        return TRUE;
    if (sobj_at(BOULDER, u.ux + 2 * dx, u.uy + 2 * dy))
        return TRUE;

    return FALSE;
}

/*pray.c*/
