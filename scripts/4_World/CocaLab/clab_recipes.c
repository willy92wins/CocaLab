// ==========================================================================
// Recipes of the maceration tray, plus the removal of the direct DrugsPLUS coca bag recipe.
// Every Init() field is set explicitly: RecipeBase does not initialize them (recipebase.c:67-86),
// and a 0 left in place rejects the quantity (:441) or sets the ingredient health to 0 (:390-394).
// The manager sorts the two items to the ingredient order before CanDo/Do
// (pluginrecipesmanager.c:321,337); the tray is still looked up on both sides.
// CanDo copies the ingredients to locals first and does not call super.CanDo: after handing the static array
// on to super, the array read back with the tray in both slots and CanDo came out false on the server (B-003).
// Of 206 vanilla CanDo overrides only craftlongtorch.c:61 calls super, as its last statement; the base check
// it would add rejects ingredients with attachments, which neither ingredient of these recipes can have.
// ==========================================================================

class CLab_RecipeAddBud extends RecipeBase
{
	override void Init()
	{
		m_Name = "#STR_CLab_AddBud";
		m_IsInstaRecipe = false;
		m_AnimationLength = 0.5;
		m_Specialty = 0;
		m_AnywhereInInventory = false;	// either ingredient must be in hands

		// Ingredient 0: one unit of coca leaves
		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = -1;
		m_MinQuantityIngredient[0] = 1;
		m_MaxQuantityIngredient[0] = -1;
		InsertIngredient(0, "DP_CocaLeaves");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = -1;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;

		// Ingredient 1: the tray, untouched by the recipe system; a ruined tray does not qualify
		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = -1;
		m_MaxQuantityIngredient[1] = -1;
		InsertIngredient(1, "CLab_CocaTray");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		ItemBase first = ingredients[0];
		ItemBase second = ingredients[1];
		CLab_CocaTray tray = CLab_CocaTray.Cast(second);
		if (!tray)
			tray = CLab_CocaTray.Cast(first);
		if (!tray)
			return false;

		return tray.CLab_CanAddBud();
	}

	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(ingredients[1]);
		if (!tray)
			tray = CLab_CocaTray.Cast(ingredients[0]);

		if (tray)
			tray.CLab_AddBud();
	}
}

class CLab_RecipeScrapePowder extends RecipeBase
{
	override void Init()
	{
		m_Name = "#STR_CLab_Scrape";
		m_IsInstaRecipe = false;
		m_AnimationLength = 1;
		m_Specialty = 0;
		m_AnywhereInInventory = false;	// either ingredient must be in hands

		// Ingredient 0: empty bags; Do takes one per scraped bag (F2)
		m_MinDamageIngredient[0] = -1;
		m_MaxDamageIngredient[0] = -1;
		m_MinQuantityIngredient[0] = 1;
		m_MaxQuantityIngredient[0] = -1;
		InsertIngredient(0, "AC_EmptyBag");
		m_IngredientAddHealth[0] = 0;
		m_IngredientSetHealth[0] = -1;
		m_IngredientAddQuantity[0] = 0;
		m_IngredientDestroy[0] = false;
		m_IngredientUseSoftSkills[0] = false;

		// Ingredient 1: the tray, untouched by the recipe system; a ruined tray does not qualify
		m_MinDamageIngredient[1] = -1;
		m_MaxDamageIngredient[1] = 3;
		m_MinQuantityIngredient[1] = -1;
		m_MaxQuantityIngredient[1] = -1;
		InsertIngredient(1, "CLab_CocaTray");
		m_IngredientAddHealth[1] = 0;
		m_IngredientSetHealth[1] = -1;
		m_IngredientAddQuantity[1] = 0;
		m_IngredientDestroy[1] = false;
		m_IngredientUseSoftSkills[1] = false;
	}

	override bool CanDo(ItemBase ingredients[], PlayerBase player)
	{
		ItemBase first = ingredients[0];
		ItemBase second = ingredients[1];
		CLab_CocaTray tray = CLab_CocaTray.Cast(second);
		if (!tray)
			tray = CLab_CocaTray.Cast(first);
		if (!tray)
			return false;

		return tray.CLab_HasPowder();
	}

	override void Do(ItemBase ingredients[], PlayerBase player, array<ItemBase> results, float specialty_weight)
	{
		CLab_CocaTray tray = CLab_CocaTray.Cast(ingredients[1]);
		ItemBase bags = ingredients[0];
		if (!tray)
		{
			tray = CLab_CocaTray.Cast(ingredients[0]);
			bags = ingredients[1];
		}

		if (tray)
			tray.CLab_Scrape(bags, player);
	}
}

modded class PluginRecipesManager
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();
		RegisterRecipe(new CLab_RecipeAddBud);
		RegisterRecipe(new CLab_RecipeScrapePowder);

		// The plugin is built once at mission start on the server too (pluginmanager.c:53): settings load there
		CLab_Settings.Preload();
	}

	override protected void GenerateRecipeCache()
	{
		// Runs after every RegisterRecipies override, whatever the mod load order
		UnregisterRecipe("DP_CraftCocaBag");
		super.GenerateRecipeCache();
	}
}
