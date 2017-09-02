/* NetHack 3.6	trap.c	$NHDT-Date: 1448492213 2015/11/25 22:56:53 $  $NHDT-Branch: master $:$NHDT-Revision: 1.249 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const char *const destroy_strings[][3]; /* from zap.c */

STATIC_DCL void FDECL(dofiretrap, (struct obj *));
STATIC_DCL void NDECL(domagictrap);
STATIC_DCL boolean FDECL(emergency_disrobe, (boolean *));
STATIC_DCL int FDECL(untrap_prob, (struct trap *));
STATIC_DCL void FDECL(move_into_trap, (struct trap *));
STATIC_DCL int FDECL(try_disarm, (struct trap *, BOOLEAN_P));
STATIC_DCL void FDECL(reward_untrap, (struct trap *, struct monst *));
STATIC_DCL int FDECL(disarm_holdingtrap, (struct trap *));
STATIC_DCL int FDECL(disarm_landmine, (struct trap *));
STATIC_DCL int FDECL(disarm_squeaky_board, (struct trap *));
STATIC_DCL int FDECL(disarm_shooting_trap, (struct trap *, int));
STATIC_DCL int FDECL(try_lift, (struct monst *, struct trap *, int,
                                BOOLEAN_P));
STATIC_DCL int FDECL(help_monster_out, (struct monst *, struct trap *));
STATIC_DCL boolean FDECL(thitm, (int, struct monst *, struct obj *, int,
                                 BOOLEAN_P));
STATIC_DCL void FDECL(launch_drop_spot, (struct obj *, XCHAR_P, XCHAR_P));
STATIC_DCL int FDECL(mkroll_launch, (struct trap *, XCHAR_P, XCHAR_P,
                                     SHORT_P, long));
STATIC_DCL boolean FDECL(isclearpath, (coord *, int, SCHAR_P, SCHAR_P));
STATIC_DCL char *FDECL(trapnote, (struct trap *, BOOLEAN_P));
#if 0
STATIC_DCL void FDECL(join_adjacent_pits, (struct trap *));
#endif
STATIC_DCL void FDECL(clear_conjoined_pits, (struct trap *));
STATIC_DCL int FDECL(steedintrap, (struct trap *, struct obj *));
STATIC_DCL boolean FDECL(keep_saddle_with_steedcorpse, (unsigned,
                                                        struct obj *,
                                                        struct obj *));
STATIC_DCL void NDECL(maybe_finish_sokoban);

/* mintrap() should take a flags argument, but for time being we use this */
STATIC_VAR int force_mintrap = 0;

STATIC_VAR const char *const a_your[2] = { "", "你的" };
STATIC_VAR const char *const A_Your[2] = { "", "你的" };
STATIC_VAR const char tower_of_flame[] = "火焰塔";
STATIC_VAR const char *const A_gush_of_water_hits = "一股水流打中了";
STATIC_VAR const char *const blindgas[6] = { "湿润的",   "无味",
                                             "刺激性的", "寒冷的",
                                             "辛辣的",   "刺骨的" };

/* called when you're hit by fire (dofiretrap,buzz,zapyourself,explode);
   returns TRUE if hit on torso */
boolean
burnarmor(victim)
struct monst *victim;
{
    struct obj *item;
    char buf[BUFSZ];
    int mat_idx, oldspe;
    boolean hitting_u;

    if (!victim)
        return 0;
    hitting_u = (victim == &youmonst);

    /* burning damage may dry wet towel */
    item = hitting_u ? carrying(TOWEL) : m_carrying(victim, TOWEL);
    while (item) {
        if (is_wet_towel(item)) {
            oldspe = item->spe;
            dry_a_towel(item, rn2(oldspe + 1), TRUE);
            if (item->spe != oldspe)
                break; /* stop once one towel has been affected */
        }
        item = item->nobj;
    }

#define burn_dmg(obj, descr) erode_obj(obj, descr, ERODE_BURN, EF_GREASE)
    while (1) {
        switch (rn2(5)) {
        case 0:
            item = hitting_u ? uarmh : which_armor(victim, W_ARMH);
            if (item) {
                mat_idx = objects[item->otyp].oc_material;
                Sprintf(buf, "%s %s", materialnm[mat_idx],
                        helm_simple_name(item));
            }
            if (!burn_dmg(item, item ? buf : "头盔"))
                continue;
            break;
        case 1:
            item = hitting_u ? uarmc : which_armor(victim, W_ARMC);
            if (item) {
                (void) burn_dmg(item, cloak_simple_name(item));
                return TRUE;
            }
            item = hitting_u ? uarm : which_armor(victim, W_ARM);
            if (item) {
                (void) burn_dmg(item, xname(item));
                return TRUE;
            }
            item = hitting_u ? uarmu : which_armor(victim, W_ARMU);
            if (item)
                (void) burn_dmg(item, "衬衫");
            return TRUE;
        case 2:
            item = hitting_u ? uarms : which_armor(victim, W_ARMS);
            if (!burn_dmg(item, "木盾"))
                continue;
            break;
        case 3:
            item = hitting_u ? uarmg : which_armor(victim, W_ARMG);
            if (!burn_dmg(item, "手套"))
                continue;
            break;
        case 4:
            item = hitting_u ? uarmf : which_armor(victim, W_ARMF);
            if (!burn_dmg(item, "靴子"))
                continue;
            break;
        }
        break; /* Out of while loop */
    }
#undef burn_dmg

    return FALSE;
}

/* Generic erode-item function.
 * "ostr", if non-null, is an alternate string to print instead of the
 *   object's name.
 * "type" is an ERODE_* value for the erosion type
 * "flags" is an or-ed list of EF_* flags
 *
 * Returns an erosion return value (ER_*)
 */
int
erode_obj(otmp, ostr, type, ef_flags)
register struct obj *otmp;
const char *ostr;
int type;
int ef_flags;
{
    static NEARDATA const char *const action[] = { "烧焦", "生锈", "腐烂",
                                                   "腐蚀" };
    static NEARDATA const char *const msg[] = { "烧焦", "生锈", "腐烂",
                                                "腐蚀" };
    boolean vulnerable = FALSE;
    boolean is_primary = TRUE;
    boolean check_grease = ef_flags & EF_GREASE;
    boolean print = ef_flags & EF_VERBOSE;
    int erosion;
    struct monst *victim;
    boolean vismon;
    boolean visobj;
    int cost_type;

    if (!otmp)
        return ER_NOTHING;

    victim = carried(otmp) ? &youmonst : mcarried(otmp) ? otmp->ocarry : NULL;
    vismon = victim && (victim != &youmonst) && canseemon(victim);
    /* Is bhitpos correct here? Ugh. */
    visobj = !victim && cansee(bhitpos.x, bhitpos.y);

    switch (type) {
    case ERODE_BURN:
        vulnerable = is_flammable(otmp);
        check_grease = FALSE;
        cost_type = COST_BURN;
        break;
    case ERODE_RUST:
        vulnerable = is_rustprone(otmp);
        cost_type = COST_RUST;
        break;
    case ERODE_ROT:
        vulnerable = is_rottable(otmp);
        check_grease = FALSE;
        is_primary = FALSE;
        cost_type = COST_ROT;
        break;
    case ERODE_CORRODE:
        vulnerable = is_corrodeable(otmp);
        is_primary = FALSE;
        cost_type = COST_CORRODE;
        break;
    default:
        impossible("Invalid erosion type in erode_obj");
        return ER_NOTHING;
    }
    erosion = is_primary ? otmp->oeroded : otmp->oeroded2;

    if (!ostr)
        ostr = cxname(otmp);

    if (check_grease && otmp->greased) {
        grease_protect(otmp, ostr, victim);
        return ER_GREASED;
    } else if (!vulnerable || (otmp->oerodeproof && otmp->rknown)) {
        if (print && flags.verbose) {
            if (victim == &youmonst)
                Your("%s %s受影响.", ostr, vtense(ostr, "没有"));
            else if (vismon)
                pline("%s %s %s受影响.", s_suffix(Monnam(victim)),
                      ostr, vtense(ostr, "没有"));
        }
        return ER_NOTHING;
    } else if (otmp->oerodeproof || (otmp->blessed && !rnl(4))) {
        if (flags.verbose && (print || otmp->oerodeproof)) {
            if (victim == &youmonst)
                pline("不知怎的, 你的 %s %s受影响.", ostr,
                      vtense(ostr, "没有"));
            else if (vismon)
                pline("不知怎的, %s %s %s受影响.",
                      s_suffix(mon_nam(victim)), ostr, vtense(ostr, "没有"));
            else if (visobj)
                pline("不知怎的, %s %s受影响.", ostr,
                      vtense(ostr, "没有"));
        }
        /* We assume here that if the object is protected because it
         * is blessed, it still shows some minor signs of wear, and
         * the hero can distinguish this from an object that is
         * actually proof against damage. */
        if (otmp->oerodeproof) {
            otmp->rknown = TRUE;
            if (victim == &youmonst)
                update_inventory();
        }

        return ER_NOTHING;
    } else if (erosion < MAX_ERODE) {
        const char *adverb = (erosion + 1 == MAX_ERODE)
                                 ? "完全"
                                 : erosion ? "更加" : "";

        if (victim == &youmonst)
            Your("%s %s%s了!", ostr, adverb, vtense(ostr, action[type]));
        else if (vismon)
            pline("%s %s %s%s了!", s_suffix(Monnam(victim)), ostr,
                  adverb, vtense(ostr, action[type]));
        else if (visobj)
            pline("%s %s%s了!", ostr, adverb, vtense(ostr, action[type]));

        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        if (is_primary)
            otmp->oeroded++;
        else
            otmp->oeroded2++;

        if (victim == &youmonst)
            update_inventory();

        return ER_DAMAGED;
    } else if (ef_flags & EF_DESTROY) {
        if (victim == &youmonst)
            Your("%s %s掉了!", ostr, vtense(ostr, action[type]));
        else if (vismon)
            pline("%s %s %s掉了!", s_suffix(Monnam(victim)), ostr,
                  vtense(ostr, action[type]));
        else if (visobj)
            pline("The %s %s掉了!", ostr, vtense(ostr, action[type]));

        if (ef_flags & EF_PAY)
            costly_alteration(otmp, cost_type);

        setnotworn(otmp);
        delobj(otmp);
        return ER_DESTROYED;
    } else {
        if (flags.verbose && print) {
            if (victim == &youmonst)
                Your("%s %s完全%s了.", ostr,
                     vtense(ostr, Blind ? "感觉" : "看起来"), msg[type]);
            else if (vismon)
                pline("%s %s %s完全%s了.", s_suffix(Monnam(victim)),
                      ostr, vtense(ostr, "看起来"), msg[type]);
            else if (visobj)
                pline("%s %s完全%s了.", ostr, vtense(ostr, "看起来"),
                      msg[type]);
        }
        return ER_NOTHING;
    }
}

/* Protect an item from erosion with grease. Returns TRUE if the grease
 * wears off.
 */
boolean
grease_protect(otmp, ostr, victim)
register struct obj *otmp;
const char *ostr;
struct monst *victim;
{
    static const char txt[] = "油脂层的保护!";
    boolean vismon = victim && (victim != &youmonst) && canseemon(victim);

    if (ostr) {
        if (victim == &youmonst)
            Your("%s %s%s", ostr, vtense(ostr, "受"), txt);
        else if (vismon)
            pline("%s的%s %s%s", Monnam(victim),
                  ostr, vtense(ostr, "受"), txt);
    } else if (victim == &youmonst || vismon) {
        pline("%s%s", Yobjnam2(otmp, "受"), txt);
    }
    if (!rn2(2)) {
        otmp->greased = 0;
        if (carried(otmp)) {
            pline_The("油脂溶解了.");
            update_inventory();
        }
        return TRUE;
    }
    return FALSE;
}

struct trap *
maketrap(x, y, typ)
register int x, y, typ;
{
    static union vlaunchinfo zero_vl;
    register struct trap *ttmp;
    register struct rm *lev;
    boolean oldplace;

    if ((ttmp = t_at(x, y)) != 0) {
        if (ttmp->ttyp == MAGIC_PORTAL || ttmp->ttyp == VIBRATING_SQUARE)
            return (struct trap *) 0;
        oldplace = TRUE;
        if (u.utrap && x == u.ux && y == u.uy
            && ((u.utraptype == TT_BEARTRAP && typ != BEAR_TRAP)
                || (u.utraptype == TT_WEB && typ != WEB)
                || (u.utraptype == TT_PIT && typ != PIT
                    && typ != SPIKED_PIT)))
            u.utrap = 0;
        /* old <tx,ty> remain valid */
    } else if (IS_FURNITURE(levl[x][y].typ)) {
        /* no trap on top of furniture (caller usually screens the
           location to inhibit this, but wizard mode wishing doesn't) */
        return (struct trap *) 0;
    } else {
        oldplace = FALSE;
        ttmp = newtrap();
        ttmp->ntrap = 0;
        ttmp->tx = x;
        ttmp->ty = y;
    }
    /* [re-]initialize all fields except ntrap (handled below) and <tx,ty> */
    ttmp->vl = zero_vl;
    ttmp->launch.x = ttmp->launch.y = -1; /* force error if used before set */
    ttmp->dst.dnum = ttmp->dst.dlevel = -1;
    ttmp->madeby_u = 0;
    ttmp->once = 0;
    ttmp->tseen = (typ == HOLE); /* hide non-holes */
    ttmp->ttyp = typ;

    switch (typ) {
    case SQKY_BOARD: {
        int tavail[12], tpick[12], tcnt = 0, k;
        struct trap *t;

        for (k = 0; k < 12; ++k)
            tavail[k] = tpick[k] = 0;
        for (t = ftrap; t; t = t->ntrap)
            if (t->ttyp == SQKY_BOARD && t != ttmp)
                tavail[t->tnote] = 1;
        /* now populate tpick[] with the available indices */
        for (k = 0; k < 12; ++k)
            if (tavail[k] == 0)
                tpick[tcnt++] = k;
        /* choose an unused note; if all are in use, pick a random one */
        ttmp->tnote = (short) ((tcnt > 0) ? tpick[rn2(tcnt)] : rn2(12));
        break;
    }
    case STATUE_TRAP: { /* create a "living" statue */
        struct monst *mtmp;
        struct obj *otmp, *statue;
        struct permonst *mptr;
        int trycount = 10;

        do { /* avoid ultimately hostile co-aligned unicorn */
            mptr = &mons[rndmonnum()];
        } while (--trycount > 0 && is_unicorn(mptr)
                 && sgn(u.ualign.type) == sgn(mptr->maligntyp));
        statue = mkcorpstat(STATUE, (struct monst *) 0, mptr, x, y,
                            CORPSTAT_NONE);
        mtmp = makemon(&mons[statue->corpsenm], 0, 0, MM_NOCOUNTBIRTH);
        if (!mtmp)
            break; /* should never happen */
        while (mtmp->minvent) {
            otmp = mtmp->minvent;
            otmp->owornmask = 0;
            obj_extract_self(otmp);
            (void) add_to_container(statue, otmp);
        }
        statue->owt = weight(statue);
        mongone(mtmp);
        break;
    }
    case ROLLING_BOULDER_TRAP: /* boulder will roll towards trigger */
        (void) mkroll_launch(ttmp, x, y, BOULDER, 1L);
        break;
    case PIT:
    case SPIKED_PIT:
        ttmp->conjoined = 0;
        /*FALLTHRU*/
    case HOLE:
    case TRAPDOOR:
        lev = &levl[x][y];
        if (*in_rooms(x, y, SHOPBASE)
            && (typ == HOLE || typ == TRAPDOOR
                || IS_DOOR(lev->typ) || IS_WALL(lev->typ)))
            add_damage(x, y, /* schedule repair */
                       ((IS_DOOR(lev->typ) || IS_WALL(lev->typ))
                        && !context.mon_moving)
                           ? 200L
                           : 0L);
        lev->doormask = 0;     /* subsumes altarmask, icedpool... */
        if (IS_ROOM(lev->typ)) /* && !IS_AIR(lev->typ) */
            lev->typ = ROOM;
        /*
         * some cases which can happen when digging
         * down while phazing thru solid areas
         */
        else if (lev->typ == STONE || lev->typ == SCORR)
            lev->typ = CORR;
        else if (IS_WALL(lev->typ) || lev->typ == SDOOR)
            lev->typ = level.flags.is_maze_lev
                           ? ROOM
                           : level.flags.is_cavernous_lev ? CORR : DOOR;

        unearth_objs(x, y);
        break;
    }

    if (!oldplace) {
        ttmp->ntrap = ftrap;
        ftrap = ttmp;
    } else {
        /* oldplace;
           it shouldn't be possible to override a sokoban pit or hole
           with some other trap, but we'll check just to be safe */
        if (Sokoban)
            maybe_finish_sokoban();
    }
    return ttmp;
}

void
fall_through(td)
boolean td; /* td == TRUE : trap door or hole */
{
    d_level dtmp;
    char msgbuf[BUFSZ];
    const char *dont_fall = 0;
    int newlevel, bottom;

    /* we'll fall even while levitating in Sokoban; otherwise, if we
       won't fall and won't be told that we aren't falling, give up now */
    if (Blind && Levitation && !Sokoban)
        return;

    bottom = dunlevs_in_dungeon(&u.uz);
    /* when in the upper half of the quest, don't fall past the
       middle "quest locate" level if hero hasn't been there yet */
    if (In_quest(&u.uz)) {
        int qlocate_depth = qlocate_level.dlevel;

        /* deepest reached < qlocate implies current < qlocate */
        if (dunlev_reached(&u.uz) < qlocate_depth)
            bottom = qlocate_depth; /* early cut-off */
    }
    newlevel = dunlev(&u.uz); /* current level */
    do {
        newlevel++;
    } while (!rn2(4) && newlevel < bottom);

    if (td) {
        struct trap *t = t_at(u.ux, u.uy);

        feeltrap(t);
        if (!Sokoban) {
            if (t->ttyp == TRAPDOOR)
                pline("一个陷阱门在你的下方打开了!");
            else
                pline("你的下方有一个大洞!");
        }
    } else
        pline_The("%s在你的下方打开了!", surface(u.ux, u.uy));

    if (Sokoban && Can_fall_thru(&u.uz))
        ; /* KMH -- You can't escape the Sokoban level traps */
    else if (Levitation || u.ustuck
             || (!Can_fall_thru(&u.uz) && !levl[u.ux][u.uy].candig) || Flying
             || is_clinger(youmonst.data)
             || (Inhell && !u.uevent.invoked && newlevel == bottom)) {
        dont_fall = "没有掉进去.";
    } else if (youmonst.data->msize >= MZ_HUGE) {
        dont_fall = "没有顺利通过.";
    } else if (!next_to_u()) {
        dont_fall = "被你的宠物猛拉回来!";
    }
    if (dont_fall) {
        You1(dont_fall);
        /* hero didn't fall through, but any objects here might */
        impact_drop((struct obj *) 0, u.ux, u.uy, 0);
        if (!td) {
            display_nhwindow(WIN_MESSAGE, FALSE);
            pline_The("你下方的开口关闭了.");
        }
        return;
    }

    if (*u.ushops)
        shopdig(1);
    if (Is_stronghold(&u.uz)) {
        find_hell(&dtmp);
    } else {
        int dist = newlevel - dunlev(&u.uz);
        dtmp.dnum = u.uz.dnum;
        dtmp.dlevel = newlevel;
        if (dist > 1)
            You("落进一个%s%s井!", dist > 3 ? "非常" : "",
                dist > 2 ? "深的" : "");
    }
    if (!td)
        Sprintf(msgbuf, "你上方%s的洞关闭了.",
                ceiling(u.ux, u.uy));

    schedule_goto(&dtmp, FALSE, TRUE, 0, (char *) 0,
                  !td ? msgbuf : (char *) 0);
}

/*
 * Animate the given statue.  May have been via shatter attempt, trap,
 * or stone to flesh spell.  Return a monster if successfully animated.
 * If the monster is animated, the object is deleted.  If fail_reason
 * is non-null, then fill in the reason for failure (or success).
 *
 * The cause of animation is:
 *
 *      ANIMATE_NORMAL  - hero "finds" the monster
 *      ANIMATE_SHATTER - hero tries to destroy the statue
 *      ANIMATE_SPELL   - stone to flesh spell hits the statue
 *
 * Perhaps x, y is not needed if we can use get_obj_location() to find
 * the statue's location... ???
 *
 * Sequencing matters:
 *      create monster; if it fails, give up with statue intact;
 *      give "statue comes to life" message;
 *      if statue belongs to shop, have shk give "you owe" message;
 *      transfer statue contents to monster (after stolen_value());
 *      delete statue.
 *      [This ordering means that if the statue ends up wearing a cloak of
 *       invisibility or a mummy wrapping, the visibility checks might be
 *       wrong, but to avoid that we'd have to clone the statue contents
 *       first in order to give them to the monster before checking their
 *       shop status--it's not worth the hassle.]
 */
struct monst *
animate_statue(statue, x, y, cause, fail_reason)
struct obj *statue;
xchar x, y;
int cause;
int *fail_reason;
{
    int mnum = statue->corpsenm;
    struct permonst *mptr = &mons[mnum];
    struct monst *mon = 0, *shkp;
    struct obj *item;
    coord cc;
    boolean historic = (Role_if(PM_ARCHEOLOGIST)
                        && (statue->spe & STATUE_HISTORIC) != 0),
            golem_xform = FALSE, use_saved_traits;
    const char *comes_to_life;
    char statuename[BUFSZ], tmpbuf[BUFSZ];
    static const char historic_statue_is_gone[] =
        "历史性的雕像现在已经不存在了";

    if (cant_revive(&mnum, TRUE, statue)) {
        /* mnum has changed; we won't be animating this statue as itself */
        if (mnum != PM_DOPPELGANGER)
            mptr = &mons[mnum];
        use_saved_traits = FALSE;
    } else if (is_golem(mptr) && cause == ANIMATE_SPELL) {
        /* statue of any golem hit by stone-to-flesh becomes flesh golem */
        golem_xform = (mptr != &mons[PM_FLESH_GOLEM]);
        mnum = PM_FLESH_GOLEM;
        mptr = &mons[PM_FLESH_GOLEM];
        use_saved_traits = (has_omonst(statue) && !golem_xform);
    } else {
        use_saved_traits = has_omonst(statue);
    }

    if (use_saved_traits) {
        /* restore a petrified monster */
        cc.x = x, cc.y = y;
        mon = montraits(statue, &cc);
        if (mon && mon->mtame && !mon->isminion)
            wary_dog(mon, TRUE);
    } else {
        /* statues of unique monsters from bones or wishing end
           up here (cant_revive() sets mnum to be doppelganger;
           mptr reflects the original form for use by newcham()) */
        if ((mnum == PM_DOPPELGANGER && mptr != &mons[PM_DOPPELGANGER])
            /* block quest guards from other roles */
            || (mptr->msound == MS_GUARDIAN
                && quest_info(MS_GUARDIAN) != mnum)) {
            mon = makemon(&mons[PM_DOPPELGANGER], x, y,
                          NO_MINVENT | MM_NOCOUNTBIRTH | MM_ADJACENTOK);
            /* if hero has protection from shape changers, cham field will
               be NON_PM; otherwise, set form to match the statue */
            if (mon && mon->cham >= LOW_PM)
                (void) newcham(mon, mptr, FALSE, FALSE);
        } else
            mon = makemon(mptr, x, y, (cause == ANIMATE_SPELL)
                                          ? (NO_MINVENT | MM_ADJACENTOK)
                                          : NO_MINVENT);
    }

    if (!mon) {
        if (fail_reason)
            *fail_reason = unique_corpstat(&mons[statue->corpsenm])
                               ? AS_MON_IS_UNIQUE
                               : AS_NO_MON;
        return (struct monst *) 0;
    }

    /* a non-montraits() statue might specify gender */
    if (statue->spe & STATUE_MALE)
        mon->female = FALSE;
    else if (statue->spe & STATUE_FEMALE)
        mon->female = TRUE;
    /* if statue has been named, give same name to the monster */
    if (has_oname(statue))
        mon = christen_monst(mon, ONAME(statue));
    /* mimic statue becomes seen mimic; other hiders won't be hidden */
    if (mon->m_ap_type)
        seemimic(mon);
    else
        mon->mundetected = FALSE;
    mon->msleeping = 0;
    if (cause == ANIMATE_NORMAL || cause == ANIMATE_SHATTER) {
        /* trap always releases hostile monster */
        mon->mtame = 0; /* (might be petrified pet tossed onto trap) */
        mon->mpeaceful = 0;
        set_malign(mon);
    }

    comes_to_life = !canspotmon(mon)
                        ? "消失了"
                        : golem_xform
                              ? "变成了肉"
                              : (nonliving(mon->data) || is_vampshifter(mon))
                                    ? "移动了"
                                    : "活了过来";
    if ((x == u.ux && y == u.uy) || cause == ANIMATE_SPELL) {
        /* "the|your|Manlobbi's statue [of a wombat]" */
        shkp = shop_keeper(*in_rooms(mon->mx, mon->my, SHOPBASE));
        Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                (cause == ANIMATE_SPELL
                 /* avoid "of a shopkeeper" if it's Manlobbi himself
                    (if carried, it can't be unpaid--hence won't be
                    described as "Manlobbi's statue"--because there
                    wasn't any living shk when statue was picked up) */
                 && (mon != shkp || carried(statue)))
                   ? xname(statue)
                   : "雕像");
        pline("%s %s!", upstart(statuename), comes_to_life);
    } else if (Hallucination) { /* They don't know it's a statue */
        pline_The("%s突然似乎更有生气的.", rndmonnam((char *) 0));
    } else if (cause == ANIMATE_SHATTER) {
        if (cansee(x, y))
            Sprintf(statuename, "%s%s", shk_your(tmpbuf, statue),
                    xname(statue));
        else
            Strcpy(statuename, "一个雕像");
        pline("没有粉碎, 相反%s突然%s!", statuename,
              comes_to_life);
    } else { /* cause == ANIMATE_NORMAL */
        You("发现%s摆着雕像的姿势.",
            canspotmon(mon) ? a_monnam(mon) : something);
        if (!canspotmon(mon) && Blind)
            map_invisible(x, y);
        stop_occupation();
    }

    /* if this isn't caused by a monster using a wand of striking,
       there might be consequences for the hero */
    if (!context.mon_moving) {
        /* if statue is owned by a shop, hero will have to pay for it;
           stolen_value gives a message (about debt or use of credit)
           which refers to "it" so needs to follow a message describing
           the object ("the statue comes to life" one above) */
        if (cause != ANIMATE_NORMAL && costly_spot(x, y)
            && (shkp = shop_keeper(*in_rooms(x, y, SHOPBASE))) != 0
            /* avoid charging for Manlobbi's statue of Manlobbi
               if stone-to-flesh is used on petrified shopkeep */
            && mon != shkp)
            (void) stolen_value(statue, x, y, (boolean) shkp->mpeaceful,
                                FALSE);

        if (historic) {
            You_feel("有罪的%s.", historic_statue_is_gone);
            adjalign(-1);
        }
    } else {
        if (historic && cansee(x, y))
            You_feel("后悔的%s.", historic_statue_is_gone);
        /* no alignment penalty */
    }

    /* transfer any statue contents to monster's inventory */
    while ((item = statue->cobj) != 0) {
        obj_extract_self(item);
        (void) mpickobj(mon, item);
    }
    m_dowear(mon, TRUE);
    /* in case statue is wielded and hero zaps stone-to-flesh at self */
    if (statue->owornmask)
        remove_worn_item(statue, TRUE);
    /* statue no longer exists */
    delobj(statue);

    /* avoid hiding under nothing */
    if (x == u.ux && y == u.uy && Upolyd && hides_under(youmonst.data)
        && !OBJ_AT(x, y))
        u.uundetected = 0;

    if (fail_reason)
        *fail_reason = AS_OK;
    return mon;
}

/*
 * You've either stepped onto a statue trap's location or you've triggered a
 * statue trap by searching next to it or by trying to break it with a wand
 * or pick-axe.
 */
struct monst *
activate_statue_trap(trap, x, y, shatter)
struct trap *trap;
xchar x, y;
boolean shatter;
{
    struct monst *mtmp = (struct monst *) 0;
    struct obj *otmp = sobj_at(STATUE, x, y);
    int fail_reason;

    /*
     * Try to animate the first valid statue.  Stop the loop when we
     * actually create something or the failure cause is not because
     * the mon was unique.
     */
    deltrap(trap);
    while (otmp) {
        mtmp = animate_statue(otmp, x, y,
                              shatter ? ANIMATE_SHATTER : ANIMATE_NORMAL,
                              &fail_reason);
        if (mtmp || fail_reason != AS_MON_IS_UNIQUE)
            break;

        otmp = nxtobj(otmp, STATUE, TRUE);
    }

    feel_newsym(x, y);
    return mtmp;
}

STATIC_OVL boolean
keep_saddle_with_steedcorpse(steed_mid, objchn, saddle)
unsigned steed_mid;
struct obj *objchn, *saddle;
{
    if (!saddle)
        return FALSE;
    while (objchn) {
        if (objchn->otyp == CORPSE && has_omonst(objchn)) {
            struct monst *mtmp = OMONST(objchn);

            if (mtmp->m_id == steed_mid) {
                /* move saddle */
                xchar x, y;
                if (get_obj_location(objchn, &x, &y, 0)) {
                    obj_extract_self(saddle);
                    place_object(saddle, x, y);
                    stackobj(saddle);
                }
                return TRUE;
            }
        }
        if (Has_contents(objchn)
            && keep_saddle_with_steedcorpse(steed_mid, objchn->cobj, saddle))
            return TRUE;
        objchn = objchn->nobj;
    }
    return FALSE;
}

void
dotrap(trap, trflags)
register struct trap *trap;
unsigned trflags;
{
    register int ttype = trap->ttyp;
    register struct obj *otmp;
    boolean already_seen = trap->tseen,
            forcetrap = (trflags & FORCETRAP) != 0,
            webmsgok = (trflags & NOWEBMSG) == 0,
            forcebungle = (trflags & FORCEBUNGLE) != 0,
            plunged = (trflags & TOOKPLUNGE) != 0,
            adj_pit = conjoined_pits(trap, t_at(u.ux0, u.uy0), TRUE);
    int oldumort;
    int steed_article = ARTICLE_THE;

    nomul(0);

    /* KMH -- You can't escape the Sokoban level traps */
    if (Sokoban && (ttype == PIT || ttype == SPIKED_PIT
                    || ttype == HOLE || ttype == TRAPDOOR)) {
        /* The "air currents" message is still appropriate -- even when
         * the hero isn't flying or levitating -- because it conveys the
         * reason why the player cannot escape the trap with a dexterity
         * check, clinging to the ceiling, etc.
         */
        pline("气流把你拉进%s %s!",
              a_your[trap->madeby_u],
              defsyms[trap_to_defsym(ttype)].explanation);
        /* then proceed to normal trap effect */
    } else if (already_seen && !forcetrap) {
        if ((Levitation || (Flying && !plunged))
            && (ttype == PIT || ttype == SPIKED_PIT || ttype == HOLE
                || ttype == BEAR_TRAP)) {
            You("%s在%s%s上.", Levitation ? "飘浮" : "飞",
                a_your[trap->madeby_u],
                defsyms[trap_to_defsym(ttype)].explanation);
            return;
        }
        if (!Fumbling && ttype != MAGIC_PORTAL && ttype != VIBRATING_SQUARE
            && ttype != ANTI_MAGIC && !forcebungle && !plunged && !adj_pit
            && (!rn2(5) || ((ttype == PIT || ttype == SPIKED_PIT)
                            && is_clinger(youmonst.data)))) {
            You("逃脱了%s %s.", (ttype == ARROW_TRAP && !trap->madeby_u)
                                     ? ""
                                     : a_your[trap->madeby_u],
                defsyms[trap_to_defsym(ttype)].explanation);
            return;
        }
    }

    if (u.usteed) {
        u.usteed->mtrapseen |= (1 << (ttype - 1));
        /* suppress article in various steed messages when using its
           name (which won't occur when hallucinating) */
        if (has_mname(u.usteed) && !Hallucination)
            steed_article = ARTICLE_NONE;
    }

    switch (ttype) {
    case ARROW_TRAP:
        if (trap->once && trap->tseen && !rn2(15)) {
            You_hear("响亮的咔哒声!");
            deltrap(trap);
            newsym(u.ux, u.uy);
            break;
        }
        trap->once = 1;
        seetrap(trap);
        pline("一枝箭射向你!");
        otmp = mksobj(ARROW, TRUE, FALSE);
        otmp->quan = 1L;
        otmp->owt = weight(otmp);
        otmp->opoisoned = 0;
        if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) { /* nothing */
            ;
        } else if (thitu(8, dmgval(otmp, &youmonst), otmp, "箭")) {
            obfree(otmp, (struct obj *) 0);
        } else {
            place_object(otmp, u.ux, u.uy);
            if (!Blind)
                otmp->dknown = 1;
            stackobj(otmp);
            newsym(u.ux, u.uy);
        }
        break;

    case DART_TRAP:
        if (trap->once && trap->tseen && !rn2(15)) {
            You_hear("温和的咔哒声.");
            deltrap(trap);
            newsym(u.ux, u.uy);
            break;
        }
        trap->once = 1;
        seetrap(trap);
        pline("一个小飞镖射向你!");
        otmp = mksobj(DART, TRUE, FALSE);
        otmp->quan = 1L;
        otmp->owt = weight(otmp);
        if (!rn2(6))
            otmp->opoisoned = 1;
        oldumort = u.umortality;
        if (u.usteed && !rn2(2) && steedintrap(trap, otmp)) { /* nothing */
            ;
        } else if (thitu(7, dmgval(otmp, &youmonst), otmp, "小飞镖")) {
            if (otmp->opoisoned)
                poisoned("飞镖", A_CON, "小飞镖",
                         /* if damage triggered life-saving,
                            poison is limited to attrib loss */
                         (u.umortality > oldumort) ? 0 : 10, TRUE);
            obfree(otmp, (struct obj *) 0);
        } else {
            place_object(otmp, u.ux, u.uy);
            if (!Blind)
                otmp->dknown = 1;
            stackobj(otmp);
            newsym(u.ux, u.uy);
        }
        break;

    case ROCKTRAP:
        if (trap->once && trap->tseen && !rn2(15)) {
            pline("%s的陷阱门打开了, 但是没有东西掉落下来!",
                  the(ceiling(u.ux, u.uy)));
            deltrap(trap);
            newsym(u.ux, u.uy);
        } else {
            int dmg = d(2, 6); /* should be std ROCK dmg? */

            trap->once = 1;
            feeltrap(trap);
            otmp = mksobj_at(ROCK, u.ux, u.uy, TRUE, FALSE);
            otmp->quan = 1L;
            otmp->owt = weight(otmp);

            pline("%s的陷阱门打开了%s落到了你的%s上!",
                  the(ceiling(u.ux, u.uy)), xname(otmp), body_part(HEAD));

            if (uarmh) {
                if (is_metallic(uarmh)) {
                    pline("幸运的是, 你穿戴着一个坚硬的头盔.");
                    dmg = 2;
                } else if (flags.verbose) {
                    pline("%s没有保护到你.", Yname2(uarmh));
                }
            }

            if (!Blind)
                otmp->dknown = 1;
            stackobj(otmp);
            newsym(u.ux, u.uy); /* map the rock */

            losehp(Maybe_Half_Phys(dmg), "掉落的岩石", KILLED_BY_AN);
            exercise(A_STR, FALSE);
        }
        break;

    case SQKY_BOARD: /* stepped on a squeaky board */
        if ((Levitation || Flying) && !forcetrap) {
            if (!Blind) {
                seetrap(trap);
                if (Hallucination)
                    You("注意到油布的折痕.");
                else
                    You("注意到你下面松动的板.");
            }
        } else {
            seetrap(trap);
            pline("你下方的板在%s%s%s.",
                  Deaf ? "" : "大声地",
                  Deaf ? "振动" : "吱吱响着",
                  Deaf ? "" : trapnote(trap, 0));
            wake_nearby();
        }
        break;

    case BEAR_TRAP: {
        int dmg = d(2, 4);

        if ((Levitation || Flying) && !forcetrap)
            break;
        feeltrap(trap);
        if (amorphous(youmonst.data) || is_whirly(youmonst.data)
            || unsolid(youmonst.data)) {
            pline("%s捕兽夹无害地关闭穿过了你.",
                  A_Your[trap->madeby_u]);
            break;
        }
        if (!u.usteed && youmonst.data->msize <= MZ_SMALL) {
            pline("%s捕兽夹在你上面无害地关闭.",
                  A_Your[trap->madeby_u]);
            break;
        }
        u.utrap = rn1(4, 4);
        u.utraptype = TT_BEARTRAP;
        if (u.usteed) {
            pline("%s捕兽夹夹住了%s %s!", A_Your[trap->madeby_u],
                  s_suffix(mon_nam(u.usteed)), mbodypart(u.usteed, FOOT));
            if (thitm(0, u.usteed, (struct obj *) 0, dmg, FALSE))
                u.utrap = 0; /* steed died, hero not trapped */
        } else {
            pline("%s捕兽夹夹住了你的%s!", A_Your[trap->madeby_u],
                  body_part(FOOT));
            set_wounded_legs(rn2(2) ? RIGHT_SIDE : LEFT_SIDE, rn1(10, 10));
            if (u.umonnum == PM_OWLBEAR || u.umonnum == PM_BUGBEAR)
                You("愤怒地嚎叫!");
            losehp(Maybe_Half_Phys(dmg), "捕兽夹", KILLED_BY_AN);
        }
        exercise(A_DEX, FALSE);
        break;
    }

    case SLP_GAS_TRAP:
        seetrap(trap);
        if (Sleep_resistance || breathless(youmonst.data)) {
            You("被一团气体笼罩!");
        } else {
            pline("一团气体使你陷入沉睡!");
            fall_asleep(-rnd(25), TRUE);
        }
        (void) steedintrap(trap, (struct obj *) 0);
        break;

    case RUST_TRAP:
        seetrap(trap);

        /* Unlike monsters, traps cannot aim their rust attacks at
         * you, so instead of looping through and taking either the
         * first rustable one or the body, we take whatever we get,
         * even if it is not rustable.
         */
        switch (rn2(5)) {
        case 0:
            pline("%s 你的%s!", A_gush_of_water_hits, body_part(HEAD));
            (void) water_damage(uarmh, helm_simple_name(uarmh), TRUE);
            break;
        case 1:
            pline("%s你的左%s!", A_gush_of_water_hits, body_part(ARM));
            if (water_damage(uarms, "盾牌", TRUE) != ER_NOTHING)
                break;
            if (u.twoweap || (uwep && bimanual(uwep)))
                (void) water_damage(u.twoweap ? uswapwep : uwep, 0, TRUE);
        glovecheck:
            (void) water_damage(uarmg, "手套", TRUE);
            /* Not "metal gauntlets" since it gets called
             * even if it's leather for the message
             */
            break;
        case 2:
            pline("%s 你的右%s!", A_gush_of_water_hits, body_part(ARM));
            (void) water_damage(uwep, 0, TRUE);
            goto glovecheck;
        default:
            pline("%s你!", A_gush_of_water_hits);
            for (otmp = invent; otmp; otmp = otmp->nobj)
                if (otmp->lamplit && otmp != uwep
                    && (otmp != uswapwep || !u.twoweap))
                    (void) snuff_lit(otmp);
            if (uarmc)
                (void) water_damage(uarmc, cloak_simple_name(uarmc), TRUE);
            else if (uarm)
                (void) water_damage(uarm, "盔甲", TRUE);
            else if (uarmu)
                (void) water_damage(uarmu, "衬衫", TRUE);
        }
        update_inventory();

        if (u.umonnum == PM_IRON_GOLEM) {
            int dam = u.mhmax;

            pline("%s 你!", A_gush_of_water_hits);
            You("被锈覆盖了!");
            losehp(Maybe_Half_Phys(dam), "锈掉", KILLED_BY);
        } else if (u.umonnum == PM_GREMLIN && rn2(3)) {
            pline("%s 你!", A_gush_of_water_hits);
            (void) split_mon(&youmonst, (struct monst *) 0);
        }

        break;

    case FIRE_TRAP:
        seetrap(trap);
        dofiretrap((struct obj *) 0);
        break;

    case PIT:
    case SPIKED_PIT:
        /* KMH -- You can't escape the Sokoban level traps */
        if (!Sokoban && (Levitation || (Flying && !plunged)))
            break;
        feeltrap(trap);
        if (!Sokoban && is_clinger(youmonst.data) && !plunged) {
            if (trap->tseen) {
                You_see("%s %s坑在你下方.", a_your[trap->madeby_u],
                        ttype == SPIKED_PIT ? "有刺的 " : "");
            } else {
                pline("%s%s坑在你下方打开了!", A_Your[trap->madeby_u],
                      ttype == SPIKED_PIT ? "满是钉子的" : "");
                You("没有掉进去!");
            }
            break;
        }
        if (!Sokoban) {
            char verbbuf[BUFSZ];

            if (u.usteed) {
                if ((trflags & RECURSIVETRAP) != 0)
                    Sprintf(verbbuf, "和%s掉落",
                            x_monnam(u.usteed, steed_article, (char *) 0,
                                     SUPPRESS_SADDLE, FALSE));
                else
                    Sprintf(verbbuf, "领着%s",
                            x_monnam(u.usteed, steed_article, "可怜的",
                                     SUPPRESS_SADDLE, FALSE));
            } else if (adj_pit) {
                You("走进了邻近的坑.");
            } else {
                Strcpy(verbbuf,
                       !plunged ? "掉落" : (Flying ? "俯冲" : "跳"));
                You("%s进%s坑!", verbbuf, a_your[trap->madeby_u]);
            }
        }
        /* wumpus reference */
        if (Role_if(PM_RANGER) && !trap->madeby_u && !trap->once
            && In_quest(&u.uz) && Is_qlocate(&u.uz)) {
            pline("幸运的是毕竟它有一个底部...");
            trap->once = 1;
        } else if (u.umonnum == PM_PIT_VIPER || u.umonnum == PM_PIT_FIEND) {
            pline("多可怜啊.  那不是坑?");
        }
        if (ttype == SPIKED_PIT) {
            const char *predicament = "在一组锋利的铁钉上";

            if (u.usteed) {
                pline("%s %s %s!",
                      upstart(x_monnam(u.usteed, steed_article, "可怜的",
                                       SUPPRESS_SADDLE, FALSE)),
                      adj_pit ? "踩" : "落", predicament);
            } else
                You("%s %s!", adj_pit ? "踩" : "落", predicament);
        }
        u.utrap = rn1(6, 2);
        u.utraptype = TT_PIT;
        if (!steedintrap(trap, (struct obj *) 0)) {
            if (ttype == SPIKED_PIT) {
                oldumort = u.umortality;
                losehp(Maybe_Half_Phys(rnd(adj_pit ? 6 : 10)),
                       plunged
                           ? "故意陷入一个有铁钉的坑"
                           : adj_pit ? "走进了一个有铁钉的坑"
                                     : "掉进了一个有铁钉的坑",
                       NO_KILLER_PREFIX);
                if (!rn2(6))
                    poisoned("钉子", A_STR,
                             adj_pit ? "踩在有毒的钉子上"
                                     : "掉在有毒的钉子上",
                             /* if damage triggered life-saving,
                                poison is limited to attrib loss */
                             (u.umortality > oldumort) ? 0 : 8, FALSE);
            } else {
                /* plunging flyers take spike damage but not pit damage */
                if (!adj_pit
                    && !(plunged && (Flying || is_clinger(youmonst.data))))
                    losehp(Maybe_Half_Phys(rnd(6)),
                           plunged ? "故意陷入一个坑"
                                   : "掉进一个坑",
                           NO_KILLER_PREFIX);
            }
            if (Punished && !carried(uball)) {
                unplacebc();
                ballfall();
                placebc();
            }
            if (!adj_pit)
                selftouch("落下, 你");
            vision_full_recalc = 1; /* vision limits change */
            exercise(A_STR, FALSE);
            exercise(A_DEX, FALSE);
        }
        break;

    case HOLE:
    case TRAPDOOR:
        if (!Can_fall_thru(&u.uz)) {
            seetrap(trap); /* normally done in fall_through */
            impossible("dotrap: %ss cannot exist on this level.",
                       defsyms[trap_to_defsym(ttype)].explanation);
            break; /* don't activate it after all */
        }
        fall_through(TRUE);
        break;

    case TELEP_TRAP:
        seetrap(trap);
        tele_trap(trap);
        break;

    case LEVEL_TELEP:
        seetrap(trap);
        level_tele_trap(trap);
        break;

    case WEB: /* Our luckless player has stumbled into a web. */
        feeltrap(trap);
        if (amorphous(youmonst.data) || is_whirly(youmonst.data)
            || unsolid(youmonst.data)) {
            if (acidic(youmonst.data) || u.umonnum == PM_GELATINOUS_CUBE
                || u.umonnum == PM_FIRE_ELEMENTAL) {
                if (webmsgok)
                    You("%s了%s蜘蛛网!",
                        (u.umonnum == PM_FIRE_ELEMENTAL) ? "烧毁"
                                                         : "溶解",
                        a_your[trap->madeby_u]);
                deltrap(trap);
                newsym(u.ux, u.uy);
                break;
            }
            if (webmsgok)
                You("流过%s蜘蛛网.", a_your[trap->madeby_u]);
            break;
        }
        if (webmaker(youmonst.data)) {
            if (webmsgok)
                pline(trap->madeby_u ? "你在你的网上散步."
                                     : "这里有一张蜘蛛网.");
            break;
        }
        if (webmsgok) {
            char verbbuf[BUFSZ];

            if (forcetrap) {
                Strcpy(verbbuf, "陷入");
            } else if (u.usteed) {
                Sprintf(verbbuf, "领着%s 进",
                        x_monnam(u.usteed, steed_article, "可怜的",
                                 SUPPRESS_SADDLE, FALSE));
            } else {
                Sprintf(verbbuf, "%s 进",
                        Levitation ? (const char *) "飘"
                                   : locomotion(youmonst.data, "蹒跚"));
            }
            You("%s %s了蜘蛛网!", verbbuf, a_your[trap->madeby_u]);
        }
        u.utraptype = TT_WEB;

        /* Time stuck in the web depends on your/steed strength. */
        {
            register int str = ACURR(A_STR);

            /* If mounted, the steed gets trapped.  Use mintrap
             * to do all the work.  If mtrapped is set as a result,
             * unset it and set utrap instead.  In the case of a
             * strongmonst and mintrap said it's trapped, use a
             * short but non-zero trap time.  Otherwise, monsters
             * have no specific strength, so use player strength.
             * This gets skipped for webmsgok, which implies that
             * the steed isn't a factor.
             */
            if (u.usteed && webmsgok) {
                /* mtmp location might not be up to date */
                u.usteed->mx = u.ux;
                u.usteed->my = u.uy;

                /* mintrap currently does not return 2(died) for webs */
                if (mintrap(u.usteed)) {
                    u.usteed->mtrapped = 0;
                    if (strongmonst(u.usteed->data))
                        str = 17;
                } else {
                    break;
                }

                webmsgok = FALSE; /* mintrap printed the messages */
            }
            if (str <= 3)
                u.utrap = rn1(6, 6);
            else if (str < 6)
                u.utrap = rn1(6, 4);
            else if (str < 9)
                u.utrap = rn1(4, 4);
            else if (str < 12)
                u.utrap = rn1(4, 2);
            else if (str < 15)
                u.utrap = rn1(2, 2);
            else if (str < 18)
                u.utrap = rnd(2);
            else if (str < 69)
                u.utrap = 1;
            else {
                u.utrap = 0;
                if (webmsgok)
                    You("撕扯穿了%s网!", a_your[trap->madeby_u]);
                deltrap(trap);
                newsym(u.ux, u.uy); /* get rid of trap symbol */
            }
        }
        break;

    case STATUE_TRAP:
        (void) activate_statue_trap(trap, u.ux, u.uy, FALSE);
        break;

    case MAGIC_TRAP: /* A magic trap. */
        seetrap(trap);
        if (!rn2(30)) {
            deltrap(trap);
            newsym(u.ux, u.uy); /* update position */
            You("被卷入了魔力的爆炸!");
            losehp(rnd(10), "魔力爆炸", KILLED_BY_AN);
            Your("身体吸收了一些魔法能量!");
            u.uen = (u.uenmax += 2);
            break;
        } else {
            domagictrap();
        }
        (void) steedintrap(trap, (struct obj *) 0);
        break;

    case ANTI_MAGIC:
        seetrap(trap);
        /* hero without magic resistance loses spell energy,
           hero with magic resistance takes damage instead;
           possibly non-intuitive but useful for play balance */
        if (!Antimagic) {
            drain_en(rnd(u.ulevel) + 1);
        } else {
            int dmgval2 = rnd(4), hp = Upolyd ? u.mh : u.uhp;

            /* Half_XXX_damage has opposite its usual effect (approx)
               but isn't cumulative if hero has more than one */
            if (Half_physical_damage || Half_spell_damage)
                dmgval2 += rnd(4);
            /* give Magicbane wielder dose of own medicine */
            if (uwep && uwep->oartifact == ART_MAGICBANE)
                dmgval2 += rnd(4);
            /* having an artifact--other than own quest one--which
               confers magic resistance simply by being carried
               also increases the effect */
            for (otmp = invent; otmp; otmp = otmp->nobj)
                if (otmp->oartifact && !is_quest_artifact(otmp)
                    && defends_when_carried(AD_MAGM, otmp))
                    break;
            if (otmp)
                dmgval2 += rnd(4);
            if (Passes_walls)
                dmgval2 = (dmgval2 + 3) / 4;

            You_feel((dmgval2 >= hp) ? "难以忍受的迟钝的!"
                                     : (dmgval2 >= hp / 4) ? "无生气的."
                                                           : "懒怠的.");
            /* opposite of magical explosion */
            losehp(dmgval2, "反魔法内爆", KILLED_BY_AN);
        }
        break;

    case POLY_TRAP: {
        char verbbuf[BUFSZ];

        seetrap(trap);
        if (u.usteed)
            Sprintf(verbbuf, "领着%s",
                    x_monnam(u.usteed, steed_article, (char *) 0,
                             SUPPRESS_SADDLE, FALSE));
        else
            Sprintf(verbbuf, "%s", Levitation
                                       ? (const char *) "飘"
                                       : locomotion(youmonst.data, "踩"));
        You("%s到一个变形陷阱!", verbbuf);
        if (Antimagic || Unchanging) {
            shieldeff(u.ux, u.uy);
            You_feel("暂时地不同.");
            /* Trap did nothing; don't remove it --KAA */
        } else {
            (void) steedintrap(trap, (struct obj *) 0);
            deltrap(trap);      /* delete trap before polymorph */
            newsym(u.ux, u.uy); /* get rid of trap symbol */
            You_feel("你改变了.");
            polyself(0);
        }
        break;
    }
    case LANDMINE: {
        unsigned steed_mid = 0;
        struct obj *saddle = 0;

        if ((Levitation || Flying) && !forcetrap) {
            if (!already_seen && rn2(3))
                break;
            feeltrap(trap);
            pline("%s你下面有%s在一堆土里.",
                  already_seen ? "" : "你发现",
                  trap->madeby_u ? "你的地雷的触发器" : "一个触发器");
            if (already_seen && rn2(3))
                break;
            pline("KAABLAMM!!!  %s %s%s!",
                  forcebungle ? "你的笨拙的尝试引爆了"
                              : "气流引爆了",
                  already_seen ? a_your[trap->madeby_u] : "",
                  already_seen ? "地雷" : "它");
        } else {
            /* prevent landmine from killing steed, throwing you to
             * the ground, and you being affected again by the same
             * mine because it hasn't been deleted yet
             */
            static boolean recursive_mine = FALSE;

            if (recursive_mine)
                break;
            feeltrap(trap);
            pline("KAABLAMM!!!  你触发了%s地雷!",
                  a_your[trap->madeby_u]);
            if (u.usteed)
                steed_mid = u.usteed->m_id;
            recursive_mine = TRUE;
            (void) steedintrap(trap, (struct obj *) 0);
            recursive_mine = FALSE;
            saddle = sobj_at(SADDLE, u.ux, u.uy);
            set_wounded_legs(LEFT_SIDE, rn1(35, 41));
            set_wounded_legs(RIGHT_SIDE, rn1(35, 41));
            exercise(A_DEX, FALSE);
        }
        blow_up_landmine(trap);
        if (steed_mid && saddle && !u.usteed)
            (void) keep_saddle_with_steedcorpse(steed_mid, fobj, saddle);
        newsym(u.ux, u.uy); /* update trap symbol */
        losehp(Maybe_Half_Phys(rnd(16)), "地雷", KILLED_BY_AN);
        /* fall recursively into the pit... */
        if ((trap = t_at(u.ux, u.uy)) != 0)
            dotrap(trap, RECURSIVETRAP);
        fill_pit(u.ux, u.uy);
        break;
    }

    case ROLLING_BOULDER_TRAP: {
        int style = ROLL | (trap->tseen ? LAUNCH_KNOWN : 0);

        feeltrap(trap);
        pline("咔哒! 你触发了一个滚动的巨石陷阱!");
        if (!launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                        trap->launch2.x, trap->launch2.y, style)) {
            deltrap(trap);
            newsym(u.ux, u.uy); /* get rid of trap symbol */
            pline("你很幸运, 没有巨石出来.");
        }
        break;
    }

    case MAGIC_PORTAL:
        feeltrap(trap);
        domagicportal(trap);
        break;

    case VIBRATING_SQUARE:
        feeltrap(trap);
        /* messages handled elsewhere; the trap symbol is merely to mark the
         * square for future reference */
        break;

    default:
        feeltrap(trap);
        impossible("You hit a trap of type %u", trap->ttyp);
    }
}

STATIC_OVL char *
trapnote(trap, noprefix)
struct trap *trap;
boolean noprefix;
{
    static char tnbuf[12];
    const char *tn,
        *tnnames[12] = { "C 调",  "降D 大调", "D 调",  "降E 大调",
                         "E 调",  "F 调", "升F 大调", "G 调",
                         "升G 大调", "A 调", "降B 大调",  "B 调" };

    tnbuf[0] = '\0';
    tn = tnnames[trap->tnote];
    if (!noprefix)
        Sprintf(tnbuf, "%s",
                (*tn == 'A' || *tn == 'E' || *tn == 'F') ? "" : "");
    Sprintf(eos(tnbuf), "%s", tn);
    return tnbuf;
}

STATIC_OVL int
steedintrap(trap, otmp)
struct trap *trap;
struct obj *otmp;
{
    struct monst *steed = u.usteed;
    int tt;
    boolean trapkilled, steedhit;

    if (!steed || !trap)
        return 0;
    tt = trap->ttyp;
    steed->mx = u.ux;
    steed->my = u.uy;
    trapkilled = steedhit = FALSE;

    switch (tt) {
    case ARROW_TRAP:
        if (!otmp) {
            impossible("steed hit by non-existant arrow?");
            return 0;
        }
        trapkilled = thitm(8, steed, otmp, 0, FALSE);
        steedhit = TRUE;
        break;
    case DART_TRAP:
        if (!otmp) {
            impossible("steed hit by non-existant dart?");
            return 0;
        }
        trapkilled = thitm(7, steed, otmp, 0, FALSE);
        steedhit = TRUE;
        break;
    case SLP_GAS_TRAP:
        if (!resists_sleep(steed) && !breathless(steed->data)
            && !steed->msleeping && steed->mcanmove) {
            if (sleep_monst(steed, rnd(25), -1))
                /* no in_sight check here; you can feel it even if blind */
                pline("%s 突然陷入了沉睡!", Monnam(steed));
        }
        steedhit = TRUE;
        break;
    case LANDMINE:
        trapkilled = thitm(0, steed, (struct obj *) 0, rnd(16), FALSE);
        steedhit = TRUE;
        break;
    case PIT:
    case SPIKED_PIT:
        trapkilled = (steed->mhp <= 0
                      || thitm(0, steed, (struct obj *) 0,
                               rnd((tt == PIT) ? 6 : 10), FALSE));
        steedhit = TRUE;
        break;
    case POLY_TRAP:
        if (!resists_magm(steed) && !resist(steed, WAND_CLASS, 0, NOTELL)) {
            (void) newcham(steed, (struct permonst *) 0, FALSE, FALSE);
            if (!can_saddle(steed) || !can_ride(steed))
                dismount_steed(DISMOUNT_POLY);
            else
                You("不得不调整你所坐在%s上的鞍.",
                    x_monnam(steed, ARTICLE_A, (char *) 0, SUPPRESS_SADDLE,
                             FALSE));
        }
        steedhit = TRUE;
        break;
    default:
        break;
    }

    if (trapkilled) {
        dismount_steed(DISMOUNT_POLY);
        return 2;
    }
    return steedhit ? 1 : 0;
}

/* some actions common to both player and monsters for triggered landmine */
void
blow_up_landmine(trap)
struct trap *trap;
{
    int x = trap->tx, y = trap->ty, dbx, dby;
    struct rm *lev = &levl[x][y];

    (void) scatter(x, y, 4,
                   MAY_DESTROY | MAY_HIT | MAY_FRACTURE | VIS_EFFECTS,
                   (struct obj *) 0);
    del_engr_at(x, y);
    wake_nearto(x, y, 400);
    if (IS_DOOR(lev->typ))
        lev->doormask = D_BROKEN;
    /* destroy drawbridge if present */
    if (lev->typ == DRAWBRIDGE_DOWN || is_drawbridge_wall(x, y) >= 0) {
        dbx = x, dby = y;
        /* if under the portcullis, the bridge is adjacent */
        if (find_drawbridge(&dbx, &dby))
            destroy_drawbridge(dbx, dby);
        trap = t_at(x, y); /* expected to be null after destruction */
    }
    /* convert landmine into pit */
    if (trap) {
        if (Is_waterlevel(&u.uz) || Is_airlevel(&u.uz)) {
            /* no pits here */
            deltrap(trap);
        } else {
            trap->ttyp = PIT;       /* explosion creates a pit */
            trap->madeby_u = FALSE; /* resulting pit isn't yours */
            seetrap(trap);          /* and it isn't concealed */
        }
    }
}

/*
 * The following are used to track launched objects to
 * prevent them from vanishing if you are killed. They
 * will reappear at the launchplace in bones files.
 */
static struct {
    struct obj *obj;
    xchar x, y;
} launchplace;

static void
launch_drop_spot(obj, x, y)
struct obj *obj;
xchar x, y;
{
    if (!obj) {
        launchplace.obj = (struct obj *) 0;
        launchplace.x = 0;
        launchplace.y = 0;
    } else {
        launchplace.obj = obj;
        launchplace.x = x;
        launchplace.y = y;
    }
}

boolean
launch_in_progress()
{
    if (launchplace.obj)
        return TRUE;
    return FALSE;
}

void
force_launch_placement()
{
    if (launchplace.obj) {
        launchplace.obj->otrapped = 0;
        place_object(launchplace.obj, launchplace.x, launchplace.y);
    }
}

/*
 * Move obj from (x1,y1) to (x2,y2)
 *
 * Return 0 if no object was launched.
 *        1 if an object was launched and placed somewhere.
 *        2 if an object was launched, but used up.
 */
int
launch_obj(otyp, x1, y1, x2, y2, style)
short otyp;
register int x1, y1, x2, y2;
int style;
{
    register struct monst *mtmp;
    register struct obj *otmp, *otmp2;
    register int dx, dy;
    struct obj *singleobj;
    boolean used_up = FALSE;
    boolean otherside = FALSE;
    int dist;
    int tmp;
    int delaycnt = 0;

    otmp = sobj_at(otyp, x1, y1);
    /* Try the other side too, for rolling boulder traps */
    if (!otmp && otyp == BOULDER) {
        otherside = TRUE;
        otmp = sobj_at(otyp, x2, y2);
    }
    if (!otmp)
        return 0;
    if (otherside) { /* swap 'em */
        int tx, ty;

        tx = x1;
        ty = y1;
        x1 = x2;
        y1 = y2;
        x2 = tx;
        y2 = ty;
    }

    if (otmp->quan == 1L) {
        obj_extract_self(otmp);
        singleobj = otmp;
        otmp = (struct obj *) 0;
    } else {
        singleobj = splitobj(otmp, 1L);
        obj_extract_self(singleobj);
    }
    newsym(x1, y1);
    /* in case you're using a pick-axe to chop the boulder that's being
       launched (perhaps a monster triggered it), destroy context so that
       next dig attempt never thinks you're resuming previous effort */
    if ((otyp == BOULDER || otyp == STATUE)
        && singleobj->ox == context.digging.pos.x
        && singleobj->oy == context.digging.pos.y)
        (void) memset((genericptr_t) &context.digging, 0,
                      sizeof(struct dig_info));

    dist = distmin(x1, y1, x2, y2);
    bhitpos.x = x1;
    bhitpos.y = y1;
    dx = sgn(x2 - x1);
    dy = sgn(y2 - y1);
    switch (style) {
    case ROLL | LAUNCH_UNSEEN:
        if (otyp == BOULDER) {
            You_hear(Hallucination ? "某人在打保龄球."
                                   : "远处的隆隆声.");
        }
        style &= ~LAUNCH_UNSEEN;
        goto roll;
    case ROLL | LAUNCH_KNOWN:
        /* use otrapped as a flag to ohitmon */
        singleobj->otrapped = 1;
        style &= ~LAUNCH_KNOWN;
    /* fall through */
    roll:
    case ROLL:
        delaycnt = 2;
    /* fall through */
    default:
        if (!delaycnt)
            delaycnt = 1;
        if (!cansee(bhitpos.x, bhitpos.y))
            curs_on_u();
        tmp_at(DISP_FLASH, obj_to_glyph(singleobj));
        tmp_at(bhitpos.x, bhitpos.y);
    }
    /* Mark a spot to place object in bones files to prevent
     * loss of object. Use the starting spot to ensure that
     * a rolling boulder will still launch, which it wouldn't
     * do if left midstream. Unfortunately we can't use the
     * target resting spot, because there are some things/situations
     * that would prevent it from ever getting there (bars), and we
     * can't tell that yet.
     */
    launch_drop_spot(singleobj, bhitpos.x, bhitpos.y);

    /* Set the object in motion */
    while (dist-- > 0 && !used_up) {
        struct trap *t;
        tmp_at(bhitpos.x, bhitpos.y);
        tmp = delaycnt;

        /* dstage@u.washington.edu -- Delay only if hero sees it */
        if (cansee(bhitpos.x, bhitpos.y))
            while (tmp-- > 0)
                delay_output();

        bhitpos.x += dx;
        bhitpos.y += dy;
        t = t_at(bhitpos.x, bhitpos.y);

        if ((mtmp = m_at(bhitpos.x, bhitpos.y)) != 0) {
            if (otyp == BOULDER && throws_rocks(mtmp->data)) {
                if (rn2(3)) {
                    pline("%s 抓住了巨石.", Monnam(mtmp));
                    singleobj->otrapped = 0;
                    (void) mpickobj(mtmp, singleobj);
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
            }
            if (ohitmon(mtmp, singleobj, (style == ROLL) ? -1 : dist,
                        FALSE)) {
                used_up = TRUE;
                launch_drop_spot((struct obj *) 0, 0, 0);
                break;
            }
        } else if (bhitpos.x == u.ux && bhitpos.y == u.uy) {
            if (multi)
                nomul(0);
            if (thitu(9 + singleobj->spe, dmgval(singleobj, &youmonst),
                      singleobj, (char *) 0))
                stop_occupation();
        }
        if (style == ROLL) {
            if (down_gate(bhitpos.x, bhitpos.y) != -1) {
                if (ship_object(singleobj, bhitpos.x, bhitpos.y, FALSE)) {
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                }
            }
            if (t && otyp == BOULDER) {
                switch (t->ttyp) {
                case LANDMINE:
                    if (rn2(10) > 2) {
                        pline(
                            "嘣!!!%s",
                            cansee(bhitpos.x, bhitpos.y)
                                ? " 滚动的巨石触发了一个地雷."
                                : "");
                        deltrap(t);
                        del_engr_at(bhitpos.x, bhitpos.y);
                        place_object(singleobj, bhitpos.x, bhitpos.y);
                        singleobj->otrapped = 0;
                        fracture_rock(singleobj);
                        (void) scatter(bhitpos.x, bhitpos.y, 4,
                                       MAY_DESTROY | MAY_HIT | MAY_FRACTURE
                                           | VIS_EFFECTS,
                                       (struct obj *) 0);
                        if (cansee(bhitpos.x, bhitpos.y))
                            newsym(bhitpos.x, bhitpos.y);
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    break;
                case LEVEL_TELEP:
                case TELEP_TRAP:
                    if (cansee(bhitpos.x, bhitpos.y))
                        pline("突然滚动的巨石消失了!");
                    else
                        You_hear("隆隆声突然停止了.");
                    singleobj->otrapped = 0;
                    if (t->ttyp == TELEP_TRAP)
                        (void) rloco(singleobj);
                    else {
                        int newlev = random_teleport_level();
                        d_level dest;

                        if (newlev == depth(&u.uz) || In_endgame(&u.uz))
                            continue;
                        add_to_migration(singleobj);
                        get_level(&dest, newlev);
                        singleobj->ox = dest.dnum;
                        singleobj->oy = dest.dlevel;
                        singleobj->owornmask = (long) MIGR_RANDOM;
                    }
                    seetrap(t);
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                    break;
                case PIT:
                case SPIKED_PIT:
                case HOLE:
                case TRAPDOOR:
                    /* the boulder won't be used up if there is a
                       monster in the trap; stop rolling anyway */
                    x2 = bhitpos.x, y2 = bhitpos.y; /* stops here */
                    if (flooreffects(singleobj, x2, y2, "掉落")) {
                        used_up = TRUE;
                        launch_drop_spot((struct obj *) 0, 0, 0);
                    }
                    dist = -1; /* stop rolling immediately */
                    break;
                }
                if (used_up || dist == -1)
                    break;
            }
            if (flooreffects(singleobj, bhitpos.x, bhitpos.y, "掉落")) {
                used_up = TRUE;
                launch_drop_spot((struct obj *) 0, 0, 0);
                break;
            }
            if (otyp == BOULDER
                && (otmp2 = sobj_at(BOULDER, bhitpos.x, bhitpos.y)) != 0) {
                const char *bmsg = "当一个巨石使得另一个移动";

                if (!isok(bhitpos.x + dx, bhitpos.y + dy) || !dist
                    || IS_ROCK(levl[bhitpos.x + dx][bhitpos.y + dy].typ))
                    bmsg = "当一个巨石撞到另一个";

                You_hear("一声巨响%s!",
                         cansee(bhitpos.x, bhitpos.y) ? bmsg : "");
                obj_extract_self(otmp2);
                /* pass off the otrapped flag to the next boulder */
                otmp2->otrapped = singleobj->otrapped;
                singleobj->otrapped = 0;
                place_object(singleobj, bhitpos.x, bhitpos.y);
                singleobj = otmp2;
                otmp2 = (struct obj *) 0;
                wake_nearto(bhitpos.x, bhitpos.y, 10 * 10);
            }
        }
        if (otyp == BOULDER && closed_door(bhitpos.x, bhitpos.y)) {
            if (cansee(bhitpos.x, bhitpos.y))
                pline_The("巨石撞开一道门.");
            levl[bhitpos.x][bhitpos.y].doormask = D_BROKEN;
            if (dist)
                unblock_point(bhitpos.x, bhitpos.y);
        }

        /* if about to hit iron bars, do so now */
        if (dist > 0 && isok(bhitpos.x + dx, bhitpos.y + dy)
            && levl[bhitpos.x + dx][bhitpos.y + dy].typ == IRONBARS) {
            x2 = bhitpos.x, y2 = bhitpos.y; /* object stops here */
            if (hits_bars(&singleobj, x2, y2, !rn2(20), 0)) {
                if (!singleobj) {
                    used_up = TRUE;
                    launch_drop_spot((struct obj *) 0, 0, 0);
                }
                break;
            }
        }
    }
    tmp_at(DISP_END, 0);
    launch_drop_spot((struct obj *) 0, 0, 0);
    if (!used_up) {
        singleobj->otrapped = 0;
        place_object(singleobj, x2, y2);
        newsym(x2, y2);
        return 1;
    } else
        return 2;
}

void
seetrap(trap)
struct trap *trap;
{
    if (!trap->tseen) {
        trap->tseen = 1;
        newsym(trap->tx, trap->ty);
    }
}

/* like seetrap() but overrides vision */
void
feeltrap(trap)
struct trap *trap;
{
    trap->tseen = 1;
    map_trap(trap, 1);
    /* in case it's beneath something, redisplay the something */
    newsym(trap->tx, trap->ty);
}

STATIC_OVL int
mkroll_launch(ttmp, x, y, otyp, ocount)
struct trap *ttmp;
xchar x, y;
short otyp;
long ocount;
{
    struct obj *otmp;
    register int tmp;
    schar dx, dy;
    int distance;
    coord cc;
    coord bcc;
    int trycount = 0;
    boolean success = FALSE;
    int mindist = 4;

    if (ttmp->ttyp == ROLLING_BOULDER_TRAP)
        mindist = 2;
    distance = rn1(5, 4); /* 4..8 away */
    tmp = rn2(8);         /* randomly pick a direction to try first */
    while (distance >= mindist) {
        dx = xdir[tmp];
        dy = ydir[tmp];
        cc.x = x;
        cc.y = y;
        /* Prevent boulder from being placed on water */
        if (ttmp->ttyp == ROLLING_BOULDER_TRAP
            && is_pool_or_lava(x + distance * dx, y + distance * dy))
            success = FALSE;
        else
            success = isclearpath(&cc, distance, dx, dy);
        if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
            boolean success_otherway;

            bcc.x = x;
            bcc.y = y;
            success_otherway = isclearpath(&bcc, distance, -(dx), -(dy));
            if (!success_otherway)
                success = FALSE;
        }
        if (success)
            break;
        if (++tmp > 7)
            tmp = 0;
        if ((++trycount % 8) == 0)
            --distance;
    }
    if (!success) {
        /* create the trap without any ammo, launch pt at trap location */
        cc.x = bcc.x = x;
        cc.y = bcc.y = y;
    } else {
        otmp = mksobj(otyp, TRUE, FALSE);
        otmp->quan = ocount;
        otmp->owt = weight(otmp);
        place_object(otmp, cc.x, cc.y);
        stackobj(otmp);
    }
    ttmp->launch.x = cc.x;
    ttmp->launch.y = cc.y;
    if (ttmp->ttyp == ROLLING_BOULDER_TRAP) {
        ttmp->launch2.x = bcc.x;
        ttmp->launch2.y = bcc.y;
    } else
        ttmp->launch_otyp = otyp;
    newsym(ttmp->launch.x, ttmp->launch.y);
    return 1;
}

STATIC_OVL boolean
isclearpath(cc, distance, dx, dy)
coord *cc;
int distance;
schar dx, dy;
{
    uchar typ;
    xchar x, y;

    x = cc->x;
    y = cc->y;
    while (distance-- > 0) {
        x += dx;
        y += dy;
        typ = levl[x][y].typ;
        if (!isok(x, y) || !ZAP_POS(typ) || closed_door(x, y))
            return FALSE;
    }
    cc->x = x;
    cc->y = y;
    return TRUE;
}

int
mintrap(mtmp)
register struct monst *mtmp;
{
    register struct trap *trap = t_at(mtmp->mx, mtmp->my);
    boolean trapkilled = FALSE;
    struct permonst *mptr = mtmp->data;
    struct obj *otmp;

    if (!trap) {
        mtmp->mtrapped = 0;      /* perhaps teleported? */
    } else if (mtmp->mtrapped) { /* is currently in the trap */
        if (!trap->tseen && cansee(mtmp->mx, mtmp->my) && canseemon(mtmp)
            && (trap->ttyp == SPIKED_PIT || trap->ttyp == BEAR_TRAP
                || trap->ttyp == HOLE || trap->ttyp == PIT
                || trap->ttyp == WEB)) {
            /* If you come upon an obviously trapped monster, then
             * you must be able to see the trap it's in too.
             */
            seetrap(trap);
        }

        if (!rn2(40)) {
            if (sobj_at(BOULDER, mtmp->mx, mtmp->my)
                && (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)) {
                if (!rn2(2)) {
                    mtmp->mtrapped = 0;
                    if (canseemon(mtmp))
                        pline("%s 使得无阻碍了...", Monnam(mtmp));
                    fill_pit(mtmp->mx, mtmp->my);
                }
            } else {
                mtmp->mtrapped = 0;
            }
        } else if (metallivorous(mptr)) {
            if (trap->ttyp == BEAR_TRAP) {
                if (canseemon(mtmp))
                    pline("%s 吃食一个捕兽夹!", Monnam(mtmp));
                deltrap(trap);
                mtmp->meating = 5;
                mtmp->mtrapped = 0;
            } else if (trap->ttyp == SPIKED_PIT) {
                if (canseemon(mtmp))
                    pline("%s 用力咀嚼一些钉子!", Monnam(mtmp));
                trap->ttyp = PIT;
                mtmp->meating = 5;
            }
        }
    } else {
        register int tt = trap->ttyp;
        boolean in_sight, tear_web, see_it,
            inescapable = force_mintrap || ((tt == HOLE || tt == PIT)
                                            && Sokoban && !trap->madeby_u);
        const char *fallverb;

        /* true when called from dotrap, inescapable is not an option */
        if (mtmp == u.usteed)
            inescapable = TRUE;
        if (!inescapable && ((mtmp->mtrapseen & (1 << (tt - 1))) != 0
                             || (tt == HOLE && !mindless(mptr)))) {
            /* it has been in such a trap - perhaps it escapes */
            if (rn2(4))
                return 0;
        } else {
            mtmp->mtrapseen |= (1 << (tt - 1));
        }
        /* Monster is aggravated by being trapped by you.
           Recognizing who made the trap isn't completely
           unreasonable; everybody has their own style. */
        if (trap->madeby_u && rnl(5))
            setmangry(mtmp);

        in_sight = canseemon(mtmp);
        see_it = cansee(mtmp->mx, mtmp->my);
        /* assume hero can tell what's going on for the steed */
        if (mtmp == u.usteed)
            in_sight = TRUE;
        switch (tt) {
        case ARROW_TRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
                if (in_sight && see_it)
                    pline("%s 触发了一个陷阱但是无事发生.",
                          Monnam(mtmp));
                deltrap(trap);
                newsym(mtmp->mx, mtmp->my);
                break;
            }
            trap->once = 1;
            otmp = mksobj(ARROW, TRUE, FALSE);
            otmp->quan = 1L;
            otmp->owt = weight(otmp);
            otmp->opoisoned = 0;
            if (in_sight)
                seetrap(trap);
            if (thitm(8, mtmp, otmp, 0, FALSE))
                trapkilled = TRUE;
            break;
        case DART_TRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
                if (in_sight && see_it)
                    pline("%s 触发了一个陷阱但是无事发生.",
                          Monnam(mtmp));
                deltrap(trap);
                newsym(mtmp->mx, mtmp->my);
                break;
            }
            trap->once = 1;
            otmp = mksobj(DART, TRUE, FALSE);
            otmp->quan = 1L;
            otmp->owt = weight(otmp);
            if (!rn2(6))
                otmp->opoisoned = 1;
            if (in_sight)
                seetrap(trap);
            if (thitm(7, mtmp, otmp, 0, FALSE))
                trapkilled = TRUE;
            break;
        case ROCKTRAP:
            if (trap->once && trap->tseen && !rn2(15)) {
                if (in_sight && see_it)
                    pline(
                        "%s 上面的陷阱门打开了, 但是什么都没有掉下来!",
                        mon_nam(mtmp));
                deltrap(trap);
                newsym(mtmp->mx, mtmp->my);
                break;
            }
            trap->once = 1;
            otmp = mksobj(ROCK, TRUE, FALSE);
            otmp->quan = 1L;
            otmp->owt = weight(otmp);
            if (in_sight)
                seetrap(trap);
            if (thitm(0, mtmp, otmp, d(2, 6), FALSE))
                trapkilled = TRUE;
            break;
        case SQKY_BOARD:
            if (is_flyer(mptr))
                break;
            /* stepped on a squeaky board */
            if (in_sight) {
                if (!Deaf) {
                    pline("%s 下方的板大声地吱吱响着%s.",
                          mon_nam(mtmp), trapnote(trap, 0));
                    seetrap(trap);
                } else {
                    pline("%s 立即停下来并显得畏缩.",
                          Monnam(mtmp));
                }
            } else
                You_hear("远处的%s吱吱声.", trapnote(trap, 1));
            /* wake up nearby monsters */
            wake_nearto(mtmp->mx, mtmp->my, 40);
            break;
        case BEAR_TRAP:
            if (mptr->msize > MZ_SMALL && !amorphous(mptr) && !is_flyer(mptr)
                && !is_whirly(mptr) && !unsolid(mptr)) {
                mtmp->mtrapped = 1;
                if (in_sight) {
                    pline("%s 被夹在%s捕兽夹里!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                    seetrap(trap);
                } else {
                    if (mptr == &mons[PM_OWLBEAR]
                        || mptr == &mons[PM_BUGBEAR])
                        You_hear("愤怒的熊的咆哮!");
                }
            } else if (force_mintrap) {
                if (in_sight) {
                    pline("%s 逃脱了%s捕兽夹!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                    seetrap(trap);
                }
            }
            if (mtmp->mtrapped)
                trapkilled = thitm(0, mtmp, (struct obj *) 0, d(2, 4), FALSE);
            break;
        case SLP_GAS_TRAP:
            if (!resists_sleep(mtmp) && !breathless(mptr) && !mtmp->msleeping
                && mtmp->mcanmove) {
                if (sleep_monst(mtmp, rnd(25), -1) && in_sight) {
                    pline("%s 突然陷入了沉睡!", Monnam(mtmp));
                    seetrap(trap);
                }
            }
            break;
        case RUST_TRAP: {
            struct obj *target;

            if (in_sight)
                seetrap(trap);
            switch (rn2(5)) {
            case 0:
                if (in_sight)
                    pline("%s %s的%s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, HEAD));
                target = which_armor(mtmp, W_ARMH);
                (void) water_damage(target, helm_simple_name(target), TRUE);
                break;
            case 1:
                if (in_sight)
                    pline("%s %s的左%s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, ARM));
                target = which_armor(mtmp, W_ARMS);
                if (water_damage(target, "盾牌", TRUE) != ER_NOTHING)
                    break;
                target = MON_WEP(mtmp);
                if (target && bimanual(target))
                    (void) water_damage(target, 0, TRUE);
            glovecheck:
                target = which_armor(mtmp, W_ARMG);
                (void) water_damage(target, "手套", TRUE);
                break;
            case 2:
                if (in_sight)
                    pline("%s %s的右%s!", A_gush_of_water_hits,
                          mon_nam(mtmp), mbodypart(mtmp, ARM));
                (void) water_damage(MON_WEP(mtmp), 0, TRUE);
                goto glovecheck;
            default:
                if (in_sight)
                    pline("%s %s!", A_gush_of_water_hits, mon_nam(mtmp));
                for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                    if (otmp->lamplit
                        && (otmp->owornmask & (W_WEP | W_SWAPWEP)) == 0)
                        (void) snuff_lit(otmp);
                if ((target = which_armor(mtmp, W_ARMC)) != 0)
                    (void) water_damage(target, cloak_simple_name(target),
                                        TRUE);
                else if ((target = which_armor(mtmp, W_ARM)) != 0)
                    (void) water_damage(target, "盔甲", TRUE);
                else if ((target = which_armor(mtmp, W_ARMU)) != 0)
                    (void) water_damage(target, "衬衫", TRUE);
            }

            if (mptr == &mons[PM_IRON_GOLEM]) {
                if (in_sight)
                    pline("%s 散落为碎片!", Monnam(mtmp));
                else if (mtmp->mtame)
                    pline("愿%s 在平静中生锈.", mon_nam(mtmp));
                mondied(mtmp);
                if (mtmp->mhp <= 0)
                    trapkilled = TRUE;
            } else if (mptr == &mons[PM_GREMLIN] && rn2(3)) {
                (void) split_mon(mtmp, (struct monst *) 0);
            }
            break;
        } /* RUST_TRAP */
        case FIRE_TRAP:
        mfiretrap:
            if (in_sight)
                pline("%s从%s下面的%s喷出!", tower_of_flame,
                      mon_nam(mtmp), surface(mtmp->mx, mtmp->my));
            else if (see_it) /* evidently `mtmp' is invisible */
                You_see("%s从%s喷出!", tower_of_flame,
                        surface(mtmp->mx, mtmp->my));

            if (resists_fire(mtmp)) {
                if (in_sight) {
                    shieldeff(mtmp->mx, mtmp->my);
                    pline("%s 未受伤.", Monnam(mtmp));
                }
            } else {
                int num = d(2, 4), alt;
                boolean immolate = FALSE;

                /* paper burns very fast, assume straw is tightly
                 * packed and burns a bit slower */
                switch (monsndx(mptr)) {
                case PM_PAPER_GOLEM:
                    immolate = TRUE;
                    alt = mtmp->mhpmax;
                    break;
                case PM_STRAW_GOLEM:
                    alt = mtmp->mhpmax / 2;
                    break;
                case PM_WOOD_GOLEM:
                    alt = mtmp->mhpmax / 4;
                    break;
                case PM_LEATHER_GOLEM:
                    alt = mtmp->mhpmax / 8;
                    break;
                default:
                    alt = 0;
                    break;
                }
                if (alt > num)
                    num = alt;

                if (thitm(0, mtmp, (struct obj *) 0, num, immolate))
                    trapkilled = TRUE;
                else
                    /* we know mhp is at least `num' below mhpmax,
                       so no (mhp > mhpmax) check is needed here */
                    mtmp->mhpmax -= rn2(num + 1);
            }
            if (burnarmor(mtmp) || rn2(3)) {
                (void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
                (void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
                (void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
            }
            if (burn_floor_objects(mtmp->mx, mtmp->my, see_it, FALSE)
                && !see_it && distu(mtmp->mx, mtmp->my) <= 3 * 3)
                You("闻到了烟味.");
            if (is_ice(mtmp->mx, mtmp->my))
                melt_ice(mtmp->mx, mtmp->my, (char *) 0);
            if (see_it)
                seetrap(trap);
            break;
        case PIT:
        case SPIKED_PIT:
            fallverb = "掉落";
            if (is_flyer(mptr) || is_floater(mptr)
                || (mtmp->wormno && count_wsegs(mtmp) > 5)
                || is_clinger(mptr)) {
                if (force_mintrap && !Sokoban) {
                    /* openfallingtrap; not inescapable here */
                    if (in_sight) {
                        seetrap(trap);
                        pline("%s 没有掉进坑里.", Monnam(mtmp));
                    }
                    break; /* inescapable = FALSE; */
                }
                if (!inescapable)
                    break;               /* avoids trap */
                fallverb = "被拖"; /* sokoban pit */
            }
            if (!passes_walls(mptr))
                mtmp->mtrapped = 1;
            if (in_sight) {
                pline("%s %s进%s坑!", Monnam(mtmp), fallverb,
                      a_your[trap->madeby_u]);
                if (mptr == &mons[PM_PIT_VIPER]
                    || mptr == &mons[PM_PIT_FIEND])
                    pline("多可怜啊.  那不是坑?");
                seetrap(trap);
            }
            mselftouch(mtmp, "落下, ", FALSE);
            if (mtmp->mhp <= 0 || thitm(0, mtmp, (struct obj *) 0,
                                        rnd((tt == PIT) ? 6 : 10), FALSE))
                trapkilled = TRUE;
            break;
        case HOLE:
        case TRAPDOOR:
            if (!Can_fall_thru(&u.uz)) {
                impossible("mintrap: %ss cannot exist on this level.",
                           defsyms[trap_to_defsym(tt)].explanation);
                break; /* don't activate it after all */
            }
            if (is_flyer(mptr) || is_floater(mptr) || mptr == &mons[PM_WUMPUS]
                || (mtmp->wormno && count_wsegs(mtmp) > 5)
                || mptr->msize >= MZ_HUGE) {
                if (force_mintrap && !Sokoban) {
                    /* openfallingtrap; not inescapable here */
                    if (in_sight) {
                        seetrap(trap);
                        if (tt == TRAPDOOR)
                            pline(
                            "一个陷阱门打开了, 但是%s 没有掉下去.",
                                  mon_nam(mtmp));
                        else /* (tt == HOLE) */
                            pline("%s 没有掉下洞去.",
                                  Monnam(mtmp));
                    }
                    break; /* inescapable = FALSE; */
                }
                if (inescapable) { /* sokoban hole */
                    if (in_sight) {
                        pline("%s 似乎被拽下来!", Monnam(mtmp));
                        /* suppress message in mlevel_tele_trap() */
                        in_sight = FALSE;
                        seetrap(trap);
                    }
                } else
                    break;
            }
            /*FALLTHRU*/
        case LEVEL_TELEP:
        case MAGIC_PORTAL: {
            int mlev_res;

            mlev_res = mlevel_tele_trap(mtmp, trap, inescapable, in_sight);
            if (mlev_res)
                return mlev_res;
            break;
        }
        case TELEP_TRAP:
            mtele_trap(mtmp, trap, in_sight);
            break;
        case WEB:
            /* Monster in a web. */
            if (webmaker(mptr))
                break;
            if (amorphous(mptr) || is_whirly(mptr) || unsolid(mptr)) {
                if (acidic(mptr) || mptr == &mons[PM_GELATINOUS_CUBE]
                    || mptr == &mons[PM_FIRE_ELEMENTAL]) {
                    if (in_sight)
                        pline("%s %s了%s蜘蛛网!", Monnam(mtmp),
                              (mptr == &mons[PM_FIRE_ELEMENTAL])
                                  ? "烧掉"
                                  : "溶解",
                              a_your[trap->madeby_u]);
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                    break;
                }
                if (in_sight) {
                    pline("%s 流过%s蜘蛛网.", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                    seetrap(trap);
                }
                break;
            }
            tear_web = FALSE;
            switch (monsndx(mptr)) {
            case PM_OWLBEAR: /* Eric Backus */
            case PM_BUGBEAR:
                if (!in_sight) {
                    You_hear("混乱的熊的咆哮!");
                    mtmp->mtrapped = 1;
                    break;
                }
            /* fall though */
            default:
                if (mptr->mlet == S_GIANT
                    /* exclude baby dragons and relatively short worms */
                    || (mptr->mlet == S_DRAGON && extra_nasty(mptr))
                    || (mtmp->wormno && count_wsegs(mtmp) > 5)) {
                    tear_web = TRUE;
                } else if (in_sight) {
                    pline("%s 陷入%s蜘蛛网.", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                    seetrap(trap);
                }
                mtmp->mtrapped = tear_web ? 0 : 1;
                break;
            /* this list is fairly arbitrary; it deliberately
               excludes wumpus & giant/ettin zombies/mummies */
            case PM_TITANOTHERE:
            case PM_BALUCHITHERIUM:
            case PM_PURPLE_WORM:
            case PM_JABBERWOCK:
            case PM_IRON_GOLEM:
            case PM_BALROG:
            case PM_KRAKEN:
            case PM_MASTODON:
            case PM_ORION:
            case PM_NORN:
            case PM_CYCLOPS:
            case PM_LORD_SURTUR:
                tear_web = TRUE;
                break;
            }
            if (tear_web) {
                if (in_sight)
                    pline("%s 撕裂%s蜘蛛网!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                deltrap(trap);
                newsym(mtmp->mx, mtmp->my);
            } else if (force_mintrap && !mtmp->mtrapped) {
                if (in_sight) {
                    pline("%s 避开了%s蜘蛛网!", Monnam(mtmp),
                          a_your[trap->madeby_u]);
                    seetrap(trap);
                }
            }
            break;
        case STATUE_TRAP:
            break;
        case MAGIC_TRAP:
            /* A magic trap.  Monsters usually immune. */
            if (!rn2(21))
                goto mfiretrap;
            break;
        case ANTI_MAGIC:
            /* similar to hero's case, more or less */
            if (!resists_magm(mtmp)) { /* lose spell energy */
                if (!mtmp->mcan && (attacktype(mptr, AT_MAGC)
                                    || attacktype(mptr, AT_BREA))) {
                    mtmp->mspec_used += d(2, 2);
                    if (in_sight) {
                        seetrap(trap);
                        pline("%s 似乎无生气的.", Monnam(mtmp));
                    }
                }
            } else { /* take some damage */
                int dmgval2 = rnd(4);

                if ((otmp = MON_WEP(mtmp)) != 0
                    && otmp->oartifact == ART_MAGICBANE)
                    dmgval2 += rnd(4);
                for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
                    if (otmp->oartifact
                        && defends_when_carried(AD_MAGM, otmp))
                        break;
                if (otmp)
                    dmgval2 += rnd(4);
                if (passes_walls(mptr))
                    dmgval2 = (dmgval2 + 3) / 4;

                if (in_sight)
                    seetrap(trap);
                if ((mtmp->mhp -= dmgval2) <= 0)
                    monkilled(mtmp,
                              in_sight
                                  ? "一个反魔法领域压缩"
                                  : (const char *) 0,
                              -AD_MAGM);
                if (mtmp->mhp <= 0)
                    trapkilled = TRUE;
                if (see_it)
                    newsym(trap->tx, trap->ty);
            }
            break;
        case LANDMINE:
            if (rn2(3))
                break; /* monsters usually don't set it off */
            if (is_flyer(mptr)) {
                boolean already_seen = trap->tseen;

                if (in_sight && !already_seen) {
                    pline("%s的下方一个触发器出现在一堆土里.",
                          mon_nam(mtmp));
                    seetrap(trap);
                }
                if (rn2(3))
                    break;
                if (in_sight) {
                    newsym(mtmp->mx, mtmp->my);
                    pline_The("气流引爆了%s!",
                              already_seen ? "一个地雷" : "它");
                }
            } else if (in_sight) {
                newsym(mtmp->mx, mtmp->my);
                pline("嘣!!!  %s 触发了%s地雷!", Monnam(mtmp),
                      a_your[trap->madeby_u]);
            }
            if (!in_sight)
                pline("嘣!  你听见远处的爆炸声!");
            blow_up_landmine(trap);
            /* explosion might have destroyed a drawbridge; don't
               dish out more damage if monster is already dead */
            if (mtmp->mhp <= 0
                || thitm(0, mtmp, (struct obj *) 0, rnd(16), FALSE))
                trapkilled = TRUE;
            else {
                /* monsters recursively fall into new pit */
                if (mintrap(mtmp) == 2)
                    trapkilled = TRUE;
            }
            /* a boulder may fill the new pit, crushing monster */
            fill_pit(trap->tx, trap->ty);
            if (mtmp->mhp <= 0)
                trapkilled = TRUE;
            if (unconscious()) {
                multi = -1;
                nomovemsg = "爆炸声弄醒了你!";
            }
            break;
        case POLY_TRAP:
            if (resists_magm(mtmp)) {
                shieldeff(mtmp->mx, mtmp->my);
            } else if (!resist(mtmp, WAND_CLASS, 0, NOTELL)) {
                if (newcham(mtmp, (struct permonst *) 0, FALSE, FALSE))
                    /* we're done with mptr but keep it up to date */
                    mptr = mtmp->data;
                if (in_sight)
                    seetrap(trap);
            }
            break;
        case ROLLING_BOULDER_TRAP:
            if (!is_flyer(mptr)) {
                int style = ROLL | (in_sight ? 0 : LAUNCH_UNSEEN);

                newsym(mtmp->mx, mtmp->my);
                if (in_sight)
                    pline("咔哒! %s 触发了%s.", Monnam(mtmp),
                          trap->tseen ? "一个滚动的巨石陷阱" : something);
                if (launch_obj(BOULDER, trap->launch.x, trap->launch.y,
                               trap->launch2.x, trap->launch2.y, style)) {
                    if (in_sight)
                        trap->tseen = TRUE;
                    if (mtmp->mhp <= 0)
                        trapkilled = TRUE;
                } else {
                    deltrap(trap);
                    newsym(mtmp->mx, mtmp->my);
                }
            }
            break;
        case VIBRATING_SQUARE:
            if (see_it && !Blind) {
                if (in_sight)
                    pline("你看到一个奇怪的振动在%s %s的下方.",
                          s_suffix(mon_nam(mtmp)),
                          makeplural(mbodypart(mtmp, FOOT)));
                else
                    pline("你看见远处的地面震动.");
                seetrap(trap);
            }
            break;
        default:
            impossible("Some monster encountered a strange trap of type %d.",
                       tt);
        }
    }
    if (trapkilled)
        return 2;
    return mtmp->mtrapped;
}

/* Combine cockatrice checks into single functions to avoid repeating code. */
void
instapetrify(str)
const char *str;
{
    if (Stone_resistance)
        return;
    if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM))
        return;
    You("变成了石头...");
    killer.format = KILLED_BY;
    if (str != killer.name)
        Strcpy(killer.name, str ? str : "");
    done(STONING);
}

void
minstapetrify(mon, byplayer)
struct monst *mon;
boolean byplayer;
{
    if (resists_ston(mon))
        return;
    if (poly_when_stoned(mon->data)) {
        mon_to_stone(mon);
        return;
    }

    /* give a "<mon> is slowing down" message and also remove
       intrinsic speed (comparable to similar effect on the hero) */
    mon_adjust_speed(mon, -3, (struct obj *) 0);

    if (cansee(mon->mx, mon->my))
        pline("%s 变成了石头.", Monnam(mon));
    if (byplayer) {
        stoned = TRUE;
        xkilled(mon, 0);
    } else
        monstone(mon);
}

void
selftouch(arg)
const char *arg;
{
    char kbuf[BUFSZ];

    if (uwep && uwep->otyp == CORPSE && touch_petrifies(&mons[uwep->corpsenm])
        && !Stone_resistance) {
        pline("%s碰到了%s尸体.", arg, mons[uwep->corpsenm].mname);
        Sprintf(kbuf, "%s尸体", mons[uwep->corpsenm].mname);
        instapetrify(kbuf);
        /* life-saved; unwield the corpse if we can't handle it */
        if (!uarmg && !Stone_resistance)
            uwepgone();
    }
    /* Or your secondary weapon, if wielded [hypothetical; we don't
       allow two-weapon combat when either weapon is a corpse] */
    if (u.twoweap && uswapwep && uswapwep->otyp == CORPSE
        && touch_petrifies(&mons[uswapwep->corpsenm]) && !Stone_resistance) {
        pline("%s碰到了%s尸体.", arg, mons[uswapwep->corpsenm].mname);
        Sprintf(kbuf, "%s尸体", mons[uswapwep->corpsenm].mname);
        instapetrify(kbuf);
        /* life-saved; unwield the corpse */
        if (!uarmg && !Stone_resistance)
            uswapwepgone();
    }
}

void
mselftouch(mon, arg, byplayer)
struct monst *mon;
const char *arg;
boolean byplayer;
{
    struct obj *mwep = MON_WEP(mon);

    if (mwep && mwep->otyp == CORPSE && touch_petrifies(&mons[mwep->corpsenm])
        && !resists_ston(mon)) {
        if (cansee(mon->mx, mon->my)) {
            pline("%s%s 碰到了%s.", arg ? arg : "",
                  arg ? mon_nam(mon) : Monnam(mon),
                  corpse_xname(mwep, (const char *) 0, CXN_PFX_THE));
        }
        minstapetrify(mon, byplayer);
        /* if life-saved, might not be able to continue wielding */
        if (mon->mhp > 0 && !which_armor(mon, W_ARMG) && !resists_ston(mon))
            mwepgone(mon);
    }
}

/* start levitating */
void
float_up()
{
    if (u.utrap) {
        if (u.utraptype == TT_PIT) {
            u.utrap = 0;
            You("飘浮起来, 出了坑!");
            vision_full_recalc = 1; /* vision limits change */
            fill_pit(u.ux, u.uy);
        } else if (u.utraptype == TT_INFLOOR) {
            Your("身体向上拉, 但是你的%s仍然被卡住.",
                 makeplural(body_part(LEG)));
        } else {
            You("飘浮起来, 只有你的%s仍然被卡住.", body_part(LEG));
        }
#if 0
    } else if (Is_waterlevel(&u.uz)) {
        pline("It feels as though you've lost some weight.");
#endif
    } else if (u.uinwater) {
        spoteffects(TRUE);
    } else if (u.uswallow) {
        You(is_animal(u.ustuck->data) ? "从%s飘走."
                                      : "盘旋上升到%s.",
            is_animal(u.ustuck->data) ? surface(u.ux, u.uy)
                                      : mon_nam(u.ustuck));
    } else if (Hallucination) {
        pline("上升, 上升, 并且升远了!  你在空中行走!");
    } else if (Is_airlevel(&u.uz)) {
        You("能控制你的移动了.");
    } else {
        You("开始飘在空中!");
    }
    if (u.usteed && !is_floater(u.usteed->data)
        && !is_flyer(u.usteed->data)) {
        if (Lev_at_will) {
            pline("%s 如魔法般地飘浮起来!", Monnam(u.usteed));
        } else {
            You("不能待在%s上.", mon_nam(u.usteed));
            dismount_steed(DISMOUNT_GENERIC);
        }
    }
    if (Flying)
        You("不再能控制你的飞行.");
    BFlying |= I_SPECIAL;
    return;
}

void
fill_pit(x, y)
int x, y;
{
    struct obj *otmp;
    struct trap *t;

    if ((t = t_at(x, y)) && ((t->ttyp == PIT) || (t->ttyp == SPIKED_PIT))
        && (otmp = sobj_at(BOULDER, x, y))) {
        obj_extract_self(otmp);
        (void) flooreffects(otmp, x, y, "陷入");
    }
}

/* stop levitating */
int
float_down(hmask, emask)
long hmask, emask; /* might cancel timeout */
{
    register struct trap *trap = (struct trap *) 0;
    d_level current_dungeon_level;
    boolean no_msg = FALSE;

    HLevitation &= ~hmask;
    ELevitation &= ~emask;
    if (Levitation)
        return 0; /* maybe another ring/potion/boots */
    if (BLevitation) {
        /* Levitation is blocked, so hero is not actually floating
           hence shouldn't have float_down effects and feedback */
        float_vs_flight(); /* before nomul() rather than after */
        return 0;
    }
    nomul(0); /* stop running or resting */
    if (BFlying) {
        /* controlled flight no longer overridden by levitation */
        BFlying &= ~I_SPECIAL;
        if (Flying) {
            You("停止了飘浮现在开始飞行.");
            return 1;
        }
    }
    if (u.uswallow) {
        You("飘落下来, 但你仍然%s.",
            is_animal(u.ustuck->data) ? "被吞下" : "被吞食");
        return 1;
    }

    if (Punished && !carried(uball)
        && (is_pool(uball->ox, uball->oy)
            || ((trap = t_at(uball->ox, uball->oy))
                && ((trap->ttyp == PIT) || (trap->ttyp == SPIKED_PIT)
                    || (trap->ttyp == TRAPDOOR) || (trap->ttyp == HOLE))))) {
        u.ux0 = u.ux;
        u.uy0 = u.uy;
        u.ux = uball->ox;
        u.uy = uball->oy;
        movobj(uchain, uball->ox, uball->oy);
        newsym(u.ux0, u.uy0);
        vision_full_recalc = 1; /* in case the hero moved. */
    }
    /* check for falling into pool - added by GAN 10/20/86 */
    if (!Flying) {
        if (!u.uswallow && u.ustuck) {
            if (sticks(youmonst.data))
                You("不能够维持你牵制的%s.",
                    mon_nam(u.ustuck));
            else
                pline("受惊吓的, %s 不再能牵制你了!",
                      mon_nam(u.ustuck));
            u.ustuck = 0;
        }
        /* kludge alert:
         * drown() and lava_effects() print various messages almost
         * every time they're called which conflict with the "fall
         * into" message below.  Thus, we want to avoid printing
         * confusing, duplicate or out-of-order messages.
         * Use knowledge of the two routines as a hack -- this
         * should really be handled differently -dlc
         */
        if (is_pool(u.ux, u.uy) && !Wwalking && !Swimming && !u.uinwater)
            no_msg = drown();

        if (is_lava(u.ux, u.uy)) {
            (void) lava_effects();
            no_msg = TRUE;
        }
    }
    if (!trap) {
        trap = t_at(u.ux, u.uy);
        if (Is_airlevel(&u.uz)) {
            You("开始就地跌倒.");
        } else if (Is_waterlevel(&u.uz) && !no_msg) {
            You_feel("沉重的.");
        /* u.uinwater msgs already in spoteffects()/drown() */
        } else if (!u.uinwater && !no_msg) {
            if (!(emask & W_SADDLE)) {
                if (Sokoban && trap) {
                    /* Justification elsewhere for Sokoban traps is based
                     * on air currents.  This is consistent with that.
                     * The unexpected additional force of the air currents
                     * once levitation ceases knocks you off your feet.
                     */
                    if (Hallucination)
                        pline("不愉快!  你崩溃了.");
                    else
                        You("跌倒");
                    losehp(rnd(2), "危险的风", KILLED_BY);
                    if (u.usteed)
                        dismount_steed(DISMOUNT_FELL);
                    selftouch("当你跌落, 你");
                } else if (u.usteed && (is_floater(u.usteed->data)
                                        || is_flyer(u.usteed->data))) {
                    You("更加坚固了鞍.");
                } else if (Hallucination) {
                    pline("不愉快!  你%s.",
                          is_pool(u.ux, u.uy)
                             ? "溅落了"
                             : "撞到地面");
                } else {
                    You("轻轻地飘到%s.", surface(u.ux, u.uy));
                }
            }
        }
    }

    /* can't rely on u.uz0 for detecting trap door-induced level change;
       it gets changed to reflect the new level before we can check it */
    assign_level(&current_dungeon_level, &u.uz);
    if (trap) {
        switch (trap->ttyp) {
        case STATUE_TRAP:
            break;
        case HOLE:
        case TRAPDOOR:
            if (!Can_fall_thru(&u.uz) || u.ustuck)
                break;
            /*FALLTHRU*/
        default:
            if (!u.utrap) /* not already in the trap */
                dotrap(trap, 0);
        }
    }
    if (!Is_airlevel(&u.uz) && !Is_waterlevel(&u.uz) && !u.uswallow
        /* falling through trap door calls goto_level,
           and goto_level does its own pickup() call */
        && on_level(&u.uz, &current_dungeon_level))
        (void) pickup(1);
    return 1;
}

/* shared code for climbing out of a pit */
void
climb_pit()
{
    if (!u.utrap || u.utraptype != TT_PIT)
        return;

    if (Passes_walls) {
        /* marked as trapped so they can pick things up */
        You("从坑里爬上来.");
        u.utrap = 0;
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (!rn2(2) && sobj_at(BOULDER, u.ux, u.uy)) {
        Your("%s 卡在了裂缝里.", body_part(LEG));
        display_nhwindow(WIN_MESSAGE, FALSE);
        clear_nhwindow(WIN_MESSAGE);
        You("解放了你的%s.", body_part(LEG));
    } else if ((Flying || is_clinger(youmonst.data)) && !Sokoban) {
        /* eg fell in pit, then poly'd to a flying monster;
           or used '>' to deliberately enter it */
        You("%s 出了坑.", Flying ? "飞" : "爬");
        u.utrap = 0;
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (!(--u.utrap)) {
        You("%s 到坑的边缘.",
            (Sokoban && Levitation)
                ? "与气流做斗争并飘"
                : u.usteed ? "乘骑" : "爬");
        fill_pit(u.ux, u.uy);
        vision_full_recalc = 1; /* vision limits change */
    } else if (u.dz || flags.verbose) {
        if (u.usteed)
            Norep("%s 仍然在坑里.", upstart(y_monnam(u.usteed)));
        else
            Norep((Hallucination && !rn2(5))
                      ? "你掉落了, 不能上来."
                      : "你仍然在坑里.");
    }
}

STATIC_OVL void
dofiretrap(box)
struct obj *box; /* null for floor trap */
{
    boolean see_it = !Blind;
    int num, alt;

    /* Bug: for box case, the equivalent of burn_floor_objects() ought
     * to be done upon its contents.
     */

    if ((box && !carried(box)) ? is_pool(box->ox, box->oy) : Underwater) {
        pline("瀑布状的气泡从%s喷出!",
              the(box ? xname(box) : surface(u.ux, u.uy)));
        if (Fire_resistance)
            You("没有受伤.");
        else
            losehp(rnd(3), "沸腾的水", KILLED_BY);
        return;
    }
    pline("%s从%s%s!", tower_of_flame,
          the(box ? xname(box) : surface(u.ux, u.uy)),
          box ? "爆发" : "喷出");
    if (Fire_resistance) {
        shieldeff(u.ux, u.uy);
        num = rn2(2);
    } else if (Upolyd) {
        num = d(2, 4);
        switch (u.umonnum) {
        case PM_PAPER_GOLEM:
            alt = u.mhmax;
            break;
        case PM_STRAW_GOLEM:
            alt = u.mhmax / 2;
            break;
        case PM_WOOD_GOLEM:
            alt = u.mhmax / 4;
            break;
        case PM_LEATHER_GOLEM:
            alt = u.mhmax / 8;
            break;
        default:
            alt = 0;
            break;
        }
        if (alt > num)
            num = alt;
        if (u.mhmax > mons[u.umonnum].mlevel)
            u.mhmax -= rn2(min(u.mhmax, num + 1)), context.botl = 1;
    } else {
        num = d(2, 4);
        if (u.uhpmax > u.ulevel)
            u.uhpmax -= rn2(min(u.uhpmax, num + 1)), context.botl = 1;
    }
    if (!num)
        You("没有受伤.");
    else
        losehp(num, tower_of_flame, KILLED_BY_AN); /* fire damage */
    burn_away_slime();

    if (burnarmor(&youmonst) || rn2(3)) {
        destroy_item(SCROLL_CLASS, AD_FIRE);
        destroy_item(SPBOOK_CLASS, AD_FIRE);
        destroy_item(POTION_CLASS, AD_FIRE);
    }
    if (!box && burn_floor_objects(u.ux, u.uy, see_it, TRUE) && !see_it)
        You("闻到纸在燃烧.");
    if (is_ice(u.ux, u.uy))
        melt_ice(u.ux, u.uy, (char *) 0);
}

STATIC_OVL void
domagictrap()
{
    register int fate = rnd(20);

    /* What happened to the poor sucker? */

    if (fate < 10) {
        /* Most of the time, it creates some monsters. */
        register int cnt = rnd(4);

        if (!resists_blnd(&youmonst)) {
            You("立即被闪光致失明!");
            make_blinded((long) rn1(5, 10), FALSE);
            if (!Blind)
                Your1(vision_clears);
        } else if (!Blind) {
            You_see("一道闪光!");
        } else
            You_hear("震耳欲聋的咆哮!");
        incr_itimeout(&HDeaf, rn1(20, 30));
        while (cnt--)
            (void) makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
    } else
        switch (fate) {
        case 10:
        case 11:
            /* sometimes nothing happens */
            break;
        case 12: /* a flash of fire */
            dofiretrap((struct obj *) 0);
            break;

        /* odd feelings */
        case 13:
            pline("颤抖在你的%s里跑上跑下!", body_part(SPINE));
            break;
        case 14:
            You_hear(Hallucination ? "月亮在对你嚎叫."
                                   : "远处的嚎叫.");
            break;
        case 15:
            if (on_level(&u.uz, &qstart_level))
                You_feel(
                    "%s像浪荡子.",
                    (flags.female || (Upolyd && is_neuter(youmonst.data)))
                        ? "古怪地"
                        : "");
            else
                You("突然思念%s.",
                    Hallucination
                        ? "克利夫兰"
                        : (In_quest(&u.uz) || at_dgn_entrance("任务"))  //The Quest
                              ? "你附近的故乡"
                              : "你遥远的故乡");
            break;
        case 16:
            Your("背包剧烈地震动!");
            break;
        case 17:
            You(Hallucination ? "闻到汉堡包." : "闻到烧焦的肉.");
            break;
        case 18:
            You_feel("累了.");
            break;

        /* very occasionally something nice happens. */
        case 19: { /* tame nearby monsters */
            int i, j;
            struct monst *mtmp;

            (void) adjattrib(A_CHA, 1, FALSE);
            for (i = -1; i <= 1; i++)
                for (j = -1; j <= 1; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    mtmp = m_at(u.ux + i, u.uy + j);
                    if (mtmp)
                        (void) tamedog(mtmp, (struct obj *) 0);
                }
            break;
        }
        case 20: { /* uncurse stuff */
            struct obj pseudo;
            long save_conf = HConfusion;

            pseudo = zeroobj; /* neither cursed nor blessed,
                                 and zero out oextra */
            pseudo.otyp = SCR_REMOVE_CURSE;
            HConfusion = 0L;
            (void) seffects(&pseudo);
            HConfusion = save_conf;
            break;
        }
        default:
            break;
        }
}

/* Set an item on fire.
 *   "force" means not to roll a luck-based protection check for the
 *     item.
 *   "x" and "y" are the coordinates to dump the contents of a
 *     container, if it burns up.
 *
 * Return whether the object was destroyed.
 */
boolean
fire_damage(obj, force, x, y)
struct obj *obj;
boolean force;
xchar x, y;
{
    int chance;
    struct obj *otmp, *ncobj;
    int in_sight = !Blind && couldsee(x, y); /* Don't care if it's lit */
    int dindx;

    /* object might light in a controlled manner */
    if (catch_lit(obj))
        return FALSE;

    if (Is_container(obj)) {
        switch (obj->otyp) {
        case ICE_BOX:
            return FALSE; /* Immune */
        case CHEST:
            chance = 40;
            break;
        case LARGE_BOX:
            chance = 30;
            break;
        default:
            chance = 20;
            break;
        }
        if ((!force && (Luck + 5) > rn2(chance))
            || (is_flammable(obj) && obj->oerodeproof))
            return FALSE;
        /* Container is burnt up - dump contents out */
        if (in_sight)
            pline("%s 着火并燃烧了.", Yname2(obj));
        if (Has_contents(obj)) {
            if (in_sight)
                pline("它里面的东西掉出来了.");
            for (otmp = obj->cobj; otmp; otmp = ncobj) {
                ncobj = otmp->nobj;
                obj_extract_self(otmp);
                if (!flooreffects(otmp, x, y, ""))
                    place_object(otmp, x, y);
            }
        }
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
          *     max luck (Luck==13):    10%
          *     avg luck (Luck==0):     75%
          *     awful luck (Luck<-4):  100%
          */
        return FALSE;
    } else if (obj->oclass == SCROLL_CLASS || obj->oclass == SPBOOK_CLASS) {
        if (obj->otyp == SCR_FIRE || obj->otyp == SPE_FIREBALL)
            return FALSE;
        if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
            if (in_sight)
                pline("烟从%s升起.", the(xname(obj)));
            return FALSE;
        }
        dindx = (obj->oclass == SCROLL_CLASS) ? 3 : 4;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (obj->oclass == POTION_CLASS) {
        dindx = (obj->otyp != POT_OIL) ? 1 : 2;
        if (in_sight)
            pline("%s %s.", Yname2(obj),
                  destroy_strings[dindx][(obj->quan > 1L)]);
        setnotworn(obj);
        delobj(obj);
        return TRUE;
    } else if (erode_obj(obj, (char *) 0, ERODE_BURN, EF_DESTROY)
               == ER_DESTROYED) {
        return TRUE;
    }
    return FALSE;
}

/*
 * Apply fire_damage() to an entire chain.
 *
 * Return number of objects destroyed. --ALI
 */
int
fire_damage_chain(chain, force, here, x, y)
struct obj *chain;
boolean force, here;
xchar x, y;
{
    struct obj *obj, *nobj;
    int num = 0;
    for (obj = chain; obj; obj = nobj) {
        nobj = here ? obj->nexthere : obj->nobj;
        if (fire_damage(obj, force, x, y))
            ++num;
    }

    if (num && (Blind && !couldsee(x, y)))
        You("闻到烟味.");
    return num;
}

void
acid_damage(obj)
struct obj *obj;
{
    /* Scrolls but not spellbooks can be erased by acid. */
    struct monst *victim;
    boolean vismon;

    if (!obj)
        return;

    victim = carried(obj) ? &youmonst : mcarried(obj) ? obj->ocarry : NULL;
    vismon = victim && (victim != &youmonst) && canseemon(victim);

    if (obj->greased) {
        grease_protect(obj, (char *) 0, victim);
    } else if (obj->oclass == SCROLL_CLASS && obj->otyp != SCR_BLANK_PAPER) {
        if (obj->otyp != SCR_BLANK_PAPER
#ifdef MAIL
            && obj->otyp != SCR_MAIL
#endif
            ) {
            if (!Blind) {
                if (victim == &youmonst)
                    pline("你的%s了.", aobjnam(obj, "褪色"));
                else if (vismon)
                    pline("%s %s了.", s_suffix(Monnam(victim)),
                          aobjnam(obj, "褪色"));
            }
        }
        obj->otyp = SCR_BLANK_PAPER;
        obj->spe = 0;
        obj->dknown = 0;
    } else
        erode_obj(obj, (char *) 0, ERODE_CORRODE, EF_GREASE | EF_VERBOSE);
}

/* context for water_damage(), managed by water_damage_chain();
   when more than one stack of potions of acid explode while processing
   a chain of objects, use alternate phrasing after the first message */
static struct h2o_ctx {
    int dkn_boom, unk_boom; /* track dknown, !dknown separately */
    boolean ctx_valid;
} acid_ctx = { 0, 0, FALSE };

/* Get an object wet and damage it appropriately.
 *   "ostr", if present, is used instead of the object name in some
 *     messages.
 *   "force" means not to roll luck to protect some objects.
 * Returns an erosion return value (ER_*)
 */
int
water_damage(obj, ostr, force)
struct obj *obj;
const char *ostr;
boolean force;
{
    if (!obj)
        return ER_NOTHING;

    if (snuff_lit(obj))
        return ER_DAMAGED;

    if (!ostr)
        ostr = cxname(obj);

    if (obj->otyp == CAN_OF_GREASE && obj->spe > 0) {
        return ER_NOTHING;
    } else if (obj->otyp == TOWEL && obj->spe < 7) {
        wet_a_towel(obj, rnd(7), TRUE);
        return ER_NOTHING;
    } else if (obj->greased) {
        if (!rn2(2))
            obj->greased = 0;
        if (carried(obj))
            update_inventory();
        return ER_GREASED;
    } else if (Is_container(obj) && !Is_box(obj)
               && (obj->otyp != OILSKIN_SACK || (obj->cursed && !rn2(3)))) {
        if (carried(obj))
            pline("水进入了你的%s!", ostr);

        water_damage_chain(obj->cobj, FALSE);
        return ER_NOTHING;
    } else if (!force && (Luck + 5) > rn2(20)) {
        /*  chance per item of sustaining damage:
            *   max luck:               10%
            *   avg luck (Luck==0):     75%
            *   awful luck (Luck<-4):  100%
            */
        return ER_NOTHING;
    } else if (obj->oclass == SCROLL_CLASS) {
#ifdef MAIL
        if (obj->otyp == SCR_MAIL)
            return 0;
#endif
        if (carried(obj))
            pline("你的%s %s了.", ostr, vtense(ostr, "褪色"));

        obj->otyp = SCR_BLANK_PAPER;
        obj->dknown = 0;
        obj->spe = 0;
        if (carried(obj))
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == SPBOOK_CLASS) {
        if (obj->otyp == SPE_BOOK_OF_THE_DEAD) {
            pline("蒸汽从%s升起.", the(xname(obj)));
            return 0;
        }

        if (carried(obj))
            pline("你的%s %s了.", ostr, vtense(ostr, "褪色"));

        if (obj->otyp == SPE_NOVEL) {
            obj->novelidx = 0;
            free_oname(obj);
        }

        obj->otyp = SPE_BLANK_PAPER;
        obj->dknown = 0;
        if (carried(obj))
            update_inventory();
        return ER_DAMAGED;
    } else if (obj->oclass == POTION_CLASS) {
        if (obj->otyp == POT_ACID) {
            char *bufp;
            boolean one = (obj->quan == 1L), update = carried(obj),
                    exploded = FALSE;

            if (Blind && !carried(obj))
                obj->dknown = 0;
            if (acid_ctx.ctx_valid)
                exploded = ((obj->dknown ? acid_ctx.dkn_boom
                                         : acid_ctx.unk_boom) > 0);
            /* First message is
             * "a [potion|<color> potion|potion of acid] explodes"
             * depending on obj->dknown (potion has been seen) and
             * objects[POT_ACID].oc_name_known (fully discovered),
             * or "some {plural version} explode" when relevant.
             * Second and subsequent messages for same chain and
             * matching dknown status are
             * "another [potion|<color> &c] explodes" or plural
             * variant.
             */
            bufp = simpleonames(obj);
            pline("%s %s %s了!", /* "A potion explodes!" */
                  !exploded ? (one ? "一瓶" : "一些")
                            : (one ? "另一个" : "更多的"),
                  bufp, vtense(bufp, "爆炸"));
            if (acid_ctx.ctx_valid) {
                if (obj->dknown)
                    acid_ctx.dkn_boom++;
                else
                    acid_ctx.unk_boom++;
            }
            setnotworn(obj);
            delobj(obj);
            if (update)
                update_inventory();
            return ER_DESTROYED;
        } else if (obj->odiluted) {
            if (carried(obj))
                pline("你的%s更加%s了.", ostr, vtense(ostr, "稀释"));

            obj->otyp = POT_WATER;
            obj->dknown = 0;
            obj->blessed = obj->cursed = 0;
            obj->odiluted = 0;
            if (carried(obj))
                update_inventory();
            return ER_DAMAGED;
        } else if (obj->otyp != POT_WATER) {
            if (carried(obj))
                pline("你的%s %s了.", ostr, vtense(ostr, "稀释"));

            obj->odiluted++;
            if (carried(obj))
                update_inventory();
            return ER_DAMAGED;
        }
    } else {
        return erode_obj(obj, ostr, ERODE_RUST, EF_NONE);
    }
    return ER_NOTHING;
}

void
water_damage_chain(obj, here)
struct obj *obj;
boolean here;
{
    struct obj *otmp;

    /* initialize acid context: so far, neither seen (dknown) potions of
       acid nor unseen have exploded during this water damage sequence */
    acid_ctx.dkn_boom = acid_ctx.unk_boom = 0;
    acid_ctx.ctx_valid = TRUE;

    for (; obj; obj = otmp) {
        otmp = here ? obj->nexthere : obj->nobj;
        water_damage(obj, (char *) 0, FALSE);
    }

    /* reset acid context */
    acid_ctx.dkn_boom = acid_ctx.unk_boom = 0;
    acid_ctx.ctx_valid = FALSE;
}

/*
 * This function is potentially expensive - rolling
 * inventory list multiple times.  Luckily it's seldom needed.
 * Returns TRUE if disrobing made player unencumbered enough to
 * crawl out of the current predicament.
 */
STATIC_OVL boolean
emergency_disrobe(lostsome)
boolean *lostsome;
{
    int invc = inv_cnt(TRUE);

    while (near_capacity() > (Punished ? UNENCUMBERED : SLT_ENCUMBER)) {
        register struct obj *obj, *otmp = (struct obj *) 0;
        register int i;

        /* Pick a random object */
        if (invc > 0) {
            i = rn2(invc);
            for (obj = invent; obj; obj = obj->nobj) {
                /*
                 * Undroppables are: body armor, boots, gloves,
                 * amulets, and rings because of the time and effort
                 * in removing them + loadstone and other cursed stuff
                 * for obvious reasons.
                 */
                if (!((obj->otyp == LOADSTONE && obj->cursed) || obj == uamul
                      || obj == uleft || obj == uright || obj == ublindf
                      || obj == uarm || obj == uarmc || obj == uarmg
                      || obj == uarmf || obj == uarmu
                      || (obj->cursed && (obj == uarmh || obj == uarms))
                      || welded(obj)))
                    otmp = obj;
                /* reached the mark and found some stuff to drop? */
                if (--i < 0 && otmp)
                    break;

                /* else continue */
            }
        }
        if (!otmp)
            return FALSE; /* nothing to drop! */
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        *lostsome = TRUE;
        dropx(otmp);
        invc--;
    }
    return TRUE;
}


/*  return TRUE iff player relocated */
boolean
drown()
{
    const char *pool_of_water;
    boolean inpool_ok = FALSE, crawl_ok;
    int i, x, y;

    /* happily wading in the same contiguous pool */
    if (u.uinwater && is_pool(u.ux - u.dx, u.uy - u.dy)
        && (Swimming || Amphibious)) {
        /* water effects on objects every now and then */
        if (!rn2(5))
            inpool_ok = TRUE;
        else
            return FALSE;
    }

    if (!u.uinwater) {
        You("%s 进水里%c", Is_waterlevel(&u.uz) ? "跳" : "落",
            Amphibious || Swimming ? '.' : '!');
        if (!Swimming && !Is_waterlevel(&u.uz))
            You("下沉得像%s.", Hallucination ? "泰坦尼克号" : "一块岩石");
    }

    water_damage_chain(invent, FALSE);

    if (u.umonnum == PM_GREMLIN && rn2(3))
        (void) split_mon(&youmonst, (struct monst *) 0);
    else if (u.umonnum == PM_IRON_GOLEM) {
        You("生锈了!");
        i = Maybe_Half_Phys(d(2, 6));
        if (u.mhmax > i)
            u.mhmax -= i;
        losehp(i, "锈掉", KILLED_BY);
    }
    if (inpool_ok)
        return FALSE;

    if ((i = number_leashed()) > 0) {
        pline_The("狗链%s滑动%s松了.", (i > 1) ? "" : "",
                  (i > 1) ? "" : "");
        unleash_all();
    }

    if (Amphibious || Swimming) {
        if (Amphibious) {
            if (flags.verbose)
                pline("但你并没有淹死.");
            if (!Is_waterlevel(&u.uz)) {
                if (Hallucination)
                    Your("平底船撞到了底部.");
                else
                    You("到达底部.");
            }
        }
        if (Punished) {
            unplacebc();
            placebc();
        }
        vision_recalc(2); /* unsee old position */
        u.uinwater = 1;
        under_water(1);
        vision_full_recalc = 1;
        return FALSE;
    }
    if ((Teleportation || can_teleport(youmonst.data)) && !Unaware
        && (Teleport_control || rn2(3) < Luck + 2)) {
        You("尝试一个传送魔法."); /* utcsri!carroll */
        if (!level.flags.noteleport) {
            (void) dotele();
            if (!is_pool(u.ux, u.uy))
                return TRUE;
        } else
            pline_The("尝试的传送魔法失败了.");
    }
    if (u.usteed) {
        dismount_steed(DISMOUNT_GENERIC);
        if (!is_pool(u.ux, u.uy))
            return TRUE;
    }
    crawl_ok = FALSE;
    x = y = 0; /* lint suppression */
    /* if sleeping, wake up now so that we don't crawl out of water
       while still asleep; we can't do that the same way that waking
       due to combat is handled; note unmul() clears u.usleep */
    if (u.usleep)
        unmul("突然你醒了!");
    /* being doused will revive from fainting */
    if (is_fainted())
        reset_faint();
    /* can't crawl if unable to move (crawl_ok flag stays false) */
    if (multi < 0 || (Upolyd && !youmonst.data->mmove))
        goto crawl;
    /* look around for a place to crawl to */
    for (i = 0; i < 100; i++) {
        x = rn1(3, u.ux - 1);
        y = rn1(3, u.uy - 1);
        if (crawl_destination(x, y)) {
            crawl_ok = TRUE;
            goto crawl;
        }
    }
    /* one more scan */
    for (x = u.ux - 1; x <= u.ux + 1; x++)
        for (y = u.uy - 1; y <= u.uy + 1; y++)
            if (crawl_destination(x, y)) {
                crawl_ok = TRUE;
                goto crawl;
            }
crawl:
    if (crawl_ok) {
        boolean lost = FALSE;
        /* time to do some strip-tease... */
        boolean succ = Is_waterlevel(&u.uz) ? TRUE : emergency_disrobe(&lost);

        You("试图爬出水面.");
        if (lost)
            You("倒出一些你的齿轮来减轻重量...");
        if (succ) {
            pline("呼!  好险.");
            teleds(x, y, TRUE);
            return TRUE;
        }
        /* still too much weight */
        pline("但没用.");
    }
    u.uinwater = 1;
    You("淹死了.");
    for (i = 0; i < 5; i++) { /* arbitrary number of loops */
        /* killer format and name are reconstructed every iteration
           because lifesaving resets them */
        pool_of_water = waterbody_name(u.ux, u.uy);
        killer.format = KILLED_BY_AN;
        /* avoid "drowned in [a] water" */
        if (!strcmp(pool_of_water, "水"))
            pool_of_water = "深水", killer.format = KILLED_BY;
        Strcpy(killer.name, pool_of_water);
        done(DROWNING);
        /* oops, we're still alive.  better get out of the water. */
        if (safe_teleds(TRUE))
            break; /* successful life-save */
        /* nowhere safe to land; repeat drowning loop... */
        pline("你还是被淹死了.");
    }
    if (u.uinwater) {
        u.uinwater = 0;
        You("发现你自己回到%s.",
            Is_waterlevel(&u.uz) ? "空气中的气泡中" : "陆地上");
    }
    return TRUE;
}

void
drain_en(n)
int n;
{
    if (!u.uenmax) {
        /* energy is completely gone */
        You_feel("立即无生气的.");
    } else {
        /* throttle further loss a bit when there's not much left to lose */
        if (n > u.uenmax || n > u.ulevel)
            n = rnd(n);

        You_feel("你的魔法能量渐渐枯竭%c", (n > u.uen) ? '!' : '.');
        u.uen -= n;
        if (u.uen < 0) {
            u.uenmax -= rnd(-u.uen);
            if (u.uenmax < 0)
                u.uenmax = 0;
            u.uen = 0;
        }
        context.botl = 1;
    }
}

/* disarm a trap */
int
dountrap()
{
    if (near_capacity() >= HVY_ENCUMBER) {
        pline("你做那个太紧张了.");
        return 0;
    }
    if ((nohands(youmonst.data) && !webmaker(youmonst.data))
        || !youmonst.data->mmove) {
        pline("只是你想怎么做?");
        return 0;
    } else if (u.ustuck && sticks(youmonst.data)) {
        pline("你将不得不首先放弃%s.", mon_nam(u.ustuck));
        return 0;
    }
    if (u.ustuck || (welded(uwep) && bimanual(uwep))) {
        Your("%s 似乎太忙碌了.", makeplural(body_part(HAND)));
        return 0;
    }
    return untrap(FALSE);
}

/* Probability of disabling a trap.  Helge Hafting */
STATIC_OVL int
untrap_prob(ttmp)
struct trap *ttmp;
{
    int chance = 3;

    /* Only spiders know how to deal with webs reliably */
    if (ttmp->ttyp == WEB && !webmaker(youmonst.data))
        chance = 30;
    if (Confusion || Hallucination)
        chance++;
    if (Blind)
        chance++;
    if (Stunned)
        chance += 2;
    if (Fumbling)
        chance *= 2;
    /* Your own traps are better known than others. */
    if (ttmp && ttmp->madeby_u)
        chance--;
    if (Role_if(PM_ROGUE)) {
        if (rn2(2 * MAXULEV) < u.ulevel)
            chance--;
        if (u.uhave.questart && chance > 1)
            chance--;
    } else if (Role_if(PM_RANGER) && chance > 1)
        chance--;
    return rn2(chance);
}

/* Replace trap with object(s).  Helge Hafting */
void
cnv_trap_obj(otyp, cnt, ttmp, bury_it)
int otyp;
int cnt;
struct trap *ttmp;
boolean bury_it;
{
    struct obj *otmp = mksobj(otyp, TRUE, FALSE);

    otmp->quan = cnt;
    otmp->owt = weight(otmp);
    /* Only dart traps are capable of being poisonous */
    if (otyp != DART)
        otmp->opoisoned = 0;
    place_object(otmp, ttmp->tx, ttmp->ty);
    if (bury_it) {
        /* magical digging first disarms this trap, then will unearth it */
        (void) bury_an_obj(otmp, (boolean *) 0);
    } else {
        /* Sell your own traps only... */
        if (ttmp->madeby_u)
            sellobj(otmp, ttmp->tx, ttmp->ty);
        stackobj(otmp);
    }
    newsym(ttmp->tx, ttmp->ty);
    if (u.utrap && ttmp->tx == u.ux && ttmp->ty == u.uy)
        u.utrap = 0;
    deltrap(ttmp);
}

/* while attempting to disarm an adjacent trap, we've fallen into it */
STATIC_OVL void
move_into_trap(ttmp)
struct trap *ttmp;
{
    int bc = 0;
    xchar x = ttmp->tx, y = ttmp->ty, bx, by, cx, cy;
    boolean unused;

    bx = by = cx = cy = 0; /* lint suppression */
    /* we know there's no monster in the way, and we're not trapped */
    if (!Punished
        || drag_ball(x, y, &bc, &bx, &by, &cx, &cy, &unused, TRUE)) {
        u.ux0 = u.ux, u.uy0 = u.uy;
        u.ux = x, u.uy = y;
        u.umoved = TRUE;
        newsym(u.ux0, u.uy0);
        vision_recalc(1);
        check_leash(u.ux0, u.uy0);
        if (Punished)
            move_bc(0, bc, bx, by, cx, cy);
        /* marking the trap unseen forces dotrap() to treat it like a new
           discovery and prevents pickup() -> look_here() -> check_here()
           from giving a redundant "there is a <trap> here" message when
           there are objects covering this trap */
        ttmp->tseen = 0; /* hack for check_here() */
        /* trigger the trap */
        spoteffects(TRUE); /* pickup() + dotrap() */
        exercise(A_WIS, FALSE);
    }
}

/* 0: doesn't even try
 * 1: tries and fails
 * 2: succeeds
 */
STATIC_OVL int
try_disarm(ttmp, force_failure)
struct trap *ttmp;
boolean force_failure;
{
    struct monst *mtmp = m_at(ttmp->tx, ttmp->ty);
    int ttype = ttmp->ttyp;
    boolean under_u = (!u.dx && !u.dy);
    boolean holdingtrap = (ttype == BEAR_TRAP || ttype == WEB);

    /* Test for monster first, monsters are displayed instead of trap. */
    if (mtmp && (!mtmp->mtrapped || !holdingtrap)) {
        pline("%s 挡在路上.", Monnam(mtmp));
        return 0;
    }
    /* We might be forced to move onto the trap's location. */
    if (sobj_at(BOULDER, ttmp->tx, ttmp->ty) && !Passes_walls && !under_u) {
        There("有巨石挡在你前面.");
        return 0;
    }
    /* duplicate tight-space checks from test_move */
    if (u.dx && u.dy && bad_rock(youmonst.data, u.ux, ttmp->ty)
        && bad_rock(youmonst.data, ttmp->tx, u.uy)) {
        if ((invent && (inv_weight() + weight_cap() > 600))
            || bigmonst(youmonst.data)) {
            /* don't allow untrap if they can't get thru to it */
            You("不能够到%s!",
                defsyms[trap_to_defsym(ttype)].explanation);
            return 0;
        }
    }
    /* untrappable traps are located on the ground. */
    if (!can_reach_floor(TRUE)) {
        if (u.usteed && P_SKILL(P_RIDING) < P_BASIC)
            rider_cant_reach();
        else
            You("不能够到%s!",
                defsyms[trap_to_defsym(ttype)].explanation);
        return 0;
    }

    /* Will our hero succeed? */
    if (force_failure || untrap_prob(ttmp)) {
        if (rnl(5)) {
            pline("哎呀...");
            if (mtmp) { /* must be a trap that holds monsters */
                if (ttype == BEAR_TRAP) {
                    if (mtmp->mtame)
                        abuse_dog(mtmp);
                    if ((mtmp->mhp -= rnd(4)) <= 0)
                        killed(mtmp);
                } else if (ttype == WEB) {
                    if (!webmaker(youmonst.data)) {
                        struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);

                        if (ttmp2) {
                            pline_The(
                                "网粘住了你. 你也陷入了!");
                            dotrap(ttmp2, NOWEBMSG);
                            if (u.usteed && u.utrap) {
                                /* you, not steed, are trapped */
                                dismount_steed(DISMOUNT_FELL);
                            }
                        }
                    } else
                        pline("%s 仍然被缠住.", Monnam(mtmp));
                }
            } else if (under_u) {
                dotrap(ttmp, 0);
            } else {
                move_into_trap(ttmp);
            }
        } else {
            pline("%s %s很难%s.",
                  ttmp->madeby_u ? "你的" : under_u ? "这个" : "那个",
                  defsyms[trap_to_defsym(ttype)].explanation,
                  (ttype == WEB) ? "移除" : "解除");
        }
        return 1;
    }
    return 2;
}

STATIC_OVL void
reward_untrap(ttmp, mtmp)
struct trap *ttmp;
struct monst *mtmp;
{
    if (!ttmp->madeby_u) {
        if (rnl(10) < 8 && !mtmp->mpeaceful && !mtmp->msleeping
            && !mtmp->mfrozen && !mindless(mtmp->data)
            && mtmp->data->mlet != S_HUMAN) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
            pline("%s 很感谢.", Monnam(mtmp));
        }
        /* Helping someone out of a trap is a nice thing to do,
         * A lawful may be rewarded, but not too often.  */
        if (!rn2(3) && !rnl(8) && u.ualign.type == A_LAWFUL) {
            adjalign(1);
            You_feel("你做了正确的事.");
        }
    }
}

STATIC_OVL int
disarm_holdingtrap(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
    struct monst *mtmp;
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;

    /* ok, disarm it. */

    /* untrap the monster, if any.
       There's no need for a cockatrice test, only the trap is touched */
    if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
        mtmp->mtrapped = 0;
        You("从%s移除了%s%s.", mon_nam(mtmp),
            the_your[ttmp->madeby_u],
            (ttmp->ttyp == BEAR_TRAP) ? "捕兽夹" : "网");
        reward_untrap(ttmp, mtmp);
    } else {
        if (ttmp->ttyp == BEAR_TRAP) {
            You("解除了%s捕兽夹.", the_your[ttmp->madeby_u]);
            cnv_trap_obj(BEARTRAP, 1, ttmp, FALSE);
        } else /* if (ttmp->ttyp == WEB) */ {
            You("成功地移除了%s网.", the_your[ttmp->madeby_u]);
            deltrap(ttmp);
        }
    }
    newsym(u.ux + u.dx, u.uy + u.dy);
    return 1;
}

STATIC_OVL int
disarm_landmine(ttmp) /* Helge Hafting */
struct trap *ttmp;
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
    You("解除了%s地雷.", the_your[ttmp->madeby_u]);
    cnv_trap_obj(LAND_MINE, 1, ttmp, FALSE);
    return 1;
}

/* getobj will filter down to cans of grease and known potions of oil */
static NEARDATA const char oil[] = { ALL_CLASSES, TOOL_CLASS, POTION_CLASS,
                                     0 };

/* it may not make much sense to use grease on floor boards, but so what? */
STATIC_OVL int
disarm_squeaky_board(ttmp)
struct trap *ttmp;
{
    struct obj *obj;
    boolean bad_tool;
    int fails;

    obj = getobj(oil, "解除使用");  //untrap with
    if (!obj)
        return 0;

    bad_tool = (obj->cursed
                || ((obj->otyp != POT_OIL || obj->lamplit)
                    && (obj->otyp != CAN_OF_GREASE || !obj->spe)));
    fails = try_disarm(ttmp, bad_tool);
    if (fails < 2)
        return fails;

    /* successfully used oil or grease to fix squeaky board */
    if (obj->otyp == CAN_OF_GREASE) {
        consume_obj_charge(obj, TRUE);
    } else {
        useup(obj); /* oil */
        makeknown(POT_OIL);
    }
    You("修复了尖板."); /* no madeby_u */
    deltrap(ttmp);
    newsym(u.ux + u.dx, u.uy + u.dy);
    more_experienced(1, 5);
    newexplevel();
    return 1;
}

/* removes traps that shoot arrows, darts, etc. */
STATIC_OVL int
disarm_shooting_trap(ttmp, otyp)
struct trap *ttmp;
int otyp;
{
    int fails = try_disarm(ttmp, FALSE);

    if (fails < 2)
        return fails;
    You("解除了%s陷阱.", the_your[ttmp->madeby_u]);
    cnv_trap_obj(otyp, 50 - rnl(50), ttmp, FALSE);
    return 1;
}

/* Is the weight too heavy?
 * Formula as in near_capacity() & check_capacity() */
STATIC_OVL int
try_lift(mtmp, ttmp, wt, stuff)
struct monst *mtmp;
struct trap *ttmp;
int wt;
boolean stuff;
{
    int wc = weight_cap();

    if (((wt * 2) / wc) >= HVY_ENCUMBER) {
        pline("%s %s 来让你搬起.", Monnam(mtmp),
              stuff ? "携带了太多的东西" : "太重了");
        if (!ttmp->madeby_u && !mtmp->mpeaceful && mtmp->mcanmove
            && !mindless(mtmp->data) && mtmp->data->mlet != S_HUMAN
            && rnl(10) < 3) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp); /* reset alignment */
            pline("%s 认为你很高兴尝试.", Monnam(mtmp));
        }
        return 0;
    }
    return 1;
}

/* Help trapped monster (out of a (spiked) pit) */
STATIC_OVL int
help_monster_out(mtmp, ttmp)
struct monst *mtmp;
struct trap *ttmp;
{
    int wt;
    struct obj *otmp;
    boolean uprob;

    /*
     * This works when levitating too -- consistent with the ability
     * to hit monsters while levitating.
     *
     * Should perhaps check that our hero has arms/hands at the
     * moment.  Helping can also be done by engulfing...
     *
     * Test the monster first - monsters are displayed before traps.
     */
    if (!mtmp->mtrapped) {
        pline("%s 没有受困.", Monnam(mtmp));
        return 0;
    }
    /* Do you have the necessary capacity to lift anything? */
    if (check_capacity((char *) 0))
        return 1;

    /* Will our hero succeed? */
    if ((uprob = untrap_prob(ttmp)) && !mtmp->msleeping && mtmp->mcanmove) {
        You("试图伸出你的%s, 但是%s 怀疑地逐渐后退.",
            makeplural(body_part(ARM)), mon_nam(mtmp));
        return 1;
    }

    /* is it a cockatrice?... */
    if (touch_petrifies(mtmp->data) && !uarmg && !Stone_resistance) {
        You("用你的光%s抓住了受困的%s.",
            makeplural(body_part(HAND)),
            mtmp->data->mname);

        if (poly_when_stoned(youmonst.data) && polymon(PM_STONE_GOLEM)) {
            display_nhwindow(WIN_MESSAGE, FALSE);
        } else {
            char kbuf[BUFSZ];

            Sprintf(kbuf, "试图帮助%s 出坑",
                    mtmp->data->mname);
            instapetrify(kbuf);
            return 1;
        }
    }
    /* need to do cockatrice check first if sleeping or paralyzed */
    if (uprob) {
        You("试图抓住%s, 但是没能抓紧.", mon_nam(mtmp));
        if (mtmp->msleeping) {
            mtmp->msleeping = 0;
            pline("%s 醒了.", Monnam(mtmp));
        }
        return 1;
    }

    You("伸出你的%s并抓住%s.", makeplural(body_part(ARM)),
        mon_nam(mtmp));

    if (mtmp->msleeping) {
        mtmp->msleeping = 0;
        pline("%s 醒了.", Monnam(mtmp));
    } else if (mtmp->mfrozen && !rn2(mtmp->mfrozen)) {
        /* After such manhandling, perhaps the effect wears off */
        mtmp->mcanmove = 1;
        mtmp->mfrozen = 0;
        pline("%s 激起.", Monnam(mtmp));
    }

    /* is the monster too heavy? */
    wt = inv_weight() + mtmp->data->cwt;
    if (!try_lift(mtmp, ttmp, wt, FALSE))
        return 1;

    /* is the monster with inventory too heavy? */
    for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
        wt += otmp->owt;
    if (!try_lift(mtmp, ttmp, wt, TRUE))
        return 1;

    You("把%s 拉出了坑.", mon_nam(mtmp));
    mtmp->mtrapped = 0;
    fill_pit(mtmp->mx, mtmp->my);
    reward_untrap(ttmp, mtmp);
    return 1;
}

int
untrap(force)
boolean force;
{
    register struct obj *otmp;
    register int x, y;
    int ch;
    struct trap *ttmp;
    struct monst *mtmp;
    const char *trapdescr;
    boolean here, useplural, confused = (Confusion || Hallucination),
                             trap_skipped = FALSE, deal_with_floor_trap;
    int boxcnt = 0;
    char the_trap[BUFSZ], qbuf[QBUFSZ];

    if (!getdir((char *) 0))
        return 0;
    x = u.ux + u.dx;
    y = u.uy + u.dy;
    if (!isok(x, y)) {
        pline_The("危险潜伏在你的掌握之外.");
        return 0;
    }
    ttmp = t_at(x, y);
    if (ttmp && !ttmp->tseen)
        ttmp = 0;
    trapdescr = ttmp ? defsyms[trap_to_defsym(ttmp->ttyp)].explanation : 0;
    here = (x == u.ux && y == u.uy); /* !u.dx && !u.dy */

    if (here) /* are there are one or more containers here? */
        for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
            if (Is_box(otmp)) {
                if (++boxcnt > 1)
                    break;
            }

    deal_with_floor_trap = can_reach_floor(FALSE);
    if (!deal_with_floor_trap) {
        *the_trap = '\0';
        if (ttmp)
            Strcat(the_trap, trapdescr);
        if (ttmp && boxcnt)
            Strcat(the_trap, "和");
        if (boxcnt)
            Strcat(the_trap, (boxcnt == 1) ? "一个容器" : "一些容器");
        useplural = ((ttmp && boxcnt > 0) || boxcnt > 1);
        /* note: boxcnt and useplural will always be 0 for !here case */
        if (ttmp || boxcnt)
            There("%s %s %s 但是你不能够到%s%s.",
                  useplural ? "有" : "有", the_trap, here ? "" : "",
                  useplural ? "他们" : "它",
                  u.usteed ? "在乘骑的时候" : "");
        trap_skipped = (ttmp != 0);
    } else { /* deal_with_floor_trap */

        if (ttmp) {
            Strcpy(the_trap, the(trapdescr));
            if (boxcnt) {
                if (ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT) {
                    You_cant("对%s做很多%s.", the_trap,
                             u.utrap ? "因为你卡在里面"
                                     : "当站在它的边缘的时候");
                    trap_skipped = TRUE;
                    deal_with_floor_trap = FALSE;
                } else {
                    Sprintf(
                        qbuf, "这里有%s和%s. %s %s?",
                        (boxcnt == 1) ? "一个容器" : "一些容器",
                        trapdescr,
                        (ttmp->ttyp == WEB) ? "移除" : "解除", the_trap);
                    switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'n':
                        trap_skipped = TRUE;
                        deal_with_floor_trap = FALSE;
                        break;
                    }
                }
            }
            if (deal_with_floor_trap) {
                if (u.utrap) {
                    You("不能处理%s当被困%s!", the_trap,
                        (x == u.ux && y == u.uy) ? "于它" : "");
                    return 1;
                }
                if ((mtmp = m_at(x, y)) != 0
                    && (mtmp->m_ap_type == M_AP_FURNITURE
                        || mtmp->m_ap_type == M_AP_OBJECT)) {
                    stumble_onto_mimic(mtmp);
                    return 1;
                }
                switch (ttmp->ttyp) {
                case BEAR_TRAP:
                case WEB:
                    return disarm_holdingtrap(ttmp);
                case LANDMINE:
                    return disarm_landmine(ttmp);
                case SQKY_BOARD:
                    return disarm_squeaky_board(ttmp);
                case DART_TRAP:
                    return disarm_shooting_trap(ttmp, DART);
                case ARROW_TRAP:
                    return disarm_shooting_trap(ttmp, ARROW);
                case PIT:
                case SPIKED_PIT:
                    if (here) {
                        You("已经在坑的边缘了.");
                        return 0;
                    }
                    if (!mtmp) {
                        pline("试着填坑.");
                        return 0;
                    }
                    return help_monster_out(mtmp, ttmp);
                default:
                    You("不能禁止%s陷阱.", !here ? "那个" : "这个");
                    return 0;
                }
            }
        } /* end if */

        if (boxcnt) {
            for (otmp = level.objects[x][y]; otmp; otmp = otmp->nexthere)
                if (Is_box(otmp)) {
                    (void) safe_qbuf(qbuf, "这里有",
                                     ".  检查它的陷阱?", otmp,
                                     doname, ansimpleoname, "a box");
                    switch (ynq(qbuf)) {
                    case 'q':
                        return 0;
                    case 'n':
                        continue;
                    }

                    if ((otmp->otrapped
                         && (force || (!confused
                                       && rn2(MAXULEV + 1 - u.ulevel) < 10)))
                        || (!force && confused && !rn2(3))) {
                        You("发现一个在%s上的陷阱!", the(xname(otmp)));
                        if (!confused)
                            exercise(A_WIS, TRUE);

                        switch (ynq("解除它?")) {
                        case 'q':
                            return 1;
                        case 'n':
                            trap_skipped = TRUE;
                            continue;
                        }

                        if (otmp->otrapped) {
                            exercise(A_DEX, TRUE);
                            ch = ACURR(A_DEX) + u.ulevel;
                            if (Role_if(PM_ROGUE))
                                ch *= 2;
                            if (!force && (confused || Fumbling
                                           || rnd(75 + level_difficulty() / 2)
                                                  > ch)) {
                                (void) chest_trap(otmp, FINGER, TRUE);
                            } else {
                                You("解除了它!");
                                otmp->otrapped = 0;
                            }
                        } else
                            pline("那个%s 没有被困.", xname(otmp));
                        return 1;
                    } else {
                        You("没有发现%s上的陷阱.", the(xname(otmp)));
                        return 1;
                    }
                }

            You(trap_skipped ? "在这儿没有发现别的陷阱."
                             : "知道在这儿没有陷阱.");
            return 0;
        }

        if (stumble_on_door_mimic(x, y))
            return 1;

    } /* deal_with_floor_trap */
    /* doors can be manipulated even while levitating/unskilled riding */

    if (!IS_DOOR(levl[x][y].typ)) {
        if (!trap_skipped)
            You("知道那里没有陷阱.");
        return 0;
    }

    switch (levl[x][y].doormask) {
    case D_NODOOR:
        You("%s那里没有门.", Blind ? "感觉" : "看见");
        return 0;
    case D_ISOPEN:
        pline("这个门是安全打开的.");
        return 0;
    case D_BROKEN:
        pline("这个门被破坏了.");
        return 0;
    }

    if ((levl[x][y].doormask & D_TRAPPED
         && (force || (!confused && rn2(MAXULEV - u.ulevel + 11) < 10)))
        || (!force && confused && !rn2(3))) {
        You("发现门上的陷阱!");
        exercise(A_WIS, TRUE);
        if (ynq("解除它?") != 'y')
            return 1;
        if (levl[x][y].doormask & D_TRAPPED) {
            ch = 15 + (Role_if(PM_ROGUE) ? u.ulevel * 3 : u.ulevel);
            exercise(A_DEX, TRUE);
            if (!force && (confused || Fumbling
                           || rnd(75 + level_difficulty() / 2) > ch)) {
                You("引爆了它!");
                b_trapped("门", FINGER);
                levl[x][y].doormask = D_NODOOR;
                unblock_point(x, y);
                newsym(x, y);
                /* (probably ought to charge for this damage...) */
                if (*in_rooms(x, y, SHOPBASE))
                    add_damage(x, y, 0L);
            } else {
                You("解除了它!");
                levl[x][y].doormask &= ~D_TRAPPED;
            }
        } else
            pline("这个门没有陷阱.");
        return 1;
    } else {
        You("没有发现门上有陷阱.");
        return 1;
    }
}

/* for magic unlocking; returns true if targetted monster (which might
   be hero) gets untrapped; the trap remains intact */
boolean
openholdingtrap(mon, noticed)
struct monst *mon;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    char buf[BUFSZ];
    const char *trapdescr, *which;
    boolean ishero = (mon == &youmonst);

    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a holding trap, we're done */
    if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
        return FALSE;

    trapdescr = defsyms[trap_to_defsym(t->ttyp)].explanation;
    which = t->tseen ? the_your[t->madeby_u]
                     : index(vowels, *trapdescr) ? "" : "";

    if (ishero) {
        if (!u.utrap)
            return FALSE;
        u.utrap = 0; /* released regardless of type */
        *noticed = TRUE;
        /* give message only if trap was the expected type */
        if (u.utraptype == TT_BEARTRAP || u.utraptype == TT_WEB) {
            if (u.usteed)
                Sprintf(buf, "%s", noit_Monnam(u.usteed));
            else
                Strcpy(buf, "你");
            pline("%s 从%s%s被释放.", buf, which, trapdescr);
        }
    } else {
        if (!mon->mtrapped)
            return FALSE;
        mon->mtrapped = 0;
        if (canspotmon(mon)) {
            *noticed = TRUE;
            pline("%s 从%s%s被释放.", Monnam(mon), which,
                  trapdescr);
        } else if (cansee(t->tx, t->ty) && t->tseen) {
            *noticed = TRUE;
            if (t->ttyp == WEB)
                pline("%s 从%s%s被释放.", Something, which,
                      trapdescr);
            else /* BEAR_TRAP */
                pline("%s%s打开了.", upstart(strcpy(buf, which)), trapdescr);
        }
        /* might pacify monster if adjacent */
        if (rn2(2) && distu(mon->mx, mon->my) <= 2)
            reward_untrap(t, mon);
    }
    return TRUE;
}

/* for magic locking; returns true if targetted monster (which might
   be hero) gets hit by a trap (might avoid actually becoming trapped) */
boolean
closeholdingtrap(mon, noticed)
struct monst *mon;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    unsigned dotrapflags;
    boolean ishero = (mon == &youmonst), result;

    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a holding trap, we're done */
    if (!t || (t->ttyp != BEAR_TRAP && t->ttyp != WEB))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrapflags = FORCETRAP;
        /* dotrap calls mintrap when mounted hero encounters a web */
        if (u.usteed)
            dotrapflags |= NOWEBMSG;
        ++force_mintrap;
        dotrap(t, dotrapflags);
        --force_mintrap;
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        ++force_mintrap;
        result = (mintrap(mon) != 0);
        --force_mintrap;
    }
    return result;
}

/* for magic unlocking; returns true if targetted monster (which might
   be hero) gets hit by a trap (target might avoid its effect) */
boolean
openfallingtrap(mon, trapdoor_only, noticed)
struct monst *mon;
boolean trapdoor_only;
boolean *noticed; /* set to true iff hero notices the effect; */
{                 /* otherwise left with its previous value intact */
    struct trap *t;
    boolean ishero = (mon == &youmonst), result;

    if (mon == u.usteed)
        ishero = TRUE;
    t = t_at(ishero ? u.ux : mon->mx, ishero ? u.uy : mon->my);
    /* if no trap here or it's not a falling trap, we're done
       (note: falling rock traps have a trapdoor in the ceiling) */
    if (!t || ((t->ttyp != TRAPDOOR && t->ttyp != ROCKTRAP)
               && (trapdoor_only || (t->ttyp != HOLE && t->ttyp != PIT
                                     && t->ttyp != SPIKED_PIT))))
        return FALSE;

    if (ishero) {
        if (u.utrap)
            return FALSE; /* already trapped */
        *noticed = TRUE;
        dotrap(t, FORCETRAP);
        result = (u.utrap != 0);
    } else {
        if (mon->mtrapped)
            return FALSE; /* already trapped */
        /* you notice it if you see the trap close/tremble/whatever
           or if you sense the monster who becomes trapped */
        *noticed = cansee(t->tx, t->ty) || canspotmon(mon);
        /* monster will be angered; mintrap doesn't handle that */
        wakeup(mon);
        ++force_mintrap;
        result = (mintrap(mon) != 0);
        --force_mintrap;
        /* mon might now be on the migrating monsters list */
    }
    return TRUE;
}

/* only called when the player is doing something to the chest directly */
boolean
chest_trap(obj, bodypart, disarm)
register struct obj *obj;
register int bodypart;
boolean disarm;
{
    register struct obj *otmp = obj, *otmp2;
    char buf[80];
    const char *msg;
    coord cc;

    if (get_obj_location(obj, &cc.x, &cc.y, 0)) /* might be carried */
        obj->ox = cc.x, obj->oy = cc.y;

    otmp->otrapped = 0; /* trap is one-shot; clear flag first in case
                           chest kills you and ends up in bones file */
    You(disarm ? "触发了它!" : "触发了一个陷阱!");
    display_nhwindow(WIN_MESSAGE, FALSE);
    if (Luck > -13 && rn2(13 + Luck) > 7) { /* saved by luck */
        /* trap went off, but good luck prevents damage */
        switch (rn2(13)) {
        case 12:
        case 11:
            msg = "炸药是哑弹";
            break;
        case 10:
        case 9:
            msg = "电荷接地了";
            break;
        case 8:
        case 7:
            msg = "火焰消失了";
            break;
        case 6:
        case 5:
        case 4:
            msg = "有毒的针没打中";
            break;
        case 3:
        case 2:
        case 1:
        case 0:
            msg = "气体云吹走了";
            break;
        default:
            impossible("chest disarm bug");
            msg = (char *) 0;
            break;
        }
        if (msg)
            pline("但幸运的是%s!", msg);
    } else {
        switch (rn2(20) ? ((Luck >= 13) ? 0 : rn2(13 - Luck)) : rn2(26)) {
        case 25:
        case 24:
        case 23:
        case 22:
        case 21: {
            struct monst *shkp = 0;
            long loss = 0L;
            boolean costly, insider;
            register xchar ox = obj->ox, oy = obj->oy;

            /* the obj location need not be that of player */
            costly = (costly_spot(ox, oy)
                      && (shkp = shop_keeper(*in_rooms(ox, oy, SHOPBASE)))
                             != (struct monst *) 0);
            insider = (*u.ushops && inside_shop(u.ux, u.uy)
                       && *in_rooms(ox, oy, SHOPBASE) == *u.ushops);

            pline("%s了!", Tobjnam(obj, "爆炸"));
            Sprintf(buf, "爆炸的%s", xname(obj));

            if (costly)
                loss += stolen_value(obj, ox, oy, (boolean) shkp->mpeaceful,
                                     TRUE);
            delete_contents(obj);
            /* we're about to delete all things at this location,
             * which could include the ball & chain.
             * If we attempt to call unpunish() in the
             * for-loop below we can end up with otmp2
             * being invalid once the chain is gone.
             * Deal with ball & chain right now instead.
             */
            if (Punished && !carried(uball)
                && ((uchain->ox == u.ux && uchain->oy == u.uy)
                    || (uball->ox == u.ux && uball->oy == u.uy)))
                unpunish();

            for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp2) {
                otmp2 = otmp->nexthere;
                if (costly)
                    loss += stolen_value(otmp, otmp->ox, otmp->oy,
                                         (boolean) shkp->mpeaceful, TRUE);
                delobj(otmp);
            }
            wake_nearby();
            losehp(Maybe_Half_Phys(d(6, 6)), buf, KILLED_BY_AN);
            exercise(A_STR, FALSE);
            if (costly && loss) {
                if (insider)
                    You("破坏了价值%ld %s 的东西.", loss,
                        currency(loss));
                else {
                    You("造成了%ld %s 价值的损害!", loss,
                        currency(loss));
                    make_angry_shk(shkp, ox, oy);
                }
            }
            return TRUE;
        } /* case 21 */
        case 20:
        case 19:
        case 18:
        case 17:
            pline("一团有毒气体从%s翻腾.", the(xname(obj)));
            poisoned("气体云", A_STR, "毒气云", 15, FALSE);
            exercise(A_CON, FALSE);
            break;
        case 16:
        case 15:
        case 14:
        case 13:
            You_feel("一个针刺到了你的%s.", body_part(bodypart));
            poisoned("针", A_CON, "有毒的针", 10, FALSE);
            exercise(A_CON, FALSE);
            break;
        case 12:
        case 11:
        case 10:
        case 9:
            dofiretrap(obj);
            break;
        case 8:
        case 7:
        case 6: {
            int dmg;

            You("被电脉冲猛击!");
            if (Shock_resistance) {
                shieldeff(u.ux, u.uy);
                You("似乎没有受影响.");
                dmg = 0;
            } else
                dmg = d(4, 4);
            destroy_item(RING_CLASS, AD_ELEC);
            destroy_item(WAND_CLASS, AD_ELEC);
            if (dmg)
                losehp(dmg, "电冲击", KILLED_BY_AN);
            break;
        } /* case 6 */
        case 5:
        case 4:
        case 3:
            if (!Free_action) {
                pline("突然你就地被冰冻了!");
                nomul(-d(5, 6));
                multi_reason = "被一个陷阱冰冻";
                exercise(A_DEX, FALSE);
                nomovemsg = You_can_move_again;
            } else
                You("立即僵硬.");
            break;
        case 2:
        case 1:
        case 0:
            pline("一团%s气体从%s翻腾.",
                  Blind ? blindgas[rn2(SIZE(blindgas))] : rndcolor(),
                  the(xname(obj)));
            if (!Stunned) {
                if (Hallucination)
                    pline("好一个绝妙的感觉!");
                else
                    You("%s%s...", stagger(youmonst.data, "蹒跚"),
                        Halluc_resistance ? ""
                                          : Blind ? "并变得眩晕"
                                                  : "且你的视力模糊了");
            }
            make_stunned((HStun & TIMEOUT) + (long) rn1(7, 16), FALSE);
            (void) make_hallucinated(
                (HHallucination & TIMEOUT) + (long) rn1(5, 16), FALSE, 0L);
            break;
        default:
            impossible("bad chest trap");
            break;
        }
        bot(); /* to get immediate botl re-display */
    }

    return FALSE;
}

struct trap *
t_at(x, y)
register int x, y;
{
    register struct trap *trap = ftrap;

    while (trap) {
        if (trap->tx == x && trap->ty == y)
            return trap;
        trap = trap->ntrap;
    }
    return (struct trap *) 0;
}

void
deltrap(trap)
register struct trap *trap;
{
    register struct trap *ttmp;

    clear_conjoined_pits(trap);
    if (trap == ftrap) {
        ftrap = ftrap->ntrap;
    } else {
        for (ttmp = ftrap; ttmp; ttmp = ttmp->ntrap)
            if (ttmp->ntrap == trap)
                break;
        if (!ttmp)
            panic("deltrap: no preceding trap!");
        ttmp->ntrap = trap->ntrap;
    }
    if (Sokoban && (trap->ttyp == PIT || trap->ttyp == HOLE))
        maybe_finish_sokoban();
    dealloc_trap(trap);
}

boolean
conjoined_pits(trap2, trap1, u_entering_trap2)
struct trap *trap2, *trap1;
boolean u_entering_trap2;
{
    int dx, dy, diridx, adjidx;

    if (!trap1 || !trap2)
        return FALSE;
    if (!isok(trap2->tx, trap2->ty) || !isok(trap1->tx, trap1->ty)
        || !(trap2->ttyp == PIT || trap2->ttyp == SPIKED_PIT)
        || !(trap1->ttyp == PIT || trap1->ttyp == SPIKED_PIT)
        || (u_entering_trap2 && !(u.utrap && u.utraptype == TT_PIT)))
        return FALSE;
    dx = sgn(trap2->tx - trap1->tx);
    dy = sgn(trap2->ty - trap1->ty);
    for (diridx = 0; diridx < 8; diridx++)
        if (xdir[diridx] == dx && ydir[diridx] == dy)
            break;
    /* diridx is valid if < 8 */
    if (diridx < 8) {
        adjidx = (diridx + 4) % 8;
        if ((trap1->conjoined & (1 << diridx))
            && (trap2->conjoined & (1 << adjidx)))
            return TRUE;
    }
    return FALSE;
}

void
clear_conjoined_pits(trap)
struct trap *trap;
{
    int diridx, adjidx, x, y;
    struct trap *t;

    if (trap && (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT)) {
        for (diridx = 0; diridx < 8; ++diridx) {
            if (trap->conjoined & (1 << diridx)) {
                x = trap->tx + xdir[diridx];
                y = trap->ty + ydir[diridx];
                if (isok(x, y)
                    && (t = t_at(x, y)) != 0
                    && (t->ttyp == PIT || t->ttyp == SPIKED_PIT)) {
                    adjidx = (diridx + 4) % 8;
                    t->conjoined &= ~(1 << adjidx);
                }
                trap->conjoined &= ~(1 << diridx);
            }
        }
    }
}

#if 0
/*
 * Mark all neighboring pits as conjoined pits.
 * (currently not called from anywhere)
 */
STATIC_OVL void
join_adjacent_pits(trap)
struct trap *trap;
{
    struct trap *t;
    int diridx, x, y;

    if (!trap)
        return;
    for (diridx = 0; diridx < 8; ++diridx) {
        x = trap->tx + xdir[diridx];
        y = trap->ty + ydir[diridx];
        if (isok(x, y)) {
            if ((t = t_at(x, y)) != 0
                && (t->ttyp == PIT || t->ttyp == SPIKED_PIT)) {
                trap->conjoined |= (1 << diridx);
                join_adjacent_pits(t);
            } else
                trap->conjoined &= ~(1 << diridx);
        }
    }
}
#endif /*0*/

/*
 * Returns TRUE if you escaped a pit and are standing on the precipice.
 */
boolean
uteetering_at_seen_pit(trap)
struct trap *trap;
{
    if (trap && trap->tseen && (!u.utrap || u.utraptype != TT_PIT)
        && (trap->ttyp == PIT || trap->ttyp == SPIKED_PIT))
        return TRUE;
    else
        return FALSE;
}

/* Destroy a trap that emanates from the floor. */
boolean
delfloortrap(ttmp)
register struct trap *ttmp;
{
    /* some of these are arbitrary -dlc */
    if (ttmp && ((ttmp->ttyp == SQKY_BOARD) || (ttmp->ttyp == BEAR_TRAP)
                 || (ttmp->ttyp == LANDMINE) || (ttmp->ttyp == FIRE_TRAP)
                 || (ttmp->ttyp == PIT) || (ttmp->ttyp == SPIKED_PIT)
                 || (ttmp->ttyp == HOLE) || (ttmp->ttyp == TRAPDOOR)
                 || (ttmp->ttyp == TELEP_TRAP) || (ttmp->ttyp == LEVEL_TELEP)
                 || (ttmp->ttyp == WEB) || (ttmp->ttyp == MAGIC_TRAP)
                 || (ttmp->ttyp == ANTI_MAGIC))) {
        register struct monst *mtmp;

        if (ttmp->tx == u.ux && ttmp->ty == u.uy) {
            u.utrap = 0;
            u.utraptype = 0;
        } else if ((mtmp = m_at(ttmp->tx, ttmp->ty)) != 0) {
            mtmp->mtrapped = 0;
        }
        deltrap(ttmp);
        return TRUE;
    }
    return FALSE;
}

/* used for doors (also tins).  can be used for anything else that opens. */
void
b_trapped(item, bodypart)
const char *item;
int bodypart;
{
    int lvl = level_difficulty(),
        dmg = rnd(5 + (lvl < 5 ? lvl : 2 + lvl / 2));

    pline("嘣!!  %s是诱杀装置!", The(item));
    wake_nearby();
    losehp(Maybe_Half_Phys(dmg), "爆炸", KILLED_BY_AN);
    exercise(A_STR, FALSE);
    if (bodypart)
        exercise(A_CON, FALSE);
    make_stunned((HStun & TIMEOUT) + (long) dmg, TRUE);
}

/* Monster is hit by trap. */
/* Note: doesn't work if both obj and d_override are null */
STATIC_OVL boolean
thitm(tlev, mon, obj, d_override, nocorpse)
int tlev;
struct monst *mon;
struct obj *obj;
int d_override;
boolean nocorpse;
{
    int strike;
    boolean trapkilled = FALSE;

    if (d_override)
        strike = 1;
    else if (obj)
        strike = (find_mac(mon) + tlev + obj->spe <= rnd(20));
    else
        strike = (find_mac(mon) + tlev <= rnd(20));

    /* Actually more accurate than thitu, which doesn't take
     * obj->spe into account.
     */
    if (!strike) {
        if (obj && cansee(mon->mx, mon->my))
            pline("%s 几乎被%s打中!", Monnam(mon), doname(obj));
    } else {
        int dam = 1;

        if (obj && cansee(mon->mx, mon->my))
            pline("%s 被%s打中!", Monnam(mon), doname(obj));
        if (d_override)
            dam = d_override;
        else if (obj) {
            dam = dmgval(obj, mon);
            if (dam < 1)
                dam = 1;
        }
        if ((mon->mhp -= dam) <= 0) {
            int xx = mon->mx;
            int yy = mon->my;

            monkilled(mon, "", nocorpse ? -AD_RBRE : AD_PHYS);
            if (mon->mhp <= 0) {
                newsym(xx, yy);
                trapkilled = TRUE;
            }
        }
    }
    if (obj && (!strike || d_override)) {
        place_object(obj, mon->mx, mon->my);
        stackobj(obj);
    } else if (obj)
        dealloc_obj(obj);

    return trapkilled;
}

boolean
unconscious()
{
    if (multi >= 0)
        return FALSE;

    return (boolean) (u.usleep
                      || (nomovemsg
                          && (!strncmp(nomovemsg, "你醒", 6)
                              || !strncmp(nomovemsg, "你重获", 9)
                              || !strncmp(nomovemsg, "你再次清", 12))));
}

static const char lava_killer[] = "熔岩";

boolean
lava_effects()
{
    register struct obj *obj, *obj2;
    int dmg = d(6, 6); /* only applicable for water walking */
    boolean usurvive, boil_away;

    burn_away_slime();
    if (likes_lava(youmonst.data))
        return FALSE;

    usurvive = Fire_resistance || (Wwalking && dmg < u.uhp);
    /*
     * A timely interrupt might manage to salvage your life
     * but not your gear.  For scrolls and potions this
     * will destroy whole stacks, where fire resistant hero
     * survivor only loses partial stacks via destroy_item().
     *
     * Flag items to be destroyed before any messages so
     * that player causing hangup at --More-- won't get an
     * emergency save file created before item destruction.
     */
    if (!usurvive)
        for (obj = invent; obj; obj = obj->nobj)
            if ((is_organic(obj) || obj->oclass == POTION_CLASS)
                && !obj->oerodeproof
                && objects[obj->otyp].oc_oprop != FIRE_RES
                && obj->otyp != SCR_FIRE && obj->otyp != SPE_FIREBALL
                && !obj_resists(obj, 0, 0)) /* for invocation items */
                obj->in_use = TRUE;

    /* Check whether we should burn away boots *first* so we know whether to
     * make the player sink into the lava. Assumption: water walking only
     * comes from boots.
     */
    if (Wwalking && uarmf && is_organic(uarmf) && !uarmf->oerodeproof) {
        obj = uarmf;
        pline("%s 出火焰!", Yobjnam2(obj, "爆发"));
        iflags.in_lava_effects++; /* (see above) */
        (void) Boots_off();
        useup(obj);
        iflags.in_lava_effects--;
    }

    if (!Fire_resistance) {
        if (Wwalking) {
            pline_The("这里的熔岩烧伤了你!");
            if (usurvive) {
                losehp(dmg, lava_killer, KILLED_BY); /* lava damage */
                goto burn_stuff;
            }
        } else
            You("掉进了熔岩!");

        usurvive = Lifesaved || discover;
        if (wizard)
            usurvive = TRUE;

        /* prevent remove_worn_item() -> Boots_off(WATER_WALKING_BOOTS) ->
           spoteffects() -> lava_effects() recursion which would
           successfully delete (via useupall) the no-longer-worn boots;
           once recursive call returned, we would try to delete them again
           here in the outer call (and access stale memory, probably panic) */
        iflags.in_lava_effects++;

        for (obj = invent; obj; obj = obj2) {
            obj2 = obj->nobj;
            /* above, we set in_use for objects which are to be destroyed */
            if (obj->otyp == SPE_BOOK_OF_THE_DEAD && !Blind) {
                if (usurvive)
                    pline("%s 发出奇怪的%s光芒, 但是保持完整.",
                          The(xname(obj)), hcolor("深红色"));
            } else if (obj->in_use) {
                if (obj->owornmask) {
                    if (usurvive)
                        pline("%s 出火焰!", Yobjnam2(obj, "爆发"));
                    remove_worn_item(obj, TRUE);
                }
                useupall(obj);
            }
        }

        iflags.in_lava_effects--;

        /* s/he died... */
        boil_away = (u.umonnum == PM_WATER_ELEMENTAL
                     || u.umonnum == PM_STEAM_VORTEX
                     || u.umonnum == PM_FOG_CLOUD);
        for (;;) {
            u.uhp = -1;
            /* killer format and name are reconstructed every iteration
               because lifesaving resets them */
            killer.format = KILLED_BY;
            Strcpy(killer.name, lava_killer);
            You("%s...", boil_away ? "不断沸腾" : "烧焦了");
            done(BURNING);
            if (safe_teleds(TRUE))
                break; /* successful life-save */
            /* nowhere safe to land; repeat burning loop */
            pline("你仍然被烧着.");
        }
        You("发现你自己退回固体的%s.", surface(u.ux, u.uy));
        return TRUE;
    } else if (!Wwalking && (!u.utrap || u.utraptype != TT_LAVA)) {
        boil_away = !Fire_resistance;
        /* if not fire resistant, sink_into_lava() will quickly be fatal;
           hero needs to escape immediately */
        u.utrap = rn1(4, 4) + ((boil_away ? 2 : rn1(4, 12)) << 8);
        u.utraptype = TT_LAVA;
        You("沉入熔岩%s!", !boil_away
                                         ? ", 但它只是轻微地燃烧"
                                         : "并准备牺牲了");
        if (u.uhp > 1)
            losehp(!boil_away ? 1 : (u.uhp / 2), lava_killer,
                   KILLED_BY); /* lava damage */
    }

burn_stuff:
    destroy_item(SCROLL_CLASS, AD_FIRE);
    destroy_item(SPBOOK_CLASS, AD_FIRE);
    destroy_item(POTION_CLASS, AD_FIRE);
    return FALSE;
}

/* called each turn when trapped in lava */
void
sink_into_lava()
{
    static const char sink_deeper[] = "你在熔岩中陷得更深.";

    if (!u.utrap || u.utraptype != TT_LAVA) {
        ; /* do nothing; this shouldn't happen */
    } else if (!is_lava(u.ux, u.uy)) {
        u.utrap = 0; /* this shouldn't happen either */
    } else if (!u.uinvulnerable) {
        /* ordinarily we'd have to be fire resistant to survive long
           enough to become stuck in lava, but it can happen without
           resistance if water walking boots allow survival and then
           get burned up; u.utrap time will be quite short in that case */
        if (!Fire_resistance)
            u.uhp = (u.uhp + 2) / 3;

        u.utrap -= (1 << 8);
        if (u.utrap < (1 << 8)) {
            killer.format = KILLED_BY;
            Strcpy(killer.name, "熔岩");
            You("沉下表面然后死了.");
            burn_away_slime(); /* add insult to injury? */
            done(DISSOLVED);
        } else if (!u.umoved) {
            /* can't fully turn into slime while in lava, but might not
               have it be burned away until you've come awfully close */
            if (Slimed && rnd(10 - 1) >= (int) (Slimed & TIMEOUT)) {
                pline(sink_deeper);
                burn_away_slime();
            } else {
                Norep(sink_deeper);
            }
            u.utrap += rnd(4);
        }
    }
}

/* called when something has been done (breaking a boulder, for instance)
   which entails a luck penalty if performed on a sokoban level */
void
sokoban_guilt()
{
    if (Sokoban) {
        change_luck(-1);
        /* TODO: issue some feedback so that player can learn that whatever
           he/she just did is a naughty thing to do in sokoban and should
           probably be avoided in future....
           Caveat: doing this might introduce message sequencing issues,
           depending upon feedback during the various actions which trigger
           Sokoban luck penalties. */
    }
}

/* called when a trap has been deleted or had its ttyp replaced */
STATIC_OVL void
maybe_finish_sokoban()
{
    struct trap *t;

    if (Sokoban && !in_mklev) {
        /* scan all remaining traps, ignoring any created by the hero;
           if this level has no more pits or holes, the current sokoban
           puzzle has been solved */
        for (t = ftrap; t; t = t->ntrap) {
            if (t->madeby_u)
                continue;
            if (t->ttyp == PIT || t->ttyp == HOLE)
                break;
        }
        if (!t) {
            /* we've passed the last trap without finding a pit or hole;
               clear the sokoban_rules flag so that luck penalties for
               things like breaking boulders or jumping will no longer
               be given, and restrictions on diagonal moves are lifted */
            Sokoban = 0; /* clear level.flags.sokoban_rules */
            /* TODO: give some feedback about solving the sokoban puzzle
               (perhaps say "congratulations" in Japanese?) */
        }
    }
}

/*trap.c*/
