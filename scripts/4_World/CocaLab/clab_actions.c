// ==========================================================================
// Actions on the maceration tray held in hands: removing a bud (single use, template: actionemptyseedspack.c)
// and emptying the tray (continuous pour, template: actionemptycookingpot.c), plus how the tray is held.
// CCINonRuined hides both actions on a ruined tray, which accepts nothing any more (B5).
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
//! sit at X = +-29 cm of the item origin and the model lies along X, 43 cm long. The pot grip (:1026) put the hands
//! inside the tray (B-005). The pot animation set stays: its pour replaces the bottle pour, cap and all, on
//! "Empty tray" (player_main_2h_pot.asi:26-41), as vanilla pairs one item's set with another's grip (:1000).
modded class ModItemRegisterCallbacks
{
	override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterTwoHanded(pType, pBehavior);
		pType.AddItemInHandsProfileIK("CLab_CocaTray", "dz/anims/workspaces/player/player_main/props/player_main_2h_pot.asi", pBehavior, "dz/anims/anm/player/ik/vehicles/BatteryTruck.anm");
	}
}
