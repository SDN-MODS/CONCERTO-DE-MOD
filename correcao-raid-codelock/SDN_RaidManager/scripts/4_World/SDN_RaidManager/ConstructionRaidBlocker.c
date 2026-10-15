// Protege a entrada da destruicao de partes, antes de remover materiais
// ou estruturas dependentes. DismantlePartServer permanece inalterado.
modded class Construction
{
	override void DestroyPartServer(Man player, string part_name, int action_id, bool destroyed_by_connected_part = false)
	{
		if (GetGame() && GetGame().IsServer())
		{
			SDN_RaidManagerManager manager = SDN_RaidManagerManager.GetInstance();
			if (!manager || !manager.IsSDN_RaidManager())
			{
				return;
			}
		}

		super.DestroyPartServer(player, part_name, action_id, destroyed_by_connected_part);
	}
}
