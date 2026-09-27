// ==========================================================================
// CocaLab server settings, read from $profile:CocaLab/CocaLab.json.
// Only the server loads the file. Clients keep the defaults and never use them for
// rules: every rule outcome reaches them as a synchronized variable of the tray.
// A missing file is created with the defaults; an unreadable one is left untouched.
// ==========================================================================

class CLab_Settings
{
	int ConfigVersion = 1;
	int MinBudsToMacerate = 2;
	float FullLevelFraction = 1.0;
	float MacerationWhitishSeconds = 300;
	float MacerationGreenSeconds = 1800;
	float CookMinTemperature = 90;
	float CookDoneSeconds = 240;
	float CookCopperSeconds = 360;
	float CookBurntSeconds = 480;
	float YieldYellow = 0.5;
	float YieldWhitish = 1.0;
	float YieldGreen = 0.0;
	float YieldWhite = 1.0;
	float YieldCopper = 0.5;
	float YieldBurnt = 0.0;
	int BudsPerBag = 2;
	bool DebugLog = false;

	static const string DIRECTORY = "$profile:CocaLab";
	static const string FILE_PATH = "$profile:CocaLab/CocaLab.json";
	static const string LOG_TAG = "[CocaLab] ";
	static const float MAX_MACERATION_SECONDS = 604800;

	protected static ref CLab_Settings s_Instance;

	static CLab_Settings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new CLab_Settings();
			if (g_Game.IsServer())
				s_Instance.LoadFromProfile();
		}
		return s_Instance;
	}

	//! Mission start: the file and the self-test appear on boot, not on the first tray use (spec S13).
	//! IsDedicatedServer is reliable during load, unlike IsServer; offline play also loads, multiplayer clients do not.
	static void Preload()
	{
		if (g_Game.IsDedicatedServer() || !g_Game.IsMultiplayer())
			Get();
	}

	protected void LoadFromProfile()
	{
		string errorMessage;
		if (!FileExist(FILE_PATH))
		{
			MakeDirectory(DIRECTORY);
			if (JsonFileLoader<CLab_Settings>.SaveFile(FILE_PATH, this, errorMessage))
				Print(LOG_TAG + "created " + FILE_PATH + " with the default values");
			else
				Print(LOG_TAG + "WARNING could not create " + FILE_PATH + ": " + errorMessage);
		}
		else
		{
			// Loaded into a fresh instance: a file that fails half-way never mixes its values with the defaults
			CLab_Settings fromFile = new CLab_Settings();
			if (JsonFileLoader<CLab_Settings>.LoadFile(FILE_PATH, fromFile, errorMessage))
				CopyFrom(fromFile);
			else
				Print(LOG_TAG + "WARNING " + FILE_PATH + " is unreadable; defaults in use and the file is left untouched: " + errorMessage);
		}

		Validate();
		Print(LOG_TAG + "settings loaded");
		if (DebugLog)
			RunSelfTest();
	}

	protected void CopyFrom(CLab_Settings other)
	{
		ConfigVersion = other.ConfigVersion;
		MinBudsToMacerate = other.MinBudsToMacerate;
		FullLevelFraction = other.FullLevelFraction;
		MacerationWhitishSeconds = other.MacerationWhitishSeconds;
		MacerationGreenSeconds = other.MacerationGreenSeconds;
		CookMinTemperature = other.CookMinTemperature;
		CookDoneSeconds = other.CookDoneSeconds;
		CookCopperSeconds = other.CookCopperSeconds;
		CookBurntSeconds = other.CookBurntSeconds;
		YieldYellow = other.YieldYellow;
		YieldWhitish = other.YieldWhitish;
		YieldGreen = other.YieldGreen;
		YieldWhite = other.YieldWhite;
		YieldCopper = other.YieldCopper;
		YieldBurnt = other.YieldBurnt;
		BudsPerBag = other.BudsPerBag;
		DebugLog = other.DebugLog;
	}

	//! Clamps every field to its valid range and logs each correction by field name (H1)
	void Validate()
	{
		MinBudsToMacerate = ClampIntField("MinBudsToMacerate", MinBudsToMacerate, 1, CLab_CocaTray.MAX_BUDS);
		FullLevelFraction = ClampFloatField("FullLevelFraction", FullLevelFraction, 0.1, 1.0);
		MacerationWhitishSeconds = ClampFloatField("MacerationWhitishSeconds", MacerationWhitishSeconds, 10, MAX_MACERATION_SECONDS);
		MacerationGreenSeconds = ClampFloatField("MacerationGreenSeconds", MacerationGreenSeconds, 10, MAX_MACERATION_SECONDS);
		// The tray tops out at 200 degrees (varTemperatureMax in config.cpp)
		CookMinTemperature = ClampFloatField("CookMinTemperature", CookMinTemperature, 30, 190);
		CookDoneSeconds = RaiseFloatField("CookDoneSeconds", CookDoneSeconds, 10);
		CookCopperSeconds = RaiseFloatField("CookCopperSeconds", CookCopperSeconds, CookDoneSeconds + 1);
		CookBurntSeconds = RaiseFloatField("CookBurntSeconds", CookBurntSeconds, CookCopperSeconds + 1);
		YieldYellow = TruncateYieldField("YieldYellow", YieldYellow);
		YieldWhitish = TruncateYieldField("YieldWhitish", YieldWhitish);
		YieldGreen = TruncateYieldField("YieldGreen", YieldGreen);
		YieldWhite = TruncateYieldField("YieldWhite", YieldWhite);
		YieldCopper = TruncateYieldField("YieldCopper", YieldCopper);
		YieldBurnt = TruncateYieldField("YieldBurnt", YieldBurnt);
		BudsPerBag = ClampIntField("BudsPerBag", BudsPerBag, 1, CLab_CocaTray.MAX_BUDS);
	}

	protected int ClampIntField(string field, int value, int minValue, int maxValue)
	{
		int clamped = value;
		if (clamped < minValue)
			clamped = minValue;
		if (clamped > maxValue)
			clamped = maxValue;
		if (clamped != value)
			Print(LOG_TAG + field + " = " + value.ToString() + " is out of range, set to " + clamped.ToString());
		return clamped;
	}

	protected float ClampFloatField(string field, float value, float minValue, float maxValue)
	{
		float clamped = Math.Clamp(value, minValue, maxValue);
		if (clamped != value)
			Print(LOG_TAG + field + " = " + value.ToString() + " is out of range, set to " + clamped.ToString());
		return clamped;
	}

	protected float RaiseFloatField(string field, float value, float minValue)
	{
		if (value >= minValue)
			return value;
		Print(LOG_TAG + field + " = " + value.ToString() + " is below its minimum, set to " + minValue.ToString());
		return minValue;
	}

	//! Yields are 0-1 with at most 3 decimals; extra decimals are truncated, so no rounding ever adds a bag (F1)
	protected float TruncateYieldField(string field, float value)
	{
		float clamped = ClampFloatField(field, value, 0, 1);
		int perMille = CLab_CocaTray.CLab_ToPerMille(clamped);
		float truncated = perMille * 0.001;
		// No original value in the line: float ToString rounds it (0.4999999 prints as 0.5) and the line would contradict itself
		if (Math.AbsFloat(clamped * 1000 - perMille) > CLab_CocaTray.PER_MILLE_EPSILON)
			Print(LOG_TAG + field + " has more than 3 decimals, truncated to " + truncated.ToString());
		return truncated;
	}

	// ----------------------------------------------------------------------
	// Self-test of the yield oracle (feature spec §7), run at load with DebugLog.
	// Every row is evaluated with the defaults except the fields it overrides.
	// ----------------------------------------------------------------------

	protected void RunSelfTest()
	{
		int failures = 0;
		int cases = 0;

		failures += CheckBags("12 whitish white", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 6, -1, -1, -1);
		failures += CheckBags("12 yellow white", 12, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_WHITE, 3, -1, -1, -1);
		failures += CheckBags("12 whitish copper", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_COPPER, 3, -1, -1, -1);
		failures += CheckBags("12 yellow copper", 12, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_COPPER, 1, -1, -1, -1);
		failures += CheckBags("12 green white", 12, CLab_CocaTray.MIX_GREEN, CLab_CocaTray.COOK_WHITE, 0, -1, -1, -1);
		failures += CheckBags("12 whitish burnt", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_BURNT, 0, -1, -1, -1);
		failures += CheckBags("11 whitish white", 11, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 5, -1, -1, -1);
		failures += CheckBags("7 whitish copper", 7, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_COPPER, 1, -1, -1, -1);
		failures += CheckBags("3 whitish white", 3, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 1, -1, -1, -1);
		failures += CheckBags("2 whitish white", 2, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 1, -1, -1, -1);
		failures += CheckBags("2 yellow white", 2, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_WHITE, 0, -1, -1, -1);
		failures += CheckBags("0 whitish white", 0, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 0, -1, -1, -1);
		failures += CheckBags("12 yellow white YieldYellow=0.99999", 12, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_WHITE, 5, 0.99999, -1, -1);
		failures += CheckBags("11 whitish white YieldWhitish=0.95", 11, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 5, -1, 0.95, -1);
		failures += CheckBags("12 yellow white YieldYellow=0.4999999", 12, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_WHITE, 2, 0.4999999, -1, -1);
		failures += CheckBags("12 whitish white BudsPerBag=1", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 12, -1, -1, 1);
		failures += CheckBags("12 yellow copper BudsPerBag=1", 12, CLab_CocaTray.MIX_YELLOW, CLab_CocaTray.COOK_COPPER, 3, -1, -1, 1);
		failures += CheckBags("7 whitish white BudsPerBag=3", 7, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 2, -1, -1, 3);
		failures += CheckBags("11 whitish copper BudsPerBag=5", 11, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_COPPER, 1, -1, -1, 5);
		failures += CheckBags("12 whitish white BudsPerBag=12", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_WHITE, 1, -1, -1, 12);
		failures += CheckBags("12 whitish copper BudsPerBag=12", 12, CLab_CocaTray.MIX_WHITISH, CLab_CocaTray.COOK_COPPER, 0, -1, -1, 12);
		cases += 21;

		failures += CheckPerMille(0.251, 251);
		failures += CheckPerMille(0.253, 253);
		failures += CheckPerMille(0.502, 502);
		failures += CheckPerMille(0.506, 506);
		failures += CheckPerMille(0.511, 511);
		failures += CheckPerMille(0.4999999, 499);
		failures += CheckPerMille(0.7009999, 700);
		cases += 7;

		if (failures == 0)
			Print(LOG_TAG + "CLab selftest PASS (" + cases.ToString() + " cases)");
		else
			Print(LOG_TAG + "CLab selftest FAIL: " + failures.ToString() + " of " + cases.ToString() + " cases");
	}

	//! Returns 1 on failure. yellowOverride / whitishOverride replace YieldYellow / YieldWhitish when >= 0,
	//! budsPerBagOverride replaces BudsPerBag when > 0.
	protected int CheckBags(string label, int buds, int mixStage, int cookStage, int expected, float yellowOverride, float whitishOverride, int budsPerBagOverride)
	{
		CLab_Settings defaults = new CLab_Settings();
		if (yellowOverride >= 0)
			defaults.YieldYellow = yellowOverride;
		if (whitishOverride >= 0)
			defaults.YieldWhitish = whitishOverride;
		if (budsPerBagOverride > 0)
			defaults.BudsPerBag = budsPerBagOverride;

		int bags = CLab_CocaTray.CLab_ComputeBags(buds, mixStage, cookStage, defaults);
		if (bags == expected)
			return 0;

		Print(LOG_TAG + "CLab selftest FAIL " + label + ": got " + bags.ToString() + ", expected " + expected.ToString());
		return 1;
	}

	//! Returns 1 on failure.
	protected int CheckPerMille(float value, int expected)
	{
		int perMille = CLab_CocaTray.CLab_ToPerMille(value);
		if (perMille == expected)
			return 0;

		Print(LOG_TAG + "CLab selftest FAIL per-mille " + value.ToString() + ": got " + perMille.ToString() + ", expected " + expected.ToString());
		return 1;
	}
}
