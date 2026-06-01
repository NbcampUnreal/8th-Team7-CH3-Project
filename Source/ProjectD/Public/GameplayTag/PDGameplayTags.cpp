#include "PDGameplayTags.h"

namespace PDGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Input_Move,     "Input.Move");
	UE_DEFINE_GAMEPLAY_TAG(Input_Fire,     "Input.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Input_Jump,     "Input.Jump");
	UE_DEFINE_GAMEPLAY_TAG(Input_Interact, "Input.Interact");
	UE_DEFINE_GAMEPLAY_TAG(Input_Inventory, "Input.Inventory");
	UE_DEFINE_GAMEPLAY_TAG(Input_Quest, "Input.Quest");
	UE_DEFINE_GAMEPLAY_TAG(Input_UIBack, "Input.UIBack");


	UE_DEFINE_GAMEPLAY_TAG(Input_Ping, "Input.Ping");
	UE_DEFINE_GAMEPLAY_TAG(Input_PingConfirm, "Input.PingConfirm");


	UE_DEFINE_GAMEPLAY_TAG(Input_Map, "Input.Map");


	UE_DEFINE_GAMEPLAY_TAG(Input_Reload,        "Input.Reload");
	UE_DEFINE_GAMEPLAY_TAG(Input_FireReleased,   "Input.FireReleased");


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
	UE_DEFINE_GAMEPLAY_TAG(Input_Bombing, "Input.Bombing");

	UE_DEFINE_GAMEPLAY_TAG(Input_SpectateNext, "Input.SpectateNext");
	UE_DEFINE_GAMEPLAY_TAG(Input_SpectatePrev, "Input.SpectatePrev");

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

	UE_DEFINE_GAMEPLAY_TAG(State_Alive, "State.Alive");
	UE_DEFINE_GAMEPLAY_TAG(State_Downed, "State.Downed");
	UE_DEFINE_GAMEPLAY_TAG(State_GettingUp, "State.GettingUp");
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");

	UE_DEFINE_GAMEPLAY_TAG(State_buff_Adrenaline, "State.buff.Adrenaline");

	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Rifle,   "Weapon.Type.Rifle");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Shotgun, "Weapon.Type.Shotgun");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Sniper,  "Weapon.Type.Sniper");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Type_Melee,   "Weapon.Type.Melee");
	UE_DEFINE_GAMEPLAY_TAG(State_Aiming, "State.Aiming");



	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Firing, "Weapon.State.Firing");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_State_Reloading, "Weapon.State.Reloading");

	UE_DEFINE_GAMEPLAY_TAG(State_Rolling, "State.Rolling");
	UE_DEFINE_GAMEPLAY_TAG(Input_Roll,    "Input.Roll");
	UE_DEFINE_GAMEPLAY_TAG(Input_Sprint,  "Input.Sprint");

	UE_DEFINE_GAMEPLAY_TAG(State_MeleeAttacking, "State.MeleeAttacking");
	UE_DEFINE_GAMEPLAY_TAG(State_Sprinting, "State.Sprinting");
	UE_DEFINE_GAMEPLAY_TAG(Anim_Notify_MeleeHit, "Anim.Notify.MeleeHit");
	UE_DEFINE_GAMEPLAY_TAG(State_Reviving, "State.Reviving");
	UE_DEFINE_GAMEPLAY_TAG(State_UsingConsumable, "State.UsingConsumable");
	UE_DEFINE_GAMEPLAY_TAG(Event_Revive_Start, "Event.Revive.Start");
	UE_DEFINE_GAMEPLAY_TAG(Event_Revive_Cancel, "Event.Revive.Cancel");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_UseConsumable, "Event.Item.UseConsumable");
	UE_DEFINE_GAMEPLAY_TAG(Event_Item_CancelConsumableUse, "Event.Item.CancelConsumableUse");


	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Fire,    "GameplayCue.Weapon.Fire");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Impact,  "GameplayCue.Weapon.Impact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Equip,   "GameplayCue.Weapon.Equip");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Swing,   "GameplayCue.Weapon.Swing");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_MeleeHit,"GameplayCue.Weapon.MeleeHit");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_CartridgeHit, "GameplayCue.Weapon.CartridgeHit");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_Reload, "GameplayCue.Weapon.Reload");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Weapon_ReloadEmpty, "GameplayCue.Weapon.ReloadEmpty");


	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Character_HitReact, "GameplayCue.Character.HitReact");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Character_Bleeding, "GameplayCue.Character.Bleeding");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Character_Roll, "GameplayCue.Character.Roll");
	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Character_Footstep, "GameplayCue.Character.Footstep");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Ability_Bombing, "GameplayCue.Ability.Bombing");

	UE_DEFINE_GAMEPLAY_TAG(GameplayCue_Item_Pickup, "GameplayCue.Item.Pickup");

	// UI Tabs - Player Hub
	UE_DEFINE_GAMEPLAY_TAG(UI_Tab_PlayerHub_Loadout,   "UI.Tab.PlayerHub.Loadout");
	UE_DEFINE_GAMEPLAY_TAG(UI_Tab_PlayerHub_Character, "UI.Tab.PlayerHub.Character");
	UE_DEFINE_GAMEPLAY_TAG(UI_Tab_PlayerHub_Skills,    "UI.Tab.PlayerHub.Skills");
	UE_DEFINE_GAMEPLAY_TAG(UI_Tab_PlayerHub_Quest,     "UI.Tab.PlayerHub.Quest");

}
