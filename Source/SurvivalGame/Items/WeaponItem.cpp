// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponItem.h"

#include "Player/SurvivalPlayerController.h"
#include "Player/SurvivalCharacter.h"

UWeaponItem::UWeaponItem()
{
}

bool UWeaponItem::Equip(ASurvivalCharacter* SC)
{
	bool bEquipSuccessful = Super::Equip(SC);

	if (bEquipSuccessful && SC)
	{
		SC->EquipWeapon(this);
		return true;
	}
	return false;
}

bool UWeaponItem::UnEquip(ASurvivalCharacter* SC)
{
	bool bEquipSuccessful = Super::UnEquip(SC);

	if (bEquipSuccessful && SC)
	{
		SC->UnEquipWeapon();
		return true;
	}
	return false;
}
