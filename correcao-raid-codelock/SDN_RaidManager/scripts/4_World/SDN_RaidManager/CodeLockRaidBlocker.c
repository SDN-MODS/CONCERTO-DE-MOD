// Integracao com o CodeLock Workshop 1646187754.
// O mod original define CodeLock em scripts/Common/Common.c.
#ifdef CodeLock
modded class ActionDestroyCodeLockOnFence
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return false;
		}

		// Preservar as condicoes do CodeLock, incluindo os valores de HP
		// usados pela interface e pelo calculo original de dano.
		return super.ActionCondition(player, target, item);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return;
		}

		// O CodeLock usa CAContinuousRepeat: este metodo aplica dano em
		// CADA ciclo, nao apenas quando a fechadura termina de ser destruida.
		super.OnFinishProgressServer(action_data);
	}
}

modded class ActionDestroyCodeLockOnTent
{
	override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return false;
		}

		// Preservar as condicoes do CodeLock, incluindo os valores de HP
		// usados pela interface e pelo calculo original de dano.
		return super.ActionCondition(player, target, item);
	}

	override void OnFinishProgressServer(ActionData action_data)
	{
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return;
		}

		// O CodeLock usa CAContinuousRepeat: este metodo aplica dano em
		// CADA ciclo, nao apenas quando a fechadura termina de ser destruida.
		super.OnFinishProgressServer(action_data);
	}
}

#endif
