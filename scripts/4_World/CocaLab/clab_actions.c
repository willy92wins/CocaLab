// ==========================================================================
// Actions on the maceration tray held in hands: removing a bud (single use, template: actionemptyseedspack.c)
// and emptying the tray (continuous pour, template: actionemptycookingpot.c), plus how the tray is held.
// CCINonRuined hides both actions on a ruined tray, which accepts nothing any more (B5).
// Two vanilla liquid actions with the tray as target: pouring into it finishes a pour that ends just short of
// full, and draining it is never offered (C5).
// ==========================================================================

class CLab_ActionRemoveBud: ActionSingleUseBase
{
	void CLab_ActionRemoveBud()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_EMPTYSEEDSPACK;
		m_FullBody = false;
		m_Text = "#STR_CLab_RemoveBud";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined;
		m_ConditionTarget = new CCTNone;
	}

	override bool HasTarget()
	{
		return false;
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(item);
		return tray && tray.CLab_CanRemoveBud();
	}

	override void OnExecuteServer(ActionData action_data)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(action_data.m_MainItem);
		if (tray)
			tray.CLab_RemoveBud(action_data.m_Player);
	}
}

//! Fixed-length pour: a tray can hold buds or powder with no liquid, and the vanilla quantity drain would end at once
class CLab_ActionEmptyTrayCB: ActionEmptyBottleBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(CLab_ActionEmptyTray.POUR_SECONDS);
	}
}

//! Pour animation and emptying sound of the vanilla vessels (actionemptybottlebase.c:27, :70-101)
class CLab_ActionEmptyTray: ActionEmptyBottleBase
{
	static const float POUR_SECONDS = 2.5;

	void CLab_ActionEmptyTray()
	{
		m_CallbackClass = CLab_ActionEmptyTrayCB;
		m_Text = "#STR_CLab_EmptyTray";
	}

	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(item);
		return tray && !tray.CLab_IsEmpty();
	}

	//! Throws away buds, gasoline and powder without returning anything (B4)
	override void OnFinishProgressServer(ActionData action_data)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(action_data.m_MainItem);
		if (tray)
			tray.CLab_Reset();
	}

	//! The tray tilts as the hands raise it, from the start of the action on each side, and levels as they lower it, when
	//! the pour loop ends. ActionEmptyBottleBase overrides the loop events without calling super
	//! (actionemptybottlebase.c:70-78), so the *Server variants of ActionContinuousBase never run for it; the public
	//! ones run on both sides
	override void OnStartServer(ActionData action_data)
	{
		super.OnStartServer(action_data);

		CLab_SetTrayPouring(action_data, true);
	}

	override void OnStartClient(ActionData action_data)
	{
		super.OnStartClient(action_data);

		CLab_SetTrayPouring(action_data, true);
	}

	override void OnEndAnimationLoop(ActionData action_data)
	{
		super.OnEndAnimationLoop(action_data);

		CLab_SetTrayPouring(action_data, false);
	}

	//! An interrupt ends the action without the end of the loop (actionbase.c:764-785)
	override void OnEndServer(ActionData action_data)
	{
		super.OnEndServer(action_data);

		CLab_SetTrayPouring(action_data, false);
	}

	override void OnEndClient(ActionData action_data)
	{
		super.OnEndClient(action_data);

		CLab_SetTrayPouring(action_data, false);
	}

	protected void CLab_SetTrayPouring(ActionData action_data, bool pouring)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(action_data.m_MainItem);
		if (tray)
			tray.CLab_SetPouring(pouring);
	}
}

//! A pour into the tray ends when the client's side of the transfer finishes, which can leave it just short of full
//! (B-008); the tray takes what is missing from the same container
modded class ActionPourLiquid
{
	override void OnEndServer(ActionData action_data)
	{
		super.OnEndServer(action_data);

		if (!action_data.m_Target)
			return;

		CLab_CocaTray tray = CLab_CocaTray.Cast(action_data.m_Target.GetObject());
		if (tray)
			tray.CLab_TopUp(action_data.m_MainItem);
	}
}

//! Gasoline never leaves the tray for another container: with a container in hands, a tray that is not full offers only
//! the pour, and a full one offers nothing (C5)
modded class ActionDrainLiquid
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		if (target && CLab_CocaTray.Cast(target.GetObject()))
			return false;

		return super.ActionCondition(player, target, item);
	}
}

modded class ActionConstructor
{
	override void RegisterActions(TTypenameArray actions)
	{
		super.RegisterActions(actions);
		actions.Insert(CLab_ActionRemoveBud);
		actions.Insert(CLab_ActionEmptyTray);
	}
}

//! Held with both hands by the short ends, with the vanilla truck battery grip (dayzplayercfgbase.c:843): its wrists
//! sit at X = +-29 cm of the item origin, the right one at +X, and the model lies along X, 47 cm long. The pot grip
//! (:1026) put the hands inside the tray (B-005). The pot animation set stays: its pour replaces the bottle pour, cap
//! and all, on "Empty tray" (player_main_2h_pot.asi:26-41), as vanilla pairs one item's set with another's grip
//! (:1000). Both poses are finished in the model, not in the grip: model.cfg lifts the tray while held and tilts it
//! while it pours (CLab_CocaTray.ANIM_HOLD, ANIM_POUR).
modded class ModItemRegisterCallbacks
{
	override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterTwoHanded(pType, pBehavior);
		pType.AddItemInHandsProfileIK("CLab_CocaTray", "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi", pBehavior, "dz/anims/anm/player/ik/vehicles/BatteryTruck.anm");
	}
}
