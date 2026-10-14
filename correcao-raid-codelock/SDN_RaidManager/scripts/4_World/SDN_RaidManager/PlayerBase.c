modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == 1899911)
		{
			bool isRaidActive = false;
			if (ctx.Read(isRaidActive))
			{
				SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
				if (manager)
				{
					manager.SetClientRaidActive(isRaidActive);
				}
			}
		}
	}
}
