/* NetHack 3.6	steed.c	$NHDT-Date: 1445906867 2015/10/27 00:47:47 $  $NHDT-Branch: master $:$NHDT-Revision: 1.47 $ */
/* Copyright (c) Kevin Hugo, 1998-1999. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Monsters that might be ridden */
static NEARDATA const char steeds[] = { S_QUADRUPED, S_UNICORN, S_ANGEL,
                                        S_CENTAUR,   S_DRAGON,  S_JABBERWOCK,
                                        '\0' };

STATIC_DCL boolean FDECL(landing_spot, (coord *, int, int));
STATIC_DCL void FDECL(maybewakesteed, (struct monst *));

/* caller has decided that hero can't reach something while mounted */
void
rider_cant_reach()
{
    You("aren't skilled enough to reach from %s.", y_monnam(u.usteed));
}

/*** Putting the saddle on ***/

/* Can this monster wear a saddle? */
boolean
can_saddle(mtmp)
struct monst *mtmp;
{
    struct permonst *ptr = mtmp->data;

    return (index(steeds, ptr->mlet) && (ptr->msize >= MZ_MEDIUM)
            && (!humanoid(ptr) || ptr->mlet == S_CENTAUR) && !amorphous(ptr)
            && !noncorporeal(ptr) && !is_whirly(ptr) && !unsolid(ptr));
}

int
use_saddle(otmp)
struct obj *otmp;
{
    struct monst *mtmp;
    struct permonst *ptr;
    int chance;
    const char *s;

    if (!u_handsy())
        return 0;

    /* Select an animal */
    if (u.uswallow || Underwater || !getdir((char *) 0)) {
        pline1(Never_mind);
        return 0;
    }
    if (!u.dx && !u.dy) {
        pline("给你自己装鞍?  非常滑稽...");
        return 0;
    }
    if (!isok(u.ux + u.dx, u.uy + u.dy)
        || !(mtmp = m_at(u.ux + u.dx, u.uy + u.dy)) || !canspotmon(mtmp)) {
        pline("我看见那里没人.");
        return 1;
    }

    /* Is this a valid monster? */
    if (mtmp->misc_worn_check & W_SADDLE || which_armor(mtmp, W_SADDLE)) {
        pline("%s 不需要又一个.", Monnam(mtmp));
        return 1;
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !uarmg && !Stone_resistance) {
        char kbuf[BUFSZ];

        You("触摸 %s.", mon_nam(mtmp));
        if (!(poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))) {
            Sprintf(kbuf, "试图给 %s 装上鞍", an(mtmp->data->mname));
            instapetrify(kbuf);
        }
    }
    if (ptr == &mons[PM_INCUBUS] || ptr == &mons[PM_SUCCUBUS]) {
        pline("你真不要脸!");
        exercise(A_WIS, FALSE);
        return 1;
    }
    if (mtmp->isminion || mtmp->isshk || mtmp->ispriest || mtmp->isgd
        || mtmp->iswiz) {
        pline("我觉得%s 会介意的.", mon_nam(mtmp));
        return 1;
    }
    if (!can_saddle(mtmp)) {
        You_cant("给这种生物装鞍.");
        return 1;
    }

    /* Calculate your chance */
    chance = ACURR(A_DEX) + ACURR(A_CHA) / 2 + 2 * mtmp->mtame;
    chance += u.ulevel * (mtmp->mtame ? 20 : 5);
    if (!mtmp->mtame)
        chance -= 10 * mtmp->m_lev;
    if (Role_if(PM_KNIGHT))
        chance += 20;
    switch (P_SKILL(P_RIDING)) {
    case P_ISRESTRICTED:
    case P_UNSKILLED:
    default:
        chance -= 20;
        break;
    case P_BASIC:
        break;
    case P_SKILLED:
        chance += 15;
        break;
    case P_EXPERT:
        chance += 30;
        break;
    }
    if (Confusion || Fumbling || Glib)
        chance -= 20;
    else if (uarmg && (s = OBJ_DESCR(objects[uarmg->otyp])) != (char *) 0
             && !strncmp(s, "骑手", 6))
        /* Bonus for wearing "riding" (but not fumbling) gloves */
        chance += 10;
    else if (uarmf && (s = OBJ_DESCR(objects[uarmf->otyp])) != (char *) 0
             && !strncmp(s, "马靴", 6))
        /* ... or for "riding boots" */
        chance += 10;
    if (otmp->cursed)
        chance -= 50;

    /* [intended] steed becomes alert if possible */
    maybewakesteed(mtmp);

    /* Make the attempt */
    if (rn2(100) < chance) {
        You("把鞍放到%s 上.", mon_nam(mtmp));
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        freeinv(otmp);
        /* mpickobj may free otmp it if merges, but we have already
           checked for a saddle above, so no merger should happen */
        (void) mpickobj(mtmp, otmp);
        mtmp->misc_worn_check |= W_SADDLE;
        otmp->owornmask = W_SADDLE;
        otmp->leashmon = mtmp->m_id;
        update_mon_intrinsics(mtmp, otmp, TRUE, FALSE);
    } else
        pline("%s 抵抗!", Monnam(mtmp));
    return 1;
}

/*** Riding the monster ***/

/* Can we ride this monster?  Caller should also check can_saddle() */
boolean
can_ride(mtmp)
struct monst *mtmp;
{
    return (mtmp->mtame && humanoid(youmonst.data)
            && !verysmall(youmonst.data) && !bigmonst(youmonst.data)
            && (!Underwater || is_swimmer(mtmp->data)));
}

int
doride()
{
    boolean forcemount = FALSE;

    if (u.usteed) {
        dismount_steed(DISMOUNT_BYCHOICE);
    } else if (getdir((char *) 0) && isok(u.ux + u.dx, u.uy + u.dy)) {
        if (wizard && yn("直接成功骑上?") == 'y')
            forcemount = TRUE;
        return (mount_steed(m_at(u.ux + u.dx, u.uy + u.dy), forcemount));
    } else {
        return 0;
    }
    return 1;
}

/* Start riding, with the given monster */
boolean
mount_steed(mtmp, force)
struct monst *mtmp; /* The animal */
boolean force;      /* Quietly force this animal */
{
    struct obj *otmp;
    char buf[BUFSZ];
    struct permonst *ptr;

    /* Sanity checks */
    if (u.usteed) {
        You("已经骑着%s.", mon_nam(u.usteed));
        return (FALSE);
    }

    /* Is the player in the right form? */
    if (Hallucination && !force) {
        pline("也许你应该找一个代驾司机.");
        return (FALSE);
    }
    /* While riding Wounded_legs refers to the steed's,
     * not the hero's legs.
     * That opens up a potential abuse where the player
     * can mount a steed, then dismount immediately to
     * heal leg damage, because leg damage is always
     * healed upon dismount (Wounded_legs context switch).
     * By preventing a hero with Wounded_legs from
     * mounting a steed, the potential for abuse is
     * reduced.  However, dismounting still immediately
     * heals the steed's wounded legs.  [In 3.4.3 and
     * earlier, that unintentionally made the hero's
     * temporary 1 point Dex loss become permanent.]
     */
    if (Wounded_legs) {
        Your("%s 还不能乘骑.", makeplural(body_part(LEG)));
        if (force && wizard && yn("治愈你的腿?") == 'y')
            HWounded_legs = EWounded_legs = 0;
        else
            return (FALSE);
    }

    if (Upolyd && (!humanoid(youmonst.data) || verysmall(youmonst.data)
                   || bigmonst(youmonst.data) || slithy(youmonst.data))) {
        You("不适合马鞍.");
        return (FALSE);
    }
    if (!force && (near_capacity() > SLT_ENCUMBER)) {
        You_cant("在携带如此多的东西时做那个.");
        return (FALSE);
    }

    /* Can the player reach and see the monster? */
    if (!mtmp || (!force && ((Blind && !Blind_telepat) || mtmp->mundetected
                             || mtmp->m_ap_type == M_AP_FURNITURE
                             || mtmp->m_ap_type == M_AP_OBJECT))) {
        pline("那儿谁也没有.");
        return (FALSE);
    }
    if (u.uswallow || u.ustuck || u.utrap || Punished
        || !test_move(u.ux, u.uy, mtmp->mx - u.ux, mtmp->my - u.uy,
                      TEST_MOVE)) {
        if (Punished || !(u.uswallow || u.ustuck || u.utrap))
            You("不能够摆动你的%s.", body_part(LEG));
        else
            You("暂时被困在这儿.");
        return (FALSE);
    }

    /* Is this a valid monster? */
    otmp = which_armor(mtmp, W_SADDLE);
    if (!otmp) {
        pline("%s 没有装鞍.", Monnam(mtmp));
        return (FALSE);
    }
    ptr = mtmp->data;
    if (touch_petrifies(ptr) && !Stone_resistance) {
        char kbuf[BUFSZ];

        You("触摸%s.", mon_nam(mtmp));
        Sprintf(kbuf, "试图骑上%s", an(mtmp->data->mname));
        instapetrify(kbuf);
    }
    if (!mtmp->mtame || mtmp->isminion) {
        pline("我认为%s 会介意.", mon_nam(mtmp));
        return (FALSE);
    }
    if (mtmp->mtrapped) {
        struct trap *t = t_at(mtmp->mx, mtmp->my);

        You_cant("骑上%s 在%s陷在%s 的时候.", mon_nam(mtmp),
                 mhe(mtmp), an(defsyms[trap_to_defsym(t->ttyp)].explanation));
        return (FALSE);
    }

    if (!force && !Role_if(PM_KNIGHT) && !(--mtmp->mtame)) {
        /* no longer tame */
        newsym(mtmp->mx, mtmp->my);
        pline("%s抵抗%s!", Monnam(mtmp),
              mtmp->mleashed ? " 并且它的链子脱落了" : "");
        if (mtmp->mleashed)
            m_unleash(mtmp, FALSE);
        return (FALSE);
    }
    if (!force && Underwater && !is_swimmer(ptr)) {
        You_cant("在水下骑那样的生物.");
        return (FALSE);
    }
    if (!can_saddle(mtmp) || !can_ride(mtmp)) {
        You_cant("骑这样的生物.");
        return (0);
    }

    /* Is the player impaired? */
    if (!force && !is_floater(ptr) && !is_flyer(ptr) && Levitation
        && !Lev_at_will) {
        You("不能够到%s.", mon_nam(mtmp));
        return (FALSE);
    }
    if (!force && uarm && is_metallic(uarm) && greatest_erosion(uarm)) {
        Your("%s盔甲太硬了来骑上%s.",
             uarm->oeroded ? "生锈的" : "腐蚀的", mon_nam(mtmp));
        return (FALSE);
    }
    if (!force
        && (Confusion || Fumbling || Glib || Wounded_legs || otmp->cursed
            || (u.ulevel + mtmp->mtame < rnd(MAXULEV / 2 + 5)))) {
        if (Levitation) {
            pline("%s 跑开了.", Monnam(mtmp));
            return FALSE;
        }
        You("在试图骑上%s 时滑了下来.", mon_nam(mtmp));

        Sprintf(buf, "在骑上%s时滑倒",
                /* "a saddled mumak" or "a saddled pony called Dobbin" */
                x_monnam(mtmp, ARTICLE_A, (char *) 0,
                         SUPPRESS_IT | SUPPRESS_INVISIBLE
                             | SUPPRESS_HALLUCINATION,
                         TRUE));
        losehp(Maybe_Half_Phys(rn1(5, 10)), buf, NO_KILLER_PREFIX);
        return (FALSE);
    }

    /* Success */
    maybewakesteed(mtmp);
    if (!force) {
        if (Levitation && !is_floater(ptr) && !is_flyer(ptr))
            /* Must have Lev_at_will at this point */
            pline("%s 如魔法般地飘浮起来!", Monnam(mtmp));
        You("骑上 %s.", mon_nam(mtmp));
    }
    /* setuwep handles polearms differently when you're mounted */
    if (uwep && is_pole(uwep))
        unweapon = FALSE;
    u.usteed = mtmp;
    remove_monster(mtmp->mx, mtmp->my);
    teleds(mtmp->mx, mtmp->my, TRUE);
    return (TRUE);
}

/* You and your steed have moved */
void
exercise_steed()
{
    if (!u.usteed)
        return;

    /* It takes many turns of riding to exercise skill */
    if (u.urideturns++ >= 100) {
        u.urideturns = 0;
        use_skill(P_RIDING, 1);
    }
    return;
}

/* The player kicks or whips the steed */
void
kick_steed()
{
    char He[4];
    if (!u.usteed)
        return;

    /* [ALI] Various effects of kicking sleeping/paralyzed steeds */
    if (u.usteed->msleeping || !u.usteed->mcanmove) {
        /* We assume a message has just been output of the form
         * "You kick <steed>."
         */
        Strcpy(He, mhe(u.usteed));
        *He = highc(*He);
        if ((u.usteed->mcanmove || u.usteed->mfrozen) && !rn2(2)) {
            if (u.usteed->mcanmove)
                u.usteed->msleeping = 0;
            else if (u.usteed->mfrozen > 2)
                u.usteed->mfrozen -= 2;
            else {
                u.usteed->mfrozen = 0;
                u.usteed->mcanmove = 1;
            }
            if (u.usteed->msleeping || !u.usteed->mcanmove)
                pline("%s stirs.", He);
            else
                pline("%s rouses %sself!", He, mhim(u.usteed));
        } else
            pline("%s does not respond.", He);
        return;
    }

    /* Make the steed less tame and check if it resists */
    if (u.usteed->mtame)
        u.usteed->mtame--;
    if (!u.usteed->mtame && u.usteed->mleashed)
        m_unleash(u.usteed, TRUE);
    if (!u.usteed->mtame
        || (u.ulevel + u.usteed->mtame < rnd(MAXULEV / 2 + 5))) {
        newsym(u.usteed->mx, u.usteed->my);
        dismount_steed(DISMOUNT_THROWN);
        return;
    }

    pline("%s gallops!", Monnam(u.usteed));
    u.ugallop += rn1(20, 30);
    return;
}

/*
 * Try to find a dismount point adjacent to the steed's location.
 * If all else fails, try enexto().  Use enexto() as a last resort because
 * enexto() chooses its point randomly, possibly even outside the
 * room's walls, which is not what we want.
 * Adapted from mail daemon code.
 */
STATIC_OVL boolean
landing_spot(spot, reason, forceit)
coord *spot; /* landing position (we fill it in) */
int reason;
int forceit;
{
    int i = 0, x, y, distance, min_distance = -1;
    boolean found = FALSE;
    struct trap *t;

    /* avoid known traps (i == 0) and boulders, but allow them as a backup */
    if (reason != DISMOUNT_BYCHOICE || Stunned || Confusion || Fumbling)
        i = 1;
    for (; !found && i < 2; ++i) {
        for (x = u.ux - 1; x <= u.ux + 1; x++)
            for (y = u.uy - 1; y <= u.uy + 1; y++) {
                if (!isok(x, y) || (x == u.ux && y == u.uy))
                    continue;

                if (accessible(x, y) && !MON_AT(x, y)) {
                    distance = distu(x, y);
                    if (min_distance < 0 || distance < min_distance
                        || (distance == min_distance && rn2(2))) {
                        if (i > 0 || (((t = t_at(x, y)) == 0 || !t->tseen)
                                      && (!sobj_at(BOULDER, x, y)
                                          || throws_rocks(youmonst.data)))) {
                            spot->x = x;
                            spot->y = y;
                            min_distance = distance;
                            found = TRUE;
                        }
                    }
                }
            }
    }

    /* If we didn't find a good spot and forceit is on, try enexto(). */
    if (forceit && min_distance < 0
        && !enexto(spot, u.ux, u.uy, youmonst.data))
        return FALSE;

    return found;
}

/* Stop riding the current steed */
void
dismount_steed(reason)
int reason; /* Player was thrown off etc. */
{
    struct monst *mtmp;
    struct obj *otmp;
    coord cc;
    const char *verb = "落";
    boolean repair_leg_damage = (Wounded_legs != 0L);
    unsigned save_utrap = u.utrap;
    boolean have_spot = landing_spot(&cc, reason, 0);

    mtmp = u.usteed; /* make a copy of steed pointer */
    /* Sanity check */
    if (!mtmp) /* Just return silently */
        return;

    /* Check the reason for dismounting */
    otmp = which_armor(mtmp, W_SADDLE);
    switch (reason) {
    case DISMOUNT_THROWN:
        verb = "被扔";
    case DISMOUNT_FELL:
        You("从%s上%s下!", mon_nam(mtmp), verb);
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
        losehp(Maybe_Half_Phys(rn1(10, 10)), "乘骑事故", KILLED_BY_AN);
        set_wounded_legs(BOTH_SIDES, (int) HWounded_legs + rn1(5, 5));
        repair_leg_damage = FALSE;
        break;
    case DISMOUNT_POLY:
        You("不能再骑着%s.", mon_nam(u.usteed));
        if (!have_spot)
            have_spot = landing_spot(&cc, reason, 1);
        break;
    case DISMOUNT_ENGULFED:
        /* caller displays message */
        break;
    case DISMOUNT_BONES:
        /* hero has just died... */
        break;
    case DISMOUNT_GENERIC:
        /* no messages, just make it so */
        break;
    case DISMOUNT_BYCHOICE:
    default:
        if (otmp && otmp->cursed) {
            You("没办法.  这个鞍%s被诅咒的.",
                otmp->bknown ? "是" : "似乎是");
            otmp->bknown = TRUE;
            return;
        }
        if (!have_spot) {
            You("没办法. 没有任何地方能让你站起来.");
            return;
        }
        if (!has_mname(mtmp)) {
            pline("你度过了地牢中的没有名字的%s .",
                  mtmp->data->mname);
            if (Hallucination)
                pline("从雨中走出来感觉很好.");
        } else
            You("dismount %s.", mon_nam(mtmp));
    }
    /* While riding, Wounded_legs refers to the steed's legs;
       after dismounting, it reverts to the hero's legs. */
    if (repair_leg_damage) {
        /* [TODO: make heal_legs() take a parameter to handle this] */
        in_steed_dismounting = TRUE;
        heal_legs();
        in_steed_dismounting = FALSE;
    }

    /* Release the steed and saddle */
    u.usteed = 0;
    u.ugallop = 0L;

    /* Set player and steed's position.  Try moving the player first
       unless we're in the midst of creating a bones file. */
    if (reason == DISMOUNT_BONES) {
        /* move the steed to an adjacent square */
        if (enexto(&cc, u.ux, u.uy, mtmp->data))
            rloc_to(mtmp, cc.x, cc.y);
        else /* evidently no room nearby; move steed elsewhere */
            (void) rloc(mtmp, FALSE);
        return;
    }
    if (mtmp->mhp > 0) {
        place_monster(mtmp, u.ux, u.uy);
        if (!u.uswallow && !u.ustuck && have_spot) {
            struct permonst *mdat = mtmp->data;

            /* The steed may drop into water/lava */
            if (!is_flyer(mdat) && !is_floater(mdat) && !is_clinger(mdat)) {
                if (is_pool(u.ux, u.uy)) {
                    if (!Underwater)
                        pline("%s 落进%s!", Monnam(mtmp),
                              surface(u.ux, u.uy));
                    if (!is_swimmer(mdat) && !amphibious(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                } else if (is_lava(u.ux, u.uy)) {
                    pline("%s 被拉进熔岩!", Monnam(mtmp));
                    if (!likes_lava(mdat)) {
                        killed(mtmp);
                        adjalign(-1);
                    }
                }
            }
            /* Steed dismounting consists of two steps: being moved to another
             * square, and descending to the floor.  We have functions to do
             * each of these activities, but they're normally called
             * individually and include an attempt to look at or pick up the
             * objects on the floor:
             * teleds() --> spoteffects() --> pickup()
             * float_down() --> pickup()
             * We use this kludge to make sure there is only one such attempt.
             *
             * Clearly this is not the best way to do it.  A full fix would
             * involve having these functions not call pickup() at all,
             * instead
             * calling them first and calling pickup() afterwards.  But it
             * would take a lot of work to keep this change from having any
             * unforeseen side effects (for instance, you would no longer be
             * able to walk onto a square with a hole, and autopickup before
             * falling into the hole).
             */
            /* [ALI] No need to move the player if the steed died. */
            if (mtmp->mhp > 0) {
                /* Keep steed here, move the player to cc;
                 * teleds() clears u.utrap
                 */
                in_steed_dismounting = TRUE;
                teleds(cc.x, cc.y, TRUE);
                in_steed_dismounting = FALSE;

                /* Put your steed in your trap */
                if (save_utrap)
                    (void) mintrap(mtmp);
            }
            /* Couldn't... try placing the steed */
        } else if (enexto(&cc, u.ux, u.uy, mtmp->data)) {
            /* Keep player here, move the steed to cc */
            rloc_to(mtmp, cc.x, cc.y);
            /* Player stays put */
            /* Otherwise, kill the steed */
        } else {
            killed(mtmp);
            adjalign(-1);
        }
    }

    /* Return the player to the floor */
    if (reason != DISMOUNT_ENGULFED) {
        in_steed_dismounting = TRUE;
        (void) float_down(0L, W_SADDLE);
        in_steed_dismounting = FALSE;
        context.botl = 1;
        (void) encumber_msg();
        vision_full_recalc = 1;
    } else
        context.botl = 1;
    /* polearms behave differently when not mounted */
    if (uwep && is_pole(uwep))
        unweapon = TRUE;
    return;
}

/* when attempting to saddle or mount a sleeping steed, try to wake it up
   (for the saddling case, it won't be u.usteed yet) */
STATIC_OVL void
maybewakesteed(steed)
struct monst *steed;
{
    int frozen = (int) steed->mfrozen;
    boolean wasimmobile = steed->msleeping || !steed->mcanmove;

    steed->msleeping = 0;
    if (frozen) {
        frozen = (frozen + 1) / 2; /* half */
        /* might break out of timed sleep or paralysis */
        if (!rn2(frozen)) {
            steed->mfrozen = 0;
            steed->mcanmove = 1;
        } else {
            /* didn't awake, but remaining duration is halved */
            steed->mfrozen = frozen;
        }
    }
    if (wasimmobile && !steed->msleeping && steed->mcanmove)
        pline("%s wakes up.", Monnam(steed));
    /* regardless of waking, terminate any meal in progress */
    finish_meating(steed);
}

/* decide whether hero's steed is able to move;
   doesn't check for holding traps--those affect the hero directly */
boolean
stucksteed(checkfeeding)
boolean checkfeeding;
{
    struct monst *steed = u.usteed;

    if (steed) {
        /* check whether steed can move */
        if (steed->msleeping || !steed->mcanmove) {
            pline("%s won't move!", upstart(y_monnam(steed)));
            return TRUE;
        }
        /* optionally check whether steed is in the midst of a meal */
        if (checkfeeding && steed->meating) {
            pline("%s is still eating.", upstart(y_monnam(steed)));
            return TRUE;
        }
    }
    return FALSE;
}

void
place_monster(mon, x, y)
struct monst *mon;
int x, y;
{
    if (mon == u.usteed
        /* special case is for convoluted vault guard handling */
        || (DEADMONSTER(mon) && !(mon->isgd && x == 0 && y == 0))) {
        impossible("placing %s onto map?",
                   (mon == u.usteed) ? "steed" : "defunct monster");
        return;
    }
    mon->mx = x, mon->my = y;
    level.monsters[x][y] = mon;
}

/*steed.c*/
