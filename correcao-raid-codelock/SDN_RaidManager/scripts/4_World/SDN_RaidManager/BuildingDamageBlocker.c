modded class BaseBuildingBase
{
	override void EEInit()
	{
		super.EEInit();

		if (!GetGame() || !GetGame().IsServer())
		{
			return;
		}

		// Registrar a estrutura e aplicar a protecao inicial no servidor.
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (manager)
		{
			manager.RegisterBase(this);
			SetAllowDamage(manager.IsSDN_RaidManager());
		}
		else
		{
			SetAllowDamage(false);
		}
	}

	private void SDN_RefreshRaidDamageProtection()
	{
		if (!GetGame() || !GetGame().IsServer())
			return;

		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (manager)
		{
			manager.RegisterBase(this);
			SetAllowDamage(manager.IsSDN_RaidManager());
		}
		else
		{
			SetAllowDamage(false);
		}
	}

	override void OnPartBuiltServer(notnull Man player, string part_name, int action_id)
	{
		super.OnPartBuiltServer(player, part_name, action_id);
		// Construction.BuildPartServer reativa o dano antes deste evento.
		SDN_RefreshRaidDamageProtection();
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();
		SDN_RefreshRaidDamageProtection();
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
			if (!manager || !manager.IsSDN_RaidManager())
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

			if (!manager || !manager.IsSDN_RaidManager())
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
