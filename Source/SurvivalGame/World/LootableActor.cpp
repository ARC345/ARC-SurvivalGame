// Fill out your copyright notice in the Description page of Project Settings.


#include "LootableActor.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "World/ItemSpawn.h"
#include "Items/Item.h"
#include "Player/SurvivalCharacter.h"

#define LOCTEXT_NAMESPACE "LootableActor"

// Sets default values
ALootableActor::ALootableActor()
{
	LootContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>("LootContainerMesh");
	SetRootComponent(LootContainerMesh);

	LootInteractionComponent = CreateDefaultSubobject<UInteractionComponent>("LootInteractionComponent");
	LootInteractionComponent->InteractableActionText = LOCTEXT("LootActorText", "Loot");
	LootInteractionComponent->InteractableNameText = LOCTEXT("LootActorName", "Chest");
	LootInteractionComponent->SetupAttachment(GetRootComponent());

	LootInventoryComponent = CreateDefaultSubobject<UInventoryComponent>("LootInventoryComponent");
	LootInventoryComponent->SetCapacity(20);
	LootInventoryComponent->SetWeightCapacity(80.0f);

	LootRoll = FIntPoint(2, 8);

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ALootableActor::BeginPlay()
{
	Super::BeginPlay();
	
	LootInteractionComponent->OnInteract.AddDynamic(this, &ALootableActor::OnInteract);
	if (HasAuthority() && LootTable)
	{
		TArray<FLootTableRow*> SpawnItems;
		LootTable->GetAllRows("", SpawnItems);

		int32 Rolls = FMath::RandRange(LootRoll.GetMin(), LootRoll.GetMax());
		for (int32 i = 0 ; i < Rolls; ++i)
		{
			const FLootTableRow* LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];

			ensure(LootRow);

			float ProbabilityRoll = FMath::FRandRange(0.f, 1.f);

			while (ProbabilityRoll > LootRow->Probability)
			{
				LootRow = SpawnItems[FMath::RandRange(0, SpawnItems.Num() - 1)];
				ProbabilityRoll = FMath::FRandRange(0.f, 1.f);
			}

			if (LootRow && LootRow->Items.Num())
			{
				for (auto& ItemClass : LootRow->Items)
				{
					if (ItemClass)
					{
						const int32 Quantity = Cast<UItem>(ItemClass->GetDefaultObject())->GetQuantity();
						LootInventoryComponent->TryAddItemFromClass(ItemClass, Quantity);
					}
				}
			}
		}
	}
}

void ALootableActor::OnInteract(class ASurvivalCharacter* Character)
{
	if (Character)
	{
		Character->SetLootSource(LootInventoryComponent);
	}
}

#undef LOCTEXT_NAMESPACE