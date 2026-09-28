// ==========================================================================
// CLab_CocaTray: maceration tray for up to 12 virtual coca buds soaked in gasoline.
// Buds are a counter plus model selections, never real items, so vanilla cooking never
// sees cargo in the tray; Cooking.CookWithEquipment diverts the tray to CLab_CookStep.
// The server owns the state. Clients receive the derived values as synchronized
// variables and only drive visuals, names and action conditions from them.
// ==========================================================================

class CLab_CocaTray: Bottle_Base
{
	static const int MAX_BUDS = 12;
	//! Most bags one tray can give: a bag per bud (BudsPerBag 1) with both yields at 1
	static const int MAX_BAGS = 12;
	//! A bag stack left on the ground is filled further only this close to the scraping player, in metres
	static const float OPEN_BAG_DISTANCE = 2.0;
	//! Mod storage schema version, independent from the engine storage version passed to OnStoreLoad
	static const int STORE_VERSION = 1;
	static const float TICK_SECONDS = 5.0;
	//! A longer gap between ticks (server stall) does not add maceration time beyond this
	static const float MAX_TICK_DELTA = 15.0;
	//! Tolerance of CLab_ToPerMille, in thousandths
	static const float PER_MILLE_EPSILON = 0.000045;
	//! Animation source of the buds breaking up (config.cpp AnimationSources, model.cfg bud drift and hide)
	static const string ANIM_BUDS = "clab_buds";
	//! Animation source of the tray held in hands (model.cfg clab_body lift and shift). Pouring tilts the tray in the
	//! player's own pour animations (data/anims), not in the model
	static const string ANIM_HOLD = "clab_hold";
	//! Stored seconds are capped here on load: every stage threshold lies far below, so a larger value reads the same
	static const float MAX_STORED_SECONDS = 10000000;
	//! A pour that ends this close to full, as a fraction of the capacity, is finished from the same container (B-008)
	static const float TOP_UP_FRACTION = 0.1;

	static const int MIX_NONE = 0;
	static const int MIX_YELLOW = 1;
	static const int MIX_WHITISH = 2;
	static const int MIX_GREEN = 3;

	static const int COOK_NONE = 0;
	static const int COOK_COOKING = 1;
	static const int COOK_WHITE = 2;
	static const int COOK_COPPER = 3;
	static const int COOK_BURNT = 4;

	static const int POWDER_WHITE = 0;
	static const int POWDER_COPPER = 1;
	static const int POWDER_BURNT = 2;
	static const int POWDER_SPOILED = 3;

	// Indices follow simpleHiddenSelections[] and hiddenSelections[] in config.cpp
	static const int SEL_FIRST_BUD = 0;
	static const int SEL_FIRST_LIQUID = 12;
	static const int SEL_POWDER = 15;
	static const int LIQUID_LEVELS = 3;
	static const int TEX_FIRST_LIQUID = 0;
	static const int TEX_POWDER = 3;

	static const string TEX_LIQUID_YELLOW = "#(argb,8,8,3)color(0.62,0.42,0.06,0.7,CA)";
	static const string TEX_LIQUID_WHITISH = "#(argb,8,8,3)color(0.80,0.78,0.68,0.88,CA)";
	static const string TEX_LIQUID_GREEN = "#(argb,8,8,3)color(0.22,0.36,0.06,0.8,CA)";
	static const string TEX_POWDER_WHITE = "CocaLab\\data\\clab_powder_white_co.paa";
	static const string TEX_POWDER_COPPER = "CocaLab\\data\\clab_powder_copper_co.paa";
	static const string TEX_POWDER_BURNT = "CocaLab\\data\\clab_powder_burnt_co.paa";
	static const string TEX_POWDER_SPOILED = "CocaLab\\data\\clab_powder_spoiled_co.paa";

	// Synchronized, derived on the server
	protected int m_CLab_BudCount;
	protected int m_CLab_MixStage;
	protected int m_CLab_CookStage;
	protected int m_CLab_BagsLeft;

	// Server only
	protected bool m_CLab_MixStarted;
	protected float m_CLab_MixSeconds;
	protected float m_CLab_CookSeconds;
	protected int m_CLab_BagsScraped;
	protected float m_CLab_LastTick;
	protected ref Timer m_CLab_MixTimer;
	//! The DP_CocaBag stack the last scrape filled. Not stored; a plain reference, so it reads null once the bag is gone
	protected ItemBase m_CLab_OpenBag;
	//! Last phase sent to ANIM_BUDS, so the phase is only set when it changes
	protected float m_CLab_BudPhase;
	//! Tick time of the last cook step that added time; -1 before the first. Not stored
	protected float m_CLab_LastCookTime = -1;
	//! Dedicated server: visible state last written by CLab_UpdateVisuals, so unchanged visuals are not written again
	protected int m_CLab_VisualKey = -1;

	void CLab_CocaTray()
	{
		RegisterNetSyncVariableInt("m_CLab_BudCount", 0, MAX_BUDS);
		RegisterNetSyncVariableInt("m_CLab_MixStage", MIX_NONE, MIX_GREEN);
		RegisterNetSyncVariableInt("m_CLab_CookStage", COOK_NONE, COOK_BURNT);
		RegisterNetSyncVariableInt("m_CLab_BagsLeft", 0, MAX_BAGS);
	}

	// ----------------------------------------------------------------------
	// Overrides
	// ----------------------------------------------------------------------

	//! The stove only registers cookware (portablegasstove.c:85-92)
	override bool IsCookware()
	{
		return true;
	}

	override bool IsOpen()
	{
		return true;
	}

	override void SetActions()
	{
		super.SetActions();

		RemoveAction(ActionFillFuel);
		RemoveAction(ActionFillCoolant);
		RemoveAction(ActionFillGeneratorTank);
		RemoveAction(ActionExtinguishFireplaceByLiquid);
		RemoveAction(ActionFillBottleSnow);
		RemoveAction(ActionWaterGardenSlot);
		RemoveAction(ActionWaterPlant);
		RemoveAction(ActionForceDrink);
		RemoveAction(ActionWashHandsItemContinuous);
		RemoveAction(ActionDrink);
		RemoveAction(ActionEmptyBottleBase);
		// Gasoline goes in only by pouring from a container held in hands (C1, C5): filling the tray held in hands plays
		// the pour-out animation, the same command as emptying (actiondrainliquid.c:16), and a pump or a well does not fill it
		RemoveAction(ActionDrainLiquid);
		RemoveAction(ActionFillBottleBase);
		// Nothing pours out of the tray into another container: a started cook needs no gasoline (R-a), so the same
		// gasoline would serve a second batch
		RemoveAction(ActionPourLiquid);

		AddAction(CLab_ActionRemoveBud);
		AddAction(CLab_ActionEmptyTray);
	}

	//! Liquid.CanFillContainer is the only reader (liquid.c:247): 0 closes pouring into the tray. Gasoline goes in at any
	//! time before cooking, with or without buds (C5); both sides read the synchronized cook stage
	override int GetLiquidContainerMask()
	{
		if (m_CLab_CookStage == COOK_NONE && !IsRuined())
			return super.GetLiquidContainerMask();

		return 0;
	}

	//! Visuals only: during client load IsServer() can still be true, and a new tray has no state to refresh. A tray
	//! created straight in hands gets its hold pose here; setting it on a client in load agrees with the server
	override void EEInit()
	{
		super.EEInit();

		CLab_UpdateVisuals();
		if (g_Game.IsServer())
			CLab_RefreshHeld();
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();

		if (!g_Game.IsServer())
			return;

		CLab_RefreshHeld();
		CLab_Log("loaded buds=" + m_CLab_BudCount.ToString() + " started=" + m_CLab_MixStarted.ToString() + " mixSec=" + m_CLab_MixSeconds.ToString() + " cookSec=" + m_CLab_CookSeconds.ToString() + " scraped=" + m_CLab_BagsScraped.ToString());

		// A crash between ruin and save leaves a ruined tray with content: it is lost now (B5)
		if (IsRuined() && CLab_HasContent())
		{
			CLab_Reset();
			return;
		}

		CLab_RefreshState();
		// Loaded fields can equal the defaults of the derived values, so RefreshState alone may not sync them
		SetSynchDirty();
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		CLab_StopTimer();
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		CLab_UpdateVisuals();
	}

	//! The gas stove drops its equipment reference before stopping the effects (portablegasstove.c:97-110), so the
	//! tray stops its own on any move; the next cooking tick restores them if it still cooks. The hold pose follows
	//! the hands. A client applies its visuals again after a move, which can bring the server's copy of them
	override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
	{
		super.EEItemLocationChanged(oldLoc, newLoc);

		if (!g_Game.IsServer())
		{
			CLab_UpdateVisuals();
			return;
		}

		CLab_StopCookingEffects();
		CLab_SetHeld(newLoc.GetType() == InventoryLocationType.HANDS);
	}

	//! Server side, fired by SetQuantity except while loading from storage (itembase.c:3398-3404)
	override void OnQuantityChanged(float delta)
	{
		super.OnQuantityChanged(delta);

		if (g_Game.IsServer())
			CLab_RefreshState();
	}

	//! A ruined tray loses its content at once and accepts nothing more (B5)
	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		super.EEHealthLevelChanged(oldLevel, newLevel, zone);

		if (g_Game.IsServer() && newLevel == GameConstants.STATE_RUINED)
		{
			CLab_Log("ruined, content lost buds=" + m_CLab_BudCount.ToString() + " q=" + GetQuantity().ToString() + " cookSec=" + m_CLab_CookSeconds.ToString());
			CLab_Reset();
		}
	}

	override void OnStoreSave(ParamsWriteContext ctx)
	{
		super.OnStoreSave(ctx);

		int storeVersion = STORE_VERSION;
		ctx.Write(storeVersion);
		ctx.Write(m_CLab_BudCount);
		ctx.Write(m_CLab_MixStarted);
		ctx.Write(m_CLab_MixSeconds);
		ctx.Write(m_CLab_CookSeconds);
		ctx.Write(m_CLab_MixStage);
		ctx.Write(m_CLab_BagsScraped);
	}

	override bool OnStoreLoad(ParamsReadContext ctx, int version)
	{
		if (!super.OnStoreLoad(ctx, version))
			return false;

		int storeVersion;
		if (!ctx.Read(storeVersion))
			return false;
		if (storeVersion < 1 || storeVersion > STORE_VERSION)
			return false;

		int budCount;
		bool mixStarted;
		float mixSeconds;
		float cookSeconds;
		int mixStage;
		int bagsScraped;
		if (!ctx.Read(budCount))
			return false;
		if (!ctx.Read(mixStarted))
			return false;
		if (!ctx.Read(mixSeconds))
			return false;
		if (!ctx.Read(cookSeconds))
			return false;
		if (!ctx.Read(mixStage))
			return false;
		if (!ctx.Read(bagsScraped))
			return false;

		m_CLab_BudCount = (int)Math.Clamp(budCount, 0, MAX_BUDS);
		m_CLab_MixStarted = mixStarted;
		m_CLab_MixSeconds = CLab_SanitizeSeconds(mixSeconds);
		m_CLab_CookSeconds = CLab_SanitizeSeconds(cookSeconds);
		m_CLab_MixStage = (int)Math.Clamp(mixStage, MIX_NONE, MIX_GREEN);
		m_CLab_BagsScraped = (int)Math.Clamp(bagsScraped, 0, MAX_BAGS);
		return true;
	}

	override bool NameOverride(out string output)
	{
		string key = CLab_GetNameKey();
		if (key == "")
			return false;

		output = key;
		return true;
	}

	//! Client side: GetTooltip falls back to descriptionShort when this returns false (inventoryitem.c:53-58)
	override bool DescriptionOverride(out string output)
	{
		string line;
		if (m_CLab_CookStage >= COOK_WHITE)
			line = string.Format(Widget.TranslateString("#STR_CLab_Desc_Bags"), m_CLab_BagsLeft);
		else if (m_CLab_BudCount > 0)
			line = string.Format(Widget.TranslateString("#STR_CLab_Desc_Buds"), m_CLab_BudCount, MAX_BUDS);

		// Until the mix starts the tray has to be filled up (C4): show how much gasoline it holds, with or without buds
		if (m_CLab_MixStage == MIX_NONE && m_CLab_CookStage == COOK_NONE && GetQuantity() > 0)
		{
			int gasoline = Math.Round(GetQuantity());
			int capacity = Math.Round(GetQuantityMax());
			string gasolineLine = string.Format(Widget.TranslateString("#STR_CLab_Desc_Gasoline"), gasoline, capacity);
			if (line == "")
				line = gasolineLine;
			else
				line = line + "\n" + gasolineLine;
		}

		if (line == "")
			return false;

		output = ConfigGetString("descriptionShort") + "\n" + line;
		return true;
	}

	override string GetPouringSoundset()
	{
		return "emptyVessle_Pot_SoundSet";
	}

	override string GetEmptyingLoopSoundsetHard()
	{
		return "pour_HardGround_Pot_SoundSet";
	}

	override string GetEmptyingLoopSoundsetSoft()
	{
		return "pour_SoftGround_Pot_SoundSet";
	}

	override string GetEmptyingLoopSoundsetWater()
	{
		return "pour_Water_Pot_SoundSet";
	}

	override string GetEmptyingEndSoundsetHard()
	{
		return "pour_End_HardGround_Pot_SoundSet";
	}

	override string GetEmptyingEndSoundsetSoft()
	{
		return "pour_End_SoftGround_Pot_SoundSet";
	}

	override string GetEmptyingEndSoundsetWater()
	{
		return "pour_End_Water_Pot_SoundSet";
	}

	// ----------------------------------------------------------------------
	// Queries for action and recipe conditions: synchronized fields only, so client and server agree
	// ----------------------------------------------------------------------

	//! With or without gasoline, until the mix turns green or cooking starts (B1, D4): a green mix would come back with a single bud
	bool CLab_CanAddBud()
	{
		return m_CLab_BudCount < MAX_BUDS && m_CLab_MixStage != MIX_GREEN && m_CLab_CookStage == COOK_NONE && !IsRuined();
	}

	bool CLab_CanRemoveBud()
	{
		return m_CLab_BudCount > 0 && GetQuantity() <= 0 && m_CLab_MixStage == MIX_NONE && m_CLab_CookStage == COOK_NONE && !IsRuined();
	}

	bool CLab_HasPowder()
	{
		return m_CLab_CookStage >= COOK_WHITE && !IsRuined();
	}

	bool CLab_IsEmpty()
	{
		return m_CLab_BudCount == 0 && GetQuantity() <= 0 && m_CLab_CookStage == COOK_NONE;
	}

	// ----------------------------------------------------------------------
	// Mutators (server)
	// ----------------------------------------------------------------------

	void CLab_AddBud()
	{
		if (!g_Game.IsServer() || !CLab_CanAddBud())
			return;

		m_CLab_BudCount++;
		// A bud added to a started mix restarts the maceration from zero, whatever went in first (D4)
		if (m_CLab_MixStarted)
		{
			m_CLab_MixSeconds = 0;
			m_CLab_LastTick = g_Game.GetTickTime();
			CLab_Log("maceration restarted buds=" + m_CLab_BudCount.ToString());
		}
		CLab_RefreshState();
		// The bud count is synchronized but not derived: RefreshState does not see it change
		SetSynchDirty();
	}

	//! Server, at the end of a pour into the tray (modded ActionPourLiquid): the vanilla transfer can end a little short of
	//! full (960 of 1000 ml measured, B-008), which keeps the mix from starting. What is missing comes from the same container.
	//! The end also follows an interrupted pour, and Liquid.CanTransfer does not check ruin, so the source is checked here.
	void CLab_TopUp(ItemBase source)
	{
		if (!g_Game.IsServer() || !source || source.IsRuined() || IsRuined() || m_CLab_CookStage != COOK_NONE)
			return;

		float missing = GetQuantityMax() - GetQuantity();
		if (missing <= 0 || missing > GetQuantityMax() * TOP_UP_FRACTION)
			return;
		if (!Liquid.CanTransfer(source, this))
			return;

		Liquid.Transfer(source, this, missing);
		CLab_Log("pour topped up missing=" + missing.ToString() + " q=" + GetQuantity().ToString());
	}

	//! The leaf is created first and the bud removed only if that worked; a new DP_CocaLeaves starts at 2 (fruits/config.cpp:32)
	void CLab_RemoveBud(PlayerBase player)
	{
		if (!g_Game.IsServer() || !player || !CLab_CanRemoveBud())
			return;

		EntityAI created = player.GetInventory().CreateInInventory("DP_CocaLeaves");
		if (!created)
			created = player.SpawnEntityOnGroundRaycastDispersed("DP_CocaLeaves");

		ItemBase leaf = ItemBase.Cast(created);
		if (!leaf)
			return;

		leaf.SetQuantity(1);
		m_CLab_BudCount--;
		CLab_RefreshState();
		SetSynchDirty();
	}

	//! One bag per recipe cycle, added to the stack being filled. The recipe repeats while the action is held; powder left
	//! when the player lets go or the empty bags run out stays for later (F2). Yield 0 empties the tray (F4).
	void CLab_Scrape(ItemBase bags, PlayerBase player)
	{
		if (!g_Game.IsServer() || !bags || !player || !CLab_HasPowder())
			return;

		if (m_CLab_BagsLeft <= 0)
		{
			CLab_Log("scrape with nothing left, tray emptied");
			CLab_Reset();
			return;
		}

		if (bags.GetQuantity() < 1)
			return;

		// The bag is made first: if that fails, nothing is taken
		ItemBase product = CLab_GetOpenBag(player);
		if (product)
		{
			product.AddQuantity(1);
		}
		else
		{
			EntityAI created = player.GetInventory().CreateInInventory("DP_CocaBag");
			if (!created)
				created = player.SpawnEntityOnGroundRaycastDispersed("DP_CocaBag");

			product = ItemBase.Cast(created);
			if (!product)
				return;

			product.SetQuantity(1);
			m_CLab_OpenBag = product;
		}

		bags.AddQuantity(-1);
		m_CLab_BagsScraped++;
		int remaining = m_CLab_BagsLeft - 1;
		CLab_Log("scraped 1 bag, stack " + product.GetQuantity().ToString() + ", left " + remaining.ToString());

		if (remaining <= 0)
			CLab_Reset();
		else
			CLab_RefreshState();
	}

	void CLab_Reset()
	{
		if (!g_Game.IsServer())
			return;

		m_CLab_BudCount = 0;
		m_CLab_MixStarted = false;
		m_CLab_MixSeconds = 0;
		m_CLab_CookSeconds = 0;
		m_CLab_MixStage = MIX_NONE;
		m_CLab_BagsScraped = 0;
		CLab_StopTimer();
		CLab_StopCookingEffects();
		SetQuantity(0);
		CLab_RefreshState();
		// The field writes above may leave the derived values unchanged; the client still needs them
		SetSynchDirty();
	}

	//! Server: derives stages and bags, runs the clock and syncs. Never changes the quantity, so calling it from OnQuantityChanged does not re-enter.
	void CLab_RefreshState()
	{
		if (!g_Game.IsServer())
			return;

		CLab_Settings settings = CLab_Settings.Get();
		bool cooking = m_CLab_CookSeconds > 0;
		bool ruined = IsRuined();

		// Maceration starts the moment the mix is complete and counts from then on (D4)
		if (!m_CLab_MixStarted && !cooking && !ruined && m_CLab_BudCount >= settings.MinBudsToMacerate && CLab_IsFull(settings))
		{
			m_CLab_MixStarted = true;
			m_CLab_LastTick = g_Game.GetTickTime();
			CLab_Log("maceration started buds=" + m_CLab_BudCount.ToString() + " q=" + GetQuantity().ToString());
		}

		// With cooking under way the persisted mix stage rules, even after a restart with another JSON
		int mixStage = m_CLab_MixStage;
		if (!cooking)
			mixStage = CLab_ComputeMixStage(m_CLab_MixStarted, m_CLab_MixSeconds, settings);

		int cookStage = CLab_ComputeCookStage(m_CLab_CookSeconds, settings);
		int bagsLeft = 0;
		if (cookStage >= COOK_WHITE)
			bagsLeft = CLab_ComputeBags(m_CLab_BudCount, mixStage, cookStage, settings) - m_CLab_BagsScraped;
		if (bagsLeft < 0)
			bagsLeft = 0;

		bool dirty = false;
		if (mixStage != m_CLab_MixStage)
		{
			CLab_Log("mix stage " + m_CLab_MixStage.ToString() + " -> " + mixStage.ToString() + " mixSec=" + m_CLab_MixSeconds.ToString());
			m_CLab_MixStage = mixStage;
			dirty = true;
		}
		if (cookStage != m_CLab_CookStage)
		{
			CLab_Log("cook stage " + m_CLab_CookStage.ToString() + " -> " + cookStage.ToString() + " cookSec=" + m_CLab_CookSeconds.ToString());
			m_CLab_CookStage = cookStage;
			dirty = true;
		}
		if (bagsLeft != m_CLab_BagsLeft)
		{
			m_CLab_BagsLeft = bagsLeft;
			dirty = true;
		}
		if (dirty)
			SetSynchDirty();

		// The buds break up while the mix macerates: they drift, sink and hide (model.cfg) and are gone by the time
		// it turns whitish. Cooking finishes off any left in a mix cooked while still yellow. Set on the server only,
		// as vanilla does for the fireplace (fireplacebase.c:761-806); the engine replicates the phase
		float budPhase = 0;
		if (m_CLab_MixStarted)
			budPhase = Math.Clamp(m_CLab_MixSeconds / settings.MacerationWhitishSeconds, 0, 1);
		if (cooking)
			budPhase = Math.Max(budPhase, Math.Clamp(m_CLab_CookSeconds / settings.CookDoneSeconds, 0, 1));
		if (budPhase != m_CLab_BudPhase)
		{
			m_CLab_BudPhase = budPhase;
			SetAnimationPhase(ANIM_BUDS, budPhase);
		}

		CLab_UpdateTimer(m_CLab_BudCount > 0 && !cooking && !ruined);

		// The server writes the visuals too, so they travel with the object; clients also update from OnVariablesSynchronized
		CLab_UpdateVisuals();
	}

	//! Timer callback (server). No allocations here: it runs every TICK_SECONDS per tray.
	void CLab_OnMixTick()
	{
		if (!g_Game.IsServer())
			return;

		float now = g_Game.GetTickTime();
		float dt = Math.Clamp(now - m_CLab_LastTick, 0, MAX_TICK_DELTA);
		m_CLab_LastTick = now;

		if (m_CLab_MixStarted && m_CLab_CookSeconds <= 0)
			m_CLab_MixSeconds += dt;

		CLab_RefreshState();
		CLab_Log("tick dt=" + dt.ToString() + " mixSec=" + m_CLab_MixSeconds.ToString() + " stage=" + m_CLab_MixStage.ToString() + " q=" + GetQuantity().ToString());
	}

	//! Called by Cooking.CookWithEquipment on the server, dt = seconds between calls. No caller reads the result.
	int CLab_CookStep(float dt)
	{
		if (!g_Game.IsServer())
			return 0;

		CLab_Settings settings = CLab_Settings.Get();
		bool started = m_CLab_CookSeconds > 0;
		if (IsRuined() || GetTemperature() < settings.CookMinTemperature)
		{
			CLab_StopCookingEffects();
			return 0;
		}

		// Starting to cook needs a started mix and the full level; once started it goes on without gasoline (R-a)
		if (!started && (!m_CLab_MixStarted || !CLab_IsFull(settings)))
		{
			CLab_StopCookingEffects();
			return 0;
		}

		// A cooling fire cooks its equipment and its direct slots in the same tick (fireplacebase.c:2043-2066), and a
		// tray moved between two fires would get the ticks of both: only the time since the last step counts
		float now = g_Game.GetTickTime();
		if (m_CLab_LastCookTime >= 0)
			dt = Math.Min(dt, now - m_CLab_LastCookTime);
		if (dt <= 0)
			return 0;
		m_CLab_LastCookTime = now;

		if (!started)
		{
			// Last mix stage before it freezes for good (R-c)
			m_CLab_MixStage = CLab_ComputeMixStage(m_CLab_MixStarted, m_CLab_MixSeconds, settings);
			CLab_StopTimer();
			CLab_Log("cooking started mixStage=" + m_CLab_MixStage.ToString() + " mixSec=" + m_CLab_MixSeconds.ToString());
		}

		m_CLab_CookSeconds += dt;

		// The liquid goes down with the progress and is gone at the done time (R-b)
		float target = GetQuantityMax() * (1 - m_CLab_CookSeconds / settings.CookDoneSeconds);
		if (target < 0)
			target = 0;
		if (GetQuantity() > target)
			SetQuantity(target);

		CLab_RefreshState();
		CLab_UpdateCookingEffects();
		CLab_Log("cook temp=" + GetTemperature().ToString() + " cookSec=" + m_CLab_CookSeconds.ToString() + " stage=" + m_CLab_CookStage.ToString() + " q=" + GetQuantity().ToString());
		return 1;
	}

	//! Client and server: selections and textures from the synchronized fields and the quantity. The server writes them
	//! as well because it sends its own copy with the object: set on the client alone, the liquid went back to the config
	//! color when a drop from hands sent the tray again (vanilla sets its runtime materials server side, chemlight.c:68)
	void CLab_UpdateVisuals()
	{
		bool hasPowder = m_CLab_CookStage >= COOK_WHITE;
		bool visible;
		int i;

		int level = 0;
		float quantity = GetQuantity();
		float quantityMax = GetQuantityMax();
		if (quantity > 0 && quantityMax > 0)
			level = (int)Math.Clamp(Math.Ceil(quantity / quantityMax * LIQUID_LEVELS), 1, LIQUID_LEVELS);

		// Every input below, packed: level 0-3, mix 0-3, cook 0-4, buds 0-12
		if (g_Game.IsDedicatedServer())
		{
			int key = level + 4 * m_CLab_MixStage + 16 * m_CLab_CookStage + 128 * m_CLab_BudCount;
			if (key == m_CLab_VisualKey)
				return;
			m_CLab_VisualKey = key;
		}

		for (i = 0; i < MAX_BUDS; i++)
		{
			visible = i < m_CLab_BudCount && !hasPowder;
			SetSimpleHiddenSelectionState(SEL_FIRST_BUD + i, visible);
		}

		string liquidTexture = CLab_GetLiquidTexture();
		for (i = 0; i < LIQUID_LEVELS; i++)
		{
			SetSimpleHiddenSelectionState(SEL_FIRST_LIQUID + i, level == i + 1);
			if (level > 0)
				SetObjectTexture(TEX_FIRST_LIQUID + i, liquidTexture);
		}

		SetSimpleHiddenSelectionState(SEL_POWDER, hasPowder);
		if (hasPowder)
			SetObjectTexture(TEX_POWDER, CLab_GetPowderTexture());
	}

	// ----------------------------------------------------------------------
	// Pure functions, shared by the tray and the settings self-test
	// ----------------------------------------------------------------------

	//! Yield in whole thousandths, truncating extra decimals. A 3-decimal yield x 1000 is off by at most 2^-15 in
	//! float32 (measured over all 1001 values; 0.251 lands just below 251), so a value that close to a thousandth is
	//! that thousandth. PER_MILLE_EPSILON stays under one float step near 1000 (6.1e-5): 0.4999999 gives 499, not 500.
	static int CLab_ToPerMille(float value)
	{
		float scaled = value * 1000;
		float nearest = Math.Round(scaled);
		if (Math.AbsFloat(scaled - nearest) <= PER_MILLE_EPSILON)
			return (int)nearest;

		return (int)Math.Floor(scaled);
	}

	//! F1 in integers: floor(buds / BudsPerBag x mix yield x cook yield), exact for yields with up to 3 decimals.
	//! The numerator stays under 12 x 1000 x 1000, far inside 32 bits.
	static int CLab_ComputeBags(int buds, int mixStage, int cookStage, CLab_Settings settings)
	{
		float mixYield = 0;
		if (mixStage == MIX_YELLOW)
			mixYield = settings.YieldYellow;
		else if (mixStage == MIX_WHITISH)
			mixYield = settings.YieldWhitish;
		else if (mixStage == MIX_GREEN)
			mixYield = settings.YieldGreen;
		else
			return 0;

		float cookYield = 0;
		if (cookStage == COOK_WHITE)
			cookYield = settings.YieldWhite;
		else if (cookStage == COOK_COPPER)
			cookYield = settings.YieldCopper;
		else if (cookStage == COOK_BURNT)
			cookYield = settings.YieldBurnt;
		else
			return 0;

		// Validate keeps BudsPerBag at 1 or more; a hand-built settings object must not divide by zero
		int budsPerBag = settings.BudsPerBag;
		if (budsPerBag < 1)
			budsPerBag = 1;

		int mixPerMille = CLab_ToPerMille(mixYield);
		int cookPerMille = CLab_ToPerMille(cookYield);
		return (buds * mixPerMille * cookPerMille) / (budsPerBag * 1000000);
	}

	static int CLab_ComputeMixStage(bool started, float seconds, CLab_Settings settings)
	{
		if (!started)
			return MIX_NONE;
		if (seconds < settings.MacerationWhitishSeconds)
			return MIX_YELLOW;
		if (seconds < settings.MacerationWhitishSeconds + settings.MacerationGreenSeconds)
			return MIX_WHITISH;
		return MIX_GREEN;
	}

	static int CLab_ComputeCookStage(float seconds, CLab_Settings settings)
	{
		if (seconds <= 0)
			return COOK_NONE;
		if (seconds < settings.CookDoneSeconds)
			return COOK_COOKING;
		if (seconds < settings.CookCopperSeconds)
			return COOK_WHITE;
		if (seconds < settings.CookBurntSeconds)
			return COOK_COPPER;
		return COOK_BURNT;
	}

	//! Stored seconds from a damaged file: NaN and negatives read as 0, anything past MAX_STORED_SECONDS is capped
	static float CLab_SanitizeSeconds(float seconds)
	{
		if (!(seconds >= 0))
			return 0;

		return Math.Min(seconds, MAX_STORED_SECONDS);
	}

	// ----------------------------------------------------------------------
	// Helpers
	// ----------------------------------------------------------------------

	//! Server: the tray rides 1 cm higher between the hands
	protected void CLab_SetHeld(bool held)
	{
		float phase = 0;
		if (held)
			phase = 1;
		SetAnimationPhase(ANIM_HOLD, phase);
	}

	//! Server: hold pose from where the tray is now, for a tray created or loaded in hands
	protected void CLab_RefreshHeld()
	{
		InventoryLocation location = new InventoryLocation();
		bool held = GetInventory().GetCurrentInventoryLocation(location) && location.GetType() == InventoryLocationType.HANDS;
		CLab_SetHeld(held);
	}

	protected bool CLab_IsFull(CLab_Settings settings)
	{
		return GetQuantity() >= settings.FullLevelFraction * GetQuantityMax() - 0.5;
	}

	protected bool CLab_HasContent()
	{
		return m_CLab_BudCount > 0 || GetQuantity() > 0 || m_CLab_CookSeconds > 0;
	}

	//! The stack the previous scrape filled, while it has room and is still on this player or loose on the ground beside them
	protected ItemBase CLab_GetOpenBag(PlayerBase player)
	{
		ItemBase bag = m_CLab_OpenBag;
		if (!bag || bag.IsRuined() || bag.GetQuantity() >= bag.GetQuantityMax())
			return null;
		if (bag.GetHierarchyRootPlayer() == player)
			return bag;
		if (!bag.GetHierarchyParent() && vector.Distance(bag.GetPosition(), player.GetPosition()) <= OPEN_BAG_DISTANCE)
			return bag;
		return null;
	}

	protected void CLab_UpdateTimer(bool shouldRun)
	{
		if (!shouldRun)
		{
			CLab_StopTimer();
			return;
		}

		if (!m_CLab_MixTimer)
			m_CLab_MixTimer = new Timer(CALL_CATEGORY_GAMEPLAY);

		if (!m_CLab_MixTimer.IsRunning())
		{
			m_CLab_LastTick = g_Game.GetTickTime();
			m_CLab_MixTimer.Run(TICK_SECONDS, this, "CLab_OnMixTick", null, true);
		}
	}

	protected void CLab_StopTimer()
	{
		if (m_CLab_MixTimer && m_CLab_MixTimer.IsRunning())
			m_CLab_MixTimer.Stop();
	}

	protected void CLab_UpdateCookingEffects()
	{
		CookingMethodType method = CookingMethodType.BOILING;
		bool done = false;
		bool burned = false;
		if (m_CLab_CookStage >= COOK_WHITE)
		{
			method = CookingMethodType.BAKING;
			done = true;
			burned = m_CLab_CookStage == COOK_BURNT;
		}

		// Only on change: each call synchronizes the entity
		if (m_CookingMethod != method || m_CookingIsDone != done || m_CookingIsBurned != burned)
			RefreshAudioVisualsOnClient(method, done, false, burned);
	}

	protected void CLab_StopCookingEffects()
	{
		if (m_CookingMethod != CookingMethodType.NONE)
			RemoveAudioVisualsOnClient();
	}

	//! Burnt wins; otherwise a green mix gives grey-green powder (E6)
	protected int CLab_GetPowderColor()
	{
		if (m_CLab_CookStage == COOK_BURNT)
			return POWDER_BURNT;
		if (m_CLab_MixStage == MIX_GREEN)
			return POWDER_SPOILED;
		if (m_CLab_CookStage == COOK_COPPER)
			return POWDER_COPPER;
		return POWDER_WHITE;
	}

	protected string CLab_GetPowderTexture()
	{
		int color = CLab_GetPowderColor();
		if (color == POWDER_BURNT)
			return TEX_POWDER_BURNT;
		if (color == POWDER_SPOILED)
			return TEX_POWDER_SPOILED;
		if (color == POWDER_COPPER)
			return TEX_POWDER_COPPER;
		return TEX_POWDER_WHITE;
	}

	protected string CLab_GetLiquidTexture()
	{
		if (m_CLab_MixStage == MIX_WHITISH)
			return TEX_LIQUID_WHITISH;
		if (m_CLab_MixStage == MIX_GREEN)
			return TEX_LIQUID_GREEN;
		return TEX_LIQUID_YELLOW;
	}

	//! Stringtable key for the current state, or "" for the config name (empty or ruined tray)
	protected string CLab_GetNameKey()
	{
		if (IsRuined())
			return "";

		int color = CLab_GetPowderColor();
		if (m_CLab_CookStage >= COOK_WHITE)
		{
			if (color == POWDER_BURNT)
				return "#STR_CLab_Tray_Burnt";
			if (color == POWDER_SPOILED)
				return "#STR_CLab_Tray_Spoiled";
			if (color == POWDER_COPPER)
				return "#STR_CLab_Tray_Copper";
			return "#STR_CLab_Tray_White";
		}

		if (m_CLab_CookStage == COOK_COOKING)
			return "#STR_CLab_Tray_Cooking";
		if (m_CLab_MixStage == MIX_YELLOW)
			return "#STR_CLab_Tray_Yellow";
		if (m_CLab_MixStage == MIX_WHITISH)
			return "#STR_CLab_Tray_Whitish";
		if (m_CLab_MixStage == MIX_GREEN)
			return "#STR_CLab_Tray_Green";
		if (GetQuantity() > 0)
			return "#STR_CLab_Tray_Wet";
		if (m_CLab_BudCount > 0)
			return "#STR_CLab_Tray_Loaded";
		return "";
	}

	//! Server log with DebugLog on; t= first so lines from both sides can be aligned
	protected void CLab_Log(string message)
	{
		if (!CLab_Settings.Get().DebugLog)
			return;

		Print("[CocaLab] t=" + g_Game.GetTickTime().ToString() + " tray " + GetNetworkIDString() + " " + message);
	}
}
