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
//! (:1026) put the hands inside the tray (B-005). The animation set is the pot's, whose pour replaces the bottle pour
//! on "Empty tray" (player_main_2h_pot.asi:26-41), with its standing pour replaced in turn (data/anims/clab_tray_2h.asi).
//! Continuous actions run without arm IK, so the pot's own pour keeps the hands on its 29 cm handles and the left one
//! off the tray; the replacement keeps both wrists on the truck battery grip and tilts the tray in the hands
//! (CocaLab_dev/tools/tray_pour_anim.py). The crouched pour is still the pot's. model.cfg only lifts the tray while
//! held (CLab_CocaTray.ANIM_HOLD).
modded class ModItemRegisterCallbacks
{
	override void RegisterTwoHanded(DayZPlayerType pType, DayzPlayerItemBehaviorCfg pBehavior)
	{
		super.RegisterTwoHanded(pType, pBehavior);
		pType.AddItemInHandsProfileIK("CLab_CocaTray", "CocaLab/data/anims/clab_tray_2h.asi", pBehavior, "dz/anims/anm/player/ik/vehicles/BatteryTruck.anm");
	}
}
