#include "ShipBuilderDomainGlue.h"

#include "ShipBuildDomain.h"
#include "ShipModuleDefinition.h"
#include "ShipModuleTypes.h"

namespace ShipBuilderDomainGluePrivate
{
	enum class ESocketDirection : uint8
	{
		Unknown,
		Front,
		Back,
		Left,
		Right,
		Top,
		Bottom
	};

	struct FEffectiveSocket
	{
		FName SocketName = NAME_None;
		EShipModuleSocketType SocketType = EShipModuleSocketType::Horizontal;
		ESocketDirection Direction = ESocketDirection::Unknown;
	};

	static bool AreSocketTypesCompatible(const EShipModuleSocketType A, const EShipModuleSocketType B)
	{
		return A == EShipModuleSocketType::Universal
			|| B == EShipModuleSocketType::Universal
			|| A == B;
	}

	static bool IsTypeAllowedBySource(const UShipModuleDefinition& Source, const EShipModuleType TargetType)
	{
		// Пустой список трактуется как "без ограничений".
		return Source.CompatibleModuleTypes.Num() == 0
			|| Source.CompatibleModuleTypes.Contains(TargetType);
	}

	static bool AreDefinitionsCompatible(const UShipModuleDefinition& A, const UShipModuleDefinition& B)
	{
		return IsTypeAllowedBySource(A, B.ModuleType) && IsTypeAllowedBySource(B, A.ModuleType);
	}

	static ESocketDirection OppositeDirection(const ESocketDirection Direction)
	{
		switch (Direction)
		{
		case ESocketDirection::Front: return ESocketDirection::Back;
		case ESocketDirection::Back: return ESocketDirection::Front;
		case ESocketDirection::Left: return ESocketDirection::Right;
		case ESocketDirection::Right: return ESocketDirection::Left;
		case ESocketDirection::Top: return ESocketDirection::Bottom;
		case ESocketDirection::Bottom: return ESocketDirection::Top;
		default: return ESocketDirection::Unknown;
		}
	}

	static ESocketDirection GuessDirection(const FShipModuleContactPoint& CP)
	{
		const FString SocketLower = CP.SocketName.ToString().ToLower();
		if (SocketLower.Contains(TEXT("front"))) return ESocketDirection::Front;
		if (SocketLower.Contains(TEXT("back")) || SocketLower.Contains(TEXT("rear"))) return ESocketDirection::Back;
		if (SocketLower.Contains(TEXT("left"))) return ESocketDirection::Left;
		if (SocketLower.Contains(TEXT("right"))) return ESocketDirection::Right;
		if (SocketLower.Contains(TEXT("top")) || SocketLower.Contains(TEXT("up"))) return ESocketDirection::Top;
		if (SocketLower.Contains(TEXT("bottom")) || SocketLower.Contains(TEXT("down"))) return ESocketDirection::Bottom;

		const FVector Abs = CP.RelativeLocation.GetAbs();
		if (Abs.X >= Abs.Y && Abs.X >= Abs.Z) return CP.RelativeLocation.X >= 0.0f ? ESocketDirection::Front : ESocketDirection::Back;
		if (Abs.Y >= Abs.X && Abs.Y >= Abs.Z) return CP.RelativeLocation.Y >= 0.0f ? ESocketDirection::Right : ESocketDirection::Left;
		if (Abs.Z >= Abs.X && Abs.Z >= Abs.Y) return CP.RelativeLocation.Z >= 0.0f ? ESocketDirection::Top : ESocketDirection::Bottom;
		return ESocketDirection::Unknown;
	}

	static void GetEffectiveSockets(const UShipModuleDefinition& Def, TArray<FEffectiveSocket>& Out)
	{
		TArray<FShipModuleContactPoint> Effective;
		Def.GatherEffectiveContactPoints(Effective);
		Out.Reset();
		Out.Reserve(Effective.Num());
		for (const FShipModuleContactPoint& CP : Effective)
		{
			if (CP.SocketName.IsNone())
			{
				continue;
			}
			Out.Add({ CP.SocketName, CP.SocketType, GuessDirection(CP) });
		}
	}

	static bool IsPairDirectionValid(const ESocketDirection ExistingDir, const ESocketDirection NewDir)
	{
		if (ExistingDir == ESocketDirection::Unknown || NewDir == ESocketDirection::Unknown)
		{
			return true;
		}
		return OppositeDirection(ExistingDir) == NewDir;
	}

	static int32 NormalizeYawStep(const int32 YawStep)
	{
		return ((YawStep % 4) + 4) % 4;
	}

	static bool TryFindCompatibleSocketPair(
		const UShipModuleDefinition& ExistingDef,
		const UShipModuleDefinition& NewDef,
		const FIntVector ExistingGridPos,
		const FIntVector NewGridPos,
		const TSet<FName>& UsedExistingSockets,
		FName& OutExistingSocket,
		FName& OutNewSocket)
	{
		if (!AreDefinitionsCompatible(ExistingDef, NewDef))
		{
			return false;
		}

		TArray<FEffectiveSocket> ExistingSockets;
		TArray<FEffectiveSocket> NewSockets;
		GetEffectiveSockets(ExistingDef, ExistingSockets);
		GetEffectiveSockets(NewDef, NewSockets);
		const bool bVerticalAttach = ExistingGridPos.Z != NewGridPos.Z;

		for (const FEffectiveSocket& ExistingCP : ExistingSockets)
		{
			if (ExistingCP.SocketName.IsNone() || UsedExistingSockets.Contains(ExistingCP.SocketName))
			{
				continue;
			}

			for (const FEffectiveSocket& NewCP : NewSockets)
			{
				if (NewCP.SocketName.IsNone())
				{
					continue;
				}

				if (!AreSocketTypesCompatible(ExistingCP.SocketType, NewCP.SocketType))
				{
					continue;
				}

				const bool bVerticalPair = ExistingCP.Direction == ESocketDirection::Top
					|| ExistingCP.Direction == ESocketDirection::Bottom
					|| NewCP.Direction == ESocketDirection::Top
					|| NewCP.Direction == ESocketDirection::Bottom;

				if (bVerticalAttach && !bVerticalPair)
				{
					continue;
				}
				if (!bVerticalAttach && bVerticalPair)
				{
					continue;
				}

				if (IsPairDirectionValid(ExistingCP.Direction, NewCP.Direction))
				{
					OutExistingSocket = ExistingCP.SocketName;
					OutNewSocket = NewCP.SocketName;
					return true;
				}
			}
		}

		return false;
	}
}

bool SpaceshipCrew_BuildDomainFromDraftChain(
	const FShipBuilderDraftConfig& Draft,
	const IShipBuildModuleResolver& Resolver,
	FShipBuildDomainModel& OutModel,
	FString& OutError)
{
	if (Draft.PlacedModules.Num() > 0)
	{
		for (const FShipBuilderDraftConfig::FPlacedModule& Placed : Draft.PlacedModules)
		{
			if (Placed.InstanceId.IsNone() || Placed.ModuleId.IsNone())
			{
				OutError = TEXT("PlacedModules содержит пустой InstanceId или ModuleId.");
				return false;
			}
			if (ShipBuilderDomainGluePrivate::NormalizeYawStep(Placed.YawStep) != Placed.YawStep)
			{
				OutError = FString::Printf(TEXT("YawStep для '%s' должен быть в диапазоне 0..3."), *Placed.InstanceId.ToString());
				return false;
			}
			if (!OutModel.AddRootModule(Placed.InstanceId, Placed.ModuleId, &OutError))
			{
				return false;
			}
		}

		for (const FShipBuilderDraftConfig::FConnection& Connection : Draft.Connections)
		{
			if (!OutModel.AddConnectionBetweenExisting(
				Connection.ModuleAInstanceId,
				Connection.ModuleASocketName,
				Connection.ModuleBInstanceId,
				Connection.ModuleBSocketName,
				&OutError))
			{
				return false;
			}
		}
		return true;
	}

	if (Draft.ModuleIds.Num() == 0)
	{
		return true;
	}

	const FName FirstId = Draft.ModuleIds[0];
	const UShipModuleDefinition* FirstDef = Resolver.ResolveModule(FirstId);
	if (!FirstDef)
	{
		OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *FirstId.ToString());
		return false;
	}

	if (!OutModel.AddRootModule(TEXT("Draft0"), FirstId, &OutError))
	{
		return false;
	}

	TMap<FName, TSet<FName>> UsedSocketsByInstance;

	for (int32 Index = 1; Index < Draft.ModuleIds.Num(); ++Index)
	{
		const FName NewModuleId = Draft.ModuleIds[Index];
		const UShipModuleDefinition* NewDef = Resolver.ResolveModule(NewModuleId);
		if (!NewDef)
		{
			OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *NewModuleId.ToString());
			return false;
		}

		const FName PrevInstance = *FString::Printf(TEXT("Draft%d"), Index - 1);
		const FName NewInstance = *FString::Printf(TEXT("Draft%d"), Index);
		const UShipModuleDefinition* PrevDef = Resolver.ResolveModule(Draft.ModuleIds[Index - 1]);
		if (!PrevDef)
		{
			OutError = FString::Printf(TEXT("Модуль '%s' не найден в каталоге."), *Draft.ModuleIds[Index - 1].ToString());
			return false;
		}

		FName PrevSocket = NAME_None;
		FName NewSocket = NAME_None;
		const TSet<FName>& UsedPrevSockets = UsedSocketsByInstance.FindOrAdd(PrevInstance);
		const FIntVector PrevGridPos = FIntVector(Index - 1, 0, 0);
		const FIntVector NewGridPos = FIntVector(Index, 0, 0);
		if (!ShipBuilderDomainGluePrivate::TryFindCompatibleSocketPair(*PrevDef, *NewDef, PrevGridPos, NewGridPos, UsedPrevSockets, PrevSocket, NewSocket))
		{
			OutError = FString::Printf(
				TEXT("Не найден свободный совместимый сокет для стыковки '%s' -> '%s'. Проверьте контактные точки и CompatibleModuleTypes."),
				*Draft.ModuleIds[Index - 1].ToString(),
				*NewModuleId.ToString());
			return false;
		}

		if (!OutModel.AddAttachedModule(NewInstance, NewModuleId, PrevInstance, NewSocket, PrevSocket, &OutError))
		{
			return false;
		}

		UsedSocketsByInstance.FindOrAdd(PrevInstance).Add(PrevSocket);
		UsedSocketsByInstance.FindOrAdd(NewInstance).Add(NewSocket);
	}

	return true;
}
