class CfgPatches
{
	class SDN_RaidManager
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = { "DZ_Data", "DZ_Scripts" };
	};
};

class CfgMods
{
	class SDN_RaidManager
	{
		dir = "SDN_RaidManager";
		name = "SDN_RaidManager";
		author = "SafeDoNordeste";
		credits = "SafeDoNordeste";
		type = "mod";
		dependencies[] = { "Game", "World", "Mission" };

		class defs
		{
			class gameScriptModule
			{
				files[] = { "SDN_RaidManager/scripts/3_Game" };
			};
			class worldScriptModule
			{
				files[] = { "SDN_RaidManager/scripts/4_World" };
			};
			class missionScriptModule
			{
				files[] = { "SDN_RaidManager/scripts/5_Mission" };
			};
		};
	};
};
