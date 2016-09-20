/* NetHack 3.6	sit.c	$NHDT-Date: 1445906863 2015/10/27 00:47:43 $  $NHDT-Branch: master $:$NHDT-Revision: 1.51 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"


/* take away the hero's money */
void
take_gold()
{
    struct obj *otmp, *nobj;
    int lost_money = 0;

    for (otmp = invent; otmp; otmp = nobj) {
        nobj = otmp->nobj;
        if (otmp->oclass == COIN_CLASS) {
            lost_money = 1;
            remove_worn_item(otmp, FALSE);
            delobj(otmp);
        }
    }
    if (!lost_money) {
        You("有一种奇怪的感觉.");
    } else {
        You("注意到你没有钱了!");
        context.botl = 1;
    }
}

/* #sit command */
int
dosit()
{
    static const char sit_message[] = "坐在 %s上.";
    register struct trap *trap = t_at(u.ux, u.uy);
    register int typ = levl[u.ux][u.uy].typ;

    if (u.usteed) {
        You("已经坐在%s 上.", mon_nam(u.usteed));
        return 0;
    }
    if (u.uundetected && is_hider(youmonst.data) && u.umonnum != PM_TRAPPER)
        u.uundetected = 0; /* no longer on the ceiling */

    if (!can_reach_floor(FALSE)) {
        if (u.uswallow)
            There("没有座位!");
        else if (Levitation)
            You("就地跌倒.");
        else
            You("坐在空中.");
        return 0;
    } else if (u.ustuck && !sticks(youmonst.data)) {
        /* holding monster is next to hero rather than beneath, but
           hero is in no condition to actually sit at has/her own spot */
        if (humanoid(u.ustuck->data))
            pline("%s 不再提供%s 膝部.", Monnam(u.ustuck), mhis(u.ustuck));
        else
            pline("%s 没有膝部.", Monnam(u.ustuck));
        return 0;
    } else if (is_pool(u.ux, u.uy) && !Underwater) { /* water walking */
        goto in_water;
    }

    if (OBJ_AT(u.ux, u.uy)
        /* ensure we're not standing on the precipice */
        && !uteetering_at_seen_pit(trap)) {
        register struct obj *obj;

        obj = level.objects[u.ux][u.uy];
        if (youmonst.data->mlet == S_DRAGON && obj->oclass == COIN_CLASS) {
            You("盘腿绕着你的%s钱财而坐.",
                (obj->quan + money_cnt(invent) < u.ulevel * 1000) ? "微薄的 "
                                                                  : "");
        } else {
            You("坐在%s 上.", the(xname(obj)));
            if (!(Is_box(obj) || objects[obj->otyp].oc_material == CLOTH))
                pline("那很不舒服...");
        }
    } else if (trap != 0 || (u.utrap && (u.utraptype >= TT_LAVA))) {
        if (u.utrap) {
            exercise(A_WIS, FALSE); /* you're getting stuck longer */
            if (u.utraptype == TT_BEARTRAP) {
                You_cant("在你的%s陷在捕兽夹时坐下来.",
                         body_part(FOOT));
                u.utrap++;
            } else if (u.utraptype == TT_PIT) {
                if (trap && trap->ttyp == SPIKED_PIT) {
                    You("坐在钉子上.  哎哟!");
                    losehp(Half_physical_damage ? rn2(2) : 1,
                           "坐在铁钉上", KILLED_BY);
                    exercise(A_STR, FALSE);
                } else
                    You("坐在坑里.");
                u.utrap += rn2(5);
            } else if (u.utraptype == TT_WEB) {
                You("坐在蜘蛛网里并更加被缠住了!");
                u.utrap += rn1(10, 5);
            } else if (u.utraptype == TT_LAVA) {
                /* Must have fire resistance or they'd be dead already */
                You("坐在熔岩里!");
                if (Slimed)
                    burn_away_slime();
                u.utrap += rnd(4);
                losehp(d(2, 10), "坐在熔岩上",
                       KILLED_BY); /* lava damage */
            } else if (u.utraptype == TT_INFLOOR
                       || u.utraptype == TT_BURIEDBALL) {
                You_cant("挪动来坐下!");
                u.utrap++;
            }
        } else {
            You("坐下了.");
            dotrap(trap, 0);
        }
    } else if (Underwater || Is_waterlevel(&u.uz)) {
        if (Is_waterlevel(&u.uz))
            There("附近没有坐垫漂浮着.");
        else
            You("坐在泥泞的底部.");
    } else if (is_pool(u.ux, u.uy)) {
    in_water:
        You("坐在水里.");
        if (!rn2(10) && uarm)
            (void) water_damage(uarm, "盔甲", TRUE);
        if (!rn2(10) && uarmf && uarmf->otyp != WATER_WALKING_BOOTS)
            (void) water_damage(uarm, "盔甲", TRUE);
    } else if (IS_SINK(typ)) {
        You(sit_message, defsyms[S_sink].explanation);
        Your("%s 打湿了.", humanoid(youmonst.data) ? "臀部" : "下面");
    } else if (IS_ALTAR(typ)) {
        You(sit_message, defsyms[S_altar].explanation);
        altar_wrath(u.ux, u.uy);
    } else if (IS_GRAVE(typ)) {
        You(sit_message, defsyms[S_grave].explanation);
    } else if (typ == STAIRS) {
        You(sit_message, "楼梯");
    } else if (typ == LADDER) {
        You(sit_message, "梯子");
    } else if (is_lava(u.ux, u.uy)) {
        /* must be WWalking */
        You(sit_message, "熔岩");
        burn_away_slime();
        if (likes_lava(youmonst.data)) {
            pline_The("熔岩感觉温暖.");
            return 1;
        }
        pline_The("熔岩烧伤了你!");
        losehp(d((Fire_resistance ? 2 : 10), 10), /* lava damage */
               "坐在熔岩上", KILLED_BY);
    } else if (is_ice(u.ux, u.uy)) {
        You(sit_message, defsyms[S_ice].explanation);
        if (!Cold_resistance)
            pline_The("冰感觉冷.");
    } else if (typ == DRAWBRIDGE_DOWN) {
        You(sit_message, "吊桥");
    } else if (IS_THRONE(typ)) {
        You(sit_message, defsyms[S_throne].explanation);
        if (rnd(6) > 4) {
            switch (rnd(13)) {
            case 1:
                (void) adjattrib(rn2(A_MAX), -rn1(4, 3), FALSE);
                losehp(rnd(10), "被诅咒的王座", KILLED_BY_AN);
                break;
            case 2:
                (void) adjattrib(rn2(A_MAX), 1, FALSE);
                break;
            case 3:
                pline("%s电冲击穿透了你的身体!",
                      (Shock_resistance) ? "" : "大量的");
                losehp(Shock_resistance ? rnd(6) : rnd(30), "电椅",
                       KILLED_BY_AN);
                exercise(A_CON, FALSE);
                break;
            case 4:
                You_feel("非常, 好多了!");
                if (Upolyd) {
                    if (u.mh >= (u.mhmax - 5))
                        u.mhmax += 4;
                    u.mh = u.mhmax;
                }
                if (u.uhp >= (u.uhpmax - 5))
                    u.uhpmax += 4;
                u.uhp = u.uhpmax;
                make_blinded(0L, TRUE);
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
                heal_legs();
                context.botl = 1;
                break;
            case 5:
                take_gold();
                break;
            case 6:
                if (u.uluck + rn2(5) < 0) {
                    You_feel("你的运气在变化.");
                    change_luck(1);
                } else
                    makewish();
                break;
            case 7:
              {
                int cnt = rnd(10);

                /* Magical voice not affected by deafness */
                pline("一个声音回响:");
                verbalize("汝之观者被召之, %s!",
                          flags.female ? "夫人" : "阁下");
                while (cnt--)
                    (void) makemon(courtmon(), u.ux, u.uy, NO_MM_FLAGS);
                break;
              }
            case 8:
                /* Magical voice not affected by deafness */
                pline("一个声音回响:");
                verbalize("汝之专命, %s...",
                          flags.female ? "夫人" : "阁下");
                do_genocide(5); /* REALLY|ONTHRONE, see do_genocide() */
                break;
            case 9:
                /* Magical voice not affected by deafness */
                pline("一个声音回响:");
                verbalize(
                 "汝坐于此最神圣王座上之诅咒!");
                if (Luck > 0) {
                    make_blinded(Blinded + rn1(100, 250), TRUE);
                } else
                    rndcurse();
                break;
            case 10:
                if (Luck < 0 || (HSee_invisible & INTRINSIC)) {
                    if (level.flags.nommap) {
                        pline("一种可怕的嗡嗡声充斥你的大脑!");
                        make_confused((HConfusion & TIMEOUT) + (long) rnd(30),
                                      FALSE);
                    } else {
                        pline("一个影像在你的心中形成.");
                        do_mapping();
                    }
                } else {
                    Your("视觉变得清晰了.");
                    HSee_invisible |= FROMOUTSIDE;
                    newsym(u.ux, u.uy);
                }
                break;
            case 11:
                if (Luck < 0) {
                    You_feel("受到威胁的.");
                    aggravate();
                } else {
                    You_feel("到一种痛苦的感觉.");
                    tele(); /* teleport him */
                }
                break;
            case 12:
                You("被准许一次洞察!");
                if (invent) {
                    /* rn2(5) agrees w/seffects() */
                    identify_pack(rn2(5), FALSE);
                }
                break;
            case 13:
                Your("内心变成了一块椒盐卷饼!");
                make_confused((HConfusion & TIMEOUT) + (long) rn1(7, 16),
                              FALSE);
                break;
            default:
                impossible("throne effect");
                break;
            }
        } else {
            if (is_prince(youmonst.data))
                You_feel("这里非常舒服.");
            else
                You_feel("不知怎么不协调...");
        }

        if (!rn2(3) && IS_THRONE(levl[u.ux][u.uy].typ)) {
            /* may have teleported */
            levl[u.ux][u.uy].typ = ROOM;
            pline_The("王座在一股逻辑中消失了.");
            newsym(u.ux, u.uy);
        }
    } else if (lays_eggs(youmonst.data)) {
        struct obj *uegg;

        if (!flags.female) {
            pline("%s 不能下蛋!",
                  Hallucination
                      ? "你可能认为你是一个鸭嘴兽, 但雄性仍然"
                      : "雄性");
            return 0;
        } else if (u.uhunger < (int) objects[EGG].oc_nutrition) {
            You("没有足够的精力来下蛋.");
            return 0;
        }

        uegg = mksobj(EGG, FALSE, FALSE);
        uegg->spe = 1;
        uegg->quan = 1L;
        uegg->owt = weight(uegg);
        /* this sets hatch timers if appropriate */
        set_corpsenm(uegg, egg_type_from_parent(u.umonnum, FALSE));
        uegg->known = uegg->dknown = 1;
        You("下了一个蛋.");
        dropy(uegg);
        stackobj(uegg);
        morehungry((int) objects[EGG].oc_nutrition);
    } else {
        pline("坐在 %s上可没意思.", surface(u.ux, u.uy));
    }
    return 1;
}

/* curse a few inventory items at random! */
void
rndcurse()
{
    int nobj = 0;
    int cnt, onum;
    struct obj *otmp;
    static const char mal_aura[] = "感觉到一个恶性的光环围绕在%s的四周.";

    if (uwep && (uwep->oartifact == ART_MAGICBANE) && rn2(20)) {
        You(mal_aura, "魔法吸收剑");
        return;
    }

    if (Antimagic) {
        shieldeff(u.ux, u.uy);
        You(mal_aura, "你");
    }

    for (otmp = invent; otmp; otmp = otmp->nobj) {
        /* gold isn't subject to being cursed or blessed */
        if (otmp->oclass == COIN_CLASS)
            continue;
        nobj++;
    }
    if (nobj) {
        for (cnt = rnd(6 / ((!!Antimagic) + (!!Half_spell_damage) + 1));
             cnt > 0; cnt--) {
            onum = rnd(nobj);
            for (otmp = invent; otmp; otmp = otmp->nobj) {
                /* as above */
                if (otmp->oclass == COIN_CLASS)
                    continue;
                if (--onum == 0)
                    break; /* found the target */
            }
            /* the !otmp case should never happen; picking an already
               cursed item happens--avoid "resists" message in that case */
            if (!otmp || otmp->cursed)
                continue; /* next target */

            if (otmp->oartifact && spec_ability(otmp, SPFX_INTEL)
                && rn2(10) < 8) {
                pline("%s!", Tobjnam(otmp, "抵抗"));
                continue;
            }

            if (otmp->blessed)
                unbless(otmp);
            else
                curse(otmp);
        }
        update_inventory();
    }

    /* treat steed's saddle as extended part of hero's inventory */
    if (u.usteed && !rn2(4) && (otmp = which_armor(u.usteed, W_SADDLE)) != 0
        && !otmp->cursed) { /* skip if already cursed */
        if (otmp->blessed)
            unbless(otmp);
        else
            curse(otmp);
        if (!Blind) {
            pline("%s %s光芒.", Yobjnam2(otmp, "发出"),
                  hcolor(otmp->cursed ? NH_BLACK : (const char *) "棕色的"));
            otmp->bknown = TRUE;
        }
    }
}

/* remove a random INTRINSIC ability */
void
attrcurse()
{
    switch (rnd(11)) {
    case 1:
        if (HFire_resistance & INTRINSIC) {
            HFire_resistance &= ~INTRINSIC;
            You_feel("更温暖了.");
            break;
        }
    case 2:
        if (HTeleportation & INTRINSIC) {
            HTeleportation &= ~INTRINSIC;
            You_feel("不那么跳跃的.");
            break;
        }
    case 3:
        if (HPoison_resistance & INTRINSIC) {
            HPoison_resistance &= ~INTRINSIC;
            You_feel("有一点生病的!");
            break;
        }
    case 4:
        if (HTelepat & INTRINSIC) {
            HTelepat &= ~INTRINSIC;
            if (Blind && !Blind_telepat)
                see_monsters(); /* Can't sense mons anymore! */
            Your("感官失效了!");
            break;
        }
    case 5:
        if (HCold_resistance & INTRINSIC) {
            HCold_resistance &= ~INTRINSIC;
            You_feel("更凉爽了.");
            break;
        }
    case 6:
        if (HInvis & INTRINSIC) {
            HInvis &= ~INTRINSIC;
            You_feel("妄想的.");
            break;
        }
    case 7:
        if (HSee_invisible & INTRINSIC) {
            HSee_invisible &= ~INTRINSIC;
            You("%s!", Hallucination ? "tawt you taw a puttie tat"
                                     : "认为你看见了什么东西");
            break;
        }
    case 8:
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            You_feel("更慢了.");
            break;
        }
    case 9:
        if (HStealth & INTRINSIC) {
            HStealth &= ~INTRINSIC;
            You_feel("笨拙的.");
            break;
        }
    case 10:
        /* intrinsic protection is just disabled, not set back to 0 */
        if (HProtection & INTRINSIC) {
            HProtection &= ~INTRINSIC;
            You_feel("易受伤害的.");
            break;
        }
    case 11:
        if (HAggravate_monster & INTRINSIC) {
            HAggravate_monster &= ~INTRINSIC;
            You_feel("不那么引人注目的.");
            break;
        }
    default:
        break;
    }
}

/*sit.c*/
