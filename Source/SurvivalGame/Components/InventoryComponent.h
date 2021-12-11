// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Item.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UENUM(BlueprintType)
enum class EItemAddResult : uint8
{
	IAR_NoItemsAdded UMETA(DisplayName = "No Items Added"),
	IAR_SomeItemsAdded UMETA(DisplayName = "Some Items Added"),
	IAR_AllItemsAdded UMETA(DisplayName = "All Items Added")
};


USTRUCT(BlueprintType)
struct FItemAddResult
{
	GENERATED_BODY()

public:

	FItemAddResult() {};
	FItemAddResult(int32 InItemQuantity) : AmountToGive(InItemQuantity), AmountActuallyGiven(0) {};
	FItemAddResult(int32 InItemQuantity, int32 InQuantityAdded) : AmountToGive(InItemQuantity), AmountActuallyGiven(InItemQuantity) {};

	UPROPERTY(BlueprintReadOnly, Category = "ItemAddResult")
	int32 AmountToGive;
	UPROPERTY(BlueprintReadOnly, Category = "ItemAddResult")
	int32 AmountActuallyGiven;
	UPROPERTY(BlueprintReadOnly, Category = "ItemAddResult")
	EItemAddResult Result;
	UPROPERTY(BlueprintReadOnly, Category = "ItemAddResult")
	FText ErrorText;
	
	static FItemAddResult AddedNone(const int32 InItemQuantity, const FText& ErrorText)
	{
		FItemAddResult AddedNoneResult(InItemQuantity);
		AddedNoneResult.Result = EItemAddResult::IAR_NoItemsAdded;
		AddedNoneResult.ErrorText = ErrorText;
		return AddedNoneResult;
	}

	static FItemAddResult AddedSome(const int32 InItemQuantity, const int32 AmountActuallyGiven, const FText& ErrorText)
	{
		FItemAddResult AddedSomeResult(InItemQuantity, AmountActuallyGiven);
		AddedSomeResult.Result = EItemAddResult::IAR_SomeItemsAdded;
		AddedSomeResult.ErrorText = ErrorText;
		return AddedSomeResult;
	}

	static FItemAddResult AddedAll(const int32 InItemQuantity)
	{
		FItemAddResult AddedAllResult(InItemQuantity, InItemQuantity);
		AddedAllResult.Result = EItemAddResult::IAR_AllItemsAdded;
		return AddedAllResult;
	}
};
 

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVALGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UItem;

public:	
	// Sets default values for this component's properties
	UInventoryComponent();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItem(UItem* Item);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FItemAddResult TryAddItemFromClass(TSubclassOf<UItem> ItemClass,const int32 Quantity);

	int32 ConsumeItem(UItem* Item);
	int32 ConsumeItem(UItem* Item, const int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(UItem* Item);

	//Utils
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(TSubclassOf<UItem> ItemClass, const int32 Quantity = 1) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItem* FindItem(UItem* Item) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UItem* FindItemByClass(TSubclassOf<UItem> ItemClass) const;
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<UItem*> FindItemsByClass(TSubclassOf<UItem> ItemClass) const;
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	float GetCurrentWeight() const;
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetWeightCapacity(const float NewWeightCapacity);
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetCapacity(const int32 NewCapacity);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE float GetWeightCapacity() const { return WeightCapacity; };
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE int32 GetCapacity() const { return Capacity; };
		
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FORCEINLINE TArray<UItem*> GetItems() const { return Items_Array; };

	UFUNCTION(Client, Reliable)
	void RefreshClientInventory();

	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdated OnInventoryUpdated;


protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Items();

	UPROPERTY()
	int32 ReplicatedItemsKey;

	UItem* AddItem(UItem* Item);

	FItemAddResult TryAddItem_Internal(UItem* Item);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Items, VisibleAnywhere, Category = "Inventory")
	TArray<UItem*> Items_Array;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = 0 , ClampMax = 300))
	int32 Capacity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	float WeightCapacity;

};
