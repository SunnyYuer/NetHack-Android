package com.nethackff;

public class Chinese {
	/*
	 * 首先，这不是一个好方法。
	 * 更改NetHack源文件的输出信息会输出乱码
	 * 本人不会转换编码，不得已用此方法。
	 * Yuer
	 */
	private int pro=0;  //运行流程

	public String replace(String s)
	{
		if(s.length()==0) return s;
		if(s.matches(".{1,2}[0-9]{1,2};[0-9]{1,2}.{1,2}")) return s;
		System.out.println(s.length()+s);
		s=gameStart(s);
		if(pro>=10&&pro<20) s=gameIn(s);
		if(pro>=20) s=gameEnd(s);
		return s;
	}
	
	private String gameStart(String s)
	{
		int t=-1;
		t=s.indexOf("Who are you");
		if(t>=0) pro=1;
		t=s.indexOf("Shall I pick");
		if(t>=0) pro=1;
		t=s.indexOf("There are files");
		if(t>=0) pro=1;
		t=s.indexOf("Checkpointing was");
		if(t>=0) pro=1;
		t=s.indexOf("Pick a role");
		if(t>=0) pro=2;
		t=s.indexOf("Pick the race");
		if(t>=0) pro=3;
		t=s.indexOf("Pick the gender");
		if(t>=0) pro=4;
		t=s.indexOf("Pick the alignment");
		if(t>=0) pro=5;
		t=s.indexOf("It is wr");
		if(t>=0) pro=6;
		t=s.indexOf("a newly");
		if(t>=0) pro=6;
		t=s.indexOf("Go bravely with");
		if(t>=0) pro=6;
		t=s.indexOf("Aloha");
		if(t>=0) pro=24;
		t=s.indexOf("welcome");
		if(t>=0) pro=7;
		t=s.indexOf("tonight");
		if(t>=0) pro=7;
		t=s.indexOf("St:");
		if(t>=0) pro=7;
		t=s.indexOf("Dlvl:");
		if(t>=0) pro=7;
		t=s.indexOf("Do you");
		if(t>=0) pro=20;
		t=s.indexOf("Final Att");
		if(t>=0) pro=21;
		t=s.indexOf("Vanquished");
		if(t>=0) pro=22;
		t=s.indexOf("Voluntary");
		if(t>=0) pro=23;
		t=s.indexOf("Goodbye");
		if(t>=0) pro=24;
		t=s.indexOf("Fare thee well");
		if(t>=0) pro=24;
		t=s.indexOf("Sayonara");
		if(t>=0) pro=24;
		t=s.indexOf("when you");
		if(t>=0) pro=24;
		t=s.indexOf("No  P");
		if(t>=0) pro=25;
		t=s.indexOf("Killed by");
		if(t>=0) pro=25;
		
		if(pro==1)
		{
			s=s.replaceAll("Who are you", "你 叫 什 么 ");
			s=s.replaceAll("Shall I pick a character's race, role, gender and alignment for you", 
				"要 我 为 你 随 机 选 择 角 色 的 职 业 、 种 族 、 性 别 和 阵 营 吗 ");
			s=s.replaceAll("There are files from a game in progress under your name. Recover", 
					"这 里 有 你 名 下 的 游 戏 保 存 记 录 ， 要 恢 复 吗 ");
			s=s.replaceAll("Checkpointing was not in effect for ", 
					"检 查 发 现 该 文 件 无 效 ");
			s=s.replaceAll("recovery impossible", 
					"恢 复 失 败 ");
			s=s.replaceAll("Couldn't recover old game. Destroy it", 
					"无 法 恢 复 。 要 删 除 吗 ");
		}
		if(pro==2)
		{
			s=s.replaceAll("Pick a role for your character", "选 择 职 业 ");
			s=s.replaceAll("- a ", "- ");
			s=s.replaceAll("- an ", "- ");
			s=character(s);
			s=s.replaceAll("Random", "随 机 ");
			s=s.replaceAll("Quit", "退 出 ");
		}
		if(pro==3)
		{
			s=s.replaceAll("Pick the race of your ", "选 择 种 族 -- 为 你 的 ");
			s=s.replaceAll("- a ", "- ");
			s=s.replaceAll("- an ", "- ");
			s=character(s);
			s=s.replaceAll("Random", "随 机 ");
			s=s.replaceAll("Quit", "退 出 ");
		}
		if(pro==4)
		{
			s=s.replaceAll("Pick the gender of your ", "选 择 性 别 -- 为 你 的 ");
			s=s.replaceAll("- a ", "- ");
			s=s.replaceAll("- an ", "- ");
			s=character(s);
			s=s.replaceAll("Random", "随 机 ");
			s=s.replaceAll("Quit", "退 出 ");
		}
		if(pro==5)
		{
			s=s.replaceAll("Pick the alignment of your ", "选 择 阵 营 -- 为 你 的 ");
			s=s.replaceAll("- a ", "- ");
			s=s.replaceAll("- an ", "- ");
			s=character(s);
			s=s.replaceAll("Random", "随 机 ");
			s=s.replaceAll("Quit", "退 出 ");
		}
		if(pro==6)
		{
			if(NetHackGame.pres!=null)
			{
				s=NetHackGame.pres+s;
				NetHackGame.pres=null;
				//System.out.println(s);
			}
			if(s.indexOf("It is")==-1||s.indexOf("--More--")==-1)
			{
				NetHackGame.pres=s;
				s="";
				return s;
			}
			s=s.replaceAll("It is written in the Book of ", "这 是 写 在 书 名 为 ");
			s=s.replaceAll("After the Creation, the cruel god Moloch rebelled", 
					"天 地 创 造 之 初 ， 破 坏 神 摩 洛 密 谋 策 划 ");
			s=s.replaceAll("against the authority of Marduk the Creator.", 
					"叛 变 创 世 神 马 尔 杜 克 。 ");
			s=s.replaceAll("Moloch stole from Marduk the most powerful of all", 
					"摩 洛 从 马 尔 杜 克 的 众 多 神 器 中 偷 走 了 ");
			s=s.replaceAll("the artifacts of the gods, the Amulet of Yendor,", 
					"最 为 强 大 的 岩 德 护 身 符 ， ");
			s=s.replaceAll("and he hid it in the dark cavities of Gehennom, the", 
					"并 将 其 藏 在 暗 黑 地 狱 葛 汉 诺 姆 ， ");
			s=s.replaceAll("Under World, where he now lurks, and bides his time.", 
					"暗 中 积 蓄 力 量 以 待 时 机 的 成 熟 。 ");
			s=s.replaceAll("Your god ", "你 的 神 ");
			s=s.replaceAll("Your goddess ", "你 的 女 神 ");
			s=s.replaceAll(" seeks to possess the Amulet, and with it", 
					"想 要 得 到 这 传 说 中 的 护 身 符 ， 有 此 神 器 为 助 ");
			s=s.replaceAll("to gain deserved ascendance over the other gods.", 
					"便 能 在 众 神 内 稳 占 主 导 地 位 。 ");
			s=s.replaceAll("You, a newly trained ", "你 现 在 还 只 是 个 新 手 ");
			s=s.replaceAll(", have been heralded", "， 出 生 起 从 小 ");
			s=s.replaceAll("from birth as the instrument of ", "就 被 教 导 成 为 ");
			s=s.replaceAll(".  You are destined", "的 忠 实 奴 仆 。 ");
			s=s.replaceAll("to recover the Amulet for your deity, or die in the", 
					"除 非 死 亡 ， 你 注 定 要 为 你 的 神 取 回 岩 德 护 身 符 。 ");
			s=s.replaceAll("attempt.  Your hour of destiny has come.  For the sake", 
					"命 运 的 一 刻 终 于 来 临 。 ");
			s=s.replaceAll("of us all:  Go bravely with ", 
					"为 了 我 们 所 有 人 ： 勇 敢 地 前 进 吧 。 ");
			s=god(s);
			s=newly(s);
			s=s.replaceAll("More", "更 多 ");
		}
		if(pro==7)
		{
			s=s.replaceAll("Velkommen ", "您 好 ");
			s=s.replaceAll("Hello ", "您 好 ");
			s=s.replaceAll("Aloha ", "您 好 ");
			s=s.replaceAll("Konnichi wa ", "您 好 ");
			s=s.replaceAll("Salutations ", "您 好 ");
			s=s.replaceAll("welcome to NetHack! You are a ", 
					"欢 迎 来 到 迷 宫 骇 客 ！ 您 是 一 位 ");
			s=s.replaceAll("welcome back to", "欢 迎 回 到 ");
			s=s.replaceAll("NetHack", "迷 宫 骇 客 ");
			s=s.replaceAll("Be careful!", "要 小 心 ！");
			s=s.replaceAll("Becareful!", "要 小 心 ！");
			s=s.replaceAll("New moon tonight.", "今 晚 新 月 。 ");
			s=s.replaceAll("the ", "");
			s=character(s);
			s=att(s);
		}
		if(pro==0) pro=10;
		return s;
	}
	
	private String gameIn(String s)
	{
		int t=-1;
		t=s.indexOf("Weapons");
		if(t>=0) s=weapon(s);
		t=s.indexOf("Armor");
		if(t>=0) s=armor(s);
		t=s.indexOf("Comestibles");
		if(t>=0) s=comesti(s);
		t=s.indexOf("Scrolls");
		if(t>=0) s=scroll(s);
		t=s.indexOf("Spellbooks");
		if(t>=0) s=spellbook(s);
		t=s.indexOf("Potions");
		if(t>=0) s=potion(s);
		t=s.indexOf("Rings");
		if(t>=0) s=ring(s);
		t=s.indexOf("Wands");
		if(t>=0) s=wand(s);
		t=s.indexOf("Tools");
		if(t>=0) s=tool(s);
		t=s.indexOf("Gems");
		if(t>=0) s=gem(s);
		t=s.indexOf("Statues");
		if(t>=0) s=statue(s);
		t=s.indexOf("Currently known spells");
		if(t>=0) pro=11;
		t=s.indexOf("Choose which spell");
		if(t>=0) pro=11;
		t=s.indexOf("Base Att");
		if(t>=0) pro=12;
		
		if(pro==11)
		{
			s=spell(s);
		}
		if(pro==12)
		{
			s=s.replaceAll("Base Attributes", "基 本 属 性 ");
			s=s.replaceAll("Starting", "起 始.");
			s=s.replaceAll("Current", "当 前.");
			s=s.replaceAll("Deities", "神 祗.");
			s=s.replaceAll("name", "名 字 ");
			s=s.replaceAll("race", "种 族 ");
			s=s.replaceAll("role", "职 业 ");
			s=s.replaceAll("gender", "性 别   ");
			s=s.replaceAll("alignment", "阵 营      ");
			s=character(s);
			s=s.replaceAll("Chaotic", "混 沌    ");
			s=s.replaceAll("Neutral", "中 立    ");
			s=s.replaceAll("Lawful", "秩 序   ");
			s=god(s);
		}
		s=info(s);
		return s;
	}
	
	private String gameEnd(String s)
	{
		s=s.replaceAll("Do you want your", "你 想 确 认 下 ");
		s=s.replaceAll("possessions", "你 的 ");
		s=s.replaceAll("identified", "物 品 吗 ");
		s=s.replaceAll("Do you want to see your attributes", "你 想 看 下 你 的 能 力 吗 ");
		s=s.replaceAll("Do you want an account of creatures vanquished", 
				"你 想 看 下 你 消 灭 怪 物 吗 ");
		s=s.replaceAll("Do youwant an account of creatures vanquished", 
				"你 想 看 下 你 消 灭 怪 物 吗 ");
		s=s.replaceAll("Do you want to see your conduct", "你 想 看 下 你 的 成 就 吗 ");
		s=s.replaceAll("Do youwant to see your conduct", "你 想 看 下 你 的 成 就 吗 ");
		
		if(pro==21)
		{
			s=s.replaceAll("Final Attributes", "最 终 能 力 ");
		}
		if(pro==22)
		{
			s=s.replaceAll("Vanquished creatures", "被 消 灭 的 怪 物 ");
			s=monster(s);
			s=s.replaceAll("creatures vanquished", "被 消 灭 的 怪 物 ");
		}
		if(pro==23)
		{
			s=s.replaceAll("Voluntary challenges", "成 就 挑 战 ");
		}
		if(pro==24)
		{
			if(NetHackGame.pres!=null)
			{
				s=NetHackGame.pres+s;
				NetHackGame.pres=null;
			}
			if(s.indexOf("...")==-1||s.indexOf("--More--")==-1)
			{
				int t=s.indexOf("level");
				if(t==-1) return s;
				NetHackGame.pres=s;
				s="";
				return s;
			}
			s=s.replaceAll(" killed by a", "被 如 下 怪 所 杀 ");
			s=s.replaceAll("Goodbye ", "再 见 ");
			s=s.replaceAll("Fare thee well ", "再 见 ");
			s=s.replaceAll("Aloha ", "再 见 ");
			s=s.replaceAll("Sayonara ", "再 见 ");
			s=s.replaceAll("the ", "");
			s=character(s);
			s=s.replaceAll("You quit in The Dungeons of Doom ", 
					"你 从 末 日 地 牢 中 退 出 了 。 ");
			s=s.replaceAll("You died in The Dungeons of Doom ", 
					"你 死 在 末 日 地 牢 中 。 ");
			s=s.replaceAll("on dungeon level ", "地 牢 层 数 ");
			s=s.replaceAll("with ", "有 ");
			s=s.replaceAll("and ", "和 ");
			s=s.replaceAll(" pieces of gold", "金 币 ");
			s=s.replaceAll("after ", "走 了 ");
			s=s.replaceAll(" moves", "步 ");
			s=s.replaceAll(" move", "步 ");
			s=s.replaceAll("You were level ", "你 的 等 级 ");
			s=s.replaceAll("a maximum of ", "最 大 ");
			s=s.replaceAll(" hit points ", "HP  ");
			s=s.replaceAll(" points", "点 数 ");
			s=s.replaceAll("when you died", "当 你 死 的 时 候 ");
			s=s.replaceAll("when you quit", "当 你 退 出 的 时 候 ");
			s=s.replaceAll("REST", " 安  ");
			s=s.replaceAll("IN", "息 ");
			s=s.replaceAll("PEACE", "  吧 ");
			s=s.replaceAll("Au", "金 ");
			s=monster(s);
		}
		if(pro==25)
		{
			s=s.replaceAll("You made the top ten list", "你 进 入 了 前 十 ");
			s=s.replaceAll("No  ", "名 次 ");
			s=s.replaceAll("Points    ", "      分 数 ");
			s=s.replaceAll("Name", "名 字 ");
			s=chalist(s);
			s=s.replaceAll("quit in The Dungeons of Doom on", 
					"从 末 日 地 牢 中 退 出 了 。 ");
			s=s.replaceAll("died in The Dungeons of Doom on", 
					"死 在 末 日 地 牢 中 。 ");
			s=s.replaceAll("level ", "层 数 ");
			s=s.replaceAll("Killed by a ", "被 该 怪 物 所 杀 ");
		}
		s=s.replaceAll("More", "更 多 ");
		return s;
	}
	
	private String att(String s)
	{
		s=s.replaceAll("St:", "力 :");
		s=s.replaceAll("Dx:", "敏 :");
		s=s.replaceAll("Co:", "体 :");
		s=s.replaceAll("In:", "智 :");
		s=s.replaceAll("Wi:", "感 :");
		//s=s.replaceAll("Ch:", "魅 :"); //无法替换掉，为什么？
		//s=s.replaceAll("Lawful", "秩 序");
		//s=s.replaceAll("Neutral", "中 立");
		//s=s.replaceAll("Chaotic", "混 沌");
		s=s.replaceAll("Dlvl:", "层 数 :");
		s=s.replaceAll("Pw:", "Mp:");
		//s=s.replaceAll("AC:", "防 :");
		s=s.replaceAll("Exp:", " Lv:");
		//s=s.replaceAll("Satiated", "吃 饱 了 ");
		return s;
	}
	
	private String weapon(String s)
	{
		s=s.replaceAll("Weapons", "武 器.");
		s=s.replaceAll("weapon in hands", "双 手 拿 着 ");
		s=s.replaceAll("weapon in hand", "拿 着 ");
		s=s.replaceAll("alternate weapon; not wielded", "备 用 ");
		s=s.replaceAll("in quiver", "准 备 ");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String armor(String s)
	{
		s=s.replaceAll("Armor", "防 具.");
		s=s.replaceAll("being worn", "穿 着 ");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String comesti(String s)
	{
		s=s.replaceAll("Comestibles", "食 物.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String scroll(String s)
	{
		s=s.replaceAll("Scrolls", "卷 轴.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String spellbook(String s)
	{
		s=s.replaceAll("Spellbooks", "魔 法 书.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String potion(String s)
	{
		s=s.replaceAll("Potions", "药 水.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String ring(String s)
	{
		s=s.replaceAll("Rings", "戒 指.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=s.replaceAll("on right hand", "右 手 上 ");
		s=s.replaceAll("on left hand", "左 手 上 ");
		s=state(s);
		return s;
	}
	
	private String wand(String s)
	{
		s=s.replaceAll("Wands", "魔 杖.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String tool(String s)
	{
		s=s.replaceAll("Tools", "工 具.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String gem(String s)
	{
		s=s.replaceAll("Gems", "宝 石.");
		s=s.replaceAll("in quiver", "准 备 ");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String statue(String s)
	{
		s=s.replaceAll("Boulders/Statues", "巨 石 / 雕 像.");
		s=s.replaceAll("- an ", "- ");
		s=s.replaceAll("- a ", "- ");
		s=state(s);
		return s;
	}
	
	private String spell(String s)
	{
		s=s.replaceAll("Currently known spells", "已 学 魔 法 ");
		s=s.replaceAll("Choose which spell to cast", "选 择 要 施 展 的 魔 法 ");
		s=s.replaceAll("Name", "名 字 ");
		s=s.replaceAll("Level", "等 级  ");
		s=s.replaceAll("Category", "种 类 ");
		s=s.replaceAll("Fail", "失 败 几 率 ");
		s=s.replaceAll("attack", "攻 击   ");
		s=s.replaceAll("healing", "治 愈    ");
		s=s.replaceAll("clerical", "神 圣     ");
		return s;
	}
	
	private String state(String s)
	{
		s=s.replaceAll("blessed", "祝 福 的 ");
		s=s.replaceAll("uncursed", "未 诅 咒 的 ");
		s=s.replaceAll("cursed", "诅 咒 的 ");
		return s;
	}
	
	private String hear(String s)
	{
		s=s.replaceAll("You hear some noises in the distance", "你 听 到 远 处 有 一 些 声 响 ");
		s=s.replaceAll("You hear some noises", "你 听 到 一 些 声 响 ");
		s=s.replaceAll("You hear bubbling water", "你 听 到 潺 潺 的 水 声 ");
		s=s.replaceAll("You hear water falling on coins", "你 听 到 水 滴 在 金 币 上 的 声 音 ");
		s=s.replaceAll("You hear a door open", "你 听 到 有 门 开 的 声 音 ");
		return s;
	}
	
	private String door(String s)
	{
		s=s.replaceAll("The door opens", "门 开 了 ");
		s=s.replaceAll("The door closes", "门 关 了 ");
		s=s.replaceAll("The door resists", "有 些 难 ");
		s=s.replaceAll("This door is already open", "门 已 经 开 了 ");
		s=s.replaceAll("This door is already closed", "门 已 经 关 了 ");
		s=s.replaceAll("As you kick the door, it crashes open", "你 把 门 踢 破 了 ");
		return s;
	}
	
	private String boulder(String s)
	{
		s=s.replaceAll("You try to move the boulder, but in vain", 
				"你 试 图 移 动 巨 石 ， 但 是 没 用 ");
		s=s.replaceAll("With great effort you move the boulder", 
				"你 尽 全 力 移 动 巨 石 ");
		return s;
	}
	
	private String pet(String s)
	{
		s=s.replaceAll("displaced ", "换 了 个 位 置 和 ");
		s=s.replaceAll("picks up", "捡 起 了 ");
		s=s.replaceAll("drops", "放 下 了 ");
		s=s.replaceAll("You stop", "你 停 了 下 来 ");
		s=s.replaceAll("Your ", "你 的 ");
		s=s.replaceAll("your ", "你 的 ");
		s=s.replaceAll("is in the way", "挡 在 路 上 ");
		return s;
	}
	
	private String attack(String s)
	{
		s=s.replaceAll("You ", "你 ");
		s=s.replaceAll("just ", "");
		s=s.replaceAll(" misses", "没 打 到 ");
		s=s.replaceAll("miss ", "没 打 到 ");
		s=s.replaceAll(" bites", "咬 了 一 口 ");
		s=s.replaceAll("hits", "打 了 一 下 ");
		s=s.replaceAll("are hit ", "被 打 中 ");
		s=s.replaceAll("hit ", "打 了 一 下 ");
		s=s.replaceAll("killed", "被 杀 死 了 ");
		s=s.replaceAll("kill ", "杀 死 了 ");
		s=s.replaceAll("destroy ", "消 灭 了 ");
		s=s.replaceAll("die", "死 了 ");
		return s;
	}
	
	private String monster(String s)
	{
		s=s.replaceAll("The ", "");
		s=s.replaceAll("the ", "");
		s=s.replaceAll("grid bug", "电 子 虫 ");
		s=s.replaceAll("newt", "蝾 螈 ");
		s=s.replaceAll("fox", "狐 狸 ");
		s=s.replaceAll("kobold zombie", "小 鬼 僵 尸 ");
		s=s.replaceAll("goblin", "小 妖 精 ");
		s=s.replaceAll("giant rat", "大 鼠 ");
		s=s.replaceAll("gecko", "壁 虎 ");
		s=s.replaceAll("lichen", "地 衣 ");
		s=s.replaceAll("iguana", "蜥 蜴 ");
		return s;
	}
	
	private String info(String s)
	{
		int t=-1;
		s=s.replaceAll("Really save", "保 存 退 出 ");
		s=s.replaceAll("Really quit", "确 定 退 出 ");
		s=s.replaceAll("In what direction", "哪 个 方 向 ");
		s=s.replaceAll("More", "更 多 ");
		s=s.replaceAll(" gold pieces", "金 币 ");
		s=s.replaceAll("see here ", "见 这 里 有 ");
		s=s.replaceAll("Things that are here", "这 里 有 ");
		s=s.replaceAll("Welcome to experience level ", "恭 喜 升 级 到 ");
		s=s.replaceAll("There's some graffiti on the floor here", "地 板 上 写 有 字 ");
		
		t=s.indexOf("hear");
		if(t>=0)
		{
			s=hear(s);
		}
		t=s.indexOf("door");
		if(t>=0)
		{
			s=door(s);
		}
		t=s.indexOf("boulder");
		if(t>=0)
		{
			s=boulder(s);
		}
		s=pet(s);
		s=monster(s);
		s=attack(s);
		return s;
	}
	
	private String chalist(String s)
	{
		s=s.replaceAll("Hum", "人 类 ");
		s=s.replaceAll("Dwa", "矮 人 ");
		s=s.replaceAll("Elf", "精 灵 ");
		s=s.replaceAll("Gno", "侏 儒 ");
		s=s.replaceAll("Orc", "兽 人 ");
		s=s.replaceAll("Fem", "女 ");
		s=s.replaceAll("Mal", "男 ");
		s=s.replaceAll("Cha", "混 沌 ");
		s=s.replaceAll("Law", "秩 序 ");
		s=s.replaceAll("Neu", "中 立 ");
		s=s.replaceAll("Arc", "考 古 学 家 ");
		s=s.replaceAll("Bar", "野 蛮 人 ");
		s=s.replaceAll("Cav", "穴 居 人 ");
		s=s.replaceAll("Hea", "医 生 ");
		s=s.replaceAll("Kni", "骑 士 ");
		s=s.replaceAll("Mon", "僧 侣 ");
		s=s.replaceAll("Pri", "牧 师 ");
		s=s.replaceAll("Rog", "盗 贼 ");
		s=s.replaceAll("Ran", "游 侠 ");
		s=s.replaceAll("Sam", "武 士 ");
		s=s.replaceAll("Tou", "旅 行 者 ");
		s=s.replaceAll("Val", "女 武 神 ");
		s=s.replaceAll("Wiz", "巫 师 ");
		return s;
	}
	
	private String character(String s)
	{
		s=s.replaceAll("human", "人 类");
		s=s.replaceAll("dwarf", "矮 人");
		s=s.replaceAll("dwarven", "矮 人");
		s=s.replaceAll("elf", "精 灵");
		s=s.replaceAll("elven", "精 灵");
		s=s.replaceAll("gnome", "侏 儒");
		s=s.replaceAll("gnomish", "侏 儒");
		s=s.replaceAll("orcish", "兽 人");
		s=s.replaceAll("orc", "兽 人");
		s=s.replaceAll("female", "女 性 ");
		s=s.replaceAll("male", "男 性");
		s=s.replaceAll("lawful", "秩 序");
		s=s.replaceAll("neutral", "中 立");
		s=s.replaceAll("chaotic", "混 沌");
		s=s.replaceAll("Archeologist", "考 古 学 家 ");
		s=s.replaceAll("Barbarian", "野 蛮 人 ");
		s=s.replaceAll("Caveman/Cavewoman", "穴 居 人 ");
		s=s.replaceAll("Caveman", "穴 居 人 ");
		s=s.replaceAll("Cavewoman", "穴 居 人 ");
		s=s.replaceAll("Healer", "医 生 ");
		s=s.replaceAll("Knight", "骑 士 ");
		s=s.replaceAll("Monk", "僧 侣 ");
		s=s.replaceAll("Priest/Priestess", "牧 师 ");
		s=s.replaceAll("Priestess", "牧 师 ");
		s=s.replaceAll("Priest", "牧 师 ");
		s=s.replaceAll("Rogue", "盗 贼 ");
		s=s.replaceAll("Ranger", "游 侠 ");
		s=s.replaceAll("Samurai", "武 士 ");
		s=s.replaceAll("Tourist", "旅 行 者 ");
		s=s.replaceAll("Valkyrie", "女 武 神 ");
		s=s.replaceAll("Wizard", "巫 师 ");
		return s;
	}
	
	private String newly(String s)
	{
		s=s.replaceAll("Digger", "挖 掘 者 ");
		s=s.replaceAll("Plunderer", "掠 夺 者 ");
		s=s.replaceAll("Plunderess", "掠 夺 者 ");
		s=s.replaceAll("Troglodyte", "穴 居 者 ");
		s=s.replaceAll("Rhizotomist", "草 药 师 ");
		s=s.replaceAll("Gallant", "豪 侠 ");
		s=s.replaceAll("Candidate", "小 和 尚 ");
		s=s.replaceAll("Aspirant", "野 心 家 ");
		s=s.replaceAll("Footpad", "拦 路 贼 ");
		s=s.replaceAll("Tenderfoot", "初 学 者 ");
		s=s.replaceAll("Hatamoto", "旗 本 ");
		s=s.replaceAll("Rambler", "漫 步 者 ");
		s=s.replaceAll("Stripling", "年 轻 人 ");
		s=s.replaceAll("Evoker", "魔 法 导 师 ");
		return s;
	}
	
	private String god(String s)
	{
		s=s.replaceAll("Quetzalcoatl:", "羽 蛇 神 之 中 ： ");
		s=s.replaceAll("Quetzalcoatl!", "羽 蛇 神 与 你 同 在 ！ ");
		s=s.replaceAll("Quetzalcoatl", "羽 蛇 神 ");
		s=s.replaceAll("Camaxtli:", "卡 玛 瑟 特 利 之 中 ： ");
		s=s.replaceAll("Camaxtli!", "卡 玛 瑟 特 利 与 你 同 在 ！ ");
		s=s.replaceAll("Camaxtli", "卡 玛 瑟 特 利 ");
		s=s.replaceAll("Crom:", "克 罗 姆 之 中 ： ");
		s=s.replaceAll("Crom!", "克 罗 姆 与 你 同 在 ！ ");
		s=s.replaceAll("Crom", "克 罗 姆 ");
		s=s.replaceAll("Set:", "赛 特 之 中 ： ");
		s=s.replaceAll("Set!", "赛 特 与 你 同 在 ！ ");
		s=s.replaceAll("Set", "赛 特 ");
		s=s.replaceAll("Anu:", "安 努 之 中 ： ");
		s=s.replaceAll("Anu!", "安 努 与 你 同 在 ！ ");
		s=s.replaceAll("Anu", "安 努 ");
		s=s.replaceAll("Ishtar:", "伊 师 塔 之 中 ： ");
		s=s.replaceAll("Ishtar!", "伊 师 塔 与 你 同 在 ！ ");
		s=s.replaceAll("Ishtar", "伊 师 塔 ");
		s=s.replaceAll("Hermes:", "赫 耳 墨 斯 之 中 ： ");
		s=s.replaceAll("Hermes!", "赫 耳 墨 斯 与 你 同 在 ！ ");
		s=s.replaceAll("Hermes", "赫 耳 墨 斯 ");
		s=s.replaceAll("Lugh:", "鲁 格 之 中 ： ");
		s=s.replaceAll("Lugh!", "鲁 格 与 你 同 在 ！ ");
		s=s.replaceAll("Lugh", "鲁 格 ");
		s=s.replaceAll("Shan Lai Ching:", "山 雷 精 之 中 ： ");
		s=s.replaceAll("Shan Lai Ching!", "山 雷 精 与 你 同 在 ！ ");
		s=s.replaceAll("Shan Lai Ching", "山 雷 精 ");
		s=s.replaceAll("Chih Sung-tzu:", "赤 松 子 之 中 ： ");
		s=s.replaceAll("Chih Sung-tzu!", "赤 松 子 与 你 同 在 ！ ");
		s=s.replaceAll("Chih Sung-tzu", "赤 松 子 ");
		s=s.replaceAll("Huan Ti:", "黄 帝 之 中 ： ");
		s=s.replaceAll("Huan Ti!", "黄 帝 与 你 同 在 ！ ");
		s=s.replaceAll("Huan Ti", "黄 帝 ");
		s=s.replaceAll("Ptah:", "卜 塔 之 中 ： ");
		s=s.replaceAll("Ptah!", "卜 塔 与 你 同 在 ！ ");
		s=s.replaceAll("Ptah", "卜 塔 ");
		s=s.replaceAll("Mercury:", "墨 丘 利 之 中 ： ");
		s=s.replaceAll("Mercury!", "墨 丘 利 与 你 同 在 ！ ");
		s=s.replaceAll("Mercury", "墨 丘 利 ");
		s=s.replaceAll("Venus:", "维 纳 斯 之 中 ： ");
		s=s.replaceAll("Venus!", "维 纳 斯 与 你 同 在 ！ ");
		s=s.replaceAll("Venus", "维 纳 斯 ");
		s=s.replaceAll("Manannan Mac Lir:", "玛 娜 曼 · 麦 克 · 利 尔 之 中 ： ");
		s=s.replaceAll("Manannan Mac Lir!", "玛 娜 曼 · 麦 克 · 利 尔 与 你 同 在 ！ ");
		s=s.replaceAll("Manannan Mac Lir", "玛 娜 曼 · 麦 克 · 利 尔 ");
		s=s.replaceAll("Susanowo:", "须 佐 之 男 之 中 ： ");
		s=s.replaceAll("Susanowo!", "须 佐 之 男 与 你 同 在 ！ ");
		s=s.replaceAll("Susanowo", "须 佐 之 男 ");
		s=s.replaceAll("Offler:", "昂 福 尔 之 中 ： ");
		s=s.replaceAll("Offler!", "昂 福 尔 与 你 同 在 ！ ");
		s=s.replaceAll("Offler", "昂 福 尔 ");
		s=s.replaceAll("Huhetotl:", "修 堤 库 特 里 之 中 ： ");
		s=s.replaceAll("Huhetotl!", "修 堤 库 特 里 与 你 同 在 ！ ");
		s=s.replaceAll("Huhetotl", "修 堤 库 特 里 ");
		s=s.replaceAll("Mars:", "马 耳 斯 之 中 ： ");
		s=s.replaceAll("Mars!", "马 耳 斯 与 你 同 在 ！ ");
		s=s.replaceAll("Mars", "马 耳 斯 ");
		s=s.replaceAll("Athena:", "雅 典 娜 之 中 ： ");
		s=s.replaceAll("Athena!", "雅 典 娜 与 你 同 在 ！ ");
		s=s.replaceAll("Athena", "雅 典 娜 ");
		s=s.replaceAll("Amaterasu Omikami:", "天 照 女 神 之 中 ： ");
		s=s.replaceAll("Amaterasu Omikami!", "天 照 女 神 与 你 同 在 ！ ");
		s=s.replaceAll("Amaterasu Omikami", "天 照 女 神 ");
		s=s.replaceAll("Blind Io:", "盲 木 卫 之 中 ： ");
		s=s.replaceAll("Blind Io!", "盲 木 卫 与 你 同 在 ！ ");
		s=s.replaceAll("Blind Io", "盲 木 卫 ");
		s=s.replaceAll("The Lady:", "昂 山 素 姬 之 中 ： ");
		s=s.replaceAll("The Lady!", "昂 山 素 姬 与 你 同 在 ！ ");
		s=s.replaceAll("The Lady", "昂 山 素 姬 ");
		s=s.replaceAll("Brigit:", "布 里 吉 特 之 中 ： ");
		s=s.replaceAll("Brigit!", "布 里 吉 特 与 你 同 在 ！ ");
		s=s.replaceAll("Brigit", "布 里 吉 特 ");
		s=s.replaceAll("Kos:", "科 斯 之 中 ： ");
		s=s.replaceAll("Kos!", "科 斯 与 你 同 在 ！ ");
		s=s.replaceAll("Kos", "科 斯 ");
		s=s.replaceAll("Mitra:", "米 特 拉 之 中 ： ");
		s=s.replaceAll("Mitra!", "米 特 拉 与 你 同 在 ！ ");
		s=s.replaceAll("Mitra", "米 特 拉 ");
		s=s.replaceAll("Raijin:", "雷 神 之 中 ： ");
		s=s.replaceAll("Raijin!", "雷 神 与 你 同 在 ！ ");
		s=s.replaceAll("Raijin", "雷 神 ");
		s=s.replaceAll("Mog:", "莫 格 之 中 ： ");
		s=s.replaceAll("Mog!", "莫 格 与 你 同 在 ！ ");
		s=s.replaceAll("Mog", "莫 格 ");
		s=s.replaceAll("Thoth:", "透 特 之 中 ： ");
		s=s.replaceAll("Thoth!", "透 特 与 你 同 在 ！ ");
		s=s.replaceAll("Thoth", "透 特 ");
		s=s.replaceAll("Anshar:", "安 沙 尔 之 中 ： ");
		s=s.replaceAll("Anshar!", "安 沙 尔 与 你 同 在 ！ ");
		s=s.replaceAll("Anshar", "安 沙 尔 ");
		s=s.replaceAll("Poseidon:", "波 塞 冬 之 中 ： ");
		s=s.replaceAll("Poseidon!", "波 塞 冬 与 你 同 在 ！ ");
		s=s.replaceAll("Poseidon", "波 塞 冬 ");
		s=s.replaceAll("Loki:", "洛 基 之 中 ： ");
		s=s.replaceAll("Loki!", "洛 基 与 你 同 在 ！ ");
		s=s.replaceAll("Loki", "洛 基 ");
		s=s.replaceAll("Tyr:", "蒂 尔 之 中 ： ");
		s=s.replaceAll("Tyr!", "蒂 尔 与 你 同 在 ！ ");
		s=s.replaceAll("Tyr", "蒂 尔 ");
		s=s.replaceAll("Odin:", "欧 丁 神 之 中 ： ");
		s=s.replaceAll("Odin!", "欧 丁 神 与 你 同 在 ！ ");
		s=s.replaceAll("Odin", "欧 丁 神 ");
		s=s.replaceAll("Anhur:", "安 赫 之 中 ： ");
		s=s.replaceAll("Anhur!", "安 赫 与 你 同 在 ！ ");
		s=s.replaceAll("Anhur", "安 赫 ");
		return s;
	}
}
