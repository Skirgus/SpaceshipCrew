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

	static FText MakePercentText(const FText& Label, float Value, float MaxValue = 1.f)
	{
		return FText::Format(
			FText::FromString(TEXT("{0}: {1}%")),
			Label,
			FText::AsNumber(FMath::RoundToInt((MaxValue > 0.f ? Value / MaxValue : 0.f) * 100.f))
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
			OxygenReserve = Systems->OxygenReserve;
			bOxygenSupplyEnabled = Systems->bOxygenSupplyEnabled;
			HullIntegrity = Systems->HullIntegrity;
			FireIntensity = Systems->FireIntensity;
			FireHotspotCount = Systems->FireHotspotCount;
			MaxFireHotspots = FMath::Max(1, Systems->MaxFireHotspots);
			AlertHistory = Systems->GetRecentAlerts();
			ModuleStates = Systems->GetCompartments();
			BulkheadStates = Systems->Bulkheads;
			ActiveShipConfigId = Systems->GetActiveShipConfigId();
			ConfigValidationErrors = Systems->GetLastConfigValidationErrors();
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
	return Clamp01(ReactorOutput / 1.2f);
}

float UShipStatusWidget::GetOxygenPercent() const
{
	return Clamp01(OxygenLevel / 21.0f);
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
	return MakePercentText(FText::FromString(TEXT("Reactor")), ReactorOutput, 1.0f);
}

FText UShipStatusWidget::GetReactorStateText() const
{
	FString State = TEXT("SAFE");
	if (ReactorOutput > 1.10f)
	{
		State = TEXT("CRITICAL");
	}
	else if (ReactorOutput > 1.0f)
	{
		State = TEXT("OVERLOAD");
	}
	else if (ReactorOutput >= 0.75f)
	{
		State = TEXT("HIGH");
	}

	return FText::Format(
		FText::FromString(TEXT("Reactor state: {0}")),
		FText::FromString(State)
	);
}

FText UShipStatusWidget::GetOxygenText() const
{
	return FText::Format(
		FText::FromString(TEXT("Oxygen: {0}%")),
		FText::AsNumber(FMath::RoundToInt(FMath::Max(0.f, OxygenLevel)))
	);
}

FText UShipStatusWidget::GetOxygenReserveText() const
{
	return FText::Format(
		FText::FromString(TEXT("O2 Reserve: {0}")),
		FText::AsNumber(FMath::RoundToInt(FMath::Max(0.f, OxygenReserve)))
	);
}

FText UShipStatusWidget::GetOxygenSupplyText() const
{
	return FText::Format(
		FText::FromString(TEXT("O2 Supply: {0}")),
		bOxygenSupplyEnabled ? FText::FromString(TEXT("ON")) : FText::FromString(TEXT("OFF"))
	);
}

FText UShipStatusWidget::GetHullText() const
{
	return MakePercentText(FText::FromString(TEXT("Hull")), HullIntegrity);
}

FText UShipStatusWidget::GetFireText() const
{
	return FText::Format(
		FText::FromString(TEXT("Fire: {0}% ({1}/{2} hotspots)")),
		FText::AsNumber(FMath::RoundToInt(Clamp01(FireIntensity) * 100.f)),
		FText::AsNumber(FireHotspotCount),
		FText::AsNumber(MaxFireHotspots)
	);
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

FText UShipStatusWidget::GetLatestAlertText() const
{
	if (AlertHistory.Num() == 0)
	{
		return FText::FromString(TEXT("Alert: none"));
	}
	const FShipAlertEntry& Last = AlertHistory.Last();
	const TCHAR* Prefix = TEXT("INFO");
	if (Last.Severity == EShipAlertSeverity::Warning)
	{
		Prefix = TEXT("WARN");
	}
	else if (Last.Severity == EShipAlertSeverity::Critical)
	{
		Prefix = TEXT("CRIT");
	}

	return FText::Format(
		FText::FromString(TEXT("Alert [{0}]: {1}")),
		FText::FromString(Prefix),
		Last.Message
	);
}

FText UShipStatusWidget::GetAlertHistoryText() const
{
	if (AlertHistory.Num() == 0)
	{
		return FText::GetEmpty();
	}

	FString Combined;
	const int32 StartIndex = FMath::Max(0, AlertHistory.Num() - 3);
	for (int32 i = StartIndex; i < AlertHistory.Num(); ++i)
	{
		const FShipAlertEntry& Entry = AlertHistory[i];
		const TCHAR* Prefix = TEXT("I");
		if (Entry.Severity == EShipAlertSeverity::Warning)
		{
			Prefix = TEXT("W");
		}
		else if (Entry.Severity == EShipAlertSeverity::Critical)
		{
			Prefix = TEXT("C");
		}

		Combined += FString::Printf(TEXT("[%s] %s"), Prefix, *Entry.Message.ToString());
		if (i < AlertHistory.Num() - 1)
		{
			Combined += TEXT("\n");
		}
	}
	return FText::FromString(Combined);
}

FText UShipStatusWidget::GetModuleStatusText() const
{
	if (ModuleStates.Num() == 0)
	{
		return FText::FromString(TEXT("Modules: none"));
	}

	FString Combined = TEXT("Modules:\n");
	for (int32 i = 0; i < ModuleStates.Num(); ++i)
	{
		const FShipCompartmentState& Module = ModuleStates[i];
		Combined += FString::Printf(
			TEXT("- %s | O2:%d%% | Fire:%d | Breach:%d%%"),
			*Module.ModuleId.ToString(),
			FMath::RoundToInt(FMath::Clamp(Module.OxygenLevel, 0.f, 100.f)),
			Module.FireHotspotCount,
			FMath::RoundToInt(FMath::Clamp(Module.BreachSeverity, 0.f, 1.f) * 100.f)
		);
		if (i < ModuleStates.Num() - 1)
		{
			Combined += TEXT("\n");
		}
	}

	return FText::FromString(Combined);
}

FText UShipStatusWidget::GetBulkheadStatusText() const
{
	if (BulkheadStates.Num() == 0)
	{
		return FText::FromString(TEXT("Bulkheads: none"));
	}

	FString Combined = TEXT("Bulkheads:\n");
	for (int32 i = 0; i < BulkheadStates.Num(); ++i)
	{
		const FShipBulkheadState& Bulkhead = BulkheadStates[i];
		Combined += FString::Printf(
			TEXT("- %s <-> %s : %s"),
			*Bulkhead.ModuleA.ToString(),
			*Bulkhead.ModuleB.ToString(),
			Bulkhead.bOpen ? TEXT("OPEN") : TEXT("CLOSED")
		);
		if (i < BulkheadStates.Num() - 1)
		{
			Combined += TEXT("\n");
		}
	}

	return FText::FromString(Combined);
}

FText UShipStatusWidget::GetActiveConfigText() const
{
	return ActiveShipConfigId.IsNone()
		? FText::FromString(TEXT("Ship Config: Default"))
		: FText::Format(FText::FromString(TEXT("Ship Config: {0}")), FText::FromName(ActiveShipConfigId));
}

FText UShipStatusWidget::GetConfigValidationText() const
{
	if (ConfigValidationErrors.Num() == 0)
	{
		return FText::FromString(TEXT("Config Validate: OK"));
	}

	FString Combined = TEXT("Config Validate:\n");
	for (int32 i = 0; i < ConfigValidationErrors.Num(); ++i)
	{
		Combined += FString::Printf(TEXT("- %s"), *ConfigValidationErrors[i].ToString());
		if (i < ConfigValidationErrors.Num() - 1)
		{
			Combined += TEXT("\n");
		}
	}
	return FText::FromString(Combined);
}

float UShipStatusWidget::GetFireFrameOpacity() const
{
	// Легкая рамка уже при малом огне и сильная при критичном.
	return FMath::Clamp(FMath::GetMappedRangeValueClamped(FVector2D(0.05f, 1.0f), FVector2D(0.0f, 0.85f), FireIntensity), 0.f, 0.85f);
}

FLinearColor UShipStatusWidget::GetFireFrameColor() const
{
	const float Alpha = GetFireFrameOpacity();
	// От темно-красного к ярко-красному по росту пожара.
	const FLinearColor Low(0.35f, 0.02f, 0.02f, Alpha);
	const FLinearColor High(1.0f, 0.05f, 0.05f, Alpha);
	return FMath::Lerp(Low, High, FMath::Clamp(FireIntensity, 0.f, 1.f));
}
