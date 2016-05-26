/* NetHack 3.6	eat.c	$NHDT-Date: 1449269916 2015/12/04 22:58:36 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.154 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

STATIC_PTR int NDECL(eatmdone);
STATIC_PTR int NDECL(eatfood);
STATIC_PTR void FDECL(costly_tin, (int));
STATIC_PTR int NDECL(opentin);
STATIC_PTR int NDECL(unfaint);

STATIC_DCL const char *FDECL(food_xname, (struct obj *, BOOLEAN_P));
STATIC_DCL void FDECL(choke, (struct obj *));
STATIC_DCL void NDECL(recalc_wt);
STATIC_DCL struct obj *FDECL(touchfood, (struct obj *));
STATIC_DCL void NDECL(do_reset_eat);
STATIC_DCL void FDECL(done_eating, (BOOLEAN_P));
STATIC_DCL void FDECL(cprefx, (int));
STATIC_DCL int FDECL(intrinsic_possible, (int, struct permonst *));
STATIC_DCL void FDECL(givit, (int, struct permonst *));
STATIC_DCL void FDECL(cpostfx, (int));
STATIC_DCL void FDECL(consume_tin, (const char *));
STATIC_DCL void FDECL(start_tin, (struct obj *));
STATIC_DCL int FDECL(eatcorpse, (struct obj *));
STATIC_DCL void FDECL(start_eating, (struct obj *));
STATIC_DCL void FDECL(fprefx, (struct obj *));
STATIC_DCL void FDECL(fpostfx, (struct obj *));
STATIC_DCL int NDECL(bite);
STATIC_DCL int FDECL(edibility_prompts, (struct obj *));
STATIC_DCL int FDECL(rottenfood, (struct obj *));
STATIC_DCL void NDECL(eatspecial);
STATIC_DCL int FDECL(bounded_increase, (int, int, int));
STATIC_DCL void FDECL(accessory_has_effect, (struct obj *));
STATIC_DCL void FDECL(eataccessory, (struct obj *));
STATIC_DCL const char *FDECL(foodword, (struct obj *));
STATIC_DCL int FDECL(tin_variety, (struct obj *, BOOLEAN_P));
STATIC_DCL boolean FDECL(maybe_cannibal, (int, BOOLEAN_P));

char msgbuf[BUFSZ];

/* also used to see if you're allowed to eat cats and dogs */
#define CANNIBAL_ALLOWED() (Role_if(PM_CAVEMAN) || Race_if(PM_ORC))

/* monster types that cause hero to be turned into stone if eaten */
#define flesh_petrifies(pm) (touch_petrifies(pm) || (pm) == &mons[PM_MEDUSA])

/* Rider corpses are treated as non-rotting so that attempting to eat one
   will be sure to reach the stage of eating where that meal is fatal */
#define nonrotting_corpse(mnum) \
    ((mnum) == PM_LIZARD || (mnum) == PM_LICHEN || is_rider(&mons[mnum]))

/* non-rotting non-corpses; unlike lizard corpses, these items will behave
   as if rotten if they are cursed (fortune cookies handled elsewhere) */
#define nonrotting_food(otyp) \
    ((otyp) == LEMBAS_WAFER || (otyp) == CRAM_RATION)

STATIC_OVL NEARDATA const char comestibles[] = { FOOD_CLASS, 0 };
STATIC_OVL NEARDATA const char offerfodder[] = { FOOD_CLASS, AMULET_CLASS,
                                                 0 };

/* Gold must come first for getobj(). */
STATIC_OVL NEARDATA const char allobj[] = {
    COIN_CLASS,   WEAPON_CLASS, ARMOR_CLASS,  POTION_CLASS,
    SCROLL_CLASS, WAND_CLASS,   RING_CLASS,   AMULET_CLASS,
    FOOD_CLASS,   TOOL_CLASS,   GEM_CLASS,    ROCK_CLASS,
    BALL_CLASS,   CHAIN_CLASS,  SPBOOK_CLASS, 0
};

STATIC_OVL boolean force_save_hs = FALSE;

/* see hunger states in hack.h - texts used on bottom line */
const char *hu_stat[] = { " 饱腹", "        ", " 饥饿", " 虚弱",
                          " 昏厥", " 晕厥", " 极度饥饿" };
/*
 * const char *hu_stat[] = { "Satiated", "        ", "Hungry  ", "Weak    ",
                          "Fainting", "Fainted ", "Starved " };
 * */

/*
 * Decide whether a particular object can be eaten by the possibly
 * polymorphed character.  Not used for monster checks.
 */
boolean
is_edible(obj)
register struct obj *obj;
{
    /* protect invocation tools but not Rider corpses (handled elsewhere)*/
    /* if (obj->oclass != FOOD_CLASS && obj_resists(obj, 0, 0)) */
    if (objects[obj->otyp].oc_unique)
        return FALSE;
    /* above also prevents the Amulet from being eaten, so we must never
       allow fake amulets to be eaten either [which is already the case] */

    if (metallivorous(youmonst.data) && is_metallic(obj)
        && (youmonst.data != &mons[PM_RUST_MONSTER] || is_rustprone(obj)))
        return TRUE;

    if (u.umonnum == PM_GELATINOUS_CUBE && is_organic(obj)
        /* [g.cubes can eat containers and retain all contents
            as engulfed items, but poly'd player can't do that] */
        && !Has_contents(obj))
        return TRUE;

    /* return (boolean) !!index(comestibles, obj->oclass); */
    return (boolean) (obj->oclass == FOOD_CLASS);
}

void
init_uhunger()
{
    u.uhunger = 900;
    u.uhs = NOT_HUNGRY;
}

/* tin types [SPINACH_TIN = -1, overrides corpsenm, nut==600] */
static const struct {
    const char *txt;                      /* description */
    int nut;                              /* nutrition */
    Bitfield(fodder, 1);                  /* stocked by health food shops */
    Bitfield(greasy, 1);                  /* causes slippery fingers */
} tintxts[] = { { "腐烂的", -50, 0, 0 },  /* ROTTEN_TIN = 0 */
                { "自制的", 50, 1, 0 }, /* HOMEMADE_TIN = 1 */
                { "做成汤的", 20, 1, 0 },
                { "油炸的", 40, 0, 1 },
                { "腌制的", 40, 1, 0 },
                { "煮熟的", 50, 1, 0 },
                { "熏制的", 50, 1, 0 },
                { "干的", 55, 1, 0 },
                { "深油炸的", 60, 0, 1 },
                { "四川", 70, 1, 0 },
                { "烧烤的", 80, 0, 0 },
                { "炒的", 80, 0, 1 },
                { "清炒的", 95, 0, 0 },
                { "糖制的", 100, 1, 0 },
                { "浓的", 500, 1, 0 },
                { "", 0, 0, 0 } };
#define TTSZ SIZE(tintxts)

static char *eatmbuf = 0; /* set by cpostfx() */

/* called after mimicing is over */
STATIC_PTR int
eatmdone(VOID_ARGS)
{
    /* release `eatmbuf' */
    if (eatmbuf) {
        if (nomovemsg == eatmbuf)
            nomovemsg = 0;
        free((genericptr_t) eatmbuf), eatmbuf = 0;
    }
    /* update display */
    if (youmonst.m_ap_type) {
        youmonst.m_ap_type = M_AP_NOTHING;
        newsym(u.ux, u.uy);
    }
    return 0;
}

/* called when hallucination is toggled */
void
eatmupdate()
{
    const char *altmsg = 0;
    int altapp = 0; /* lint suppression */

    if (!eatmbuf || nomovemsg != eatmbuf)
        return;

    if (is_obj_mappear(&youmonst,ORANGE) && !Hallucination) {
        /* revert from hallucinatory to "normal" mimicking */
        altmsg = "你现在更喜欢模仿自己.";
        altapp = GOLD_PIECE;
    } else if (is_obj_mappear(&youmonst,GOLD_PIECE) && Hallucination) {
        /* won't happen; anything which might make immobilized
           hero begin hallucinating (black light attack, theft
           of Grayswandir) will terminate the mimicry first */
        altmsg = "Your rind escaped intact.";
        altapp = ORANGE;
    }

    if (altmsg) {
        /* replace end-of-mimicking message */
        if (strlen(altmsg) > strlen(eatmbuf)) {
            free((genericptr_t) eatmbuf);
            eatmbuf = (char *) alloc(strlen(altmsg) + 1);
        }
        nomovemsg = strcpy(eatmbuf, altmsg);
        /* update current image */
        youmonst.mappearance = altapp;
        newsym(u.ux, u.uy);
    }
}

/* ``[the(] singular(food, xname) [)]'' */
STATIC_OVL const char *
food_xname(food, the_pfx)
struct obj *food;
boolean the_pfx;
{
    const char *result;

    if (food->otyp == CORPSE) {
        result = corpse_xname(food, (const char *) 0,
                              CXN_SINGULAR | (the_pfx ? CXN_PFX_THE : 0));
        /* not strictly needed since pname values are capitalized
           and the() is a no-op for them */
        if (type_is_pname(&mons[food->corpsenm]))
            the_pfx = FALSE;
    } else {
        /* the ordinary case */
        result = singular(food, xname);
    }
    if (the_pfx)
        result = result;
    return result;
}

/* Created by GAN 01/28/87
 * Amended by AKP 09/22/87: if not hard, don't choke, just vomit.
 * Amended by 3.  06/12/89: if not hard, sometimes choke anyway, to keep risk.
 *                11/10/89: if hard, rarely vomit anyway, for slim chance.
 *
 * To a full belly all food is bad. (It.)
 */
STATIC_OVL void
choke(food)
struct obj *food;
{
    /* only happens if you were satiated */
    if (u.uhs != SATIATED) {
        if (!food || food->otyp != AMULET_OF_STRANGULATION)
            return;
    } else if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL) {
        adjalign(-1); /* gluttony is unchivalrous */
        You_feel("像一个吃货!");
    }

    exercise(A_CON, FALSE);

    if (Breathless || (!Strangled && !rn2(20))) {
        /* choking by eating AoS doesn't involve stuffing yourself */
        if (food && food->otyp == AMULET_OF_STRANGULATION) {
            You("噎着了, 但是在镇静之后恢复了.");
            return;
        }
        You("吃的过多并大量的呕吐.");
        morehungry(1000); /* you just got *very* sick! */
        vomit();
    } else {
        killer.format = KILLED_BY_AN;
        /*
         * Note all "killer"s below read "Choked on %s" on the
         * high score list & tombstone.  So plan accordingly.
         */
        if (food) {
            You("因你的%s 而噎着了.", foodword(food));
            if (food->oclass == COIN_CLASS) {
                Strcpy(killer.name, "很丰盛的一顿饭");
            } else {
                killer.format = KILLED_BY;
                Strcpy(killer.name, killer_xname(food));
            }
        } else {
            You("因它而噎着了.");
            Strcpy(killer.name, "一顿快餐");
        }
        You("死了...");
        done(CHOKING);
    }
}

/* modify object wt. depending on time spent consuming it */
STATIC_OVL void
recalc_wt()
{
    struct obj *piece = context.victual.piece;
    if (!piece) {
        impossible("recalc_wt without piece");
        return;
    }
    debugpline1("Old weight = %d", piece->owt);
    debugpline2("Used time = %d, Req'd time = %d", context.victual.usedtime,
                context.victual.reqtime);
    piece->owt = weight(piece);
    debugpline1("New weight = %d", piece->owt);
}

/* called when eating interrupted by an event */
void
reset_eat()
{
    /* we only set a flag here - the actual reset process is done after
     * the round is spent eating.
     */
    if (context.victual.eating && !context.victual.doreset) {
        debugpline0("reset_eat...");
        context.victual.doreset = TRUE;
    }
    return;
}

STATIC_OVL struct obj *
touchfood(otmp)
struct obj *otmp;
{
    if (otmp->quan > 1L) {
        if (!carried(otmp))
            (void) splitobj(otmp, otmp->quan - 1L);
        else
            otmp = splitobj(otmp, 1L);
        debugpline0("split object,");
    }

    if (!otmp->oeaten) {
        costly_alteration(otmp, COST_BITE);
        otmp->oeaten =
            (otmp->otyp == CORPSE ? mons[otmp->corpsenm].cnutrit
                                  : objects[otmp->otyp].oc_nutrition);
    }

    if (carried(otmp)) {
        freeinv(otmp);
        if (inv_cnt(FALSE) >= 52) {
            sellobj_state(SELL_DONTSELL);
            dropy(otmp);
            sellobj_state(SELL_NORMAL);
        } else {
            otmp->nomerge = 1; /* used to prevent merge */
            otmp = addinv(otmp);
            otmp->nomerge = 0;
        }
    }
    return otmp;
}

/* When food decays, in the middle of your meal, we don't want to dereference
 * any dangling pointers, so set it to null (which should still trigger
 * do_reset_eat() at the beginning of eatfood()) and check for null pointers
 * in do_reset_eat().
 */
void
food_disappears(obj)
struct obj *obj;
{
    if (obj == context.victual.piece) {
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
    }
    if (obj->timed)
        obj_stop_timers(obj);
}

/* renaming an object used to result in it having a different address,
   so the sequence start eating/opening, get interrupted, name the food,
   resume eating/opening would restart from scratch */
void
food_substitution(old_obj, new_obj)
struct obj *old_obj, *new_obj;
{
    if (old_obj == context.victual.piece) {
        context.victual.piece = new_obj;
        context.victual.o_id = new_obj->o_id;
    }
    if (old_obj == context.tin.tin) {
        context.tin.tin = new_obj;
        context.tin.o_id = new_obj->o_id;
    }
}

STATIC_OVL void
do_reset_eat()
{
    debugpline0("do_reset_eat...");
    if (context.victual.piece) {
        context.victual.o_id = 0;
        context.victual.piece = touchfood(context.victual.piece);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
        recalc_wt();
    }
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
    /* Do not set canchoke to FALSE; if we continue eating the same object
     * we need to know if canchoke was set when they started eating it the
     * previous time.  And if we don't continue eating the same object
     * canchoke always gets recalculated anyway.
     */
    stop_occupation();
    newuhs(FALSE);
}

/* called each move during eating process */
STATIC_PTR int
eatfood(VOID_ARGS)
{
    if (!context.victual.piece
        || (!carried(context.victual.piece)
            && !obj_here(context.victual.piece, u.ux, u.uy))) {
        /* maybe it was stolen? */
        do_reset_eat();
        return 0;
    }
    if (!context.victual.eating)
        return 0;

    if (++context.victual.usedtime <= context.victual.reqtime) {
        if (bite())
            return 0;
        return 1; /* still busy */
    } else {        /* done */
        done_eating(TRUE);
        return 0;
    }
}

STATIC_OVL void
done_eating(message)
boolean message;
{
    context.victual.piece->in_use = TRUE;
    occupation = 0; /* do this early, so newuhs() knows we're done */
    newuhs(FALSE);
    if (nomovemsg) {
        if (message)
            pline1(nomovemsg);
        nomovemsg = 0;
    } else if (message)
        You("吃完了%s.", food_xname(context.victual.piece, TRUE));

    if (context.victual.piece->otyp == CORPSE)
        cpostfx(context.victual.piece->corpsenm);
    else
        fpostfx(context.victual.piece);

    if (carried(context.victual.piece))
        useup(context.victual.piece);
    else
        useupf(context.victual.piece, 1L);
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.fullwarn = context.victual.eating =
        context.victual.doreset = FALSE;
}

void
eating_conducts(pd)
struct permonst *pd;
{
    u.uconduct.food++;
    if (!vegan(pd))
        u.uconduct.unvegan++;
    if (!vegetarian(pd))
        violated_vegetarian();
}

/* handle side-effects of mind flayer's tentacle attack */
int
eat_brains(magr, mdef, visflag, dmg_p)
struct monst *magr, *mdef;
boolean visflag;
int *dmg_p; /* for dishing out extra damage in lieu of Int loss */
{
    struct permonst *pd = mdef->data;
    boolean give_nutrit = FALSE;
    int result = MM_HIT, xtra_dmg = rnd(10);

    if (noncorporeal(pd)) {
        if (visflag)
            pline("%s 脑子没有受伤.",
                  (mdef == &youmonst) ? "你的" : s_suffix(Monnam(mdef)));
        return MM_MISS; /* side-effects can't occur */
    } else if (magr == &youmonst) {
        You("吃%s 脑子!", s_suffix(mon_nam(mdef)));
    } else if (mdef == &youmonst) {
        Your("脑子被吃了!");
    } else { /* monster against monster */
        if (visflag)
            pline("%s 脑子被吃了!", s_suffix(Monnam(mdef)));
    }

    if (flesh_petrifies(pd)) {
        /* mind flayer has attempted to eat the brains of a petrification
           inducing critter (most likely Medusa; attacking a cockatrice via
           tentacle-touch should have been caught before reaching this far) */
        if (magr == &youmonst) {
            if (!Stone_resistance && !Stoned)
                make_stoned(5L, (char *) 0, KILLED_BY_AN, pd->mname);
        } else {
            /* no need to check for poly_when_stoned or Stone_resistance;
               mind flayers don't have those capabilities */
            if (visflag)
                pline("%s 变成了石头!", Monnam(magr));
            monstone(magr);
            if (magr->mhp > 0) {
                /* life-saved; don't continue eating the brains */
                return MM_MISS;
            } else {
                if (magr->mtame && !visflag)
                    /* parallels mhitm.c's brief_feeling */
                    You("片刻有一种悲伤的感觉, 然后消失了.");
                return MM_AGR_DIED;
            }
        }
    }

    if (magr == &youmonst) {
        /*
         * player mind flayer is eating something's brain
         */
        eating_conducts(pd);
        if (mindless(pd)) { /* (cannibalism not possible here) */
            pline("%s 没有注意.", Monnam(mdef));
            /* all done; no extra harm inflicted upon target */
            return MM_MISS;
        } else if (is_rider(pd)) {
            pline("吸取那个是致命的.");
            Sprintf(killer.name, "不明智地吃了%s的脑子", pd->mname);
            killer.format = NO_KILLER_PREFIX;
            done(DIED);
            /* life-saving needed to reach here */
            exercise(A_WIS, FALSE);
            *dmg_p += xtra_dmg; /* Rider takes extra damage */
        } else {
            morehungry(-rnd(30)); /* cannot choke */
            if (ABASE(A_INT) < AMAX(A_INT)) {
                /* recover lost Int; won't increase current max */
                ABASE(A_INT) += rnd(4);
                if (ABASE(A_INT) > AMAX(A_INT))
                    ABASE(A_INT) = AMAX(A_INT);
                context.botl = 1;
            }
            exercise(A_WIS, TRUE);
            *dmg_p += xtra_dmg;
        }
        /* targetting another mind flayer or your own underlying species
           is cannibalism */
        (void) maybe_cannibal(monsndx(pd), TRUE);

    } else if (mdef == &youmonst) {
        /*
         * monster mind flayer is eating hero's brain
         */
        /* no such thing as mindless players */
        if (ABASE(A_INT) <= ATTRMIN(A_INT)) {
            static NEARDATA const char brainlessness[] = "无脑的";

            if (Lifesaved) {
                Strcpy(killer.name, brainlessness);
                killer.format = KILLED_BY;
                done(DIED);
                /* amulet of life saving has now been used up */
                pline("Unfortunately your brain is still gone.");
                /* sanity check against adding other forms of life-saving */
                u.uprops[LIFESAVED].extrinsic =
                    u.uprops[LIFESAVED].intrinsic = 0L;
            } else {
                Your("最后的思想逐渐消失.");
            }
            Strcpy(killer.name, brainlessness);
            killer.format = KILLED_BY;
            done(DIED);
            /* can only get here when in wizard or explore mode and user has
               explicitly chosen not to die; arbitrarily boost intelligence */
            ABASE(A_INT) = ATTRMIN(A_INT) + 2;
            You_feel("像一个稻草人.");
        }
        give_nutrit = TRUE; /* in case a conflicted pet is doing this */
        exercise(A_WIS, FALSE);
        /* caller handles Int and memory loss */

    } else { /* mhitm */
        /*
         * monster mind flayer is eating another monster's brain
         */
        if (mindless(pd)) {
            if (visflag)
                pline("%s 没有注意.", Monnam(mdef));
            return MM_MISS;
        } else if (is_rider(pd)) {
            mondied(magr);
            if (magr->mhp <= 0)
                result = MM_AGR_DIED;
            /* Rider takes extra damage regardless of whether attacker dies */
            *dmg_p += xtra_dmg;
        } else {
            *dmg_p += xtra_dmg;
            give_nutrit = TRUE;
            if (*dmg_p >= mdef->mhp && visflag)
                pline("%s 最后的思想逐渐消失...",
                      s_suffix(Monnam(mdef)));
        }
    }

    if (give_nutrit && magr->mtame && !magr->isminion) {
        EDOG(magr)->hungrytime += rnd(60);
        magr->mconf = 0;
    }

    return result;
}

/* eating a corpse or egg of one's own species is usually naughty */
STATIC_OVL boolean
maybe_cannibal(pm, allowmsg)
int pm;
boolean allowmsg;
{
    static NEARDATA long ate_brains = 0L;
    struct permonst *fptr = &mons[pm]; /* food type */

    /* when poly'd into a mind flayer, multiple tentacle hits in one
       turn cause multiple digestion checks to occur; avoid giving
       multiple luck penalties for the same attack */
    if (moves == ate_brains)
        return FALSE;
    ate_brains = moves; /* ate_anything, not just brains... */

    if (!CANNIBAL_ALLOWED()
        /* non-cannibalistic heroes shouldn't eat own species ever
           and also shouldn't eat current species when polymorphed
           (even if having the form of something which doesn't care
           about cannibalism--hero's innate traits aren't altered) */
        && (your_race(fptr) || (Upolyd && same_race(youmonst.data, fptr)))) {
        if (allowmsg) {
            if (Upolyd && your_race(fptr))
                You("内心深处有一种不好的感觉.");
            You("食同类了!  你会后悔的!");
        }
        HAggravate_monster |= FROMOUTSIDE;
        change_luck(-rn1(4, 2)); /* -5..-2 */
        return TRUE;
    }
    return FALSE;
}

STATIC_OVL void
cprefx(pm)
register int pm;
{
    (void) maybe_cannibal(pm, TRUE);
    if (flesh_petrifies(&mons[pm])) {
        if (!Stone_resistance
            && !(poly_when_stoned(youmonst.data)
                 && polymon(PM_STONE_GOLEM))) {
            Sprintf(killer.name, "品尝%s肉", mons[pm].mname);
            killer.format = KILLED_BY;
            You("变成了石头.");
            done(STONING);
            if (context.victual.piece)
                context.victual.eating = FALSE;
            return; /* lifesaved */
        }
    }

    switch (pm) {
    case PM_LITTLE_DOG:
    case PM_DOG:
    case PM_LARGE_DOG:
    case PM_KITTEN:
    case PM_HOUSECAT:
    case PM_LARGE_CAT:
        /* cannibals are allowed to eat domestic animals without penalty */
        if (!CANNIBAL_ALLOWED()) {
            You_feel("吃%s个很糟糕的主意.", mons[pm].mname);
            HAggravate_monster |= FROMOUTSIDE;
        }
        break;
    case PM_LIZARD:
        if (Stoned)
            fix_petrification();
        break;
    case PM_DEATH:
    case PM_PESTILENCE:
    case PM_FAMINE: {
        pline("吃那个是立即致命的.");
        Sprintf(killer.name, "不明智地吃了%s的身体", mons[pm].mname);
        killer.format = NO_KILLER_PREFIX;
        done(DIED);
        /* life-saving needed to reach here */
        exercise(A_WIS, FALSE);
        /* It so happens that since we know these monsters */
        /* cannot appear in tins, context.victual.piece will always */
        /* be what we want, which is not generally true. */
        if (revive_corpse(context.victual.piece)) {
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
        }
        return;
    }
    case PM_GREEN_SLIME:
        if (!Slimed && !Unchanging && !slimeproof(youmonst.data)) {
            You("感觉不是很好.");
            make_slimed(10L, (char *) 0);
            delayed_killer(SLIMED, KILLED_BY_AN, "");
        }
    /* Fall through */
    default:
        if (acidic(&mons[pm]) && Stoned)
            fix_petrification();
        break;
    }
}

void
fix_petrification()
{
    char buf[BUFSZ];

    if (Hallucination)
        Sprintf(buf, "真可惜-- 你刚刚破坏了一段未来的%s艺术!",
                ACURR(A_CHA) > 15 ? "精湛的" : "");
    else
        Strcpy(buf, "你感觉柔软了!");
    make_stoned(0L, buf, 0, (char *) 0);
}

/*
 * If you add an intrinsic that can be gotten by eating a monster, add it
 * to intrinsic_possible() and givit().  (It must already be in prop.h to
 * be an intrinsic property.)
 * It would be very easy to make the intrinsics not try to give you one
 * that you already had by checking to see if you have it in
 * intrinsic_possible() instead of givit(), but we're not that nice.
 */

/* intrinsic_possible() returns TRUE iff a monster can give an intrinsic. */
STATIC_OVL int
intrinsic_possible(type, ptr)
int type;
register struct permonst *ptr;
{
    int res = 0;

#ifdef DEBUG
#define ifdebugresist(Msg)      \
    do {                        \
        if (res)                \
            debugpline0(Msg);   \
    } while (0)
#else
#define ifdebugresist(Msg) /*empty*/
#endif
    switch (type) {
    case FIRE_RES:
        res = (ptr->mconveys & MR_FIRE) != 0;
        ifdebugresist("can get fire resistance");
        break;
    case SLEEP_RES:
        res = (ptr->mconveys & MR_SLEEP) != 0;
        ifdebugresist("can get sleep resistance");
        break;
    case COLD_RES:
        res = (ptr->mconveys & MR_COLD) != 0;
        ifdebugresist("can get cold resistance");
        break;
    case DISINT_RES:
        res = (ptr->mconveys & MR_DISINT) != 0;
        ifdebugresist("can get disintegration resistance");
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        res = (ptr->mconveys & MR_ELEC) != 0;
        ifdebugresist("can get shock resistance");
        break;
    case POISON_RES:
        res = (ptr->mconveys & MR_POISON) != 0;
        ifdebugresist("can get poison resistance");
        break;
    case TELEPORT:
        res = can_teleport(ptr);
        ifdebugresist("can get teleport");
        break;
    case TELEPORT_CONTROL:
        res = control_teleport(ptr);
        ifdebugresist("can get teleport control");
        break;
    case TELEPAT:
        res = telepathic(ptr);
        ifdebugresist("can get telepathy");
        break;
    default:
        /* res stays 0 */
        break;
    }
#undef ifdebugresist
    return res;
}

/* givit() tries to give you an intrinsic based on the monster's level
 * and what type of intrinsic it is trying to give you.
 */
STATIC_OVL void
givit(type, ptr)
int type;
register struct permonst *ptr;
{
    register int chance;

    debugpline1("Attempting to give intrinsic %d", type);
    /* some intrinsics are easier to get than others */
    switch (type) {
    case POISON_RES:
        if ((ptr == &mons[PM_KILLER_BEE] || ptr == &mons[PM_SCORPION])
            && !rn2(4))
            chance = 1;
        else
            chance = 15;
        break;
    case TELEPORT:
        chance = 10;
        break;
    case TELEPORT_CONTROL:
        chance = 12;
        break;
    case TELEPAT:
        chance = 1;
        break;
    default:
        chance = 15;
        break;
    }

    if (ptr->mlevel <= rn2(chance))
        return; /* failed die roll */

    switch (type) {
    case FIRE_RES:
        debugpline0("Trying to give fire resistance");
        if (!(HFire_resistance & FROMOUTSIDE)) {
            You(Hallucination ? "是chillin'." : "感觉短暂的寒冷.");
            HFire_resistance |= FROMOUTSIDE;
        }
        break;
    case SLEEP_RES:
        debugpline0("Trying to give sleep resistance");
        if (!(HSleep_resistance & FROMOUTSIDE)) {
            You_feel("清醒的.");
            HSleep_resistance |= FROMOUTSIDE;
        }
        break;
    case COLD_RES:
        debugpline0("Trying to give cold resistance");
        if (!(HCold_resistance & FROMOUTSIDE)) {
            You_feel("夸夸其谈.");
            HCold_resistance |= FROMOUTSIDE;
        }
        break;
    case DISINT_RES:
        debugpline0("Trying to give disintegration resistance");
        if (!(HDisint_resistance & FROMOUTSIDE)) {
            You_feel(Hallucination ? "整个在一起, 哈." : "非常结实.");
            HDisint_resistance |= FROMOUTSIDE;
        }
        break;
    case SHOCK_RES: /* shock (electricity) resistance */
        debugpline0("Trying to give shock resistance");
        if (!(HShock_resistance & FROMOUTSIDE)) {
            if (Hallucination)
                You_feel("基于现实的.");
            else
                Your("健康现在感觉放大了!");
            HShock_resistance |= FROMOUTSIDE;
        }
        break;
    case POISON_RES:
        debugpline0("Trying to give poison resistance");
        if (!(HPoison_resistance & FROMOUTSIDE)) {
            You_feel(Poison_resistance ? "格外的健康." : "健康的.");
            HPoison_resistance |= FROMOUTSIDE;
        }
        break;
    case TELEPORT:
        debugpline0("Trying to give teleport");
        if (!(HTeleportation & FROMOUTSIDE)) {
            You_feel(Hallucination ? "弥漫的." : "很神经兮兮的.");
            HTeleportation |= FROMOUTSIDE;
        }
        break;
    case TELEPORT_CONTROL:
        debugpline0("Trying to give teleport control");
        if (!(HTeleport_control & FROMOUTSIDE)) {
            You_feel(Hallucination ? "集中在你的个人空间."
                                   : "在控制你自己.");
            HTeleport_control |= FROMOUTSIDE;
        }
        break;
    case TELEPAT:
        debugpline0("Trying to give telepathy");
        if (!(HTelepat & FROMOUTSIDE)) {
            You_feel(Hallucination ? "接触到了宇宙."
                                   : "奇怪的精神敏锐.");
            HTelepat |= FROMOUTSIDE;
            /* If blind, make sure monsters show up. */
            if (Blind)
                see_monsters();
        }
        break;
    default:
        debugpline0("Tried to give an impossible intrinsic");
        break;
    }
}

/* called after completely consuming a corpse */
STATIC_OVL void
cpostfx(pm)
register int pm;
{
    register int tmp = 0;
    boolean catch_lycanthropy = FALSE;

    /* in case `afternmv' didn't get called for previously mimicking
       gold, clean up now to avoid `eatmbuf' memory leak */
    if (eatmbuf)
        (void) eatmdone();

    switch (pm) {
    case PM_NEWT:
        /* MRKR: "eye of newt" may give small magical energy boost */
        if (rn2(3) || 3 * u.uen <= 2 * u.uenmax) {
            int old_uen = u.uen;
            u.uen += rnd(3);
            if (u.uen > u.uenmax) {
                if (!rn2(3))
                    u.uenmax++;
                u.uen = u.uenmax;
            }
            if (old_uen != u.uen) {
                You_feel("到轻微的嗡嗡声.");
                context.botl = 1;
            }
        }
        break;
    case PM_WRAITH:
        pluslvl(FALSE);
        break;
    case PM_HUMAN_WERERAT:
        catch_lycanthropy = TRUE;
        u.ulycn = PM_WERERAT;
        break;
    case PM_HUMAN_WEREJACKAL:
        catch_lycanthropy = TRUE;
        u.ulycn = PM_WEREJACKAL;
        break;
    case PM_HUMAN_WEREWOLF:
        catch_lycanthropy = TRUE;
        u.ulycn = PM_WEREWOLF;
        break;
    case PM_NURSE:
        if (Upolyd)
            u.mh = u.mhmax;
        else
            u.uhp = u.uhpmax;
        context.botl = 1;
        break;
    case PM_STALKER:
        if (!Invis) {
            set_itimeout(&HInvis, (long) rn1(100, 50));
            if (!Blind && !BInvis)
                self_invis_message();
        } else {
            if (!(HInvis & INTRINSIC))
                You_feel("隐秘的!");
            HInvis |= FROMOUTSIDE;
            HSee_invisible |= FROMOUTSIDE;
        }
        newsym(u.ux, u.uy);
        /*FALLTHRU*/
    case PM_YELLOW_LIGHT:
    case PM_GIANT_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        /*FALLTHRU*/
    case PM_BAT:
        make_stunned((HStun & TIMEOUT) + 30L, FALSE);
        break;
    case PM_GIANT_MIMIC:
        tmp += 10;
        /*FALLTHRU*/
    case PM_LARGE_MIMIC:
        tmp += 20;
        /*FALLTHRU*/
    case PM_SMALL_MIMIC:
        tmp += 20;
        if (youmonst.data->mlet != S_MIMIC && !Unchanging) {
            char buf[BUFSZ];

            u.uconduct.polyselfs++; /* you're changing form */
            You_cant("抵抗模仿%s的诱惑.",
                     Hallucination ? "一个橙子" : "一堆金币");
            /* A pile of gold can't ride. */
            if (u.usteed)
                dismount_steed(DISMOUNT_FELL);
            nomul(-tmp);
            multi_reason = "假装作为一堆金币";
            Sprintf(buf,
                    Hallucination
                       ? "你突然恐惧被剥皮恐惧再次模仿%s!"
                       : "你现在更愿意再次模仿%s.",
                    Upolyd ? youmonst.data->mname : urace.noun);
            eatmbuf = dupstr(buf);
            nomovemsg = eatmbuf;
            afternmv = eatmdone;
            /* ??? what if this was set before? */
            youmonst.m_ap_type = M_AP_OBJECT;
            youmonst.mappearance = Hallucination ? ORANGE : GOLD_PIECE;
            newsym(u.ux, u.uy);
            curs_on_u();
            /* make gold symbol show up now */
            display_nhwindow(WIN_MAP, TRUE);
        }
        break;
    case PM_QUANTUM_MECHANIC:
        Your("速度突然非常不确定!");
        if (HFast & INTRINSIC) {
            HFast &= ~INTRINSIC;
            You("似乎变慢了.");
        } else {
            HFast |= FROMOUTSIDE;
            You("似乎变快了.");
        }
        break;
    case PM_LIZARD:
        if ((HStun & TIMEOUT) > 2)
            make_stunned(2L, FALSE);
        if ((HConfusion & TIMEOUT) > 2)
            make_confused(2L, FALSE);
        break;
    case PM_CHAMELEON:
    case PM_DOPPELGANGER:
    case PM_SANDESTIN: /* moot--they don't leave corpses */
        if (Unchanging) {
            You_feel("暂时的不同了."); /* same as poly trap */
        } else {
            You_feel("会有一种改变.");
            polyself(0);
        }
        break;
    case PM_DISENCHANTER:
        /* picks an intrinsic at random and removes it; there's
           no feedback if hero already lacks the chosen ability */
        debugpline0("using attrcurse to strip an intrinsic");
        attrcurse();
        break;
    case PM_MIND_FLAYER:
    case PM_MASTER_MIND_FLAYER:
        if (ABASE(A_INT) < ATTRMAX(A_INT)) {
            if (!rn2(2)) {
                pline("很好! 这是真正的大脑食物!");
                (void) adjattrib(A_INT, 1, FALSE);
                break; /* don't give them telepathy, too */
            }
        } else {
            pline("出于某些原因, 那个尝起来清淡.");
        }
    /*FALLTHRU*/
    default: {
        struct permonst *ptr = &mons[pm];
        boolean conveys_STR = is_giant(ptr);
        int i, count;

        if (dmgtype(ptr, AD_STUN) || dmgtype(ptr, AD_HALU)
            || pm == PM_VIOLET_FUNGUS) {
            pline("哦哇!  好东西!");
            (void) make_hallucinated((HHallucination & TIMEOUT) + 200L, FALSE,
                                     0L);
        }

        /* Check the monster for all of the intrinsics.  If this
         * monster can give more than one, pick one to try to give
         * from among all it can give.
         *
         * Strength from giants is now treated like an intrinsic
         * rather than being given unconditionally.
         */
        count = 0; /* number of possible intrinsics */
        tmp = 0;   /* which one we will try to give */
        if (conveys_STR) {
            count = 1;
            tmp = -1; /* use -1 as fake prop index for STR */
            debugpline1("\"Intrinsic\" strength, %d", tmp);
        }
        for (i = 1; i <= LAST_PROP; i++) {
            if (!intrinsic_possible(i, ptr))
                continue;
            ++count;
            /* a 1 in count chance of replacing the old choice
               with this one, and a count-1 in count chance
               of keeping the old choice (note that 1 in 1 and
               0 in 1 are what we want for the first candidate) */
            if (!rn2(count)) {
                debugpline2("Intrinsic %d replacing %d", i, tmp);
                tmp = i;
            }
        }
        /* if strength is the only candidate, give it 50% chance */
        if (conveys_STR && count == 1 && !rn2(2))
            tmp = 0;
        /* if something was chosen, give it now (givit() might fail) */
        if (tmp == -1)
            gainstr((struct obj *) 0, 0, TRUE);
        else if (tmp > 0)
            givit(tmp, ptr);
    } break;
    }

    if (catch_lycanthropy)
        retouch_equipment(2);

    return;
}

void
violated_vegetarian()
{
    u.uconduct.unvegetarian++;
    if (Role_if(PM_MONK)) {
        You_feel("有罪的.");
        adjalign(-1);
    }
    return;
}

/* common code to check and possibly charge for 1 context.tin.tin,
 * will split() context.tin.tin if necessary */
STATIC_PTR void
costly_tin(alter_type)
int alter_type; /* COST_xxx */
{
    struct obj *tin = context.tin.tin;

    if (carried(tin) ? tin->unpaid
                     : (costly_spot(tin->ox, tin->oy) && !tin->no_charge)) {
        if (tin->quan > 1L) {
            tin = context.tin.tin = splitobj(tin, 1L);
            context.tin.o_id = tin->o_id;
        }
        costly_alteration(tin, alter_type);
    }
}

int
tin_variety_txt(s, tinvariety)
char *s;
int *tinvariety;
{
    int k, l;

    if (s && tinvariety) {
        *tinvariety = -1;
        for (k = 0; k < TTSZ - 1; ++k) {
            l = (int) strlen(tintxts[k].txt);
            if (!strncmpi(s, tintxts[k].txt, l) && ((int) strlen(s) > l)
                && s[l] == ' ') {
                *tinvariety = k;
                return (l + 1);
            }
        }
    }
    return 0;
}

/*
 * This assumes that buf already contains the word "tin",
 * as is the case with caller xname().
 */
void
tin_details(obj, mnum, buf)
struct obj *obj;
int mnum;
char *buf;
{
    char buf2[BUFSZ];
    int r = tin_variety(obj, TRUE);

    if (obj && buf) {
        if (r == SPINACH_TIN)
            Strcat(buf, " 之菠菜");
        else if (mnum == NON_PM)
            Strcpy(buf, "空的罐头");
        else {
            if ((obj->cknown || iflags.override_ID) && obj->spe < 0) {
                if (r == ROTTEN_TIN || r == HOMEMADE_TIN) {
                    /* put these before the word tin */
                    Sprintf(buf2, "%s %s 之 ", tintxts[r].txt, buf);
                    Strcpy(buf, buf2);
                } else {
                    Sprintf(eos(buf), " 之 %s ", tintxts[r].txt);
                }
            } else {
                Strcpy(eos(buf), " 之 ");
            }
            if (vegetarian(&mons[mnum]))
                Sprintf(eos(buf), "%s", mons[mnum].mname);
            else
                Sprintf(eos(buf), "%s 肉", mons[mnum].mname);
        }
    }
}

void
set_tin_variety(obj, forcetype)
struct obj *obj;
int forcetype;
{
    register int r;

    if (forcetype == SPINACH_TIN
        || (forcetype == HEALTHY_TIN
            && (obj->corpsenm == NON_PM /* empty or already spinach */
                || !vegetarian(&mons[obj->corpsenm])))) { /* replace meat */
        obj->corpsenm = NON_PM; /* not based on any monster */
        obj->spe = 1;           /* spinach */
        return;
    } else if (forcetype == HEALTHY_TIN) {
        r = tin_variety(obj, FALSE);
        if (r < 0 || r >= TTSZ)
            r = ROTTEN_TIN; /* shouldn't happen */
        while ((r == ROTTEN_TIN && !obj->cursed) || !tintxts[r].fodder)
            r = rn2(TTSZ - 1);
    } else if (forcetype >= 0 && forcetype < TTSZ - 1) {
        r = forcetype;
    } else {               /* RANDOM_TIN */
        r = rn2(TTSZ - 1); /* take your pick */
        if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
            r = HOMEMADE_TIN; /* lizards don't rot */
    }
    obj->spe = -(r + 1); /* offset by 1 to allow index 0 */
}

STATIC_OVL int
tin_variety(obj, disp)
struct obj *obj;
boolean disp; /* we're just displaying so leave things alone */
{
    register int r;

    if (obj->spe == 1) {
        r = SPINACH_TIN;
    } else if (obj->cursed) {
        r = ROTTEN_TIN; /* always rotten if cursed */
    } else if (obj->spe < 0) {
        r = -(obj->spe);
        --r; /* get rid of the offset */
    } else
        r = rn2(TTSZ - 1);

    if (!disp && r == HOMEMADE_TIN && !obj->blessed && !rn2(7))
        r = ROTTEN_TIN; /* some homemade tins go bad */

    if (r == ROTTEN_TIN && nonrotting_corpse(obj->corpsenm))
        r = HOMEMADE_TIN; /* lizards don't rot */
    return r;
}

STATIC_OVL void
consume_tin(mesg)
const char *mesg;
{
    const char *what;
    int which, mnum, r;
    struct obj *tin = context.tin.tin;

    r = tin_variety(tin, FALSE);
    if (tin->otrapped || (tin->cursed && r != HOMEMADE_TIN && !rn2(8))) {
        b_trapped("罐头", 0);
        costly_tin(COST_DSTROY);
        goto use_up_tin;
    }

    pline1(mesg); /* "You succeed in opening the tin." */

    if (r != SPINACH_TIN) {
        mnum = tin->corpsenm;
        if (mnum == NON_PM) {
            pline("原来是空的.");
            tin->dknown = tin->known = 1;
            costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        which = 0; /* 0=>plural, 1=>as-is, 2=>"the" prefix */
        if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
            && (Stone_resistance || Hallucination)) {
            what = "小鸡";
            which = 1; /* suppress pluralization */
        } else if (Hallucination) {
            what = rndmonnam(NULL);
        } else {
            what = mons[mnum].mname;
            if (the_unique_pm(&mons[mnum]))
                which = 2;
            else if (type_is_pname(&mons[mnum]))
                which = 1;
        }
        if (which == 0)
            what = makeplural(what);
        else if (which == 2)
            what = the(what);

        pline("它闻起来像是%s.", what);
        if (yn("吃了它?") == 'n') {
            if (flags.verbose)
                You("丢弃了打开的罐头.");
            if (!Hallucination)
                tin->dknown = tin->known = 1;
            costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /* in case stop_occupation() was called on previous meal */
        context.victual.piece = (struct obj *) 0;
        context.victual.o_id = 0;
        context.victual.fullwarn = context.victual.eating =
            context.victual.doreset = FALSE;

        You("吃光了%s %s.", tintxts[r].txt, mons[mnum].mname);

        eating_conducts(&mons[mnum]);

        tin->dknown = tin->known = 1;
        cprefx(mnum);
        cpostfx(mnum);

        /* charge for one at pre-eating cost */
        costly_tin(COST_OPEN);

        if (tintxts[r].nut < 0) /* rotten */
            make_vomiting((long) rn1(15, 10), FALSE);
        else
            lesshungry(tintxts[r].nut);

        if (tintxts[r].greasy) {
            /* Assume !Glib, because you can't open tins when Glib. */
            incr_itimeout(&Glib, rnd(15));
            pline("吃%s 食物使你的%s变得非常滑.",
                  tintxts[r].txt, makeplural(body_part(FINGER)));
        }

    } else { /* spinach... */
        if (tin->cursed) {
            pline("里面有一些腐烂的%s%s物质.",
                  Blind ? "" : " ", Blind ? "" : hcolor(NH_GREEN));
        } else {
            pline("里面有菠菜.");
            tin->dknown = tin->known = 1;
        }

        if (yn("吃了它?") == 'n') {
            if (flags.verbose)
                You("丢弃了打开的罐头.");
            costly_tin(COST_OPEN);
            goto use_up_tin;
        }

        /*
         * Same order as with non-spinach above:
         * conduct update, side-effects, shop handling, and nutrition.
         */
        u.uconduct
            .food++; /* don't need vegan/vegetarian checks for spinach */
        if (!tin->cursed)
            pline("这让你感觉像是%s!",
                  Hallucination ? "小豆子" : "大力水手");
        gainstr(tin, 0, FALSE);

        costly_tin(COST_OPEN);

        lesshungry(tin->blessed
                      ? 600                   /* blessed */
                      : !tin->cursed
                         ? (400 + rnd(200))   /* uncursed */
                         : (200 + rnd(400))); /* cursed */
    }

use_up_tin:
    if (carried(tin))
        useup(tin);
    else
        useupf(tin, 1L);
    context.tin.tin = (struct obj *) 0;
    context.tin.o_id = 0;
}

/* called during each move whilst opening a tin */
STATIC_PTR int
opentin(VOID_ARGS)
{
    /* perhaps it was stolen (although that should cause interruption) */
    if (!carried(context.tin.tin)
        && (!obj_here(context.tin.tin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return 0; /* %% probably we should use tinoid */
    if (context.tin.usedtime++ >= 50) {
        You("放弃尝试打开罐头.");
        return 0;
    }
    if (context.tin.usedtime < context.tin.reqtime)
        return 1; /* still busy */

    consume_tin("你成功打开了罐头.");
    return 0;
}

/* called when starting to open a tin */
STATIC_OVL void
start_tin(otmp)
struct obj *otmp;
{
    const char *mesg = 0;
    register int tmp;

    if (metallivorous(youmonst.data)) {
        mesg = "你完全咬入金属罐头...";
        tmp = 0;
    } else if (cantwield(youmonst.data)) { /* nohands || verysmall */
        You("不能正确地拿着罐头来打开.");
        return;
    } else if (otmp->blessed) {
        /* 50/50 chance for immediate access vs 1 turn delay (unless
           wielding blessed tin opener which always yields immediate
           access); 1 turn delay case is non-deterministic:  getting
           interrupted and retrying might yield another 1 turn delay
           or might open immediately on 2nd (or 3rd, 4th, ...) try */
        tmp = (uwep && uwep->blessed && uwep->otyp == TIN_OPENER) ? 0 : rn2(2);
        if (!tmp)
            mesg = "这个罐头打开得像魔法一样!";
        else
            pline_The("罐头似乎很容易打开.");
    } else if (uwep) {
        switch (uwep->otyp) {
        case TIN_OPENER:
            mesg = "你轻松地打开了罐头."; /* iff tmp==0 */
            tmp = rn2(uwep->cursed ? 3 : !uwep->blessed ? 2 : 1);
            break;
        case DAGGER:
        case SILVER_DAGGER:
        case ELVEN_DAGGER:
        case ORCISH_DAGGER:
        case ATHAME:
        case CRYSKNIFE:
            tmp = 3;
            break;
        case PICK_AXE:
        case AXE:
            tmp = 6;
            break;
        default:
            goto no_opener;
        }
        pline("你试图使用%s来打开罐头.", yobjnam(uwep, (char *) 0));
    } else {
    no_opener:
        pline("打开这个罐头不是那么容易的.");
        if (Glib) {
            pline_The("罐头从你的%s滑落了.",
                      makeplural(body_part(FINGER)));
            if (otmp->quan > 1L) {
                otmp = splitobj(otmp, 1L);
            }
            if (carried(otmp))
                dropx(otmp);
            else
                stackobj(otmp);
            return;
        }
        tmp = rn1(1 + 500 / ((int) (ACURR(A_DEX) + ACURRSTR)), 10);
    }

    context.tin.tin = otmp;
    context.tin.o_id = otmp->o_id;
    if (!tmp) {
        consume_tin(mesg); /* begin immediately */
    } else {
        context.tin.reqtime = tmp;
        context.tin.usedtime = 0;
        set_occupation(opentin, "开启罐头", 0);
    }
    return;
}

/* called when waking up after fainting */
int
Hear_again(VOID_ARGS)
{
    /* Chance of deafness going away while fainted/sleeping/etc. */
    if (!rn2(2))
        make_deaf(0L, FALSE);
    return 0;
}

/* called on the "first bite" of rotten food */
STATIC_OVL int
rottenfood(obj)
struct obj *obj;
{
    pline("呸!  腐烂的%s!", foodword(obj));
    if (!rn2(4)) {
        if (Hallucination)
            You_feel("相当迷幻.");
        else
            You_feel("相当%s.", body_part(LIGHT_HEADED));
        make_confused(HConfusion + d(2, 4), FALSE);
    } else if (!rn2(4) && !Blind) {
        pline("一切突然变黑.");
        make_blinded((long) d(2, 10), FALSE);
        if (!Blind)
            Your1(vision_clears);
    } else if (!rn2(3)) {
        const char *what, *where;
        int duration = rnd(10);

        if (!Blind)
            what = "变", where = "黑";
        else if (Levitation || Is_airlevel(&u.uz) || Is_waterlevel(&u.uz))
            what = "你失去了自我", where = "控制";
        else
            what = "你拍打",
            where = (u.usteed) ? "鞍" : surface(u.ux, u.uy);
        pline_The("世界旋转并且%s %s.", what, where);
        incr_itimeout(&HDeaf, duration);
        nomul(-duration);
        multi_reason = "因腐烂的事物失去意识";
        nomovemsg = "你再次清醒了.";
        afternmv = Hear_again;
        return 1;
    }
    return 0;
}

/* called when a corpse is selected as food */
STATIC_OVL int
eatcorpse(otmp)
struct obj *otmp;
{
    int tp = 0, mnum = otmp->corpsenm;
    long rotted = 0L;
    int retcode = 0;
    boolean stoneable = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                         && !poly_when_stoned(youmonst.data));

    /* KMH, conduct */
    if (!vegan(&mons[mnum]))
        u.uconduct.unvegan++;
    if (!vegetarian(&mons[mnum]))
        violated_vegetarian();

    if (!nonrotting_corpse(mnum)) {
        long age = peek_at_iced_corpse_age(otmp);

        rotted = (monstermoves - age) / (10L + rn2(20));
        if (otmp->cursed)
            rotted += 2L;
        else if (otmp->blessed)
            rotted -= 2L;
    }

    if (mnum != PM_ACID_BLOB && !stoneable && rotted > 5L) {
        boolean cannibal = maybe_cannibal(mnum, FALSE);

        pline("嗷 -  那个%s被感染了%s!",
              mons[mnum].mlet == S_FUNGUS
                  ? "菌类植物"
                  : !vegetarian(&mons[mnum]) ? "肉" : "原生质",
              cannibal ? ", 你食同类了" : "");
        if (Sick_resistance) {
            pline("它看起来一点也不恶心, 可是...");
        } else {
            long sick_time;

            sick_time = (long) rn1(10, 10);
            /* make sure new ill doesn't result in improvement */
            if (Sick && (sick_time > Sick))
                sick_time = (Sick > 1L) ? Sick - 1L : 1L;
            make_sick(sick_time, corpse_xname(otmp, "腐烂的", CXN_NORMAL),
                      TRUE, SICK_VOMITABLE);
        }
        if (carried(otmp))
            useup(otmp);
        else
            useupf(otmp, 1L);
        return 2;
    } else if (acidic(&mons[mnum]) && !Acid_resistance) {
        tp++;
        You("的胃酸进入了一种非常糟糕了情况.");   /* not body_part() */
        losehp(rnd(15), "酸性的尸体", KILLED_BY_AN); /* acid damage */
    } else if (poisonous(&mons[mnum]) && rn2(5)) {
        tp++;
        pline("额 -  那一定有毒!");
        if (!Poison_resistance) {
            losestr(rnd(4));
            losehp(rnd(15), "有毒的尸体", KILLED_BY_AN);
        } else
            You("似乎不受毒的影响.");
        /* now any corpse left too long will make you mildly ill */
    } else if ((rotted > 5L || (rotted > 3L && rn2(5))) && !Sick_resistance) {
        tp++;
        You_feel("%s不健康.", (Sick) ? "非常 " : "");
        losehp(rnd(8), "死尸", KILLED_BY_AN);
    }

    /* delay is weight dependent */
    context.victual.reqtime = 3 + (mons[mnum].cwt >> 6);

    if (!tp && !nonrotting_corpse(mnum) && (otmp->orotten || !rn2(7))) {
        if (rottenfood(otmp)) {
            otmp->orotten = TRUE;
            (void) touchfood(otmp);
            retcode = 1;
        }

        if (!mons[otmp->corpsenm].cnutrit) {
            /* no nutrition: rots away, no message if you passed out */
            if (!retcode)
                pline_The("尸体完全腐烂了.");
            if (carried(otmp))
                useup(otmp);
            else
                useupf(otmp, 1L);
            retcode = 2;
        }

        if (!retcode)
            consume_oeaten(otmp, 2); /* oeaten >>= 2 */
    } else if ((mnum == PM_COCKATRICE || mnum == PM_CHICKATRICE)
               && (Stone_resistance || Hallucination)) {
        pline("这尝起来像鸡肉!");
    } else if (mnum == PM_FLOATING_EYE && u.umonnum == PM_RAVEN) {
        You("高兴地啄眼球.");
    } else {
        /* [is this right?  omnivores end up always disliking the taste] */
        boolean yummy = vegan(&mons[mnum])
                           ? (!carnivorous(youmonst.data)
                              && herbivorous(youmonst.data))
                           : (carnivorous(youmonst.data)
                              && !herbivorous(youmonst.data));

        pline("%s%s %s!",
              type_is_pname(&mons[mnum])
                 ? "" : the_unique_pm(&mons[mnum]) ? "" : "这个",
              food_xname(otmp, FALSE),
              Hallucination
                  ? (yummy ? ((u.umonnum == PM_TIGER) ? "是棒棒哒"
                                                      : "是粗糙的")
                           : "是恶劣的")
                  : (yummy ? "是美味的" : "尝起来很难吃"));
    }

    return retcode;
}

/* called as you start to eat */
STATIC_OVL void
start_eating(otmp)
struct obj *otmp;
{
    const char *old_nomovemsg, *save_nomovemsg;

    debugpline2("start_eating: %lx (victual = %lx)", (unsigned long) otmp,
                (unsigned long) context.victual.piece);
    debugpline1("reqtime = %d", context.victual.reqtime);
    debugpline1("(original reqtime = %d)", objects[otmp->otyp].oc_delay);
    debugpline1("nmod = %d", context.victual.nmod);
    debugpline1("oeaten = %d", otmp->oeaten);
    context.victual.fullwarn = context.victual.doreset = FALSE;
    context.victual.eating = TRUE;

    if (otmp->otyp == CORPSE || otmp->globby) {
        cprefx(context.victual.piece->corpsenm);
        if (!context.victual.piece || !context.victual.eating) {
            /* rider revived, or died and lifesaved */
            return;
        }
    }

    old_nomovemsg = nomovemsg;
    if (bite()) {
        /* survived choking, finish off food that's nearly done;
           need this to handle cockatrice eggs, fortune cookies, etc */
        if (++context.victual.usedtime >= context.victual.reqtime) {
            /* don't want done_eating() to issue nomovemsg if it
               is due to vomit() called by bite() */
            save_nomovemsg = nomovemsg;
            if (!old_nomovemsg)
                nomovemsg = 0;
            done_eating(FALSE);
            if (!old_nomovemsg)
                nomovemsg = save_nomovemsg;
        }
        return;
    }

    if (++context.victual.usedtime >= context.victual.reqtime) {
        /* print "finish eating" message if they just resumed -dlc */
        done_eating(context.victual.reqtime > 1 ? TRUE : FALSE);
        return;
    }

    Sprintf(msgbuf, "吃 %s", food_xname(otmp, TRUE));
    set_occupation(eatfood, msgbuf, 0);
}

/*
 * called on "first bite" of (non-corpse) food.
 * used for non-rotten non-tin non-corpse food
 */
STATIC_OVL void
fprefx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case FOOD_RATION:
        if (u.uhunger <= 200)
            pline(Hallucination ? "哇, 像, 君, 子!"
                                : "那个食物正令人满意!");
        else if (u.uhunger <= 700)
            pline("那个填饱了你的%s!", body_part(STOMACH));
        break;
    case TRIPE_RATION:
        if (carnivorous(youmonst.data) && !humanoid(youmonst.data))
            pline("那牛肚出奇的好吃!");
        else if (maybe_polyd(is_orc(youmonst.data), Race_if(PM_ORC)))
            pline(Hallucination ? "味道好极了! 没有缺陷!"
                                : "嗯, 牛肚...  不错!");
        else {
            pline("呸 -  狗粮!");
            more_experienced(1, 0);
            newexplevel();
            /* not cannibalism, but we use similar criteria
               for deciding whether to be sickened by this meal */
            if (rn2(2) && !CANNIBAL_ALLOWED())
                make_vomiting((long) rn1(context.victual.reqtime, 14), FALSE);
        }
        break;
    case MEATBALL:
    case MEAT_STICK:
    case HUGE_CHUNK_OF_MEAT:
    case MEAT_RING:
        goto give_feedback;
    case CLOVE_OF_GARLIC:
        if (is_undead(youmonst.data)) {
            make_vomiting((long) rn1(context.victual.reqtime, 5), FALSE);
            break;
        }
        /* else FALLTHRU */
    default:
        if (otmp->otyp == SLIME_MOLD && !otmp->cursed
            && otmp->spe == context.current_fruit) {
            pline("哎呀, 那真是%s %s!",
                  Hallucination ? "一流的" : "好吃的",
                  singular(otmp, xname));
        } else if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
            ; /* skip core joke; feedback deferred til fpostfx() */

#if defined(MAC) || defined(MACOSX)
        /* KMH -- Why should Unix have all the fun?
           We check MACOSX before UNIX to get the Apple-specific apple
           message; the '#if UNIX' code will still kick in for pear. */
        } else if (otmp->otyp == APPLE) {
            pline("Delicious!  Must be a Macintosh!");
#endif

#ifdef UNIX
        } else if (otmp->otyp == APPLE || otmp->otyp == PEAR) {
            if (!Hallucination) {
                pline("信息转储.");
            } else {
                /* This is based on an old Usenet joke, a fake a.out manual
                 * page
                 */
                int x = rnd(100);

                pline("%s --  信息转储.",
                      (x <= 75)
                         ? "分段错误"
                         : (x <= 99)
                            ? "总线错误"
                            : "Yo' mama");
            }
#endif
        } else if (otmp->otyp == EGG && stale_egg(otmp)) {
            pline("呸.  臭蛋."); /* perhaps others like it */
            make_vomiting((Vomiting & TIMEOUT) + (long) d(10, 4), TRUE);
        } else {
        give_feedback:
            pline("这个%s是%s", singular(otmp, xname),
                  otmp->cursed
                     ? (Hallucination ? "恶劣的!" : "很糟的!")
                     : (otmp->otyp == CRAM_RATION
                        || otmp->otyp == K_RATION
                        || otmp->otyp == C_RATION)
                        ? "无味的."
                        : Hallucination ? "粗糙的!" : "美味的!");
        }
        break; /* default */
    } /* switch */
}

/* increment a combat intrinsic with limits on its growth */
STATIC_OVL int
bounded_increase(old, inc, typ)
int old, inc, typ;
{
    int absold, absinc, sgnold, sgninc;

    /* don't include any amount coming from worn rings */
    if (uright && uright->otyp == typ)
        old -= uright->spe;
    if (uleft && uleft->otyp == typ)
        old -= uleft->spe;
    absold = abs(old), absinc = abs(inc);
    sgnold = sgn(old), sgninc = sgn(inc);

    if (absinc == 0 || sgnold != sgninc || absold + absinc < 10) {
        ; /* use inc as-is */
    } else if (absold + absinc < 20) {
        absinc = rnd(absinc); /* 1..n */
        if (absold + absinc < 10)
            absinc = 10 - absold;
        inc = sgninc * absinc;
    } else if (absold + absinc < 40) {
        absinc = rn2(absinc) ? 1 : 0;
        if (absold + absinc < 20)
            absinc = rnd(20 - absold);
        inc = sgninc * absinc;
    } else {
        inc = 0; /* no further increase allowed via this method */
    }
    return old + inc;
}

STATIC_OVL void
accessory_has_effect(otmp)
struct obj *otmp;
{
    pline("当你消化%s时魔力在你的身体里到处传递.",
          otmp->oclass == RING_CLASS ? "戒指" : "护身符");
}

STATIC_OVL void
eataccessory(otmp)
struct obj *otmp;
{
    int typ = otmp->otyp;
    long oldprop;

    /* Note: rings are not so common that this is unbalancing. */
    /* (How often do you even _find_ 3 rings of polymorph in a game?) */
    oldprop = u.uprops[objects[typ].oc_oprop].intrinsic;
    if (otmp == uleft || otmp == uright) {
        Ring_gone(otmp);
        if (u.uhp <= 0)
            return; /* died from sink fall */
    }
    otmp->known = otmp->dknown = 1; /* by taste */
    if (!rn2(otmp->oclass == RING_CLASS ? 3 : 5)) {
        switch (otmp->otyp) {
        default:
            if (!objects[typ].oc_oprop)
                break; /* should never happen */

            if (!(u.uprops[objects[typ].oc_oprop].intrinsic & FROMOUTSIDE))
                accessory_has_effect(otmp);

            u.uprops[objects[typ].oc_oprop].intrinsic |= FROMOUTSIDE;

            switch (typ) {
            case RIN_SEE_INVISIBLE:
                set_mimic_blocking();
                see_monsters();
                if (Invis && !oldprop && !ESee_invisible
                    && !perceives(youmonst.data) && !Blind) {
                    newsym(u.ux, u.uy);
                    pline("突然你能看见自己了.");
                    makeknown(typ);
                }
                break;
            case RIN_INVISIBILITY:
                if (!oldprop && !EInvis && !BInvis && !See_invisible
                    && !Blind) {
                    newsym(u.ux, u.uy);
                    Your("身体呈现出%s 透明...",
                         Hallucination ? "一般的" : "奇怪的");
                    makeknown(typ);
                }
                break;
            case RIN_PROTECTION_FROM_SHAPE_CHAN:
                rescham();
                break;
            case RIN_LEVITATION:
                /* undo the `.intrinsic |= FROMOUTSIDE' done above */
                u.uprops[LEVITATION].intrinsic = oldprop;
                if (!Levitation) {
                    float_up();
                    incr_itimeout(&HLevitation, d(10, 20));
                    makeknown(typ);
                }
                break;
            } /* inner switch */
            break; /* default case of outer switch */

        case RIN_ADORNMENT:
            accessory_has_effect(otmp);
            if (adjattrib(A_CHA, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_STRENGTH:
            accessory_has_effect(otmp);
            if (adjattrib(A_STR, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_GAIN_CONSTITUTION:
            accessory_has_effect(otmp);
            if (adjattrib(A_CON, otmp->spe, -1))
                makeknown(typ);
            break;
        case RIN_INCREASE_ACCURACY:
            accessory_has_effect(otmp);
            u.uhitinc = (schar) bounded_increase((int) u.uhitinc, otmp->spe,
                                                 RIN_INCREASE_ACCURACY);
            break;
        case RIN_INCREASE_DAMAGE:
            accessory_has_effect(otmp);
            u.udaminc = (schar) bounded_increase((int) u.udaminc, otmp->spe,
                                                 RIN_INCREASE_DAMAGE);
            break;
        case RIN_PROTECTION:
            accessory_has_effect(otmp);
            HProtection |= FROMOUTSIDE;
            u.ublessed = bounded_increase(u.ublessed, otmp->spe,
                                          RIN_PROTECTION);
            context.botl = 1;
            break;
        case RIN_FREE_ACTION:
            /* Give sleep resistance instead */
            if (!(HSleep_resistance & FROMOUTSIDE))
                accessory_has_effect(otmp);
            if (!Sleep_resistance)
                You_feel("清醒的.");
            HSleep_resistance |= FROMOUTSIDE;
            break;
        case AMULET_OF_CHANGE:
            accessory_has_effect(otmp);
            makeknown(typ);
            change_sex();
            You("突然非常%s!",
                flags.female ? "女性化" : "男性化");
            context.botl = 1;
            break;
        case AMULET_OF_UNCHANGING:
            /* un-change: it's a pun */
            if (!Unchanging && Upolyd) {
                accessory_has_effect(otmp);
                makeknown(typ);
                rehumanize();
            }
            break;
        case AMULET_OF_STRANGULATION: /* bad idea! */
            /* no message--this gives no permanent effect */
            choke(otmp);
            break;
        case AMULET_OF_RESTFUL_SLEEP: { /* another bad idea! */
            long newnap = (long) rnd(100), oldnap = (HSleepy & TIMEOUT);

            if (!(HSleepy & FROMOUTSIDE))
                accessory_has_effect(otmp);
            HSleepy |= FROMOUTSIDE;
            /* might also be wearing one; use shorter of two timeouts */
            if (newnap < oldnap || oldnap == 0L)
                HSleepy = (HSleepy & ~TIMEOUT) | newnap;
            break;
        }
        case RIN_SUSTAIN_ABILITY:
        case AMULET_OF_LIFE_SAVING:
        case AMULET_OF_REFLECTION: /* nice try */
            /* can't eat Amulet of Yendor or fakes,
             * and no oc_prop even if you could -3.
             */
            break;
        }
    }
}

/* called after eating non-food */
STATIC_OVL void
eatspecial()
{
    struct obj *otmp = context.victual.piece;

    /* lesshungry wants an occupation to handle choke messages correctly */
    set_occupation(eatfood, "吃非食品", 0);
    lesshungry(context.victual.nmod);
    occupation = 0;
    context.victual.piece = (struct obj *) 0;
    context.victual.o_id = 0;
    context.victual.eating = 0;
    if (otmp->oclass == COIN_CLASS) {
        if (carried(otmp))
            useupall(otmp);
        else
            useupf(otmp, otmp->quan);
        vault_gd_watching(GD_EATGOLD);
        return;
    }
#ifdef MAIL
    if (otmp->otyp == SCR_MAIL) {
        /* no nutrition */
        pline("This junk mail is less than satisfying.");
    }
#endif
    if (otmp->oclass == POTION_CLASS) {
        otmp->quan++; /* dopotion() does a useup() */
        (void) dopotion(otmp);
    } else if (otmp->oclass == RING_CLASS || otmp->oclass == AMULET_CLASS) {
        eataccessory(otmp);
    } else if (otmp->otyp == LEASH && otmp->leashmon) {
        o_unleash(otmp);
    }

    /* KMH -- idea by "Tommy the Terrorist" */
    if (otmp->otyp == TRIDENT && !otmp->cursed) {
        /* sugarless chewing gum which used to be heavily advertised on TV */
        pline(Hallucination ? "五个里有四个牙科医生同意."
                            : "那是纯粹的咀嚼满足!");
        exercise(A_WIS, TRUE);
    }
    if (otmp->otyp == FLINT && !otmp->cursed) {
        /* chewable vitamin for kids based on "The Flintstones" TV cartoon */
        pline("Yabba-dabba 美味的!");
        exercise(A_CON, TRUE);
    }

    if (otmp == uwep && otmp->quan == 1L)
        uwepgone();
    if (otmp == uquiver && otmp->quan == 1L)
        uqwepgone();
    if (otmp == uswapwep && otmp->quan == 1L)
        uswapwepgone();

    if (otmp == uball)
        unpunish();
    if (otmp == uchain)
        unpunish(); /* but no useup() */
    else if (carried(otmp))
        useup(otmp);
    else
        useupf(otmp, 1L);
}

/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
static const char *foodwords[] = {
    "一餐",    "液体",  "蜡",       "食物", "肉",     "纸",
    "布",   "皮革", "木头",      "骨头", "鳞片",    "金属",
    "金属",   "金属",   "银",    "金", "白金", "秘银",
    "塑料", "玻璃",   "油腻的食品", "石头"
};

STATIC_OVL const char *
foodword(otmp)
struct obj *otmp;
{
    if (otmp->oclass == FOOD_CLASS)
        return "食物";
    if (otmp->oclass == GEM_CLASS && objects[otmp->otyp].oc_material == GLASS
        && otmp->dknown)
        makeknown(otmp->otyp);
    return foodwords[objects[otmp->otyp].oc_material];
}

/* called after consuming (non-corpse) food */
STATIC_OVL void
fpostfx(otmp)
struct obj *otmp;
{
    switch (otmp->otyp) {
    case SPRIG_OF_WOLFSBANE:
        if (u.ulycn >= LOW_PM || is_were(youmonst.data))
            you_unwere(TRUE);
        break;
    case CARROT:
        if (!u.uswallow
            || !attacktype_fordmg(u.ustuck->data, AT_ENGL, AD_BLND))
            make_blinded((long) u.ucreamed, TRUE);
        break;
    case FORTUNE_COOKIE:
        outrumor(bcsign(otmp), BY_COOKIE);
        if (!Blind)
            u.uconduct.literate++;
        break;
    case LUMP_OF_ROYAL_JELLY:
        /* This stuff seems to be VERY healthy! */
        gainstr(otmp, 1, TRUE);
        if (Upolyd) {
            u.mh += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.mh > u.mhmax) {
                if (!rn2(17))
                    u.mhmax++;
                u.mh = u.mhmax;
            } else if (u.mh <= 0) {
                rehumanize();
            }
        } else {
            u.uhp += otmp->cursed ? -rnd(20) : rnd(20);
            if (u.uhp > u.uhpmax) {
                if (!rn2(17))
                    u.uhpmax++;
                u.uhp = u.uhpmax;
            } else if (u.uhp <= 0) {
                killer.format = KILLED_BY_AN;
                Strcpy(killer.name, "腐烂的蜂王浆");
                done(POISONING);
            }
        }
        if (!otmp->cursed)
            heal_legs();
        break;
    case EGG:
        if (flesh_petrifies(&mons[otmp->corpsenm])) {
            if (!Stone_resistance
                && !(poly_when_stoned(youmonst.data)
                     && polymon(PM_STONE_GOLEM))) {
                if (!Stoned) {
                    Sprintf(killer.name, "%s蛋",
                            mons[otmp->corpsenm].mname);
                    make_stoned(5L, (char *) 0, KILLED_BY_AN, killer.name);
                }
            }
            /* note: no "tastes like chicken" message for eggs */
        }
        break;
    case EUCALYPTUS_LEAF:
        if (Sick && !otmp->cursed)
            make_sick(0L, (char *) 0, TRUE, SICK_ALL);
        if (Vomiting && !otmp->cursed)
            make_vomiting(0L, TRUE);
        break;
    case APPLE:
        if (otmp->cursed && !Sleep_resistance) {
            /* Snow White; 'poisoned' applies to [a subset of] weapons,
               not food, so we substitute cursed; fortunately our hero
               won't have to wait for a prince to be rescued/revived */
            if (Race_if(PM_DWARF) && Hallucination)
                verbalize("嗨, 哼呣, 我想我会跳过今天的工作.");
            else if (Deaf || !flags.acoustics)
                You("陷入沉睡.");
            else
                You_hear("邪恶的笑声在你陷入沉睡的时候...");
            fall_asleep(-rn1(11, 20), TRUE);
        }
        break;
    }
    return;
}

#if 0
/* intended for eating a spellbook while polymorphed, but not used;
   "leather" applied to appearance, not composition, and has been
   changed to "leathery" to reflect that */
STATIC_DCL boolean FDECL(leather_cover, (struct obj *));

STATIC_OVL boolean
leather_cover(otmp)
struct obj *otmp;
{
    const char *odesc = OBJ_DESCR(objects[otmp->otyp]);

    if (odesc && (otmp->oclass == SPBOOK_CLASS)) {
        if (!strcmp(odesc, "leather"))
            return TRUE;
    }
    return FALSE;
}
#endif

/*
 * return 0 if the food was not dangerous.
 * return 1 if the food was dangerous and you chose to stop.
 * return 2 if the food was dangerous and you chose to eat it anyway.
 */
STATIC_OVL int
edibility_prompts(otmp)
struct obj *otmp;
{
    /* Blessed food detection grants hero a one-use
     * ability to detect food that is unfit for consumption
     * or dangerous and avoid it.
     */
    char buf[BUFSZ], foodsmell[BUFSZ],
         it_or_they[QBUFSZ], eat_it_anyway[QBUFSZ];
    boolean cadaver = (otmp->otyp == CORPSE), stoneorslime = FALSE;
    int material = objects[otmp->otyp].oc_material, mnum = otmp->corpsenm;
    long rotted = 0L;

    Strcpy(foodsmell, Tobjnam(otmp, "闻起来"));
    Strcpy(it_or_they, (otmp->quan == 1L) ? "它" : "它们");
    Sprintf(eat_it_anyway, "无论如何都要吃%s?",
            (otmp->quan == 1L) ? "它" : "一个");

    if (cadaver || otmp->otyp == EGG || otmp->otyp == TIN) {
        /* These checks must match those in eatcorpse() */
        stoneorslime = (flesh_petrifies(&mons[mnum]) && !Stone_resistance
                        && !poly_when_stoned(youmonst.data));

        if (mnum == PM_GREEN_SLIME || otmp->otyp == GLOB_OF_GREEN_SLIME)
            stoneorslime = (!Unchanging && !slimeproof(youmonst.data));

        if (cadaver && !nonrotting_corpse(mnum)) {
            long age = peek_at_iced_corpse_age(otmp);
            /* worst case rather than random
               in this calculation to force prompt */
            rotted = (monstermoves - age) / (10L + 0 /* was rn2(20) */);
            if (otmp->cursed)
                rotted += 2L;
            else if (otmp->blessed)
                rotted -= 2L;
        }
    }

    /*
     * These problems with food should be checked in
     * order from most detrimental to least detrimental.
     */
    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && !Sick_resistance) {
        /* Tainted meat */
        Sprintf(buf, "%s像%s可能被感染了! %s", foodsmell, it_or_they,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (stoneorslime) {
        Sprintf(buf, "%s像%s可能会有些非常危险! %s",
                foodsmell, it_or_they, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->orotten || (cadaver && rotted > 3L)) {
        /* Rotten */
        Sprintf(buf, "%s像%s可能会是腐烂! %s", foodsmell, it_or_they,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && poisonous(&mons[mnum]) && !Poison_resistance) {
        /* poisonous */
        Sprintf(buf, "%s像%s可能会有毒! %s", foodsmell,
                it_or_they, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (otmp->otyp == APPLE && otmp->cursed && !Sleep_resistance) {
        /* causes sleep, for long enough to be dangerous */
        Sprintf(buf, "%s像%s可能会已经中毒了. %s", foodsmell,
                it_or_they, eat_it_anyway);
        return (yn_function(buf, ynchars, 'n') == 'n') ? 1 : 2;
    }
    if (cadaver && !vegetarian(&mons[mnum]) && !u.uconduct.unvegetarian
        && Role_if(PM_MONK)) {
        Sprintf(buf, "%s不健康. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (cadaver && acidic(&mons[mnum]) && !Acid_resistance) {
        Sprintf(buf, "%s相当酸的. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (Upolyd && u.umonnum == PM_RUST_MONSTER && is_metallic(otmp)
        && otmp->oerodeproof) {
        Sprintf(buf, "%s使你立马厌恶. %s", foodsmell,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    /*
     * Breaks conduct, but otherwise safe.
     */
    if (!u.uconduct.unvegan
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE || material == WAX)
            || (cadaver && !vegan(&mons[mnum])))) {
        Sprintf(buf, "%s 不正当的你对它不熟悉. %s", foodsmell,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    if (!u.uconduct.unvegetarian
        && ((material == LEATHER || material == BONE
             || material == DRAGON_HIDE)
            || (cadaver && !vegetarian(&mons[mnum])))) {
        Sprintf(buf, "%s 你对它不熟. %s", foodsmell, eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }

    if (cadaver && mnum != PM_ACID_BLOB && rotted > 5L && Sick_resistance) {
        /* Tainted meat with Sick_resistance */
        Sprintf(buf, "%s像%s可能会被感染了! %s", foodsmell, it_or_they,
                eat_it_anyway);
        if (yn_function(buf, ynchars, 'n') == 'n')
            return 1;
        else
            return 2;
    }
    return 0;
}

/* 'e' command */
int
doeat()
{
    struct obj *otmp;
    int basenutrit; /* nutrition of full item */
    boolean dont_start = FALSE, nodelicious = FALSE;

    if (Strangled) {
        pline("如果你不能呼吸空气, 你怎么消化实体?");
        return 0;
    }
    if (!(otmp = floorfood("吃", 0)))  //eat
        return 0;
    if (check_capacity((char *) 0))
        return 0;

    if (u.uedibility) {
        int res = edibility_prompts(otmp);
        if (res) {
            Your(
               "%s 停止了刺痛, 你的嗅觉回到正常.",
                 body_part(NOSE));
            u.uedibility = 0;
            if (res == 1)
                return 0;
        }
    }

    /* We have to make non-foods take 1 move to eat, unless we want to
     * do ridiculous amounts of coding to deal with partly eaten plate
     * mails, players who polymorph back to human in the middle of their
     * metallic meal, etc....
     */
    if (!(carried(otmp) ? retouch_object(&otmp, FALSE)
                        : touch_artifact(otmp, &youmonst))) {
        return 1;
    } else if (!is_edible(otmp)) {
        You("不能吃那个!");
        return 0;
    } else if ((otmp->owornmask & (W_ARMOR | W_TOOL | W_AMUL | W_SADDLE))
               != 0) {
        /* let them eat rings */
        You_cant("吃%s 你正穿戴的.", something);
        return 0;
    }
    if (is_metallic(otmp) && u.umonnum == PM_RUST_MONSTER
        && otmp->oerodeproof) {
        otmp->rknown = TRUE;
        if (otmp->quan > 1L) {
            if (!carried(otmp))
                (void) splitobj(otmp, otmp->quan - 1L);
            else
                otmp = splitobj(otmp, 1L);
        }
        pline("额 -  那个%s是防锈的!", xname(otmp));
        /* The regurgitated object's rustproofing is gone now */
        otmp->oerodeproof = 0;
        make_stunned((HStun & TIMEOUT) + (long) rn2(10), TRUE);
        You("吐出%s到%s.", the(xname(otmp)),
            surface(u.ux, u.uy));
        if (carried(otmp)) {
            freeinv(otmp);
            dropy(otmp);
        }
        stackobj(otmp);
        return 1;
    }
    /* KMH -- Slow digestion is... indigestible */
    if (otmp->otyp == RIN_SLOW_DIGESTION) {
        pline("这个戒指很难消化!");
        (void) rottenfood(otmp);
        if (otmp->dknown && !objects[otmp->otyp].oc_name_known
            && !objects[otmp->otyp].oc_uname)
            docall(otmp);
        return 1;
    }
    if (otmp->oclass != FOOD_CLASS) {
        int material;

        context.victual.reqtime = 1;
        context.victual.piece = otmp;
        context.victual.o_id = otmp->o_id;
        /* Don't split it, we don't need to if it's 1 move */
        context.victual.usedtime = 0;
        context.victual.canchoke = (u.uhs == SATIATED);
        /* Note: gold weighs 1 pt. for each 1000 pieces (see
           pickup.c) so gold and non-gold is consistent. */
        if (otmp->oclass == COIN_CLASS)
            basenutrit = ((otmp->quan > 200000L)
                             ? 2000
                             : (int) (otmp->quan / 100L));
        else if (otmp->oclass == BALL_CLASS || otmp->oclass == CHAIN_CLASS)
            basenutrit = weight(otmp);
        /* oc_nutrition is usually weight anyway */
        else
            basenutrit = objects[otmp->otyp].oc_nutrition;
#ifdef MAIL
        if (otmp->otyp == SCR_MAIL) {
            basenutrit = 0;
            nodelicious = TRUE;
        }
#endif
        context.victual.nmod = basenutrit;
        context.victual.eating = TRUE; /* needed for lesshungry() */

        material = objects[otmp->otyp].oc_material;
        if (material == LEATHER || material == BONE
            || material == DRAGON_HIDE) {
            u.uconduct.unvegan++;
            violated_vegetarian();
        } else if (material == WAX)
            u.uconduct.unvegan++;
        u.uconduct.food++;

        if (otmp->cursed)
            (void) rottenfood(otmp);

        if (otmp->oclass == WEAPON_CLASS && otmp->opoisoned) {
            pline("额 -  那一定有毒!");
            if (!Poison_resistance) {
                losestr(rnd(4));
                losehp(rnd(15), xname(otmp), KILLED_BY_AN);
            } else
                You("似乎不受毒的影响.");
        } else if (!otmp->cursed && !nodelicious) {
            pline("%s%s很美味!",
                  (obj_is_pname(otmp)
                   && otmp->oartifact < ART_ORB_OF_DETECTION)
                      ? ""
                      : "这个 ",
                  (otmp->oclass == COIN_CLASS)
                      ? foodword(otmp)
                      : singular(otmp, xname));
        }
        eatspecial();
        return 1;
    }

    if (otmp == context.victual.piece) {
        /* If they weren't able to choke, they don't suddenly become able to
         * choke just because they were interrupted.  On the other hand, if
         * they were able to choke before, if they lost food it's possible
         * they shouldn't be able to choke now.
         */
        if (u.uhs != SATIATED)
            context.victual.canchoke = FALSE;
        context.victual.o_id = 0;
        context.victual.piece = touchfood(otmp);
        if (context.victual.piece)
            context.victual.o_id = context.victual.piece->o_id;
        You("继续你的进餐.");
        start_eating(context.victual.piece);
        return 1;
    }

    /* nothing in progress - so try to find something. */
    /* tins are a special case */
    /* tins must also check conduct separately in case they're discarded */
    if (otmp->otyp == TIN) {
        start_tin(otmp);
        return 1;
    }

    /* KMH, conduct */
    u.uconduct.food++;

    context.victual.o_id = 0;
    context.victual.piece = otmp = touchfood(otmp);
    if (context.victual.piece)
        context.victual.o_id = context.victual.piece->o_id;
    context.victual.usedtime = 0;

    /* Now we need to calculate delay and nutritional info.
     * The base nutrition calculated here and in eatcorpse() accounts
     * for normal vs. rotten food.  The reqtime and nutrit values are
     * then adjusted in accordance with the amount of food left.
     */
    if (otmp->otyp == CORPSE || otmp->globby) {
        int tmp = eatcorpse(otmp);

        if (tmp == 2) {
            /* used up */
            context.victual.piece = (struct obj *) 0;
            context.victual.o_id = 0;
            return 1;
        } else if (tmp)
            dont_start = TRUE;
        /* if not used up, eatcorpse sets up reqtime and may modify oeaten */
    } else {
        /* No checks for WAX, LEATHER, BONE, DRAGON_HIDE.  These are
         * all handled in the != FOOD_CLASS case, above.
         */
        switch (objects[otmp->otyp].oc_material) {
        case FLESH:
            u.uconduct.unvegan++;
            if (otmp->otyp != EGG) {
                violated_vegetarian();
            }
            break;

        default:
            if (otmp->otyp == PANCAKE || otmp->otyp == FORTUNE_COOKIE /*eggs*/
                || otmp->otyp == CREAM_PIE || otmp->otyp == CANDY_BAR /*milk*/
                || otmp->otyp == LUMP_OF_ROYAL_JELLY)
                u.uconduct.unvegan++;
            break;
        }

        context.victual.reqtime = objects[otmp->otyp].oc_delay;
        if (otmp->otyp != FORTUNE_COOKIE
            && (otmp->cursed || (!nonrotting_food(otmp->otyp)
                                 && (monstermoves - otmp->age)
                                        > (otmp->blessed ? 50L : 30L)
                                 && (otmp->orotten || !rn2(7))))) {
            if (rottenfood(otmp)) {
                otmp->orotten = TRUE;
                dont_start = TRUE;
            }
            consume_oeaten(otmp, 1); /* oeaten >>= 1 */
        } else
            fprefx(otmp);
    }

    /* re-calc the nutrition */
    if (otmp->otyp == CORPSE)
        basenutrit = mons[otmp->corpsenm].cnutrit;
    else
        basenutrit = objects[otmp->otyp].oc_nutrition;

    debugpline1("before rounddiv: context.victual.reqtime == %d",
                context.victual.reqtime);
    debugpline2("oeaten == %d, basenutrit == %d", otmp->oeaten, basenutrit);
    context.victual.reqtime = (basenutrit == 0)
                                 ? 0
                                 : rounddiv(context.victual.reqtime
                                            * (long) otmp->oeaten,
                                            basenutrit);
    debugpline1("after rounddiv: context.victual.reqtime == %d",
                context.victual.reqtime);
    /*
     * calculate the modulo value (nutrit. units per round eating)
     * note: this isn't exact - you actually lose a little nutrition due
     *       to this method.
     * TODO: add in a "remainder" value to be given at the end of the meal.
     */
    if (context.victual.reqtime == 0 || otmp->oeaten == 0)
        /* possible if most has been eaten before */
        context.victual.nmod = 0;
    else if ((int) otmp->oeaten >= context.victual.reqtime)
        context.victual.nmod = -((int) otmp->oeaten
                                 / context.victual.reqtime);
    else
        context.victual.nmod = context.victual.reqtime % otmp->oeaten;
    context.victual.canchoke = (u.uhs == SATIATED);

    if (!dont_start)
        start_eating(otmp);
    return 1;
}

/* Take a single bite from a piece of food, checking for choking and
 * modifying usedtime.  Returns 1 if they choked and survived, 0 otherwise.
 */
STATIC_OVL int
bite()
{
    if (context.victual.canchoke && u.uhunger >= 2000) {
        choke(context.victual.piece);
        return 1;
    }
    if (context.victual.doreset) {
        do_reset_eat();
        return 0;
    }
    force_save_hs = TRUE;
    if (context.victual.nmod < 0) {
        lesshungry(-context.victual.nmod);
        consume_oeaten(context.victual.piece,
                       context.victual.nmod); /* -= -nmod */
    } else if (context.victual.nmod > 0
               && (context.victual.usedtime % context.victual.nmod)) {
        lesshungry(1);
        consume_oeaten(context.victual.piece, -1); /* -= 1 */
    }
    force_save_hs = FALSE;
    recalc_wt();
    return 0;
}

/* as time goes by - called by moveloop() and domove() */
void
gethungry()
{
    if (u.uinvulnerable)
        return; /* you don't feel hungrier */

    if ((!u.usleep || !rn2(10)) /* slow metabolic rate while asleep */
        && (carnivorous(youmonst.data) || herbivorous(youmonst.data))
        && !Slow_digestion)
        u.uhunger--; /* ordinary food consumption */

    if (moves % 2) { /* odd turns */
        /* Regeneration uses up food, unless due to an artifact */
        if ((HRegeneration & ~FROMFORM)
            || (ERegeneration & ~(W_ARTI | W_WEP)))
            u.uhunger--;
        if (near_capacity() > SLT_ENCUMBER)
            u.uhunger--;
    } else { /* even turns */
        if (Hunger)
            u.uhunger--;
        /* Conflict uses up food too */
        if (HConflict || (EConflict & (~W_ARTI)))
            u.uhunger--;
        /* +0 charged rings don't do anything, so don't affect hunger */
        /* Slow digestion still uses ring hunger */
        switch ((int) (moves % 20)) { /* note: use even cases only */
        case 4:
            if (uleft && (uleft->spe || !objects[uleft->otyp].oc_charged))
                u.uhunger--;
            break;
        case 8:
            if (uamul)
                u.uhunger--;
            break;
        case 12:
            if (uright && (uright->spe || !objects[uright->otyp].oc_charged))
                u.uhunger--;
            break;
        case 16:
            if (u.uhave.amulet)
                u.uhunger--;
            break;
        default:
            break;
        }
    }
    newuhs(TRUE);
}

/* called after vomiting and after performing feats of magic */
void
morehungry(num)
int num;
{
    u.uhunger -= num;
    newuhs(TRUE);
}

/* called after eating (and after drinking fruit juice) */
void
lesshungry(num)
int num;
{
    /* See comments in newuhs() for discussion on force_save_hs */
    boolean iseating = (occupation == eatfood) || force_save_hs;

    debugpline1("lesshungry(%d)", num);
    u.uhunger += num;
    if (u.uhunger >= 2000) {
        if (!iseating || context.victual.canchoke) {
            if (iseating) {
                choke(context.victual.piece);
                reset_eat();
            } else
                choke(occupation == opentin ? context.tin.tin
                                            : (struct obj *) 0);
            /* no reset_eat() */
        }
    } else {
        /* Have lesshungry() report when you're nearly full so all eating
         * warns when you're about to choke.
         */
        if (u.uhunger >= 1500) {
            if (!context.victual.eating
                || (context.victual.eating && !context.victual.fullwarn)) {
                pline("你很难把它吃完.");
                nomovemsg = "你最后吃完了.";
                if (!context.victual.eating) {
                    multi = -2;
                } else {
                    context.victual.fullwarn = TRUE;
                    if (context.victual.canchoke
                        && context.victual.reqtime > 1) {
                        /* a one-gulp food will not survive a stop */
                        if (yn_function("继续吃吗?", ynchars, 'n')
                            != 'y') {
                            reset_eat();
                            nomovemsg = (char *) 0;
                        }
                    }
                }
            }
        }
    }
    newuhs(FALSE);
}

STATIC_PTR
int
unfaint(VOID_ARGS)
{
    (void) Hear_again();
    if (u.uhs > FAINTING)
        u.uhs = FAINTING;
    stop_occupation();
    context.botl = 1;
    return 0;
}

boolean
is_fainted()
{
    return (boolean) (u.uhs == FAINTED);
}

/* call when a faint must be prematurely terminated */
void
reset_faint()
{
    if (afternmv == unfaint)
        unmul("你恢复了.");
}

/* compute and comment on your (new?) hunger status */
void
newuhs(incr)
boolean incr;
{
    unsigned newhs;
    static unsigned save_hs;
    static boolean saved_hs = FALSE;
    int h = u.uhunger;

    newhs = (h > 1000)
                ? SATIATED
                : (h > 150) ? NOT_HUNGRY
                            : (h > 50) ? HUNGRY : (h > 0) ? WEAK : FAINTING;

    /* While you're eating, you may pass from WEAK to HUNGRY to NOT_HUNGRY.
     * This should not produce the message "you only feel hungry now";
     * that message should only appear if HUNGRY is an endpoint.  Therefore
     * we check to see if we're in the middle of eating.  If so, we save
     * the first hunger status, and at the end of eating we decide what
     * message to print based on the _entire_ meal, not on each little bit.
     */
    /* It is normally possible to check if you are in the middle of a meal
     * by checking occupation == eatfood, but there is one special case:
     * start_eating() can call bite() for your first bite before it
     * sets the occupation.
     * Anyone who wants to get that case to work _without_ an ugly static
     * force_save_hs variable, feel free.
     */
    /* Note: If you become a certain hunger status in the middle of the
     * meal, and still have that same status at the end of the meal,
     * this will incorrectly print the associated message at the end of
     * the meal instead of the middle.  Such a case is currently
     * impossible, but could become possible if a message for SATIATED
     * were added or if HUNGRY and WEAK were separated by a big enough
     * gap to fit two bites.
     */
    if (occupation == eatfood || force_save_hs) {
        if (!saved_hs) {
            save_hs = u.uhs;
            saved_hs = TRUE;
        }
        u.uhs = newhs;
        return;
    } else {
        if (saved_hs) {
            u.uhs = save_hs;
            saved_hs = FALSE;
        }
    }

    if (newhs == FAINTING) {
        if (is_fainted())
            newhs = FAINTED;
        if (u.uhs <= WEAK || rn2(20 - u.uhunger / 10) >= 19) {
            if (!is_fainted() && multi >= 0 /* %% */) {
                int duration = 10 - (u.uhunger / 10);

                /* stop what you're doing, then faint */
                stop_occupation();
                You("因缺乏食物而昏倒.");
                if (!Levitation)
                    selftouch("掉落, 你");
                incr_itimeout(&HDeaf, duration);
                nomul(-duration);
                multi_reason = "因缺乏食物而昏厥";
                nomovemsg = "你重获了意识.";
                afternmv = unfaint;
                newhs = FAINTED;
            }
        } else if (u.uhunger < -(int) (200 + 20 * ACURR(A_CON))) {
            u.uhs = STARVED;
            context.botl = 1;
            bot();
            You("死于饥饿.");
            killer.format = KILLED_BY;
            Strcpy(killer.name, "饥饿");
            done(STARVING);
            /* if we return, we lifesaved, and that calls newuhs */
            return;
        }
    }

    if (newhs != u.uhs) {
        if (newhs >= WEAK && u.uhs < WEAK)
            losestr(1); /* this may kill you -- see below */
        else if (newhs < WEAK && u.uhs >= WEAK)
            losestr(-1);
        switch (newhs) {
        case HUNGRY:
            if (Hallucination) {
                You((!incr) ? "现在有较小的饥饿感."
                            : "有些饥饿感.");
            } else
                You((!incr) ? "现在只感觉饿."
                            : (u.uhunger < 145)
                                  ? "感觉饿了."
                                  : "开始感觉饿了.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        case WEAK:
            if (Hallucination)
                pline((!incr) ? "你仍然有些饥饿感."
              : "饥饿感影响了你的运动能力.");
            else if (incr && (Role_if(PM_WIZARD) || Race_if(PM_ELF)
                              || Role_if(PM_VALKYRIE)))
                pline("%s 需要食物, 非常需要!",
                      (Role_if(PM_WIZARD) || Role_if(PM_VALKYRIE))
                          ? urole.name.m
                          : "精灵");
            else
                You((!incr)
                        ? "现在感觉虚弱."
                        : (u.uhunger < 45) ? "感觉虚弱."
                                           : "开始感觉虚弱.");
            if (incr && occupation
                && (occupation != eatfood && occupation != opentin))
                stop_occupation();
            context.travel = context.travel1 = context.mv = context.run = 0;
            break;
        }
        u.uhs = newhs;
        context.botl = 1;
        bot();
        if ((Upolyd ? u.mh : u.uhp) < 1) {
            You("死于饥饿和精疲力竭.");
            killer.format = KILLED_BY;
            Strcpy(killer.name, "精疲力竭");
            done(STARVING);
            return;
        }
    }
}

/* Returns an object representing food.
 * Object may be either on floor or in inventory.
 */
struct obj *
floorfood(verb, corpsecheck)
const char *verb;
int corpsecheck; /* 0, no check, 1, corpses, 2, tinnable corpses */
{
    register struct obj *otmp;
    char qbuf[QBUFSZ];
    char c;
    boolean feeding = !strcmp(verb, "吃"),    /* corpsecheck==0 */  //eat
        offering = !strcmp(verb, "献祭"); /* corpsecheck==1 */  //sacrifice

    /* if we can't touch floor objects then use invent food only */
    if (!can_reach_floor(TRUE) || (feeding && u.usteed)
        || (is_pool_or_lava(u.ux, u.uy)
            && (Wwalking || is_clinger(youmonst.data)
                || (Flying && !Breathless))))
        goto skipfloor;

    if (feeding && metallivorous(youmonst.data)) {
        struct obj *gold;
        struct trap *ttmp = t_at(u.ux, u.uy);

        if (ttmp && ttmp->tseen && ttmp->ttyp == BEAR_TRAP) {
            /* If not already stuck in the trap, perhaps there should
               be a chance to becoming trapped?  Probably not, because
               then the trap would just get eaten on the _next_ turn... */
            Sprintf(qbuf, "这里有捕兽夹( %s); 吃了它?",
                    (u.utrap && u.utraptype == TT_BEARTRAP) ? "牵制着你"
                                                            : "随身的");
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                u.utrap = u.utraptype = 0;
                deltrap(ttmp);
                return mksobj(BEARTRAP, TRUE, FALSE);
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }

        if (youmonst.data != &mons[PM_RUST_MONSTER]
            && (gold = g_at(u.ux, u.uy)) != 0) {
            if (gold->quan == 1L)
                Sprintf(qbuf, "这里有1 金币; 吃了它?");
            else
                Sprintf(qbuf, "这里有%ld 金币; 吃了它们?",
                        gold->quan);
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y') {
                return gold;
            } else if (c == 'q') {
                return (struct obj *) 0;
            }
        }
    }

    /* Is there some food (probably a heavy corpse) here on the ground? */
    for (otmp = level.objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere) {
        if (corpsecheck
                ? (otmp->otyp == CORPSE
                   && (corpsecheck == 1 || tinnable(otmp)))
                : feeding ? (otmp->oclass != COIN_CLASS && is_edible(otmp))
                          : otmp->oclass == FOOD_CLASS) {
            char qsfx[QBUFSZ];
            boolean one = (otmp->quan == 1L);

            /* "There is <an object> here; <verb> it?" or
               "There are <N objects> here; <verb> one?" */
            Sprintf(qbuf, "这里有");
            Sprintf(qsfx, ";  %s %s?", verb, one ? "了它" : "一个");
            (void) safe_qbuf(qbuf, qbuf, qsfx, otmp, doname, ansimpleoname,
                             one ? something : (const char *) "things");
            if ((c = yn_function(qbuf, ynqchars, 'n')) == 'y')
                return  otmp;
            else if (c == 'q')
                return (struct obj *) 0;
        }
    }

skipfloor:
    /* We cannot use ALL_CLASSES since that causes getobj() to skip its
     * "ugly checks" and we need to check for inedible items.
     */
    otmp =
        getobj(feeding ? allobj : offering ? offerfodder : comestibles, verb);
    if (corpsecheck && otmp && !(offering && otmp->oclass == AMULET_CLASS))
        if (otmp->otyp != CORPSE || (corpsecheck == 2 && !tinnable(otmp))) {
            You_cant("%s 那个!", verb);
            return (struct obj *) 0;
        }
    return otmp;
}

/* Side effects of vomiting */
/* added nomul (MRS) - it makes sense, you're too busy being sick! */
void
vomit() /* A good idea from David Neves */
{
    if (cantvomit(youmonst.data))
        /* doesn't cure food poisoning; message assumes that we aren't
           dealing with some esoteric body_part() */
        Your("jaw gapes convulsively.");
    else
        make_sick(0L, (char *) 0, TRUE, SICK_VOMITABLE);
    nomul(-2);
    multi_reason = "呕吐";
    nomovemsg = You_can_move_again;
}

int
eaten_stat(base, obj)
int base;
struct obj *obj;
{
    long uneaten_amt, full_amount;

    uneaten_amt = (long) obj->oeaten;
    full_amount = (obj->otyp == CORPSE)
                      ? (long) mons[obj->corpsenm].cnutrit
                      : (long) objects[obj->otyp].oc_nutrition;
    if (uneaten_amt > full_amount) {
        impossible(
          "partly eaten food (%ld) more nutritious than untouched food (%ld)",
                   uneaten_amt, full_amount);
        uneaten_amt = full_amount;
    }

    base = (int) (full_amount ? (long) base * uneaten_amt / full_amount : 0L);
    return (base < 1) ? 1 : base;
}

/* reduce obj's oeaten field, making sure it never hits or passes 0 */
void
consume_oeaten(obj, amt)
struct obj *obj;
int amt;
{
    /*
     * This is a hack to try to squelch several long standing mystery
     * food bugs.  A better solution would be to rewrite the entire
     * victual handling mechanism from scratch using a less complex
     * model.  Alternatively, this routine could call done_eating()
     * or food_disappears() but its callers would need revisions to
     * cope with context.victual.piece unexpectedly going away.
     *
     * Multi-turn eating operates by setting the food's oeaten field
     * to its full nutritional value and then running a counter which
     * independently keeps track of whether there is any food left.
     * The oeaten field can reach exactly zero on the last turn, and
     * the object isn't removed from inventory until the next turn
     * when the "you finish eating" message gets delivered, so the
     * food would be restored to the status of untouched during that
     * interval.  This resulted in unexpected encumbrance messages
     * at the end of a meal (if near enough to a threshold) and would
     * yield full food if there was an interruption on the critical
     * turn.  Also, there have been reports over the years of food
     * becoming massively heavy or producing unlimited satiation;
     * this would occur if reducing oeaten via subtraction attempted
     * to drop it below 0 since its unsigned type would produce a
     * huge positive value instead.  So far, no one has figured out
     * _why_ that inappropriate subtraction might sometimes happen.
     */

    if (amt > 0) {
        /* bit shift to divide the remaining amount of food */
        obj->oeaten >>= amt;
    } else {
        /* simple decrement; value is negative so we actually add it */
        if ((int) obj->oeaten > -amt)
            obj->oeaten += amt;
        else
            obj->oeaten = 0;
    }

    if (obj->oeaten == 0) {
        if (obj == context.victual.piece) /* always true unless wishing... */
            context.victual.reqtime =
                context.victual.usedtime; /* no bites left */
        obj->oeaten = 1; /* smallest possible positive value */
    }
}

/* called when eatfood occupation has been interrupted,
   or in the case of theft, is about to be interrupted */
boolean
maybe_finished_meal(stopping)
boolean stopping;
{
    /* in case consume_oeaten() has decided that the food is all gone */
    if (occupation == eatfood
        && context.victual.usedtime >= context.victual.reqtime) {
        if (stopping)
            occupation = 0; /* for do_reset_eat */
        (void) eatfood();   /* calls done_eating() to use up
                               context.victual.piece */
        return TRUE;
    }
    return FALSE;
}

/* Tin of <something> to the rescue?  Decide whether current occupation
   is an attempt to eat a tin of something capable of saving hero's life.
   We don't care about consumption of non-tinned food here because special
   effects there take place on first bite rather than at end of occupation.
   [Popeye the Sailor gets out of trouble by eating tins of spinach. :-] */
boolean
Popeye(threat)
int threat;
{
    struct obj *otin;
    int mndx;

    if (occupation != opentin)
        return FALSE;
    otin = context.tin.tin;
    /* make sure hero still has access to tin */
    if (!carried(otin)
        && (!obj_here(otin, u.ux, u.uy) || !can_reach_floor(TRUE)))
        return FALSE;
    /* unknown tin is assumed to be helpful */
    if (!otin->known)
        return TRUE;
    /* known tin is helpful if it will stop life-threatening problem */
    mndx = otin->corpsenm;
    switch (threat) {
    /* note: not used; hunger code bypasses stop_occupation() when eating */
    case HUNGER:
        return (boolean) (mndx != NON_PM || otin->spe == 1);
    /* flesh from lizards and acidic critters stops petrification */
    case STONED:
        return (boolean) (mndx >= LOW_PM
                          && (mndx == PM_LIZARD || acidic(&mons[mndx])));
    /* no tins can cure these (yet?) */
    case SLIMED:
    case SICK:
    case VOMITING:
        break;
    default:
        break;
    }
    return FALSE;
}

/*eat.c*/
