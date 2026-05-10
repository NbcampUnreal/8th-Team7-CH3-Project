#include "PDGameplayTags.h"

namespace PDGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Move,     "Input.Move")
	UE_DEFINE_GAMEPLAY_TAG(Input_Fire,     "Input.Fire")
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump,     "Input.Jump")
	UE_DEFINE_GAMEPLAY_TAG(Input_Interact, "Input.Interact")
	UE_DEFINE_GAMEPLAY_TAG(Input_Inventory, "Input.Inventory")

	// Input 무기 GAS 어빌리티
	UE_DEFINE_GAMEPLAY_TAG(Input_Reload, "Input.Reload")

	// Input 무기
	UE_DEFINE_GAMEPLAY_TAG(Input_Zoom, "Input.Zoom")
	UE_DEFINE_GAMEPLAY_TAG(Input_ToggleFireMode, "Input.ToggleFireMode")
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot1, "Input.SwitchSlot1")
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot2, "Input.SwitchSlot2")
	UE_DEFINE_GAMEPLAY_TAG(Input_SwitchSlot3, "Input.SwitchSlot3")
	UE_DEFINE_GAMEPLAY_TAG(Input_DropWeapon, "Input.DropWeapon")

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage")

	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegDamaged,  "State.Debuff.LegDamaged")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_LegCrippled, "State.Debuff.LegCrippled")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmDamaged,  "State.Debuff.ArmDamaged")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_ArmCrippled, "State.Debuff.ArmCrippled")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Bleeding,    "State.Debuff.Bleeding")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Starving,    "State.Debuff.Starving")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Dehydrated,  "State.Debuff.Dehydrated")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_Fear,        "State.Debuff.Fear")
	UE_DEFINE_GAMEPLAY_TAG(State_Debuff_HeadInjury,  "State.Debuff.HeadInjury")

	UE_DEFINE_GAMEPLAY_TAG(State_buff_Adrenaline, "State.buff.Adrenaline")
	
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Rifle,   "Weapon.Type.Rifle")
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Shotgun, "Weapon.Type.Shotgun")
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Sniper,  "Weapon.Type.Sniper")
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming")

	UE_DEFINE_GAMEPLAY_TAG(Anim_Notify_EquipEnd,      "Anim.Notify.EquipEnd")
	UE_DEFINE_GAMEPLAY_TAG(Anim_Notify_ReloadEnd,     "Anim.Notify.ReloadEnd")
	UE_DEFINE_GAMEPLAY_TAG(Anim_Notify_WeaponVisible, "Anim.Notify.WeaponVisible")

	// Weapon State
	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Firing, "Weapon.State.Firing")
	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Reloading, "Weapon.State.Reloading")

}
