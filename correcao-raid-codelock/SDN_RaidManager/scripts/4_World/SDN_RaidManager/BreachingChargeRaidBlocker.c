// Macro confirmada em scripts/Common/Define.c do BreachingCharge.
#ifdef BREACHINGCHARGE
modded class HDSN_ActionDeployBreachingCharge
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDeployBreachingCharge())
		{
			return false;
		}

		return super.ActionCondition(player, target, item);
	}

	override bool ActionConditionContinue(ActionData action_data)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDeployBreachingCharge())
		{
			return false;
		}

		return super.ActionConditionContinue(action_data);
	}

	override void OnFinishProgressClient(ActionData action_data)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDeployBreachingCharge())
		{
			return;
		}

		super.OnFinishProgressClient(action_data);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		// Nao confiar apenas na interface: conferir no servidor antes de concluir.
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDeployBreachingCharge())
		{
			return;
		}

		super.OnFinishProgressServer(action_data);
	}
}

#endif
