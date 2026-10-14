modded class BaseBuildingBase
{
	override void EEInit()
	{
		super.EEInit();

		if (!GetGame() || !GetGame().IsServer())
		{
			return;
		}

		// Sets initial state to allow damage tracking. Actual raid checks
		// are now handled instantly at EEOnDamageCalculated and EEHitBy for performance optimization.
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (manager)
		{
			manager.RegisterBase(this);
			SetAllowDamage(manager.IsSDN_RaidManager());
		}
		else
		{
			SetAllowDamage(true);
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		if (GetGame() && GetGame().IsServer())
		{
			SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
			if (manager)
			{
				manager.UnregisterBase(this);
			}
		}
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (GetGame() && GetGame().IsServer())
		{
			SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
			if (manager && manager.IsEnabled() && !manager.IsSDN_RaidManager())
			{
				SDN_RaidManagerNotifyBlockedHit(source, manager);
				return;
			}
		}

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (GetGame() && GetGame().IsServer())
		{
			SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
			
			if (manager && manager.IsEnabled() && !manager.IsSDN_RaidManager())
			{
				// Not raid time: Block damage outright and notify
				SDN_RaidManagerNotifyBlockedHit(source, manager);
				return false; 
			}
		}

		return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	private void SDN_RaidManagerNotifyBlockedHit(EntityAI source, SDN_RaidManagerManager manager)
	{
		if (!source || !manager)
		{
			return;
		}

		PlayerBase player = PlayerBase.Cast(source);
		if (!player)
		{
			player = PlayerBase.Cast(source.GetHierarchyRootPlayer());
		}

		if (player)
		{
			string message = manager.GetRaidUnavailableMessage();
			if (message != "")
			{
				string playerKey = "unknown";
				PlayerIdentity identity = player.GetIdentity();
				if (identity)
				{
					playerKey = identity.GetPlainId();
				}

				if (manager.CanNotifyPlayer(playerKey))
				{
					player.MessageStatus(message);
				}
			}
		}
	}
}
