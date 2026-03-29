// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShipStatusWidget.h"

#include "CrewInteractionComponent.h"
#include "CrewRoleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "SpaceshipCrewPlayerController.h"
#include "Internationalization/Text.h"
#include "ShipActor.h"
#include "ShipCrewCharacter.h"
#include "ShipGameState.h"
#include "ShipSystemsComponent.h"

namespace
{
	static float Clamp01(float Value)
	{
		return FMath::Clamp(Value, 0.f, 1.f);
	}

	static FText MakePercentText(const FText& Label, float Value)
	{
		return FText::Format(
			FText::FromString(TEXT("{0}: {1}%")),
			Label,
			FText::AsNumber(FMath::RoundToInt(Clamp01(Value) * 100.f))
		);
	}

	static FText GetInteractKeyLabel(AShipCrewCharacter* CrewChar, APlayerController* PlayerController)
	{
		if (!CrewChar || !PlayerController)
		{
			return FText::FromString(TEXT("?"));
		}
		if (!CrewChar->InteractAction)
		{
			return FText::FromString(TEXT("—"));
		}
		if (const ASpaceshipCrewPlayerController* ShipPC = Cast<ASpaceshipCrewPlayerController>(PlayerController))
		{
			const FKey K = ShipPC->GetFirstKeyMappedToAction(CrewChar->InteractAction);
			if (K.IsValid())
			{
				return K.GetDisplayName();
			}
		}
		return FText::FromString(TEXT("E"));
	}
}

void UShipStatusWidget::RefreshFromController(APlayerController* PlayerController)
{
	InteractionPromptText = FText::GetEmpty();

	if (!PlayerController)
	{
		InvalidateLayoutAndVolatility();
		return;
	}

	UWorld* World = PlayerController->GetWorld();
	if (!World)
	{
		InvalidateLayoutAndVolatility();
		return;
	}

	for (TActorIterator<AShipActor> It(World); It; ++It)
	{
		if (const UShipSystemsComponent* Systems = It->GetShipSystems())
		{
			ReactorOutput = Systems->ReactorOutput;
			OxygenLevel = Systems->OxygenLevel;
			HullIntegrity = Systems->HullIntegrity;
			FireIntensity = Systems->FireIntensity;
			break;
		}
	}

	CrewSlotIndex = INDEX_NONE;
	CrewRoleId = NAME_None;

	if (const AShipGameState* ShipState = World->GetGameState<AShipGameState>())
	{
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			for (const FCrewSlotReplicationData& CrewSlotData : ShipState->CrewSlots)
			{
				if (CrewSlotData.AssignedPawn == Pawn)
				{
					CrewSlotIndex = CrewSlotData.SlotIndex;
					CrewRoleId = CrewSlotData.RoleId;
					break;
				}
			}
		}
	}

	if (AShipCrewCharacter* CrewChar = Cast<AShipCrewCharacter>(PlayerController->GetPawn()))
	{
		if (UCrewInteractionComponent* Interact = CrewChar->CrewInteraction)
		{
			switch (Interact->GetInteractAvailability())
			{
			case ECrewInteractAvailability::None:
				break;
			case ECrewInteractAvailability::NeedBindInteractAction:
				InteractionPromptText = FText::FromString(TEXT("Set Interact Action (IA) on BP_ShipCrewCharacter"));
				break;
			case ECrewInteractAvailability::TooFar:
				InteractionPromptText = FText::FromString(TEXT("Too far to interact"));
				break;
			case ECrewInteractAvailability::NoPermission:
				InteractionPromptText = FText::FromString(TEXT("No permission for this station"));
				break;
			case ECrewInteractAvailability::NoCrewRole:
				InteractionPromptText = FText::FromString(TEXT("Crew role not assigned (check game mode / slot)"));
				break;
			case ECrewInteractAvailability::Ready:
			{
				const FText KeyLabel = GetInteractKeyLabel(CrewChar, PlayerController);
				InteractionPromptText = FText::Format(
					FText::FromString(TEXT("Press {0} — interact")),
					KeyLabel);
				break;
			}
			default:
				break;
			}
		}
	}

	// Иначе привязка UMG к GetInteractionPromptText / свойству не перерисовывается после C++-обновления.
	InvalidateLayoutAndVolatility();
}

float UShipStatusWidget::GetReactorPercent() const
{
	return Clamp01(ReactorOutput);
}

float UShipStatusWidget::GetOxygenPercent() const
{
	return Clamp01(OxygenLevel);
}

float UShipStatusWidget::GetHullPercent() const
{
	return Clamp01(HullIntegrity);
}

float UShipStatusWidget::GetFirePercent() const
{
	return Clamp01(FireIntensity);
}

FText UShipStatusWidget::GetReactorText() const
{
	return MakePercentText(FText::FromString(TEXT("Reactor")), ReactorOutput);
}

FText UShipStatusWidget::GetOxygenText() const
{
	return MakePercentText(FText::FromString(TEXT("Oxygen")), OxygenLevel);
}

FText UShipStatusWidget::GetHullText() const
{
	return MakePercentText(FText::FromString(TEXT("Hull")), HullIntegrity);
}

FText UShipStatusWidget::GetFireText() const
{
	return MakePercentText(FText::FromString(TEXT("Fire")), FireIntensity);
}

FText UShipStatusWidget::GetRoleText() const
{
	if (CrewRoleId.IsNone())
	{
		return FText::FromString(TEXT("Role: None"));
	}
	return FText::Format(FText::FromString(TEXT("Role: {0}")), FText::FromName(CrewRoleId));
}

FText UShipStatusWidget::GetSlotText() const
{
	if (CrewSlotIndex == INDEX_NONE)
	{
		return FText::FromString(TEXT("Slot: -"));
	}
	return FText::Format(FText::FromString(TEXT("Slot: {0}")), FText::AsNumber(CrewSlotIndex));
}
