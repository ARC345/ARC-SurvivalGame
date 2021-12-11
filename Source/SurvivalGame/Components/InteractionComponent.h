// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "InteractionComponent.generated.h"

class ASurvivalCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginInteract, ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndInteract, ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBeginFocus, ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEndFocus, ASurvivalCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteract, ASurvivalCharacter*, Character);

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SURVIVALGAME_API UInteractionComponent : public UWidgetComponent
{
	GENERATED_BODY()

	/*--------------FUNCS--------------*/
	public:
		UInteractionComponent();
		virtual void Deactivate() override;
		bool CanInteract(ASurvivalCharacter* Character) const;
		void RefreshWidget();

		void BeginFocus( ASurvivalCharacter* SCharacter);
		void EndFocus( ASurvivalCharacter* SCharacter);

		void BeginInteract( ASurvivalCharacter* SCharacter);
		void EndInteract( ASurvivalCharacter* SCharacter);

		void Interact( ASurvivalCharacter* SCharacter);

		UFUNCTION(BlueprintPure, Category = "Interaction")
			float GetInteractPercentage();

		UFUNCTION(BlueprintCallable, Category = "Interaction")
			void SetInteractableNameText(const FText& NewNameText);

		UFUNCTION(BlueprintCallable, Category = "Interaction")
			void SetInteractableActionText(const FText& NewActionText);
	protected:
	private:
	/*---------------------------------*/
	
	/*---------------VARS--------------*/
	public:

		UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
			FOnBeginInteract OnBeginInteract;

		UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
			FOnEndInteract OnEndInteract;

		UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
			FOnBeginFocus OnBeginFocus;

		UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
			FOnEndFocus OnEndFocus;

		UPROPERTY(EditDefaultsOnly, BlueprintAssignable)
			FOnInteract OnInteract;

		//The time for which the player must hold the interaction key with this object
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
			float InteractionTime;

		//The max distance the player can be away from the actor to interact with it
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
			float InteractionDistance;

		//Name that will come up when the player looks at the interactable
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
			FText InteractableNameText;

		//The verb that describes how the interactable works
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
			FText InteractableActionText;

		//Whether we allow multiple players to interact with the object at the same time
		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
			bool bAllowMultipleInteractors;
	protected:
		UPROPERTY()
			TArray< ASurvivalCharacter*> Interactors;

	private:
	/*---------------------------------*/
};
