modded class HDSN_ActionDeployBreachingCharge : ActionContinuousBase
{
	
	string GetCustomActionText(string objectType, string itemDisplayName)
	{		
		if (IsDestructionUN())
            return "Plant " + itemDisplayName + " on " + objectType;

        
		return " " ;
	}

	override bool ActionConditionContinue( ActionData action_data )
	{
		if (IsDestructionUN())
            return super.ActionConditionContinue(action_data);

        return false;
	}
}