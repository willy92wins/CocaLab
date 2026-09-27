// ==========================================================================
// Single cooking hook for the maceration tray. Every vanilla heat source cooks its
// equipment through Cooking.CookWithEquipment (stove: portablegasstove.c:212-217;
// fireplaces and direct cooking slots: fireplacebase.c:2137-2159). The tray is diverted
// before the vanilla pass, which would evaporate its liquid and damage its cargo.
// m_UpdateTime is the seconds between calls set by the heat source; cooking_time_coef is
// not a time (on the stove it scales with the energy consumed, portablegasstove.c:203).
// ==========================================================================

modded class Cooking
{
	override int CookWithEquipment(ItemBase cooking_equipment, float cooking_time_coef = 1)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(cooking_equipment);
		if (tray)
			return tray.CLab_CookStep(m_UpdateTime);

		return super.CookWithEquipment(cooking_equipment, cooking_time_coef);
	}
}
