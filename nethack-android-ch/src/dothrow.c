/* NetHack 3.6	dothrow.c	$NHDT-Date: 1446975465 2015/11/08 09:37:45 $  $NHDT-Branch: master $:$NHDT-Revision: 1.113 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* Contains code for 't' (throw) */

#include "hack.h"

STATIC_DCL int FDECL(throw_obj, (struct obj *, int));
STATIC_DCL boolean FDECL(ok_to_throw, (int *));
STATIC_DCL void NDECL(autoquiver);
STATIC_DCL int FDECL(gem_accept, (struct monst *, struct obj *));
STATIC_DCL void FDECL(tmiss, (struct obj *, struct monst *, BOOLEAN_P));
STATIC_DCL int FDECL(throw_gold, (struct obj *));
STATIC_DCL void FDECL(check_shop_obj, (struct obj *, XCHAR_P, XCHAR_P,
                                       BOOLEAN_P));
STATIC_DCL void FDECL(breakmsg, (struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(toss_up, (struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(throwing_weapon, (struct obj *));
STATIC_DCL void FDECL(sho_obj_return_to_u, (struct obj * obj));
STATIC_DCL boolean FDECL(mhurtle_step, (genericptr_t, int, int));

static NEARDATA const char toss_objs[] = { ALLOW_COUNT, COIN_CLASS,
                                           ALL_CLASSES, WEAPON_CLASS, 0 };
/* different default choices when wielding a sling (gold must be included) */
static NEARDATA const char bullets[] = { ALLOW_COUNT, COIN_CLASS, ALL_CLASSES,
                                         GEM_CLASS, 0 };

/* thrownobj (decl.c) tracks an object until it lands */

extern boolean notonhead; /* for long worms */

/* Throw the selected object, asking for direction */
STATIC_OVL int
throw_obj(obj, shotlimit)
struct obj *obj;
int shotlimit;
{
    struct obj *otmp;
    int multishot;
    schar skill;
    long wep_mask;
    boolean twoweap, weakmultishot;

    /* ask "in what direction?" */
    if (!getdir((char *) 0)) {
        /* No direction specified, so cancel the throw;
         * might need to undo an object split.
         * We used to use freeinv(obj),addinv(obj) here, but that can
         * merge obj into another stack--usually quiver--even if it hadn't
         * been split from there (possibly triggering a panic in addinv),
         * and freeinv+addinv potentially has other side-effects.
         */
        if (obj->o_id == context.objsplit.parent_oid
            || obj->o_id == context.objsplit.child_oid)
            (void) unsplitobj(obj);
        return 0; /* no time passes */
    }

    /*
     * Throwing money is usually for getting rid of it when
     * a leprechaun approaches, or for bribing an oncoming
     * angry monster.  So throw the whole object.
     *
     * If the money is in quiver, throw one coin at a time,
     * possibly using a sling.
     */
    if (obj->oclass == COIN_CLASS && obj != uquiver)
        return throw_gold(obj);

    if (!canletgo(obj, "投掷"))  //throw
        return 0;
    if (obj->oartifact == ART_MJOLLNIR && obj != uwep) {
        pline("在%s能被投掷之前要必须拿着它.", The(xname(obj)));
        return 0;
    }
    if ((obj->oartifact == ART_MJOLLNIR && ACURR(A_STR) < STR19(25))
        || (obj->otyp == BOULDER && !throws_rocks(youmonst.data))) {
        pline("它太重了.");
        return 1;
    }
    if (!u.dx && !u.dy && !u.dz) {
        You("不能向你自己投掷东西.");
        return 0;
    }
    u_wipe_engr(2);
    if (!uarmg && obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])
        && !Stone_resistance) {
        You("用你的赤%s投掷%s.",
            makeplural(body_part(HAND)),
            corpse_xname(obj, (const char *) 0, CXN_PFX_THE)
            /* throwing with one hand, but pluralize since the
               expression "with your bare hands" sounds better */
            );
        Sprintf(killer.name, "赤手投掷%s", killer_xname(obj));
        instapetrify(killer.name);
    }
    if (welded(obj)) {
        weldmsg(obj);
        return 1;
    }
    if (is_wet_towel(obj))
        dry_a_towel(obj, -1, FALSE);

    /* Multishot calculations
     * (potential volley of up to N missiles; default for N is 1)
     */
    multishot = 1;
    skill = objects[obj->otyp].oc_skill;
    if (obj->quan > 1L /* no point checking if there's only 1 */
        /* ammo requires corresponding launcher be wielded */
        && (is_ammo(obj) ? matching_launcher(obj, uwep)
                         /* otherwise any stackable (non-ammo) weapon */
                         : obj->oclass == WEAPON_CLASS)
        && !(Confusion || Stunned)) {
        /* some roles don't get a volley bonus until becoming expert */
        weakmultishot = (Role_if(PM_WIZARD) || Role_if(PM_PRIEST)
                         || (Role_if(PM_HEALER) && skill != P_KNIFE)
                         || (Role_if(PM_TOURIST) && skill != -P_DART)
                         /* poor dexterity also inhibits multishot */
                         || Fumbling || ACURR(A_DEX) <= 6);

        /* Bonus if the player is proficient in this weapon... */
        switch (P_SKILL(weapon_type(obj))) {
        case P_EXPERT:
            multishot++;
        /*FALLTHRU*/
        case P_SKILLED:
            if (!weakmultishot)
                multishot++;
            break;
        default: /* basic or unskilled: no bonus */
            break;
        }
        /* ...or is using a special weapon for their role... */
        switch (Role_switch) {
        case PM_CAVEMAN:
            /* give bonus for low-tech gear */
            if (skill == -P_SLING || skill == P_SPEAR)
                multishot++;
            break;
        case PM_MONK:
            /* allow higher volley count despite skill limitation */
            if (skill == -P_SHURIKEN)
                multishot++;
            break;
        case PM_RANGER:
            /* arbitrary; encourage use of other missiles beside daggers */
            if (skill != P_DAGGER)
                multishot++;
            break;
        case PM_ROGUE:
            /* possibly should add knives... */
            if (skill == P_DAGGER)
                multishot++;
            break;
        case PM_SAMURAI:
            /* role-specific launcher and its ammo */
            if (obj->otyp == YA && uwep && uwep->otyp == YUMI)
                multishot++;
            break;
        default:
            break; /* No bonus */
        }
        /* ...or using their race's special bow; no bonus for spears */
        if (!weakmultishot)
            switch (Race_switch) {
            case PM_ELF:
                if (obj->otyp == ELVEN_ARROW && uwep
                    && uwep->otyp == ELVEN_BOW)
                    multishot++;
                break;
            case PM_ORC:
                if (obj->otyp == ORCISH_ARROW && uwep
                    && uwep->otyp == ORCISH_BOW)
                    multishot++;
                break;
            case PM_GNOME:
                /* arbitrary; there isn't any gnome-specific gear */
                if (skill == -P_CROSSBOW)
                    multishot++;
                break;
            case PM_HUMAN:
            case PM_DWARF:
            default:
                break; /* No bonus */
            }

        /* crossbows are slow to load and probably shouldn't allow multiple
           shots at all, but that would result in players never using them;
           instead, high strength is necessary to load and shoot quickly */
        if (multishot > 1 && skill == -P_CROSSBOW
            && ammo_and_launcher(obj, uwep)
            && (int) ACURRSTR < (Race_if(PM_GNOME) ? 16 : 18))
            multishot = rnd(multishot);

        multishot = rnd(multishot);
        if ((long) multishot > obj->quan)
            multishot = (int) obj->quan;
        if (shotlimit > 0 && multishot > shotlimit)
            multishot = shotlimit;
    }

    m_shot.s = ammo_and_launcher(obj, uwep) ? TRUE : FALSE;
    /* give a message if shooting more than one, or if player
       attempted to specify a count */
    if (multishot > 1 || shotlimit > 0) {
        /* "You shoot N arrows." or "You throw N daggers." */
        You("%s %d %s.", m_shot.s ? "发射" : "投掷",
            multishot, /* (might be 1 if player gave shotlimit) */
            (multishot == 1) ? singular(obj, xname) : xname(obj));
    }

    wep_mask = obj->owornmask;
    m_shot.o = obj->otyp;
    m_shot.n = multishot;
    for (m_shot.i = 1; m_shot.i <= m_shot.n; m_shot.i++) {
        twoweap = u.twoweap;
        /* split this object off from its slot if necessary */
        if (obj->quan > 1L) {
            otmp = splitobj(obj, 1L);
        } else {
            otmp = obj;
            if (otmp->owornmask)
                remove_worn_item(otmp, FALSE);
        }
        freeinv(otmp);
        throwit(otmp, wep_mask, twoweap);
    }
    m_shot.n = m_shot.i = 0;
    m_shot.o = STRANGE_OBJECT;
    m_shot.s = FALSE;

    return 1;
}

/* common to dothrow() and dofire() */
STATIC_OVL boolean
ok_to_throw(shotlimit_p)
int *shotlimit_p; /* (see dothrow()) */
{
    /* kludge to work around parse()'s pre-decrement of `multi' */
    *shotlimit_p = (multi || save_cm) ? multi + 1 : 0;
    multi = 0; /* reset; it's been used up */

    if (notake(youmonst.data)) {
        You("的身体不能投掷或发射任何东西.");
        return FALSE;
    } else if (nohands(youmonst.data)) {
        You_cant("没有手就投掷或发射."); /* not body_part(HAND) */
        return FALSE;
        /*[what about !freehand(), aside from cursed missile launcher?]*/
    }
    if (check_capacity((char *) 0))
        return FALSE;
    return TRUE;
}

/* t command - throw */
int
dothrow()
{
    register struct obj *obj;
    int shotlimit;

    /*
     * Since some characters shoot multiple missiles at one time,
     * allow user to specify a count prefix for 'f' or 't' to limit
     * number of items thrown (to avoid possibly hitting something
     * behind target after killing it, or perhaps to conserve ammo).
     *
     * Prior to 3.3.0, command ``3t'' meant ``t(shoot) t(shoot) t(shoot)''
     * and took 3 turns.  Now it means ``t(shoot at most 3 missiles)''.
     *
     * [3.6.0:  shot count setup has been moved into ok_to_throw().]
     */
    if (!ok_to_throw(&shotlimit))
        return 0;

    obj = getobj(uslinging() ? bullets : toss_objs, "投掷");  //throw
    /* it is also possible to throw food */
    /* (or jewels, or iron balls... ) */

    return obj ? throw_obj(obj, shotlimit) : 0;
}

/* KMH -- Automatically fill quiver */
/* Suggested by Jeffrey Bay <jbay@convex.hp.com> */
static void
autoquiver()
{
    struct obj *otmp, *oammo = 0, *omissile = 0, *omisc = 0, *altammo = 0;

    if (uquiver)
        return;

    /* Scan through the inventory */
    for (otmp = invent; otmp; otmp = otmp->nobj) {
        if (otmp->owornmask || otmp->oartifact || !otmp->dknown) {
            ; /* Skip it */
        } else if (otmp->otyp == ROCK
                   /* seen rocks or known flint or known glass */
                   || (otmp->otyp == FLINT
                       && objects[otmp->otyp].oc_name_known)
                   || (otmp->oclass == GEM_CLASS
                       && objects[otmp->otyp].oc_material == GLASS
                       && objects[otmp->otyp].oc_name_known)) {
            if (uslinging())
                oammo = otmp;
            else if (ammo_and_launcher(otmp, uswapwep))
                altammo = otmp;
            else if (!omisc)
                omisc = otmp;
        } else if (otmp->oclass == GEM_CLASS) {
            ; /* skip non-rock gems--they're ammo but
                 player has to select them explicitly */
        } else if (is_ammo(otmp)) {
            if (ammo_and_launcher(otmp, uwep))
                /* Ammo matched with launcher (bow+arrow, crossbow+bolt) */
                oammo = otmp;
            else if (ammo_and_launcher(otmp, uswapwep))
                altammo = otmp;
            else
                /* Mismatched ammo (no better than an ordinary weapon) */
                omisc = otmp;
        } else if (is_missile(otmp)) {
            /* Missile (dart, shuriken, etc.) */
            omissile = otmp;
        } else if (otmp->oclass == WEAPON_CLASS && throwing_weapon(otmp)) {
            /* Ordinary weapon */
            if (objects[otmp->otyp].oc_skill == P_DAGGER && !omissile)
                omissile = otmp;
            else
                omisc = otmp;
        }
    }

    /* Pick the best choice */
    if (oammo)
        setuqwep(oammo);
    else if (omissile)
        setuqwep(omissile);
    else if (altammo)
        setuqwep(altammo);
    else if (omisc)
        setuqwep(omisc);

    return;
}

/* f command -- fire: throw from the quiver */
int
dofire()
{
    int shotlimit;
    struct obj *obj;

    /*
     * Same as dothrow(), except we use quivered missile instead
     * of asking what to throw/shoot.
     *
     * If quiver is empty, we use autoquiver to fill it when the
     * corresponding option is on.  If the option is off or if
     * autoquiver doesn't select anything, we ask what to throw.
     * For the last, if player's response is a stack, we put
     * that stack into quiver slot provided it's not wielded.
     */
    if (!ok_to_throw(&shotlimit))
        return 0;

    if ((obj = uquiver) == 0) {
        if (!flags.autoquiver) {
            You("没有准备好的发射物.");
        } else {
            autoquiver();
            if ((obj = uquiver) == 0)
                You("没有适合的东西来准备.");
        }
        /* if autoquiver is disabled or has failed, prompt for missile;
           fill quiver with it if it's not wielded */
        if (!obj) {
            obj = getobj(uslinging() ? bullets : toss_objs, "投掷");  //throw
            /* Q command doesn't allow gold in quiver */
            if (obj && !obj->owornmask && obj->oclass != COIN_CLASS)
                setuqwep(obj); /* demi-autoquiver */
        }
        /* give feedback if quiver has now been filled */
        if (uquiver) {
            uquiver->owornmask &= ~W_QUIVER; /* less verbose */
            prinv("你准备:", uquiver, 0L);
            uquiver->owornmask |= W_QUIVER;
        }
    }

    return obj ? throw_obj(obj, shotlimit) : 0;
}

/* if in midst of multishot shooting/throwing, stop early */
void
endmultishot(verbose)
boolean verbose;
{
    if (m_shot.i < m_shot.n) {
        if (verbose && !context.mon_moving) {
            You("在第%d 次%s之后停止了%s.",
                m_shot.i,
                m_shot.s ? "射击" : "投掷",
                m_shot.s ? "射击" : "投掷");
        }
        m_shot.n = m_shot.i; /* make current shot be the last */
    }
}

/*
 * Object hits floor at hero's feet.  Called from drop() and throwit().
 */
void
hitfloor(obj)
register struct obj *obj;
{
    if (IS_SOFT(levl[u.ux][u.uy].typ) || u.uinwater) {
        dropy(obj);
        return;
    }
    if (IS_ALTAR(levl[u.ux][u.uy].typ))
        doaltarobj(obj);
    else
        pline("%s 打中了%s.", Doname2(obj),
              surface(u.ux, u.uy));

    if (hero_breaks(obj, u.ux, u.uy, TRUE))
        return;
    if (ship_object(obj, u.ux, u.uy, FALSE))
        return;
    dropz(obj, TRUE);
}

/*
 * Walk a path from src_cc to dest_cc, calling a proc for each location
 * except the starting one.  If the proc returns FALSE, stop walking
 * and return FALSE.  If stopped early, dest_cc will be the location
 * before the failed callback.
 */
boolean
walk_path(src_cc, dest_cc, check_proc, arg)
coord *src_cc;
coord *dest_cc;
boolean FDECL((*check_proc), (genericptr_t, int, int));
genericptr_t arg;
{
    int x, y, dx, dy, x_change, y_change, err, i, prev_x, prev_y;
    boolean keep_going = TRUE;

    /* Use Bresenham's Line Algorithm to walk from src to dest */
    dx = dest_cc->x - src_cc->x;
    dy = dest_cc->y - src_cc->y;
    prev_x = x = src_cc->x;
    prev_y = y = src_cc->y;

    if (dx < 0) {
        x_change = -1;
        dx = -dx;
    } else
        x_change = 1;
    if (dy < 0) {
        y_change = -1;
        dy = -dy;
    } else
        y_change = 1;

    i = err = 0;
    if (dx < dy) {
        while (i++ < dy) {
            prev_x = x;
            prev_y = y;
            y += y_change;
            err += dx << 1;
            if (err > dy) {
                x += x_change;
                err -= dy << 1;
            }
            /* check for early exit condition */
            if (!(keep_going = (*check_proc)(arg, x, y)))
                break;
        }
    } else {
        while (i++ < dx) {
            prev_x = x;
            prev_y = y;
            x += x_change;
            err += dy << 1;
            if (err > dx) {
                y += y_change;
                err -= dx << 1;
            }
            /* check for early exit condition */
            if (!(keep_going = (*check_proc)(arg, x, y)))
                break;
        }
    }

    if (keep_going)
        return TRUE; /* successful */

    dest_cc->x = prev_x;
    dest_cc->y = prev_y;
    return FALSE;
}

/*
 * Single step for the hero flying through the air from jumping, flying,
 * etc.  Called from hurtle() and jump() via walk_path().  We expect the
 * argument to be a pointer to an integer -- the range -- which is
 * used in the calculation of points off if we hit something.
 *
 * Bumping into monsters won't cause damage but will wake them and make
 * them angry.  Auto-pickup isn't done, since you don't have control over
 * your movements at the time.
 *
 * Possible additions/changes:
 *      o really attack monster if we hit one
 *      o set stunned if we hit a wall or door
 *      o reset nomul when we stop
 *      o creepy feeling if pass through monster (if ever implemented...)
 *      o bounce off walls
 *      o let jumps go over boulders
 */
boolean
hurtle_step(arg, x, y)
genericptr_t arg;
int x, y;
{
    int ox, oy, *range = (int *) arg;
    struct obj *obj;
    struct monst *mon;
    boolean may_pass = TRUE;
    struct trap *ttmp;
    int dmg = 0;

    if (!isok(x, y)) {
        You_feel("精神阻碍了你.");
        return FALSE;
    } else if (!in_out_region(x, y)) {
        return FALSE;
    } else if (*range == 0) {
        return FALSE; /* previous step wants to stop now */
    }

    if (!Passes_walls || !(may_pass = may_passwall(x, y))) {
        if (IS_ROCK(levl[x][y].typ) || closed_door(x, y)) {
            const char *s;

            pline("嗷!");
            if (IS_TREE(levl[x][y].typ))
                s = "碰到树";
            else if (IS_ROCK(levl[x][y].typ))
                s = "碰到墙";
            else
                s = "碰到门";
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), s, KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if (levl[x][y].typ == IRONBARS) {
            You("撞上一些铁球.  嗷!");
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "撞上铁球",
                   KILLED_BY);
            wake_nearto(x,y, 20);
            return FALSE;
        }
        if ((obj = sobj_at(BOULDER, x, y)) != 0) {
            You("撞上一个%s.  嗷!", xname(obj));
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "撞上巨石", KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if (!may_pass) {
            /* did we hit a no-dig non-wall position? */
            You("撞到什么东西!");
            dmg = rnd(2 + *range);
            losehp(Maybe_Half_Phys(dmg), "触碰到宇宙的边缘",
                   KILLED_BY);
            wake_nearto(x,y, 10);
            return FALSE;
        }
        if ((u.ux - x) && (u.uy - y) && bad_rock(youmonst.data, u.ux, y)
            && bad_rock(youmonst.data, x, u.uy)) {
            boolean too_much =
                (invent && (inv_weight() + weight_cap() > 600));
            /* Move at a diagonal. */
            if (bigmonst(youmonst.data) || too_much) {
                You("%s强行挤入一个细缝.",
                    too_much ? "和你所有的物品" : "");
                dmg = rnd(2 + *range);
                losehp(Maybe_Half_Phys(dmg), "挤进一个狭窄的缝隙",
                       KILLED_BY);
                wake_nearto(x,y, 10);
                return FALSE;
            }
        }
    }

    if ((mon = m_at(x, y)) != 0) {
        You("撞到%s.", a_monnam(mon));
        wakeup(mon);
        wake_nearto(x,y, 10);
        return FALSE;
    }
    if ((u.ux - x) && (u.uy - y) && bad_rock(youmonst.data, u.ux, y)
        && bad_rock(youmonst.data, x, u.uy)) {
        /* Move at a diagonal. */
        if (Sokoban) {
            You("突然停下来!");
            return FALSE;
        }
    }

    ox = u.ux;
    oy = u.uy;
    u_on_newpos(x, y); /* set u.<ux,uy>, u.usteed-><mx,my>; cliparound(); */
    newsym(ox, oy);    /* update old position */
    vision_recalc(1);  /* update for new position */
    flush_screen(1);
    /* FIXME:
     * Each trap should really trigger on the recoil if
     * it would trigger during normal movement. However,
     * not all the possible side-effects of this are
     * tested [as of 3.4.0] so we trigger those that
     * we have tested, and offer a message for the
     * ones that we have not yet tested.
     */
    if ((ttmp = t_at(x, y)) != 0) {
        if (ttmp->ttyp == MAGIC_PORTAL) {
            dotrap(ttmp, 0);
            return FALSE;
        } else if (ttmp->ttyp == VIBRATING_SQUARE) {
            pline("你通过它时地面在震动.");
            dotrap(ttmp, 0); /* doesn't print messages */
        } else if (ttmp->ttyp == FIRE_TRAP) {
            dotrap(ttmp, 0);
        } else if ((ttmp->ttyp == PIT || ttmp->ttyp == SPIKED_PIT
                    || ttmp->ttyp == HOLE || ttmp->ttyp == TRAPDOOR)
                   && Sokoban) {
            /* Air currents overcome the recoil */
            dotrap(ttmp, 0);
            *range = 0;
            return TRUE;
        } else {
            if (ttmp->tseen)
                You("越过了%s.",
                    defsyms[trap_to_defsym(ttmp->ttyp)].explanation);
        }
    }
    if (--*range < 0) /* make sure our range never goes negative */
        *range = 0;
    if (*range != 0)
        delay_output();
    return TRUE;
}

STATIC_OVL boolean
mhurtle_step(arg, x, y)
genericptr_t arg;
int x, y;
{
    struct monst *mon = (struct monst *) arg;

    /* TODO: Treat walls, doors, iron bars, pools, lava, etc. specially
     * rather than just stopping before.
     */
    if (goodpos(x, y, mon, 0) && m_in_out_region(mon, x, y)) {
        remove_monster(mon->mx, mon->my);
        newsym(mon->mx, mon->my);
        place_monster(mon, x, y);
        newsym(mon->mx, mon->my);
        set_apparxy(mon);
        (void) mintrap(mon);
        return TRUE;
    }
    return FALSE;
}

/*
 * The player moves through the air for a few squares as a result of
 * throwing or kicking something.
 *
 * dx and dy should be the direction of the hurtle, not of the original
 * kick or throw and be only.
 */
void
hurtle(dx, dy, range, verbose)
int dx, dy, range;
boolean verbose;
{
    coord uc, cc;

    /* The chain is stretched vertically, so you shouldn't be able to move
     * very far diagonally.  The premise that you should be able to move one
     * spot leads to calculations that allow you to only move one spot away
     * from the ball, if you are levitating over the ball, or one spot
     * towards the ball, if you are at the end of the chain.  Rather than
     * bother with all of that, assume that there is no slack in the chain
     * for diagonal movement, give the player a message and return.
     */
    if (Punished && !carried(uball)) {
        You_feel("被铁球拉着.");
        nomul(0);
        return;
    } else if (u.utrap) {
        You("被%s固定.",
            u.utraptype == TT_WEB
                ? "网"
                : u.utraptype == TT_LAVA
                      ? "熔岩"
                      : u.utraptype == TT_INFLOOR
                            ? surface(u.ux, u.uy)
                            : u.utraptype == TT_BURIEDBALL ? "埋葬的球"
                                                           : "陷阱");
        nomul(0);
        return;
    }

    /* make sure dx and dy are [-1,0,1] */
    dx = sgn(dx);
    dy = sgn(dy);

    if (!range || (!dx && !dy) || u.ustuck)
        return; /* paranoia */

    nomul(-range);
    multi_reason = "穿过空气";
    nomovemsg = ""; /* it just happens */
    if (verbose)
        You("向反方向%s.", range > 1 ? "猛冲" : "飘浮");
    /* if we're in the midst of shooting multiple projectiles, stop */
    endmultishot(TRUE);
    sokoban_guilt();
    uc.x = u.ux;
    uc.y = u.uy;
    /* this setting of cc is only correct if dx and dy are [-1,0,1] only */
    cc.x = u.ux + (dx * range);
    cc.y = u.uy + (dy * range);
    (void) walk_path(&uc, &cc, hurtle_step, (genericptr_t) &range);
}

/* Move a monster through the air for a few squares.
 */
void
mhurtle(mon, dx, dy, range)
struct monst *mon;
int dx, dy, range;
{
    coord mc, cc;

    /* At the very least, debilitate the monster */
    mon->movement = 0;
    mon->mstun = 1;

    /* Is the monster stuck or too heavy to push?
     * (very large monsters have too much inertia, even floaters and flyers)
     */
    if (mon->data->msize >= MZ_HUGE || mon == u.ustuck || mon->mtrapped)
        return;

    /* Make sure dx and dy are [-1,0,1] */
    dx = sgn(dx);
    dy = sgn(dy);
    if (!range || (!dx && !dy))
        return; /* paranoia */
    /* don't let grid bugs be hurtled diagonally */
    if (dx && dy && NODIAG(monsndx(mon->data)))
        return;

    /* Send the monster along the path */
    mc.x = mon->mx;
    mc.y = mon->my;
    cc.x = mon->mx + (dx * range);
    cc.y = mon->my + (dy * range);
    (void) walk_path(&mc, &cc, mhurtle_step, (genericptr_t) mon);
    return;
}

STATIC_OVL void
check_shop_obj(obj, x, y, broken)
struct obj *obj;
xchar x, y;
boolean broken;
{
    struct monst *shkp = shop_keeper(*u.ushops);

    if (!shkp)
        return;

    if (broken || !costly_spot(x, y)
        || *in_rooms(x, y, SHOPBASE) != *u.ushops) {
        /* thrown out of a shop or into a different shop */
        if (is_unpaid(obj))
            (void) stolen_value(obj, u.ux, u.uy, (boolean) shkp->mpeaceful,
                                FALSE);
        if (broken)
            obj->no_charge = 1;
    } else {
        if (costly_spot(u.ux, u.uy) && costly_spot(x, y)) {
            if (is_unpaid(obj))
                subfrombill(obj, shkp);
            else if (x != shkp->mx || y != shkp->my)
                sellobj(obj, x, y);
        }
    }
}

/*
 * Hero tosses an object upwards with appropriate consequences.
 *
 * Returns FALSE if the object is gone.
 */
STATIC_OVL boolean
toss_up(obj, hitsroof)
struct obj *obj;
boolean hitsroof;
{
    const char *action;
    boolean petrifier = ((obj->otyp == EGG || obj->otyp == CORPSE)
                         && touch_petrifies(&mons[obj->corpsenm]));
    /* note: obj->quan == 1 */

    if (!has_ceiling(&u.uz)) {
        action = "飞到"; /* into "the sky" or "the water above" */
    } else if (hitsroof) {
        if (breaktest(obj)) {
            pline("%s 打中了%s.", Doname2(obj), ceiling(u.ux, u.uy));
            breakmsg(obj, !Blind);
            breakobj(obj, u.ux, u.uy, TRUE, TRUE);
            return FALSE;
        }
        action = "打中了";
    } else {
        action = "几乎打中了";
    }
    pline("%s %s %s, 然后落到了你的%s上.", Doname2(obj),
          action, ceiling(u.ux, u.uy), body_part(HEAD));

    /* object now hits you */

    if (obj->oclass == POTION_CLASS) {
        potionhit(&youmonst, obj, TRUE);
    } else if (breaktest(obj)) {
        int otyp = obj->otyp;
        int blindinc;

        /* need to check for blindness result prior to destroying obj */
        blindinc = ((otyp == CREAM_PIE || otyp == BLINDING_VENOM)
                    /* AT_WEAP is ok here even if attack type was AT_SPIT */
                    && can_blnd(&youmonst, &youmonst, AT_WEAP, obj))
                       ? rnd(25)
                       : 0;
        breakmsg(obj, !Blind);
        breakobj(obj, u.ux, u.uy, TRUE, TRUE);
        obj = 0; /* it's now gone */
        switch (otyp) {
        case EGG:
            if (petrifier && !Stone_resistance
                && !(poly_when_stoned(youmonst.data)
                     && polymon(PM_STONE_GOLEM))) {
                /* egg ends up "all over your face"; perhaps
                   visored helmet should still save you here */
                if (uarmh)
                    Your("%s 没能保护你.", helm_simple_name(uarmh));
                goto petrify;
            }
        case CREAM_PIE:
        case BLINDING_VENOM:
            pline("你弄得%s上到处都是!", body_part(FACE));
            if (blindinc) {
                if (otyp == BLINDING_VENOM && !Blind)
                    pline("它使你失明!");
                u.ucreamed += blindinc;
                make_blinded(Blinded + (long) blindinc, FALSE);
                if (!Blind)
                    Your1(vision_clears);
            }
            break;
        default:
            break;
        }
        return FALSE;
    } else { /* neither potion nor other breaking object */
        boolean less_damage = uarmh && is_metallic(uarmh), artimsg = FALSE;
        int dmg = dmgval(obj, &youmonst);

        if (obj->oartifact)
            /* need a fake die roll here; rn1(18,2) avoids 1 and 20 */
            artimsg = artifact_hit((struct monst *) 0, &youmonst, obj, &dmg,
                                   rn1(18, 2));

        if (!dmg) { /* probably wasn't a weapon; base damage on weight */
            dmg = (int) obj->owt / 100;
            if (dmg < 1)
                dmg = 1;
            else if (dmg > 6)
                dmg = 6;
            if (youmonst.data == &mons[PM_SHADE]
                && objects[obj->otyp].oc_material != SILVER)
                dmg = 0;
        }
        if (dmg > 1 && less_damage)
            dmg = 1;
        if (dmg > 0)
            dmg += u.udaminc;
        if (dmg < 0)
            dmg = 0; /* beware negative rings of increase damage */
        dmg = Maybe_Half_Phys(dmg);

        if (uarmh) {
            if (less_damage && dmg < (Upolyd ? u.mh : u.uhp)) {
                if (!artimsg)
                    pline("幸运的是, 你戴着一个坚硬的头盔.");
                /* helmet definitely protects you when it blocks petrification
                 */
            } else if (!petrifier) {
                if (flags.verbose)
                    Your("%s 没有保护你.", helm_simple_name(uarmh));
            }
        } else if (petrifier && !Stone_resistance
                   && !(poly_when_stoned(youmonst.data)
                        && polymon(PM_STONE_GOLEM))) {
        petrify:
            killer.format = KILLED_BY;
            Strcpy(killer.name, "基础物理"); /* "what goes up..." */
            You("变成了石头.");
            if (obj)
                dropy(obj); /* bypass most of hitfloor() */
            thrownobj = 0;  /* now either gone or on floor */
            done(STONING);
            return obj ? TRUE : FALSE;
        }
        hitfloor(obj);
        thrownobj = 0;
        losehp(Maybe_Half_Phys(dmg), "掉落的东西", KILLED_BY_AN);
    }
    return TRUE;
}

/* return true for weapon meant to be thrown; excludes ammo */
STATIC_OVL boolean
throwing_weapon(obj)
struct obj *obj;
{
    return (boolean) (is_missile(obj) || is_spear(obj)
                      /* daggers and knife (excludes scalpel) */
                      || (is_blade(obj) && !is_sword(obj)
                          && (objects[obj->otyp].oc_dir & PIERCE))
                      /* special cases [might want to add AXE] */
                      || obj->otyp == WAR_HAMMER || obj->otyp == AKLYS);
}

/* the currently thrown object is returning to you (not for boomerangs) */
STATIC_OVL void
sho_obj_return_to_u(obj)
struct obj *obj;
{
    /* might already be our location (bounced off a wall) */
    if (bhitpos.x != u.ux || bhitpos.y != u.uy) {
        int x = bhitpos.x - u.dx, y = bhitpos.y - u.dy;

        tmp_at(DISP_FLASH, obj_to_glyph(obj));
        while (x != u.ux || y != u.uy) {
            tmp_at(x, y);
            delay_output();
            x -= u.dx;
            y -= u.dy;
        }
        tmp_at(DISP_END, 0);
    }
}

/* throw an object, NB: obj may be consumed in the process */
void
throwit(obj, wep_mask, twoweap)
struct obj *obj;
long wep_mask; /* used to re-equip returning boomerang */
boolean
    twoweap; /* used to restore twoweapon mode if wielded weapon returns */
{
    register struct monst *mon;
    register int range, urange;
    boolean crossbowing, impaired = (Confusion || Stunned || Blind
                                     || Hallucination || Fumbling);

    notonhead = FALSE; /* reset potentially stale value */
    if ((obj->cursed || obj->greased) && (u.dx || u.dy) && !rn2(7)) {
        boolean slipok = TRUE;
        if (ammo_and_launcher(obj, uwep))
            pline("%s!", Tobjnam(obj, "失败了"));
        else {
            /* only slip if it's greased or meant to be thrown */
            if (obj->greased || throwing_weapon(obj))
                /* BUG: this message is grammatically incorrect if obj has
                   a plural name; greased gloves or boots for instance. */
                pline("%s在你投掷它的时候!", Tobjnam(obj, "滑出"));
            else
                slipok = FALSE;
        }
        if (slipok) {
            u.dx = rn2(3) - 1;
            u.dy = rn2(3) - 1;
            if (!u.dx && !u.dy)
                u.dz = 1;
            impaired = TRUE;
        }
    }

    if ((u.dx || u.dy || (u.dz < 1))
        && calc_capacity((int) obj->owt) > SLT_ENCUMBER
        && (Upolyd ? (u.mh < 5 && u.mh != u.mhmax)
                   : (u.uhp < 10 && u.uhp != u.uhpmax))
        && obj->owt > (unsigned) ((Upolyd ? u.mh : u.uhp) * 2)
        && !Is_airlevel(&u.uz)) {
        You("有很少的持久力, 你没能拿住%s而掉落了.",
            the(xname(obj)));
        exercise(A_CON, FALSE);
        u.dx = u.dy = 0;
        u.dz = 1;
    }

    thrownobj = obj;
    thrownobj->was_thrown = 1;

    if (u.uswallow) {
        mon = u.ustuck;
        bhitpos.x = mon->mx;
        bhitpos.y = mon->my;
    } else if (u.dz) {
        if (u.dz < 0 && Role_if(PM_VALKYRIE) && obj->oartifact == ART_MJOLLNIR
            && !impaired) {
            pline("%s了%s并回到你的手中!", Tobjnam(obj, "打中"),
                  ceiling(u.ux, u.uy));
            obj = addinv(obj);
            (void) encumber_msg();
            setuwep(obj);
            u.twoweap = twoweap;
        } else if (u.dz < 0) {
            (void) toss_up(obj, rn2(5) && !Underwater);
        } else if (u.dz > 0 && u.usteed && obj->oclass == POTION_CLASS
                   && rn2(6)) {
            /* alternative to prayer or wand of opening/spell of knock
               for dealing with cursed saddle:  throw holy water > */
            potionhit(u.usteed, obj, TRUE);
        } else {
            hitfloor(obj);
        }
        thrownobj = (struct obj *) 0;
        return;

    } else if (obj->otyp == BOOMERANG && !Underwater) {
        if (Is_airlevel(&u.uz) || Levitation)
            hurtle(-u.dx, -u.dy, 1, TRUE);
        mon = boomhit(obj, u.dx, u.dy);
        if (mon == &youmonst) { /* the thing was caught */
            exercise(A_DEX, TRUE);
            obj = addinv(obj);
            (void) encumber_msg();
            if (wep_mask && !(obj->owornmask & wep_mask)) {
                setworn(obj, wep_mask);
                u.twoweap = twoweap;
            }
            thrownobj = (struct obj *) 0;
            return;
        }
    } else {
        /* crossbow range is independent of strength */
        crossbowing =
            (ammo_and_launcher(obj, uwep) && weapon_type(uwep) == P_CROSSBOW);
        urange = (crossbowing ? 18 : (int) ACURRSTR) / 2;
        /* balls are easy to throw or at least roll;
         * also, this insures the maximum range of a ball is greater
         * than 1, so the effects from throwing attached balls are
         * actually possible
         */
        if (obj->otyp == HEAVY_IRON_BALL)
            range = urange - (int) (obj->owt / 100);
        else
            range = urange - (int) (obj->owt / 40);
        if (obj == uball) {
            if (u.ustuck)
                range = 1;
            else if (range >= 5)
                range = 5;
        }
        if (range < 1)
            range = 1;

        if (is_ammo(obj)) {
            if (ammo_and_launcher(obj, uwep)) {
                if (crossbowing)
                    range = BOLT_LIM;
                else
                    range++;
            } else if (obj->oclass != GEM_CLASS)
                range /= 2;
        }

        if (Is_airlevel(&u.uz) || Levitation) {
            /* action, reaction... */
            urange -= range;
            if (urange < 1)
                urange = 1;
            range -= urange;
            if (range < 1)
                range = 1;
        }

        if (obj->otyp == BOULDER)
            range = 20; /* you must be giant */
        else if (obj->oartifact == ART_MJOLLNIR)
            range = (range + 1) / 2; /* it's heavy */
        else if (obj == uball && u.utrap && u.utraptype == TT_INFLOOR)
            range = 1;

        if (Underwater)
            range = 1;

        mon = bhit(u.dx, u.dy, range, THROWN_WEAPON,
                   (int FDECL((*), (MONST_P, OBJ_P))) 0,
                   (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
        thrownobj = obj; /* obj may be null now */

        /* have to do this after bhit() so u.ux & u.uy are correct */
        if (Is_airlevel(&u.uz) || Levitation)
            hurtle(-u.dx, -u.dy, urange, TRUE);

        if (!obj)
            return;
    }

    if (mon) {
        boolean obj_gone;

        if (mon->isshk && obj->where == OBJ_MINVENT && obj->ocarry == mon) {
            thrownobj = (struct obj *) 0;
            return; /* alert shk caught it */
        }
        (void) snuff_candle(obj);
        notonhead = (bhitpos.x != mon->mx || bhitpos.y != mon->my);
        obj_gone = thitmonst(mon, obj);
        /* Monster may have been tamed; this frees old mon */
        mon = m_at(bhitpos.x, bhitpos.y);

        /* [perhaps this should be moved into thitmonst or hmon] */
        if (mon && mon->isshk
            && (!inside_shop(u.ux, u.uy)
                || !index(in_rooms(mon->mx, mon->my, SHOPBASE), *u.ushops)))
            hot_pursuit(mon);

        if (obj_gone)
            thrownobj = 0;
    }

    if (!thrownobj) {
        ; /* missile has already been handled */
    } else if (u.uswallow) {
        /* ball is not picked up by monster */
        if (obj != uball)
            (void) mpickobj(u.ustuck, obj);
        thrownobj = (struct obj *) 0;
    } else {
        /* the code following might become part of dropy() */
        if (obj->oartifact == ART_MJOLLNIR && Role_if(PM_VALKYRIE)
            && rn2(100)) {
            /* we must be wearing Gauntlets of Power to get here */
            sho_obj_return_to_u(obj); /* display its flight */

            if (!impaired && rn2(100)) {
                pline("%s你的手中!", Tobjnam(obj, "回到"));
                obj = addinv(obj);
                (void) encumber_msg();
                setuwep(obj);
                u.twoweap = twoweap;
                if (cansee(bhitpos.x, bhitpos.y))
                    newsym(bhitpos.x, bhitpos.y);
            } else {
                int dmg = rn2(2);
                if (!dmg) {
                    pline(Blind ? "%s到达你的%s%s."
                                : "%s了你, 到达你的%s%s.",
                          Blind ? Something : Tobjnam(obj, "回到"),
                          makeplural(body_part(FOOT)),
                          Levitation ? "下" : "");
                } else {
                    dmg += rnd(3);
                    pline(Blind ? "%s了你的%s!"
                                : "%s回到你, 打中你的%s!",
                          Tobjnam(obj, Blind ? "打中" : "飞"),
                          body_part(ARM));
                    (void) artifact_hit((struct monst *) 0, &youmonst, obj,
                                        &dmg, 0);
                    losehp(Maybe_Half_Phys(dmg), killer_xname(obj),
                           KILLED_BY);
                }
                if (ship_object(obj, u.ux, u.uy, FALSE)) {
                    thrownobj = (struct obj *) 0;
                    return;
                }
                dropy(obj);
            }
            thrownobj = (struct obj *) 0;
            return;
        }

        if (!IS_SOFT(levl[bhitpos.x][bhitpos.y].typ) && breaktest(obj)) {
            tmp_at(DISP_FLASH, obj_to_glyph(obj));
            tmp_at(bhitpos.x, bhitpos.y);
            delay_output();
            tmp_at(DISP_END, 0);
            breakmsg(obj, cansee(bhitpos.x, bhitpos.y));
            breakobj(obj, bhitpos.x, bhitpos.y, TRUE, TRUE);
            thrownobj = (struct obj *) 0;
            return;
        }
        if (flooreffects(obj, bhitpos.x, bhitpos.y, "掉落")) {
            thrownobj = (struct obj *) 0;
            return;
        }
        obj_no_longer_held(obj);
        if (mon && mon->isshk && is_pick(obj)) {
            if (cansee(bhitpos.x, bhitpos.y))
                pline("%s 抢夺%s.", Monnam(mon), the(xname(obj)));
            if (*u.ushops || obj->unpaid)
                check_shop_obj(obj, bhitpos.x, bhitpos.y, FALSE);
            (void) mpickobj(mon, obj); /* may merge and free obj */
            thrownobj = (struct obj *) 0;
            return;
        }
        (void) snuff_candle(obj);
        if (!mon && ship_object(obj, bhitpos.x, bhitpos.y, FALSE)) {
            thrownobj = (struct obj *) 0;
            return;
        }
        thrownobj = (struct obj *) 0;
        place_object(obj, bhitpos.x, bhitpos.y);
        /* container contents might break;
           do so before turning ownership of thrownobj over to shk
           (container_impact_dmg handles item already owned by shop) */
        if (!IS_SOFT(levl[bhitpos.x][bhitpos.y].typ))
            /* <x,y> is spot where you initiated throw, not bhitpos */
            container_impact_dmg(obj, u.ux, u.uy);
        /* charge for items thrown out of shop;
           shk takes possession for items thrown into one */
        if ((*u.ushops || obj->unpaid) && obj != uball)
            check_shop_obj(obj, bhitpos.x, bhitpos.y, FALSE);

        stackobj(obj);
        if (obj == uball)
            drop_ball(bhitpos.x, bhitpos.y);
        if (cansee(bhitpos.x, bhitpos.y))
            newsym(bhitpos.x, bhitpos.y);
        if (obj_sheds_light(obj))
            vision_full_recalc = 1;
    }
}

/* an object may hit a monster; various factors adjust the chance of hitting
 */
int
omon_adj(mon, obj, mon_notices)
struct monst *mon;
struct obj *obj;
boolean mon_notices;
{
    int tmp = 0;

    /* size of target affects the chance of hitting */
    tmp += (mon->data->msize - MZ_MEDIUM); /* -2..+5 */
    /* sleeping target is more likely to be hit */
    if (mon->msleeping) {
        tmp += 2;
        if (mon_notices)
            mon->msleeping = 0;
    }
    /* ditto for immobilized target */
    if (!mon->mcanmove || !mon->data->mmove) {
        tmp += 4;
        if (mon_notices && mon->data->mmove && !rn2(10)) {
            mon->mcanmove = 1;
            mon->mfrozen = 0;
        }
    }
    /* some objects are more likely to hit than others */
    switch (obj->otyp) {
    case HEAVY_IRON_BALL:
        if (obj != uball)
            tmp += 2;
        break;
    case BOULDER:
        tmp += 6;
        break;
    default:
        if (obj->oclass == WEAPON_CLASS || is_weptool(obj)
            || obj->oclass == GEM_CLASS)
            tmp += hitval(obj, mon);
        break;
    }
    return tmp;
}

/* thrown object misses target monster */
STATIC_OVL void
tmiss(obj, mon, maybe_wakeup)
struct obj *obj;
struct monst *mon;
boolean maybe_wakeup;
{
    const char *missile = mshot_xname(obj);

    /* If the target can't be seen or doesn't look like a valid target,
       avoid "the arrow misses it," or worse, "the arrows misses the mimic."
       An attentive player will still notice that this is different from
       an arrow just landing short of any target (no message in that case),
       so will realize that there is a valid target here anyway. */
    if (!canseemon(mon) || (mon->m_ap_type && mon->m_ap_type != M_AP_MONSTER))
        pline("%s %s.", The(missile), otense(obj, "没打中"));
    else
        miss(missile, mon);
    if (maybe_wakeup && !rn2(3))
        wakeup(mon);
    return;
}

#define quest_arti_hits_leader(obj, mon)      \
    (obj->oartifact && is_quest_artifact(obj) \
     && mon->m_id == quest_status.leader_m_id)

/*
 * Object thrown by player arrives at monster's location.
 * Return 1 if obj has disappeared or otherwise been taken care of,
 * 0 if caller must take care of it.
 * Also used for kicked objects and for polearms/grapnel applied at range.
 */
int
thitmonst(mon, obj)
register struct monst *mon;
register struct obj *obj; /* thrownobj or kickedobj or uwep */
{
    register int tmp;     /* Base chance to hit */
    register int disttmp; /* distance modifier */
    int otyp = obj->otyp, hmode;
    boolean guaranteed_hit = (u.uswallow && mon == u.ustuck);

    hmode = (obj == uwep) ? HMON_APPLIED : (obj == kickedobj) ? HMON_KICKED
                                                              : HMON_THROWN;

    /* Differences from melee weapons:
     *
     * Dex still gives a bonus, but strength does not.
     * Polymorphed players lacking attacks may still throw.
     * There's a base -1 to hit.
     * No bonuses for fleeing or stunned targets (they don't dodge
     *    melee blows as readily, but dodging arrows is hard anyway).
     * Not affected by traps, etc.
     * Certain items which don't in themselves do damage ignore tmp.
     * Distance and monster size affect chance to hit.
     */
    tmp = -1 + Luck + find_mac(mon) + u.uhitinc
          + maybe_polyd(youmonst.data->mlevel, u.ulevel);
    if (ACURR(A_DEX) < 4)
        tmp -= 3;
    else if (ACURR(A_DEX) < 6)
        tmp -= 2;
    else if (ACURR(A_DEX) < 8)
        tmp -= 1;
    else if (ACURR(A_DEX) >= 14)
        tmp += (ACURR(A_DEX) - 14);

    /* Modify to-hit depending on distance; but keep it sane.
     * Polearms get a distance penalty even when wielded; it's
     * hard to hit at a distance.
     */
    disttmp = 3 - distmin(u.ux, u.uy, mon->mx, mon->my);
    if (disttmp < -4)
        disttmp = -4;
    tmp += disttmp;

    /* gloves are a hindrance to proper use of bows */
    if (uarmg && uwep && objects[uwep->otyp].oc_skill == P_BOW) {
        switch (uarmg->otyp) {
        case GAUNTLETS_OF_POWER: /* metal */
            tmp -= 2;
            break;
        case GAUNTLETS_OF_FUMBLING:
            tmp -= 3;
            break;
        case LEATHER_GLOVES:
        case GAUNTLETS_OF_DEXTERITY:
            break;
        default:
            impossible("Unknown type of gloves (%d)", uarmg->otyp);
            break;
        }
    }

    tmp += omon_adj(mon, obj, TRUE);
    if (is_orc(mon->data)
        && maybe_polyd(is_elf(youmonst.data), Race_if(PM_ELF)))
        tmp++;
    if (guaranteed_hit) {
        tmp += 1000; /* Guaranteed hit */
    }

    if (obj->oclass == GEM_CLASS && is_unicorn(mon->data)) {
        if (mon->msleeping || !mon->mcanmove) {
            tmiss(obj, mon, FALSE);
            return 0;
        } else if (mon->mtame) {
            pline("%s 接住并扔掉了%s.", Monnam(mon), the(xname(obj)));
            return 0;
        } else {
            pline("%s 接住了%s.", Monnam(mon), the(xname(obj)));
            return gem_accept(mon, obj);
        }
    }

    /* don't make game unwinnable if naive player throws artifact
       at leader... (kicked artifact is ok too; HMON_APPLIED could
       occur if quest artifact polearm or grapnel ever gets added) */
    if (hmode != HMON_APPLIED && quest_arti_hits_leader(obj, mon)) {
        /* not wakeup(), which angers non-tame monsters */
        mon->msleeping = 0;
        mon->mstrategy &= ~STRAT_WAITMASK;

        if (mon->mcanmove) {
            pline("%s 接住了%s.", Monnam(mon), the(xname(obj)));
            if (mon->mpeaceful) {
                boolean next2u = monnear(mon, u.ux, u.uy);

                finish_quest(obj); /* acknowledge quest completion */
                pline("%s %s %s给你.", Monnam(mon),
                      (next2u ? "归还" : "投回"), the(xname(obj)));
                if (!next2u)
                    sho_obj_return_to_u(obj);
                obj = addinv(obj); /* back into your inventory */
                (void) encumber_msg();
            } else {
                /* angry leader caught it and isn't returning it */
                if (*u.ushops || obj->unpaid) /* not very likely... */
                    check_shop_obj(obj, mon->mx, mon->my, FALSE);
                (void) mpickobj(mon, obj);
            }
            return 1; /* caller doesn't need to place it */
        }
        return 0;
    }

    if (obj->oclass == WEAPON_CLASS || is_weptool(obj)
        || obj->oclass == GEM_CLASS) {
        if (hmode == HMON_KICKED) {
            /* throwing adjustments and weapon skill bonus don't apply */
            tmp -= (is_ammo(obj) ? 5 : 3);
        } else if (is_ammo(obj)) {
            if (!ammo_and_launcher(obj, uwep)) {
                tmp -= 4;
            } else {
                tmp += uwep->spe - greatest_erosion(uwep);
                tmp += weapon_hit_bonus(uwep);
                if (uwep->oartifact)
                    tmp += spec_abon(uwep, mon);
                /*
                 * Elves and Samurais are highly trained w/bows,
                 * especially their own special types of bow.
                 * Polymorphing won't make you a bow expert.
                 */
                if ((Race_if(PM_ELF) || Role_if(PM_SAMURAI))
                    && (!Upolyd || your_race(youmonst.data))
                    && objects[uwep->otyp].oc_skill == P_BOW) {
                    tmp++;
                    if (Race_if(PM_ELF) && uwep->otyp == ELVEN_BOW)
                        tmp++;
                    else if (Role_if(PM_SAMURAI) && uwep->otyp == YUMI)
                        tmp++;
                }
            }
        } else { /* thrown non-ammo or applied polearm/grapnel */
            if (otyp == BOOMERANG) /* arbitrary */
                tmp += 4;
            else if (throwing_weapon(obj)) /* meant to be thrown */
                tmp += 2;
            else if (obj == thrownobj) /* not meant to be thrown */
                tmp -= 2;
            /* we know we're dealing with a weapon or weptool handled
               by WEAPON_SKILLS once ammo objects have been excluded */
            tmp += weapon_hit_bonus(obj);
        }

        if (tmp >= rnd(20)) {
            if (hmode == HMON_APPLIED)
                u.uconduct.weaphit++;
            if (hmon(mon, obj, hmode)) { /* mon still alive */
                cutworm(mon, bhitpos.x, bhitpos.y, obj);
            }
            exercise(A_DEX, TRUE);
            /* if hero is swallowed and projectile kills the engulfer,
               obj gets added to engulfer's inventory and then dropped,
               so we can't safely use that pointer anymore; it escapes
               the chance to be used up here... */
            if (!thrownobj)
                return 1;
            /* projectiles other than magic stones
               sometimes disappear when thrown */
            if (objects[otyp].oc_skill < P_NONE
                && objects[otyp].oc_skill > -P_BOOMERANG
                && !objects[otyp].oc_magic) {
                /* we were breaking 2/3 of everything unconditionally.
                 * we still don't want anything to survive unconditionally,
                 * but we need ammo to stay around longer on average.
                 */
                int broken, chance;
                chance = 3 + greatest_erosion(obj) - obj->spe;
                if (chance > 1)
                    broken = rn2(chance);
                else
                    broken = !rn2(4);
                if (obj->blessed && !rnl(4))
                    broken = 0;

                if (broken) {
                    if (*u.ushops || obj->unpaid)
                        check_shop_obj(obj, bhitpos.x, bhitpos.y, TRUE);
                    obfree(obj, (struct obj *) 0);
                    return 1;
                }
            }
            passive_obj(mon, obj, (struct attack *) 0);
        } else {
            tmiss(obj, mon, TRUE);
            if (hmode == HMON_APPLIED)
                wakeup(mon);
        }

    } else if (otyp == HEAVY_IRON_BALL) {
        exercise(A_STR, TRUE);
        if (tmp >= rnd(20)) {
            int was_swallowed = guaranteed_hit;

            exercise(A_DEX, TRUE);
            if (!hmon(mon, obj, hmode)) { /* mon killed */
                if (was_swallowed && !u.uswallow && obj == uball)
                    return 1; /* already did placebc() */
            }
        } else {
            tmiss(obj, mon, TRUE);
        }

    } else if (otyp == BOULDER) {
        exercise(A_STR, TRUE);
        if (tmp >= rnd(20)) {
            exercise(A_DEX, TRUE);
            (void) hmon(mon, obj, hmode);
        } else {
            tmiss(obj, mon, TRUE);
        }

    } else if ((otyp == EGG || otyp == CREAM_PIE || otyp == BLINDING_VENOM
                || otyp == ACID_VENOM)
               && (guaranteed_hit || ACURR(A_DEX) > rnd(25))) {
        (void) hmon(mon, obj, hmode);
        return 1; /* hmon used it up */

    } else if (obj->oclass == POTION_CLASS
               && (guaranteed_hit || ACURR(A_DEX) > rnd(25))) {
        potionhit(mon, obj, TRUE);
        return 1;

    } else if (befriend_with_obj(mon->data, obj)
               || (mon->mtame && dogfood(mon, obj) <= ACCFOOD)) {
        if (tamedog(mon, obj)) {
            return 1; /* obj is gone */
        } else {
            tmiss(obj, mon, FALSE);
            mon->msleeping = 0;
            mon->mstrategy &= ~STRAT_WAITMASK;
        }
    } else if (guaranteed_hit) {
        /* this assumes that guaranteed_hit is due to swallowing */
        wakeup(mon);
        if (obj->otyp == CORPSE && touch_petrifies(&mons[obj->corpsenm])) {
            if (is_animal(u.ustuck->data)) {
                minstapetrify(u.ustuck, TRUE);
                /* Don't leave a cockatrice corpse available in a statue */
                if (!u.uswallow) {
                    delobj(obj);
                    return 1;
                }
            }
        }
        pline("%s 进%s %s.", Tobjnam(obj, "消失"),
              s_suffix(mon_nam(mon)),
              is_animal(u.ustuck->data) ? "内部" : "气流");
    } else {
        tmiss(obj, mon, TRUE);
    }

    return 0;
}

STATIC_OVL int
gem_accept(mon, obj)
register struct monst *mon;
register struct obj *obj;
{
    char buf[BUFSZ];
    boolean is_buddy = sgn(mon->data->maligntyp) == sgn(u.ualign.type);
    boolean is_gem = objects[obj->otyp].oc_material == GEMSTONE;
    int ret = 0;
    static NEARDATA const char nogood[] = "对你的垃圾不感兴趣.";
    static NEARDATA const char acceptgift[] = "接受了你的礼物.";
    static NEARDATA const char maybeluck[] = "踌躇地";
    static NEARDATA const char noluck[] = "和蔼地";
    static NEARDATA const char addluck[] = "感激地";

    Strcpy(buf, Monnam(mon));
    mon->mpeaceful = 1;
    mon->mavenge = 0;

    /* object properly identified */
    if (obj->dknown && objects[obj->otyp].oc_name_known) {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(5);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(7) - 3);
            }
        } else {
            Strcat(buf, nogood);
            goto nopick;
        }
        /* making guesses */
    } else if (has_oname(obj) || objects[obj->otyp].oc_uname) {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(2);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(3) - 1);
            }
        } else {
            Strcat(buf, nogood);
            goto nopick;
        }
        /* value completely unknown to @ */
    } else {
        if (is_gem) {
            if (is_buddy) {
                Strcat(buf, addluck);
                change_luck(1);
            } else {
                Strcat(buf, maybeluck);
                change_luck(rn2(3) - 1);
            }
        } else {
            Strcat(buf, noluck);
        }
    }
    Strcat(buf, acceptgift);
    if (*u.ushops || obj->unpaid)
        check_shop_obj(obj, mon->mx, mon->my, TRUE);
    (void) mpickobj(mon, obj); /* may merge and free obj */
    ret = 1;

nopick:
    if (!Blind)
        pline1(buf);
    if (!tele_restrict(mon))
        (void) rloc(mon, TRUE);
    return ret;
}

/*
 * Comments about the restructuring of the old breaks() routine.
 *
 * There are now three distinct phases to object breaking:
 *     breaktest() - which makes the check/decision about whether the
 *                   object is going to break.
 *     breakmsg()  - which outputs a message about the breakage,
 *                   appropriate for that particular object. Should
 *                   only be called after a positive breaktest().
 *                   on the object and, if it going to be called,
 *                   it must be called before calling breakobj().
 *                   Calling breakmsg() is optional.
 *     breakobj()  - which actually does the breakage and the side-effects
 *                   of breaking that particular object. This should
 *                   only be called after a positive breaktest() on the
 *                   object.
 *
 * Each of the above routines is currently static to this source module.
 * There are two routines callable from outside this source module which
 * perform the routines above in the correct sequence.
 *
 *   hero_breaks() - called when an object is to be broken as a result
 *                   of something that the hero has done. (throwing it,
 *                   kicking it, etc.)
 *   breaks()      - called when an object is to be broken for some
 *                   reason other than the hero doing something to it.
 */

/*
 * The hero causes breakage of an object (throwing, dropping it, etc.)
 * Return 0 if the object didn't break, 1 if the object broke.
 */
int
hero_breaks(obj, x, y, from_invent)
struct obj *obj;
xchar x, y;          /* object location (ox, oy may not be right) */
boolean from_invent; /* thrown or dropped by player; maybe on shop bill */
{
    boolean in_view = Blind ? FALSE : (from_invent || cansee(x, y));
    if (!breaktest(obj))
        return 0;
    breakmsg(obj, in_view);
    breakobj(obj, x, y, TRUE, from_invent);
    return 1;
}

/*
 * The object is going to break for a reason other than the hero doing
 * something to it.
 * Return 0 if the object doesn't break, 1 if the object broke.
 */
int
breaks(obj, x, y)
struct obj *obj;
xchar x, y; /* object location (ox, oy may not be right) */
{
    boolean in_view = Blind ? FALSE : cansee(x, y);

    if (!breaktest(obj))
        return 0;
    breakmsg(obj, in_view);
    breakobj(obj, x, y, FALSE, FALSE);
    return 1;
}

void
release_camera_demon(obj, x, y)
struct obj *obj;
xchar x, y;
{
    struct monst *mtmp;
    if (!rn2(3)
        && (mtmp = makemon(&mons[rn2(3) ? PM_HOMUNCULUS : PM_IMP], x, y,
                           NO_MM_FLAGS)) != 0) {
        if (canspotmon(mtmp))
            pline("%s 被释放了!", Hallucination
                                         ? rndmonnam(NULL)
                                         : "画图恶魔");
        mtmp->mpeaceful = !obj->cursed;
        set_malign(mtmp);
    }
}

/*
 * Unconditionally break an object. Assumes all resistance checks
 * and break messages have been delivered prior to getting here.
 */
void
breakobj(obj, x, y, hero_caused, from_invent)
struct obj *obj;
xchar x, y;          /* object location (ox, oy may not be right) */
boolean hero_caused; /* is this the hero's fault? */
boolean from_invent;
{
    boolean fracture = FALSE;

    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    case MIRROR:
        if (hero_caused)
            change_luck(-2);
        break;
    case POT_WATER:      /* really, all potions */
        obj->in_use = 1; /* in case it's fatal */
        if (obj->otyp == POT_OIL && obj->lamplit) {
            explode_oil(obj, x, y);
        } else if (distu(x, y) <= 2) {
            if (!breathless(youmonst.data) || haseyes(youmonst.data)) {
                if (obj->otyp != POT_WATER) {
                    if (!breathless(youmonst.data)) {
                        /* [what about "familiar odor" when known?] */
                        You("闻到了古怪的气味...");
                    } else {
                        const char *eyes = body_part(EYE);

                        if (eyecount(youmonst.data) != 1)
                            eyes = makeplural(eyes);
                        Your("%s %s.", eyes, vtense(eyes, "流泪"));
                    }
                }
                potionbreathe(obj);
            }
        }
        /* monster breathing isn't handled... [yet?] */
        break;
    case EXPENSIVE_CAMERA:
        release_camera_demon(obj, x, y);
        break;
    case EGG:
        /* breaking your own eggs is bad luck */
        if (hero_caused && obj->spe && obj->corpsenm >= LOW_PM)
            change_luck((schar) -min(obj->quan, 5L));
        break;
    case BOULDER:
    case STATUE:
        /* caller will handle object disposition;
           we're just doing the shop theft handling */
        fracture = TRUE;
        break;
    default:
        break;
    }

    if (hero_caused) {
        if (from_invent || obj->unpaid) {
            if (*u.ushops || obj->unpaid)
                check_shop_obj(obj, x, y, TRUE);
        } else if (!obj->no_charge && costly_spot(x, y)) {
            /* it is assumed that the obj is a floor-object */
            char *o_shop = in_rooms(x, y, SHOPBASE);
            struct monst *shkp = shop_keeper(*o_shop);

            if (shkp) { /* (implies *o_shop != '\0') */
                static NEARDATA long lastmovetime = 0L;
                static NEARDATA boolean peaceful_shk = FALSE;
                /*  We want to base shk actions on her peacefulness
                    at start of this turn, so that "simultaneous"
                    multiple breakage isn't drastically worse than
                    single breakage.  (ought to be done via ESHK)  */
                if (moves != lastmovetime)
                    peaceful_shk = shkp->mpeaceful;
                if (stolen_value(obj, x, y, peaceful_shk, FALSE) > 0L
                    && (*o_shop != u.ushops[0] || !inside_shop(u.ux, u.uy))
                    && moves != lastmovetime)
                    make_angry_shk(shkp, x, y);
                lastmovetime = moves;
            }
        }
    }
    if (!fracture)
        delobj(obj);
}

/*
 * Check to see if obj is going to break, but don't actually break it.
 * Return 0 if the object isn't going to break, 1 if it is.
 */
boolean
breaktest(obj)
struct obj *obj;
{
    if (obj_resists(obj, 1, 99))
        return 0;
    if (objects[obj->otyp].oc_material == GLASS && !obj->oartifact
        && obj->oclass != GEM_CLASS)
        return 1;
    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    case EXPENSIVE_CAMERA:
    case POT_WATER: /* really, all potions */
    case EGG:
    case CREAM_PIE:
    case MELON:
    case ACID_VENOM:
    case BLINDING_VENOM:
        return 1;
    default:
        return 0;
    }
}

STATIC_OVL void
breakmsg(obj, in_view)
struct obj *obj;
boolean in_view;
{
    const char *to_pieces;

    to_pieces = "";
    switch (obj->oclass == POTION_CLASS ? POT_WATER : obj->otyp) {
    default: /* glass or crystal wand */
        if (obj->oclass != WAND_CLASS)
            impossible("breaking odd object?");
    case CRYSTAL_PLATE_MAIL:
    case LENSES:
    case MIRROR:
    case CRYSTAL_BALL:
    case EXPENSIVE_CAMERA:
        to_pieces = " 成了许多碎片";
    /*FALLTHRU*/
    case POT_WATER: /* really, all potions */
        if (!in_view)
            You_hear("%s 破碎了!", something);
        else
            pline("%s 破碎%s%s!", Doname2(obj),
                  (obj->quan == 1L) ? "" : "", to_pieces);
        break;
    case EGG:
    case MELON:
        pline("啪嗒!");
        break;
    case CREAM_PIE:
        if (in_view)
            pline("真是一团糟!");
        break;
    case ACID_VENOM:
    case BLINDING_VENOM:
        pline("飞溅!");
        break;
    }
}

STATIC_OVL int
throw_gold(obj)
struct obj *obj;
{
    int range, odx, ody;
    register struct monst *mon;

    if (!u.dx && !u.dy && !u.dz) {
        You("不能向自己投掷金币.");
        return 0;
    }
    freeinv(obj);
    if (u.uswallow) {
        pline(is_animal(u.ustuck->data) ? "%s 在%s 的内部."
                                        : "%s 进%s.",
              "钱消失", mon_nam(u.ustuck));
        add_to_minv(u.ustuck, obj);
        return 1;
    }

    if (u.dz) {
        if (u.dz < 0 && !Is_airlevel(&u.uz) && !Underwater
            && !Is_waterlevel(&u.uz)) {
            pline_The("金币打中了%s, 然后掉落到你的%s上.",
                      ceiling(u.ux, u.uy), body_part(HEAD));
            /* some self damage? */
            if (uarmh)
                pline("幸运的是, 你穿戴着%s!",
                      helm_simple_name(uarmh));
        }
        bhitpos.x = u.ux;
        bhitpos.y = u.uy;
    } else {
        /* consistent with range for normal objects */
        range = (int) ((ACURRSTR) / 2 - obj->owt / 40);

        /* see if the gold has a place to move into */
        odx = u.ux + u.dx;
        ody = u.uy + u.dy;
        if (!ZAP_POS(levl[odx][ody].typ) || closed_door(odx, ody)) {
            bhitpos.x = u.ux;
            bhitpos.y = u.uy;
        } else {
            mon = bhit(u.dx, u.dy, range, THROWN_WEAPON,
                       (int FDECL((*), (MONST_P, OBJ_P))) 0,
                       (int FDECL((*), (OBJ_P, OBJ_P))) 0, &obj);
            if (!obj)
                return 1; /* object is gone */
            if (mon) {
                if (ghitm(mon, obj)) /* was it caught? */
                    return 1;
            } else {
                if (ship_object(obj, bhitpos.x, bhitpos.y, FALSE))
                    return 1;
            }
        }
    }

    if (flooreffects(obj, bhitpos.x, bhitpos.y, "掉落"))
        return 1;
    if (u.dz > 0)
        pline_The("金币打中了%s.", surface(bhitpos.x, bhitpos.y));
    place_object(obj, bhitpos.x, bhitpos.y);
    if (*u.ushops)
        sellobj(obj, bhitpos.x, bhitpos.y);
    stackobj(obj);
    newsym(bhitpos.x, bhitpos.y);
    return 1;
}

/*dothrow.c*/
