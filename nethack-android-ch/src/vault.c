/* NetHack 3.6	vault.c	$NHDT-Date: 1446078792 2015/10/29 00:33:12 $  $NHDT-Branch: master $:$NHDT-Revision: 1.39 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL struct monst *NDECL(findgd);

STATIC_DCL boolean FDECL(clear_fcorr, (struct monst *, BOOLEAN_P));
STATIC_DCL void FDECL(blackout, (int, int));
STATIC_DCL void FDECL(restfakecorr, (struct monst *));
STATIC_DCL boolean FDECL(in_fcorridor, (struct monst *, int, int));
STATIC_DCL void FDECL(move_gold, (struct obj *, int));
STATIC_DCL void FDECL(wallify_vault, (struct monst *));

void
newegd(mtmp)
struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EGD(mtmp)) {
        EGD(mtmp) = (struct egd *) alloc(sizeof(struct egd));
        (void) memset((genericptr_t) EGD(mtmp), 0, sizeof(struct egd));
    }
}

void
free_egd(mtmp)
struct monst *mtmp;
{
    if (mtmp->mextra && EGD(mtmp)) {
        free((genericptr_t) EGD(mtmp));
        EGD(mtmp) = (struct egd *) 0;
    }
    mtmp->isgd = 0;
}

STATIC_OVL boolean
clear_fcorr(grd, forceshow)
struct monst *grd;
boolean forceshow;
{
    register int fcx, fcy, fcbeg;
    struct monst *mtmp;
    boolean sawcorridor = FALSE;
    struct egd *egrd = EGD(grd);
    struct trap *trap;
    struct rm *lev;

    if (!on_level(&egrd->gdlevel, &u.uz))
        return TRUE;

    while ((fcbeg = egrd->fcbeg) < egrd->fcend) {
        fcx = egrd->fakecorr[fcbeg].fx;
        fcy = egrd->fakecorr[fcbeg].fy;
        if ((grd->mhp <= 0 || !in_fcorridor(grd, u.ux, u.uy)) && egrd->gddone)
            forceshow = TRUE;
        if ((u.ux == fcx && u.uy == fcy && grd->mhp > 0)
            || (!forceshow && couldsee(fcx, fcy))
            || (Punished && !carried(uball) && uball->ox == fcx
                && uball->oy == fcy))
            return FALSE;

        if ((mtmp = m_at(fcx, fcy)) != 0) {
            if (mtmp->isgd) {
                return FALSE;
            } else if (!in_fcorridor(grd, u.ux, u.uy)) {
                if (mtmp->mtame)
                    yelp(mtmp);
                (void) rloc(mtmp, FALSE);
            }
        }
        lev = &levl[fcx][fcy];
        if (lev->typ == CORR && cansee(fcx, fcy))
            sawcorridor = TRUE;
        lev->typ = egrd->fakecorr[fcbeg].ftyp;
        if (IS_STWALL(lev->typ)) {
            /* destroy any trap here (pit dug by you, hole dug via
               wand while levitating or by monster, bear trap or land
               mine via object, spun web) when spot reverts to stone */
            if ((trap = t_at(fcx, fcy)) != 0)
                deltrap(trap);
            /* undo scroll/wand/spell of light affecting this spot */
            if (lev->typ == STONE)
                blackout(fcx, fcy);
        }
        map_location(fcx, fcy, 1); /* bypass vision */
        if (!ACCESSIBLE(lev->typ))
            block_point(fcx, fcy);
        vision_full_recalc = 1;
        egrd->fcbeg++;
    }
    if (sawcorridor)
        pline_The("通道消失了.");
    if (IS_ROCK(levl[u.ux][u.uy].typ))
        You("被围在岩石里.");
    return TRUE;
}

/* as a temporary corridor is removed, set stone locations and adjacent
   spots to unlit; if player used scroll/wand/spell of light while inside
   the corridor, we don't want the light to reappear if/when a new tunnel
   goes through the same area */
STATIC_OVL void
blackout(x, y)
int x, y;
{
    struct rm *lev;
    int i, j;

    for (i = x - 1; i <= x + 1; ++i)
        for (j = y - 1; j <= y + 1; ++j) {
            if (!isok(i, j))
                continue;
            lev = &levl[i][j];
            /* [possible bug: when (i != x || j != y), perhaps we ought
               to check whether the spot on the far side is lit instead
               of doing a blanket blackout of adjacent locations] */
            if (lev->typ == STONE)
                lev->lit = lev->waslit = 0;
            /* mark <i,j> as not having been seen from <x,y> */
            unset_seenv(lev, x, y, i, j);
        }
}

STATIC_OVL void
restfakecorr(grd)
struct monst *grd;
{
    /* it seems you left the corridor - let the guard disappear */
    if (clear_fcorr(grd, FALSE)) {
        grd->isgd = 0; /* dmonsfree() should delete this mon */
        mongone(grd);
    }
}

/* called in mon.c */
boolean
grddead(grd)
struct monst *grd;
{
    boolean dispose = clear_fcorr(grd, TRUE);

    if (!dispose) {
        /* destroy guard's gold; drop any other inventory */
        relobj(grd, 0, FALSE);
        /* guard is dead; monster traversal loops should skip it */
        grd->mhp = 0;
        if (grd == context.polearm.hitmon)
            context.polearm.hitmon = 0;
        /* see comment by newpos in gd_move() */
        remove_monster(grd->mx, grd->my);
        newsym(grd->mx, grd->my);
        place_monster(grd, 0, 0);
        EGD(grd)->ogx = grd->mx;
        EGD(grd)->ogy = grd->my;
        dispose = clear_fcorr(grd, TRUE);
    }
    if (dispose)
        grd->isgd = 0; /* for dmonsfree() */
    return dispose;
}

STATIC_OVL boolean
in_fcorridor(grd, x, y)
struct monst *grd;
int x, y;
{
    register int fci;
    struct egd *egrd = EGD(grd);

    for (fci = egrd->fcbeg; fci < egrd->fcend; fci++)
        if (x == egrd->fakecorr[fci].fx && y == egrd->fakecorr[fci].fy)
            return TRUE;
    return FALSE;
}

STATIC_OVL
struct monst *
findgd()
{
    register struct monst *mtmp;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp))
            continue;
        if (mtmp->isgd && on_level(&(EGD(mtmp)->gdlevel), &u.uz))
            return mtmp;
    }
    return (struct monst *) 0;
}

char
vault_occupied(array)
char *array;
{
    register char *ptr;

    for (ptr = array; *ptr; ptr++)
        if (rooms[*ptr - ROOMOFFSET].rtype == VAULT)
            return *ptr;
    return '\0';
}

void
invault()
{
#ifdef BSD_43_BUG
    int dummy; /* hack to avoid schain botch */
#endif
    struct monst *guard;
    boolean gsensed;
    int trycount, vaultroom = (int) vault_occupied(u.urooms);

    if (!vaultroom) {
        u.uinvault = 0;
        return;
    }

    vaultroom -= ROOMOFFSET;

    guard = findgd();
    if (++u.uinvault % 30 == 0 && !guard) { /* if time ok and no guard now. */
        char buf[BUFSZ];
        register int x, y, dd, gx, gy;
        int lx = 0, ly = 0;
        long umoney;

        /* first find the goal for the guard */
        for (dd = 2; (dd < ROWNO || dd < COLNO); dd++) {
            for (y = u.uy - dd; y <= u.uy + dd; ly = y, y++) {
                if (y < 0 || y > ROWNO - 1)
                    continue;
                for (x = u.ux - dd; x <= u.ux + dd; lx = x, x++) {
                    if (y != u.uy - dd && y != u.uy + dd && x != u.ux - dd)
                        x = u.ux + dd;
                    if (x < 1 || x > COLNO - 1)
                        continue;
                    if (levl[x][y].typ == CORR) {
                        if (x < u.ux)
                            lx = x + 1;
                        else if (x > u.ux)
                            lx = x - 1;
                        else
                            lx = x;
                        if (y < u.uy)
                            ly = y + 1;
                        else if (y > u.uy)
                            ly = y - 1;
                        else
                            ly = y;
                        if (levl[lx][ly].typ != STONE
                            && levl[lx][ly].typ != CORR)
                            goto incr_radius;
                        goto fnd;
                    }
                }
            }
        incr_radius:
            ;
        }
        impossible("Not a single corridor on this level??");
        tele();
        return;
    fnd:
        gx = x;
        gy = y;

        /* next find a good place for a door in the wall */
        x = u.ux;
        y = u.uy;
        if (levl[x][y].typ != ROOM) { /* player dug a door and is in it */
            if (levl[x + 1][y].typ == ROOM)
                x = x + 1;
            else if (levl[x][y + 1].typ == ROOM)
                y = y + 1;
            else if (levl[x - 1][y].typ == ROOM)
                x = x - 1;
            else if (levl[x][y - 1].typ == ROOM)
                y = y - 1;
            else if (levl[x + 1][y + 1].typ == ROOM) {
                x = x + 1;
                y = y + 1;
            } else if (levl[x - 1][y - 1].typ == ROOM) {
                x = x - 1;
                y = y - 1;
            } else if (levl[x + 1][y - 1].typ == ROOM) {
                x = x + 1;
                y = y - 1;
            } else if (levl[x - 1][y + 1].typ == ROOM) {
                x = x - 1;
                y = y + 1;
            }
        }
        while (levl[x][y].typ == ROOM) {
            register int dx, dy;

            dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
            dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
            if (abs(gx - x) >= abs(gy - y))
                x += dx;
            else
                y += dy;
        }
        if (x == u.ux && y == u.uy) {
            if (levl[x + 1][y].typ == HWALL || levl[x + 1][y].typ == DOOR)
                x = x + 1;
            else if (levl[x - 1][y].typ == HWALL
                     || levl[x - 1][y].typ == DOOR)
                x = x - 1;
            else if (levl[x][y + 1].typ == VWALL
                     || levl[x][y + 1].typ == DOOR)
                y = y + 1;
            else if (levl[x][y - 1].typ == VWALL
                     || levl[x][y - 1].typ == DOOR)
                y = y - 1;
            else
                return;
        }

        /* make something interesting happen */
        if (!(guard = makemon(&mons[PM_GUARD], x, y, MM_EGD)))
            return;
        guard->isgd = 1;
        guard->mpeaceful = 1;
        set_malign(guard);
        EGD(guard)->gddone = 0;
        EGD(guard)->ogx = x;
        EGD(guard)->ogy = y;
        assign_level(&(EGD(guard)->gdlevel), &u.uz);
        EGD(guard)->vroom = vaultroom;
        EGD(guard)->warncnt = 0;

        reset_faint(); /* if fainted - wake up */
        gsensed = !canspotmon(guard);
        if (!gsensed)
            pline("突然一个金库的%s 进来了!",
                  makeplural(guard->data->mname));
        else
            pline("有人已经进入了金库.");
        newsym(guard->mx, guard->my);
        if (u.uswallow) {
            /* can't interrogate hero, don't interrogate engulfer */
            verbalize("这里发生了什么事?");
            if (gsensed)
                pline_The("其他人不见了踪影.");
            mongone(guard);
            return;
        }
        if (youmonst.m_ap_type == M_AP_OBJECT || u.uundetected) {
            if (youmonst.m_ap_type == M_AP_OBJECT
                && youmonst.mappearance != GOLD_PIECE)
                verbalize("嘿! 谁把%s 放在这儿的?",
                          mimic_obj_name(&youmonst));
            /* You're mimicking some object or you're hidden. */
            pline("很困惑, %s 转身离开.", mhe(guard));
            mongone(guard);
            return;
        }
        if (Strangled || is_silent(youmonst.data) || multi < 0) {
            /* [we ought to record whether this this message has already
               been given in order to vary it upon repeat visits, but
               discarding the monster and its egd data renders that hard] */
            verbalize("当你准备跟我说话时, 我会回来的!");
            mongone(guard);
            return;
        }

        stop_occupation(); /* if occupied, stop it *now* */
        if (multi > 0) {
            nomul(0);
            unmul((char *) 0);
        }
        trycount = 5;
        do {
            getlin("\" 你好陌生人, 你是谁?\" -", buf);
            (void) mungspaces(buf);
        } while (!letter(buf[0]) && --trycount > 0);

        if (u.ualign.type == A_LAWFUL
            /* ignore trailing text, in case player includes rank */
            && strncmpi(buf, plname, (int) strlen(plname)) != 0) {
            adjalign(-1); /* Liar! */
        }

        if (!strcmpi(buf, "Croesus") || !strcmpi(buf, "Kroisos")
            || !strcmpi(buf, "Creosote") || !strcmpi(buf, "克罗伊斯")) {
            if (!mvitals[PM_CROESUS].died) {
                verbalize(
                    "哦, 是的, 当然.  抱歉打扰你了.");
                mongone(guard);
            } else {
                setmangry(guard);
                verbalize("你死而复生?  我会纠正你!");
                /* don't want guard to waste next turn wielding a weapon */
                if (!MON_WEP(guard)) {
                    guard->weapon_check = NEED_HTH_WEAPON;
                    (void) mon_wield_item(guard);
                }
            }
            return;
        }
        verbalize("我不认识你.");
        umoney = money_cnt(invent);
        if (Deaf) {
            ;
        } else if (!umoney && !hidden_gold()) {
            verbalize("请跟我来.");
        } else {
            if (!umoney)
                verbalize("你隐藏了金币.");
            verbalize(
                "很可能你所有的金币都是从这个的金库偷的.");
            verbalize("请扔掉金币然后跟着我.");
        }
        EGD(guard)->gdx = gx;
        EGD(guard)->gdy = gy;
        EGD(guard)->fcbeg = 0;
        EGD(guard)->fakecorr[0].fx = x;
        EGD(guard)->fakecorr[0].fy = y;
        if (IS_WALL(levl[x][y].typ))
            EGD(guard)->fakecorr[0].ftyp = levl[x][y].typ;
        else { /* the initial guard location is a dug door */
            int vlt = EGD(guard)->vroom;
            xchar lowx = rooms[vlt].lx, hix = rooms[vlt].hx;
            xchar lowy = rooms[vlt].ly, hiy = rooms[vlt].hy;

            if (x == lowx - 1 && y == lowy - 1)
                EGD(guard)->fakecorr[0].ftyp = TLCORNER;
            else if (x == hix + 1 && y == lowy - 1)
                EGD(guard)->fakecorr[0].ftyp = TRCORNER;
            else if (x == lowx - 1 && y == hiy + 1)
                EGD(guard)->fakecorr[0].ftyp = BLCORNER;
            else if (x == hix + 1 && y == hiy + 1)
                EGD(guard)->fakecorr[0].ftyp = BRCORNER;
            else if (y == lowy - 1 || y == hiy + 1)
                EGD(guard)->fakecorr[0].ftyp = HWALL;
            else if (x == lowx - 1 || x == hix + 1)
                EGD(guard)->fakecorr[0].ftyp = VWALL;
        }
        levl[x][y].typ = DOOR;
        levl[x][y].doormask = D_NODOOR;
        unblock_point(x, y); /* doesn't block light */
        EGD(guard)->fcend = 1;
        EGD(guard)->warncnt = 1;
    }
}

STATIC_OVL void
move_gold(gold, vroom)
struct obj *gold;
int vroom;
{
    xchar nx, ny;

    remove_object(gold);
    newsym(gold->ox, gold->oy);
    nx = rooms[vroom].lx + rn2(2);
    ny = rooms[vroom].ly + rn2(2);
    place_object(gold, nx, ny);
    stackobj(gold);
    newsym(nx, ny);
}

STATIC_OVL void
wallify_vault(grd)
struct monst *grd;
{
    int x, y, typ;
    int vlt = EGD(grd)->vroom;
    char tmp_viz;
    xchar lox = rooms[vlt].lx - 1, hix = rooms[vlt].hx + 1,
          loy = rooms[vlt].ly - 1, hiy = rooms[vlt].hy + 1;
    struct monst *mon;
    struct obj *gold;
    struct trap *trap;
    boolean fixed = FALSE;
    boolean movedgold = FALSE;

    for (x = lox; x <= hix; x++)
        for (y = loy; y <= hiy; y++) {
            /* if not on the room boundary, skip ahead */
            if (x != lox && x != hix && y != loy && y != hiy)
                continue;

            if (!IS_WALL(levl[x][y].typ) && !in_fcorridor(grd, x, y)) {
                if ((mon = m_at(x, y)) != 0 && mon != grd) {
                    if (mon->mtame)
                        yelp(mon);
                    (void) rloc(mon, FALSE);
                }
                if ((gold = g_at(x, y)) != 0) {
                    move_gold(gold, EGD(grd)->vroom);
                    movedgold = TRUE;
                }
                if ((trap = t_at(x, y)) != 0)
                    deltrap(trap);
                if (x == lox)
                    typ =
                        (y == loy) ? TLCORNER : (y == hiy) ? BLCORNER : VWALL;
                else if (x == hix)
                    typ =
                        (y == loy) ? TRCORNER : (y == hiy) ? BRCORNER : VWALL;
                else /* not left or right side, must be top or bottom */
                    typ = HWALL;
                levl[x][y].typ = typ;
                levl[x][y].doormask = 0;
                /*
                 * hack: player knows walls are restored because of the
                 * message, below, so show this on the screen.
                 */
                tmp_viz = viz_array[y][x];
                viz_array[y][x] = IN_SIGHT | COULD_SEE;
                newsym(x, y);
                viz_array[y][x] = tmp_viz;
                block_point(x, y);
                fixed = TRUE;
            }
        }

    if (movedgold || fixed) {
        if (in_fcorridor(grd, grd->mx, grd->my) || cansee(grd->mx, grd->my))
            pline("%s 低声念了一句咒语.", noit_Monnam(grd));
        else
            You_hear("远处的咏唱声.");
        if (movedgold)
            pline("一种神秘的力量把金币移进了金库.");
        if (fixed)
            pline_The("被破坏的金库的墙壁魔法般的恢复了!");
    }
}

/*
 * return  1: guard moved,  0: guard didn't,  -1: let m_move do it,  -2: died
 */
int
gd_move(grd)
register struct monst *grd;
{
    int x, y, nx, ny, m, n;
    int dx, dy, gx, gy, fci;
    uchar typ;
    struct fakecorridor *fcp;
    register struct egd *egrd = EGD(grd);
    struct rm *crm;
    boolean goldincorridor = FALSE,
            u_in_vault = vault_occupied(u.urooms) ? TRUE : FALSE,
            grd_in_vault = *in_rooms(grd->mx, grd->my, VAULT) ? TRUE : FALSE;
    boolean disappear_msg_seen = FALSE, semi_dead = (grd->mhp <= 0);
    long umoney = money_cnt(invent);
    register boolean u_carry_gold = ((umoney + hidden_gold()) > 0L);
    boolean see_guard, newspot = FALSE;

    if (!on_level(&(egrd->gdlevel), &u.uz))
        return -1;
    nx = ny = m = n = 0;
    if (!u_in_vault && !grd_in_vault)
        wallify_vault(grd);
    if (!grd->mpeaceful) {
        if (semi_dead) {
            egrd->gddone = 1;
            goto newpos;
        }
        if (!u_in_vault
            && (grd_in_vault || (in_fcorridor(grd, grd->mx, grd->my)
                                 && !in_fcorridor(grd, u.ux, u.uy)))) {
            (void) rloc(grd, FALSE);
            wallify_vault(grd);
            (void) clear_fcorr(grd, TRUE);
            goto letknow;
        }
        if (!in_fcorridor(grd, grd->mx, grd->my))
            (void) clear_fcorr(grd, TRUE);
        return -1;
    }
    if (abs(egrd->ogx - grd->mx) > 1 || abs(egrd->ogy - grd->my) > 1)
        return -1; /* teleported guard - treat as monster */

    if (egrd->witness) {
        verbalize("你竟敢%s金币, 恶棍!",
                  (egrd->witness & GD_EATGOLD) ? "吃掉" : "毁坏");
        egrd->witness = 0;
        grd->mpeaceful = 0;
        return -1;
    }
    if (egrd->fcend == 1) {
        if (u_in_vault && (u_carry_gold || um_dist(grd->mx, grd->my, 1))) {
            if (egrd->warncnt == 3 && !Deaf)
                verbalize("我重复一遍, %s跟我来!",
                          u_carry_gold
                              ? (!umoney ? "扔掉那个隐藏的金币并"
                                         : "扔掉那个金币并")
                              : "");
            if (egrd->warncnt == 7) {
                m = grd->mx;
                n = grd->my;
                if (!Deaf)
                    verbalize("已经警告过你了, 无赖!");
                mnexto(grd);
                levl[m][n].typ = egrd->fakecorr[0].ftyp;
                newsym(m, n);
                grd->mpeaceful = 0;
                return -1;
            }
            /* not fair to get mad when (s)he's fainted or paralyzed */
            if (!is_fainted() && multi >= 0)
                egrd->warncnt++;
            return 0;
        }

        if (!u_in_vault) {
            if (u_carry_gold) { /* player teleported */
                m = grd->mx;
                n = grd->my;
                (void) rloc(grd, TRUE);
                levl[m][n].typ = egrd->fakecorr[0].ftyp;
                newsym(m, n);
                grd->mpeaceful = 0;
            letknow:
                if (!cansee(grd->mx, grd->my) || !mon_visible(grd))
                    You_hear("警卫尖锐的哨声.");
                else
                    You(um_dist(grd->mx, grd->my, 2)
                            ? "看见%s接近."
                            : "面对着%s.",
                        /* "an angry guard" */
                        x_monnam(grd, ARTICLE_A, "生气的", 0, FALSE));
                return -1;
            } else {
                if (!Deaf)
                    verbalize("好吧, 去吧.");
                wallify_vault(grd);
                egrd->gddone = 1;
                goto cleanup;
            }
        }
    }

    if (egrd->fcend > 1) {
        if (egrd->fcend > 2 && in_fcorridor(grd, grd->mx, grd->my)
            && !egrd->gddone && !in_fcorridor(grd, u.ux, u.uy)
            && levl[egrd->fakecorr[0].fx][egrd->fakecorr[0].fy].typ
                   == egrd->fakecorr[0].ftyp) {
            pline("%s, 混乱的, 消失了.", noit_Monnam(grd));
            disappear_msg_seen = TRUE;
            goto cleanup;
        }
        if (u_carry_gold && (in_fcorridor(grd, u.ux, u.uy)
                             /* cover a 'blind' spot */
                             || (egrd->fcend > 1 && u_in_vault))) {
            if (!grd->mx) {
                restfakecorr(grd);
                return -2;
            }
            if (egrd->warncnt < 6) {
                egrd->warncnt = 6;
                if (!Deaf)
                    verbalize("扔掉你所有的金币, 恶棍!");
                return 0;
            } else {
                if (!Deaf)
                    verbalize("就这样, 无赖!");
                grd->mpeaceful = 0;
                return -1;
            }
        }
    }
    for (fci = egrd->fcbeg; fci < egrd->fcend; fci++)
        if (g_at(egrd->fakecorr[fci].fx, egrd->fakecorr[fci].fy)) {
            m = egrd->fakecorr[fci].fx;
            n = egrd->fakecorr[fci].fy;
            goldincorridor = TRUE;
        }
    if (goldincorridor && !egrd->gddone) {
        x = grd->mx;
        y = grd->my;
        if (m == u.ux && n == u.uy) {
            struct obj *gold = g_at(m, n);
            /* Grab the gold from between the hero's feet.  */
            obj_extract_self(gold);
            add_to_minv(grd, gold);
            newsym(m, n);
        } else if (m == x && n == y) {
            mpickgold(grd); /* does a newsym */
        } else {
            /* just for insurance... */
            if (MON_AT(m, n) && m != grd->mx && n != grd->my) {
                if (!Deaf)
                    verbalize("别挡道, 浮渣!");
                (void) rloc(m_at(m, n), FALSE);
            }
            remove_monster(grd->mx, grd->my);
            newsym(grd->mx, grd->my);
            place_monster(grd, m, n);
            mpickgold(grd); /* does a newsym */
        }
        if (cansee(m, n))
            pline("%s%s 捡起了金币.", Monnam(grd),
                  grd->mpeaceful ? "平静下来" : "");
        if (x != grd->mx || y != grd->my) {
            remove_monster(grd->mx, grd->my);
            newsym(grd->mx, grd->my);
            place_monster(grd, x, y);
            newsym(x, y);
        }
        if (!grd->mpeaceful)
            return -1;
        egrd->warncnt = 5;
        return 0;
    }
    if (um_dist(grd->mx, grd->my, 1) || egrd->gddone) {
        if (!egrd->gddone && !rn2(10) && !Deaf && !u.uswallow
            && !(u.ustuck && !sticks(youmonst.data)))
            verbalize("往前走!");
        restfakecorr(grd);
        return 0; /* didn't move */
    }
    x = grd->mx;
    y = grd->my;

    if (u_in_vault)
        goto nextpos;

    /* look around (hor & vert only) for accessible places */
    for (nx = x - 1; nx <= x + 1; nx++)
        for (ny = y - 1; ny <= y + 1; ny++) {
            if ((nx == x || ny == y) && (nx != x || ny != y)
                && isok(nx, ny)) {
                typ = (crm = &levl[nx][ny])->typ;
                if (!IS_STWALL(typ) && !IS_POOL(typ)) {
                    if (in_fcorridor(grd, nx, ny))
                        goto nextnxy;

                    if (*in_rooms(nx, ny, VAULT))
                        continue;

                    /* seems we found a good place to leave him alone */
                    egrd->gddone = 1;
                    if (ACCESSIBLE(typ))
                        goto newpos;
#ifdef STUPID
                    if (typ == SCORR)
                        crm->typ = CORR;
                    else
                        crm->typ = DOOR;
#else
                    crm->typ = (typ == SCORR) ? CORR : DOOR;
#endif
                    if (crm->typ == DOOR)
                        crm->doormask = D_NODOOR;
                    goto proceed;
                }
            }
        nextnxy:
            ;
        }
nextpos:
    nx = x;
    ny = y;
    gx = egrd->gdx;
    gy = egrd->gdy;
    dx = (gx > x) ? 1 : (gx < x) ? -1 : 0;
    dy = (gy > y) ? 1 : (gy < y) ? -1 : 0;
    if (abs(gx - x) >= abs(gy - y))
        nx += dx;
    else
        ny += dy;

    while ((typ = (crm = &levl[nx][ny])->typ) != 0) {
        /* in view of the above we must have IS_WALL(typ) or typ == POOL */
        /* must be a wall here */
        if (isok(nx + nx - x, ny + ny - y) && !IS_POOL(typ)
            && IS_ROOM(levl[nx + nx - x][ny + ny - y].typ)) {
            crm->typ = DOOR;
            crm->doormask = D_NODOOR;
            goto proceed;
        }
        if (dy && nx != x) {
            nx = x;
            ny = y + dy;
            continue;
        }
        if (dx && ny != y) {
            ny = y;
            nx = x + dx;
            dy = 0;
            continue;
        }
        /* I don't like this, but ... */
        if (IS_ROOM(typ)) {
            crm->typ = DOOR;
            crm->doormask = D_NODOOR;
            goto proceed;
        }
        break;
    }
    crm->typ = CORR;
proceed:
    newspot = TRUE;
    unblock_point(nx, ny); /* doesn't block light */
    if (cansee(nx, ny))
        newsym(nx, ny);

    fcp = &(egrd->fakecorr[egrd->fcend]);
    if (egrd->fcend++ == FCSIZ)
        panic("fakecorr overflow");
    fcp->fx = nx;
    fcp->fy = ny;
    fcp->ftyp = typ;
newpos:
    if (egrd->gddone) {
        /* The following is a kludge.  We need to keep    */
        /* the guard around in order to be able to make   */
        /* the fake corridor disappear as the player      */
        /* moves out of it, but we also need the guard    */
        /* out of the way.  We send the guard to never-   */
        /* never land.  We set ogx ogy to mx my in order  */
        /* to avoid a check at the top of this function.  */
        /* At the end of the process, the guard is killed */
        /* in restfakecorr().                             */
    cleanup:
        x = grd->mx;
        y = grd->my;

        see_guard = canspotmon(grd);
        wallify_vault(grd);
        remove_monster(grd->mx, grd->my);
        newsym(grd->mx, grd->my);
        place_monster(grd, 0, 0);
        egrd->ogx = grd->mx;
        egrd->ogy = grd->my;
        restfakecorr(grd);
        if (!semi_dead && (in_fcorridor(grd, u.ux, u.uy) || cansee(x, y))) {
            if (!disappear_msg_seen && see_guard)
                pline("突然, %s消失了.", noit_mon_nam(grd));
            return 1;
        }
        return -2;
    }
    egrd->ogx = grd->mx; /* update old positions */
    egrd->ogy = grd->my;
    remove_monster(grd->mx, grd->my);
    place_monster(grd, nx, ny);
    if (newspot && g_at(nx, ny)) {
        /* if there's gold already here (most likely from mineralize()),
           pick it up now so that guard doesn't later think hero dropped
           it and give an inappropriate message */
        mpickgold(grd);
        if (canspotmon(grd))
            pline("%s 捡起了一些金币.", Monnam(grd));
    } else
        newsym(grd->mx, grd->my);
    restfakecorr(grd);
    return 1;
}

/* Routine when dying or quitting with a vault guard around */
void
paygd()
{
    register struct monst *grd = findgd();
    long umoney = money_cnt(invent);
    struct obj *coins, *nextcoins;
    int gx, gy;
    char buf[BUFSZ];

    if (!umoney || !grd)
        return;

    if (u.uinvault) {
        Your("%ld %s 进入了魔力的记忆金库.", umoney,
             currency(umoney));
        gx = u.ux;
        gy = u.uy;
    } else {
        if (grd->mpeaceful) { /* guard has no "right" to your gold */
            mongone(grd);
            return;
        }
        mnexto(grd);
        pline("%s 把你的金币传送回了金库.", Monnam(grd));
        gx = rooms[EGD(grd)->vroom].lx + rn2(2);
        gy = rooms[EGD(grd)->vroom].ly + rn2(2);
        Sprintf(buf, "致克罗伊斯: 这里是从%s %s获得的金币.",
                plname, mons[u.umonster].mname);
        make_grave(gx, gy, buf);
    }
    for (coins = invent; coins; coins = nextcoins) {
        nextcoins = coins->nobj;
        if (objects[coins->otyp].oc_class == COIN_CLASS) {
            freeinv(coins);
            place_object(coins, gx, gy);
            stackobj(coins);
        }
    }
    mongone(grd);
}

long
hidden_gold()
{
    long value = 0L;
    struct obj *obj;

    for (obj = invent; obj; obj = obj->nobj)
        if (Has_contents(obj))
            value += contained_gold(obj);
    /* unknown gold stuck inside statues may cause some consternation... */

    return value;
}

/* prevent "You hear footsteps.." when inappropriate */
boolean
gd_sound()
{
    struct monst *grd = findgd();

    if (vault_occupied(u.urooms))
        return FALSE;
    else
        return (boolean) (grd == (struct monst *) 0);
}

void
vault_gd_watching(activity)
unsigned int activity;
{
    struct monst *guard = findgd();

    if (guard && guard->mcansee && m_canseeu(guard)) {
        if (activity == GD_EATGOLD || activity == GD_DESTROYGOLD)
            EGD(guard)->witness = activity;
    }
}

/*vault.c*/
