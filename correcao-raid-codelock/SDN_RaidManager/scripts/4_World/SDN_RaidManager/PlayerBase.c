modded class PlayerBase
{
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		if (rpc_type == 1899911 && GetGame() && GetGame().IsClient() && !GetGame().IsServer())
		{
			bool isRaidActive = false;
			bool blockBreachingChargeOutsideRaid = false;
			if (ctx.Read(isRaidActive) && ctx.Read(blockBreachingChargeOutsideRaid))
			{
				SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
				if (manager)
				{
					manager.SetClientRaidActive(isRaidActive);
					manager.SetClientBlockBreachingChargeOutsideRaid(blockBreachingChargeOutsideRaid);
				}
			}
		}
	}
}
