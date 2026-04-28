#if WITH_EDITOR

#include "ShipBuilder/ShipModuleEditorToolAssetEditor.h"

#include "ShipBuilder/ShipModuleEditorTool.h"
#include "ShipModule/ShipModuleDefinition.h"
#include "ShipModule/ShipModuleVisualOverride.h"
#include "AdvancedPreviewScene.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "EditorViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "SceneManagement.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "CollisionQueryParams.h"
#include "Editor.h"
#include "SceneView.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "SEditorViewport.h"
#include "UnrealWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"

namespace ShipModuleEditorToolAssetEditorTabs
{
	static const FName Viewport(TEXT("ShipModuleEditorTool_Viewport"));
	static const FName Details(TEXT("ShipModuleEditorTool_Details"));
static const FName Parts(TEXT("ShipModuleEditorTool_Parts"));
}

namespace ShipModuleEditorToolAssetEditorPrivate
{
	static UClass* ResolveActorClassFromAssetData(const FAssetData& Asset)
	{
		if (Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()))
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
			{
				UClass* Generated = Blueprint->GeneratedClass;
				if (Generated && Generated->IsChildOf(AActor::StaticClass()))
				{
					return Generated;
				}
			}
		}
		else if (Asset.GetClass()->IsChildOf(UClass::StaticClass()))
		{
			if (UClass* ClassAsset = Cast<UClass>(Asset.GetAsset()))
			{
				if (ClassAsset->IsChildOf(AActor::StaticClass()))
				{
					return ClassAsset;
				}
			}
		}
		return nullptr;
	}
}

class FShipModuleEditorToolViewportClient final : public FEditorViewportClient
{
public:
	explicit FShipModuleEditorToolViewportClient(FPreviewScene& InPreviewScene, const TSharedRef<SEditorViewport>& InViewportWidget, const TWeakPtr<class SShipModuleEditorToolViewport>& InOwnerViewport)
		: FEditorViewportClient(nullptr, &InPreviewScene, InViewportWidget)
		, OwnerViewport(InOwnerViewport)
	{
		SetViewLocation(FVector(-500.0f, 0.0f, 220.0f));
		SetViewRotation(FRotator(-12.0f, 0.0f, 0.0f));
		SetViewModes(VMI_Lit, VMI_Lit);
		bSetListenerPosition = false;
	}

	virtual FVector GetWidgetLocation() const override;
	virtual bool InputWidgetDelta(
		FViewport* InViewport,
		EAxisList::Type CurrentAxis,
		FVector& Drag,
		FRotator& Rot,
		FVector& Scale) override;
	virtual void Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual UE::Widget::EWidgetMode GetWidgetMode() const override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputAxis(const FInputKeyEventArgs& EventArgs) override;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) override;

private:
	TWeakPtr<class SShipModuleEditorToolViewport> OwnerViewport;
};

class SShipModuleEditorToolViewport final : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SShipModuleEditorToolViewport) {}
		SLATE_ARGUMENT(TFunction<void(int32)>, OnSelectPartFromViewport)
		SLATE_ARGUMENT(TFunction<void()>, OnDuplicateSelectedFromViewport)
		SLATE_ARGUMENT(TFunction<void()>, OnDeleteSelectedFromViewport)
		SLATE_ARGUMENT(TFunction<void()>, OnUseSelectedAssetFromViewport)
		SLATE_ARGUMENT(TFunction<void()>, OnUndoFromViewport)
		SLATE_ARGUMENT(TFunction<void()>, OnRedoFromViewport)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UShipModuleEditorTool* InToolAsset)
	{
		ToolAsset = InToolAsset;
		CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		PreviewScene = MakeUnique<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
		OnSelectPartFromViewport = InArgs._OnSelectPartFromViewport;
		OnDuplicateSelectedFromViewport = InArgs._OnDuplicateSelectedFromViewport;
		OnDeleteSelectedFromViewport = InArgs._OnDeleteSelectedFromViewport;
		OnUseSelectedAssetFromViewport = InArgs._OnUseSelectedAssetFromViewport;
		OnUndoFromViewport = InArgs._OnUndoFromViewport;
		OnRedoFromViewport = InArgs._OnRedoFromViewport;
		SEditorViewport::Construct(SEditorViewport::FArguments());
		RebuildPreview();
	}

	void RebuildPreview()
	{
		PreviewFloorOffsetZ = 0.0f;
		for (USceneComponent* Comp : PreviewComponents)
		{
			if (IsValid(Comp))
			{
				if (PreviewScene)
				{
					PreviewScene->RemoveComponent(Comp);
				}
			}
		}
		if (PreviewScene && PreviewScene->GetWorld())
		{
			for (AActor* Actor : PreviewActors)
			{
				if (IsValid(Actor))
				{
					PreviewScene->GetWorld()->DestroyActor(Actor);
				}
			}
		}
		PreviewActors.Reset();
		PreviewComponents.Reset();
		ComponentToPartIndex.Reset();
		PartIndexToComponent.Reset();

		if (!ToolAsset.IsValid() || !CubeMesh)
		{
			return;
		}

		const UShipModuleDefinition* Def = ToolAsset->TargetModuleDefinition;
		const UShipModuleVisualOverride* OverrideAsset = ToolAsset->TargetVisualOverride;
		if (Def)
		{
			if (const UShipModuleVisualOverride* DefinitionOverride = Def->GetVisualOverride())
			{
				OverrideAsset = DefinitionOverride;
				ToolAsset->TargetVisualOverride = const_cast<UShipModuleVisualOverride*>(OverrideAsset);
			}
		}

		if (OverrideAsset && OverrideAsset->VisualParts.Num() > 0)
		{
			for (int32 PartIndex = 0; PartIndex < OverrideAsset->VisualParts.Num(); ++PartIndex)
			{
				const FShipModuleVisualPart& Part = OverrideAsset->VisualParts[PartIndex];
				UStaticMesh* PartMesh = Part.Mesh.Get();
				UClass* PartActorClass = Part.ActorClass.IsNull() ? nullptr : Part.ActorClass.LoadSynchronous();
				if (PartMesh)
				{
					UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(GetTransientPackage());
					Comp->SetStaticMesh(PartMesh);
					Comp->SetMobility(EComponentMobility::Movable);
					Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
					if (PreviewScene)
					{
						PreviewScene->AddComponent(Comp, Part.RelativeTransform);
					}
					PreviewComponents.Add(Comp);
					ComponentToPartIndex.Add(Comp, PartIndex);
					PartIndexToComponent.Add(PartIndex, Comp);
				}
				else if (PartActorClass && PartActorClass->IsChildOf(AActor::StaticClass()) && PreviewScene && PreviewScene->GetWorld())
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.ObjectFlags = RF_Transient;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AActor* PreviewActor = PreviewScene->GetWorld()->SpawnActor<AActor>(PartActorClass, Part.RelativeTransform, SpawnParams);
					if (!IsValid(PreviewActor))
					{
						continue;
					}

					USceneComponent* RootComp = PreviewActor->GetRootComponent();
					UPrimitiveComponent* PrimaryPrimitive = Cast<UPrimitiveComponent>(RootComp);
					if (!PrimaryPrimitive)
					{
						TArray<UPrimitiveComponent*> Components;
						PreviewActor->GetComponents<UPrimitiveComponent>(Components);
						for (UPrimitiveComponent* Prim : Components)
						{
							if (Prim)
							{
								PrimaryPrimitive = Prim;
								break;
							}
						}
					}
					if (RootComp)
					{
						PreviewComponents.Add(RootComp);
						PartIndexToComponent.Add(PartIndex, RootComp);
						if (PrimaryPrimitive)
						{
							ComponentToPartIndex.Add(PrimaryPrimitive, PartIndex);
						}
						PreviewActors.Add(PreviewActor);
					}
					else
					{
						PreviewScene->GetWorld()->DestroyActor(PreviewActor);
					}
				}
			}
			AlignPreviewAboveFloor();
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] RebuildPreview: override parts=%d mappedComponents=%d"),
				OverrideAsset->VisualParts.Num(), PartIndexToComponent.Num());
			return;
		}

		if (!Def)
		{
			return;
		}

		const FVector Size = Def->Size.ComponentMax(FVector(20.0f, 20.0f, 20.0f));
		const float Thickness = FMath::Clamp(ToolAsset->PanelThickness, 2.0f, FMath::Min3(Size.X, Size.Y, Size.Z) * 0.25f);

		auto AddBox = [this](const FVector& Center, const FVector& BoxSize)
		{
			UStaticMeshComponent* Comp = NewObject<UStaticMeshComponent>(GetTransientPackage());
			Comp->SetStaticMesh(CubeMesh);
			Comp->SetMobility(EComponentMobility::Movable);
			Comp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Comp->SetWorldScale3D(BoxSize / 100.0f);
			if (PreviewScene)
			{
				PreviewScene->AddComponent(Comp, FTransform(FRotator::ZeroRotator, Center, BoxSize / 100.0f));
			}
			PreviewComponents.Add(Comp);
		};

		if (!Def->bHasInterior)
		{
			AddBox(FVector::ZeroVector, Size);
			return;
		}

		AddBox(FVector(0.0f, 0.0f, -Size.Z * 0.5f + Thickness * 0.5f), FVector(Size.X, Size.Y, Thickness));
		AddBox(FVector(0.0f, 0.0f, Size.Z * 0.5f - Thickness * 0.5f), FVector(Size.X, Size.Y, Thickness));
		AddBox(FVector(0.0f, -Size.Y * 0.5f + Thickness * 0.5f, 0.0f), FVector(Size.X, Thickness, Size.Z));
		AddBox(FVector(0.0f, Size.Y * 0.5f - Thickness * 0.5f, 0.0f), FVector(Size.X, Thickness, Size.Z));
		AddBox(FVector(-Size.X * 0.5f + Thickness * 0.5f, 0.0f, 0.0f), FVector(Thickness, Size.Y, Size.Z));
		AddBox(FVector(Size.X * 0.5f - Thickness * 0.5f, 0.0f, 0.0f), FVector(Thickness, Size.Y, Size.Z));
		AlignPreviewAboveFloor();
		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] RebuildPreview: procedural mode, mappedComponents=%d"), PartIndexToComponent.Num());
	}

private:
	void AlignPreviewAboveFloor()
	{
		if (PreviewComponents.Num() == 0)
		{
			return;
		}

		double MinZ = TNumericLimits<double>::Max();
		for (USceneComponent* Comp : PreviewComponents)
		{
			if (!IsValid(Comp))
			{
				continue;
			}
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp);
			if (!Prim)
			{
				continue;
			}
			MinZ = FMath::Min<double>(MinZ, Prim->Bounds.GetBox().Min.Z);
		}
		if (!FMath::IsFinite(MinZ))
		{
			return;
		}

		constexpr float TargetFloorClearanceZ = 10.0f;
		const double DeltaZ = static_cast<double>(TargetFloorClearanceZ) - MinZ;
		if (FMath::IsNearlyZero(static_cast<float>(DeltaZ), 0.1f))
		{
			PreviewFloorOffsetZ = 0.0f;
			return;
		}
		PreviewFloorOffsetZ = static_cast<float>(DeltaZ);

		for (USceneComponent* Comp : PreviewComponents)
		{
			if (!IsValid(Comp))
			{
				continue;
			}
			FTransform World = Comp->GetComponentTransform();
			World.AddToTranslation(FVector(0.0f, 0.0f, static_cast<float>(DeltaZ)));
			Comp->SetWorldTransform(World);
		}
	}

	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
	{
		check(PreviewScene);
		ViewportClient = MakeShared<FShipModuleEditorToolViewportClient>(*PreviewScene.Get(), SharedThis(this), StaticCastSharedRef<SShipModuleEditorToolViewport>(SharedThis(this)));
		return ViewportClient.ToSharedRef();
	}

	virtual void BindCommands() override
	{
		SEditorViewport::BindCommands();
	}

public:
	void SetSelectedPartIndex(const int32 NewIndex)
	{
		SelectedPartIndex = NewIndex;
		if (ViewportClient.IsValid())
		{
			ViewportClient->Invalidate();
		}
	}

	void SetTransformWidgetMode(const UE::Widget::EWidgetMode NewMode)
	{
		CurrentWidgetMode = NewMode;
		if (ViewportClient.IsValid())
		{
			ViewportClient->Invalidate();
		}
	}

	UE::Widget::EWidgetMode GetTransformWidgetMode() const
	{
		return CurrentWidgetMode;
	}

	bool HasSelectedPart() const
	{
		return SelectedPartIndex != INDEX_NONE
			&& ToolAsset.IsValid()
			&& ToolAsset->TargetVisualOverride
			&& ToolAsset->TargetVisualOverride->VisualParts.IsValidIndex(SelectedPartIndex)
			&& PartIndexToComponent.Contains(SelectedPartIndex);
	}

	bool GetSelectedPartBounds(FBox& OutBounds) const
	{
		if (!HasSelectedPart())
		{
			return false;
		}
		const TWeakObjectPtr<USceneComponent>* CompPtr = PartIndexToComponent.Find(SelectedPartIndex);
		if (CompPtr == nullptr || !CompPtr->IsValid())
		{
			return false;
		}
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CompPtr->Get()))
		{
			OutBounds = Prim->Bounds.GetBox();
			return true;
		}
		AActor* Owner = CompPtr->Get()->GetOwner();
		if (IsValid(Owner))
		{
			OutBounds = Owner->GetComponentsBoundingBox(true);
			return OutBounds.IsValid != 0;
		}
		return false;
	}

	void RefreshSelectedPartVisualFromData()
	{
		if (!HasSelectedPart())
		{
			return;
		}

		const FTransform& Relative = ToolAsset->TargetVisualOverride->VisualParts[SelectedPartIndex].RelativeTransform;
		if (const TWeakObjectPtr<USceneComponent>* CompPtr = PartIndexToComponent.Find(SelectedPartIndex))
		{
			if (CompPtr->IsValid())
			{
				FTransform PreviewWorldTransform = Relative;
				PreviewWorldTransform.AddToTranslation(FVector(0.0f, 0.0f, PreviewFloorOffsetZ));
				(*CompPtr)->SetWorldTransform(PreviewWorldTransform);
				if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(CompPtr->Get()))
				{
					Prim->MarkRenderTransformDirty();
				}
			}
		}

		if (ViewportClient.IsValid())
		{
			ViewportClient->Invalidate();
		}
	}

	int32 GetFirstVisiblePartIndex() const
	{
		int32 Result = INDEX_NONE;
		for (const TPair<int32, TWeakObjectPtr<USceneComponent>>& Pair : PartIndexToComponent)
		{
			if (!Pair.Value.IsValid())
			{
				continue;
			}
			if (Result == INDEX_NONE || Pair.Key < Result)
			{
				Result = Pair.Key;
			}
		}
		return Result;
	}

	FVector GetSelectedPartWidgetLocation() const
	{
		if (!HasSelectedPart())
		{
			return FVector::ZeroVector;
		}
		return ToolAsset->TargetVisualOverride->VisualParts[SelectedPartIndex].RelativeTransform.GetTranslation()
			+ FVector(0.0f, 0.0f, PreviewFloorOffsetZ);
	}

	bool ApplyWidgetDeltaToSelectedPart(FVector& Drag, FRotator& Rot, FVector& Scale)
	{
		if (!HasSelectedPart())
		{
			return false;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "MovePart", "Move Ship Module Part"));
		ToolAsset->TargetVisualOverride->SetFlags(RF_Transactional);
		ToolAsset->TargetVisualOverride->Modify();
		FShipModuleVisualPart& Part = ToolAsset->TargetVisualOverride->VisualParts[SelectedPartIndex];
		FTransform T = Part.RelativeTransform;
		T.AddToTranslation(Drag);
		FRotator R = T.Rotator();
		R += Rot;
		T.SetRotation(R.Quaternion());
		if (!Scale.IsNearlyZero())
		{
			T.SetScale3D(T.GetScale3D() + Scale);
		}
		Part.RelativeTransform = T;
		ToolAsset->TargetVisualOverride->MarkPackageDirty();
		if (const TWeakObjectPtr<USceneComponent>* CompPtr = PartIndexToComponent.Find(SelectedPartIndex))
		{
			if (CompPtr->IsValid())
			{
				FTransform PreviewWorldTransform = Part.RelativeTransform;
				PreviewWorldTransform.AddToTranslation(FVector(0.0f, 0.0f, PreviewFloorOffsetZ));
				(*CompPtr)->SetWorldTransform(PreviewWorldTransform);
			}
		}
		return true;
	}

	bool HandleViewportClickSelection(FViewport* InViewport)
	{
		if (!ViewportClient.IsValid() || !InViewport)
		{
			return false;
		}
		return HandleViewportClickSelectionByScreen(InViewport->GetMouseX(), InViewport->GetMouseY(), nullptr);
	}

	bool HandleViewportClickSelectionByScreen(const int32 MouseX, const int32 MouseY, const FSceneView* OptionalSceneView)
	{
		if (!ViewportClient.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ClickSelect: ViewportClient invalid"));
			return false;
		}
		const FSceneView* SceneView = OptionalSceneView;
		if (SceneView == nullptr)
		{
			FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
				ViewportClient->Viewport,
				ViewportClient->GetScene(),
				ViewportClient->EngineShowFlags).SetRealtimeUpdate(ViewportClient->IsRealtime()));
			SceneView = ViewportClient->CalcSceneView(&ViewFamily);
		}
		if (SceneView == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ClickSelect: SceneView null (MouseX=%d MouseY=%d)"), MouseX, MouseY);
			return false;
		}
		FViewportCursorLocation Cursor(SceneView, ViewportClient.Get(), MouseX, MouseY);
		const FVector RayStart = Cursor.GetOrigin();
		const FVector RayDir = Cursor.GetDirection().GetSafeNormal();
		const FVector RayEnd = RayStart + RayDir * 500000.0f;

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ShipModuleEditorClickSelection), true);
		FHitResult HitResult;
		int32 HitPartIndex = INDEX_NONE;
		float BestDistanceSq = TNumericLimits<float>::Max();

		for (const TPair<int32, TWeakObjectPtr<USceneComponent>>& Pair : PartIndexToComponent)
		{
			if (!Pair.Value.IsValid())
			{
				continue;
			}

			USceneComponent* SceneComp = Pair.Value.Get();
			UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(SceneComp);
			if (!Component && IsValid(SceneComp))
			{
				AActor* Owner = SceneComp->GetOwner();
				if (IsValid(Owner))
				{
					TArray<UPrimitiveComponent*> Components;
					Owner->GetComponents<UPrimitiveComponent>(Components);
					for (UPrimitiveComponent* Prim : Components)
					{
						if (Prim)
						{
							Component = Prim;
							break;
						}
					}
				}
			}
			if (!IsValid(Component))
			{
				continue;
			}

			HitResult.Reset();
			float DistSq = TNumericLimits<float>::Max();
			if (Component->LineTraceComponent(HitResult, RayStart, RayEnd, QueryParams))
			{
				DistSq = FVector::DistSquared(RayStart, HitResult.ImpactPoint);
			}
			else
			{
				// Fallback for meshes without collision: choose component when click ray passes near its bounds sphere.
				const FVector Origin = Component->Bounds.Origin;
				const FVector ToCenter = Origin - RayStart;
				const float Projection = FVector::DotProduct(ToCenter, RayDir);
				if (Projection <= 0.0f)
				{
					continue;
				}
				const FVector ClosestPoint = RayStart + RayDir * Projection;
				const float DistanceToRaySq = FVector::DistSquared(ClosestPoint, Origin);
				const float RadiusSq = FMath::Square(Component->Bounds.SphereRadius * 1.15f);
				if (DistanceToRaySq > RadiusSq)
				{
					continue;
				}
				DistSq = FMath::Square(Projection);
			}

			if (DistSq < BestDistanceSq)
			{
				BestDistanceSq = DistSq;
				HitPartIndex = Pair.Key;
			}
		}

		if (HitPartIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ClickSelect: no hit (MouseX=%d MouseY=%d Parts=%d)"), MouseX, MouseY, PartIndexToComponent.Num());
			return false;
		}

		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ClickSelect: hit part index %d"), HitPartIndex);
		if (OnSelectPartFromViewport)
		{
			OnSelectPartFromViewport(HitPartIndex);
			return true;
		}
		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ClickSelect: OnSelectPartFromViewport not bound"));
		return false;
	}

	void RequestDuplicateSelected()
	{
		if (OnDuplicateSelectedFromViewport)
		{
			OnDuplicateSelectedFromViewport();
		}
	}

	void RequestDeleteSelected()
	{
		if (OnDeleteSelectedFromViewport)
		{
			OnDeleteSelectedFromViewport();
		}
	}

	void RequestUseSelectedAsset()
	{
		if (OnUseSelectedAssetFromViewport)
		{
			OnUseSelectedAssetFromViewport();
		}
	}

	void RequestUndo()
	{
		if (OnUndoFromViewport)
		{
			OnUndoFromViewport();
		}
	}

	void RequestRedo()
	{
		if (OnRedoFromViewport)
		{
			OnRedoFromViewport();
		}
	}

	TWeakObjectPtr<UShipModuleEditorTool> ToolAsset;
	TObjectPtr<UStaticMesh> CubeMesh = nullptr;
	TUniquePtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FShipModuleEditorToolViewportClient> ViewportClient;
	TArray<TObjectPtr<USceneComponent>> PreviewComponents;
	TArray<TObjectPtr<AActor>> PreviewActors;
	TMap<const UPrimitiveComponent*, int32> ComponentToPartIndex;
	TMap<int32, TWeakObjectPtr<USceneComponent>> PartIndexToComponent;
	int32 SelectedPartIndex = INDEX_NONE;
	float PreviewFloorOffsetZ = 0.0f;
	UE::Widget::EWidgetMode CurrentWidgetMode = UE::Widget::WM_Translate;
	TFunction<void(int32)> OnSelectPartFromViewport;
	TFunction<void()> OnDuplicateSelectedFromViewport;
	TFunction<void()> OnDeleteSelectedFromViewport;
	TFunction<void()> OnUseSelectedAssetFromViewport;
	TFunction<void()> OnUndoFromViewport;
	TFunction<void()> OnRedoFromViewport;
};

FVector FShipModuleEditorToolViewportClient::GetWidgetLocation() const
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	return Pinned.IsValid() ? Pinned->GetSelectedPartWidgetLocation() : FVector::ZeroVector;
}

bool FShipModuleEditorToolViewportClient::InputWidgetDelta(
	FViewport* InViewport,
	EAxisList::Type CurrentAxis,
	FVector& Drag,
	FRotator& Rot,
	FVector& Scale)
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (Pinned.IsValid() && Pinned->ApplyWidgetDeltaToSelectedPart(Drag, Rot, Scale))
	{
		return true;
	}
	return FEditorViewportClient::InputWidgetDelta(InViewport, CurrentAxis, Drag, Rot, Scale);
}

void FShipModuleEditorToolViewportClient::Draw(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	FEditorViewportClient::Draw(View, PDI);

	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (!Pinned.IsValid() || PDI == nullptr)
	{
		return;
	}

	FBox SelectedBounds(EForceInit::ForceInitToZero);
	if (!Pinned->GetSelectedPartBounds(SelectedBounds))
	{
		return;
	}

	const FVector Expand(2.0f, 2.0f, 2.0f);
	DrawWireBox(PDI, SelectedBounds.ExpandBy(Expand), FLinearColor::Yellow, SDPG_Foreground, 1.5f);
}

UE::Widget::EWidgetMode FShipModuleEditorToolViewportClient::GetWidgetMode() const
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (!Pinned.IsValid() || !Pinned->HasSelectedPart())
	{
		return UE::Widget::WM_None;
	}
	return Pinned->GetTransformWidgetMode();
}

bool FShipModuleEditorToolViewportClient::InputKey(const FInputKeyEventArgs& EventArgs)
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (!Pinned.IsValid())
	{
		return FEditorViewportClient::InputKey(EventArgs);
	}
	const bool bHasCtrl = EventArgs.Viewport
		&& (EventArgs.Viewport->KeyState(EKeys::LeftControl) || EventArgs.Viewport->KeyState(EKeys::RightControl));
	const bool bHasAlt = EventArgs.Viewport
		&& (EventArgs.Viewport->KeyState(EKeys::LeftAlt) || EventArgs.Viewport->KeyState(EKeys::RightAlt));

	if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::LeftMouseButton && EventArgs.Viewport)
	{
		// Fallback click-selection path for this custom viewport; do not steal gizmo axis clicks.
		HHitProxy* HitProxy = EventArgs.Viewport->GetHitProxy(EventArgs.Viewport->GetMouseX(), EventArgs.Viewport->GetMouseY());
		const bool bIsWidgetAxisHit = HitProxy && HitProxy->IsA(HWidgetAxis::StaticGetType());
		if (!bIsWidgetAxisHit && Pinned->HandleViewportClickSelection(EventArgs.Viewport))
		{
			return true;
		}
	}

	if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::Delete)
	{
		Pinned->RequestDeleteSelected();
		return true;
	}

	if (EventArgs.Event == IE_Pressed && EventArgs.Key == EKeys::Enter)
	{
		Pinned->RequestUseSelectedAsset();
		return true;
	}

	if (EventArgs.Event == IE_Pressed && !bHasCtrl && !bHasAlt)
	{
		if (EventArgs.Key == EKeys::W)
		{
			Pinned->SetTransformWidgetMode(UE::Widget::WM_Translate);
			return true;
		}
		if (EventArgs.Key == EKeys::E)
		{
			Pinned->SetTransformWidgetMode(UE::Widget::WM_Rotate);
			return true;
		}
		if (EventArgs.Key == EKeys::R)
		{
			Pinned->SetTransformWidgetMode(UE::Widget::WM_Scale);
			return true;
		}
	}

	if (EventArgs.Event == IE_Pressed
		&& EventArgs.Key == EKeys::W
		&& bHasCtrl)
	{
		Pinned->RequestDuplicateSelected();
		return true;
	}

	if (EventArgs.Event == IE_Pressed
		&& EventArgs.Key == EKeys::Z
		&& bHasCtrl)
	{
		Pinned->RequestUndo();
		return true;
	}

	if (EventArgs.Event == IE_Pressed
		&& EventArgs.Key == EKeys::Y
		&& bHasCtrl)
	{
		Pinned->RequestRedo();
		return true;
	}

	return FEditorViewportClient::InputKey(EventArgs);
}

bool FShipModuleEditorToolViewportClient::InputAxis(const FInputKeyEventArgs& EventArgs)
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (!Pinned.IsValid() || EventArgs.Viewport == nullptr)
	{
		return FEditorViewportClient::InputAxis(EventArgs);
	}

	if (EventArgs.Key != EKeys::MouseX && EventArgs.Key != EKeys::MouseY)
	{
		return FEditorViewportClient::InputAxis(EventArgs);
	}

	const float MouseXDelta = (EventArgs.Key == EKeys::MouseX) ? EventArgs.AmountDepressed : 0.0f;
	const float MouseYDelta = (EventArgs.Key == EKeys::MouseY) ? EventArgs.AmountDepressed : 0.0f;
	const bool bLMB = EventArgs.Viewport->KeyState(EKeys::LeftMouseButton);
	const bool bRMB = EventArgs.Viewport->KeyState(EKeys::RightMouseButton);
	const bool bMMB = EventArgs.Viewport->KeyState(EKeys::MiddleMouseButton);

	// LMB is reserved for selection and gizmo interaction.
	if (bLMB && !bRMB && !bMMB)
	{
		return FEditorViewportClient::InputAxis(EventArgs);
	}

	// MMB drag: move camera (pan in view plane), like Blueprint viewport.
	if (bMMB && !bLMB && !bRMB)
	{
		const FRotator ViewRot = GetViewRotation();
		const FVector Right = FRotationMatrix(ViewRot).GetScaledAxis(EAxis::Y);
		const FVector Up = FRotationMatrix(ViewRot).GetScaledAxis(EAxis::Z);
		const FVector CameraDelta = (Right * MouseXDelta + Up * -MouseYDelta) * 2.0f;
		SetViewLocation(GetViewLocation() + CameraDelta);
		Invalidate();
		return true;
	}

	// RMB drag: change camera look direction without translating position.
	if (bRMB && !bLMB && !bMMB)
	{
		FRotator NewRot = GetViewRotation();
		NewRot.Yaw += MouseXDelta * 0.25f;
		NewRot.Pitch = FMath::Clamp(NewRot.Pitch + MouseYDelta * 0.25f, -89.0f, 89.0f);
		SetViewRotation(NewRot);
		Invalidate();
		return true;
	}

	return FEditorViewportClient::InputAxis(EventArgs);
}

void FShipModuleEditorToolViewportClient::ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY)
{
	const TSharedPtr<SShipModuleEditorToolViewport> Pinned = OwnerViewport.Pin();
	if (Pinned.IsValid() && Event == IE_Pressed && Key == EKeys::LeftMouseButton)
	{
		// Let UE handle gizmo axis clicks, otherwise dragging the widget turns into panel re-selection.
		if (HitProxy && HitProxy->IsA(HWidgetAxis::StaticGetType()))
		{
			FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ProcessClick: LMB pressed at (%u, %u)"), HitX, HitY);
		if (Pinned->HandleViewportClickSelectionByScreen(static_cast<int32>(HitX), static_cast<int32>(HitY), &View))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ProcessClick: selection handled"));
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] ProcessClick: selection not handled"));
	}
	FEditorViewportClient::ProcessClick(View, HitProxy, Key, Event, HitX, HitY);
}

const FName FShipModuleEditorToolAssetEditor::ViewportTabId = ShipModuleEditorToolAssetEditorTabs::Viewport;
const FName FShipModuleEditorToolAssetEditor::DetailsTabId = ShipModuleEditorToolAssetEditorTabs::Details;
const FName FShipModuleEditorToolAssetEditor::PartsTabId = ShipModuleEditorToolAssetEditorTabs::Parts;

void FShipModuleEditorToolAssetEditor::InitEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UShipModuleEditorTool* InToolAsset)
{
	ToolAsset = InToolAsset;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	DetailsView->SetObject(ToolAsset);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FShipModuleEditorToolAssetEditor::OnDetailsFinishedChanging);

	ViewportWidget = SNew(SShipModuleEditorToolViewport, ToolAsset)
		.OnSelectPartFromViewport([this](int32 Index)
		{
			SelectPartIndex(Index, true);
		})
		.OnDuplicateSelectedFromViewport([this]()
		{
			HandleViewportDuplicateSelected();
		})
		.OnDeleteSelectedFromViewport([this]()
		{
			HandleViewportDeleteSelected();
		})
		.OnUseSelectedAssetFromViewport([this]()
		{
			HandleViewportUseSelectedAsset();
		})
		.OnUndoFromViewport([this]()
		{
			if (GEditor)
			{
				GEditor->UndoTransaction();
			}
			OnRefreshPreview();
		})
		.OnRedoFromViewport([this]()
		{
			if (GEditor)
			{
				GEditor->RedoTransaction();
			}
			OnRefreshPreview();
		});

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("ShipModuleEditorToolAssetEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.62f)
				->AddTab(ViewportTabId, ETabState::OpenedTab)
			)
			->Split(
				FTabManager::NewSplitter()->SetOrientation(Orient_Vertical)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.56f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
				)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.44f)
					->AddTab(PartsTabId, ETabState::OpenedTab)
				)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, true, true, InToolAsset);
	ExtendToolbar();
}

FName FShipModuleEditorToolAssetEditor::GetToolkitFName() const
{
	return TEXT("ShipModuleEditorToolAssetEditor");
}

FText FShipModuleEditorToolAssetEditor::GetBaseToolkitName() const
{
	return FText::FromString(TEXT("Ship Module Editor Tool"));
}

FString FShipModuleEditorToolAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("ShipModuleEditorTool");
}

FLinearColor FShipModuleEditorToolAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.15f, 0.5f, 0.9f);
}

void FShipModuleEditorToolAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FShipModuleEditorToolAssetEditor::SpawnViewportTab))
		.SetDisplayName(FText::FromString(TEXT("Viewport")));
	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FShipModuleEditorToolAssetEditor::SpawnDetailsTab))
		.SetDisplayName(FText::FromString(TEXT("Details")));
	InTabManager->RegisterTabSpawner(PartsTabId, FOnSpawnTab::CreateSP(this, &FShipModuleEditorToolAssetEditor::SpawnPartsTab))
		.SetDisplayName(FText::FromString(TEXT("Parts")));
}

void FShipModuleEditorToolAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(PartsTabId);
}

TSharedRef<SDockTab> FShipModuleEditorToolAssetEditor::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SWidget> Content = SNew(SBorder)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Viewport unavailable")))
	];
	if (ViewportWidget.IsValid())
	{
		Content = ViewportWidget.ToSharedRef();
	}

	return SNew(SDockTab)
	[
		Content
	];
}

TSharedRef<SDockTab> FShipModuleEditorToolAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SWidget> Content = SNew(SBorder)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Details unavailable")))
	];
	if (DetailsView.IsValid())
	{
		Content = DetailsView.ToSharedRef();
	}

	return SNew(SDockTab)
	[
		Content
	];
}

TSharedRef<SDockTab> FShipModuleEditorToolAssetEditor::SpawnPartsTab(const FSpawnTabArgs& Args)
{
	RebuildPartItems();

	TSharedRef<SWidget> ListArea =
		SNew(SBorder)
		[
			SAssignNew(PartsListView, SListView<TSharedPtr<int32>>)
			.ListItemsSource(&PartItems)
			.OnGenerateRow(this, &FShipModuleEditorToolAssetEditor::GeneratePartRow)
			.OnSelectionChanged(this, &FShipModuleEditorToolAssetEditor::OnPartSelectionChanged)
		];

	auto AddVisualPartFromAsset = [this](UStaticMesh* InMesh, UClass* InActorClass)
	{
		if (!InMesh && !InActorClass)
		{
			return;
		}

		UShipModuleVisualOverride* Override = ResolveEditableOverride(true);
		if (!Override)
		{
			return;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "AddPartFromPicker", "Add Ship Module Part From Picker"));
		Override->Modify();
		FShipModuleVisualPart NewPart;
		NewPart.Mesh = InMesh;
		NewPart.ActorClass = InActorClass;
		Override->VisualParts.Add(NewPart);
		SelectedPartIndex = Override->VisualParts.Num() - 1;
		Override->MarkPackageDirty();
		OnRefreshPreview();
		SelectPartIndex(SelectedPartIndex, false);
	};

	TSharedPtr<SComboButton> AddPartComboButton;
	TSharedRef<SWidget> AddPartMenu =
		SNew(SBox)
		.WidthOverride(320.0f)
		[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 8.0f, 8.0f, 2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Static Mesh")))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 0.0f, 8.0f, 6.0f))
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UStaticMesh::StaticClass())
			.OnObjectChanged_Lambda([AddVisualPartFromAsset, &AddPartComboButton](const FAssetData& Asset)
			{
				if (UStaticMesh* Mesh = Cast<UStaticMesh>(Asset.GetAsset()))
				{
					AddVisualPartFromAsset(Mesh, nullptr);
					if (AddPartComboButton.IsValid())
					{
						AddPartComboButton->SetIsOpen(false);
					}
				}
			})
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 2.0f, 8.0f, 2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Actor Blueprint")))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(8.0f, 0.0f, 8.0f, 8.0f))
		[
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UBlueprint::StaticClass())
			.OnObjectChanged_Lambda([AddVisualPartFromAsset, &AddPartComboButton](const FAssetData& Asset)
			{
				UClass* ActorClass = ShipModuleEditorToolAssetEditorPrivate::ResolveActorClassFromAssetData(Asset);
				if (ActorClass)
				{
					AddVisualPartFromAsset(nullptr, ActorClass);
					if (AddPartComboButton.IsValid())
					{
						AddPartComboButton->SetIsOpen(false);
					}
				}
			})
		]
		];

	TSharedRef<SWidget> ActionButtons =
		SNew(SUniformGridPanel)
		.SlotPadding(FMargin(2.0f))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SAssignNew(AddPartComboButton, SComboButton)
			.ButtonContent()
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Добавить")))
			]
			.MenuContent()
			[
				AddPartMenu
			]
		]
		+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Duplicate")))
			.OnClicked(this, &FShipModuleEditorToolAssetEditor::OnDuplicatePart)
		]
		+ SUniformGridPanel::Slot(2, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Remove")))
			.OnClicked(this, &FShipModuleEditorToolAssetEditor::OnRemovePart)
		];

	auto GetSelectedTransform = [this]() -> const FTransform*
	{
		if (UShipModuleVisualOverride* Override = ResolveEditableOverride(false))
		{
			if (Override->VisualParts.IsValidIndex(SelectedPartIndex))
			{
				return &Override->VisualParts[SelectedPartIndex].RelativeTransform;
			}
		}
		return nullptr;
	};

	auto CommitSelectedTransform = [this](TFunctionRef<void(FTransform&)> Mutator)
	{
		UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
		if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
		{
			return;
		}
		const int32 PreservedPartIndex = SelectedPartIndex;
		const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "EditTransformFields", "Edit Ship Module Part Transform"));
		Override->Modify();
		Mutator(Override->VisualParts[SelectedPartIndex].RelativeTransform);
		Override->MarkPackageDirty();
		OnRefreshPreview();
		SelectPartIndex(PreservedPartIndex, false);
		if (ViewportWidget.IsValid())
		{
			ViewportWidget->Invalidate();
		}
	};

	auto PreviewSelectedTransformValue = [this](int32 Row, int32 Axis, float NewValue)
	{
		UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
		if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
		{
			return;
		}

		FTransform& T = Override->VisualParts[SelectedPartIndex].RelativeTransform;
		if (Row == 0)
		{
			FVector L = T.GetTranslation();
			if (Axis == 0) L.X = NewValue;
			else if (Axis == 1) L.Y = NewValue;
			else L.Z = NewValue;
			T.SetTranslation(L);
		}
		else if (Row == 1)
		{
			FRotator R = T.Rotator();
			if (Axis == 0) R.Roll = NewValue;
			else if (Axis == 1) R.Pitch = NewValue;
			else R.Yaw = NewValue;
			T.SetRotation(R.Quaternion());
		}
		else
		{
			FVector S = T.GetScale3D();
			if (Axis == 0) S.X = NewValue;
			else if (Axis == 1) S.Y = NewValue;
			else S.Z = NewValue;
			T.SetScale3D(S);
		}

		if (ViewportWidget.IsValid())
		{
			ViewportWidget->RefreshSelectedPartVisualFromData();
		}
	};

	auto OptionalComponent = [](const FTransform* T, int32 Row, int32 Axis) -> TOptional<float>
	{
		if (!T)
		{
			return TOptional<float>();
		}
		if (Row == 0)
		{
			const FVector L = T->GetTranslation();
			return Axis == 0 ? L.X : (Axis == 1 ? L.Y : L.Z);
		}
		if (Row == 1)
		{
			const FRotator R = T->Rotator();
			return Axis == 0 ? R.Roll : (Axis == 1 ? R.Pitch : R.Yaw);
		}
		const FVector S = T->GetScale3D();
		return Axis == 0 ? S.X : (Axis == 1 ? S.Y : S.Z);
	};

	TSharedRef<SGridPanel> TransformGrid =
		SNew(SGridPanel)
		.FillColumn(0, 0.9f)
		.FillColumn(1, 1.0f)
		.FillColumn(2, 1.0f)
		.FillColumn(3, 1.0f)
		+ SGridPanel::Slot(0, 0).Padding(FMargin(2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("")))
		]
		+ SGridPanel::Slot(1, 0).Padding(FMargin(2.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("X")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.25f, 0.25f)))
		]
		+ SGridPanel::Slot(2, 0).Padding(FMargin(2.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Y")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.25f, 0.85f, 0.25f)))
		]
		+ SGridPanel::Slot(3, 0).Padding(FMargin(2.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Z")))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.50f, 1.00f)))
		]
		+ SGridPanel::Slot(0, 1).Padding(FMargin(2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Location")))
		]
		+ SGridPanel::Slot(0, 2).Padding(FMargin(2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Rotation")))
		]
		+ SGridPanel::Slot(0, 3).Padding(FMargin(2.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Scale")))
		];

	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const int32 LocalRow = Row;
			const int32 LocalAxis = Axis;
			TransformGrid->AddSlot(LocalAxis + 1, LocalRow + 1)
			.Padding(FMargin(2.0f))
			[
				SNew(SNumericEntryBox<float>)
				.MinDesiredValueWidth(70.0f)
				.Value_Lambda([GetSelectedTransform, OptionalComponent, LocalRow, LocalAxis]()
				{
					return OptionalComponent(GetSelectedTransform(), LocalRow, LocalAxis);
				})
				.OnValueChanged_Lambda([PreviewSelectedTransformValue, LocalRow, LocalAxis](float NewValue)
				{
					PreviewSelectedTransformValue(LocalRow, LocalAxis, NewValue);
				})
				.OnValueCommitted_Lambda([CommitSelectedTransform, LocalRow, LocalAxis](float NewValue, ETextCommit::Type)
				{
					CommitSelectedTransform([LocalRow, LocalAxis, NewValue](FTransform& T)
					{
						if (LocalRow == 0)
						{
							FVector L = T.GetTranslation();
							if (LocalAxis == 0) L.X = NewValue;
							else if (LocalAxis == 1) L.Y = NewValue;
							else L.Z = NewValue;
							T.SetTranslation(L);
						}
						else if (LocalRow == 1)
						{
							FRotator R = T.Rotator();
							if (LocalAxis == 0) R.Roll = NewValue;
							else if (LocalAxis == 1) R.Pitch = NewValue;
							else R.Yaw = NewValue;
							T.SetRotation(R.Quaternion());
						}
						else
						{
							FVector S = T.GetScale3D();
							if (LocalAxis == 0) S.X = NewValue;
							else if (LocalAxis == 1) S.Y = NewValue;
							else S.Z = NewValue;
							T.SetScale3D(S);
						}
					});
				})
			];
		}
	}
	TSharedRef<SWidget> TransformFields = TransformGrid;

	TSharedRef<SWidget> GizmoModeButtons =
		SNew(SUniformGridPanel)
		.SlotPadding(FMargin(2.0f))
		+ SUniformGridPanel::Slot(0, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Move (W)")))
			.OnClicked_Lambda([this]()
			{
				if (ViewportWidget.IsValid())
				{
					ViewportWidget->SetTransformWidgetMode(UE::Widget::WM_Translate);
				}
				return FReply::Handled();
			})
		]
		+ SUniformGridPanel::Slot(1, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Rotate (E)")))
			.OnClicked_Lambda([this]()
			{
				if (ViewportWidget.IsValid())
				{
					ViewportWidget->SetTransformWidgetMode(UE::Widget::WM_Rotate);
				}
				return FReply::Handled();
			})
		]
		+ SUniformGridPanel::Slot(2, 0)
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Scale (R)")))
			.OnClicked_Lambda([this]()
			{
				if (ViewportWidget.IsValid())
				{
					ViewportWidget->SetTransformWidgetMode(UE::Widget::WM_Scale);
				}
				return FReply::Handled();
			})
		];

	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Visual Parts")))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[ListArea]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[ActionButtons]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Transform selected")))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[TransformFields]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[
			SNew(STextBlock).Text(FText::FromString(TEXT("Gizmo mode")))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(6.0f, 0.0f, 6.0f, 6.0f))
		[GizmoModeButtons]
	];
}

void FShipModuleEditorToolAssetEditor::ExtendToolbar()
{
	struct Local
	{
		static void Fill(FToolBarBuilder& ToolbarBuilder, FShipModuleEditorToolAssetEditor* Editor)
		{
			ToolbarBuilder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateRaw(Editor, &FShipModuleEditorToolAssetEditor::OnRefreshPreview)),
				NAME_None,
				FText::FromString(TEXT("Refresh Preview")),
				FText::FromString(TEXT("Пересобрать превью во вьюпорте.")));
		}
	};

	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateStatic(&Local::Fill, this));
	AddToolbarExtender(Extender);
	RegenerateMenusAndToolbars();
}

void FShipModuleEditorToolAssetEditor::OnRefreshPreview()
{
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->RebuildPreview();
		ViewportWidget->Invalidate();
	}
	RebuildPartItems();

	int32 DesiredSelection = SelectedPartIndex;
	if (ViewportWidget.IsValid())
	{
		const bool bCurrentVisible = DesiredSelection != INDEX_NONE
			&& ViewportWidget->HasSelectedPart();
		if (!bCurrentVisible)
		{
			DesiredSelection = ViewportWidget->GetFirstVisiblePartIndex();
		}
	}
	if (DesiredSelection == INDEX_NONE && PartItems.Num() > 0)
	{
		DesiredSelection = 0;
	}
	if (DesiredSelection != INDEX_NONE)
	{
		SelectPartIndex(DesiredSelection, false);
	}
}

void FShipModuleEditorToolAssetEditor::OnDetailsFinishedChanging(const FPropertyChangedEvent& PropertyChangedEvent)
{
	OnRefreshPreview();
}

UShipModuleVisualOverride* FShipModuleEditorToolAssetEditor::ResolveEditableOverride(const bool bCreateIfMissing)
{
	if (!ToolAsset)
	{
		return nullptr;
	}
	if (ToolAsset->TargetVisualOverride)
	{
		ToolAsset->TargetVisualOverride->SetFlags(RF_Transactional);
		return ToolAsset->TargetVisualOverride;
	}
	if (ToolAsset->TargetModuleDefinition)
	{
		if (const UShipModuleVisualOverride* Existing = ToolAsset->TargetModuleDefinition->GetVisualOverride())
		{
			ToolAsset->TargetVisualOverride = const_cast<UShipModuleVisualOverride*>(Existing);
			ToolAsset->TargetVisualOverride->SetFlags(RF_Transactional);
			return ToolAsset->TargetVisualOverride;
		}
	}
	if (!bCreateIfMissing)
	{
		return nullptr;
	}

	ToolAsset->GenerateOrUpdateVisualOverride();
	if (ToolAsset->TargetVisualOverride)
	{
		ToolAsset->TargetVisualOverride->SetFlags(RF_Transactional);
	}
	return ToolAsset->TargetVisualOverride;
}

void FShipModuleEditorToolAssetEditor::RebuildPartItems()
{
	PartItems.Reset();
	if (UShipModuleVisualOverride* Override = ResolveEditableOverride(false))
	{
		for (int32 Index = 0; Index < Override->VisualParts.Num(); ++Index)
		{
			PartItems.Add(MakeShared<int32>(Index));
		}
	}

	if (PartsListView.IsValid())
	{
		PartsListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FShipModuleEditorToolAssetEditor::GeneratePartRow(TSharedPtr<int32> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const int32 Index = Item.IsValid() ? *Item : INDEX_NONE;
	FString Label = FString::Printf(TEXT("Part %d"), Index);
	if (UShipModuleVisualOverride* Override = ResolveEditableOverride(false))
	{
		if (Override->VisualParts.IsValidIndex(Index))
		{
			const UStaticMesh* Mesh = Override->VisualParts[Index].Mesh.Get();
			if (Mesh)
			{
				Label = FString::Printf(TEXT("%d: %s"), Index, *Mesh->GetName());
			}
			else if (UClass* ActorClass = Override->VisualParts[Index].ActorClass.IsNull()
				? nullptr
				: Override->VisualParts[Index].ActorClass.LoadSynchronous())
			{
				Label = FString::Printf(TEXT("%d: %s (Actor)"), Index, *ActorClass->GetName());
			}
		}
	}

	return SNew(STableRow<TSharedPtr<int32>>, OwnerTable)
	[
		SNew(STextBlock).Text(FText::FromString(Label))
	];
}

void FShipModuleEditorToolAssetEditor::OnPartSelectionChanged(TSharedPtr<int32> Item, ESelectInfo::Type SelectInfo)
{
	if (bSyncingPartSelection)
	{
		return;
	}

	// During list refresh/rebuild Slate may emit transient "no item" selection events.
	// Ignore them to keep current selected part stable while editing transform fields.
	if (!Item.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] PartsList selection changed: transient null item ignored"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] PartsList selection changed: index=%d"), *Item);
	SelectPartIndex(*Item, false);
}

void FShipModuleEditorToolAssetEditor::SelectPartIndex(const int32 NewIndex, const bool bFromViewport)
{
	if (SelectedPartIndex == NewIndex && !bFromViewport)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] SelectPartIndex: unchanged index=%d fromViewport=%d"), NewIndex, bFromViewport ? 1 : 0);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] SelectPartIndex: %d -> %d (fromViewport=%d)"),
		SelectedPartIndex, NewIndex, bFromViewport ? 1 : 0);
	SelectedPartIndex = NewIndex;
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->SetSelectedPartIndex(SelectedPartIndex);
	}
	if (ViewportWidget.IsValid())
	{
		ViewportWidget->Invalidate();
	}
	if (PartsListView.IsValid())
	{
		bSyncingPartSelection = true;
		TSharedPtr<int32> Desired;
		if (SelectedPartIndex != INDEX_NONE)
		{
			for (const TSharedPtr<int32>& Item : PartItems)
			{
				if (Item.IsValid() && *Item == SelectedPartIndex)
				{
					Desired = Item;
					break;
				}
			}
		}

		if (Desired.IsValid())
		{
			const TArray<TSharedPtr<int32>> CurrentSelection = PartsListView->GetSelectedItems();
			if (CurrentSelection.Num() != 1 || !CurrentSelection[0].IsValid() || *CurrentSelection[0] != *Desired)
			{
				UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] PartsList sync: selecting row for part index %d"), *Desired);
				PartsListView->SetSelection(Desired, ESelectInfo::Direct);
				PartsListView->RequestScrollIntoView(Desired);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ShipModuleEditor] PartsList sync: clearing selection for part index %d"), SelectedPartIndex);
			PartsListView->ClearSelection();
		}
		bSyncingPartSelection = false;
	}
}

FReply FShipModuleEditorToolAssetEditor::OnAddPart()
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(true);
	if (!Override)
	{
		return FReply::Handled();
	}

	TArray<FAssetData> SelectedAssets;
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.Get().GetSelectedAssets(SelectedAssets);

	UStaticMesh* SelectedMesh = nullptr;
	UClass* SelectedActorClass = nullptr;
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (!SelectedMesh && Asset.GetClass()->IsChildOf(UStaticMesh::StaticClass()))
		{
			SelectedMesh = Cast<UStaticMesh>(Asset.GetAsset());
		}
		if (!SelectedActorClass)
		{
			SelectedActorClass = ShipModuleEditorToolAssetEditorPrivate::ResolveActorClassFromAssetData(Asset);
		}
		if (SelectedMesh || SelectedActorClass)
		{
			break;
		}
	}

	// Add only supported visual assets: static meshes or placeable actor classes.
	if (!SelectedMesh && !SelectedActorClass)
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "AddPart", "Add Ship Module Part"));
	Override->Modify();

	FShipModuleVisualPart NewPart;
	NewPart.Mesh = SelectedMesh;
	NewPart.ActorClass = SelectedActorClass;
	Override->VisualParts.Add(NewPart);
	SelectedPartIndex = Override->VisualParts.Num() - 1;
	Override->MarkPackageDirty();
	OnRefreshPreview();
	SelectPartIndex(SelectedPartIndex, false);
	return FReply::Handled();
}

FReply FShipModuleEditorToolAssetEditor::OnDuplicatePart()
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
	if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "DuplicatePart", "Duplicate Ship Module Part"));
	Override->Modify();
	Override->VisualParts.Add(Override->VisualParts[SelectedPartIndex]);
	Override->MarkPackageDirty();
	OnRefreshPreview();
	return FReply::Handled();
}

FReply FShipModuleEditorToolAssetEditor::OnRemovePart()
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
	if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "RemovePart", "Remove Ship Module Part"));
	Override->Modify();
	Override->VisualParts.RemoveAt(SelectedPartIndex);
	SelectedPartIndex = INDEX_NONE;
	Override->MarkPackageDirty();
	OnRefreshPreview();
	return FReply::Handled();
}

FReply FShipModuleEditorToolAssetEditor::OnUseSelectedAsset()
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(true);
	if (!Override)
	{
		return FReply::Handled();
	}

	TArray<FAssetData> SelectedAssets;
	FContentBrowserModule& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowser.Get().GetSelectedAssets(SelectedAssets);

	UStaticMesh* SelectedMesh = nullptr;
	UClass* SelectedActorClass = nullptr;
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (Asset.GetClass()->IsChildOf(UStaticMesh::StaticClass()))
		{
			SelectedMesh = Cast<UStaticMesh>(Asset.GetAsset());
			if (SelectedMesh)
			{
				break;
			}
		}
		SelectedActorClass = ShipModuleEditorToolAssetEditorPrivate::ResolveActorClassFromAssetData(Asset);
		if (SelectedActorClass)
		{
			break;
		}
	}
	if (!SelectedMesh && !SelectedActorClass)
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "AssignVisualAsset", "Assign Visual Asset to Ship Module Part"));
	Override->Modify();

	if (Override->VisualParts.IsValidIndex(SelectedPartIndex))
	{
		Override->VisualParts[SelectedPartIndex].Mesh = SelectedMesh;
		Override->VisualParts[SelectedPartIndex].ActorClass = SelectedActorClass;
	}
	else
	{
		FShipModuleVisualPart NewPart;
		NewPart.Mesh = SelectedMesh;
		NewPart.ActorClass = SelectedActorClass;
		Override->VisualParts.Add(NewPart);
		SelectedPartIndex = Override->VisualParts.Num() - 1;
	}

	Override->MarkPackageDirty();
	OnRefreshPreview();
	SelectPartIndex(SelectedPartIndex, false);
	return FReply::Handled();
}

FReply FShipModuleEditorToolAssetEditor::OnNudgeSelectedPart(const FVector& Delta)
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
	if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "NudgePart", "Move Ship Module Part"));
	Override->Modify();
	FTransform& T = Override->VisualParts[SelectedPartIndex].RelativeTransform;
	T.AddToTranslation(Delta);
	Override->MarkPackageDirty();
	OnRefreshPreview();
	return FReply::Handled();
}

void FShipModuleEditorToolAssetEditor::HandleViewportDuplicateSelected()
{
	OnDuplicatePart();
}

void FShipModuleEditorToolAssetEditor::HandleViewportDeleteSelected()
{
	OnRemovePart();
}

void FShipModuleEditorToolAssetEditor::HandleViewportUseSelectedAsset()
{
	OnUseSelectedAsset();
}

FReply FShipModuleEditorToolAssetEditor::OnPartsDragOver(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent)
{
	const TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (!AssetOp.IsValid())
	{
		return FReply::Unhandled();
	}

	for (const FAssetData& Asset : AssetOp->GetAssets())
	{
		if (Asset.GetClass()->IsChildOf(UStaticMesh::StaticClass())
			|| ShipModuleEditorToolAssetEditorPrivate::ResolveActorClassFromAssetData(Asset) != nullptr)
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply FShipModuleEditorToolAssetEditor::OnPartsDrop(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent)
{
	const TSharedPtr<FAssetDragDropOp> AssetOp = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
	if (!AssetOp.IsValid())
	{
		return FReply::Unhandled();
	}

	UShipModuleVisualOverride* Override = ResolveEditableOverride(true);
	if (!Override)
	{
		return FReply::Unhandled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "DropVisualAssets", "Drop Visual Assets to Ship Module Parts"));
	Override->Modify();

	bool bApplied = false;
	bool bUsedSelectionSlot = false;
	for (const FAssetData& Asset : AssetOp->GetAssets())
	{
		UStaticMesh* Mesh = nullptr;
		UClass* ActorClass = nullptr;
		if (Asset.GetClass()->IsChildOf(UStaticMesh::StaticClass()))
		{
			Mesh = Cast<UStaticMesh>(Asset.GetAsset());
		}
		else
		{
			ActorClass = ShipModuleEditorToolAssetEditorPrivate::ResolveActorClassFromAssetData(Asset);
		}
		if (!Mesh && !ActorClass)
		{
			continue;
		}

		if (!bUsedSelectionSlot && Override->VisualParts.IsValidIndex(SelectedPartIndex))
		{
			Override->VisualParts[SelectedPartIndex].Mesh = Mesh;
			Override->VisualParts[SelectedPartIndex].ActorClass = ActorClass;
			bUsedSelectionSlot = true;
		}
		else
		{
			FShipModuleVisualPart NewPart;
			NewPart.Mesh = Mesh;
			NewPart.ActorClass = ActorClass;
			Override->VisualParts.Add(NewPart);
			SelectedPartIndex = Override->VisualParts.Num() - 1;
		}
		bApplied = true;
	}

	if (!bApplied)
	{
		return FReply::Unhandled();
	}

	Override->MarkPackageDirty();
	OnRefreshPreview();
	SelectPartIndex(SelectedPartIndex, false);
	return FReply::Handled();
}

FReply FShipModuleEditorToolAssetEditor::OnRotateSelectedPart(const float DeltaYaw)
{
	UShipModuleVisualOverride* Override = ResolveEditableOverride(false);
	if (!Override || !Override->VisualParts.IsValidIndex(SelectedPartIndex))
	{
		return FReply::Handled();
	}
	const FScopedTransaction Transaction(NSLOCTEXT("ShipModuleEditor", "RotatePart", "Rotate Ship Module Part"));
	Override->Modify();
	FTransform& T = Override->VisualParts[SelectedPartIndex].RelativeTransform;
	FRotator R = T.GetRotation().Rotator();
	R.Yaw += DeltaYaw;
	T.SetRotation(R.Quaternion());
	Override->MarkPackageDirty();
	OnRefreshPreview();
	return FReply::Handled();
}

#endif // WITH_EDITOR

