#include "PDGameplayTags.h"

namespace PDGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Move, "Input.Move")
	UE_DEFINE_GAMEPLAY_TAG(Input_Fire, "Input.Fire")
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump, "Input.Jump")
	UE_DEFINE_GAMEPLAY_TAG(Input_Interact, "Input.Interact")
	
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage")
	
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegDamaged,  "State.Debuff.LegDamaged")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegCrippled, "State.Debuff.LegCrippled")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmDamaged,  "State.Debuff.ArmDamaged")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmCrippled, "State.Debuff.ArmCrippled")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Bleeding,    "State.Debuff.Bleeding")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Starving,    "State.Debuff.Starving")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Dehydrated,  "State.Debuff.Dehydrated")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Fear,       "State.Debuff.Fear")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_HeadInjury, "State.Debuff.HeadInjury")
	
	UE_DEFINE_GAMEPLAY_TAG(State_buff_Adrenaline, "State.buff.Adrenaline")
}