/* NetHack 3.6	fountain.c	$NHDT-Date: 1444937416 2015/10/15 19:30:16 $  $NHDT-Branch: master $:$NHDT-Revision: 1.55 $ */
/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

STATIC_DCL void NDECL(dowatersnakes);
STATIC_DCL void NDECL(dowaterdemon);
STATIC_DCL void NDECL(dowaternymph);
STATIC_PTR void FDECL(gush, (int, int, genericptr_t));
STATIC_DCL void NDECL(dofindgem);

/* used when trying to dip in or drink from fountain or sink or pool while
   levitating above it, or when trying to move downwards in that state */
void
floating_above(what)
const char *what;
{
    const char *umsg = "很高地飘浮在%s上.";

    if (u.utrap && (u.utraptype == TT_INFLOOR || u.utraptype == TT_LAVA)) {
        /* when stuck in floor (not possible at fountain or sink location,
           so must be attempting to move down), override the usual message */
        umsg = "被困在%s里.";
        what = surface(u.ux, u.uy); /* probably redundant */
    }
    You(umsg, what);
}

/* Fountain of snakes! */
STATIC_OVL void
dowatersnakes()
{
    register int num = rn1(5, 2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
        if (!Blind)
            pline("络绎不绝的%s涌流出来!",
                  Hallucination ? makeplural(rndmonnam(NULL)) : "蛇");
        else
            You_hear("%s 发出嘶嘶声!", something);
        while (num-- > 0)
            if ((mtmp = makemon(&mons[PM_WATER_MOCCASIN], u.ux, u.uy,
                                NO_MM_FLAGS)) != 0
                && t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
    } else
        pline_The("喷泉片刻剧烈地冒泡, 然后平静了.");
}

/* Water demon */
STATIC_OVL void
dowaterdemon()
{
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
        if ((mtmp = makemon(&mons[PM_WATER_DEMON], u.ux, u.uy,
                            NO_MM_FLAGS)) != 0) {
            if (!Blind)
                You("解放了%s!", a_monnam(mtmp));
            else
                You_feel("到邪恶的存在.");

            /* Give those on low levels a (slightly) better chance of survival
             */
            if (rnd(100) > (80 + level_difficulty())) {
                pline("感激%s解放, %s满足你一个愿望!",
                      mhis(mtmp), mhe(mtmp));
                /* give a wish and discard the monster (mtmp set to null) */
                mongrantswish(&mtmp);
            } else if (t_at(mtmp->mx, mtmp->my))
                (void) mintrap(mtmp);
        }
    } else
        pline_The("喷泉片刻剧烈地冒泡, 然后平静了.");
}

/* Water Nymph */
STATIC_OVL void
dowaternymph()
{
    register struct monst *mtmp;

    if (!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE)
        && (mtmp = makemon(&mons[PM_WATER_NYMPH], u.ux, u.uy,
                           NO_MM_FLAGS)) != 0) {
        if (!Blind)
            You("吸引了%s!", a_monnam(mtmp));
        else
            You_hear("一个诱人的声音.");
        mtmp->msleeping = 0;
        if (t_at(mtmp->mx, mtmp->my))
            (void) mintrap(mtmp);
    } else if (!Blind)
        pline("一个大气泡上升到了表面然后破了.");
    else
        You_hear("啪的一声.");
}

/* Gushing forth along LOS from (u.ux, u.uy) */
void
dogushforth(drinking)
int drinking;
{
    int madepool = 0;

    do_clear_area(u.ux, u.uy, 7, gush, (genericptr_t) &madepool);
    if (!madepool) {
        if (drinking)
            Your("口渴缓和了.");
        else
            pline("水洒到你全身.");
    }
}

STATIC_PTR void
gush(x, y, poolcnt)
int x, y;
genericptr_t poolcnt;
{
    register struct monst *mtmp;
    register struct trap *ttmp;

    if (((x + y) % 2) || (x == u.ux && y == u.uy)
        || (rn2(1 + distmin(u.ux, u.uy, x, y))) || (levl[x][y].typ != ROOM)
        || (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
        return;

    if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
        return;

    if (!((*(int *) poolcnt)++))
        pline("水从满溢的喷泉里喷出!");

    /* Put a pool at x, y */
    levl[x][y].typ = POOL;
    /* No kelp! */
    del_engr_at(x, y);
    water_damage_chain(level.objects[x][y], TRUE);

    if ((mtmp = m_at(x, y)) != 0)
        (void) minliquid(mtmp);
    else
        newsym(x, y);
}

/* Find a gem in the sparkling waters. */
STATIC_OVL void
dofindgem()
{
    if (!Blind)
        You("在气泡的水中发现了一颗宝石!");
    else
        You_feel("这里有一颗宝石!");
    (void) mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1), u.ux, u.uy,
                     FALSE, FALSE);
    SET_FOUNTAIN_LOOTED(u.ux, u.uy);
    newsym(u.ux, u.uy);
    exercise(A_WIS, TRUE); /* a discovery! */
}

void
dryup(x, y, isyou)
xchar x, y;
boolean isyou;
{
    if (IS_FOUNTAIN(levl[x][y].typ)
        && (!rn2(3) || FOUNTAIN_IS_WARNED(x, y))) {
        if (isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x, y)) {
            struct monst *mtmp;

            SET_FOUNTAIN_WARNED(x, y);
            /* Warn about future fountain use. */
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                if (is_watch(mtmp->data) && couldsee(mtmp->mx, mtmp->my)
                    && mtmp->mpeaceful) {
                    pline("%s 叫:", Amonnam(mtmp));
                    verbalize("喂, 停止使用那个喷泉!");
                    break;
                }
            }
            /* You can see or hear this effect */
            if (!mtmp)
                pline_The("涌流减少为细流.");
            return;
        }
        if (isyou && wizard) {
            if (yn("让喷泉干涸?") == 'n')
                return;
        }
        /* replace the fountain with ordinary floor */
        levl[x][y].typ = ROOM;
        levl[x][y].looted = 0;
        levl[x][y].blessedftn = 0;
        if (cansee(x, y))
            pline_The("喷泉干涸了!");
        /* The location is seen if the hero/monster is invisible
           or felt if the hero is blind. */
        newsym(x, y);
        level.flags.nfountains--;
        if (isyou && in_town(x, y))
            (void) angry_guards(FALSE);
    }
}

void
drinkfountain()
{
    /* What happens when you drink from a fountain? */
    register boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
    register int fate = rnd(30);

    if (Levitation) {
        floating_above("喷泉");
        return;
    }

    if (mgkftn && u.uluck >= 0 && fate >= 10) {
        int i, ii, littleluck = (u.uluck < 4);

        pline("哇!  这让你感觉很棒!");
        /* blessed restore ability */
        for (ii = 0; ii < A_MAX; ii++)
            if (ABASE(ii) < AMAX(ii)) {
                ABASE(ii) = AMAX(ii);
                context.botl = 1;
            }
        /* gain ability, blessed if "natural" luck is high */
        i = rn2(A_MAX); /* start at a random attribute */
        for (ii = 0; ii < A_MAX; ii++) {
            if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
                break;
            if (++i >= A_MAX)
                i = 0;
        }
        display_nhwindow(WIN_MESSAGE, FALSE);
        pline("一缕蒸汽从喷泉中逸出...");
        exercise(A_WIS, TRUE);
        levl[u.ux][u.uy].blessedftn = 0;
        return;
    }

    if (fate < 10) {
        pline_The("凉爽的风让你神清气爽.");
        u.uhunger += rnd(10); /* don't choke on water */
        newuhs(FALSE);
        if (mgkftn)
            return;
    } else {
        switch (fate) {
        case 19: /* Self-knowledge */
            You_feel("自知的...");
            display_nhwindow(WIN_MESSAGE, FALSE);
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            exercise(A_WIS, TRUE);
            pline_The("感觉消退了.");
            break;
        case 20: /* Foul water */
            pline_The("水很脏!  你作呕并呕吐.");
            morehungry(rn1(20, 11));
            vomit();
            break;
        case 21: /* Poisonous */
            pline_The("水被污染了!");
            if (Poison_resistance) {
                pline("也许它是从附近的%s农场流过来的.",
                      fruitname(FALSE));
                losehp(rnd(4), "未冷藏的一小口果汁", KILLED_BY_AN);
                break;
            }
            losestr(rn1(4, 3));
            losehp(rnd(10), "受污染的水", KILLED_BY);
            exercise(A_CON, FALSE);
            break;
        case 22: /* Fountain of snakes! */
            dowatersnakes();
            break;
        case 23: /* Water demon */
            dowaterdemon();
            break;
        case 24: /* Curse an item */ {
            register struct obj *obj;

            pline("这个水不好!");
            morehungry(rn1(20, 11));
            exercise(A_CON, FALSE);
            for (obj = invent; obj; obj = obj->nobj)
                if (!rn2(5))
                    curse(obj);
            break;
        }
        case 25: /* See invisible */
            if (Blind) {
                if (Invisible) {
                    You("感觉透明的.");
                } else {
                    You("感觉非常自我意识的.");
                    pline("然后消失了.");
                }
            } else {
                You_see("某人的影像在偷偷接近你.");
                pline("但它消失了.");
            }
            HSee_invisible |= FROMOUTSIDE;
            newsym(u.ux, u.uy);
            exercise(A_WIS, TRUE);
            break;
        case 26: /* See Monsters */
            (void) monster_detect((struct obj *) 0, 0);
            exercise(A_WIS, TRUE);
            break;
        case 27: /* Find a gem in the sparkling waters. */
            if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
                dofindgem();
                break;
            }
        case 28: /* Water Nymph */
            dowaternymph();
            break;
        case 29: /* Scare */
        {
            register struct monst *mtmp;

            pline("这个水让你口臭!");
            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
                if (DEADMONSTER(mtmp))
                    continue;
                monflee(mtmp, 0, FALSE, FALSE);
            }
            break;
        }
        case 30: /* Gushing forth in this room */
            dogushforth(TRUE);
            break;
        default:
            pline("这个温水是无味的.");
            break;
        }
    }
    dryup(u.ux, u.uy, TRUE);
}

void
dipfountain(obj)
register struct obj *obj;
{
    if (Levitation) {
        floating_above("喷泉");
        return;
    }

    /* Don't grant Excalibur when there's more than one object.  */
    /* (quantity could be > 1 if merged daggers got polymorphed) */
    if (obj->otyp == LONG_SWORD && obj->quan == 1L && u.ulevel >= 5 && !rn2(6)
        && !obj->oartifact
        && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {
        if (u.ualign.type != A_LAWFUL) {
            /* Ha!  Trying to cheat her. */
            pline(
             "冰冷的薄雾从水中升起并包裹住了剑.");
            pline_The("喷泉消失了!");
            curse(obj);
            if (obj->spe > -6 && !rn2(3))
                obj->spe--;
            obj->oerodeproof = FALSE;
            exercise(A_WIS, FALSE);
        } else {
            /* The lady of the lake acts! - Eric Backus */
            /* Be *REAL* nice */
            pline(
              "从黑暗深处, 一只手伸上来祝福那把剑.");
            pline("当手撤离的时候, 喷泉消失了!");
            obj = oname(obj, artiname(ART_EXCALIBUR));
            discover_artifact(ART_EXCALIBUR);
            bless(obj);
            obj->oeroded = obj->oeroded2 = 0;
            obj->oerodeproof = TRUE;
            exercise(A_WIS, TRUE);
        }
        update_inventory();
        levl[u.ux][u.uy].typ = ROOM;
        levl[u.ux][u.uy].looted = 0;
        newsym(u.ux, u.uy);
        level.flags.nfountains--;
        if (in_town(u.ux, u.uy))
            (void) angry_guards(FALSE);
        return;
    } else {
        int er = water_damage(obj, NULL, TRUE);

        if (obj->otyp == POT_ACID
            && er != ER_DESTROYED) { /* Acid and water don't mix */
            useup(obj);
            return;
        } else if (er != ER_NOTHING && !rn2(2)) { /* no further effect */
            return;
        }
    }

    switch (rnd(30)) {
    case 16: /* Curse the item */
        curse(obj);
        break;
    case 17:
    case 18:
    case 19:
    case 20: /* Uncurse the item */
        if (obj->cursed) {
            if (!Blind)
                pline_The("水发光了片刻.");
            uncurse(obj);
        } else {
            pline("一种失落感来袭.");
        }
        break;
    case 21: /* Water Demon */
        dowaterdemon();
        break;
    case 22: /* Water Nymph */
        dowaternymph();
        break;
    case 23: /* an Endless Stream of Snakes */
        dowatersnakes();
        break;
    case 24: /* Find a gem */
        if (!FOUNTAIN_IS_LOOTED(u.ux, u.uy)) {
            dofindgem();
            break;
        }
    case 25: /* Water gushes forth */
        dogushforth(FALSE);
        break;
    case 26: /* Strange feeling */
        pline("一种奇怪的刺痛感出现在你的%s上.", body_part(ARM));
        break;
    case 27: /* Strange feeling */
        You_feel("突然的寒意.");
        break;
    case 28: /* Strange feeling */
        pline("想要洗澡的冲动淹没了你.");
        {
            long money = money_cnt(invent);
            struct obj *otmp;
            if (money > 10) {
                /* Amount to lose.  Might get rounded up as fountains don't
                 * pay change... */
                money = somegold(money) / 10;
                for (otmp = invent; otmp && money > 0; otmp = otmp->nobj)
                    if (otmp->oclass == COIN_CLASS) {
                        int denomination = objects[otmp->otyp].oc_cost;
                        long coin_loss =
                            (money + denomination - 1) / denomination;
                        coin_loss = min(coin_loss, otmp->quan);
                        otmp->quan -= coin_loss;
                        money -= coin_loss * denomination;
                        if (!otmp->quan)
                            delobj(otmp);
                    }
                You("在喷泉中丢失了一些钱!");
                CLEAR_FOUNTAIN_LOOTED(u.ux, u.uy);
                exercise(A_WIS, FALSE);
            }
        }
        break;
    case 29: /* You see coins */
        /* We make fountains have more coins the closer you are to the
         * surface.  After all, there will have been more people going
         * by.	Just like a shopping mall!  Chris Woodbury  */

        if (FOUNTAIN_IS_LOOTED(u.ux, u.uy))
            break;
        SET_FOUNTAIN_LOOTED(u.ux, u.uy);
        (void) mkgold((long) (rnd((dunlevs_in_dungeon(&u.uz) - dunlev(&u.uz)
                                   + 1) * 2) + 5),
                      u.ux, u.uy);
        if (!Blind)
            pline("下面远处, 你看见金币在水里闪闪发光.");
        exercise(A_WIS, TRUE);
        newsym(u.ux, u.uy);
        break;
    }
    update_inventory();
    dryup(u.ux, u.uy, TRUE);
}

void
breaksink(x, y)
int x, y;
{
    if (cansee(x, y) || (x == u.ux && y == u.uy))
        pline_The("水管破裂!  水喷出来了!");
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x, y);
}

void
drinksink()
{
    struct obj *otmp;
    struct monst *mtmp;

    if (Levitation) {
        floating_above("水槽");
        return;
    }
    switch (rn2(20)) {
    case 0:
        You("喝了一小口非常冷的水.");
        break;
    case 1:
        You("喝了一小口非常温暖的水.");
        break;
    case 2:
        You("喝了一小口滚烫的热水.");
        if (Fire_resistance)
            pline("似乎相当可口.");
        else
            losehp(rnd(6), "喝沸腾的水", KILLED_BY);
        /* boiling water burns considered fire damage */
        break;
    case 3:
        if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
            pline_The("水槽似乎相当肮脏.");
        else {
            mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy, NO_MM_FLAGS);
            if (mtmp)
                pline("呀!  那里有%s在水槽里!",
                      (Blind || !canspotmon(mtmp)) ? "蠕动的什么东西"
                                                   : a_monnam(mtmp));
        }
        break;
    case 4:
        do {
            otmp = mkobj(POTION_CLASS, FALSE);
            if (otmp->otyp == POT_WATER) {
                obfree(otmp, (struct obj *) 0);
                otmp = (struct obj *) 0;
            }
        } while (!otmp);
        otmp->cursed = otmp->blessed = 0;
        pline("一些%s 液体从水龙头流出.",
              Blind ? "古怪的" : hcolor(OBJ_DESCR(objects[otmp->otyp])));
        otmp->dknown = !(Blind || Hallucination);
        otmp->quan++;       /* Avoid panic upon useup() */
        otmp->fromsink = 1; /* kludge for docall() */
        (void) dopotion(otmp);
        obfree(otmp, (struct obj *) 0);
        break;
    case 5:
        if (!(levl[u.ux][u.uy].looted & S_LRING)) {
            You("在水槽里找到一枚戒指!");
            (void) mkobj_at(RING_CLASS, u.ux, u.uy, TRUE);
            levl[u.ux][u.uy].looted |= S_LRING;
            exercise(A_WIS, TRUE);
            newsym(u.ux, u.uy);
        } else
            pline("一些肮脏的水淤积到下水道中.");
        break;
    case 6:
        breaksink(u.ux, u.uy);
        break;
    case 7:
        pline_The("水移动得就像它自己的意愿!");
        if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
            || !makemon(&mons[PM_WATER_ELEMENTAL], u.ux, u.uy, NO_MM_FLAGS))
            pline("但它安静下来.");
        break;
    case 8:
        pline("恶心, 这水味道很糟.");
        more_experienced(1, 0);
        newexplevel();
        break;
    case 9:
        pline("呕...  这尝起来像污水!  你呕吐了.");
        morehungry(rn1(30 - ACURR(A_CON), 11));
        vomit();
        break;
    case 10:
        pline("这水含有有毒废物!");
        if (!Unchanging) {
            You("经受奇特的变形!");
            polyself(0);
        }
        break;
    /* more odd messages --JJB */
    case 11:
        You_hear("来自管道的叮当声...");
        break;
    case 12:
        You_hear("下水道里的片段歌声...");
        break;
    case 19:
        if (Hallucination) {
            pline("从阴暗的下水道, 一只手伸出来... -- 哎哟--");
            break;
        }
    default:
        You("喝了一小口%s水.",
            rn2(3) ? (rn2(2) ? "冷" : "温") : "热");
    }
}

/*fountain.c*/
