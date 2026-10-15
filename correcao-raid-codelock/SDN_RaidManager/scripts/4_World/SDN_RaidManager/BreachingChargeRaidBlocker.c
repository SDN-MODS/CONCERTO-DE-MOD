// Requer o BreachingCharge que declara HDSN_ActionDeployBreachingCharge.
// Carregar esse mod antes do SDN_RaidManager, no cliente e no servidor.
modded class HDSN_ActionDeployBreachingCharge
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return false;
		}

		return super.ActionCondition(player, target, item);
	}

	override bool ActionConditionContinue(ActionData action_data)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return false;
		}

		return super.ActionConditionContinue(action_data);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		// Nao confiar apenas na interface: conferir no servidor antes de concluir.
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return;
		}

		super.OnFinishProgressServer(action_data);
	}
}
