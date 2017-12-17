/* NetHack 3.6	read.c	$NHDT-Date: 1448862378 2015/11/30 05:46:18 $  $NHDT-Branch: master $:$NHDT-Revision: 1.125 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#define Your_Own_Role(mndx)  \
    ((mndx) == urole.malenum \
     || (urole.femalenum != NON_PM && (mndx) == urole.femalenum))
#define Your_Own_Race(mndx)  \
    ((mndx) == urace.malenum \
     || (urace.femalenum != NON_PM && (mndx) == urace.femalenum))

boolean known;

static NEARDATA const char readable[] = { ALL_CLASSES, SCROLL_CLASS,
                                          SPBOOK_CLASS, 0 };
static const char all_count[] = { ALLOW_COUNT, ALL_CLASSES, 0 };

STATIC_DCL boolean FDECL(learnscrolltyp, (SHORT_P));
STATIC_DCL char * FDECL(erode_obj_text, (struct obj *, char *));
STATIC_DCL void NDECL(do_class_genocide);
STATIC_DCL void FDECL(stripspe, (struct obj *));
STATIC_DCL void FDECL(p_glow1, (struct obj *));
STATIC_DCL void FDECL(p_glow2, (struct obj *, const char *));
STATIC_DCL void FDECL(randomize, (int *, int));
STATIC_DCL void FDECL(forget_single_object, (int));
STATIC_DCL void FDECL(forget, (int));
STATIC_DCL int FDECL(maybe_tame, (struct monst *, struct obj *));
STATIC_DCL boolean FDECL(is_valid_stinking_cloud_pos, (int, int, BOOLEAN_P));
STATIC_DCL void FDECL(display_stinking_cloud_positions, (int));
STATIC_PTR void FDECL(set_lit, (int, int, genericptr));

STATIC_OVL boolean
learnscrolltyp(scrolltyp)
short scrolltyp;
{
    if (!objects[scrolltyp].oc_name_known) {
        makeknown(scrolltyp);
        more_experienced(0, 10);
        return TRUE;
    } else
        return FALSE;
}

/* also called from teleport.c for scroll of teleportation */
void
learnscroll(sobj)
struct obj *sobj;
{
    /* it's implied that sobj->dknown is set;
       we couldn't be reading this scroll otherwise */
    if (sobj->oclass != SPBOOK_CLASS)
        (void) learnscrolltyp(sobj->otyp);
}

char *
erode_obj_text(otmp, buf)
struct obj *otmp;
char *buf;
{
    int erosion = greatest_erosion(otmp);
    if (erosion)
        wipeout_text(buf, (int) (strlen(buf) * erosion / (2 * MAX_ERODE)),
                     otmp->o_id ^ (unsigned) ubirthday);
    return buf;
}

char *
tshirt_text(tshirt, buf)
struct obj *tshirt;
char *buf;
{
    static const char *shirt_msgs[] = {
        /* Scott Bigham */
      "I explored the Dungeons of Doom and all I got was this lousy T-shirt!",
        "Is that Mjollnir in your pocket or are you just happy to see me?",
      "It's not the size of your sword, it's how #enhance'd you are with it.",
        "Madame Elvira's House O' Succubi Lifetime Customer",
        "Madame Elvira's House O' Succubi Employee of the Month",
        "Ludios Vault Guards Do It In Small, Dark Rooms",
        "Yendor Military Soldiers Do It In Large Groups",
        "I survived Yendor Military Boot Camp",
        "Ludios Accounting School Intra-Mural Lacrosse Team",
        "Oracle(TM) Fountains 10th Annual Wet T-Shirt Contest",
        "Hey, black dragon!  Disintegrate THIS!",
        "I'm With Stupid -->",
        "Don't blame me, I voted for Izchak!",
        "Don't Panic", /* HHGTTG */
        "Furinkan High School Athletic Dept.",                /* Ranma 1/2 */
        "Hel-LOOO, Nurse!",                                   /* Animaniacs */
        "=^.^=",
        "100% goblin hair - do not wash",
        "Aberzombie and Fitch",
        "cK -- Cockatrice touches the Kop",
        "Don't ask me, I only adventure here",
        "Down with pants!",
        "d, your dog or a killer?",
        "FREE PUG AND NEWT!",
        "Go team ant!",
        "Got newt?",
        "Hello, my darlings!", /* Charlie Drake */
        "Hey! Nymphs! Steal This T-Shirt!",
        "I <3 Dungeon of Doom",
        "I <3 Maud",
        "I am a Valkyrie. If you see me running, try to keep up.",
        "I am not a pack rat - I am a collector",
        "I bounced off a rubber tree",         /* Monkey Island */
        "Plunder Island Brimstone Beach Club", /* Monkey Island */
        "If you can read this, I can hit you with my polearm",
        "I'm confused!",
        "I scored with the princess",
        "I want to live forever or die in the attempt.",
        "Lichen Park",
        "LOST IN THOUGHT - please send search party",
        "Meat is Mordor",
        "Minetown Better Business Bureau",
        "Minetown Watch",
 "Ms. Palm's House of Negotiable Affection -- A Very Reputable House Of Disrepute",
        "Protection Racketeer",
        "Real men love Crom",
        "Somebody stole my Mojo!",
        "The Hellhound Gang",
        "The Werewolves",
        "They Might Be Storm Giants",
        "Weapons don't kill people, I kill people",
        "White Zombie",
        "You're killing me!",
        "Anhur State University - Home of the Fighting Fire Ants!",
        "FREE HUGS",
        "Serial Ascender",
        "Real men are valkyries",
        "Young Men's Cavedigging Association",
        "Occupy Fort Ludios",
        "I couldn't afford this T-shirt so I stole it!",
        "Mind flayers suck",
        "I'm not wearing any pants",
        "Down with the living!",
        "Pudding farmer",
        "Vegetarian",
        "Hello, I'm War!",
    };

    Strcpy(buf, shirt_msgs[tshirt->o_id % SIZE(shirt_msgs)]);
    return erode_obj_text(tshirt, buf);
}

char *
apron_text(apron, buf)
struct obj *apron;
char *buf;
{
    static const char *apron_msgs[] = {
        "Kiss the cook",
        "I'm making SCIENCE!",
        "Don't mess with the chef",
        "Don't make me poison you",
        "Gehennom's Kitchen",
        "Rat: The other white meat",
        "If you can't stand the heat, get out of Gehennom!",
        "If we weren't meant to eat animals, why are they made out of meat?",
        "If you don't like the food, I'll stab you",
    };

    Strcpy(buf, apron_msgs[apron->o_id % SIZE(apron_msgs)]);
    return erode_obj_text(apron, buf);
}

int
doread()
{
    register struct obj *scroll;
    boolean confused, nodisappear;

    known = FALSE;
    if (check_capacity((char *) 0))
        return 0;
    scroll = getobj(readable, "阅读");  //read
    if (!scroll)
        return 0;

    /* outrumor has its own blindness check */
    if (scroll->otyp == FORTUNE_COOKIE) {
        if (flags.verbose)
            You("打碎了饼干并扔掉了碎屑.");
        outrumor(bcsign(scroll), BY_COOKIE);
        if (!Blind)
            u.uconduct.literate++;
        useup(scroll);
        return 1;
    } else if (scroll->otyp == T_SHIRT || scroll->otyp == ALCHEMY_SMOCK) {
        char buf[BUFSZ];
        if (Blind) {
            You_cant("感觉到任何盲文.");
            return 0;
        }
        /* can't read shirt worn under suit (under cloak is ok though) */
        if (scroll->otyp == T_SHIRT && uarm && scroll == uarmu) {
            pline("%s 衬衫被%s%s遮掩了.",
                  scroll->unpaid ? "那个" : "你的", shk_your(buf, uarm),
                  suit_simple_name(uarm));
            return 0;
        }
        u.uconduct.literate++;
        if (flags.verbose)
            pline("上面写着:");
        pline("\"%s\"", (scroll->otyp == T_SHIRT) ? tshirt_text(scroll, buf)
                                                  : apron_text(scroll, buf));
        return 1;
    } else if (scroll->otyp == CREDIT_CARD) {
        static const char *card_msgs[] = {
            "Leprechaun Gold Tru$t - Shamrock Card",
            "Magic Memory Vault Charge Card", "Larn National Bank", /* Larn */
            "First Bank of Omega",               /* Omega */
            "Bank of Zork - Frobozz Magic Card", /* Zork */
            "Ankh-Morpork Merchant's Guild Barter Card",
            "Ankh-Morpork Thieves' Guild Unlimited Transaction Card",
            "Ransmannsby Moneylenders Association",
            "Bank of Gehennom - 99% Interest Card",
            "Yendorian Express - Copper Card",
            "Yendorian Express - Silver Card",
            "Yendorian Express - Gold Card",
            "Yendorian Express - Mithril Card",
            "Yendorian Express - Platinum Card", /* must be last */
        };

        if (Blind) {
            You("感受凸起的数字:");
        } else {
            if (flags.verbose)
                pline("上面写着:");
            pline("\"%s\"",
                  scroll->oartifact
                      ? card_msgs[SIZE(card_msgs) - 1]
                      : card_msgs[scroll->o_id % (SIZE(card_msgs) - 1)]);
        }
        /* Make a credit card number */
        pline("\"%d0%d %d%d1 0%d%d0\"", ((scroll->o_id % 89) + 10),
              (scroll->o_id % 4), (((scroll->o_id * 499) % 899999) + 100000),
              (scroll->o_id % 10), (!(scroll->o_id % 3)),
              ((scroll->o_id * 7) % 10));
        u.uconduct.literate++;
        return 1;
    } else if (scroll->otyp == CAN_OF_GREASE) {
        pline("这个%s 没有标签.", singular(scroll, xname));
        return 0;
    } else if (scroll->otyp == MAGIC_MARKER) {
        if (Blind) {
            You_cant("感觉到任何盲文.");
            return 0;
        }
        if (flags.verbose)
            pline("上面写着:");
        pline("\"Magic Marker(TM) Red Ink Marker Pen. Water Soluble.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oclass == COIN_CLASS) {
        if (Blind)
            You("感受凸起的文字:");
        else if (flags.verbose)
            You("读道:");
        pline("\"1 Zorkmid. 857 GUE. In Frobs We Trust.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oartifact == ART_ORB_OF_FATE) {
        if (Blind)
            You("感受雕刻的签名:");
        else
            pline("署名是:");
        pline("\" 欧丁.\"");
        u.uconduct.literate++;
        return 1;
    } else if (scroll->otyp == CANDY_BAR) {
        static const char *wrapper_msgs[] = {
            "Apollo",       /* Lost */
            "Moon Crunchy", /* South Park */
            "Snacky Cake",    "Chocolate Nuggie", "The Small Bar",
            "Crispy Yum Yum", "Nilla Crunchie",   "Berry Bar",
            "Choco Nummer",   "Om-nom", /* Cat Macro */
            "Fruity Oaty",              /* Serenity */
            "Wonka Bar" /* Charlie and the Chocolate Factory */
        };

        if (Blind) {
            You_cant("感觉到任何盲文.");
            return 0;
        }
        pline("包装纸上写着: \"%s\"",
              wrapper_msgs[scroll->o_id % SIZE(wrapper_msgs)]);
        u.uconduct.literate++;
        return 1;
    } else if (scroll->oclass != SCROLL_CLASS
               && scroll->oclass != SPBOOK_CLASS) {
        pline(silly_thing_to, "阅读");
        return 0;
    } else if (Blind && (scroll->otyp != SPE_BOOK_OF_THE_DEAD)) {
        const char *what = 0;
        if (scroll->oclass == SPBOOK_CLASS)
            what = "神秘的符文";
        else if (!scroll->dknown)
            what = "卷轴上的术式";
        if (what) {
            pline("作为盲人, 你不能阅读%s.", what);
            return 0;
        }
    }

    confused = (Confusion != 0);
#ifdef MAIL
    if (scroll->otyp == SCR_MAIL) {
        confused = FALSE; /* override */
        /* reading mail is a convenience for the player and takes
           place outside the game, so shouldn't affect gameplay;
           on the other hand, it starts by explicitly making the
           hero actively read something, which is pretty hard
           to simply ignore; as a compromise, if the player has
           maintained illiterate conduct so far, and this mail
           scroll didn't come from bones, ask for confirmation */
        if (!u.uconduct.literate) {
            if (!scroll->spe && yn(
             "Reading mail will violate \"illiterate\" conduct.  Read anyway?"
                                   ) != 'y')
                return 0;
        }
    }
#endif

    /* Actions required to win the game aren't counted towards conduct */
    /* Novel conduct is handled in read_tribute so exclude it too*/
    if (scroll->otyp != SPE_BOOK_OF_THE_DEAD
        && scroll->otyp != SPE_BLANK_PAPER && scroll->otyp != SCR_BLANK_PAPER
        && scroll->otyp != SPE_NOVEL)
        u.uconduct.literate++;

    if (scroll->oclass == SPBOOK_CLASS) {
        return study_book(scroll);
    }
    scroll->in_use = TRUE; /* scroll, not spellbook, now being read */
    if (scroll->otyp != SCR_BLANK_PAPER) {
        /* a few scroll feedback messages describe something happening
           to the scroll itself, so avoid "it disappears" for those */
        nodisappear = (scroll->otyp == SCR_FIRE
                       || (scroll->otyp == SCR_REMOVE_CURSE
                           && scroll->cursed));
        if (Blind)
            pline(nodisappear
                      ? "你%s 卷轴上的术式."
                      : "当你%s 它上面的术式, 卷轴消失了.",
                  is_silent(youmonst.data) ? "思考" : "朗读");
        else
            pline(nodisappear ? "你阅读卷轴."
                              : "当你念完卷轴, 它就消失了.");
        if (confused) {
            if (Hallucination)
                pline("在如此的幻觉中, 你搞砸了...");
            else
                pline("混乱的你%s 了咒语...",
                      is_silent(youmonst.data) ? "误解"
                                               : "读错");
        }
    }
    if (!seffects(scroll)) {
        if (!objects[scroll->otyp].oc_name_known) {
            if (known)
                learnscroll(scroll);
            else if (!objects[scroll->otyp].oc_uname)
                docall(scroll);
        }
        scroll->in_use = FALSE;
        if (scroll->otyp != SCR_BLANK_PAPER)
            useup(scroll);
    }
    return 1;
}

STATIC_OVL void
stripspe(obj)
register struct obj *obj;
{
    if (obj->blessed || obj->spe <= 0) {
        pline1(nothing_happens);
    } else {
        /* order matters: message, shop handling, actual transformation */
        pline("%s 短暂的.", Yobjnam2(obj, "振动"));
        costly_alteration(obj, COST_UNCHRG);
        obj->spe = 0;
        if (obj->otyp == OIL_LAMP || obj->otyp == BRASS_LANTERN)
            obj->age = 0;
    }
}

STATIC_OVL void
p_glow1(otmp)
register struct obj *otmp;
{
    pline("%s 短暂的.", Yobjnam2(otmp, Blind ? "振动" : "发光"));
}

STATIC_OVL void
p_glow2(otmp, color)
register struct obj *otmp;
register const char *color;
{
    pline("%s%s%s了片刻.", Yobjnam2(otmp, Blind ? "振动" : "发光"),
          Blind ? "" : " ", Blind ? "" : hcolor(color));
}

/* Is the object chargeable?  For purposes of inventory display; it is
   possible to be able to charge things for which this returns FALSE. */
boolean
is_chargeable(obj)
struct obj *obj;
{
    if (obj->oclass == WAND_CLASS)
        return TRUE;
    /* known && !oc_name_known is possible after amnesia/mind flayer */
    if (obj->oclass == RING_CLASS)
        return (boolean) (objects[obj->otyp].oc_charged
                          && (obj->known
                              || (obj->dknown
                                  && objects[obj->otyp].oc_name_known)));
    if (is_weptool(obj)) /* specific check before general tools */
        return FALSE;
    if (obj->oclass == TOOL_CLASS)
        return (boolean) objects[obj->otyp].oc_charged;
    return FALSE; /* why are weapons/armor considered charged anyway? */
}

/* recharge an object; curse_bless is -1 if the recharging implement
   was cursed, +1 if blessed, 0 otherwise. */
void
recharge(obj, curse_bless)
struct obj *obj;
int curse_bless;
{
    register int n;
    boolean is_cursed, is_blessed;

    is_cursed = curse_bless < 0;
    is_blessed = curse_bless > 0;

    if (obj->oclass == WAND_CLASS) {
        int lim = (obj->otyp == WAN_WISHING)
                      ? 3
                      : (objects[obj->otyp].oc_dir != NODIR) ? 8 : 15;

        /* undo any prior cancellation, even when is_cursed */
        if (obj->spe == -1)
            obj->spe = 0;

        /*
         * Recharging might cause wands to explode.
         *      v = number of previous recharges
         *            v = percentage chance to explode on this attempt
         *                    v = cumulative odds for exploding
         *      0 :   0       0
         *      1 :   0.29    0.29
         *      2 :   2.33    2.62
         *      3 :   7.87   10.28
         *      4 :  18.66   27.02
         *      5 :  36.44   53.62
         *      6 :  62.97   82.83
         *      7 : 100     100
         */
        n = (int) obj->recharged;
        if (n > 0 && (obj->otyp == WAN_WISHING
                      || (n * n * n > rn2(7 * 7 * 7)))) { /* recharge_limit */
            wand_explode(obj, rnd(lim));
            return;
        }
        /* didn't explode, so increment the recharge count */
        obj->recharged = (unsigned) (n + 1);

        /* now handle the actual recharging */
        if (is_cursed) {
            stripspe(obj);
        } else {
            n = (lim == 3) ? 3 : rn1(5, lim + 1 - 5);
            if (!is_blessed)
                n = rnd(n);

            if (obj->spe < n)
                obj->spe = n;
            else
                obj->spe++;
            if (obj->otyp == WAN_WISHING && obj->spe > 3) {
                wand_explode(obj, 1);
                return;
            }
            if (obj->spe >= lim)
                p_glow2(obj, NH_BLUE);
            else
                p_glow1(obj);
#if 0 /*[shop price doesn't vary by charge count]*/
            /* update shop bill to reflect new higher price */
            if (obj->unpaid)
                alter_cost(obj, 0L);
#endif
        }

    } else if (obj->oclass == RING_CLASS && objects[obj->otyp].oc_charged) {
        /* charging does not affect ring's curse/bless status */
        int s = is_blessed ? rnd(3) : is_cursed ? -rnd(2) : 1;
        boolean is_on = (obj == uleft || obj == uright);

        /* destruction depends on current state, not adjustment */
        if (obj->spe > rn2(7) || obj->spe <= -5) {
            pline("%s, 然后%s了!", Yobjnam2(obj, "立即振动"),
                  otense(obj, "爆炸"));
            if (is_on)
                Ring_gone(obj);
            s = rnd(3 * abs(obj->spe)); /* amount of damage */
            useup(obj);
            losehp(Maybe_Half_Phys(s), "爆炸的戒指", KILLED_BY_AN);
        } else {
            long mask = is_on ? (obj == uleft ? LEFT_RING : RIGHT_RING) : 0L;

            pline("%s%s时针旋转了片刻.", Yname2(obj),
                  s < 0 ? "逆" : "顺");
            if (s < 0)
                costly_alteration(obj, COST_DECHNT);
            /* cause attributes and/or properties to be updated */
            if (is_on)
                Ring_off(obj);
            obj->spe += s; /* update the ring while it's off */
            if (is_on)
                setworn(obj, mask), Ring_on(obj);
            /* oartifact: if a touch-sensitive artifact ring is
               ever created the above will need to be revised  */
            /* update shop bill to reflect new higher price */
            if (s > 0 && obj->unpaid)
                alter_cost(obj, 0L);
        }

    } else if (obj->oclass == TOOL_CLASS) {
        int rechrg = (int) obj->recharged;

        if (objects[obj->otyp].oc_charged) {
            /* tools don't have a limit, but the counter used does */
            if (rechrg < 7) /* recharge_limit */
                obj->recharged++;
        }
        switch (obj->otyp) {
        case BELL_OF_OPENING:
            if (is_cursed)
                stripspe(obj);
            else if (is_blessed)
                obj->spe += rnd(3);
            else
                obj->spe += 1;
            if (obj->spe > 5)
                obj->spe = 5;
            break;
        case MAGIC_MARKER:
        case TINNING_KIT:
        case EXPENSIVE_CAMERA:
            if (is_cursed)
                stripspe(obj);
            else if (rechrg
                     && obj->otyp
                            == MAGIC_MARKER) { /* previously recharged */
                obj->recharged = 1; /* override increment done above */
                if (obj->spe < 3)
                    Your("笔似乎永久的干了.");
                else
                    pline1(nothing_happens);
            } else if (is_blessed) {
                n = rn1(16, 15); /* 15..30 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else if (obj->spe + n <= 75)
                    obj->spe = 75;
                else {
                    int chrg = (int) obj->spe;
                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_BLUE);
            } else {
                n = rn1(11, 10); /* 10..20 */
                if (obj->spe + n <= 50)
                    obj->spe = 50;
                else {
                    int chrg = (int) obj->spe;
                    if ((chrg + n) > 127)
                        obj->spe = 127;
                    else
                        obj->spe += n;
                }
                p_glow2(obj, NH_WHITE);
            }
            break;
        case OIL_LAMP:
        case BRASS_LANTERN:
            if (is_cursed) {
                stripspe(obj);
                if (obj->lamplit) {
                    if (!Blind)
                        pline("%s灭了!", Tobjnam(obj, "熄"));
                    end_burn(obj, TRUE);
                }
            } else if (is_blessed) {
                obj->spe = 1;
                obj->age = 1500;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe = 1;
                obj->age += 750;
                if (obj->age > 1500)
                    obj->age = 1500;
                p_glow1(obj);
            }
            break;
        case CRYSTAL_BALL:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe = 6;
                p_glow2(obj, NH_BLUE);
            } else {
                if (obj->spe < 5) {
                    obj->spe++;
                    p_glow1(obj);
                } else
                    pline1(nothing_happens);
            }
            break;
        case HORN_OF_PLENTY:
        case BAG_OF_TRICKS:
        case CAN_OF_GREASE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                if (obj->spe <= 10)
                    obj->spe += rn1(10, 6);
                else
                    obj->spe += rn1(5, 6);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rnd(5);
                if (obj->spe > 50)
                    obj->spe = 50;
                p_glow1(obj);
            }
            break;
        case MAGIC_FLUTE:
        case MAGIC_HARP:
        case FROST_HORN:
        case FIRE_HORN:
        case DRUM_OF_EARTHQUAKE:
            if (is_cursed) {
                stripspe(obj);
            } else if (is_blessed) {
                obj->spe += d(2, 4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow2(obj, NH_BLUE);
            } else {
                obj->spe += rnd(4);
                if (obj->spe > 20)
                    obj->spe = 20;
                p_glow1(obj);
            }
            break;
        default:
            goto not_chargable;
            /*NOTREACHED*/
            break;
        } /* switch */

    } else {
    not_chargable:
        You("有一种失落感.");
    }
}

/* Forget known information about this object type. */
STATIC_OVL void
forget_single_object(obj_id)
int obj_id;
{
    objects[obj_id].oc_name_known = 0;
    objects[obj_id].oc_pre_discovered = 0; /* a discovery when relearned */
    if (objects[obj_id].oc_uname) {
        free((genericptr_t) objects[obj_id].oc_uname);
        objects[obj_id].oc_uname = 0;
    }
    undiscover_object(obj_id); /* after clearing oc_name_known */

    /* clear & free object names from matching inventory items too? */
}

#if 0 /* here if anyone wants it.... */
/* Forget everything known about a particular object class. */
STATIC_OVL void
forget_objclass(oclass)
int oclass;
{
    int i;

    for (i = bases[oclass];
         i < NUM_OBJECTS && objects[i].oc_class == oclass; i++)
        forget_single_object(i);
}
#endif

/* randomize the given list of numbers  0 <= i < count */
STATIC_OVL void
randomize(indices, count)
int *indices;
int count;
{
    int i, iswap, temp;

    for (i = count - 1; i > 0; i--) {
        if ((iswap = rn2(i + 1)) == i)
            continue;
        temp = indices[i];
        indices[i] = indices[iswap];
        indices[iswap] = temp;
    }
}

/* Forget % of known objects. */
void
forget_objects(percent)
int percent;
{
    int i, count;
    int indices[NUM_OBJECTS];

    if (percent == 0)
        return;
    if (percent <= 0 || percent > 100) {
        impossible("forget_objects: bad percent %d", percent);
        return;
    }

    indices[0] = 0; /* lint suppression */
    for (count = 0, i = 1; i < NUM_OBJECTS; i++)
        if (OBJ_DESCR(objects[i])
            && (objects[i].oc_name_known || objects[i].oc_uname))
            indices[count++] = i;

    if (count > 0) {
        randomize(indices, count);

        /* forget first % of randomized indices */
        count = ((count * percent) + rn2(100)) / 100;
        for (i = 0; i < count; i++)
            forget_single_object(indices[i]);
    }
}

/* Forget some or all of map (depends on parameters). */
void
forget_map(howmuch)
int howmuch;
{
    register int zx, zy;

    if (Sokoban)
        return;

    known = TRUE;
    for (zx = 0; zx < COLNO; zx++)
        for (zy = 0; zy < ROWNO; zy++)
            if (howmuch & ALL_MAP || rn2(7)) {
                /* Zonk all memory of this location. */
                levl[zx][zy].seenv = 0;
                levl[zx][zy].waslit = 0;
                levl[zx][zy].glyph = cmap_to_glyph(S_stone);
                lastseentyp[zx][zy] = STONE;
            }
    /* forget overview data for this level */
    forget_mapseen(ledger_no(&u.uz));
}

/* Forget all traps on the level. */
void
forget_traps()
{
    register struct trap *trap;

    /* forget all traps (except the one the hero is in :-) */
    for (trap = ftrap; trap; trap = trap->ntrap)
        if ((trap->tx != u.ux || trap->ty != u.uy) && (trap->ttyp != HOLE))
            trap->tseen = 0;
}

/*
 * Forget given % of all levels that the hero has visited and not forgotten,
 * except this one.
 */
void
forget_levels(percent)
int percent;
{
    int i, count;
    xchar maxl, this_lev;
    int indices[MAXLINFO];

    if (percent == 0)
        return;

    if (percent <= 0 || percent > 100) {
        impossible("forget_levels: bad percent %d", percent);
        return;
    }

    this_lev = ledger_no(&u.uz);
    maxl = maxledgerno();

    /* count & save indices of non-forgotten visited levels */
    /* Sokoban levels are pre-mapped for the player, and should stay
     * so, or they become nearly impossible to solve.  But try to
     * shift the forgetting elsewhere by fiddling with percent
     * instead of forgetting fewer levels.
     */
    indices[0] = 0; /* lint suppression */
    for (count = 0, i = 0; i <= maxl; i++)
        if ((level_info[i].flags & VISITED)
            && !(level_info[i].flags & FORGOTTEN) && i != this_lev) {
            if (ledger_to_dnum(i) == sokoban_dnum)
                percent += 2;
            else
                indices[count++] = i;
        }

    if (percent > 100)
        percent = 100;

    if (count > 0) {
        randomize(indices, count);

        /* forget first % of randomized indices */
        count = ((count * percent) + 50) / 100;
        for (i = 0; i < count; i++) {
            level_info[indices[i]].flags |= FORGOTTEN;
            forget_mapseen(indices[i]);
        }
    }
}

/*
 * Forget some things (e.g. after reading a scroll of amnesia).  When called,
 * the following are always forgotten:
 *      - felt ball & chain
 *      - traps
 *      - part (6 out of 7) of the map
 *
 * Other things are subject to flags:
 *      howmuch & ALL_MAP       = forget whole map
 *      howmuch & ALL_SPELLS    = forget all spells
 */
STATIC_OVL void
forget(howmuch)
int howmuch;
{
    if (Punished)
        u.bc_felt = 0; /* forget felt ball&chain */

    forget_map(howmuch);
    forget_traps();

    /* 1 in 3 chance of forgetting some levels */
    if (!rn2(3))
        forget_levels(rn2(25));

    /* 1 in 3 chance of forgetting some objects */
    if (!rn2(3))
        forget_objects(rn2(25));

    if (howmuch & ALL_SPELLS)
        losespells();
    /*
     * Make sure that what was seen is restored correctly.  To do this,
     * we need to go blind for an instant --- turn off the display,
     * then restart it.  All this work is needed to correctly handle
     * walls which are stone on one side and wall on the other.  Turning
     * off the seen bits above will make the wall revert to stone,  but
     * there are cases where we don't want this to happen.  The easiest
     * thing to do is to run it through the vision system again, which
     * is always correct.
     */
    docrt(); /* this correctly will reset vision */
}

/* monster is hit by scroll of taming's effect */
STATIC_OVL int
maybe_tame(mtmp, sobj)
struct monst *mtmp;
struct obj *sobj;
{
    int was_tame = mtmp->mtame;
    unsigned was_peaceful = mtmp->mpeaceful;

    if (sobj->cursed) {
        setmangry(mtmp);
        if (was_peaceful && !mtmp->mpeaceful)
            return -1;
    } else {
        if (mtmp->isshk)
            make_happy_shk(mtmp, FALSE);
        else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
            (void) tamedog(mtmp, (struct obj *) 0);
        if ((!was_peaceful && mtmp->mpeaceful) || (!was_tame && mtmp->mtame))
            return 1;
    }
    return 0;
}

boolean
is_valid_stinking_cloud_pos(x, y, showmsg)
int x, y;
boolean showmsg;
{
    if (!cansee(x, y) || !ACCESSIBLE(levl[x][y].typ) || distu(x, y) >= 32) {
        if (showmsg)
            You("闻到腐烂的蛋.");
        return FALSE;
    }
    return TRUE;
}

void
display_stinking_cloud_positions(state)
int state;
{
    if (state == 0) {
        tmp_at(DISP_BEAM, cmap_to_glyph(S_goodpos));
    } else if (state == 1) {
        int x, y, dx, dy;
        int dist = 6;

        for (dx = -dist; dx <= dist; dx++)
            for (dy = -dist; dy <= dist; dy++) {
                x = u.ux + dx;
                y = u.uy + dy;
                if (isok(x, y) && is_valid_stinking_cloud_pos(x, y, FALSE))
                    tmp_at(x, y);
            }
    } else {
        tmp_at(DISP_END, 0);
    }
}

/* scroll effects; return 1 if we use up the scroll and possibly make it
   become discovered, 0 if caller should take care of those side-effects */
int
seffects(sobj)
struct obj *sobj; /* scroll, or fake spellbook object for scroll-like spell */
{
    int cval, otyp = sobj->otyp;
    boolean confused = (Confusion != 0), sblessed = sobj->blessed,
            scursed = sobj->cursed, already_known, old_erodeproof,
            new_erodeproof;
    struct obj *otmp;

    if (objects[otyp].oc_magic)
        exercise(A_WIS, TRUE);                       /* just for trying */
    already_known = (sobj->oclass == SPBOOK_CLASS /* spell */
                     || objects[otyp].oc_name_known);

    switch (otyp) {
#ifdef MAIL
    case SCR_MAIL:
        known = TRUE;
        if (sobj->spe)
            pline(
    "This seems to be junk mail addressed to the finder of the Eye of Larn.");
        /* note to the puzzled: the game Larn actually sends you junk
         * mail if you win!
         */
        else
            readmail(sobj);
        break;
#endif
    case SCR_ENCHANT_ARMOR: {
        register schar s;
        boolean special_armor;
        boolean same_color;

        otmp = some_armor(&youmonst);
        if (!otmp) {
            strange_feeling(sobj, !Blind
                                      ? "你的皮肤发光然后暗淡了."
                                      : "你的皮肤片刻感觉到了温暖.");
            sobj = 0; /* useup() in strange_feeling() */
            exercise(A_CON, !scursed);
            exercise(A_STR, !scursed);
            break;
        }
        if (confused) {
            old_erodeproof = (otmp->oerodeproof != 0);
            new_erodeproof = !scursed;
            otmp->oerodeproof = 0; /* for messages */
            if (Blind) {
                otmp->rknown = FALSE;
                pline("%s温暖了片刻.", Yobjnam2(otmp, "感觉"));
            } else {
                otmp->rknown = TRUE;
                pline("%s被%s %s %s覆盖!", Yobjnam2(otmp, "是"),
                      scursed ? "斑驳的" : "闪烁的",
                      hcolor(scursed ? NH_BLACK : NH_GOLDEN),
                      scursed ? "光芒"
                              : (is_shield(otmp) ? "膜" : "防护物"));
            }
            if (new_erodeproof && (otmp->oeroded || otmp->oeroded2)) {
                otmp->oeroded = otmp->oeroded2 = 0;
                pline("%s像新的一样好!",
                      Yobjnam2(otmp, Blind ? "感觉" : "看起来"));
            }
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                otmp->oerodeproof = 1;
                costly_alteration(otmp, COST_DEGRD);
            }
            otmp->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        /* elven armor vibrates warningly when enchanted beyond a limit */
        special_armor = is_elven_armor(otmp)
                        || (Role_if(PM_WIZARD) && otmp->otyp == CORNUTHAUM);
        if (scursed)
            same_color = (otmp->otyp == BLACK_DRAGON_SCALE_MAIL
                          || otmp->otyp == BLACK_DRAGON_SCALES);
        else
            same_color = (otmp->otyp == SILVER_DRAGON_SCALE_MAIL
                          || otmp->otyp == SILVER_DRAGON_SCALES
                          || otmp->otyp == SHIELD_OF_REFLECTION);
        if (Blind)
            same_color = FALSE;

        /* KMH -- catch underflow */
        s = scursed ? -otmp->spe : otmp->spe;
        if (s > (special_armor ? 5 : 3) && rn2(s)) {
            otmp->in_use = TRUE;
            pline("%s猛烈地%s%s%s了一会儿, 然后%s了.", Yname2(otmp),
                  otense(otmp, Blind ? "振动" : "发出"),
                  (!Blind && !same_color) ? " " : "",
                  (Blind || same_color) ? "" : hcolor(scursed ? NH_BLACK
                                                              : NH_SILVER),
                  otense(otmp, "消失"));
            remove_worn_item(otmp, FALSE);
            useup(otmp);
            break;
        }
        s = scursed ? -1 : otmp->spe >= 9
                               ? (rn2(otmp->spe) == 0)
                               : sblessed ? rnd(3 - otmp->spe / 3) : 1;
        if (s >= 0 && Is_dragon_scales(otmp)) {
            /* dragon scales get turned into dragon scale mail */
            pline("%s合并并硬化!", Yname2(otmp));
            setworn((struct obj *) 0, W_ARM);
            /* assumes same order */
            otmp->otyp += GRAY_DRAGON_SCALE_MAIL - GRAY_DRAGON_SCALES;
            if (sblessed) {
                otmp->spe++;
                if (!otmp->blessed)
                    bless(otmp);
            } else if (otmp->cursed)
                uncurse(otmp);
            otmp->known = 1;
            setworn(otmp, W_ARM);
            if (otmp->unpaid)
                alter_cost(otmp, 0L); /* shop bill */
            break;
        }
        pline("%s %s%s%s%s 了%s.", Yname2(otmp),
              s == 0 ? "猛烈地 " : "",
              otense(otmp, Blind ? "振动" : "发出"),
              (!Blind && !same_color) ? " " : "",
              (Blind || same_color) ? ""
                                    : hcolor(scursed ? NH_BLACK : NH_SILVER),
              (s * s > 1) ? "一会儿" : "片刻");
        /* [this cost handling will need updating if shop pricing is
           ever changed to care about curse/bless status of armor] */
        if (s < 0)
            costly_alteration(otmp, COST_DECHNT);
        if (scursed && !otmp->cursed)
            curse(otmp);
        else if (sblessed && !otmp->blessed)
            bless(otmp);
        if (s) {
            otmp->spe += s;
            adj_abon(otmp, s);
            known = otmp->known;
            /* update shop bill to reflect new higher price */
            if (s > 0 && otmp->unpaid)
                alter_cost(otmp, 0L);
        }

        if ((otmp->spe > (special_armor ? 5 : 3))
            && (special_armor || !rn2(7)))
            pline("%s%s振动.", Yobjnam2(otmp, "突然"),
                  Blind ? "再次" : "意外的");
        break;
    }
    case SCR_DESTROY_ARMOR: {
        otmp = some_armor(&youmonst);
        if (confused) {
            if (!otmp) {
                strange_feeling(sobj, "你的骨头发痒.");
                sobj = 0; /* useup() in strange_feeling() */
                exercise(A_STR, FALSE);
                exercise(A_CON, FALSE);
                break;
            }
            old_erodeproof = (otmp->oerodeproof != 0);
            new_erodeproof = scursed;
            otmp->oerodeproof = 0; /* for messages */
            p_glow2(otmp, NH_PURPLE);
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                otmp->oerodeproof = 1;
                costly_alteration(otmp, COST_DEGRD);
            }
            otmp->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        if (!scursed || !otmp || !otmp->cursed) {
            if (!destroy_arm(otmp)) {
                strange_feeling(sobj, "你的皮肤发痒.");
                sobj = 0; /* useup() in strange_feeling() */
                exercise(A_STR, FALSE);
                exercise(A_CON, FALSE);
                break;
            } else
                known = TRUE;
        } else { /* armor and scroll both cursed */
            pline("%s.", Yobjnam2(otmp, "振动"));
            if (otmp->spe >= -6) {
                otmp->spe += -1;
                adj_abon(otmp, -1);
            }
            make_stunned((HStun & TIMEOUT) + (long) rn1(10, 10), TRUE);
        }
    } break;
    case SCR_CONFUSE_MONSTER:
    case SPE_CONFUSE_MONSTER:
        if (youmonst.data->mlet != S_HUMAN || scursed) {
            if (!HConfusion)
                You_feel("混乱的.");
            make_confused(HConfusion + rnd(100), FALSE);
        } else if (confused) {
            if (!sblessed) {
                Your("%s 开始%s%s.", makeplural(body_part(HAND)),
                     Blind ? "感到刺痛" : "发出",
                     Blind ? "" : hcolor(NH_PURPLE));
                make_confused(HConfusion + rnd(100), FALSE);
            } else {
                pline("%s%s 围绕在你的%s上.",
                      Blind ? "" : hcolor(NH_RED),
                      Blind ? "微弱的嗡嗡声" : "光芒", body_part(HEAD));
                make_confused(0L, TRUE);
            }
        } else {
            if (!sblessed) {
                Your("%s%s %s%s.", makeplural(body_part(HAND)),
                     Blind ? "" : "开始发出",
                     Blind ? (const char *) "刺痛得" : hcolor(NH_RED),
                     u.umconf ? " 更厉害了" : "光芒");
                u.umconf++;
            } else {
                if (Blind)
                    Your("%s 急剧刺痛得%s.", makeplural(body_part(HAND)),
                         u.umconf ? "更厉害了" : "非常厉害");
                else
                    Your("%s 发出%s灿烂的%s光芒.",
                         makeplural(body_part(HAND)),
                         u.umconf ? "更加" : "", hcolor(NH_RED));
                /* after a while, repeated uses become less effective */
                if (u.umconf >= 40)
                    u.umconf++;
                else
                    u.umconf += rn1(8, 2);
            }
        }
        break;
    case SCR_SCARE_MONSTER:
    case SPE_CAUSE_FEAR: {
        register int ct = 0;
        register struct monst *mtmp;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (cansee(mtmp->mx, mtmp->my)) {
                if (confused || scursed) {
                    mtmp->mflee = mtmp->mfrozen = mtmp->msleeping = 0;
                    mtmp->mcanmove = 1;
                } else if (!resist(mtmp, sobj->oclass, 0, NOTELL))
                    monflee(mtmp, 0, FALSE, FALSE);
                if (!mtmp->mtame)
                    ct++; /* pets don't laugh at you */
            }
        }
        if (otyp == SCR_SCARE_MONSTER || !ct)
            You_hear("%s %s.", (confused || scursed) ? "伤心的哭声"
                                                     : "疯狂的笑声",
                     !ct ? "在远处" : "在附近");
        break;
    }
    case SCR_BLANK_PAPER:
        if (Blind)
            You("不记得这张卷轴有任何咒语.");
        else
            pline("这张卷轴似乎是空白的.");
        known = TRUE;
        break;
    case SCR_REMOVE_CURSE:
    case SPE_REMOVE_CURSE: {
        register struct obj *obj;

        You_feel(!Hallucination
                     ? (!confused ? "像是有人在帮助你."
                                  : "像是你需要帮助.")
                     : (!confused ? "与宇宙合一了."
                                  : "原力在和你作对!"));

        if (scursed) {
            pline_The("卷轴破裂了.");
        } else {
            for (obj = invent; obj; obj = obj->nobj) {
                long wornmask;
                /* gold isn't subject to cursing and blessing */
                if (obj->oclass == COIN_CLASS)
                    continue;
                wornmask = (obj->owornmask & ~(W_BALL | W_ART | W_ARTI));
                if (wornmask && !sblessed) {
                    /* handle a couple of special cases; we don't
                       allow auxiliary weapon slots to be used to
                       artificially increase number of worn items */
                    if (obj == uswapwep) {
                        if (!u.twoweap)
                            wornmask = 0L;
                    } else if (obj == uquiver) {
                        if (obj->oclass == WEAPON_CLASS) {
                            /* mergeable weapon test covers ammo,
                               missiles, spears, daggers & knives */
                            if (!objects[obj->otyp].oc_merge)
                                wornmask = 0L;
                        } else if (obj->oclass == GEM_CLASS) {
                            /* possibly ought to check whether
                               alternate weapon is a sling... */
                            if (!uslinging())
                                wornmask = 0L;
                        } else {
                            /* weptools don't merge and aren't
                               reasonable quivered weapons */
                            wornmask = 0L;
                        }
                    }
                }
                if (sblessed || wornmask || obj->otyp == LOADSTONE
                    || (obj->otyp == LEASH && obj->leashmon)) {
                    /* water price varies by curse/bless status */
                    boolean shop_h2o =
                        (obj->unpaid && obj->otyp == POT_WATER);

                    if (confused) {
                        blessorcurse(obj, 2);
                        /* lose knowledge of this object's curse/bless
                           state (even if it didn't actually change) */
                        obj->bknown = 0;
                        /* blessorcurse() only affects uncursed items
                           so no need to worry about price of water
                           going down (hence no costly_alteration) */
                        if (shop_h2o && (obj->cursed || obj->blessed))
                            alter_cost(obj, 0L); /* price goes up */
                    } else if (obj->cursed) {
                        if (shop_h2o)
                            costly_alteration(obj, COST_UNCURS);
                        uncurse(obj);
                    }
                }
            }
        }
        if (Punished && !confused)
            unpunish();
        if (u.utrap && u.utraptype == TT_BURIEDBALL) {
            buried_ball_to_freedom();
            pline_The("你%s上的夹子消失了.", body_part(LEG));
        }
        update_inventory();
        break;
    }
    case SCR_CREATE_MONSTER:
    case SPE_CREATE_MONSTER:
        if (create_critters(1 + ((confused || scursed) ? 12 : 0)
                                + ((sblessed || rn2(73)) ? 0 : rnd(4)),
                            confused ? &mons[PM_ACID_BLOB]
                                     : (struct permonst *) 0,
                            FALSE))
            known = TRUE;
        /* no need to flush monsters; we ask for identification only if the
         * monsters are not visible
         */
        break;
    case SCR_ENCHANT_WEAPON:
        if (confused && uwep
            && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))) {
            old_erodeproof = (uwep->oerodeproof != 0);
            new_erodeproof = !scursed;
            uwep->oerodeproof = 0; /* for messages */
            if (Blind) {
                uwep->rknown = FALSE;
                Your("武器感到了片刻温暖.");
            } else {
                uwep->rknown = TRUE;
                pline("%s 被%s %s %s覆盖了!", Yobjnam2(uwep, "是"),
                      scursed ? "斑驳的" : "闪烁的",
                      hcolor(scursed ? NH_PURPLE : NH_GOLDEN),
                      scursed ? "光芒" : "防护物");
            }
            if (new_erodeproof && (uwep->oeroded || uwep->oeroded2)) {
                uwep->oeroded = uwep->oeroded2 = 0;
                pline("%s 像新的一样好!",
                      Yobjnam2(uwep, Blind ? "感觉" : "看起来"));
            }
            if (old_erodeproof && !new_erodeproof) {
                /* restore old_erodeproof before shop charges */
                uwep->oerodeproof = 1;
                costly_alteration(uwep, COST_DEGRD);
            }
            uwep->oerodeproof = new_erodeproof ? 1 : 0;
            break;
        }
        if (!chwepon(sobj,
                     scursed
                         ? -1
                         : !uwep ? 1 : (uwep->spe >= 9)
                                           ? !rn2(uwep->spe)
                                           : sblessed ? rnd(3 - uwep->spe / 3)
                                                      : 1))
            sobj = 0; /* nothing enchanted: strange_feeling -> useup */
        break;
    case SCR_TAMING:
    case SPE_CHARM_MONSTER: {
        int candidates, res, results, vis_results;

        if (u.uswallow) {
            candidates = 1;
            results = vis_results = maybe_tame(u.ustuck, sobj);
        } else {
            int i, j, bd = confused ? 5 : 1;
            struct monst *mtmp;

            /* note: maybe_tame() can return either positive or
               negative values, but not both for the same scroll */
            candidates = results = vis_results = 0;
            for (i = -bd; i <= bd; i++)
                for (j = -bd; j <= bd; j++) {
                    if (!isok(u.ux + i, u.uy + j))
                        continue;
                    if ((mtmp = m_at(u.ux + i, u.uy + j)) != 0
                        || (!i && !j && (mtmp = u.usteed) != 0)) {
                        ++candidates;
                        res = maybe_tame(mtmp, sobj);
                        results += res;
                        if (canspotmon(mtmp))
                            vis_results += res;
                    }
                }
        }
        if (!results) {
            pline("没有有趣的事情%s.",
                  !candidates ? "发生" : "似乎要发生");
        } else {
            pline_The("附近的%s %s友好的.",
                      vis_results ? "是" : "似乎",
                      (results < 0) ? "不" : "");
            if (vis_results > 0)
                known = TRUE;
        }
    } break;
    case SCR_GENOCIDE:
        if (!already_known)
            You("发现了灭绝卷轴!");
        known = TRUE;
        if (sblessed)
            do_class_genocide();
        else
            do_genocide(!scursed | (2 * !!Confusion));
        break;
    case SCR_LIGHT:
        if (!confused || rn2(5)) {
            if (!Blind)
                known = TRUE;
            litroom(!confused && !scursed, sobj);
            if (!confused && !scursed) {
                if (lightdamage(sobj, TRUE, 5))
                    known = TRUE;
            }
        } else {
            /* could be scroll of create monster, don't set known ...*/
            (void) create_critters(1, !scursed ? &mons[PM_YELLOW_LIGHT]
                                               : &mons[PM_BLACK_LIGHT],
                                   TRUE);
            if (!objects[sobj->otyp].oc_uname)
                docall(sobj);
        }
        break;
    case SCR_TELEPORTATION:
        if (confused || scursed) {
            level_tele();
        } else {
            known = scrolltele(sobj);
        }
        break;
    case SCR_GOLD_DETECTION:
        if ((confused || scursed) ? trap_detect(sobj) : gold_detect(sobj))
            sobj = 0; /* failure: strange_feeling() -> useup() */
        break;
    case SCR_FOOD_DETECTION:
    case SPE_DETECT_FOOD:
        if (food_detect(sobj))
            sobj = 0; /* nothing detected: strange_feeling -> useup */
        break;
    case SCR_IDENTIFY:
        /* known = TRUE; -- handled inline here */
        /* use up the scroll first, before makeknown() performs a
           perm_invent update; also simplifies empty invent check */
        useup(sobj);
        sobj = 0; /* it's gone */
        if (confused)
            You("确定这是一张鉴定卷轴.");
        else if (!already_known || !invent)
            /* force feedback now if invent became
               empty after using up this scroll */
            pline("这是一张鉴定卷轴.");
        if (!already_known)
            (void) learnscrolltyp(SCR_IDENTIFY);
        /*FALLTHRU*/
    case SPE_IDENTIFY:
        cval = 1;
        if (sblessed || (!scursed && !rn2(5))) {
            cval = rn2(5);
            /* note: if cval==0, identify all items */
            if (cval == 1 && sblessed && Luck > 0)
                ++cval;
        }
        if (invent && !confused) {
            identify_pack(cval, !already_known);
        } else if (otyp == SPE_IDENTIFY) {
            /* when casting a spell we know we're not confused,
               so inventory must be empty (another message has
               already been given above if reading a scroll) */
            pline("你没有携带任何待鉴定的东西.");
        }
        break;
    case SCR_CHARGING:
        if (confused) {
            if (scursed) {
                You_feel("泄气了.");
                u.uen = 0;
            } else {
                You_feel("充满了能量!");
                u.uen += d(sblessed ? 6 : 4, 4);
                if (u.uen > u.uenmax) /* if current energy is already at   */
                    u.uenmax = u.uen; /* or near maximum, increase maximum */
                else
                    u.uen = u.uenmax; /* otherwise restore current to max  */
            }
            context.botl = 1;
            break;
        }
        /* known = TRUE; -- handled inline here */
        if (!already_known) {
            pline("这是一张充能卷轴.");
            learnscroll(sobj);
        }
        /* use it up now to prevent it from showing in the
           getobj picklist because the "disappears" message
           was already delivered */
        useup(sobj);
        sobj = 0; /* it's gone */
        otmp = getobj(all_count, "充能");  //charge
        if (otmp)
            recharge(otmp, scursed ? -1 : sblessed ? 1 : 0);
        break;
    case SCR_MAGIC_MAPPING:
        if (level.flags.nommap) {
            Your("脑中充满了不可思议的路线!");
            if (Hallucination)
                pline("哇!  现代艺术.");
            else
                Your("%s 混乱地旋转.", body_part(HEAD));
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        if (sblessed) {
            register int x, y;

            for (x = 1; x < COLNO; x++)
                for (y = 0; y < ROWNO; y++)
                    if (levl[x][y].typ == SDOOR)
                        cvt_sdoor_to_door(&levl[x][y]);
            /* do_mapping() already reveals secret passages */
        }
        known = TRUE;
    case SPE_MAGIC_MAPPING:
        if (level.flags.nommap) {
            Your("%s 旋转得就像%s使咒语成了块!", body_part(HEAD),
                 something);
            make_confused(HConfusion + rnd(30), FALSE);
            break;
        }
        pline("地图在你的脑中合并!");
        cval = (scursed && !confused);
        if (cval)
            HConfusion = 1; /* to screw up map */
        do_mapping();
        if (cval) {
            HConfusion = 0; /* restore */
            pline("不幸的是, 你无法掌握细节.");
        }
        break;
    case SCR_AMNESIA:
        known = TRUE;
        forget((!sblessed ? ALL_SPELLS : 0)
               | (!confused || scursed ? ALL_MAP : 0));
        if (Hallucination) /* Ommmmmm! */
            Your("思想从世俗的关注中释放出来.");
        else if (!strncmpi(plname, "Maud", 4))
            pline(
          "当你的内心转向内在, 你忘记了一切.");
        else if (rn2(2))
            pline("莫德到底是谁?");
        else
            pline("你想起了莫德而忘记了一切.");
        exercise(A_WIS, FALSE);
        break;
    case SCR_FIRE:
        cval = bcsign(sobj);
        useup(sobj);
        sobj = 0; /* it's gone */
        if (!already_known)
            (void) learnscrolltyp(SCR_FIRE);
        if (confused) {
            if (Fire_resistance) {
                shieldeff(u.ux, u.uy);
                if (!Blind)
                    pline("哦, 看, 多么漂亮的火在你的%s上.",
                          makeplural(body_part(HAND)));
                else
                    You_feel("一种舒适的温暖在你的%s上.",
                             makeplural(body_part(HAND)));
            } else {
                pline_The("卷轴着火了并烧到了你的%s.",
                          makeplural(body_part(HAND)));
                losehp(1, "火卷轴", KILLED_BY_AN);
            }
            break;
        }
        if (Underwater) {
            pline_The("你周围的水猛烈地蒸发!");
        } else {
            pline_The("卷轴喷出火焰塔!");
            iflags.last_msg = PLNMSG_TOWER_OF_FLAME; /* for explode() */
            burn_away_slime();
        }
        explode(u.ux, u.uy, 11, (2 * (rn1(3, 3) + 2 * cval) + 1) / 3,
                SCROLL_CLASS, EXPL_FIERY);
        break;
    case SCR_EARTH:
        /* TODO: handle steeds */
        if (!Is_rogue_level(&u.uz) && has_ceiling(&u.uz)
            && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
            register int x, y;
            int nboulders = 0;

            /* Identify the scroll */
            if (u.uswallow)
                You_hear("隆隆声.");
            else
                pline_The("%s在你的%s隆隆作响!", ceiling(u.ux, u.uy),
                          sblessed ? "附近" : "上面");
            known = 1;
            sokoban_guilt();

            /* Loop through the surrounding squares */
            if (!scursed)
                for (x = u.ux - 1; x <= u.ux + 1; x++) {
                    for (y = u.uy - 1; y <= u.uy + 1; y++) {
                        /* Is this a suitable spot? */
                        if (isok(x, y) && !closed_door(x, y)
                            && !IS_ROCK(levl[x][y].typ)
                            && !IS_AIR(levl[x][y].typ)
                            && (x != u.ux || y != u.uy)) {
                            nboulders +=
                                drop_boulder_on_monster(x, y, confused, TRUE);
                        }
                    }
                }
            /* Attack the player */
            if (!sblessed) {
                drop_boulder_on_player(confused, !scursed, TRUE, FALSE);
            } else if (!nboulders)
                pline("但没有其他事情发生.");
        }
        break;
    case SCR_PUNISHMENT:
        known = TRUE;
        if (confused || sblessed) {
            You_feel("有罪的.");
            break;
        }
        punish(sobj);
        break;
    case SCR_STINKING_CLOUD: {
        coord cc;

        if (!already_known)
            You("发现了一张臭云卷轴!");
        known = TRUE;
        pline("你想在哪儿放出%s云?",
              already_known ? "臭 " : "");
        cc.x = u.ux;
        cc.y = u.uy;
        getpos_sethilite(display_stinking_cloud_positions);
        if (getpos(&cc, TRUE, "期望的位置") < 0) {
            pline1(Never_mind);
            break;
        }
        if (!is_valid_stinking_cloud_pos(cc.x, cc.y, TRUE))
            break;
        (void) create_gas_cloud(cc.x, cc.y, 3 + bcsign(sobj),
                                8 + 4 * bcsign(sobj));
        break;
    }
    default:
        impossible("What weird effect is this? (%u)", otyp);
    }
    return sobj ? 0 : 1;
}

void
drop_boulder_on_player(confused, helmet_protects, byu, skip_uswallow)
boolean confused, helmet_protects, byu, skip_uswallow;
{
    int dmg;
    struct obj *otmp2;

    /* hit monster if swallowed */
    if (u.uswallow && !skip_uswallow) {
        drop_boulder_on_monster(u.ux, u.uy, confused, byu);
        return;
    }

    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return;
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);
    if (!amorphous(youmonst.data) && !Passes_walls
        && !noncorporeal(youmonst.data) && !unsolid(youmonst.data)) {
        You("被%s打中了!", doname(otmp2));
        dmg = dmgval(otmp2, &youmonst) * otmp2->quan;
        if (uarmh && helmet_protects) {
            if (is_metallic(uarmh)) {
                pline("幸运的是, 你穿戴着一顶坚硬的头盔.");
                if (dmg > 2)
                    dmg = 2;
            } else if (flags.verbose) {
                pline("%s 没有保护到你.", Yname2(uarmh));
            }
        }
    } else
        dmg = 0;
    /* Must be before the losehp(), for bones files */
    if (!flooreffects(otmp2, u.ux, u.uy, "掉落")) {
        place_object(otmp2, u.ux, u.uy);
        stackobj(otmp2);
        newsym(u.ux, u.uy);
    }
    if (dmg)
        losehp(Maybe_Half_Phys(dmg), "大地卷轴", KILLED_BY_AN);
}

boolean
drop_boulder_on_monster(x, y, confused, byu)
int x, y;
boolean confused, byu;
{
    register struct obj *otmp2;
    register struct monst *mtmp;

    /* Make the object(s) */
    otmp2 = mksobj(confused ? ROCK : BOULDER, FALSE, FALSE);
    if (!otmp2)
        return FALSE; /* Shouldn't happen */
    otmp2->quan = confused ? rn1(5, 2) : 1;
    otmp2->owt = weight(otmp2);

    /* Find the monster here (won't be player) */
    mtmp = m_at(x, y);
    if (mtmp && !amorphous(mtmp->data) && !passes_walls(mtmp->data)
        && !noncorporeal(mtmp->data) && !unsolid(mtmp->data)) {
        struct obj *helmet = which_armor(mtmp, W_ARMH);
        int mdmg;

        if (cansee(mtmp->mx, mtmp->my)) {
            pline("%s 被%s打中!", Monnam(mtmp), doname(otmp2));
            if (mtmp->minvis && !canspotmon(mtmp))
                map_invisible(mtmp->mx, mtmp->my);
        } else if (u.uswallow && mtmp == u.ustuck)
            You_hear("什么东西打中了%s %s 在你的%s上!",
                     s_suffix(mon_nam(mtmp)), mbodypart(mtmp, STOMACH),
                     body_part(HEAD));

        mdmg = dmgval(otmp2, mtmp) * otmp2->quan;
        if (helmet) {
            if (is_metallic(helmet)) {
                if (canspotmon(mtmp))
                    pline("幸运的是, %s 穿戴着一顶坚硬的头盔.",
                          mon_nam(mtmp));
                else if (!Deaf)
                    You_hear("铿锵声.");
                if (mdmg > 2)
                    mdmg = 2;
            } else {
                if (canspotmon(mtmp))
                    pline("%s 的%s没有保护到%s.", Monnam(mtmp),
                          xname(helmet), mhim(mtmp));
            }
        }
        mtmp->mhp -= mdmg;
        if (mtmp->mhp <= 0) {
            if (byu)
                xkilled(mtmp, 1);
            else {
                pline("%s 被杀死了.", Monnam(mtmp));
                mondied(mtmp);
            }
        }
    } else if (u.uswallow && mtmp == u.ustuck) {
        obfree(otmp2, (struct obj *) 0);
        /* fall through to player */
        drop_boulder_on_player(confused, TRUE, FALSE, TRUE);
        return 1;
    }
    /* Drop the rock/boulder to the floor */
    if (!flooreffects(otmp2, x, y, "fall")) {
        place_object(otmp2, x, y);
        stackobj(otmp2);
        newsym(x, y); /* map the rock */
    }
    return TRUE;
}

/* overcharging any wand or zapping/engraving cursed wand */
void
wand_explode(obj, chg)
struct obj *obj;
int chg; /* recharging */
{
    const char *expl = !chg ? "突然" : "剧烈地振动并";
    int dmg, n, k;

    /* number of damage dice */
    if (!chg)
        chg = 2; /* zap/engrave adjustment */
    n = obj->spe + chg;
    if (n < 2)
        n = 2; /* arbitrary minimum */
    /* size of damage dice */
    switch (obj->otyp) {
    case WAN_WISHING:
        k = 12;
        break;
    case WAN_CANCELLATION:
    case WAN_DEATH:
    case WAN_POLYMORPH:
    case WAN_UNDEAD_TURNING:
        k = 10;
        break;
    case WAN_COLD:
    case WAN_FIRE:
    case WAN_LIGHTNING:
    case WAN_MAGIC_MISSILE:
        k = 8;
        break;
    case WAN_NOTHING:
        k = 4;
        break;
    default:
        k = 6;
        break;
    }
    /* inflict damage and destroy the wand */
    dmg = d(n, k);
    obj->in_use = TRUE; /* in case losehp() is fatal (or --More--^C) */
    pline("%s %s 爆炸了!", Yname2(obj), expl);
    losehp(Maybe_Half_Phys(dmg), "爆炸的魔杖", KILLED_BY_AN);
    useup(obj);
    /* obscure side-effect */
    exercise(A_STR, FALSE);
}

/* used to collect gremlins being hit by light so that they can be processed
   after vision for the entire lit area has been brought up to date */
struct litmon {
    struct monst *mon;
    struct litmon *nxt;
};
STATIC_VAR struct litmon *gremlins = 0;

/*
 * Low-level lit-field update routine.
 */
STATIC_PTR void
set_lit(x, y, val)
int x, y;
genericptr_t val;
{
    struct monst *mtmp;
    struct litmon *gremlin;

    if (val) {
        levl[x][y].lit = 1;
        if ((mtmp = m_at(x, y)) != 0 && mtmp->data == &mons[PM_GREMLIN]) {
            gremlin = (struct litmon *) alloc(sizeof *gremlin);
            gremlin->mon = mtmp;
            gremlin->nxt = gremlins;
            gremlins = gremlin;
        }
    } else {
        levl[x][y].lit = 0;
        snuff_light_source(x, y);
    }
}

void
litroom(on, obj)
register boolean on;
struct obj *obj;
{
    char is_lit; /* value is irrelevant; we use its address
                    as a `not null' flag for set_lit() */

    /* first produce the text (provided you're not blind) */
    if (!on) {
        register struct obj *otmp;

        if (!Blind) {
            if (u.uswallow) {
                pline("在这里似乎比以前更黑暗了.");
            } else {
                if (uwep && artifact_light(uwep) && uwep->lamplit)
                    pline("突然, 唯一剩下的光来自%s!",
                          the(xname(uwep)));
                else
                    You("被黑暗所包围!");
            }
        }

        /* the magic douses lamps, et al, too */
        for (otmp = invent; otmp; otmp = otmp->nobj)
            if (otmp->lamplit)
                (void) snuff_lit(otmp);
    } else { /* on */
        if (u.uswallow) {
            if (Blind)
                ; /* no feedback */
            else if (is_animal(u.ustuck->data))
                pline("%s %s 被照亮的.", s_suffix(Monnam(u.ustuck)),
                      mbodypart(u.ustuck, STOMACH));
            else if (is_whirly(u.ustuck->data))
                pline("%s 短暂地照亮.", Monnam(u.ustuck));
            else
                pline("%s 闪烁.", Monnam(u.ustuck));
        } else if (!Blind)
            pline("照亮的区域围绕着你!");
    }

    /* No-op when swallowed or in water */
    if (u.uswallow || Underwater || Is_waterlevel(&u.uz))
        return;
    /*
     *  If we are darkening the room and the hero is punished but not
     *  blind, then we have to pick up and replace the ball and chain so
     *  that we don't remember them if they are out of sight.
     */
    if (Punished && !on && !Blind)
        move_bc(1, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);

    if (Is_rogue_level(&u.uz)) {
        /* Can't use do_clear_area because MAX_RADIUS is too small */
        /* rogue lighting must light the entire room */
        int rnum = levl[u.ux][u.uy].roomno - ROOMOFFSET;
        int rx, ry;

        if (rnum >= 0) {
            for (rx = rooms[rnum].lx - 1; rx <= rooms[rnum].hx + 1; rx++)
                for (ry = rooms[rnum].ly - 1; ry <= rooms[rnum].hy + 1; ry++)
                    set_lit(rx, ry,
                            (genericptr_t) (on ? &is_lit : (char *) 0));
            rooms[rnum].rlit = on;
        }
        /* hallways remain dark on the rogue level */
    } else
        do_clear_area(u.ux, u.uy,
                      (obj && obj->oclass == SCROLL_CLASS && obj->blessed)
                         ? 9 : 5,
                      set_lit, (genericptr_t) (on ? &is_lit : (char *) 0));

    /*
     *  If we are not blind, then force a redraw on all positions in sight
     *  by temporarily blinding the hero.  The vision recalculation will
     *  correctly update all previously seen positions *and* correctly
     *  set the waslit bit [could be messed up from above].
     */
    if (!Blind) {
        vision_recalc(2);

        /* replace ball&chain */
        if (Punished && !on)
            move_bc(0, 0, uball->ox, uball->oy, uchain->ox, uchain->oy);
    }

    vision_full_recalc = 1; /* delayed vision recalculation */
    if (gremlins) {
        struct litmon *gremlin;

        /* can't delay vision recalc after all */
        vision_recalc(0);
        /* after vision has been updated, monsters who are affected
           when hit by light can now be hit by it */
        do {
            gremlin = gremlins;
            gremlins = gremlin->nxt;
            light_hits_gremlin(gremlin->mon, rnd(5));
            free((genericptr_t) gremlin);
        } while (gremlins);
    }
}

STATIC_OVL void
do_class_genocide()
{
    int i, j, immunecnt, gonecnt, goodcnt, class, feel_dead = 0;
    char buf[BUFSZ];
    boolean gameover = FALSE; /* true iff killed self */

    for (j = 0;; j++) {
        if (j >= 5) {
            pline1(thats_enough_tries);
            return;
        }
        do {
            getlin("你想灭绝哪类怪物?", buf);
            (void) mungspaces(buf);
        } while (!*buf);
        /* choosing "none" preserves genocideless conduct */
        if (*buf == '\033' || !strcmpi(buf, "none")
            || !strcmpi(buf, "nothing"))
            return;

        class = name_to_monclass(buf, (int *) 0);
        if (class == 0 && (i = name_to_mon(buf)) != NON_PM)
            class = mons[i].mlet;
        immunecnt = gonecnt = goodcnt = 0;
        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                if (!(mons[i].geno & G_GENO))
                    immunecnt++;
                else if (mvitals[i].mvflags & G_GENOD)
                    gonecnt++;
                else
                    goodcnt++;
            }
        }
        if (!goodcnt && class != mons[urole.malenum].mlet
            && class != mons[urace.malenum].mlet) {
            if (gonecnt)
                pline("所有的这种怪物都已经不存在了.");
            else if (immunecnt || class == S_invisible)
                You("不被允许灭绝这种怪物.");
            else if (wizard && buf[0] == '*') {
                register struct monst *mtmp, *mtmp2;

                gonecnt = 0;
                for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                    mtmp2 = mtmp->nmon;
                    if (DEADMONSTER(mtmp))
                        continue;
                    mongone(mtmp);
                    gonecnt++;
                }
                pline("消除了%d 个怪物.", gonecnt);
                return;
            } else
                pline("那个%s不代表任何怪物.",
                      strlen(buf) == 1 ? "字符" : "回答");
            continue;
        }

        for (i = LOW_PM; i < NUMMONS; i++) {
            if (mons[i].mlet == class) {
                char nam[BUFSZ];

                Strcpy(nam, makeplural(mons[i].mname));
                /* Although "genus" is Latin for race, the hero benefits
                 * from both race and role; thus genocide affects either.
                 */
                if (Your_Own_Role(i) || Your_Own_Race(i)
                    || ((mons[i].geno & G_GENO)
                        && !(mvitals[i].mvflags & G_GENOD))) {
                    /* This check must be first since player monsters might
                     * have G_GENOD or !G_GENO.
                     */
                    mvitals[i].mvflags |= (G_GENOD | G_NOCORPSE);
                    reset_rndmonst(i);
                    kill_genocided_monsters();
                    update_inventory(); /* eggs & tins */
                    pline("清除了所有的%s.", nam);
                    if (Upolyd && i == u.umonnum) {
                        u.mh = -1;
                        if (Unchanging) {
                            if (!feel_dead++)
                                You("死了.");
                            /* finish genociding this class of
                               monsters before ultimately dying */
                            gameover = TRUE;
                        } else
                            rehumanize();
                    }
                    /* Self-genocide if it matches either your race
                       or role.  Assumption:  male and female forms
                       share same monster class. */
                    if (i == urole.malenum || i == urace.malenum) {
                        u.uhp = -1;
                        if (Upolyd) {
                            if (!feel_dead++)
                                You_feel("内心已死.");
                        } else {
                            if (!feel_dead++)
                                You("死了.");
                            gameover = TRUE;
                        }
                    }
                } else if (mvitals[i].mvflags & G_GENOD) {
                    if (!gameover)
                        pline("所有的%s 都已经不存在了.", nam);
                } else if (!gameover) {
                    /* suppress feedback about quest beings except
                       for those applicable to our own role */
                    if ((mons[i].msound != MS_LEADER
                         || quest_info(MS_LEADER) == i)
                        && (mons[i].msound != MS_NEMESIS
                            || quest_info(MS_NEMESIS) == i)
                        && (mons[i].msound != MS_GUARDIAN
                            || quest_info(MS_GUARDIAN) == i)
                        /* non-leader/nemesis/guardian role-specific monster
                           */
                        && (i != PM_NINJA /* nuisance */
                            || Role_if(PM_SAMURAI))) {
                        boolean named, uniq;

                        named = type_is_pname(&mons[i]) ? TRUE : FALSE;
                        uniq = (mons[i].geno & G_UNIQ) ? TRUE : FALSE;
                        /* one special case */
                        if (i == PM_HIGH_PRIEST)
                            uniq = FALSE;

                        You("不可以灭绝%s%s.",
                            (uniq && !named) ? "这个 " : "",
                            (uniq || named) ? mons[i].mname : nam);
                    }
                }
            }
        }
        if (gameover || u.uhp == -1) {
            killer.format = KILLED_BY_AN;
            Strcpy(killer.name, "灭绝卷轴");
            if (gameover)
                done(GENOCIDED);
        }
        return;
    }
}

#define REALLY 1
#define PLAYER 2
#define ONTHRONE 4
void
do_genocide(how)
int how;
/* 0 = no genocide; create monsters (cursed scroll) */
/* 1 = normal genocide */
/* 3 = forced genocide of player */
/* 5 (4 | 1) = normal genocide from throne */
{
    char buf[BUFSZ];
    register int i, killplayer = 0;
    register int mndx;
    register struct permonst *ptr;
    const char *which;

    if (how & PLAYER) {
        mndx = u.umonster; /* non-polymorphed mon num */
        ptr = &mons[mndx];
        Strcpy(buf, ptr->mname);
        killplayer++;
    } else {
        for (i = 0;; i++) {
            if (i >= 5) {
                pline1(thats_enough_tries);
                return;
            }
            getlin("你想灭绝什么怪物? [ 输入名字]",
                   buf);
            (void) mungspaces(buf);
            /* choosing "none" preserves genocideless conduct */
            if (!strcmpi(buf, "none") || !strcmpi(buf, "nothing")) {
                /* ... but no free pass if cursed */
                if (!(how & REALLY)) {
                    ptr = rndmonst();
                    if (!ptr)
                        return; /* no message, like normal case */
                    mndx = monsndx(ptr);
                    break; /* remaining checks don't apply */
                } else
                    return;
            }

            mndx = name_to_mon(buf);
            if (mndx == NON_PM || (mvitals[mndx].mvflags & G_GENOD)) {
                pline("这种生物在这个世界%s存在.",
                      (mndx == NON_PM) ? "不" : "不再");
                continue;
            }
            ptr = &mons[mndx];
            /* Although "genus" is Latin for race, the hero benefits
             * from both race and role; thus genocide affects either.
             */
            if (Your_Own_Role(mndx) || Your_Own_Race(mndx)) {
                killplayer++;
                break;
            }
            if (is_human(ptr))
                adjalign(-sgn(u.ualign.type));
            if (is_demon(ptr))
                adjalign(sgn(u.ualign.type));

            if (!(ptr->geno & G_GENO)) {
                if (!Deaf) {
                    /* fixme: unconditional "caverns" will be silly in some
                     * circumstances */
                    if (flags.verbose)
                        pline(
                            "一个雷鸣般的声音从洞穴里传来:");
                    verbalize("不, 凡人!  那不会实现的.");
                }
                continue;
            }
            /* KMH -- Unchanging prevents rehumanization */
            if (Unchanging && ptr == youmonst.data)
                killplayer++;
            break;
        }
    }

    which = "所有的 ";
    if (Hallucination) {
        if (Upolyd)
            Strcpy(buf, youmonst.data->mname);
        else {
            Strcpy(buf, (flags.female && urole.name.f) ? urole.name.f
                                                       : urole.name.m);
            buf[0] = lowc(buf[0]);
        }
    } else {
        Strcpy(buf, ptr->mname); /* make sure we have standard singular */
        if ((ptr->geno & G_UNIQ) && ptr != &mons[PM_HIGH_PRIEST])
            which = !type_is_pname(ptr) ? "" : "";
    }
    if (how & REALLY) {
        /* setting no-corpse affects wishing and random tin generation */
        mvitals[mndx].mvflags |= (G_GENOD | G_NOCORPSE);
        pline("清除了%s%s.", which,
              (*which != 'a') ? buf : makeplural(buf));

        if (killplayer) {
            /* might need to wipe out dual role */
            if (urole.femalenum != NON_PM && mndx == urole.malenum)
                mvitals[urole.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urole.femalenum != NON_PM && mndx == urole.femalenum)
                mvitals[urole.malenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.malenum)
                mvitals[urace.femalenum].mvflags |= (G_GENOD | G_NOCORPSE);
            if (urace.femalenum != NON_PM && mndx == urace.femalenum)
                mvitals[urace.malenum].mvflags |= (G_GENOD | G_NOCORPSE);

            u.uhp = -1;
            if (how & PLAYER) {
                killer.format = KILLED_BY;
                Strcpy(killer.name, "灭绝混乱");
            } else if (how & ONTHRONE) {
                /* player selected while on a throne */
                killer.format = KILLED_BY_AN;
                Strcpy(killer.name, "专横的命令");
            } else { /* selected player deliberately, not confused */
                killer.format = KILLED_BY_AN;
                Strcpy(killer.name, "灭绝卷轴");
            }

            /* Polymorphed characters will die as soon as they're rehumanized.
             */
            /* KMH -- Unchanging prevents rehumanization */
            if (Upolyd && ptr != youmonst.data) {
                delayed_killer(POLYMORPH, killer.format, killer.name);
                You_feel("内心已死.");
            } else
                done(GENOCIDED);
        } else if (ptr == youmonst.data) {
            rehumanize();
        }
        reset_rndmonst(mndx);
        kill_genocided_monsters();
        update_inventory(); /* in case identified eggs were affected */
    } else {
        int cnt = 0, census = monster_census(FALSE);

        if (!(mons[mndx].geno & G_UNIQ)
            && !(mvitals[mndx].mvflags & (G_GENOD | G_EXTINCT)))
            for (i = rn1(3, 4); i > 0; i--) {
                if (!makemon(ptr, u.ux, u.uy, NO_MINVENT))
                    break; /* couldn't make one */
                ++cnt;
                if (mvitals[mndx].mvflags & G_EXTINCT)
                    break; /* just made last one */
            }
        if (cnt) {
            /* accumulated 'cnt' doesn't take groups into account;
               assume bringing in new mon(s) didn't remove any old ones */
            cnt = monster_census(FALSE) - census;
            pline("派来了%s%s.", (cnt > 1) ? "一些" : "",
                  (cnt > 1) ? makeplural(buf) : buf);
        } else
            pline1(nothing_happens);
    }
}

void
punish(sobj)
struct obj *sobj;
{
    struct obj *reuse_ball = (sobj && sobj->otyp == HEAVY_IRON_BALL)
                                ? sobj : (struct obj *) 0;

    /* KMH -- Punishment is still okay when you are riding */
    if (!reuse_ball)
        You("因你的品行不端而被惩罚!");
    if (Punished) {
        Your("铁球变得沉重些了.");
        uball->owt += 160 * (1 + sobj->cursed);
        return;
    }
    if (amorphous(youmonst.data) || is_whirly(youmonst.data)
        || unsolid(youmonst.data)) {
        if (!reuse_ball) {
            pline("一个球和链出现了, 然后掉落了.");
            dropy(mkobj(BALL_CLASS, TRUE));
        } else {
            dropy(reuse_ball);
        }
        return;
    }
    setworn(mkobj(CHAIN_CLASS, TRUE), W_CHAIN);
    if (!reuse_ball)
        setworn(mkobj(BALL_CLASS, TRUE), W_BALL);
    else
        setworn(reuse_ball, W_BALL);
    uball->spe = 1; /* special ball (see save) */

    /*
     *  Place ball & chain if not swallowed.  If swallowed, the ball &
     *  chain variables will be set at the next call to placebc().
     */
    if (!u.uswallow) {
        placebc();
        if (Blind)
            set_bc(1);      /* set up ball and chain variables */
        newsym(u.ux, u.uy); /* see ball&chain if can't see self */
    }
}

/* remove the ball and chain */
void
unpunish()
{
    struct obj *savechain = uchain;

    obj_extract_self(uchain);
    newsym(uchain->ox, uchain->oy);
    setworn((struct obj *) 0, W_CHAIN);
    dealloc_obj(savechain);
    uball->spe = 0;
    setworn((struct obj *) 0, W_BALL);
}

/* some creatures have special data structures that only make sense in their
 * normal locations -- if the player tries to create one elsewhere, or to
 * revive one, the disoriented creature becomes a zombie
 */
boolean
cant_revive(mtype, revival, from_obj)
int *mtype;
boolean revival;
struct obj *from_obj;
{
    /* SHOPKEEPERS can be revived now */
    if (*mtype == PM_GUARD || (*mtype == PM_SHOPKEEPER && !revival)
        || *mtype == PM_HIGH_PRIEST || *mtype == PM_ALIGNED_PRIEST
        || *mtype == PM_ANGEL) {
        *mtype = PM_HUMAN_ZOMBIE;
        return TRUE;
    } else if (*mtype == PM_LONG_WORM_TAIL) { /* for create_particular() */
        *mtype = PM_LONG_WORM;
        return TRUE;
    } else if (unique_corpstat(&mons[*mtype])
               && (!from_obj || !has_omonst(from_obj))) {
        /* unique corpses (from bones or wizard mode wish) or
           statues (bones or any wish) end up as shapechangers */
        *mtype = PM_DOPPELGANGER;
        return TRUE;
    }
    return FALSE;
}

/*
 * Make a new monster with the type controlled by the user.
 *
 * Note:  when creating a monster by class letter, specifying the
 * "strange object" (']') symbol produces a random monster rather
 * than a mimic.  This behavior quirk is useful so don't "fix" it
 * (use 'm'--or "mimic"--to create a random mimic).
 *
 * Used in wizard mode only (for ^G command and for scroll or spell
 * of create monster).  Once upon a time, an earlier incarnation of
 * this code was also used for the scroll/spell in explore mode.
 */
boolean
create_particular()
{
    char buf[BUFSZ], *bufp, monclass;
    int which, tryct, i, firstchoice = NON_PM;
    struct permonst *whichpm = NULL;
    struct monst *mtmp;
    boolean madeany = FALSE;
    boolean maketame, makepeaceful, makehostile;
    boolean randmonst = FALSE;

    tryct = 5;
    do {
        monclass = MAXMCLASSES;
        which = urole.malenum; /* an arbitrary index into mons[] */
        maketame = makepeaceful = makehostile = FALSE;
        getlin("生成什么样的怪物? [ 输入名字或符号]", buf);
        bufp = mungspaces(buf);
        if (*bufp == '\033')
            return FALSE;
        /* allow the initial disposition to be specified */
        if (!strncmpi(bufp, "驯服的", 9)) {
            bufp += 9;
            maketame = TRUE;
        } else if (!strncmpi(bufp, "和平的", 9)) {
            bufp += 9;
            makepeaceful = TRUE;
        } else if (!strncmpi(bufp, "敌对的", 9)) {
            bufp += 9;
            makehostile = TRUE;
        }
        /* decide whether a valid monster was chosen */
        if (wizard && (!strcmp(bufp, "*") || !strcmp(bufp, "random"))) {
            randmonst = TRUE;
            break;
        }
        which = name_to_mon(bufp);
        if (which >= LOW_PM)
            break; /* got one */
        monclass = name_to_monclass(bufp, &which);
        if (which >= LOW_PM) {
            monclass = MAXMCLASSES; /* matters below */
            break;
        } else if (monclass > 0) {
            which = urole.malenum; /* reset from NON_PM */
            break;
        }
        /* no good; try again... */
        pline("游戏中没有这样的怪物.");
    } while (--tryct > 0);

    if (!tryct) {
        pline1(thats_enough_tries);
    } else {
        if (!randmonst) {
            firstchoice = which;
            if (cant_revive(&which, FALSE, (struct obj *) 0)) {
                /* wizard mode can override handling of special monsters */
                Sprintf(buf, "生成%s 代替; 强行生成%s?",
                        mons[which].mname, mons[firstchoice].mname);
                if (yn(buf) == 'y')
                    which = firstchoice;
            }
            whichpm = &mons[which];
        }
        for (i = 0; i <= multi; i++) {
            if (monclass != MAXMCLASSES)
                whichpm = mkclass(monclass, 0);
            else if (randmonst)
                whichpm = rndmonst();
            mtmp = makemon(whichpm, u.ux, u.uy, NO_MM_FLAGS);
            if (!mtmp) {
                /* quit trying if creation failed and is going to repeat */
                if (monclass == MAXMCLASSES && !randmonst)
                    break;
                /* otherwise try again */
                continue;
            }
            if (maketame) {
                (void) tamedog(mtmp, (struct obj *) 0);
            } else if (makepeaceful || makehostile) {
                mtmp->mtame = 0; /* sanity precaution */
                mtmp->mpeaceful = makepeaceful ? 1 : 0;
                set_malign(mtmp);
            }
            madeany = TRUE;
            /* in case we got a doppelganger instead of what was asked
               for, make it start out looking like what was asked for */
            if (mtmp->cham != NON_PM && firstchoice != NON_PM
                && mtmp->cham != firstchoice)
                (void) newcham(mtmp, &mons[firstchoice], FALSE, FALSE);
        }
    }
    return madeany;
}

/*read.c*/
