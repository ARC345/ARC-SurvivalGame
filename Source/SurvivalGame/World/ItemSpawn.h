// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "Engine/DataTable.h"
#include "ItemSpawn.generated.h"

USTRUCT(BlueprintType)
struct FLootTableRow : public FTableRowBase
{
	GENERATED_BODY()
public:

	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TArray<TSubclassOf<class UItem>> Items;
	UPROPERTY(EditDefaultsOnly, Category = "Loot", meta = (ClampMin = 0.001f, ClampMax = 1.0f))
	float Probability = 1.f;
};


/**
 * 
 */
UCLASS()
class SURVIVALGAME_API AItemSpawn : public ATargetPoint
{
	GENERATED_BODY()

public:
	AItemSpawn();

	UPROPERTY(EditAnywhere, Category = "Loot")
	class UDataTable* LootTable; 
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<class APickup> PickupClass;
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FIntPoint RespawnRange;
protected:
	FTimerHandle TimerHandle_RespawnItem;
	UPROPERTY()
	TArray<AActor*> SpawnedPickups;

	virtual void BeginPlay() override;

	UFUNCTION()
	void SpawnItem();
	UFUNCTION()
	void OnItemTaken(AActor* DestroyedActor);

};
