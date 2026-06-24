#pragma once

#include "CoreMinimal.h"

#include "ShipBuilder/ShipBuilderDraftTypes.h"

class IShipBuildModuleResolver;
class FShipBuildDomainModel;
class UShipModuleDefinition;

/**
 * Строит доменную конфигурацию из draft.
 * - Если в draft есть явные PlacedModules/Connections, использует их.
 * - Иначе работает fallback-логика legacy-цепочки по ModuleIds.
 */
bool SpaceshipCrew_BuildDomainFromDraftChain(
	const FShipBuilderDraftConfig& Draft,
	const IShipBuildModuleResolver& Resolver,
	FShipBuildDomainModel& OutModel,
	FString& OutError);

/** Локальный сокет модуля, обращённый к соседу по GridDelta (A -> B). YawStep: 0..3. */
FName SpaceshipCrew_LocalSocketForGridDelta(const FIntVector& GridDeltaFromAToB, int32 ModuleYawStep);

/** Выставляет локальные флаги проёма по имени сокета (Front/Back/Left/Right). */
void SpaceshipCrew_SetLocalHorizontalOpeningFromSocketName(
	FName SocketName,
	bool& bOpenFront,
	bool& bOpenBack,
	bool& bOpenLeft,
	bool& bOpenRight);

struct FShipBuilderModuleWorldPlacement
{
	FIntVector GridPos = FIntVector::ZeroValue;
	int32 YawStep = 0;
	FVector Center = FVector::ZeroVector;
	FVector HalfExtent = FVector::ZeroVector;
};

/** Мировое размещение модуля по GridPos/YawStep (шаги сетки как в конструкторе). */
FShipBuilderModuleWorldPlacement SpaceshipCrew_BuildModuleWorldPlacement(
	const FShipBuilderPlacedModule& Placed,
	const UShipModuleDefinition& Def,
	float GridStepXY = 400.0f,
	float GridStepZ = 300.0f);

/** Центр модуля в мире (GridPos = угол footprint). */
FVector SpaceshipCrew_ComputeModuleWorldCenter(
	const FShipBuilderPlacedModule& Placed,
	const UShipModuleDefinition& Def,
	float GridStepXY = 400.0f,
	float GridStepZ = 300.0f);

/** Подбирает GridPos (угол footprint) для перемещаемого модуля: магнит panel-socket к panel-socket. */
bool SpaceshipCrew_TryFindBestSocketSnapCell(
	const FShipBuilderDraftConfig& Draft,
	const FShipBuilderPlacedModule& MovingModule,
	const UShipModuleDefinition& MovingDef,
	const FIntVector& RawCornerCell,
	FName MovingInstanceId,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	FIntVector& OutBestCornerCell,
	float GridStepXY = 400.0f,
	float GridStepZ = 300.0f,
	float SnapRadiusWorld = 650.0f);

/** Пересчитывает Draft.Connections по геометрическому совпадению panel-socket соседних модулей. */
void SpaceshipCrew_RebuildDraftConnectionsFromAdjacency(
	FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	float GridStepXY = 400.0f,
	float GridStepZ = 300.0f);

/** Следующий угол footprint для добавления модуля в цепочку по +X (пустой draft → (0,0,Z)). */
FIntVector SpaceshipCrew_ComputeNextDraftAppendCornerCell(
	const FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	int32 ZLevel = 0);

/** Миграция schema v1→v2: GridPos из центра в min-угол footprint. */
void SpaceshipCrew_MigrateDraftGridPosCenterToCorner(
	FShipBuilderDraftConfig& Draft,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule);

/** Пересекаются ли footprint двух размещений в ячейках сетки. */
bool SpaceshipCrew_DoModuleFootprintsOverlap(
	const FShipBuilderPlacedModule& A,
	const UShipModuleDefinition& DefA,
	const FShipBuilderPlacedModule& B,
	const UShipModuleDefinition& DefB);

/** Соседние модули касаются гранями (с учётом YawStep); возвращает имена сокетов. */
bool SpaceshipCrew_TryGetSocketDockingBetweenPlacedModules(
	const FShipBuilderModuleWorldPlacement& PlacementA,
	const UShipModuleDefinition& DefA,
	const FShipBuilderModuleWorldPlacement& PlacementB,
	const UShipModuleDefinition& DefB,
	FName& OutSocketA,
	FName& OutSocketB);

/**
 * Приводит имя сокета из связи к panel-формату (Front_X0_Y0_Z0).
 * Legacy-имена (Front/Back/…) сопоставляются с ближайшей panel-точкой.
 */
FName SpaceshipCrew_ResolvePanelSocketName(
	const UShipModuleDefinition& Def,
	FName SocketName,
	const FShipBuilderModuleWorldPlacement* OptionalPlacement = nullptr,
	int32 YawStep = 0,
	const FVector* OptionalSocketWorldPos = nullptr);

/** Совпадает ли panel-сокет (Front_X0_Y0_Z0) с именем из связи (legacy или panel). */
bool SpaceshipCrew_DoPanelSocketsMatch(FName PanelSocketName, FName ConnectionSocketName);

/** Геометрический перехлёст модуля с остальными (footprint + AABB). */
bool SpaceshipCrew_WouldModulePlacementOverlap(
	const FShipBuilderDraftConfig& Draft,
	FName IgnoreInstanceId,
	const FShipBuilderPlacedModule& CandidatePlaced,
	const UShipModuleDefinition& ModuleDef,
	const TFunction<const UShipModuleDefinition*(FName)>& ResolveModule,
	float GridStepXY = 400.0f,
	float GridStepZ = 300.0f);
