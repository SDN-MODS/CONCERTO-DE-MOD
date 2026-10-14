// Fechadura numerica ORIGINAL do DayZ. A integracao do CodeLock depende
// da classe real da acao do mod instalado (nao fornecida no ZIP original).
modded class ActionDestroyCombinationLock
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

	override void OnFinishProgressServer(ActionData action_data)
	{
		// Revalidar antes do efeito: o raid pode terminar durante a acao.
		SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
		if (!manager || !manager.CanDestroyRaidLock())
		{
			return;
		}

		super.OnFinishProgressServer(action_data);
	}
}
