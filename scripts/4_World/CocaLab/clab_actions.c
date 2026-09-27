// ==========================================================================
// Single-use actions on the maceration tray held in hands (template: actionemptyseedspack.c).
// CCINonRuined hides both on a ruined tray, which accepts nothing any more (B5).
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

class CLab_ActionEmptyTray: ActionSingleUseBase
{
	void CLab_ActionEmptyTray()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_EMPTYSEEDSPACK;
		m_FullBody = false;
		m_Text = "#STR_CLab_EmptyTray";
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
		return tray && !tray.CLab_IsEmpty();
	}

	//! Throws away buds, gasoline and powder without returning anything (B4)
	override void OnExecuteServer(ActionData action_data)
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
