class CfgPatches
{
	class CocaLab
	{
		units[]=
		{
			"CLab_CocaTray"
		};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data",
			"DZ_Gear_Cooking",
			"DP_scripts",
			"DP_Fruits",
			"DP_Bags"
		};
	};
};
class CfgMods
{
	class CocaLab
	{
		dir="CocaLab";
		picture="";
		action="";
		hideName=1;
		hidePicture=1;
		name="CocaLab";
		credits="";
		author="";
		authorID="0";
		version="0.1.0";
		extra=0;
		type="mod";
		dependencies[]=
		{
			"World"
		};
		class defs
		{
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"CocaLab/scripts/4_World"
				};
			};
		};
	};
};
class CfgVehicles
{
	class Bottle_Base;
	class CLab_CocaTray: Bottle_Base
	{
		scope=2;
		displayName="$STR_CLab_Tray";
		descriptionShort="$STR_CLab_Tray_Desc";
		model="\CocaLab\data\clab_tray.p3d";
		rotationFlags=2;
		weight=900;
		itemSize[]={5,3};
		inventorySlot[]=
		{
			"CookingEquipment",
			"DirectCookingA",
			"DirectCookingB",
			"DirectCookingC"
		};
		canBeDigged=0;
		allowOwnedCargoManipulation=0;
		varLiquidTypeInit=8192;
		liquidContainerType="8192";
		destroyOnEmpty=0;
		varQuantityDestroyOnMin=0;
		varQuantityInit=0;
		varQuantityMin=0;
		varQuantityMax=1000;
		varTemperatureMax=200;
		varTemperatureFreezePoint=-200;
		varTemperatureThawPoint=-200;
		varTemperatureFreezeTime=3300;
		varTemperatureThawTime=3300;
		varTemperatureOverheatTime=-1;
		temperaturePerQuantityWeight=6;
		hiddenSelections[]=
		{
			"liquid_1",
			"liquid_2",
			"liquid_3",
			"powder"
		};
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.62,0.42,0.06,0.7,CA)",
			"#(argb,8,8,3)color(0.62,0.42,0.06,0.7,CA)",
			"#(argb,8,8,3)color(0.62,0.42,0.06,0.7,CA)",
			"CocaLab\data\clab_powder_white_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"CocaLab\data\clab_liquid.rvmat",
			"CocaLab\data\clab_liquid.rvmat",
			"CocaLab\data\clab_liquid.rvmat",
			"CocaLab\data\clab_powder.rvmat"
		};
		simpleHiddenSelections[]=
		{
			"coca_01",
			"coca_02",
			"coca_03",
			"coca_04",
			"coca_05",
			"coca_06",
			"coca_07",
			"coca_08",
			"coca_09",
			"coca_10",
			"coca_11",
			"coca_12",
			"liquid_1",
			"liquid_2",
			"liquid_3",
			"powder"
		};
		class AnimationSources
		{
			// Buds breaking up 0..1 (maceration to whitish, then cooking), set on the server; model.cfg drifts and hides the buds
			class clab_buds
			{
				source="user";
				animPeriod=0.0099999998;
				initPhase=0;
			};
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=200;
					healthLevels[]=
					{
						{1,{"CocaLab\data\clab_tray.rvmat"}},
						{0.69999999,{"CocaLab\data\clab_tray.rvmat"}},
						{0.5,{"CocaLab\data\clab_tray_damage.rvmat"}},
						{0.30000001,{"CocaLab\data\clab_tray_damage.rvmat"}},
						{0,{"CocaLab\data\clab_tray_destruct.rvmat"}}
					};
				};
			};
		};
		repairableWithKits[]={10};
		repairCosts[]={25};
		soundImpactType="metal";
		class AnimEvents
		{
			class SoundWeapon
			{
				class pickUpPotLight
				{
					soundSet="pickUpPotLight_SoundSet";
					id=796;
				};
				class pickUpPot
				{
					soundSet="pickUpPot_SoundSet";
					id=797;
				};
				class drop
				{
					soundset="pot_drop_SoundSet";
					id=898;
				};
			};
		};
	};
};
