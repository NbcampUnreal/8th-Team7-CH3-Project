#include "PDGameplayTags.h"

namespace PDGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Move,     "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Fire,     "Input.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump,     "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Interact, "Input.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Input_Inventory, "Input.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quest, "Input.Quest");

	//Input 핑
	UE_DEFINE_GAMEPLAY_TAG(Input_Ping, "Input.Ping");
	UE_DEFINE_GAMEPLAY_TAG(Input_PingConfirm, "Input.PingConfirm");

	//Input 월드맵
	UE_DEFINE_GAMEPLAY_TAG(Input_Map, "Input.Map");

	// Input 무기 GAS 어빌리티
	UE_DEFINE_GAMEPLAY_TAG(Input_Reload,        "Input.Reload");
	UE_DEFINE_GAMEPLAY_TAG(Input_FireReleased,   "Input.FireReleased");

	// Input 무기
	UE_DEFINE_GAMEPLAY_TAG(Input_Aim, "Input.Aim");
	UE_DEFINE_GAMEPLAY_TAG(Input_ToggleFireMode, "Input.ToggleFireMode");
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot1, "Input.SwitchSlot1");
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot2, "Input.SwitchSlot2");
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot3, "Input.SwitchSlot3");
	UE_DEFINE_GAMEPLAY_TAG(Input_DropWeapon, "Input.DropWeapon");

	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot1, "Input.Quickslot1");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot2, "Input.Quickslot2");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot3, "Input.Quickslot3");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot4, "Input.Quickslot4");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot5, "Input.Quickslot5");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quickslot6, "Input.Quickslot6");

	UE_DEFINE_GAMEPLAY_TAG(Input_CancelConsumableUse, "Input.CancelConsumableUse");

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");

	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegDamaged,  "State.Debuff.LegDamaged");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegCrippled, "State.Debuff.LegCrippled");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmDamaged,  "State.Debuff.ArmDamaged");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmCrippled, "State.Debuff.ArmCrippled");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Bleeding,    "State.Debuff.Bleeding");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Starving,     "State.Debuff.Starving");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Dehydrated,   "State.Debuff.Dehydrated");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_GasExposure,  "State.Debuff.GasExposure");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Fear,        "State.Debuff.Fear");
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_HeadInjury,  "State.Debuff.HeadInjury");

	UE_DEFINE_GAMEPLAY_TAG(State_buff_Adrenaline, "State.buff.Adrenaline");

	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Rifle,   "Weapon.Type.Rifle");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Shotgun, "Weapon.Type.Shotgun");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Sniper,  "Weapon.Type.Sniper");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Melee,   "Weapon.Type.Melee");
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming");


	// Weapon State
	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Firing, "Weapon.State.Firing");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Reloading, "Weapon.State.Reloading");

	UE_DEFINE_GAMEPLAY_TAG(Cover_Active, "Cover.Active");
	UE_DEFINE_GAMEPLAY_TAG(State_CoverAim, "State.CoverAim");

	UE_DEFINE_GAMEPLAY_TAG(State_Rolling, "State.Rolling");
	UE_DEFINE_GAMEPLAY_TAG(Input_Roll,    "Input.Roll");

	UE_DEFINE_GAMEPLAY_TAG(Input_Cover,   "Input.Cover");

	UE_DEFINE_GAMEPLAY_TAG(State_MeleeAttacking, "State.MeleeAttacking");
	UE_DEFINE_GAMEPLAY_TAG(Anim_Notify_MeleeHit, "Anim.Notify.MeleeHit");

}
