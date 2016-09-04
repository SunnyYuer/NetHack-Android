/* NetHack 3.6	potion.c	$NHDT-Date: 1446861768 2015/11/07 02:02:48 $  $NHDT-Branch: master $:$NHDT-Revision: 1.121 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

boolean notonhead = FALSE;

static NEARDATA int nothing, unkn;
static NEARDATA const char beverages[] = { POTION_CLASS, 0 };

STATIC_DCL long FDECL(itimeout, (long));
STATIC_DCL long FDECL(itimeout_incr, (long, int));
STATIC_DCL void NDECL(ghost_from_bottle);
STATIC_DCL boolean
FDECL(H2Opotion_dip, (struct obj *, struct obj *, BOOLEAN_P, const char *));
STATIC_DCL short FDECL(mixtype, (struct obj *, struct obj *));

/* force `val' to be within valid range for intrinsic timeout value */
STATIC_OVL long
itimeout(val)
long val;
{
    if (val >= TIMEOUT)
        val = TIMEOUT;
    else if (val < 1)
        val = 0;

    return val;
}

/* increment `old' by `incr' and force result to be valid intrinsic timeout */
STATIC_OVL long
itimeout_incr(old, incr)
long old;
int incr;
{
    return itimeout((old & TIMEOUT) + (long) incr);
}

/* set the timeout field of intrinsic `which' */
void
set_itimeout(which, val)
long *which, val;
{
    *which &= ~TIMEOUT;
    *which |= itimeout(val);
}

/* increment the timeout field of intrinsic `which' */
void
incr_itimeout(which, incr)
long *which;
int incr;
{
    set_itimeout(which, itimeout_incr(*which, incr));
}

void
make_confused(xtime, talk)
long xtime;
boolean talk;
{
    long old = HConfusion;

    if (Unaware)
        talk = FALSE;

    if (!xtime && old) {
        if (talk)
            You_feel("现在不那么%s.", Hallucination ? "幻觉的" : "混乱的");
    }
    if ((xtime && !old) || (!xtime && old))
        context.botl = TRUE;

    set_itimeout(&HConfusion, xtime);
}

void
make_stunned(xtime, talk)
long xtime;
boolean talk;
{
    long old = HStun;

    if (Unaware)
        talk = FALSE;

    if (!xtime && old) {
        if (talk)
            You_feel("现在%s.",
                     Hallucination ? "不那么摇晃了" : "有些稳定了");
    }
    if (xtime && !old) {
        if (talk) {
            if (u.usteed)
                You("在在坐骑上摇晃.");
            else
                You("%s...", stagger(youmonst.data, "摇摇晃晃"));
        }
    }
    if ((!xtime && old) || (xtime && !old))
        context.botl = TRUE;

    set_itimeout(&HStun, xtime);
}

void
make_sick(xtime, cause, talk, type)
long xtime;
const char *cause; /* sickness cause */
boolean talk;
int type;
{
    long old = Sick;

#if 0
    if (Unaware)
        talk = FALSE;
#endif
    if (xtime > 0L) {
        if (Sick_resistance)
            return;
        if (!old) {
            /* newly sick */
            You_feel("致命的生病.");
        } else {
            /* already sick */
            if (talk)
                You_feel("%s糟了.", xtime <= Sick / 2L ? "非常" : "更加");
        }
        set_itimeout(&Sick, xtime);
        u.usick_type |= type;
        context.botl = TRUE;
    } else if (old && (type & u.usick_type)) {
        /* was sick, now not */
        u.usick_type &= ~type;
        if (u.usick_type) { /* only partly cured */
            if (talk)
                You_feel("稍微好些了.");
            set_itimeout(&Sick, Sick * 2); /* approximation */
        } else {
            if (talk)
                You_feel("痊愈了.  轻松多了!");
            Sick = 0L; /* set_itimeout(&Sick, 0L) */
        }
        context.botl = TRUE;
    }

    if (Sick) {
        exercise(A_CON, FALSE);
        delayed_killer(SICK, KILLED_BY_AN, cause);
    } else
        dealloc_killer(find_delayed_killer(SICK));
}

void
make_slimed(xtime, msg)
long xtime;
const char *msg;
{
    long old = Slimed;

#if 0
    if (Unaware)
        msg = 0;
#endif
    if ((!xtime && old) || (xtime && !old)) {
        if (msg)
            pline1(msg);
        context.botl = 1;
    }
    set_itimeout(&Slimed, xtime);
    if (!Slimed)
        dealloc_killer(find_delayed_killer(SLIMED));
}

/* start or stop petrification */
void
make_stoned(xtime, msg, killedby, killername)
long xtime;
const char *msg;
int killedby;
const char *killername;
{
    long old = Stoned;

#if 0
    if (Unaware)
        msg = 0;
#endif
    if ((!xtime && old) || (xtime && !old)) {
        if (msg)
            pline1(msg);
        /* context.botl = 1;   --- Stoned is not a status line item */
    }
    set_itimeout(&Stoned, xtime);
    if (!Stoned)
        dealloc_killer(find_delayed_killer(STONED));
    else if (!old)
        delayed_killer(STONED, killedby, killername);
}

void
make_vomiting(xtime, talk)
long xtime;
boolean talk;
{
    long old = Vomiting;

    if (Unaware)
        talk = FALSE;

    if (!xtime && old)
        if (talk)
            You_feel("现在完全不那么恶心了.");

    set_itimeout(&Vomiting, xtime);
}

static const char vismsg[] = "视野似乎%s了片刻, 但现在是%s.";
static const char eyemsg[] = "%s暂时的%s.";

void
make_blinded(xtime, talk)
long xtime;
boolean talk;
{
    long old = Blinded;
    boolean u_could_see, can_see_now;
    const char *eyes;

    /* we need to probe ahead in case the Eyes of the Overworld
       are or will be overriding blindness */
    u_could_see = !Blind;
    Blinded = xtime ? 1L : 0L;
    can_see_now = !Blind;
    Blinded = old; /* restore */

    if (Unaware)
        talk = FALSE;

    if (can_see_now && !u_could_see) { /* regaining sight */
        if (talk) {
            if (Hallucination)
                pline("非常奇怪!  一切都再次无边的变化!");
            else
                You("又能看见了.");
        }
    } else if (old && !xtime) {
        /* clearing temporary blindness without toggling blindness */
        if (talk) {
            if (!haseyes(youmonst.data)) {
                strange_feeling((struct obj *) 0, (char *) 0);
            } else if (Blindfolded) {
                eyes = body_part(EYE);
                if (eyecount(youmonst.data) != 1)
                    eyes = makeplural(eyes);
                Your(eyemsg, eyes, vtense(eyes, "发痒"));
            } else { /* Eyes of the Overworld */
                Your(vismsg, "明亮", Hallucination ? "更伤心的" : "正常的");
            }
        }
    }

    if (u_could_see && !can_see_now) { /* losing sight */
        if (talk) {
            if (Hallucination)
                pline("啊, 迷幻!  一切都是黑的!  救命!");
            else
                pline("一片黑暗的乌云落在你身上.");
        }
        /* Before the hero goes blind, set the ball&chain variables. */
        if (Punished)
            set_bc(0);
    } else if (!old && xtime) {
        /* setting temporary blindness without toggling blindness */
        if (talk) {
            if (!haseyes(youmonst.data)) {
                strange_feeling((struct obj *) 0, (char *) 0);
            } else if (Blindfolded) {
                eyes = body_part(EYE);
                if (eyecount(youmonst.data) != 1)
                    eyes = makeplural(eyes);
                Your(eyemsg, eyes, vtense(eyes, "抽动"));
            } else { /* Eyes of the Overworld */
                Your(vismsg, "昏暗", Hallucination ? "更高兴的" : "正常的");
            }
        }
    }

    set_itimeout(&Blinded, xtime);

    if (u_could_see ^ can_see_now) { /* one or the other but not both */
        context.botl = 1;
        vision_full_recalc = 1; /* blindness just got toggled */
        /* this vision recalculation used to be deferred until
           moveloop(), but that made it possible for vision
           irregularities to occur (cited case was force bolt
           hitting adjacent potion of blindness and then a
           secret door; hero was blinded by vapors but then
           got the message "a door appears in the wall") */
        vision_recalc(0);
        if (Blind_telepat || Infravision)
            see_monsters();

        /* avoid either of the sequences
           "Sting starts glowing", [become blind], "Sting stops quivering" or
           "Sting starts quivering", [regain sight], "Sting stops glowing"
           by giving "Sting is quivering" when becoming blind or
           "Sting is glowing" when regaining sight so that the eventual
           "stops" message matches */
        if (warn_obj_cnt && uwep && (EWarn_of_mon & W_WEP) != 0L)
            Sting_effects(-1);
        /* update dknown flag for inventory picked up while blind */
        if (can_see_now)
            learn_unseen_invent();
    }
}

boolean
make_hallucinated(xtime, talk, mask)
long xtime; /* nonzero if this is an attempt to turn on hallucination */
boolean talk;
long mask; /* nonzero if resistance status should change by mask */
{
    long old = HHallucination;
    boolean changed = 0;
    const char *message, *verb;

    if (Unaware)
        talk = FALSE;

    message = (!xtime) ? "现在一切%s都如此的无聊."
                       : "哦哇!  一切%s都是如此无边的变化!";
    verb = (!Blind) ? "看起来" : "感觉";

    if (mask) {
        if (HHallucination)
            changed = TRUE;

        if (!xtime)
            EHalluc_resistance |= mask;
        else
            EHalluc_resistance &= ~mask;
    } else {
        if (!EHalluc_resistance && (!!HHallucination != !!xtime))
            changed = TRUE;
        set_itimeout(&HHallucination, xtime);

        /* clearing temporary hallucination without toggling vision */
        if (!changed && !HHallucination && old && talk) {
            if (!haseyes(youmonst.data)) {
                strange_feeling((struct obj *) 0, (char *) 0);
            } else if (Blind) {
                const char *eyes = body_part(EYE);

                if (eyecount(youmonst.data) != 1)
                    eyes = makeplural(eyes);
                Your(eyemsg, eyes, vtense(eyes, "发痒"));
            } else { /* Grayswandir */
                Your(vismsg, "失去光泽", "正常的");
            }
        }
    }

    if (changed) {
        /* in case we're mimicking an orange (hallucinatory form
           of mimicking gold) update the mimicking's-over message */
        if (!Hallucination)
            eatmupdate();

        if (u.uswallow) {
            swallowed(0); /* redraw swallow display */
        } else {
            /* The see_* routines should be called *before* the pline. */
            see_monsters();
            see_objects();
            see_traps();
        }

        /* for perm_inv and anything similar
        (eg. Qt windowport's equipped items display) */
        update_inventory();

        context.botl = 1;
        if (talk)
            pline(message, verb);
    }
    return changed;
}

void
make_deaf(xtime, talk)
long xtime;
boolean talk;
{
    long old = HDeaf;
    boolean toggled = FALSE;

    if (Unaware)
        talk = FALSE;

    if (!xtime && old) {
        if (talk)
            You("又能听见了.");
        toggled = TRUE;
    } else if (xtime && !old) {
        if (talk)
            You("不能听见任何声音了.");
        toggled = TRUE;
    }
    /* deafness isn't presently shown on status line, but
       request a status update in case that changes someday */
    if (toggled)
        context.botl = TRUE;

    set_itimeout(&HDeaf, xtime);
}

STATIC_OVL void
ghost_from_bottle()
{
    struct monst *mtmp = makemon(&mons[PM_GHOST], u.ux, u.uy, NO_MM_FLAGS);

    if (!mtmp) {
        pline("这个瓶子原来是空的.");
        return;
    }
    if (Blind) {
        pline("当你打开了瓶子, %s浮现出来.", something);
        return;
    }
    pline("当你打开了瓶子, 一个巨大的%s 浮现出来!",
          Hallucination ? rndmonnam(NULL) : (const char *) "鬼魂");
    if (flags.verbose)
        You("害怕得要死, 无法移动了.");
    nomul(-3);
    multi_reason = "害怕得要死";
    nomovemsg = "你重获了你的镇静.";
}

/* "Quaffing is like drinking, except you spill more." - Terry Pratchett */
int
dodrink()
{
    register struct obj *otmp;
    const char *potion_descr;

    if (Strangled) {
        pline("如果你不能呼吸空气, 你如何喝液体?");
        return 0;
    }
    /* Is there a fountain to drink from here? */
    if (IS_FOUNTAIN(levl[u.ux][u.uy].typ)
        /* not as low as floor level but similar restrictions apply */
        && can_reach_floor(FALSE)) {
        if (yn("饮用泉水?") == 'y') {
            drinkfountain();
            return 1;
        }
    }
    /* Or a kitchen sink? */
    if (IS_SINK(levl[u.ux][u.uy].typ)
        /* not as low as floor level but similar restrictions apply */
        && can_reach_floor(FALSE)) {
        if (yn("饮用水槽的水?") == 'y') {
            drinksink();
            return 1;
        }
    }
    /* Or are you surrounded by water? */
    if (Underwater && !u.uswallow) {
        if (yn("喝你周围的水?") == 'y') {
            pline("你知道什么生存在这水里吗?");
            return 1;
        }
    }

    otmp = getobj(beverages, "喝");  //drink
    if (!otmp)
        return 0;

    /* quan > 1 used to be left to useup(), but we need to force
       the current potion to be unworn, and don't want to do
       that for the entire stack when starting with more than 1.
       [Drinking a wielded potion of polymorph can trigger a shape
       change which causes hero's weapon to be dropped.  In 3.4.x,
       that led to an "object lost" panic since subsequent useup()
       was no longer dealing with an inventory item.  Unwearing
       the current potion is intended to keep it in inventory.] */
    if (otmp->quan > 1L) {
        otmp = splitobj(otmp, 1L);
        otmp->owornmask = 0L; /* rest of original stuck unaffected */
    } else if (otmp->owornmask) {
        remove_worn_item(otmp, FALSE);
    }
    otmp->in_use = TRUE; /* you've opened the stopper */

    potion_descr = OBJ_DESCR(objects[otmp->otyp]);
    if (potion_descr) {
        if (!strcmp(potion_descr, "乳白色")
            && !(mvitals[PM_GHOST].mvflags & G_GONE)
            && !rn2(POTION_OCCUPANT_CHANCE(mvitals[PM_GHOST].born))) {
            ghost_from_bottle();
            useup(otmp);
            return 1;
        } else if (!strcmp(potion_descr, "冒烟的")
                   && !(mvitals[PM_DJINNI].mvflags & G_GONE)
                   && !rn2(POTION_OCCUPANT_CHANCE(mvitals[PM_DJINNI].born))) {
            djinni_from_bottle(otmp);
            useup(otmp);
            return 1;
        }
    }
    return dopotion(otmp);
}

int
dopotion(otmp)
register struct obj *otmp;
{
    int retval;

    otmp->in_use = TRUE;
    nothing = unkn = 0;
    if ((retval = peffects(otmp)) >= 0)
        return retval;

    if (nothing) {
        unkn++;
        You("片刻有一种%s感觉, 然后消退了.",
            Hallucination ? "正常的" : "奇特的");
    }
    if (otmp->dknown && !objects[otmp->otyp].oc_name_known) {
        if (!unkn) {
            makeknown(otmp->otyp);
            more_experienced(0, 10);
        } else if (!objects[otmp->otyp].oc_uname)
            docall(otmp);
    }
    useup(otmp);
    return 1;
}

int
peffects(otmp)
register struct obj *otmp;
{
    register int i, ii, lim;

    switch (otmp->otyp) {
    case POT_RESTORE_ABILITY:
    case SPE_RESTORE_ABILITY:
        unkn++;
        if (otmp->cursed) {
            pline("额!  这让你感觉平庸!");
            break;
        } else {
            /* unlike unicorn horn, overrides Fixed_abil */
            pline("哇!  这让你感觉%s!",
                  (otmp->blessed)
                      ? (unfixable_trouble_count(FALSE) ? "好些" : "极好")
                      : "很好");
            i = rn2(A_MAX); /* start at a random point */
            for (ii = 0; ii < A_MAX; ii++) {
                lim = AMAX(i);
                if (i == A_STR && u.uhs >= 3)
                    --lim; /* WEAK */
                if (ABASE(i) < lim) {
                    ABASE(i) = lim;
                    context.botl = 1;
                    /* only first found if not blessed */
                    if (!otmp->blessed)
                        break;
                }
                if (++i >= A_MAX)
                    i = 0;
            }
        }
        break;
    case POT_HALLUCINATION:
        if (Hallucination || Halluc_resistance)
            nothing++;
        (void) make_hallucinated(
            itimeout_incr(HHallucination, rn1(200, 600 - 300 * bcsign(otmp))),
            TRUE, 0L);
        break;
    case POT_WATER:
        if (!otmp->blessed && !otmp->cursed) {
            pline("这尝起来像水.");
            u.uhunger += rnd(10);
            newuhs(FALSE);
            break;
        }
        unkn++;
        if (is_undead(youmonst.data) || is_demon(youmonst.data)
            || u.ualign.type == A_CHAOTIC) {
            if (otmp->blessed) {
                pline("这烧得像酸!");
                exercise(A_CON, FALSE);
                if (u.ulycn >= LOW_PM) {
                    Your("对%s 的倾向消失了!",
                         makeplural(mons[u.ulycn].mname));
                    if (youmonst.data == &mons[u.ulycn])
                        you_unwere(FALSE);
                    u.ulycn = NON_PM; /* cure lycanthropy */
                }
                losehp(Maybe_Half_Phys(d(2, 6)), "圣水",
                       KILLED_BY_AN);
            } else if (otmp->cursed) {
                You_feel("对你自己非常骄傲.");
                healup(d(2, 6), 0, 0, 0);
                if (u.ulycn >= LOW_PM && !Upolyd)
                    you_were();
                exercise(A_CON, TRUE);
            }
        } else {
            if (otmp->blessed) {
                You_feel("充满敬畏.");
                make_sick(0L, (char *) 0, TRUE, SICK_ALL);
                exercise(A_WIS, TRUE);
                exercise(A_CON, TRUE);
                if (u.ulycn >= LOW_PM)
                    you_unwere(TRUE); /* "Purified" */
                /* make_confused(0L, TRUE); */
            } else {
                if (u.ualign.type == A_LAWFUL) {
                    pline("这烧得像酸!");
                    losehp(Maybe_Half_Phys(d(2, 6)), "邪水",
                           KILLED_BY_AN);
                } else
                    You_feel("充满恐惧.");
                if (u.ulycn >= LOW_PM && !Upolyd)
                    you_were();
                exercise(A_CON, FALSE);
            }
        }
        break;
    case POT_BOOZE:
        unkn++;
        pline("啊!  这尝起来像%s%s!",
              otmp->odiluted ? "掺水的 " : "",
              Hallucination ? "蒲公英酒" : "液态火");
        if (!otmp->blessed)
            make_confused(itimeout_incr(HConfusion, d(3, 8)), FALSE);
        /* the whiskey makes us feel better */
        if (!otmp->odiluted)
            healup(1, 0, FALSE, FALSE);
        u.uhunger += 10 * (2 + bcsign(otmp));
        newuhs(FALSE);
        exercise(A_WIS, FALSE);
        if (otmp->cursed) {
            You("失去知觉.");
            multi = -rnd(15);
            nomovemsg = "你醒来时头痛.";
        }
        break;
    case POT_ENLIGHTENMENT:
        if (otmp->cursed) {
            unkn++;
            You("有一种不安的感觉...");
            exercise(A_WIS, FALSE);
        } else {
            if (otmp->blessed) {
                (void) adjattrib(A_INT, 1, FALSE);
                (void) adjattrib(A_WIS, 1, FALSE);
            }
            You_feel("自知的...");
            display_nhwindow(WIN_MESSAGE, FALSE);
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            pline_The("感觉减弱了.");
            exercise(A_WIS, TRUE);
        }
        break;
    case SPE_INVISIBILITY:
        /* spell cannot penetrate mummy wrapping */
        if (BInvis && uarmc->otyp == MUMMY_WRAPPING) {
            You_feel("在%s 之下相当痒.", yname(uarmc));
            break;
        }
        /* FALLTHRU */
    case POT_INVISIBILITY:
        if (Invis || Blind || BInvis) {
            nothing++;
        } else {
            self_invis_message();
        }
        if (otmp->blessed)
            HInvis |= FROMOUTSIDE;
        else
            incr_itimeout(&HInvis, rn1(15, 31));
        newsym(u.ux, u.uy); /* update position */
        if (otmp->cursed) {
            pline("由于某种原因, 你感觉你的存在为人所知了.");
            aggravate();
        }
        break;
    case POT_SEE_INVISIBLE: /* tastes like fruit juice in Rogue */
    case POT_FRUIT_JUICE: {
        int msg = Invisible && !Blind;

        unkn++;
        if (otmp->cursed)
            pline("哟!  这尝起来%s.",
                  Hallucination ? "过熟的" : "恶臭的");
        else
            pline(
                Hallucination
                    ? "这尝起来像%s10%% %s 含量的纯天然饮料."
                    : "这尝起来像%s%s.",
                otmp->odiluted ? "再造的 " : "", fruitname(TRUE));
        if (otmp->otyp == POT_FRUIT_JUICE) {
            u.uhunger += (otmp->odiluted ? 5 : 10) * (2 + bcsign(otmp));
            newuhs(FALSE);
            break;
        }
        if (!otmp->cursed) {
            /* Tell them they can see again immediately, which
             * will help them identify the potion...
             */
            make_blinded(0L, TRUE);
        }
        if (otmp->blessed)
            HSee_invisible |= FROMOUTSIDE;
        else
            incr_itimeout(&HSee_invisible, rn1(100, 750));
        set_mimic_blocking(); /* do special mimic handling */
        see_monsters();       /* see invisible monsters */
        newsym(u.ux, u.uy);   /* see yourself! */
        if (msg && !Blind) {  /* Blind possible if polymorphed */
            You("你能看见透明的自己, 但你是可见的!");
            unkn--;
        }
        break;
    }
    case POT_PARALYSIS:
        if (Free_action) {
            You("瞬间僵硬了下.");
        } else {
            if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
                You("不能动地飘浮着.");
            else if (u.usteed)
                You("就地被冰冻了!");
            else
                Your("%s 被冻结到%s!", makeplural(body_part(FOOT)),
                     surface(u.ux, u.uy));
            nomul(-(rn1(10, 25 - 12 * bcsign(otmp))));
            multi_reason = "被一瓶药水冰冻";
            nomovemsg = You_can_move_again;
            exercise(A_DEX, FALSE);
        }
        break;
    case POT_SLEEPING:
        if (Sleep_resistance || Free_action) {
            You("打呵欠.");
        } else {
            You("突然睡着了!");
            fall_asleep(-rn1(10, 25 - 12 * bcsign(otmp)), TRUE);
        }
        break;
    case POT_MONSTER_DETECTION:
    case SPE_DETECT_MONSTERS:
        if (otmp->blessed) {
            int x, y;

            if (Detect_monsters)
                nothing++;
            unkn++;
            /* after a while, repeated uses become less effective */
            if ((HDetect_monsters & TIMEOUT) >= 300L)
                i = 1;
            else
                i = rn1(40, 21);
            incr_itimeout(&HDetect_monsters, i);
            for (x = 1; x < COLNO; x++) {
                for (y = 0; y < ROWNO; y++) {
                    if (levl[x][y].glyph == GLYPH_INVISIBLE) {
                        unmap_object(x, y);
                        newsym(x, y);
                    }
                    if (MON_AT(x, y))
                        unkn = 0;
                }
            }
            see_monsters();
            if (unkn)
                You_feel("孤单的.");
            break;
        }
        if (monster_detect(otmp, 0))
            return 1; /* nothing detected */
        exercise(A_WIS, TRUE);
        break;
    case POT_OBJECT_DETECTION:
    case SPE_DETECT_TREASURE:
        if (object_detect(otmp, 0))
            return 1; /* nothing detected */
        exercise(A_WIS, TRUE);
        break;
    case POT_SICKNESS:
        pline("啊!  这个东西尝起来像毒药.");
        if (otmp->blessed) {
            pline("( 但实际上它是有点不新鲜的%s.)", fruitname(TRUE));
            if (!Role_if(PM_HEALER)) {
                /* NB: blessed otmp->fromsink is not possible */
                losehp(1, "轻微污染的药水", KILLED_BY_AN);
            }
        } else {
            if (Poison_resistance)
                pline("( 但实际上它是被生物污染的%s.)",
                      fruitname(TRUE));
            if (Role_if(PM_HEALER)) {
                pline("幸运的是, 你有免疫.");
            } else {
                char contaminant[BUFSZ];
                int typ = rn2(A_MAX);

                Sprintf(contaminant, "%s%s",
                        (Poison_resistance) ? "稍微 " : "",
                        (otmp->fromsink) ? "污染的自来水"
                                         : "污染的药水");
                if (!Fixed_abil) {
                    poisontell(typ, FALSE);
                    (void) adjattrib(typ, Poison_resistance ? -1 : -rn1(4, 3),
                                     1);
                }
                if (!Poison_resistance) {
                    if (otmp->fromsink)
                        losehp(rnd(10) + 5 * !!(otmp->cursed), contaminant,
                               KILLED_BY);
                    else
                        losehp(rnd(10) + 5 * !!(otmp->cursed), contaminant,
                               KILLED_BY_AN);
                } else {
                    /* rnd loss is so that unblessed poorer than blessed */
                    losehp(1 + rn2(2), contaminant,
                           (otmp->fromsink) ? KILLED_BY : KILLED_BY_AN);
                }
                exercise(A_CON, FALSE);
            }
        }
        if (Hallucination) {
            You("被电回你的感官!");
            (void) make_hallucinated(0L, FALSE, 0L);
        }
        break;
    case POT_CONFUSION:
        if (!Confusion) {
            if (Hallucination) {
                pline("好一个古怪的幻觉!");
                unkn++;
            } else
                pline("哈, 什么?  我在哪儿?");
        } else
            nothing++;
        make_confused(itimeout_incr(HConfusion,
                                    rn1(7, 16 - 8 * bcsign(otmp))),
                      FALSE);
        break;
    case POT_GAIN_ABILITY:
        if (otmp->cursed) {
            pline("啊!  那个药水尝起来令人作呕!");
            unkn++;
        } else if (Fixed_abil) {
            nothing++;
        } else {      /* If blessed, increase all; if not, try up to */
            int itmp; /* 6 times to find one which can be increased. */

            i = -1;   /* increment to 0 */
            for (ii = A_MAX; ii > 0; ii--) {
                i = (otmp->blessed ? i + 1 : rn2(A_MAX));
                /* only give "your X is already as high as it can get"
                   message on last attempt (except blessed potions) */
                itmp = (otmp->blessed || ii == 1) ? 0 : -1;
                if (adjattrib(i, 1, itmp) && !otmp->blessed)
                    break;
            }
        }
        break;
    case POT_SPEED:
        if (Wounded_legs && !otmp->cursed && !u.usteed) {
            /* heal_legs() would heal steeds legs */
            heal_legs();
            unkn++;
            break;
        }
        /* FALLTHRU */
    case SPE_HASTE_SELF:
        if (!Very_fast) { /* wwf@doe.carleton.ca */
            You("突然移动得%s快些了.", Fast ? "" : "非常 ");
        } else {
            Your("%s 获得了新活力.", makeplural(body_part(LEG)));
            unkn++;
        }
        exercise(A_DEX, TRUE);
        incr_itimeout(&HFast, rn1(10, 100 + 60 * bcsign(otmp)));
        break;
    case POT_BLINDNESS:
        if (Blind)
            nothing++;
        make_blinded(itimeout_incr(Blinded,
                                   rn1(200, 250 - 125 * bcsign(otmp))),
                     (boolean) !Blind);
        break;
    case POT_GAIN_LEVEL:
        if (otmp->cursed) {
            unkn++;
            /* they went up a level */
            if ((ledger_no(&u.uz) == 1 && u.uhave.amulet)
                || Can_rise_up(u.ux, u.uy, &u.uz)) {
                const char *riseup = "升了起来, 穿过了%s!";

                if (ledger_no(&u.uz) == 1) {
                    You(riseup, ceiling(u.ux, u.uy));
                    goto_level(&earth_level, FALSE, FALSE, FALSE);
                } else {
                    register int newlev = depth(&u.uz) - 1;
                    d_level newlevel;

                    get_level(&newlevel, newlev);
                    if (on_level(&newlevel, &u.uz)) {
                        pline("它尝起来很糟糕.");
                        break;
                    } else
                        You(riseup, ceiling(u.ux, u.uy));
                    goto_level(&newlevel, FALSE, FALSE, FALSE);
                }
            } else
                You("有一种不安的感觉.");
            break;
        }
        pluslvl(FALSE);
        /* blessed potions place you at a random spot in the
           middle of the new level instead of the low point */
        if (otmp->blessed)
            u.uexp = rndexp(TRUE);
        break;
    case POT_HEALING:
        You_feel("好些了.");
        healup(d(6 + 2 * bcsign(otmp), 4), !otmp->cursed ? 1 : 0,
               !!otmp->blessed, !otmp->cursed);
        exercise(A_CON, TRUE);
        break;
    case POT_EXTRA_HEALING:
        You_feel("好多了.");
        healup(d(6 + 2 * bcsign(otmp), 8),
               otmp->blessed ? 5 : !otmp->cursed ? 2 : 0, !otmp->cursed,
               TRUE);
        (void) make_hallucinated(0L, TRUE, 0L);
        exercise(A_CON, TRUE);
        exercise(A_STR, TRUE);
        break;
    case POT_FULL_HEALING:
        You_feel("完全愈合了.");
        healup(400, 4 + 4 * bcsign(otmp), !otmp->cursed, TRUE);
        /* Restore one lost level if blessed */
        if (otmp->blessed && u.ulevel < u.ulevelmax) {
            /* when multiple levels have been lost, drinking
               multiple potions will only get half of them back */
            u.ulevelmax -= 1;
            pluslvl(FALSE);
        }
        (void) make_hallucinated(0L, TRUE, 0L);
        exercise(A_STR, TRUE);
        exercise(A_CON, TRUE);
        break;
    case POT_LEVITATION:
    case SPE_LEVITATION:
        if (otmp->cursed)
            HLevitation &= ~I_SPECIAL;
        if (!Levitation && !BLevitation) {
            /* kludge to ensure proper operation of float_up() */
            set_itimeout(&HLevitation, 1L);
            float_up();
            /* reverse kludge */
            set_itimeout(&HLevitation, 0L);
            if (otmp->cursed) {
                if ((u.ux == xupstair && u.uy == yupstair)
                    || (sstairs.up && u.ux == sstairs.sx
                        && u.uy == sstairs.sy)
                    || (xupladder && u.ux == xupladder
                        && u.uy == yupladder)) {
                    (void) doup();
                } else if (has_ceiling(&u.uz)) {
                    int dmg = uarmh ? 1 : rnd(10);

                    You("你的%s 撞到了%s.", body_part(HEAD),
                        ceiling(u.ux, u.uy));
                    losehp(Maybe_Half_Phys(dmg), "撞到了天花板",
                           KILLED_BY);
                }
            } /*cursed*/
        } else
            nothing++;
        if (otmp->blessed) {
            incr_itimeout(&HLevitation, rn1(50, 250));
            HLevitation |= I_SPECIAL;
        } else
            incr_itimeout(&HLevitation, rn1(140, 10));
        if (Levitation)
            spoteffects(FALSE); /* for sinks */
        float_vs_flight();
        break;
    case POT_GAIN_ENERGY: { /* M. Stephenson */
        int num;

        if (otmp->cursed)
            You_feel("无生气.");
        else
            pline("魔法能量流过你的身体.");

        /* old: num = rnd(5) + 5 * otmp->blessed + 1;
         *      blessed:  +7..11 max & current (+9 avg)
         *      uncursed: +2.. 6 max & current (+4 avg)
         *      cursed:   -2.. 6 max & current (-4 avg)
         * new: (3.6.0)
         *      blessed:  +3..18 max (+10.5 avg), +9..54 current (+31.5 avg)
         *      uncursed: +2..12 max (+ 7   avg), +6..36 current (+21   avg)
         *      cursed:   -1.. 6 max (- 3.5 avg), -3..18 current (-10.5 avg)
         */
        num = d(otmp->blessed ? 3 : !otmp->cursed ? 2 : 1, 6);
        if (otmp->cursed)
            num = -num; /* subtract instead of add when cursed */
        u.uenmax += num;
        if (u.uenmax <= 0)
            u.uenmax = 0;
        u.uen += 3 * num;
        if (u.uen > u.uenmax)
            u.uen = u.uenmax;
        else if (u.uen <= 0)
            u.uen = 0;
        context.botl = 1;
        exercise(A_WIS, TRUE);
        break;
    }
    case POT_OIL: { /* P. Winner */
        boolean good_for_you = FALSE;

        if (otmp->lamplit) {
            if (likes_fire(youmonst.data)) {
                pline("啊, 提神的饮料.");
                good_for_you = TRUE;
            } else {
                You("的%s 被烧伤.", body_part(FACE));
                /* fire damage */
                losehp(d(Fire_resistance ? 1 : 3, 4), "燃烧的油药水",
                       KILLED_BY_AN);
            }
        } else if (otmp->cursed)
            pline("这尝起来像蓖麻油.");
        else
            pline("那是光滑的!");
        exercise(A_WIS, good_for_you);
        break;
    }
    case POT_ACID:
        if (Acid_resistance) {
            /* Not necessarily a creature who _likes_ acid */
            pline("这尝起来是%s.", Hallucination ? "刺鼻的" : "酸的");
        } else {
            int dmg;

            pline("这烧得%s!",
                  otmp->blessed ? " 有一点" : otmp->cursed ? " 许多"
                                                             : " 像酸");
            dmg = d(otmp->cursed ? 2 : 1, otmp->blessed ? 4 : 8);
            losehp(Maybe_Half_Phys(dmg), "酸药水", KILLED_BY_AN);
            exercise(A_CON, FALSE);
        }
        if (Stoned)
            fix_petrification();
        unkn++; /* holy/unholy water can burn like acid too */
        break;
    case POT_POLYMORPH:
        You_feel("有些%s.", Hallucination ? "正常" : "奇怪");
        if (!Unchanging)
            polyself(0);
        break;
    default:
        impossible("What a funny potion! (%u)", otmp->otyp);
        return 0;
    }
    return -1;
}

void
healup(nhp, nxtra, curesick, cureblind)
int nhp, nxtra;
register boolean curesick, cureblind;
{
    if (nhp) {
        if (Upolyd) {
            u.mh += nhp;
            if (u.mh > u.mhmax)
                u.mh = (u.mhmax += nxtra);
        } else {
            u.uhp += nhp;
            if (u.uhp > u.uhpmax)
                u.uhp = (u.uhpmax += nxtra);
        }
    }
    if (cureblind)
        make_blinded(0L, TRUE);
    if (curesick) {
        make_vomiting(0L, TRUE);
        make_sick(0L, (char *) 0, TRUE, SICK_ALL);
    }
    context.botl = 1;
    return;
}

void
strange_feeling(obj, txt)
struct obj *obj;
const char *txt;
{
    if (flags.beginner || !txt)
        You("片刻有一种%s感觉, 然后它消失了.",
            Hallucination ? "正常的" : "奇怪的");
    else
        pline1(txt);

    if (!obj) /* e.g., crystal ball finds no traps */
        return;

    if (obj->dknown && !objects[obj->otyp].oc_name_known
        && !objects[obj->otyp].oc_uname)
        docall(obj);

    useup(obj);
}

const char *bottlenames[] = { "瓶子", "小药瓶", "酒壶", "玻璃水瓶",
                              "细颈瓶",  "广口瓶",   "小瓶" };

const char *
bottlename()
{
    return bottlenames[rn2(SIZE(bottlenames))];
}

/* handle item dipped into water potion or steed saddle splashed by same */
STATIC_OVL boolean
H2Opotion_dip(potion, targobj, useeit, objphrase)
struct obj *potion, *targobj;
boolean useeit;
const char *objphrase; /* "Your widget glows" or "Steed's saddle glows" */
{
    void FDECL((*func), (OBJ_P)) = 0;
    const char *glowcolor = 0;
#define COST_alter (-2)
#define COST_none (-1)
    int costchange = COST_none;
    boolean altfmt = FALSE, res = FALSE;

    if (!potion || potion->otyp != POT_WATER)
        return FALSE;

    if (potion->blessed) {
        if (targobj->cursed) {
            func = uncurse;
            glowcolor = NH_AMBER;
            costchange = COST_UNCURS;
        } else if (!targobj->blessed) {
            func = bless;
            glowcolor = NH_LIGHT_BLUE;
            costchange = COST_alter;
            altfmt = TRUE; /* "with a <color> aura" */
        }
    } else if (potion->cursed) {
        if (targobj->blessed) {
            func = unbless;
            glowcolor = "棕色的";
            costchange = COST_UNBLSS;
        } else if (!targobj->cursed) {
            func = curse;
            glowcolor = NH_BLACK;
            costchange = COST_alter;
            altfmt = TRUE;
        }
    } else {
        /* dipping into uncursed water; carried() check skips steed saddle */
        if (carried(targobj)) {
            if (water_damage(targobj, 0, TRUE) != ER_NOTHING)
                res = TRUE;
        }
    }
    if (func) {
        /* give feedback before altering the target object;
           this used to set obj->bknown even when not seeing
           the effect; now hero has to see the glow, and bknown
           is cleared instead of set if perception is distorted */
        if (useeit) {
            glowcolor = hcolor(glowcolor);
            if (altfmt)
                pline("%s的光芒带有一个%s光环.", objphrase, glowcolor);
            else
                pline("%s %s光芒.", objphrase, glowcolor);
            iflags.last_msg = PLNMSG_OBJ_GLOWS;
            targobj->bknown = !Hallucination;
        }
        /* potions of water are the only shop goods whose price depends
           on their curse/bless state */
        if (targobj->unpaid && targobj->otyp == POT_WATER) {
            if (costchange == COST_alter)
                /* added blessing or cursing; update shop
                   bill to reflect item's new higher price */
                alter_cost(targobj, 0L);
            else if (costchange != COST_none)
                /* removed blessing or cursing; you
                   degraded it, now you'll have to buy it... */
                costly_alteration(targobj, costchange);
        }
        /* finally, change curse/bless state */
        (*func)(targobj);
        res = TRUE;
    }
    return res;
}

void
potionhit(mon, obj, your_fault)
register struct monst *mon;
register struct obj *obj;
boolean your_fault;
{
    const char *botlnam = bottlename();
    boolean isyou = (mon == &youmonst);
    int distance;
    struct obj *saddle = (struct obj *) 0;
    boolean hit_saddle = FALSE;

    if (isyou) {
        distance = 0;
        pline_The("%s砸在你的%s上然后破碎了.", botlnam,
                  body_part(HEAD));
        losehp(Maybe_Half_Phys(rnd(2)), "投掷的药水", KILLED_BY_AN);
    } else {
        /* sometimes it hits the saddle */
        if (((mon->misc_worn_check & W_SADDLE)
             && (saddle = which_armor(mon, W_SADDLE)))
            && (!rn2(10)
                || (obj->otyp == POT_WATER
                    && ((rnl(10) > 7 && obj->cursed)
                        || (rnl(10) < 4 && obj->blessed) || !rn2(3)))))
            hit_saddle = TRUE;
        distance = distu(mon->mx, mon->my);
        if (!cansee(mon->mx, mon->my))
            pline("破碎声!");
        else {
            char *mnam = mon_nam(mon);
            char buf[BUFSZ];

            if (hit_saddle && saddle) {
                Sprintf(buf, "%s 鞍",
                        s_suffix(x_monnam(mon, ARTICLE_THE, (char *) 0,
                                          (SUPPRESS_IT | SUPPRESS_SADDLE),
                                          FALSE)));
            } else if (has_head(mon->data)) {
                Sprintf(buf, "%s %s", s_suffix(mnam),
                        (notonhead ? "身体" : "头"));
            } else {
                Strcpy(buf, mnam);
            }
            pline_The("%s砸在%s上然后破碎了.", botlnam,
                      buf);
        }
        if (rn2(5) && mon->mhp > 1 && !hit_saddle)
            mon->mhp--;
    }

    /* oil doesn't instantly evaporate; Neither does a saddle hit */
    if (obj->otyp != POT_OIL && !hit_saddle && cansee(mon->mx, mon->my))
        pline("%s了.", Tobjnam(obj, "蒸发"));

    if (isyou) {
        switch (obj->otyp) {
        case POT_OIL:
            if (obj->lamplit)
                explode_oil(obj, u.ux, u.uy);
            break;
        case POT_POLYMORPH:
            You_feel("有点%s.", Hallucination ? "正常" : "奇怪");
            if (!Unchanging && !Antimagic)
                polyself(0);
            break;
        case POT_ACID:
            if (!Acid_resistance) {
                int dmg;
                pline("这烧伤了%s!",
                      obj->blessed ? "一点"
                                   : obj->cursed ? "许多" : "");
                dmg = d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
                losehp(Maybe_Half_Phys(dmg), "酸药水", KILLED_BY_AN);
            }
            break;
        }
    } else if (hit_saddle && saddle) {
        char *mnam, buf[BUFSZ], saddle_glows[BUFSZ];
        boolean affected = FALSE;
        boolean useeit = !Blind && canseemon(mon) && cansee(mon->mx, mon->my);

        mnam = x_monnam(mon, ARTICLE_THE, (char *) 0,
                        (SUPPRESS_IT | SUPPRESS_SADDLE), FALSE);
        Sprintf(buf, "%s", upstart(s_suffix(mnam)));

        switch (obj->otyp) {
        case POT_WATER:
            Sprintf(saddle_glows, "%s %s", buf, aobjnam(saddle, "发出"));
            affected = H2Opotion_dip(obj, saddle, useeit, saddle_glows);
            break;
        case POT_POLYMORPH:
            /* Do we allow the saddle to polymorph? */
            break;
        }
        if (useeit && !affected)
            pline("%s %s湿了.", buf, aobjnam(saddle, "变"));
    } else {
        boolean angermon = TRUE;

        if (!your_fault)
            angermon = FALSE;
        switch (obj->otyp) {
        case POT_HEALING:
        case POT_EXTRA_HEALING:
        case POT_FULL_HEALING:
            if (mon->data == &mons[PM_PESTILENCE])
                goto do_illness;
        /*FALLTHRU*/
        case POT_RESTORE_ABILITY:
        case POT_GAIN_ABILITY:
        do_healing:
            angermon = FALSE;
            if (mon->mhp < mon->mhpmax) {
                mon->mhp = mon->mhpmax;
                if (canseemon(mon))
                    pline("%s 再次看起来健康和强壮的.", Monnam(mon));
            }
            break;
        case POT_SICKNESS:
            if (mon->data == &mons[PM_PESTILENCE])
                goto do_healing;
            if (dmgtype(mon->data, AD_DISE)
                /* won't happen, see prior goto */
                || dmgtype(mon->data, AD_PEST)
                /* most common case */
                || resists_poison(mon)) {
                if (canseemon(mon))
                    pline("%s 看起来没有受伤.", Monnam(mon));
                break;
            }
        do_illness:
            if ((mon->mhpmax > 3) && !resist(mon, POTION_CLASS, 0, NOTELL))
                mon->mhpmax /= 2;
            if ((mon->mhp > 2) && !resist(mon, POTION_CLASS, 0, NOTELL))
                mon->mhp /= 2;
            if (mon->mhp > mon->mhpmax)
                mon->mhp = mon->mhpmax;
            if (canseemon(mon))
                pline("%s 看起来相当生病的.", Monnam(mon));
            break;
        case POT_CONFUSION:
        case POT_BOOZE:
            if (!resist(mon, POTION_CLASS, 0, NOTELL))
                mon->mconf = TRUE;
            break;
        case POT_INVISIBILITY:
            angermon = FALSE;
            mon_set_minvis(mon);
            break;
        case POT_SLEEPING:
            /* wakeup() doesn't rouse victims of temporary sleep */
            if (sleep_monst(mon, rnd(12), POTION_CLASS)) {
                pline("%s 陷入了沉睡.", Monnam(mon));
                slept_monst(mon);
            }
            break;
        case POT_PARALYSIS:
            if (mon->mcanmove) {
                /* really should be rnd(5) for consistency with players
                 * breathing potions, but...
                 */
                paralyze_monst(mon, rnd(25));
            }
            break;
        case POT_SPEED:
            angermon = FALSE;
            mon_adjust_speed(mon, 1, obj);
            break;
        case POT_BLINDNESS:
            if (haseyes(mon->data)) {
                register int btmp = 64 + rn2(32)
                            + rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);

                btmp += mon->mblinded;
                mon->mblinded = min(btmp, 127);
                mon->mcansee = 0;
            }
            break;
        case POT_WATER:
            if (is_undead(mon->data) || is_demon(mon->data)
                || is_were(mon->data) || is_vampshifter(mon)) {
                if (obj->blessed) {
                    pline("%s 在痛苦中%s!", Monnam(mon),
                          is_silent(mon->data) ? "翻滚" : "尖叫");
                    if (!is_silent(mon->data))
                        wake_nearto(mon->mx, mon->my, mon->data->mlevel * 10);
                    mon->mhp -= d(2, 6);
                    /* should only be by you */
                    if (mon->mhp < 1)
                        killed(mon);
                    else if (is_were(mon->data) && !is_human(mon->data))
                        new_were(mon); /* revert to human */
                } else if (obj->cursed) {
                    angermon = FALSE;
                    if (canseemon(mon))
                        pline("%s 看起来更健康了.", Monnam(mon));
                    mon->mhp += d(2, 6);
                    if (mon->mhp > mon->mhpmax)
                        mon->mhp = mon->mhpmax;
                    if (is_were(mon->data) && is_human(mon->data)
                        && !Protection_from_shape_changers)
                        new_were(mon); /* transform into beast */
                }
            } else if (mon->data == &mons[PM_GREMLIN]) {
                angermon = FALSE;
                (void) split_mon(mon, (struct monst *) 0);
            } else if (mon->data == &mons[PM_IRON_GOLEM]) {
                if (canseemon(mon))
                    pline("%s 生锈了.", Monnam(mon));
                mon->mhp -= d(1, 6);
                /* should only be by you */
                if (mon->mhp < 1)
                    killed(mon);
            }
            break;
        case POT_OIL:
            if (obj->lamplit)
                explode_oil(obj, mon->mx, mon->my);
            break;
        case POT_ACID:
            if (!resists_acid(mon) && !resist(mon, POTION_CLASS, 0, NOTELL)) {
                pline("%s 在痛苦中%s!", Monnam(mon),
                      is_silent(mon->data) ? "翻滚" : "尖叫");
                if (!is_silent(mon->data))
                    wake_nearto(mon->mx, mon->my, mon->data->mlevel * 10);
                mon->mhp -= d(obj->cursed ? 2 : 1, obj->blessed ? 4 : 8);
                if (mon->mhp < 1) {
                    if (your_fault)
                        killed(mon);
                    else
                        monkilled(mon, "", AD_ACID);
                }
            }
            break;
        case POT_POLYMORPH:
            (void) bhitm(mon, obj);
            break;
        /*
        case POT_GAIN_LEVEL:
        case POT_LEVITATION:
        case POT_FRUIT_JUICE:
        case POT_MONSTER_DETECTION:
        case POT_OBJECT_DETECTION:
            break;
        */
        }
        if (angermon)
            wakeup(mon);
        else
            mon->msleeping = 0;
    }

    /* Note: potionbreathe() does its own docall() */
    if ((distance == 0 || ((distance < 3) && rn2(5)))
        && (!breathless(youmonst.data) || haseyes(youmonst.data)))
        potionbreathe(obj);
    else if (obj->dknown && !objects[obj->otyp].oc_name_known
             && !objects[obj->otyp].oc_uname && cansee(mon->mx, mon->my))
        docall(obj);
    if (*u.ushops && obj->unpaid) {
        struct monst *shkp = shop_keeper(*in_rooms(u.ux, u.uy, SHOPBASE));

        if (shkp)
            (void) stolen_value(obj, u.ux, u.uy, (boolean) shkp->mpeaceful,
                                FALSE);
        else
            obj->unpaid = 0;
    }
    obfree(obj, (struct obj *) 0);
}

/* vapors are inhaled or get in your eyes */
void
potionbreathe(obj)
register struct obj *obj;
{
    register int i, ii, isdone, kn = 0;

    switch (obj->otyp) {
    case POT_RESTORE_ABILITY:
    case POT_GAIN_ABILITY:
        if (obj->cursed) {
            if (!breathless(youmonst.data))
                pline("额!  那个药水闻起来很糟!");
            else if (haseyes(youmonst.data)) {
                const char *eyes = body_part(EYE);

                if (eyecount(youmonst.data) != 1)
                    eyes = makeplural(eyes);
                Your("%s %s!", eyes, vtense(eyes, "刺痛"));
            }
            break;
        } else {
            i = rn2(A_MAX); /* start at a random point */
            for (isdone = ii = 0; !isdone && ii < A_MAX; ii++) {
                if (ABASE(i) < AMAX(i)) {
                    ABASE(i)++;
                    /* only first found if not blessed */
                    isdone = !(obj->blessed);
                    context.botl = 1;
                }
                if (++i >= A_MAX)
                    i = 0;
            }
        }
        break;
    case POT_FULL_HEALING:
        if (Upolyd && u.mh < u.mhmax)
            u.mh++, context.botl = 1;
        if (u.uhp < u.uhpmax)
            u.uhp++, context.botl = 1;
        /*FALLTHRU*/
    case POT_EXTRA_HEALING:
        if (Upolyd && u.mh < u.mhmax)
            u.mh++, context.botl = 1;
        if (u.uhp < u.uhpmax)
            u.uhp++, context.botl = 1;
        /*FALLTHRU*/
    case POT_HEALING:
        if (Upolyd && u.mh < u.mhmax)
            u.mh++, context.botl = 1;
        if (u.uhp < u.uhpmax)
            u.uhp++, context.botl = 1;
        exercise(A_CON, TRUE);
        break;
    case POT_SICKNESS:
        if (!Role_if(PM_HEALER)) {
            if (Upolyd) {
                if (u.mh <= 5)
                    u.mh = 1;
                else
                    u.mh -= 5;
            } else {
                if (u.uhp <= 5)
                    u.uhp = 1;
                else
                    u.uhp -= 5;
            }
            context.botl = 1;
            exercise(A_CON, FALSE);
        }
        break;
    case POT_HALLUCINATION:
        You("有一种瞬间的幻视.");
        break;
    case POT_CONFUSION:
    case POT_BOOZE:
        if (!Confusion)
            You_feel("稍微晕眩的.");
        make_confused(itimeout_incr(HConfusion, rnd(5)), FALSE);
        break;
    case POT_INVISIBILITY:
        if (!Blind && !Invis) {
            kn++;
            pline("有一瞬间你%s!",
                  See_invisible ? "能看见透明的自己"
                                : "看不到你自己");
        }
        break;
    case POT_PARALYSIS:
        kn++;
        if (!Free_action) {
            pline("%s 似乎在牵制着你.", Something);
            nomul(-rnd(5));
            multi_reason = "被一瓶药水冰冻";
            nomovemsg = You_can_move_again;
            exercise(A_DEX, FALSE);
        } else
            You("有一瞬间僵硬了.");
        break;
    case POT_SLEEPING:
        kn++;
        if (!Free_action && !Sleep_resistance) {
            You_feel("相当的累.");
            nomul(-rnd(5));
            multi_reason = "因魔力的药水睡过头";
            nomovemsg = You_can_move_again;
            exercise(A_DEX, FALSE);
        } else
            You("打哈欠.");
        break;
    case POT_SPEED:
        if (!Fast)
            Your("膝盖现在似乎更加灵活了.");
        incr_itimeout(&HFast, rnd(5));
        exercise(A_DEX, TRUE);
        break;
    case POT_BLINDNESS:
        if (!Blind && !Unaware) {
            kn++;
            pline("突然变黑了.");
        }
        make_blinded(itimeout_incr(Blinded, rnd(5)), FALSE);
        if (!Blind && !Unaware)
            Your1(vision_clears);
        break;
    case POT_WATER:
        if (u.umonnum == PM_GREMLIN) {
            (void) split_mon(&youmonst, (struct monst *) 0);
        } else if (u.ulycn >= LOW_PM) {
            /* vapor from [un]holy water will trigger
               transformation but won't cure lycanthropy */
            if (obj->blessed && youmonst.data == &mons[u.ulycn])
                you_unwere(FALSE);
            else if (obj->cursed && !Upolyd)
                you_were();
        }
        break;
    case POT_ACID:
    case POT_POLYMORPH:
        exercise(A_CON, FALSE);
        break;
    /*
    case POT_GAIN_LEVEL:
    case POT_LEVITATION:
    case POT_FRUIT_JUICE:
    case POT_MONSTER_DETECTION:
    case POT_OBJECT_DETECTION:
    case POT_OIL:
        break;
     */
    }
    /* note: no obfree() */
    if (obj->dknown) {
        if (kn)
            makeknown(obj->otyp);
        else if (!objects[obj->otyp].oc_name_known
                 && !objects[obj->otyp].oc_uname)
            docall(obj);
    }
}

/* returns the potion type when o1 is dipped in o2 */
STATIC_OVL short
mixtype(o1, o2)
register struct obj *o1, *o2;
{
    /* cut down on the number of cases below */
    if (o1->oclass == POTION_CLASS
        && (o2->otyp == POT_GAIN_LEVEL || o2->otyp == POT_GAIN_ENERGY
            || o2->otyp == POT_HEALING || o2->otyp == POT_EXTRA_HEALING
            || o2->otyp == POT_FULL_HEALING || o2->otyp == POT_ENLIGHTENMENT
            || o2->otyp == POT_FRUIT_JUICE)) {
        struct obj *swp;

        swp = o1;
        o1 = o2;
        o2 = swp;
    }

    switch (o1->otyp) {
    case POT_HEALING:
        switch (o2->otyp) {
        case POT_SPEED:
        case POT_GAIN_LEVEL:
        case POT_GAIN_ENERGY:
            return POT_EXTRA_HEALING;
        }
    case POT_EXTRA_HEALING:
        switch (o2->otyp) {
        case POT_GAIN_LEVEL:
        case POT_GAIN_ENERGY:
            return POT_FULL_HEALING;
        }
    case POT_FULL_HEALING:
        switch (o2->otyp) {
        case POT_GAIN_LEVEL:
        case POT_GAIN_ENERGY:
            return POT_GAIN_ABILITY;
        }
    case UNICORN_HORN:
        switch (o2->otyp) {
        case POT_SICKNESS:
            return POT_FRUIT_JUICE;
        case POT_HALLUCINATION:
        case POT_BLINDNESS:
        case POT_CONFUSION:
            return POT_WATER;
        }
        break;
    case AMETHYST: /* "a-methyst" == "not intoxicated" */
        if (o2->otyp == POT_BOOZE)
            return POT_FRUIT_JUICE;
        break;
    case POT_GAIN_LEVEL:
    case POT_GAIN_ENERGY:
        switch (o2->otyp) {
        case POT_CONFUSION:
            return (rn2(3) ? POT_BOOZE : POT_ENLIGHTENMENT);
        case POT_HEALING:
            return POT_EXTRA_HEALING;
        case POT_EXTRA_HEALING:
            return POT_FULL_HEALING;
        case POT_FULL_HEALING:
            return POT_GAIN_ABILITY;
        case POT_FRUIT_JUICE:
            return POT_SEE_INVISIBLE;
        case POT_BOOZE:
            return POT_HALLUCINATION;
        }
        break;
    case POT_FRUIT_JUICE:
        switch (o2->otyp) {
        case POT_SICKNESS:
            return POT_SICKNESS;
        case POT_SPEED:
            return POT_BOOZE;
        case POT_GAIN_LEVEL:
        case POT_GAIN_ENERGY:
            return POT_SEE_INVISIBLE;
        }
        break;
    case POT_ENLIGHTENMENT:
        switch (o2->otyp) {
        case POT_LEVITATION:
            if (rn2(3))
                return POT_GAIN_LEVEL;
            break;
        case POT_FRUIT_JUICE:
            return POT_BOOZE;
        case POT_BOOZE:
            return POT_CONFUSION;
        }
        break;
    }

    return 0;
}

/* #dip command */
int
dodip()
{
    register struct obj *potion, *obj;
    struct obj *singlepotion;
    uchar here;
    char allowall[2];
    short mixture;
    char qbuf[QBUFSZ], qtoo[QBUFSZ];

    allowall[0] = ALL_CLASSES;
    allowall[1] = '\0';
    if (!(obj = getobj(allowall, "浸")))  //dip
        return 0;
    if (inaccessible_equipment(obj, "浸", FALSE))
        return 0;

    Sprintf(qbuf, "把%s 浸入到", thesimpleoname(obj));
    here = levl[u.ux][u.uy].typ;
    /* Is there a fountain to dip into here? */
    if (IS_FOUNTAIN(here)) {
        /* "Dip <the object> into the fountain?" */
        Sprintf(qtoo, "%s喷泉?", qbuf);
        if (yn(upstart(qtoo)) == 'y') {
            dipfountain(obj);
            return 1;
        }
    } else if (is_pool(u.ux, u.uy)) {
        const char *pooltype = waterbody_name(u.ux, u.uy);

        /* "Dip <the object> into the {pool, moat, &c}?" */
        Sprintf(qtoo, "%s %s?", qbuf, pooltype);
        if (yn(upstart(qtoo)) == 'y') {
            if (Levitation) {
                floating_above(pooltype);
            } else if (u.usteed && !is_swimmer(u.usteed->data)
                       && P_SKILL(P_RIDING) < P_BASIC) {
                rider_cant_reach(); /* not skilled enough to reach */
            } else {
                if (obj->otyp == POT_ACID)
                    obj->in_use = 1;
                if (water_damage(obj, 0, TRUE) != ER_DESTROYED && obj->in_use)
                    useup(obj);
            }
            return 1;
        }
    }

    /* "What do you want to dip <the object> into?" */
    potion = getobj(beverages, qbuf); /* "dip into" */
    if (!potion)
        return 0;
    if (potion == obj && potion->quan == 1L) {
        pline("那是一个药水瓶, 不是克莱因瓶!");
        return 0;
    }
    potion->in_use = TRUE; /* assume it will be used up */
    if (potion->otyp == POT_WATER) {
        boolean useeit = !Blind || (obj == ublindf && Blindfolded_only);
        const char *obj_glows = Yobjnam2(obj, "发出");

        if (H2Opotion_dip(potion, obj, useeit, obj_glows))
            goto poof;
    } else if (obj->otyp == POT_POLYMORPH || potion->otyp == POT_POLYMORPH) {
        /* some objects can't be polymorphed */
        if (obj->otyp == potion->otyp /* both POT_POLY */
            || obj->otyp == WAN_POLYMORPH || obj->otyp == SPE_POLYMORPH
            || obj == uball || obj == uskin
            || obj_resists(obj->otyp == POT_POLYMORPH ? potion : obj,
                           5, 95)) {
            pline1(nothing_happens);
        } else {
            boolean was_wep, was_swapwep, was_quiver;
            short save_otyp = obj->otyp;

            /* KMH, conduct */
            u.uconduct.polypiles++;

            was_wep = (obj == uwep);
            was_swapwep = (obj == uswapwep);
            was_quiver = (obj == uquiver);

            obj = poly_obj(obj, STRANGE_OBJECT);

            if (was_wep)
                setuwep(obj);
            else if (was_swapwep)
                setuswapwep(obj);
            else if (was_quiver)
                setuqwep(obj);

            if (obj->otyp != save_otyp) {
                makeknown(POT_POLYMORPH);
                useup(potion);
                prinv((char *) 0, obj, 0L);
                return 1;
            } else {
                pline("似乎没有什么要发生.");
                goto poof;
            }
        }
        potion->in_use = FALSE; /* didn't go poof */
        return 1;
    } else if (obj->oclass == POTION_CLASS && obj->otyp != potion->otyp) {
        long amt = obj->quan;

        Strcpy(qbuf, "");
        if (amt > (objects[potion->otyp].oc_magic ? 2L : 9L)) {
            /* trying to dip multiple potions will usually affect only a
               subset; pick an amount between 2 and min(N,9), inclusive */
            amt -= 1L;
            do {
                amt = (long) rnd((int) amt);
            } while (amt >= 9L);
            amt += 1L;
            if (amt < obj->quan) {
                obj = splitobj(obj, amt);
                Sprintf(qbuf, "%ld 个", obj->quan);
            }
        }
        /* [N of] the {obj(s)} mix(es) with [one of] {the potion}... */
        pline("%s %s 和%s%s%s...", qbuf, simpleonames(obj),
              (potion->quan > 1L) ? "一个" : "",
              thesimpleoname(potion), otense(obj, "混合"));
        /* Mixing potions is dangerous...
           KMH, balance patch -- acid is particularly unstable */
        if (obj->cursed || obj->otyp == POT_ACID || !rn2(10)) {
            /* it would be better to use up the whole stack in advance
               of the message, but we can't because we need to keep it
               around for potionbreathe() [and we can't set obj->in_use
               to 'amt' because that's not implemented] */
            obj->in_use = 1;
            pline("嘣!  它们爆炸了!");
            wake_nearto(u.ux, u.uy, (BOLT_LIM + 1) * (BOLT_LIM + 1));
            exercise(A_STR, FALSE);
            if (!breathless(youmonst.data) || haseyes(youmonst.data))
                potionbreathe(obj);
            useupall(obj);
            useup(potion);
            losehp((int) (amt + rnd(9)), /* not physical damage */
                   "炼金术爆炸", KILLED_BY_AN);
            return 1;
        }

        obj->blessed = obj->cursed = obj->bknown = 0;
        if (Blind || Hallucination)
            obj->dknown = 0;

        if ((mixture = mixtype(obj, potion)) != 0) {
            obj->otyp = mixture;
        } else {
            switch (obj->odiluted ? 1 : rnd(8)) {
            case 1:
                obj->otyp = POT_WATER;
                break;
            case 2:
            case 3:
                obj->otyp = POT_SICKNESS;
                break;
            case 4: {
                struct obj *otmp = mkobj(POTION_CLASS, FALSE);

                obj->otyp = otmp->otyp;
                obfree(otmp, (struct obj *) 0);
                break;
            }
            default:
                useupall(obj);
                useup(potion);
                if (!Blind)
                    pline_The("混合物明亮地发光然后蒸发了.");
                return 1;
            }
        }
        obj->odiluted = (obj->otyp != POT_WATER);

        if (obj->otyp == POT_WATER && !Hallucination) {
            pline_The("混合物冒泡%s.", Blind ? "" : ", 然后清澈了");
        } else if (!Blind) {
            pline_The("混合物看起来是%s.",
                      hcolor(OBJ_DESCR(objects[obj->otyp])));
        }

        useup(potion);
        /* this is required when 'obj' was split off from a bigger stack,
           so that 'obj' will now be assigned its own inventory slot;
           it has a side-effect of merging 'obj' into another compatible
           stack if there is one, so we do it even when no split has
           been made in order to get the merge result for both cases;
           as a consequence, mixing while Fumbling drops the mixture */
        freeinv(obj);
        (void) hold_another_object(obj, "你掉落了 %s!", doname(obj),
                                   (const char *) 0);
        return 1;
    }

    if (potion->otyp == POT_ACID && obj->otyp == CORPSE
        && obj->corpsenm == PM_LICHEN && !Blind) {
        pline("%s边缘的周围%s了%s.", The(cxname(obj)),
              otense(obj, "变为"),
              potion->odiluted ? hcolor(NH_ORANGE) : hcolor(NH_RED));
        potion->in_use = FALSE; /* didn't go poof */
        return 1;
    }

    if (potion->otyp == POT_WATER && obj->otyp == TOWEL) {
        pline_The("毛巾把它吸收了!");
        /* wetting towel already done via water_damage() in H2Opotion_dip */
        goto poof;
    }

    if (is_poisonable(obj)) {
        if (potion->otyp == POT_SICKNESS && !obj->opoisoned) {
            char buf[BUFSZ];

            if (potion->quan > 1L)
                Sprintf(buf, "一个%s", the(xname(potion)));
            else
                Strcpy(buf, The(xname(potion)));
            pline("%s 在%s上形成了涂层.", buf, the(xname(obj)));
            obj->opoisoned = TRUE;
            goto poof;
        } else if (obj->opoisoned && (potion->otyp == POT_HEALING
                                      || potion->otyp == POT_EXTRA_HEALING
                                      || potion->otyp == POT_FULL_HEALING)) {
            pline("涂层磨损了%s.", the(xname(obj)));
            obj->opoisoned = 0;
            goto poof;
        }
    }

    if (potion->otyp == POT_ACID) {
        if (erode_obj(obj, 0, ERODE_CORRODE, EF_GREASE) != ER_NOTHING)
            goto poof;
    }

    if (potion->otyp == POT_OIL) {
        boolean wisx = FALSE;

        if (potion->lamplit) { /* burning */
            fire_damage(obj, TRUE, u.ux, u.uy);
        } else if (potion->cursed) {
            pline_The("药水溅出来, 你的%s被覆盖上了油.",
                      makeplural(body_part(FINGER)));
            incr_itimeout(&Glib, d(2, 10));
        } else if (obj->oclass != WEAPON_CLASS && !is_weptool(obj)) {
            /* the following cases apply only to weapons */
            goto more_dips;
            /* Oil removes rust and corrosion, but doesn't unburn.
             * Arrows, etc are classed as metallic due to arrowhead
             * material, but dipping in oil shouldn't repair them.
             */
        } else if ((!is_rustprone(obj) && !is_corrodeable(obj))
                   || is_ammo(obj) || (!obj->oeroded && !obj->oeroded2)) {
            /* uses up potion, doesn't set obj->greased */
            pline("%s %s着油性光泽.", Yname2(obj),
                  otense(obj, "闪烁"));
        } else {
            pline("%s %s不那么%s.", Yname2(obj), otense(obj, "现在"),
                  (obj->oeroded && obj->oeroded2)
                      ? "腐蚀和生锈的"
                      : obj->oeroded ? "生锈的" : "腐蚀的");
            if (obj->oeroded > 0)
                obj->oeroded--;
            if (obj->oeroded2 > 0)
                obj->oeroded2--;
            wisx = TRUE;
        }
        exercise(A_WIS, wisx);
        makeknown(potion->otyp);
        useup(potion);
        return 1;
    }
more_dips:

    /* Allow filling of MAGIC_LAMPs to prevent identification by player */
    if ((obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP)
        && (potion->otyp == POT_OIL)) {
        /* Turn off engine before fueling, turn off fuel too :-)  */
        if (obj->lamplit || potion->lamplit) {
            useup(potion);
            explode(u.ux, u.uy, 11, d(6, 6), 0, EXPL_FIERY);
            exercise(A_WIS, FALSE);
            return 1;
        }
        /* Adding oil to an empty magic lamp renders it into an oil lamp */
        if ((obj->otyp == MAGIC_LAMP) && obj->spe == 0) {
            obj->otyp = OIL_LAMP;
            obj->age = 0;
        }
        if (obj->age > 1000L) {
            pline("%s %s满的.", Yname2(obj), otense(obj, "是"));
            potion->in_use = FALSE; /* didn't go poof */
        } else {
            You("用油装满了%s.", yname(obj));
            check_unpaid(potion);        /* Yendorian Fuel Tax */
            obj->age += 2 * potion->age; /* burns more efficiently */
            if (obj->age > 1500L)
                obj->age = 1500L;
            useup(potion);
            exercise(A_WIS, TRUE);
        }
        makeknown(POT_OIL);
        obj->spe = 1;
        update_inventory();
        return 1;
    }

    potion->in_use = FALSE; /* didn't go poof */
    if ((obj->otyp == UNICORN_HORN || obj->otyp == AMETHYST)
        && (mixture = mixtype(obj, potion)) != 0) {
        char oldbuf[BUFSZ], newbuf[BUFSZ];
        short old_otyp = potion->otyp;
        boolean old_dknown = FALSE;
        boolean more_than_one = potion->quan > 1L;

        oldbuf[0] = '\0';
        if (potion->dknown) {
            old_dknown = TRUE;
            Sprintf(oldbuf, "%s ", hcolor(OBJ_DESCR(objects[potion->otyp])));
        }
        /* with multiple merged potions, split off one and
           just clear it */
        if (potion->quan > 1L) {
            singlepotion = splitobj(potion, 1L);
        } else
            singlepotion = potion;

        costly_alteration(singlepotion, COST_NUTRLZ);
        singlepotion->otyp = mixture;
        singlepotion->blessed = 0;
        if (mixture == POT_WATER)
            singlepotion->cursed = singlepotion->odiluted = 0;
        else
            singlepotion->cursed = obj->cursed; /* odiluted left as-is */
        singlepotion->bknown = FALSE;
        if (Blind) {
            singlepotion->dknown = FALSE;
        } else {
            singlepotion->dknown = !Hallucination;
            if (mixture == POT_WATER && singlepotion->dknown)
                Sprintf(newbuf, "清澈了");
            else
                Sprintf(newbuf, "变为了%s",
                        hcolor(OBJ_DESCR(objects[mixture])));
            pline_The("%s%s药水%s.",
                      more_than_one ? "你浸入的那瓶" : "", 
                      oldbuf,
                      newbuf);
            if (!objects[old_otyp].oc_uname
                && !objects[old_otyp].oc_name_known && old_dknown) {
                struct obj fakeobj;
                fakeobj = zeroobj;
                fakeobj.dknown = 1;
                fakeobj.otyp = old_otyp;
                fakeobj.oclass = POTION_CLASS;
                docall(&fakeobj);
            }
        }
        obj_extract_self(singlepotion);
        singlepotion =
            hold_another_object(singlepotion, "你变戏法并掉落了%s!",
                                doname(singlepotion), (const char *) 0);
        update_inventory();
        return 1;
    }

    pline("有趣...");
    return 1;

poof:
    if (!objects[potion->otyp].oc_name_known
        && !objects[potion->otyp].oc_uname)
        docall(potion);
    useup(potion);
    return 1;
}

/* *monp grants a wish and then leaves the game */
void
mongrantswish(monp)
struct monst **monp;
{
    struct monst *mon = *monp;
    int mx = mon->mx, my = mon->my, glyph = glyph_at(mx, my);

    /* remove the monster first in case wish proves to be fatal
       (blasted by artifact), to keep it out of resulting bones file */
    mongone(mon);
    *monp = 0; /* inform caller that monster is gone */
    /* hide that removal from player--map is visible during wish prompt */
    tmp_at(DISP_ALWAYS, glyph);
    tmp_at(mx, my);
    /* grant the wish */
    makewish();
    /* clean up */
    tmp_at(DISP_END, 0);
}

void
djinni_from_bottle(obj)
struct obj *obj;
{
    struct monst *mtmp;
    int chance;

    if (!(mtmp = makemon(&mons[PM_DJINNI], u.ux, u.uy, NO_MM_FLAGS))) {
        pline("原来是空的.");
        return;
    }

    if (!Blind) {
        pline("在一团烟雾中, %s 浮现出来!", a_monnam(mtmp));
        pline("%s 说.", Monnam(mtmp));
    } else {
        You("闻到刺鼻的气味.");
        pline("%s 说.", Something);
    }

    chance = rn2(5);
    if (obj->blessed)
        chance = (chance == 4) ? rnd(4) : 0;
    else if (obj->cursed)
        chance = (chance == 0) ? rn2(4) : 4;
    /* 0,1,2,3,4:  b=80%,5,5,5,5; nc=20%,20,20,20,20; c=5%,5,5,5,80 */

    switch (chance) {
    case 0:
        verbalize("我欠你一个人情.  我满足你一个愿望!");
        /* give a wish and discard the monster (mtmp set to null) */
        mongrantswish(&mtmp);
        break;
    case 1:
        verbalize("谢谢你放我出来!");
        (void) tamedog(mtmp, (struct obj *) 0);
        break;
    case 2:
        verbalize("你释放了我!");
        mtmp->mpeaceful = TRUE;
        set_malign(mtmp);
        break;
    case 3:
        verbalize("时间差不多了!");
        if (canspotmon(mtmp))
            pline("%s 消失了.", Monnam(mtmp));
        mongone(mtmp);
        break;
    default:
        verbalize("你打扰了我, 笨蛋!");
        mtmp->mpeaceful = FALSE;
        set_malign(mtmp);
        break;
    }
}

/* clone a gremlin or mold (2nd arg non-null implies heat as the trigger);
   hit points are cut in half (odd HP stays with original) */
struct monst *
split_mon(mon, mtmp)
struct monst *mon,  /* monster being split */
             *mtmp; /* optional attacker whose heat triggered it */
{
    struct monst *mtmp2;
    char reason[BUFSZ];

    reason[0] = '\0';
    if (mtmp)
        Sprintf(reason, "因%s热量",
                (mtmp == &youmonst) ? the_your[1]
                                    : (const char *) s_suffix(mon_nam(mtmp)));

    if (mon == &youmonst) {
        mtmp2 = cloneu();
        if (mtmp2) {
            mtmp2->mhpmax = u.mhmax / 2;
            u.mhmax -= mtmp2->mhpmax;
            context.botl = 1;
            You("%s繁殖了!", reason);
        }
    } else {
        mtmp2 = clone_mon(mon, 0, 0);
        if (mtmp2) {
            mtmp2->mhpmax = mon->mhpmax / 2;
            mon->mhpmax -= mtmp2->mhpmax;
            if (canspotmon(mon))
                pline("%s %s繁殖了!", Monnam(mon), reason);
        }
    }
    return mtmp2;
}

/*potion.c*/
