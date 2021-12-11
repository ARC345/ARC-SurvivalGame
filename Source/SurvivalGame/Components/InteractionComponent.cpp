// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractionComponent.h"
#include "../Player/SurvivalCharacter.h"
#include "../Widget/InteractionWidget.h"

UInteractionComponent::UInteractionComponent()
{
	SetComponentTickEnabled(false);

	InteractionTime = 0.0f;
	InteractionDistance = 200.0f;
	InteractableNameText = FText::FromString("InteractableObject");
	InteractableActionText = FText::FromString("Interact");
	bAllowMultipleInteractors = true;

	Space = EWidgetSpace::Screen;
	DrawSize = FIntPoint(600, 100);
	bDrawAtDesiredSize = true;

	SetActive(true);
	SetHiddenInGame(true);

}

void UInteractionComponent::Deactivate()
{
	Super::Deactivate();

	for (int32 i = Interactors.Num() - 1; i >= 0; --i)
	{
		if (ASurvivalCharacter* Interactor = Interactors[i])
		{
			EndFocus(Interactor);
			EndInteract(Interactor);
		}
	}

	Interactors.Empty();
}

bool UInteractionComponent::CanInteract(class ASurvivalCharacter* Character) const
{
	const bool bPlayerAlreadyInteracting = !bAllowMultipleInteractors && Interactors.Num() >= 1;
	return !bPlayerAlreadyInteracting && IsActive() && GetOwner() != nullptr && Character != nullptr;
}

void UInteractionComponent::RefreshWidget()
{
	if (!bHiddenInGame && GetOwner()->GetNetMode() != NM_DedicatedServer)
	{
		if (UInteractionWidget* InteractionWidget = Cast<UInteractionWidget>(GetUserWidgetObject()))
		{
			InteractionWidget->UpdateInteractionWidget(this);
		}
	}
}

void UInteractionComponent::BeginFocus(class ASurvivalCharacter* SCharacter)
{
	if (!IsActive() || !SCharacter || !GetOwner()) { return; }

	OnBeginInteract.Broadcast(SCharacter);

	SetHiddenInGame(false);

	if (!GetOwner()->HasAuthority())
	{
		for (auto& Comps : GetOwner()->GetComponents())
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comps))
			{
				Prim->SetRenderCustomDepth(true);

			}
		}
	}

	RefreshWidget();
}

void UInteractionComponent::EndFocus(class ASurvivalCharacter* SCharacter)
{
	OnEndFocus.Broadcast(SCharacter);

	SetHiddenInGame(true);

	TArray<UPrimitiveComponent*> out_components;
	GetOwner()->GetComponents<UPrimitiveComponent>(out_components, true);

	if (!GetOwner()->HasAuthority())
	{
		for (auto& VisualComp : out_components)
		{
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(VisualComp))
			{
				Prim->SetRenderCustomDepth(false);
			}
		}
	}
}

void UInteractionComponent::BeginInteract(class ASurvivalCharacter* SCharacter)
{
	if (CanInteract(SCharacter))
	{
		Interactors.AddUnique(SCharacter);
		OnBeginInteract.Broadcast(SCharacter);
	}
}

void UInteractionComponent::EndInteract(class ASurvivalCharacter* SCharacter)
{
	Interactors.RemoveSingle(SCharacter);
	OnEndInteract.Broadcast(SCharacter);
}

void UInteractionComponent::Interact(class ASurvivalCharacter* SCharacter)
{
	if (CanInteract(SCharacter))
	{
		OnInteract.Broadcast(SCharacter);
	}
}

float UInteractionComponent::GetInteractPercentage()
{
	if (Interactors.IsValidIndex(0))
	{
		if (ASurvivalCharacter* Interactor = Interactors[0])
		{
			if (Interactor && Interactor->IsInteracting())
			{
				return 1.f - FMath::Abs(Interactor->GetRemainingInteractionTime() / InteractionTime);
			}
		}
	}
	return 0;
}

void UInteractionComponent::SetInteractableNameText(const FText& NewNameText)
{
	InteractableNameText = NewNameText;
	RefreshWidget();
}

void UInteractionComponent::SetInteractableActionText(const FText& NewActionText)
{
	InteractableActionText = NewActionText;
	RefreshWidget();
}