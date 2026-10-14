modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();
		SDN_RaidManagerManager.GetInstance().Init();
	}

	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);

		// Sync the raid state to the player that just connected
		if (player)
		{
			SDN_RaidManagerManager.GetInstance().SyncStateToPlayer(player);
		}
	}
}
