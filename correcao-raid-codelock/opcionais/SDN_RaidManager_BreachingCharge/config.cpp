class CfgPatches
{
	class SDN_RaidManager_BreachingCharge
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = { "DZ_Data", "DZ_Scripts", "SDN_RaidManager" };
	};
};

class CfgMods
{
	class SDN_RaidManager_BreachingCharge
	{
		dir = "SDN_RaidManager_BreachingCharge";
		name = "SDN RaidManager - BreachingCharge Compatibility";
		author = "SafeDoNordeste";
		type = "mod";
		dependencies[] = { "World" };

		class defs
		{
			class worldScriptModule
			{
				files[] = { "SDN_RaidManager_BreachingCharge/scripts/4_World" };
			};
		};
	};
};
