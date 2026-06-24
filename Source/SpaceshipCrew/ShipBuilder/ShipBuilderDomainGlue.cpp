#include "ShipBuilderDomainGlue.h"

#include "ShipBuildDomain.h"
#include "ShipBuilder/ShipBuilderGridConstants.h"
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

	static FName LocalHorizontalSocketForWorldDirection(const FVector& WorldDirection, const int32 YawStep)
	{
		const FVector Dir = WorldDirection.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return NAME_None;
		}

		struct FFaceEntry
		{
			FName Name;
			FVector LocalNormal;
		};

		static const FFaceEntry Faces[] = {
			{ FName(TEXT("Front")), FVector(1.0f, 0.0f, 0.0f) },
			{ FName(TEXT("Back")), FVector(-1.0f, 0.0f, 0.0f) },
			{ FName(TEXT("Right")), FVector(0.0f, 1.0f, 0.0f) },
			{ FName(TEXT("Left")), FVector(0.0f, -1.0f, 0.0f) },
		};

		const FRotator ModuleYaw(0.0f, static_cast<float>(NormalizeYawStep(YawStep)) * 90.0f, 0.0f);
		float BestDot = -2.0f;
		FName BestName = NAME_None;
		for (const FFaceEntry& Face : Faces)
		{
			const float Dot = FVector::DotProduct(ModuleYaw.RotateVector(Face.LocalNormal), Dir);
			if (Dot > BestDot)
			{
				BestDot = Dot;
				BestName = Face.Name;
			}
		}
		return BestName;
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

	static FVector GetRotatedHalfExtent(const FVector& Size, const int32 YawStep)
	{
		const bool bSwapXY = (NormalizeYawStep(YawStep) % 2) == 1;
		return FVector(
			(bSwapXY ? Size.Y : Size.X) * 0.5f,
			(bSwapXY ? Size.X : Size.Y) * 0.5f,
			Size.Z * 0.5f);
	}

	static FRotator YawStepToRotator(const int32 YawStep)
	{
		return FRotator(0.0f, static_cast<float>(NormalizeYawStep(YawStep)) * 90.0f, 0.0f);
	}

	static FVector GridCornerToWorldCenter(
		const FIntVector& CornerCell,
		const FIntVector& CellSize,
		const float GridStepXY,
		const float GridStepZ)
	{
		const FVector CornerWorld(
			static_cast<float>(CornerCell.X) * GridStepXY,
			static_cast<float>(CornerCell.Y) * GridStepXY,
			static_cast<float>(CornerCell.Z) * GridStepZ);
		return CornerWorld + ShipBuilderGrid::CellSizeToWorldSize(CellSize) * 0.5f;
	}

	static FIntVector CornerFromDesiredCenter(
		const FVector& DesiredCenter,
		const FIntVector& CellSize,
		const float GridStepXY,
		const float GridStepZ)
	{
		return ShipBuilderGrid::WorldCenterToGridCorner(DesiredCenter, CellSize, GridStepXY, GridStepZ);
	}

	static FVector GetSocketWorldPosition(
		const FVector& ModuleCenter,
		const int32 YawStep,
		const FShipModuleContactPoint& Socket)
	{
		return ModuleCenter + YawStepToRotator(YawStep).RotateVector(Socket.RelativeLocation);
	}

	static bool DoWorldBoxesOverlap(
		const FVector& CenterA,
		const FVector& HalfA,
		const FVector& CenterB,
		const FVector& HalfB,
		const float SeparationEpsilon = 5.0f)
	{
		const FVector Delta = (CenterA - CenterB).GetAbs();
		return Delta.X < (HalfA.X + HalfB.X - SeparationEpsilon)
			&& Delta.Y < (HalfA.Y + HalfB.Y - SeparationEpsilon)
			&& Delta.Z < (HalfA.Z + HalfB.Z - SeparationEpsilon);
	}

	static bool TryGetTouchDirectionFromAToB(
		const FVector& Delta,
		const FVector& HalfA,
		const FVector& HalfB,
		FVector& OutDirectionFromAToB)
	{
		const float TouchEpsilon = 20.0f;
		const FVector AbsDelta = Delta.GetAbs();

		auto TryAxis = [&](const int32 Axis, const float ExpectedDistance) -> bool
		{
			if (!FMath::IsNearlyEqual(AbsDelta[Axis], ExpectedDistance, TouchEpsilon))
			{
				return false;
			}
			for (int32 OtherAxis = 0; OtherAxis < 3; ++OtherAxis)
			{
				if (OtherAxis == Axis)
				{
					continue;
				}
				if (AbsDelta[OtherAxis] > HalfA[OtherAxis] + HalfB[OtherAxis] - TouchEpsilon)
				{
					return false;
				}
			}
			OutDirectionFromAToB = FVector::ZeroVector;
			OutDirectionFromAToB[Axis] = FMath::Sign(Delta[Axis]);
			return !FMath::IsNearlyZero(OutDirectionFromAToB[Axis]);
		};

		if (TryAxis(0, HalfA.X + HalfB.X))
		{
			return true;
		}
		if (TryAxis(1, HalfA.Y + HalfB.Y))
		{
			return true;
		}
		if (TryAxis(2, HalfA.Z + HalfB.Z))
		{
			return true;
		}
		return false;
	}

	static bool IsVerticalSocketContact(const FShipModuleContactPoint& Socket)
	{
		const ESocketDirection Direction = GuessDirection(Socket);
		return Direction == ESocketDirection::Top || Direction == ESocketDirection::Bottom;
	}

	static void GatherContactPointsForPlacement(const UShipModuleDefinition& Def, TArray<FShipModuleContactPoint>& OutPoints)
	{
		Def.GatherContactPointsForPlacement(OutPoints);
	}

	static bool FindContactPointByName(
		const UShipModuleDefinition& Def,
		const FName SocketName,
		FShipModuleContactPoint& OutPoint)
	{
		TArray<FShipModuleContactPoint> Effective;
		GatherContactPointsForPlacement(Def, Effective);
		for (const FShipModuleContactPoint& Point : Effective)
		{
			if (Point.SocketName == SocketName)
			{
				OutPoint = Point;
				return true;
			}
		}
		return false;
	}

	static FShipBuilderModuleWorldPlacement BuildModuleWorldPlacementInternal(
		const FShipBuilderPlacedModule& Placed,
		const UShipModuleDefinition& Def,
		const float GridStepXY,
		const float GridStepZ)
	{
		FShipBuilderModuleWorldPlacement Result;
		Result.GridPos = Placed.GridPos;
		Result.YawStep = NormalizeYawStep(Placed.YawStep);
		Result.HalfExtent = GetRotatedHalfExtent(Def.Size, Result.YawStep);
		Result.Center = GridCornerToWorldCenter(
			Placed.GridPos,
			Def.GetEffectiveCellSize(),
			GridStepXY,
			GridStepZ);
		return Result;
	}

	static bool WouldFootprintOverlapDraft(
		const FShipBuilderDraftConfig& Draft,
		const FName MovingInstanceId,
		const FIntVector& CandidateCorner,
		const FIntVector& CandidateCellSize,
		const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule)
	{
		for (const FShipBuilderPlacedModule& OtherPlaced : Draft.PlacedModules)
		{
			if (OtherPlaced.InstanceId == MovingInstanceId)
			{
				continue;
			}
			const UShipModuleDefinition* OtherDef = ResolveModule(OtherPlaced.ModuleId);
			if (!OtherDef)
			{
				continue;
			}
			if (ShipBuilderGrid::DoFootprintsOverlap(
				CandidateCorner,
				CandidateCellSize,
				OtherPlaced.GridPos,
				OtherDef->GetEffectiveCellSize()))
			{
				return true;
			}
		}
		return false;
	}

	static bool WouldExactPlacementOverlapDraft(
		const FShipBuilderDraftConfig& Draft,
		const FName MovingInstanceId,
		const FShipBuilderModuleWorldPlacement& CandidatePlacement,
		const FIntVector& CandidateCellSize,
		const float GridStepXY,
		const float GridStepZ,
		const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule)
	{
		if (WouldFootprintOverlapDraft(
			Draft,
			MovingInstanceId,
			CandidatePlacement.GridPos,
			CandidateCellSize,
			ResolveModule))
		{
			return true;
		}

		for (const FShipBuilderPlacedModule& OtherPlaced : Draft.PlacedModules)
		{
			if (OtherPlaced.InstanceId == MovingInstanceId)
			{
				continue;
			}
			const UShipModuleDefinition* OtherDef = ResolveModule(OtherPlaced.ModuleId);
			if (!OtherDef)
			{
				continue;
			}
			const FShipBuilderModuleWorldPlacement OtherPlacement = BuildModuleWorldPlacementInternal(
				OtherPlaced,
				*OtherDef,
				GridStepXY,
				GridStepZ);
			if (DoWorldBoxesOverlap(
				CandidatePlacement.Center,
				CandidatePlacement.HalfExtent,
				OtherPlacement.Center,
				OtherPlacement.HalfExtent,
				1.0f))
			{
				return true;
			}
		}
		return false;
	}

	static void ConsiderSnapCandidate(
		const FShipBuilderDraftConfig& Draft,
		const FName MovingInstanceId,
		const FShipBuilderPlacedModule& CandidatePlaced,
		const UShipModuleDefinition& MovingDef,
		const FShipBuilderModuleWorldPlacement& CandidatePlacement,
		const FVector& RawCenter,
		const float GridStepXY,
		const float GridStepZ,
		const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
		const float MaxSnapDistSq,
		const float CandidateSocketAlignSq,
		FIntVector& BestCorner,
		float& BestSocketAlignSq,
		float& BestCenterDistSq,
		bool& bFoundSnap)
	{
		if (WouldExactPlacementOverlapDraft(
			Draft,
			MovingInstanceId,
			CandidatePlacement,
			MovingDef.GetEffectiveCellSize(),
			GridStepXY,
			GridStepZ,
			ResolveModule))
		{
			return;
		}

		const float CenterDistSq = FVector::DistSquared(CandidatePlacement.Center, RawCenter);
		if (CenterDistSq > MaxSnapDistSq)
		{
			return;
		}

		const bool bBetterSocketAlign = CandidateSocketAlignSq + 1.0f < BestSocketAlignSq;
		const bool bSameSocketAlignBetterCenter = FMath::IsNearlyEqual(CandidateSocketAlignSq, BestSocketAlignSq, 1.0f)
			&& CenterDistSq < BestCenterDistSq;
		if (bBetterSocketAlign || bSameSocketAlignBetterCenter)
		{
			BestSocketAlignSq = CandidateSocketAlignSq;
			BestCenterDistSq = CenterDistSq;
			BestCorner = CandidatePlaced.GridPos;
			bFoundSnap = true;
		}
	}

	static bool TryFindCoincidentPanelSocketPair(
		const FShipBuilderModuleWorldPlacement& PlacementA,
		const UShipModuleDefinition& DefA,
		const int32 YawA,
		const FShipBuilderModuleWorldPlacement& PlacementB,
		const UShipModuleDefinition& DefB,
		const int32 YawB,
		FName& OutSocketA,
		FName& OutSocketB,
		const float MaxCoincidentDistanceCm,
		const bool bAllowVertical)
	{
		TArray<FShipModuleContactPoint> SocketsA;
		TArray<FShipModuleContactPoint> SocketsB;
		GatherContactPointsForPlacement(DefA, SocketsA);
		GatherContactPointsForPlacement(DefB, SocketsB);

		const float MaxDistSq = MaxCoincidentDistanceCm * MaxCoincidentDistanceCm;
		float BestDistSq = MaxDistSq;
		bool bFound = false;

		for (const FShipModuleContactPoint& SocketA : SocketsA)
		{
			if (!bAllowVertical && IsVerticalSocketContact(SocketA))
			{
				continue;
			}

			const FVector WorldA = GetSocketWorldPosition(PlacementA.Center, YawA, SocketA);
			for (const FShipModuleContactPoint& SocketB : SocketsB)
			{
				if (!AreSocketTypesCompatible(SocketA.SocketType, SocketB.SocketType))
				{
					continue;
				}
				if (!IsPairDirectionValid(GuessDirection(SocketA), GuessDirection(SocketB)))
				{
					continue;
				}
				if (!bAllowVertical && IsVerticalSocketContact(SocketB))
				{
					continue;
				}

				const FVector WorldB = GetSocketWorldPosition(PlacementB.Center, YawB, SocketB);
				const FVector DirFromAToB = PlacementB.Center - PlacementA.Center;
				if (!DirFromAToB.IsNearlyZero())
				{
					const FVector NormalDirAToB = DirFromAToB.GetSafeNormal();
					const FVector OutwardA = (WorldA - PlacementA.Center).GetSafeNormal();
					const FVector OutwardB = (WorldB - PlacementB.Center).GetSafeNormal();
					if (FVector::DotProduct(OutwardA, NormalDirAToB) < 0.35f
						|| FVector::DotProduct(OutwardB, -NormalDirAToB) < 0.35f)
					{
						continue;
					}
				}

				const float DistSq = FVector::DistSquared(WorldA, WorldB);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					OutSocketA = SocketA.SocketName;
					OutSocketB = SocketB.SocketName;
					bFound = true;
				}
			}
		}
		return bFound;
	}

	static bool WouldPlacementOverlapDraft(
		const FShipBuilderDraftConfig& Draft,
		const FName MovingInstanceId,
		const FShipBuilderModuleWorldPlacement& CandidatePlacement,
		const float GridStepXY,
		const float GridStepZ,
		const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule)
	{
		for (const FShipBuilderPlacedModule& OtherPlaced : Draft.PlacedModules)
		{
			if (OtherPlaced.InstanceId == MovingInstanceId)
			{
				continue;
			}
			const UShipModuleDefinition* OtherDef = ResolveModule(OtherPlaced.ModuleId);
			if (!OtherDef)
			{
				continue;
			}
			const FShipBuilderModuleWorldPlacement OtherPlacement = BuildModuleWorldPlacementInternal(
				OtherPlaced,
				*OtherDef,
				GridStepXY,
				GridStepZ);
			if (DoWorldBoxesOverlap(
				CandidatePlacement.Center,
				CandidatePlacement.HalfExtent,
				OtherPlacement.Center,
				OtherPlacement.HalfExtent))
			{
				return true;
			}
		}
		return false;
	}

	static bool AreDockedSocketsOpposing(
		const FShipBuilderModuleWorldPlacement& PlacementA,
		const int32 YawA,
		const UShipModuleDefinition& DefA,
		const FName SocketA,
		const FShipBuilderModuleWorldPlacement& PlacementB,
		const int32 YawB,
		const UShipModuleDefinition& DefB,
		const FName SocketB)
	{
		FShipModuleContactPoint PointA;
		FShipModuleContactPoint PointB;
		if (!FindContactPointByName(DefA, SocketA, PointA) || !FindContactPointByName(DefB, SocketB, PointB))
		{
			return false;
		}
		if (!IsPairDirectionValid(GuessDirection(PointA), GuessDirection(PointB)))
		{
			return false;
		}

		const FVector WorldA = GetSocketWorldPosition(PlacementA.Center, YawA, PointA);
		const FVector WorldB = GetSocketWorldPosition(PlacementB.Center, YawB, PointB);
		const FVector DirFromAToB = PlacementB.Center - PlacementA.Center;
		if (DirFromAToB.IsNearlyZero())
		{
			return true;
		}

		const FVector NormalDirAToB = DirFromAToB.GetSafeNormal();
		const FVector OutwardA = (WorldA - PlacementA.Center).GetSafeNormal();
		const FVector OutwardB = (WorldB - PlacementB.Center).GetSafeNormal();
		return FVector::DotProduct(OutwardA, NormalDirAToB) > 0.35f
			&& FVector::DotProduct(OutwardB, -NormalDirAToB) > 0.35f;
	}

	static bool TryFindBestPanelSocketPair(
		const FShipBuilderModuleWorldPlacement& PlacementA,
		const UShipModuleDefinition& DefA,
		const int32 YawA,
		const FShipBuilderModuleWorldPlacement& PlacementB,
		const UShipModuleDefinition& DefB,
		const int32 YawB,
		FName& OutSocketA,
		FName& OutSocketB)
	{
		const bool bAllowVertical = PlacementA.GridPos.Z != PlacementB.GridPos.Z;
		if (TryFindCoincidentPanelSocketPair(
			PlacementA,
			DefA,
			YawA,
			PlacementB,
			DefB,
			YawB,
			OutSocketA,
			OutSocketB,
			35.0f,
			bAllowVertical))
		{
			return true;
		}

		FVector TouchDirection = FVector::ZeroVector;
		if (!TryGetTouchDirectionFromAToB(
			PlacementB.Center - PlacementA.Center,
			PlacementA.HalfExtent,
			PlacementB.HalfExtent,
			TouchDirection))
		{
			return false;
		}

		TArray<FShipModuleContactPoint> SocketsA;
		TArray<FShipModuleContactPoint> SocketsB;
		GatherContactPointsForPlacement(DefA, SocketsA);
		GatherContactPointsForPlacement(DefB, SocketsB);

		const FVector DirFromAToB = TouchDirection.GetSafeNormal();
		const FVector DirFromBToA = -DirFromAToB;
		float BestDistSq = FMath::Square(80.0f);
		bool bFound = false;

		for (const FShipModuleContactPoint& SocketA : SocketsA)
		{
			const FVector WorldA = GetSocketWorldPosition(PlacementA.Center, YawA, SocketA);
			const FVector OutwardA = (WorldA - PlacementA.Center).GetSafeNormal();
			if (FVector::DotProduct(OutwardA, DirFromAToB) < 0.35f)
			{
				continue;
			}

			for (const FShipModuleContactPoint& SocketB : SocketsB)
			{
				if (!AreSocketTypesCompatible(SocketA.SocketType, SocketB.SocketType))
				{
					continue;
				}
				if (!IsPairDirectionValid(GuessDirection(SocketA), GuessDirection(SocketB)))
				{
					continue;
				}
				const FVector WorldB = GetSocketWorldPosition(PlacementB.Center, YawB, SocketB);
				const FVector OutwardB = (WorldB - PlacementB.Center).GetSafeNormal();
				if (FVector::DotProduct(OutwardB, DirFromBToA) < 0.35f)
				{
					continue;
				}

				const float DistSq = FVector::DistSquared(WorldA, WorldB);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					OutSocketA = SocketA.SocketName;
					OutSocketB = SocketB.SocketName;
					bFound = true;
				}
			}
		}
		return bFound;
	}

	static FName ResolvePanelSocketNameInternal(
		const UShipModuleDefinition& Def,
		const FName SocketName,
		const FShipBuilderModuleWorldPlacement* OptionalPlacement,
		const int32 YawStep,
		const FVector* OptionalSocketWorldPos)
	{
		const FString SocketStr = SocketName.ToString();
		if (SocketStr.Contains(TEXT("_X")))
		{
			return SocketName;
		}

		TArray<FShipModuleContactPoint> PlacementSockets;
		GatherContactPointsForPlacement(Def, PlacementSockets);

		FVector ReferenceWorld = FVector::ZeroVector;
		bool bHasReferenceWorld = false;
		if (OptionalSocketWorldPos)
		{
			ReferenceWorld = *OptionalSocketWorldPos;
			bHasReferenceWorld = true;
		}
		else if (OptionalPlacement)
		{
			FShipModuleContactPoint SourcePoint;
			if (FindContactPointByName(Def, SocketName, SourcePoint))
			{
				ReferenceWorld = GetSocketWorldPosition(OptionalPlacement->Center, YawStep, SourcePoint);
				bHasReferenceWorld = true;
			}
		}

		FVector ReferenceLocal = FVector::ZeroVector;
		bool bHasReferenceLocal = false;
		if (!bHasReferenceWorld)
		{
			FShipModuleContactPoint SourcePoint;
			if (FindContactPointByName(Def, SocketName, SourcePoint))
			{
				ReferenceLocal = SourcePoint.RelativeLocation;
				bHasReferenceLocal = true;
			}
		}

		FName BestPanelName = SocketName;
		float BestDistSq = TNumericLimits<float>::Max();
		int32 PanelMatchesOnFace = 0;
		for (const FShipModuleContactPoint& CP : PlacementSockets)
		{
			const FString PanelStr = CP.SocketName.ToString();
			if (!PanelStr.StartsWith(SocketStr + TEXT("_")))
			{
				continue;
			}
			++PanelMatchesOnFace;

			if (bHasReferenceWorld && OptionalPlacement)
			{
				const FVector PanelWorld = GetSocketWorldPosition(OptionalPlacement->Center, YawStep, CP);
				const float DistSq = FVector::DistSquared(ReferenceWorld, PanelWorld);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestPanelName = CP.SocketName;
				}
			}
			else if (bHasReferenceLocal)
			{
				const float DistSq = FVector::DistSquared(ReferenceLocal, CP.RelativeLocation);
				if (DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestPanelName = CP.SocketName;
				}
			}
			else
			{
				BestPanelName = CP.SocketName;
			}
		}

		if (PanelMatchesOnFace == 1)
		{
			for (const FShipModuleContactPoint& CP : PlacementSockets)
			{
				const FString PanelStr = CP.SocketName.ToString();
				if (PanelStr.StartsWith(SocketStr + TEXT("_")))
				{
					return CP.SocketName;
				}
			}
		}

		return BestPanelName;
	}

	static bool PanelSocketMatchesConnectionName(const FName PanelSocketName, const FName ConnectedSocketName)
	{
		if (PanelSocketName == ConnectedSocketName)
		{
			return true;
		}
		const FString PanelStr = PanelSocketName.ToString();
		const FString ConnStr = ConnectedSocketName.ToString();
		if (PanelStr.StartsWith(ConnStr + TEXT("_")))
		{
			return true;
		}
		if (ConnStr.StartsWith(PanelStr + TEXT("_")))
		{
			return true;
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

FName SpaceshipCrew_LocalSocketForGridDelta(const FIntVector& GridDeltaFromAToB, const int32 ModuleYawStep)
{
	if (GridDeltaFromAToB.Z > 0)
	{
		return FName(TEXT("Top"));
	}
	if (GridDeltaFromAToB.Z < 0)
	{
		return FName(TEXT("Bottom"));
	}

	FVector WorldDirection = FVector::ZeroVector;
	if (GridDeltaFromAToB.X != 0)
	{
		WorldDirection.X = static_cast<float>(FMath::Sign(GridDeltaFromAToB.X));
	}
	else if (GridDeltaFromAToB.Y != 0)
	{
		WorldDirection.Y = static_cast<float>(FMath::Sign(GridDeltaFromAToB.Y));
	}

	return ShipBuilderDomainGluePrivate::LocalHorizontalSocketForWorldDirection(WorldDirection, ModuleYawStep);
}

void SpaceshipCrew_SetLocalHorizontalOpeningFromSocketName(
	const FName SocketName,
	bool& bOpenFront,
	bool& bOpenBack,
	bool& bOpenLeft,
	bool& bOpenRight)
{
	const FString SocketLower = SocketName.ToString().ToLower();
	if (SocketLower.Contains(TEXT("front")))
	{
		bOpenFront = true;
	}
	else if (SocketLower.Contains(TEXT("back")) || SocketLower.Contains(TEXT("rear")))
	{
		bOpenBack = true;
	}
	else if (SocketLower.Contains(TEXT("left")))
	{
		bOpenLeft = true;
	}
	else if (SocketLower.Contains(TEXT("right")))
	{
		bOpenRight = true;
	}
}

FShipBuilderModuleWorldPlacement SpaceshipCrew_BuildModuleWorldPlacement(
	const FShipBuilderPlacedModule& Placed,
	const UShipModuleDefinition& Def,
	const float GridStepXY,
	const float GridStepZ)
{
	return ShipBuilderDomainGluePrivate::BuildModuleWorldPlacementInternal(Placed, Def, GridStepXY, GridStepZ);
}

FVector SpaceshipCrew_ComputeModuleWorldCenter(
	const FShipBuilderPlacedModule& Placed,
	const UShipModuleDefinition& Def,
	const float GridStepXY,
	const float GridStepZ)
{
	return SpaceshipCrew_BuildModuleWorldPlacement(Placed, Def, GridStepXY, GridStepZ).Center;
}

bool SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
	const FShipBuilderModuleWorldPlacement& PlacementA,
	const UShipModuleDefinition& DefA,
	const FShipBuilderModuleWorldPlacement& PlacementB,
	const UShipModuleDefinition& DefB,
	FName& OutSocketA,
	FName& OutSocketB)
{
	return ShipBuilderDomainGluePrivate::TryFindBestPanelSocketPair(
		PlacementA,
		DefA,
		PlacementA.YawStep,
		PlacementB,
		DefB,
		PlacementB.YawStep,
		OutSocketA,
		OutSocketB);
}

FName SpaceshipCrew_ResolvePanelSocketName(
	const UShipModuleDefinition& Def,
	const FName SocketName,
	const FShipBuilderModuleWorldPlacement* OptionalPlacement,
	const int32 YawStep,
	const FVector* OptionalSocketWorldPos)
{
	return ShipBuilderDomainGluePrivate::ResolvePanelSocketNameInternal(
		Def,
		SocketName,
		OptionalPlacement,
		YawStep,
		OptionalSocketWorldPos);
}

bool SpaceshipCrew_DoPanelSocketsMatch(const FName PanelSocketName, const FName ConnectionSocketName)
{
	return ShipBuilderDomainGluePrivate::PanelSocketMatchesConnectionName(PanelSocketName, ConnectionSocketName);
}

static FName ResolveConnectionSocketToPanelName(
	const UShipModuleDefinition& Def,
	const FName SocketName,
	const FShipBuilderModuleWorldPlacement& Placement,
	const int32 YawStep,
	const FVector* OptionalReferenceWorldPos = nullptr)
{
	FVector SocketWorldPos = FVector::ZeroVector;
	const FVector* WorldPosPtr = OptionalReferenceWorldPos;
	if (!WorldPosPtr)
	{
		FShipModuleContactPoint SourcePoint;
		if (ShipBuilderDomainGluePrivate::FindContactPointByName(Def, SocketName, SourcePoint))
		{
			SocketWorldPos = ShipBuilderDomainGluePrivate::GetSocketWorldPosition(
				Placement.Center,
				YawStep,
				SourcePoint);
			WorldPosPtr = &SocketWorldPos;
		}
	}
	return SpaceshipCrew_ResolvePanelSocketName(Def, SocketName, &Placement, YawStep, WorldPosPtr);
}

bool SpaceshipCrew_DoModuleFootprintsOverlap(
	const FShipBuilderPlacedModule& A,
	const UShipModuleDefinition& DefA,
	const FShipBuilderPlacedModule& B,
	const UShipModuleDefinition& DefB)
{
	return ShipBuilderGrid::DoFootprintsOverlap(
		A.GridPos,
		DefA.GetEffectiveCellSize(),
		B.GridPos,
		DefB.GetEffectiveCellSize());
}

FIntVector SpaceshipCrew_ComputeNextDraftAppendCornerCell(
	const FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	const int32 ZLevel)
{
	if (Draft.PlacedModules.Num() == 0)
	{
		return FIntVector(0, 0, ZLevel);
	}

	int32 MaxExtentX = 0;
	for (const FShipBuilderPlacedModule& Placed : Draft.PlacedModules)
	{
		const UShipModuleDefinition* Def = ResolveModule(Placed.ModuleId);
		const FIntVector Cells = Def ? Def->GetEffectiveCellSize() : FIntVector(1, 1, 1);
		MaxExtentX = FMath::Max(MaxExtentX, Placed.GridPos.X + Cells.X);
	}
	return FIntVector(MaxExtentX, 0, ZLevel);
}

void SpaceshipCrew_MigrateDraftGridPosCenterToCorner(
	FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule)
{
	for (FShipBuilderPlacedModule& Placed : Draft.PlacedModules)
	{
		const UShipModuleDefinition* Def = ResolveModule(Placed.ModuleId);
		const FIntVector Cells = Def ? Def->GetEffectiveCellSize() : FIntVector(1, 1, 1);
		Placed.GridPos = ShipBuilderGrid::CenterGridCellToCornerGridCell(Placed.GridPos, Cells);
	}
	Draft.Connections.Reset();
}

bool SpaceshipCrew_WouldModulePlacementOverlap(
	const FShipBuilderDraftConfig& Draft,
	const FName IgnoreInstanceId,
	const FShipBuilderPlacedModule& CandidatePlaced,
	const UShipModuleDefinition& ModuleDef,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	const float GridStepXY,
	const float GridStepZ)
{
	const FShipBuilderModuleWorldPlacement CandidatePlacement = SpaceshipCrew_BuildModuleWorldPlacement(
		CandidatePlaced,
		ModuleDef,
		GridStepXY,
		GridStepZ);
	return ShipBuilderDomainGluePrivate::WouldExactPlacementOverlapDraft(
		Draft,
		IgnoreInstanceId,
		CandidatePlacement,
		ModuleDef.GetEffectiveCellSize(),
		GridStepXY,
		GridStepZ,
		ResolveModule);
}

bool SpaceshipCrew_TryFindBestSocketSnapCell(
	const FShipBuilderDraftConfig& Draft,
	const FShipBuilderPlacedModule& MovingModule,
	const UShipModuleDefinition& MovingDef,
	const FIntVector& RawCornerCell,
	const FName MovingInstanceId,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	FIntVector& OutBestCornerCell,
	const float GridStepXY,
	const float GridStepZ,
	const float SnapRadiusWorld)
{
	const FIntVector MovingCells = MovingDef.GetEffectiveCellSize();

	FShipBuilderPlacedModule RawModuleState = MovingModule;
	RawModuleState.GridPos = RawCornerCell;
	const FShipBuilderModuleWorldPlacement RawPlacement = SpaceshipCrew_BuildModuleWorldPlacement(
		RawModuleState,
		MovingDef,
		GridStepXY,
		GridStepZ);

	const bool bRawOverlaps = ShipBuilderDomainGluePrivate::WouldExactPlacementOverlapDraft(
		Draft,
		MovingInstanceId,
		RawPlacement,
		MovingCells,
		GridStepXY,
		GridStepZ,
		ResolveModule);

	const float SnapRadiusSq = SnapRadiusWorld * SnapRadiusWorld;
	const float MaxSnapDistSq = bRawOverlaps ? TNumericLimits<float>::Max() : SnapRadiusSq;

	FIntVector BestCorner = MovingModule.GridPos;
	float BestSocketAlignSq = TNumericLimits<float>::Max();
	float BestCenterDistSq = TNumericLimits<float>::Max();
	bool bFoundSnap = false;

	TArray<FShipModuleContactPoint> MovingSockets;
	ShipBuilderDomainGluePrivate::GatherContactPointsForPlacement(MovingDef, MovingSockets);
	const FRotator MovingRot = ShipBuilderDomainGluePrivate::YawStepToRotator(MovingModule.YawStep);

	for (const FShipBuilderPlacedModule& AnchorPlaced : Draft.PlacedModules)
	{
		if (AnchorPlaced.InstanceId == MovingInstanceId)
		{
			continue;
		}
		const UShipModuleDefinition* AnchorDef = ResolveModule(AnchorPlaced.ModuleId);
		if (!AnchorDef)
		{
			continue;
		}

		const FShipBuilderModuleWorldPlacement AnchorPlacement = SpaceshipCrew_BuildModuleWorldPlacement(
			AnchorPlaced,
			*AnchorDef,
			GridStepXY,
			GridStepZ);
		const bool bSameFloorAsAnchor = AnchorPlaced.GridPos.Z == MovingModule.GridPos.Z;
		TArray<FShipModuleContactPoint> AnchorSockets;
		ShipBuilderDomainGluePrivate::GatherContactPointsForPlacement(*AnchorDef, AnchorSockets);

		for (const FShipModuleContactPoint& AnchorSocket : AnchorSockets)
		{
			const bool bAnchorSocketVertical = ShipBuilderDomainGluePrivate::IsVerticalSocketContact(AnchorSocket);
			if (bSameFloorAsAnchor && bAnchorSocketVertical)
			{
				continue;
			}

			const FVector AnchorSocketWorld = ShipBuilderDomainGluePrivate::GetSocketWorldPosition(
				AnchorPlacement.Center,
				AnchorPlaced.YawStep,
				AnchorSocket);

			for (const FShipModuleContactPoint& MovingSocket : MovingSockets)
			{
				if (!ShipBuilderDomainGluePrivate::AreSocketTypesCompatible(AnchorSocket.SocketType, MovingSocket.SocketType))
				{
					continue;
				}
				if (!ShipBuilderDomainGluePrivate::IsPairDirectionValid(
					ShipBuilderDomainGluePrivate::GuessDirection(AnchorSocket),
					ShipBuilderDomainGluePrivate::GuessDirection(MovingSocket)))
				{
					continue;
				}
				const bool bMovingSocketVertical = ShipBuilderDomainGluePrivate::IsVerticalSocketContact(MovingSocket);
				if (bSameFloorAsAnchor && bMovingSocketVertical)
				{
					continue;
				}

				const FVector MovingSocketOffset = MovingRot.RotateVector(MovingSocket.RelativeLocation);
				const FVector SnappedCenter = AnchorSocketWorld - MovingSocketOffset;
				FIntVector CandidateCorner = ShipBuilderDomainGluePrivate::CornerFromDesiredCenter(
					SnappedCenter,
					MovingCells,
					GridStepXY,
					GridStepZ);
				if (bSameFloorAsAnchor && !bAnchorSocketVertical && !bMovingSocketVertical)
				{
					CandidateCorner.Z = MovingModule.GridPos.Z;
				}

				FShipBuilderPlacedModule CandidatePlaced = MovingModule;
				CandidatePlaced.GridPos = CandidateCorner;
				const FShipBuilderModuleWorldPlacement CandidatePlacement = SpaceshipCrew_BuildModuleWorldPlacement(
					CandidatePlaced,
					MovingDef,
					GridStepXY,
					GridStepZ);

				const FVector CandidateMovingSocketWorld = ShipBuilderDomainGluePrivate::GetSocketWorldPosition(
					CandidatePlacement.Center,
					MovingModule.YawStep,
					MovingSocket);
				const float SocketAlignSq = FVector::DistSquared(AnchorSocketWorld, CandidateMovingSocketWorld);
				if (SocketAlignSq > FMath::Square(40.0f))
				{
					continue;
				}

				const bool bAllowVerticalForPair = !bSameFloorAsAnchor;
				FName SocketA = NAME_None;
				FName SocketB = NAME_None;
				if (!ShipBuilderDomainGluePrivate::TryFindCoincidentPanelSocketPair(
					AnchorPlacement,
					*AnchorDef,
					AnchorPlaced.YawStep,
					CandidatePlacement,
					MovingDef,
					MovingModule.YawStep,
					SocketA,
					SocketB,
					40.0f,
					bAllowVerticalForPair))
				{
					continue;
				}

				const float WeightedAlignSq = (bAnchorSocketVertical || bMovingSocketVertical)
					? SocketAlignSq
					: SocketAlignSq * 0.85f;

				ShipBuilderDomainGluePrivate::ConsiderSnapCandidate(
					Draft,
					MovingInstanceId,
					CandidatePlaced,
					MovingDef,
					CandidatePlacement,
					RawPlacement.Center,
					GridStepXY,
					GridStepZ,
					ResolveModule,
					MaxSnapDistSq,
					WeightedAlignSq,
					BestCorner,
					BestSocketAlignSq,
					BestCenterDistSq,
					bFoundSnap);
			}
		}
	}

	if (!bFoundSnap
		&& !ShipBuilderDomainGluePrivate::WouldExactPlacementOverlapDraft(
			Draft,
			MovingInstanceId,
			RawPlacement,
			MovingCells,
			GridStepXY,
			GridStepZ,
			ResolveModule))
	{
		ShipBuilderDomainGluePrivate::ConsiderSnapCandidate(
			Draft,
			MovingInstanceId,
			RawModuleState,
			MovingDef,
			RawPlacement,
			RawPlacement.Center,
			GridStepXY,
			GridStepZ,
			ResolveModule,
			MaxSnapDistSq,
			TNumericLimits<float>::Max(),
			BestCorner,
			BestSocketAlignSq,
			BestCenterDistSq,
			bFoundSnap);
	}

	if (bFoundSnap)
	{
		OutBestCornerCell = BestCorner;
		return OutBestCornerCell != MovingModule.GridPos;
	}

	OutBestCornerCell = MovingModule.GridPos;
	return false;
}

void SpaceshipCrew_RebuildDraftConnectionsFromAdjacency(
	FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	const float GridStepXY,
	const float GridStepZ)
{
	Draft.Connections.Reset();
	TSet<FString> SeenPairs;
	for (int32 i = 0; i < Draft.PlacedModules.Num(); ++i)
	{
		for (int32 j = i + 1; j < Draft.PlacedModules.Num(); ++j)
		{
			const FShipBuilderPlacedModule& A = Draft.PlacedModules[i];
			const FShipBuilderPlacedModule& B = Draft.PlacedModules[j];
			const UShipModuleDefinition* DefA = ResolveModule(A.ModuleId);
			const UShipModuleDefinition* DefB = ResolveModule(B.ModuleId);
			if (!DefA || !DefB)
			{
				continue;
			}

			const FShipBuilderModuleWorldPlacement PlacementA = SpaceshipCrew_BuildModuleWorldPlacement(
				A,
				*DefA,
				GridStepXY,
				GridStepZ);
			const FShipBuilderModuleWorldPlacement PlacementB = SpaceshipCrew_BuildModuleWorldPlacement(
				B,
				*DefB,
				GridStepXY,
				GridStepZ);

			FName SocketA = NAME_None;
			FName SocketB = NAME_None;
			if (!SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
				PlacementA,
				*DefA,
				PlacementB,
				*DefB,
				SocketA,
				SocketB))
			{
				continue;
			}

			if (!ShipBuilderDomainGluePrivate::AreDockedSocketsOpposing(
				PlacementA,
				A.YawStep,
				*DefA,
				SocketA,
				PlacementB,
				B.YawStep,
				*DefB,
				SocketB))
			{
				const FIntVector Delta = A.GridPos - B.GridPos;
				const FIntVector DeltaFromAToB(-Delta.X, -Delta.Y, -Delta.Z);
				SocketA = SpaceshipCrew_LocalSocketForGridDelta(DeltaFromAToB, A.YawStep);
				SocketB = SpaceshipCrew_LocalSocketForGridDelta(Delta, B.YawStep);
			}

			const FVector InterfaceCenter = (PlacementA.Center + PlacementB.Center) * 0.5f;

			FShipBuilderDraftConnection Link;
			Link.ModuleAInstanceId = A.InstanceId;
			Link.ModuleBInstanceId = B.InstanceId;
			Link.ModuleASocketName = ResolveConnectionSocketToPanelName(
				*DefA,
				SocketA,
				PlacementA,
				A.YawStep,
				&InterfaceCenter);
			Link.ModuleBSocketName = ResolveConnectionSocketToPanelName(
				*DefB,
				SocketB,
				PlacementB,
				B.YawStep,
				&InterfaceCenter);

			const FString PairKey = FString::Printf(TEXT("%s|%s|%s|%s"),
				*A.InstanceId.ToString(),
				*B.InstanceId.ToString(),
				*Link.ModuleASocketName.ToString(),
				*Link.ModuleBSocketName.ToString());
			if (!SeenPairs.Contains(PairKey))
			{
				SeenPairs.Add(PairKey);
				Draft.Connections.Add(Link);
			}
		}
	}
}
