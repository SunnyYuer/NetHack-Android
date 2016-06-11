/* NetHack 3.6	sounds.c	$NHDT-Date: 1446713641 2015/11/05 08:54:01 $  $NHDT-Branch: master $:$NHDT-Revision: 1.74 $ */
/*      Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_DCL boolean FDECL(mon_is_gecko, (struct monst *));
STATIC_DCL int FDECL(domonnoise, (struct monst *));
STATIC_DCL int NDECL(dochat);
STATIC_DCL int FDECL(mon_in_room, (struct monst *, int));

/* this easily could be a macro, but it might overtax dumb compilers */
STATIC_OVL int
mon_in_room(mon, rmtyp)
struct monst *mon;
int rmtyp;
{
    int rno = levl[mon->mx][mon->my].roomno;
    if (rno >= ROOMOFFSET)
        return rooms[rno - ROOMOFFSET].rtype == rmtyp;
    return FALSE;
}

void
dosounds()
{
    register struct mkroom *sroom;
    register int hallu, vx, vy;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
    int xx;
#endif
    struct monst *mtmp;

    if (Deaf || !flags.acoustics || u.uswallow || Underwater)
        return;

    hallu = Hallucination ? 1 : 0;

    if (level.flags.nfountains && !rn2(400)) {
        static const char *const fountain_msg[4] = {
            "水中的冒泡声.", "水落在金币上的声音.",
            "水中仙女的溅水声.", "冷饮柜的声音!",
        };
        You_hear1(fountain_msg[rn2(3) + hallu]);
    }
    if (level.flags.nsinks && !rn2(300)) {
        static const char *const sink_msg[3] = {
            "缓缓滴水的声音.", "潺潺的水声.", "洗餐具的声音!",
        };
        You_hear1(sink_msg[rn2(2) + hallu]);
    }
    if (level.flags.has_court && !rn2(200)) {
        static const char *const throne_msg[4] = {
            "宫廷腔调的谈话声.",
            "在审判中权杖的重击声.",
            "有人叫道 \" 砍掉%s头!\"", "贝露庭尔王后的猫!",
        };
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->msleeping || is_lord(mtmp->data)
                 || is_prince(mtmp->data)) && !is_animal(mtmp->data)
                && mon_in_room(mtmp, COURT)) {
                /* finding one is enough, at least for now */
                int which = rn2(3) + hallu;

                if (which != 2)
                    You_hear1(throne_msg[which]);
                else
                    pline(throne_msg[2], uhis());
                return;
            }
        }
    }
    if (level.flags.has_swamp && !rn2(200)) {
        static const char *const swamp_msg[3] = {
            "听见蚊子的声音!", "闻到沼气!", /* so it's a smell...*/
            "听见唐老鸭!",
        };
        You1(swamp_msg[rn2(2) + hallu]);
        return;
    }
    if (level.flags.has_vault && !rn2(200)) {
        if (!(sroom = search_special(VAULT))) {
            /* strange ... */
            level.flags.has_vault = 0;
            return;
        }
        if (gd_sound())
            switch (rn2(2) + hallu) {
            case 1: {
                boolean gold_in_vault = FALSE;

                for (vx = sroom->lx; vx <= sroom->hx; vx++)
                    for (vy = sroom->ly; vy <= sroom->hy; vy++)
                        if (g_at(vx, vy))
                            gold_in_vault = TRUE;
#if defined(AMIGA) && defined(AZTEC_C_WORKAROUND)
                /* Bug in aztec assembler here. Workaround below */
                xx = ROOM_INDEX(sroom) + ROOMOFFSET;
                xx = (xx != vault_occupied(u.urooms));
                if (xx)
#else
                if (vault_occupied(u.urooms)
                    != (ROOM_INDEX(sroom) + ROOMOFFSET))
#endif /* AZTEC_C_WORKAROUND */
                {
                    if (gold_in_vault)
                        You_hear(!hallu
                                     ? "某人在数钱."
                                     : "四分卫在提场喊话.");
                    else
                        You_hear("某人在搜索.");
                    break;
                }
                /* fall into... (yes, even for hallucination) */
            }
            case 0:
                You_hear("巡逻警卫的脚步声.");
                break;
            case 2:
                You_hear("埃比尼泽斯克鲁奇!");
                break;
            }
        return;
    }
    if (level.flags.has_beehive && !rn2(200)) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data))
                && mon_in_room(mtmp, BEEHIVE)) {
                switch (rn2(2) + hallu) {
                case 0:
                    You_hear("低沉的嗡嗡声.");
                    break;
                case 1:
                    You_hear("愤怒的嗡嗡声.");
                    break;
                case 2:
                    You_hear("蜜蜂在你的%s软帽里!",
                             uarmh ? "" : "( 不存在的) ");
                    break;
                }
                return;
            }
        }
    }
    if (level.flags.has_morgue && !rn2(200)) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((is_undead(mtmp->data) || is_vampshifter(mtmp))
                && mon_in_room(mtmp, MORGUE)) {
                const char *hair = body_part(HAIR); /* hair/fur/scales */

                switch (rn2(2) + hallu) {
                case 0:
                    You("突然意识到这不自然的宁静.");
                    break;
                case 1:
                    pline_The("你的%s后面的%s%s起来了.",
                              body_part(NECK), hair, vtense(hair, "竖"));
                    break;
                case 2:
                    pline_The("你%s上的%s%s要竖起来了.",
                              body_part(HEAD), hair, vtense(hair, "似乎"));
                    break;
                }
                return;
            }
        }
    }
    if (level.flags.has_barracks && !rn2(200)) {
        static const char *const barracks_msg[4] = {
            "磨剑的声音.", "响亮的鼾声.", "掷骰子的声音.",
            "麦克阿瑟将军!",
        };
        int count = 0;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (is_mercenary(mtmp->data)
#if 0 /* don't bother excluding these */
                && !strstri(mtmp->data->mname, "watch")
                && !strstri(mtmp->data->mname, "guard")
#endif
                && mon_in_room(mtmp, BARRACKS)
                /* sleeping implies not-yet-disturbed (usually) */
                && (mtmp->msleeping || ++count > 5)) {
                You_hear1(barracks_msg[rn2(3) + hallu]);
                return;
            }
        }
    }
    if (level.flags.has_zoo && !rn2(200)) {
        static const char *const zoo_msg[3] = {
            "一只大象踩着花生的怀旧的声音.",
            "海豹在叫的怀旧的声音.", "怪医杜立德!",
        };
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if ((mtmp->msleeping || is_animal(mtmp->data))
                && mon_in_room(mtmp, ZOO)) {
                You_hear1(zoo_msg[rn2(2) + hallu]);
                return;
            }
        }
    }
    if (level.flags.has_shop && !rn2(200)) {
        if (!(sroom = search_special(ANY_SHOP))) {
            /* strange... */
            level.flags.has_shop = 0;
            return;
        }
        if (tended_shop(sroom)
            && !index(u.ushops, (int) (ROOM_INDEX(sroom) + ROOMOFFSET))) {
            static const char *const shop_msg[3] = {
                "有人在咒骂商店扒手.",
                "收银机的鸣响.", "内曼和马库斯争吵!",
            };
            You_hear1(shop_msg[rn2(2) + hallu]);
        }
        return;
    }
    if (level.flags.has_temple && !rn2(200)
        && !(Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->ispriest && inhistemple(mtmp)
                /* priest must be active */
                && mtmp->mcanmove && !mtmp->msleeping
                /* hero must be outside this temple */
                && temple_occupied(u.urooms) != EPRI(mtmp)->shroom)
                break;
        }
        if (mtmp) {
            /* Generic temple messages; no attempt to match topic or tone
               to the pantheon involved, let alone to the specific deity.
               These are assumed to be coming from the attending priest;
               asterisk means that the priest must be capable of speech;
               pound sign (octathorpe,&c--don't go there) means that the
               priest and the altar must not be directly visible (we don't
               care if telepathy or extended detection reveals that the
               priest is not currently standing on the altar; he's mobile). */
            static const char *const temple_msg[] = {
                "*有人在歌颂%s.", "*有人在祈求%s.",
                "#一具动物尸体在被献祭.",
                "*尖锐的呼吁捐款声",
            };
            const char *msg;
            int trycount = 0, ax = EPRI(mtmp)->shrpos.x,
                ay = EPRI(mtmp)->shrpos.y;
            boolean speechless = (mtmp->data->msound <= MS_ANIMAL),
                    in_sight = canseemon(mtmp) || cansee(ax, ay);

            do {
                msg = temple_msg[rn2(SIZE(temple_msg) - 1 + hallu)];
                if (index(msg, '*') && speechless)
                    continue;
                if (index(msg, '#') && in_sight)
                    continue;
                break; /* msg is acceptable */
            } while (++trycount < 50);
            while (!letter(*msg))
                ++msg; /* skip control flags */
            if (index(msg, '%'))
                You_hear(msg, halu_gname(EPRI(mtmp)->shralign));
            else
                You_hear1(msg);
            return;
        }
    }
    if (Is_oracle_level(&u.uz) && !rn2(400)) {
        /* make sure the Oracle is still here */
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (mtmp->data == &mons[PM_ORACLE])
                break;
        }
        /* and don't produce silly effects when she's clearly visible */
        if (mtmp && (hallu || !canseemon(mtmp))) {
            static const char *const ora_msg[5] = {
                "一阵奇怪的风.",     /* Jupiter at Dodona */
                "痉挛性的胡言乱语.", /* Apollo at Delphi */
                "蛇的打鼾声.",     /* AEsculapius at Epidaurus */
                "有人说 \" 不再有土拨鼠!\"",
                "吵闹的若特!" /* both rec.humor.oracle */
            };
            You_hear1(ora_msg[rn2(3) + hallu * 2]);
        }
        return;
    }
}

static const char *const h_sounds[] = {
    "嘟嘟响",   "嘣",   "唱歌",   "belche", "咯吱咯吱响",   "咳嗽",
    "发出咯咯声", "哀嚎", "砰的一声",    "叮当作响", "抽泣", "发叮当声",
    "eep",    "发出哗啦声", "发低哼声",    "发嘶嘶声", "吱吱叫", "喘息",
    "沙沙作响", "鸣响",    "咬舌发音",   "真假嗓音互换地唱",  "咕咕地叫",     "打嗝",
    "发出哞哞声",    "发隆隆声",    "低语", "呼噜声",   "嘎嘎叫",   "隆隆响",
    "砰然一声",  "大声喊叫",  "发嘟嘟声",   "发漱音", "大叫",    "啭鸣"
};

const char *
growl_sound(mtmp)
register struct monst *mtmp;
{
    const char *ret;

    switch (mtmp->data->msound) {
    case MS_MEW:
    case MS_HISS:
        ret = "发出嘶嘶声";
        break;
    case MS_BARK:
    case MS_GROWL:
        ret = "咆哮";
        break;
    case MS_ROAR:
        ret = "吼叫";
        break;
    case MS_BUZZ:
        ret = "嗡嗡叫";
        break;
    case MS_SQEEK:
        ret = "啸叫";
        break;
    case MS_SQAWK:
        ret = "尖叫";
        break;
    case MS_NEIGH:
        ret = "嘶叫";
        break;
    case MS_WAIL:
        ret = "呼啸";
        break;
    case MS_SILENT:
        ret = "喧闹";
        break;
    default:
        ret = "惊叫";
    }
    return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void
growl(mtmp)
register struct monst *mtmp;
{
    register const char *growl_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        growl_verb = growl_sound(mtmp);
    if (growl_verb) {
        pline("%s %s!", Monnam(mtmp), vtense((char *) 0, growl_verb));
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void
yelp(mtmp)
register struct monst *mtmp;
{
    register const char *yelp_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
            yelp_verb = "嚎叫";
            break;
        case MS_BARK:
        case MS_GROWL:
            yelp_verb = "嗷叫";
            break;
        case MS_ROAR:
            yelp_verb = "咆哮";
            break;
        case MS_SQEEK:
            yelp_verb = "啸叫";
            break;
        case MS_SQAWK:
            yelp_verb = "嘎嘎作响";
            break;
        case MS_WAIL:
            yelp_verb = "哀号";
            break;
        }
    if (yelp_verb) {
        pline("%s %s!", Monnam(mtmp), vtense((char *) 0, yelp_verb));
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
}

/* the sounds of distressed pets */
void
whimper(mtmp)
register struct monst *mtmp;
{
    register const char *whimper_verb = 0;

    if (mtmp->msleeping || !mtmp->mcanmove || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
        case MS_GROWL:
            whimper_verb = "呜咽";
            break;
        case MS_BARK:
            whimper_verb = "悲鸣";
            break;
        case MS_SQEEK:
            whimper_verb = "啸叫";
            break;
        }
    if (whimper_verb) {
        pline("%s %s.", Monnam(mtmp), vtense((char *) 0, whimper_verb));
        if (context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
}

/* pet makes "I'm hungry" noises */
void
beg(mtmp)
register struct monst *mtmp;
{
    if (mtmp->msleeping || !mtmp->mcanmove
        || !(carnivorous(mtmp->data) || herbivorous(mtmp->data)))
        return;

    /* presumably nearness and soundok checks have already been made */
    if (!is_silent(mtmp->data) && mtmp->data->msound <= MS_ANIMAL)
        (void) domonnoise(mtmp);
    else if (mtmp->data->msound >= MS_HUMANOID) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        verbalize("我饿了.");
    }
}

/* return True if mon is a gecko or seems to look like one (hallucination) */
STATIC_OVL boolean
mon_is_gecko(mon)
struct monst *mon;
{
    int glyph;

    /* return True if it is actually a gecko */
    if (mon->data == &mons[PM_GECKO])
        return TRUE;
    /* return False if it is a long worm; we might be chatting to its tail
       (not strictly needed; long worms are MS_SILENT so won't get here) */
    if (mon->data == &mons[PM_LONG_WORM])
        return FALSE;
    /* result depends upon whether map spot shows a gecko, which will
       be due to hallucination or to mimickery since mon isn't one */
    glyph = glyph_at(mon->mx, mon->my);
    return (boolean) (glyph_to_mon(glyph) == PM_GECKO);
}

STATIC_OVL int
domonnoise(mtmp)
register struct monst *mtmp;
{
    char verbuf[BUFSZ];
    register const char *pline_msg = 0, /* Monnam(mtmp) will be prepended */
        *verbl_msg = 0,                 /* verbalize() */
            *verbl_msg_mcan = 0;        /* verbalize() if cancelled */
    struct permonst *ptr = mtmp->data;
    int msound = ptr->msound;

    /* presumably nearness and sleep checks have already been made */
    if (Deaf)
        return 0;
    if (is_silent(ptr))
        return 0;

    /* leader might be poly'd; if he can still speak, give leader speech */
    if (mtmp->m_id == quest_status.leader_m_id && msound > MS_ANIMAL)
        msound = MS_LEADER;
    /* make sure it's your role's quest guardian; adjust if not */
    else if (msound == MS_GUARDIAN && ptr != &mons[urole.guardnum])
        msound = mons[genus(monsndx(ptr), 1)].msound;
    /* some normally non-speaking types can/will speak if hero is similar */
    else if (msound == MS_ORC         /* note: MS_ORC is same as MS_GRUNT */
             && (same_race(ptr, youmonst.data)           /* current form, */
                 || same_race(ptr, &mons[Race_switch]))) /* unpoly'd form */
        msound = MS_HUMANOID;
    /* silliness, with slight chance to interfere with shopping */
    else if (Hallucination && mon_is_gecko(mtmp))
        msound = MS_SELL;

    /* be sure to do this before talking; the monster might teleport away, in
     * which case we want to check its pre-teleport position
     */
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    switch (msound) {
    case MS_ORACLE:
        return doconsult(mtmp);
    case MS_PRIEST:
        priest_talk(mtmp);
        break;
    case MS_LEADER:
    case MS_NEMESIS:
    case MS_GUARDIAN:
        quest_chat(mtmp);
        break;
    case MS_SELL: /* pitch, pay, total */
        if (!Hallucination || (mtmp->isshk && !rn2(2))) {
            shk_chat(mtmp);
        } else {
            /* approximation of GEICO's advertising slogan (it actually
               concludes with "save you 15% or more on car insurance.") */
            Sprintf(verbuf, "15 分钟能给你节省15 %s.",
                    currency(15L)); /* "zorkmids" */
            verbl_msg = verbuf;
        }
        break;
    case MS_VAMPIRE: {
        /* vampire messages are varied by tameness, peacefulness, and time of
         * night */
        boolean isnight = night();
        boolean kindred = (Upolyd && (u.umonnum == PM_VAMPIRE
                                      || u.umonnum == PM_VAMPIRE_LORD));
        boolean nightchild =
            (Upolyd && (u.umonnum == PM_WOLF || u.umonnum == PM_WINTER_WOLF
                        || u.umonnum == PM_WINTER_WOLF_CUB));
        const char *racenoun =
            (flags.female && urace.individual.f)
                ? urace.individual.f
                : (urace.individual.m) ? urace.individual.m : urace.noun;

        if (mtmp->mtame) {
            if (kindred) {
                Sprintf(verbuf, "%s好主人%s",
                        isnight ? "晚上" : "白天",
                        isnight ? "!" : ".  为什么我们不休息?");
                verbl_msg = verbuf;
            } else {
                Sprintf(verbuf, "%s%s",
                        nightchild ? "夜之子, " : "",
                        midnight()
                         ? "我不再能忍受这种渴望!"
                         : isnight
                          ? "我求你, 帮我满足不断增长的渴望!"
                          : "我发现自己有点疲倦.");
                verbl_msg = verbuf;
            }
        } else if (mtmp->mpeaceful) {
            if (kindred && isnight) {
                Sprintf(verbuf, "好的饲养人%s!",
                        flags.female ? "姐姐" : "哥哥");
                verbl_msg = verbuf;
            } else if (nightchild && isnight) {
                Sprintf(verbuf, "很高兴听到你的声音, 夜之子!");
                verbl_msg = verbuf;
            } else
                verbl_msg = "我只喝...  药水.";
        } else {
            int vampindex;
            static const char *const vampmsg[] = {
                /* These first two (0 and 1) are specially handled below */
                "我想要吮吸你的%s!",
                "我会不后悔而紧跟着%s!",
                /* other famous vampire quotes can follow here if desired */
            };
            if (kindred)
                verbl_msg =
                    "你敢潜行于我的狩猎场!";
            else if (youmonst.data == &mons[PM_SILVER_DRAGON]
                     || youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
                /* Silver dragons are silver in color, not made of silver */
                Sprintf(verbuf, "%s! 你的银色光泽不会吓到我!",
                        youmonst.data == &mons[PM_SILVER_DRAGON]
                            ? "傻瓜"
                            : "小傻瓜");
                verbl_msg = verbuf;
            } else {
                vampindex = rn2(SIZE(vampmsg));
                if (vampindex == 0) {
                    Sprintf(verbuf, vampmsg[vampindex], body_part(BLOOD));
                    verbl_msg = verbuf;
                } else if (vampindex == 1) {
                    Sprintf(verbuf, vampmsg[vampindex],
                            Upolyd ? mons[u.umonnum].mname
                                   : racenoun);
                    verbl_msg = verbuf;
                } else
                    verbl_msg = vampmsg[vampindex];
            }
        }
    } break;
    case MS_WERE:
        if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
            pline("%s 使%s头向后扬并让血液凝固%s!",
                  Monnam(mtmp), mhis(mtmp),
                  ptr == &mons[PM_HUMAN_WERERAT] ? "尖叫" : "长嚎");
            wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        } else
            pline_msg =
                "以几乎听不见的声音低语.  你所能辨认出的就是 \" 月亮\".";
        break;
    case MS_BARK:
        if (flags.moonphase == FULL_MOON && night()) {
            pline_msg = "嚎叫.";
        } else if (mtmp->mpeaceful) {
            if (mtmp->mtame
                && (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                    || moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
                pline_msg = "呜咽.";
            else if (mtmp->mtame && EDOG(mtmp)->hungrytime > moves + 1000)
                pline_msg = "犬吠.";
            else {
                if (mtmp->data
                    != &mons[PM_DINGO]) /* dingos do not actually bark */
                    pline_msg = "吠叫.";
            }
        } else {
            pline_msg = "狂吠.";
        }
        break;
    case MS_MEW:
        if (mtmp->mtame) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                || mtmp->mtame < 5)
                pline_msg = "号叫.";
            else if (moves > EDOG(mtmp)->hungrytime)
                pline_msg = "喵喵叫.";
            else if (EDOG(mtmp)->hungrytime > moves + 1000)
                pline_msg = "呜呜.";
            else
                pline_msg = "咪咪叫.";
            break;
        } /* else FALLTHRU */
    case MS_GROWL:
        pline_msg = mtmp->mpeaceful ? "吼叫." : "咆哮!";
        break;
    case MS_ROAR:
        pline_msg = mtmp->mpeaceful ? "吼叫." : "咆哮!";
        break;
    case MS_SQEEK:
        pline_msg = "吱吱叫.";
        break;
    case MS_SQAWK:
        if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful)
            verbl_msg = "决不再!";
        else
            pline_msg = "抱怨.";
        break;
    case MS_HISS:
        if (!mtmp->mpeaceful)
            pline_msg = "嘶嘶叫!";
        else
            return 0; /* no sound */
        break;
    case MS_BUZZ:
        pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily.";
        break;
    case MS_GRUNT:
        pline_msg = "咕噜叫.";
        break;
    case MS_NEIGH:
        if (mtmp->mtame < 5)
            pline_msg = "嘶叫.";
        else if (moves > EDOG(mtmp)->hungrytime)
            pline_msg = "轻嘶.";
        else
            pline_msg = "嘶鸣.";
        break;
    case MS_WAIL:
        pline_msg = "凄惨地哀号.";
        break;
    case MS_GURGLE:
        pline_msg = "咯咯叫.";
        break;
    case MS_BURBLE:
        pline_msg = "汩汩作响.";
        break;
    case MS_SHRIEK:
        pline_msg = "尖叫.";
        aggravate();
        break;
    case MS_IMITATE:
        pline_msg = "模仿你.";
        break;
    case MS_BONES:
        pline("%s 吵闹地喋喋不休.", Monnam(mtmp));
        You("被僵硬了片刻.");
        nomul(-2);
        multi_reason = "被喋喋不休吓到了";
        nomovemsg = 0;
        break;
    case MS_LAUGH: {
        static const char *const laugh_msg[4] = {
            "格格笑.", "轻笑.", "窃笑.", "笑.",
        };
        pline_msg = laugh_msg[rn2(4)];
    } break;
    case MS_MUMBLE:
        pline_msg = "费解的喃喃自语.";
        break;
    case MS_DJINNI:
        if (mtmp->mtame) {
            verbl_msg = "抱歉, 我不想做任何事.";
        } else if (mtmp->mpeaceful) {
            if (ptr == &mons[PM_WATER_DEMON])
                pline_msg = "汩汩作响.";
            else
                verbl_msg = "我自由了!";
        } else {
            if (ptr != &mons[PM_PRISONER])
                verbl_msg = "这会教你不要打扰我!";
#if 0
            else
                verbl_msg = "??????????";
#endif
        }
        break;
    case MS_BOAST: /* giants */
        if (!mtmp->mpeaceful) {
            switch (rn2(4)) {
            case 0:
                pline("%s 夸耀着%s宝石收藏.", Monnam(mtmp),
                      mhis(mtmp));
                break;
            case 1:
                pline_msg = "抱怨羊肉饮食.";
                break;
            default:
                pline_msg = "叫喊 \"Fee Fie Foe Foo!\"  然后哈哈大笑.";
                wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
                break;
            }
            break;
        }
    /* else FALLTHRU */
    case MS_HUMANOID:
        if (!mtmp->mpeaceful) {
            if (In_endgame(&u.uz) && is_mplayer(ptr))
                mplayer_talk(mtmp);
            else
                pline_msg = "恐吓你.";
            break;
        }
        /* Generic peaceful humanoid behaviour. */
        if (mtmp->mflee)
            pline_msg = "不想与你做任何事.";
        else if (mtmp->mhp < mtmp->mhpmax / 4)
            pline_msg = "呻吟.";
        else if (mtmp->mconf || mtmp->mstun)
            verbl_msg = !rn2(3) ? "哈?" : rn2(2) ? "什么?" : "嗯?";
        else if (!mtmp->mcansee)
            verbl_msg = "我看不见了!";
        else if (mtmp->mtrapped) {
            struct trap *t = t_at(mtmp->mx, mtmp->my);

            if (t)
                t->tseen = 1;
            verbl_msg = "我被困住了!";
        } else if (mtmp->mhp < mtmp->mhpmax / 2)
            pline_msg = "向你要一瓶治愈药水.";
        else if (mtmp->mtame && !mtmp->isminion
                 && moves > EDOG(mtmp)->hungrytime)
            verbl_msg = "我饿了.";
        /* Specific monsters' interests */
        else if (is_elf(ptr))
            pline_msg = "咒骂兽人.";
        else if (is_dwarf(ptr))
            pline_msg = "谈论着采矿.";
        else if (likes_magic(ptr))
            pline_msg = "谈论着辨识魔法.";
        else if (ptr->mlet == S_CENTAUR)
            pline_msg = "讨论着打猎.";
        else
            switch (monsndx(ptr)) {
            case PM_HOBBIT:
                pline_msg =
                    (mtmp->mhpmax - mtmp->mhp >= 10)
                        ? "抱怨着讨厌的地牢环境."
                        : "问你关于至尊魔戒的事.";
                break;
            case PM_ARCHEOLOGIST:
                pline_msg =
                "描述杂志上的最近一篇文章 \"Spelunker Today\".";
                break;
            case PM_TOURIST:
                verbl_msg = "你好.";
                break;
            default:
                pline_msg = "讨论地牢的探险.";
                break;
            }
        break;
    case MS_SEDUCE: {
        int swval;
        if (SYSOPT_SEDUCE) {
            if (ptr->mlet != S_NYMPH
                && could_seduce(mtmp, &youmonst, (struct attack *) 0) == 1) {
                (void) doseduce(mtmp);
                break;
            }
            swval = ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0);
        } else
            swval = ((poly_gender() == 0) ? rn2(3) : 0);
        switch (swval) {
        case 2:
            verbl_msg = "你好, 水手.";
            break;
        case 1:
            pline_msg = "调戏你.";
            break;
        default:
            pline_msg = "哄骗你.";
        }
    } break;
    case MS_ARREST:
        if (mtmp->mpeaceful)
            verbalize("只是事实, %s.", flags.female ? "夫人" : "阁下");
        else {
            static const char *const arrest_msg[3] = {
                "你所说的任何话都会被用作对你不利的供词.",
                "你被捕了!", "以法律的名义命令停下!",
            };
            verbl_msg = arrest_msg[rn2(3)];
        }
        break;
    case MS_BRIBE:
        if (mtmp->mpeaceful && !mtmp->mtame) {
            (void) demon_talk(mtmp);
            break;
        }
    /* fall through */
    case MS_CUSS:
        if (!mtmp->mpeaceful)
            cuss(mtmp);
        else if (is_lminion(mtmp))
            verbl_msg = "这还不算太晚.";
        else
            verbl_msg = "我们劫数难逃.";
        break;
    case MS_SPELL:
        /* deliberately vague, since it's not actually casting any spell */
        pline_msg = "似乎低声说了一个咒语.";
        break;
    case MS_NURSE:
        verbl_msg_mcan = "我恨这个工作!";
        if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
            verbl_msg = "在你伤害别人之前把那个武器拿开!";
        else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
            verbl_msg = Role_if(PM_HEALER)
                            ? "医生, 除非你合作否则我没法帮助你."
                            : "请脱下衣服让我检查你.";
        else if (uarmu)
            verbl_msg = "请脱下你的衬衫.";
        else
            verbl_msg = "放松, 这一点也不疼.";
        break;
    case MS_GUARD:
        if (money_cnt(invent))
            verbl_msg = "请放下那个金币然后跟着我.";
        else
            verbl_msg = "请跟我来.";
        break;
    case MS_SOLDIER: {
        static const char
            *const soldier_foe_msg[3] =
                {
                  "抵抗是无用的!", "你死定了!", "投降吧!",
                },
                   *const soldier_pax_msg[3] = {
                       "我在这里拿如此糟糕的工资!",
                       "这食物不适合兽人!",
                       "我的脚受伤了, 我整天都在使用它们!",
                   };
        verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
                                    : soldier_foe_msg[rn2(3)];
        break;
    }
    case MS_RIDER:
        /* 3.6.0 tribute */
        if (ptr == &mons[PM_DEATH]
            && !context.tribute.Deathnotice && u_have_novel()) {
            struct obj *book = u_have_novel();
            const char *tribtitle = (char *)0;

            if (book) {
                int novelidx = book->novelidx;

                tribtitle = noveltitle(&novelidx);
            }
            if (tribtitle) {
                Sprintf(verbuf, "啊, 所以你有一本 /%s/.", tribtitle);
                /* no Death featured in these two, so exclude them */
                if (!(strcmpi(tribtitle, "Snuff") == 0
                      || strcmpi(tribtitle, "The Wee Free Men") == 0))
                    Strcat(verbuf, " 我可能错误地引用了那里.");
                verbl_msg = verbuf;
                context.tribute.Deathnotice = 1;
            }
        } else if (ptr == &mons[PM_DEATH]
                   && !rn2(2) && Death_quote(verbuf, BUFSZ)) {
                verbl_msg = verbuf;
        }
        /* end of tribute addition */
        else if (ptr == &mons[PM_DEATH] && !rn2(10))
            pline_msg = "正忙着阅读一本 Sandman #8.";
        else
            verbl_msg = "你以为你是谁, 战争?";
        break;
    }

    if (pline_msg)
        pline("%s %s", Monnam(mtmp), pline_msg);
    else if (mtmp->mcan && verbl_msg_mcan)
        verbalize1(verbl_msg_mcan);
    else if (verbl_msg) {
        if (ptr == &mons[PM_DEATH]) {
            /* Death talks in CAPITAL LETTERS
               and without quotation marks */
            char tmpbuf[BUFSZ];
            Sprintf(tmpbuf, "%s", verbl_msg);
            pline1(ucase(tmpbuf));
        } else {
            verbalize1(verbl_msg);
        }
    }
    return 1;
}

/* #chat command */
int
dotalk()
{
    int result;

    result = dochat();
    return result;
}

STATIC_OVL int
dochat()
{
    struct monst *mtmp;
    int tx, ty;
    struct obj *otmp;

    if (is_silent(youmonst.data)) {
        pline("作为一只%s, 你无法说话.", youmonst.data->mname);
        return 0;
    }
    if (Strangled) {
        You_cant("说话.  你被窒息!");
        return 0;
    }
    if (u.uswallow) {
        pline("它们在外面听不见你.");
        return 0;
    }
    if (Underwater) {
        Your("话语在水下难以理解.");
        return 0;
    }
    if (Deaf) {
        pline("你听不见时如何进行谈话?");
        return 0;
    }

    if (!Blind && (otmp = shop_object(u.ux, u.uy)) != (struct obj *) 0) {
        /* standing on something in a shop and chatting causes the shopkeeper
           to describe the price(s).  This can inhibit other chatting inside
           a shop, but that shouldn't matter much.  shop_object() returns an
           object iff inside a shop and the shopkeeper is present and willing
           (not angry) and able (not asleep) to speak and the position
           contains any objects other than just gold.
        */
        price_quote(otmp);
        return 1;
    }

    if (!getdir("和谁交谈? ( 哪个方向)")) {
        /* decided not to chat */
        return 0;
    }

    if (u.usteed && u.dz > 0) {
        if (!u.usteed->mcanmove || u.usteed->msleeping) {
            pline("%s 似乎没有注意到你.", Monnam(u.usteed));
            return 1;
        } else
            return domonnoise(u.usteed);
    }

    if (u.dz) {
        pline("它们在%s听不见你.", u.dz < 0 ? "上面" : "下面");
        return 0;
    }

    if (u.dx == 0 && u.dy == 0) {
        /*
         * Let's not include this.
         * It raises all sorts of questions: can you wear
         * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
         * etc...  --KAA
        if (u.umonnum == PM_ETTIN) {
            You("discover that your other head makes boring conversation.");
            return 1;
        }
         */
        pline("自我交谈是冒险家的一个坏习惯.");
        return 0;
    }

    tx = u.ux + u.dx;
    ty = u.uy + u.dy;

    if (!isok(tx, ty))
        return 0;

    mtmp = m_at(tx, ty);

    if ((!mtmp || mtmp->mundetected)
        && (otmp = vobj_at(tx, ty)) != 0 && otmp->otyp == STATUE) {
        /* Talking to a statue */
        if (!Blind) {
            pline_The("%s 似乎没有注意到你.",
                      /* if hallucinating, you can't tell it's a statue */
                      Hallucination ? rndmonnam((char *) 0) : "雕像");
        }
        return 0;
    }

    if (!mtmp || mtmp->mundetected || mtmp->m_ap_type == M_AP_FURNITURE
        || mtmp->m_ap_type == M_AP_OBJECT)
        return 0;

    /* sleeping monsters won't talk, except priests (who wake up) */
    if ((!mtmp->mcanmove || mtmp->msleeping) && !mtmp->ispriest) {
        /* If it is unseen, the player can't tell the difference between
           not noticing him and just not existing, so skip the message. */
        if (canspotmon(mtmp))
            pline("%s 似乎没有注意到你.", Monnam(mtmp));
        return 0;
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (mtmp->mtame && mtmp->meating) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        pline("%s 在大声地吃.", Monnam(mtmp));
        return 0;
    }

    return domonnoise(mtmp);
}

#ifdef USER_SOUNDS

extern void FDECL(play_usersound, (const char *, int));

typedef struct audio_mapping_rec {
    struct nhregex *regex;
    char *filename;
    int volume;
    struct audio_mapping_rec *next;
} audio_mapping;

static audio_mapping *soundmap = 0;

char *sounddir = ".";

/* adds a sound file mapping, returns 0 on failure, 1 on success */
int
add_sound_mapping(mapping)
const char *mapping;
{
    char text[256];
    char filename[256];
    char filespec[256];
    int volume;

    if (sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d", text,
               filename, &volume) == 3) {
        audio_mapping *new_map;

        if (strlen(sounddir) + strlen(filename) > 254) {
            raw_print("sound file name too long");
            return 0;
        }
        Sprintf(filespec, "%s/%s", sounddir, filename);

        if (can_read_file(filespec)) {
            new_map = (audio_mapping *) alloc(sizeof(audio_mapping));
            new_map->regex = regex_init();
            new_map->filename = dupstr(filespec);
            new_map->volume = volume;
            new_map->next = soundmap;

            if (!regex_compile(text, new_map->regex)) {
                raw_print(regex_error_desc(new_map->regex));
                regex_free(new_map->regex);
                free(new_map->filename);
                free(new_map);
                return 0;
            } else {
                soundmap = new_map;
            }
        } else {
            Sprintf(text, "cannot read %.243s", filespec);
            raw_print(text);
            return 0;
        }
    } else {
        raw_print("syntax error in SOUND");
        return 0;
    }

    return 1;
}

void
play_sound_for_message(msg)
const char *msg;
{
    audio_mapping *cursor = soundmap;

    while (cursor) {
        if (regex_match(msg, cursor->regex)) {
            play_usersound(cursor->filename, cursor->volume);
        }
        cursor = cursor->next;
    }
}

#endif /* USER_SOUNDS */

/*sounds.c*/
