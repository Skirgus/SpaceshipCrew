#include "SSpaceshipShipBuilderRoot.h"

#include "SpaceshipShipBuilderPlayerController.h"
#include "ShipBuilderParameterEvaluator.h"
#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "ShipModuleTypes.h"

#define LOCTEXT_NAMESPACE "SpaceshipShipBuilderRoot"

void SSpaceshipShipBuilderRoot::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.02f, 0.03f, 0.06f, 0.92f))
		.Padding(FMargin(24.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 12.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
				.ColorAndOpacity(FLinearColor(0.2f, 0.85f, 0.95f))
				.Text(LOCTEXT("Title", "КОНСТРУКТОР КОРАБЛЯ"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.ColorAndOpacity(FLinearColor(0.75f, 0.78f, 0.82f))
				.Text_Lambda([this]()
				{
					if (!OwnerPC.IsValid())
					{
						return FText::GetEmpty();
					}
					const int32 N = OwnerPC->AccessDraft().ModuleIds.Num();
					return FText::Format(
						LOCTEXT("DraftLine", "Модулей в черновике: {0} (стыковка в сцене — отдельная задача)"),
						FText::AsNumber(N));
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 16.0f, 0.0f, 8.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FLinearColor(0.35f, 0.9f, 1.0f))
				.Text(LOCTEXT("ReactorHdr", "РЕАКТОР / ПИТАНИЕ (УСЛ.)"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
				.ColorAndOpacity(FLinearColor::White)
				.Text_Lambda([this]()
				{
					if (!OwnerPC.IsValid())
					{
						return FText::GetEmpty();
					}
					UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
					if (!Cat)
					{
						return LOCTEXT("NoCat", "Каталог недоступен.");
					}
					FShipBuilderParameterSnapshot Snap;
					FShipBuilderParameterEvaluator::ComputeSnapshot(OwnerPC->AccessDraft(), *Cat, Snap);
					double Reactor = 0.0;
					double Power = 0.0;
					for (const FShipBuilderParameterEntry& E : Snap.Entries)
					{
						if (E.Id == ShipBuilderParameterIds::ReactorOutput)
						{
							Reactor = E.Value;
						}
						if (E.Id == ShipBuilderParameterIds::PowerDemand)
						{
							Power = E.Value;
						}
					}
					return FText::Format(
						LOCTEXT("ReactorFmt", "Реактор: {0}   Питание: {1}"),
						FText::AsNumber(FMath::RoundToInt(Reactor)),
						FText::AsNumber(FMath::RoundToInt(Power)));
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 20.0f, 0.0f, 8.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FLinearColor(0.35f, 0.9f, 1.0f))
				.Text(LOCTEXT("StatsHdr", "ПАРАМЕТРЫ КОРАБЛЯ"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
				.WrapTextAt(1200.0f)
				.ColorAndOpacity(FLinearColor::White)
				.Text_Lambda([this]()
				{
					if (!OwnerPC.IsValid())
					{
						return FText::GetEmpty();
					}
					UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
					if (!Cat)
					{
						return FText::GetEmpty();
					}
					FShipBuilderParameterSnapshot Base;
					FShipBuilderParameterEvaluator::ComputeSnapshot(OwnerPC->AccessDraft(), *Cat, Base);
					const FName Hover = OwnerPC->GetHoveredCatalogModule();
					FShipBuilderParameterSnapshot Preview;
					if (!Hover.IsNone())
					{
						FShipBuilderParameterEvaluator::ComputeSnapshotWithPreviewAppend(
							OwnerPC->AccessDraft(),
							Hover,
							*Cat,
							Preview);
					}
					FString Line;
					for (const FShipBuilderParameterEntry& E : Base.Entries)
					{
						Line += E.Label.ToString();
						Line += TEXT(": ");
						Line += FString::Printf(TEXT("%.0f"), E.Value);
						if (!Hover.IsNone())
						{
							const FShipBuilderParameterEntry* P = Preview.Entries.FindByPredicate(
								[&E](const FShipBuilderParameterEntry& X)
								{
									return X.Id == E.Id;
								});
							if (P)
							{
								const double Delta = P->Value - E.Value;
								if (!FMath::IsNearlyZero(Delta))
								{
									const bool bBetter = (Delta > 0.0 && E.bHigherIsBetter)
										|| (Delta < 0.0 && !E.bHigherIsBetter);
									Line += bBetter ? TEXT(" (▲") : TEXT(" (▼");
									Line += FString::Printf(TEXT("%+.0f)"), Delta);
								}
							}
						}
						Line += TEXT("   ");
					}
					return FText::FromString(Line);
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 16.0f, 0.0f, 8.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.ColorAndOpacity(FLinearColor(0.65f, 0.7f, 0.75f))
				.Text_Lambda([this]()
				{
					if (!OwnerPC.IsValid())
					{
						return FText::GetEmpty();
					}
					const FShipBuildValidationResult V = OwnerPC->ComputeValidation();
					return FText::Format(
						LOCTEXT("ValFmt", "Валидация T02b: {0} | ошибок: {1} | предупреждений: {2}"),
						V.bIsValid ? LOCTEXT("Ok", "OK") : LOCTEXT("Bad", "ЕСТЬ ОШИБКИ"),
						FText::AsNumber(V.Errors.Num()),
						FText::AsNumber(V.Warnings.Num()));
				})
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(FMargin(0.0f, 12.0f, 0.0f, 12.0f))
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.95f))
				.Padding(FMargin(12.0f))
				.Visibility_Lambda([this]()
				{
					return OwnerPC.IsValid() && OwnerPC->IsChecklistOpen()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(FLinearColor(1.0f, 0.45f, 0.35f))
						.Text(LOCTEXT("ErrHdr", "ОШИБКИ"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 6.0f, 0.0f, 12.0f))
					[
						SNew(STextBlock)
						.WrapTextAt(700.0f)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor::White)
						.Text_Lambda([this]()
						{
							if (!OwnerPC.IsValid())
							{
								return FText::GetEmpty();
							}
							const TArray<FString> E = OwnerPC->ComputeValidation().Errors;
							return FText::FromString(FString::Join(E, TEXT("\n")));
						})
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.35f))
						.Text(LOCTEXT("WarnHdr", "ПРЕДУПРЕЖДЕНИЯ"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
					[
						SNew(STextBlock)
						.WrapTextAt(700.0f)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FLinearColor::White)
						.Text_Lambda([this]()
						{
							if (!OwnerPC.IsValid())
							{
								return FText::GetEmpty();
							}
							const TArray<FString> W = OwnerPC->ComputeValidation().Warnings;
							return FText::FromString(FString::Join(W, TEXT("\n")));
						})
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.06f, 0.09f, 0.95f))
				.Padding(FMargin(12.0f))
				.Visibility_Lambda([this]()
				{
					return OwnerPC.IsValid() && OwnerPC->IsCatalogOpen()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(FLinearColor(0.35f, 0.9f, 1.0f))
						.Text(LOCTEXT("CatHdr", "КАТАЛОГ МОДУЛЕЙ (наведите — предпросмотр дельт, клик — в черновик)"))
					]
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(CatalogList, SVerticalBox)
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.ColorAndOpacity(FLinearColor(0.55f, 0.6f, 0.65f))
				.Text(LOCTEXT("Hints", "[G] каталог   [C] чеклист   [Tab] главное меню"))
			]
		]
	];
	RebuildCatalogList();
}

void SSpaceshipShipBuilderRoot::RequestRefresh()
{
	RebuildCatalogList();
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SSpaceshipShipBuilderRoot::RebuildCatalogList()
{
	if (!CatalogList.IsValid() || !OwnerPC.IsValid())
	{
		return;
	}
	CatalogList->ClearChildren();
	UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
	if (!Cat)
	{
		return;
	}
	const TArray<UShipModuleDefinition*> Modules = Cat->GetAllModules();
	TMap<EShipModuleType, TArray<UShipModuleDefinition*>> ByType;
	for (UShipModuleDefinition* Def : Modules)
	{
		if (Def)
		{
			ByType.FindOrAdd(Def->ModuleType).Add(Def);
		}
	}
	ByType.KeySort([](const EShipModuleType A, const EShipModuleType B)
	{
		return static_cast<uint8>(A) < static_cast<uint8>(B);
	});
	for (const TPair<EShipModuleType, TArray<UShipModuleDefinition*>>& Pair : ByType)
	{
		CatalogList->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 8.0f, 0.0f, 4.0f))
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.ColorAndOpacity(FLinearColor(0.45f, 0.95f, 1.0f))
			.Text(FText::Format(
				LOCTEXT("TypeHdr", "[{0}]"),
				StaticEnum<EShipModuleType>()
					? StaticEnum<EShipModuleType>()->GetDisplayNameTextByValue(static_cast<int64>(Pair.Key))
					: FText::GetEmpty()))
		];
		for (UShipModuleDefinition* Def : Pair.Value)
		{
			if (!Def)
			{
				continue;
			}
			const FName Mid = Def->ModuleId;
			CatalogList->AddSlot()
			.AutoHeight()
			.Padding(FMargin(8.0f, 2.0f, 0.0f, 2.0f))
			[
				SNew(SButton)
				.OnClicked_Lambda([this, Mid]()
				{
					if (OwnerPC.IsValid())
					{
						OwnerPC->AppendModuleToDraft(Mid);
						OwnerPC->SetHoveredCatalogModule(NAME_None);
						OwnerPC->SetCatalogOpen(false);
						OwnerPC->RefreshShipBuilderUi();
					}
					return FReply::Handled();
				})
				.OnHovered_Lambda([this, Mid]()
				{
					if (OwnerPC.IsValid())
					{
						OwnerPC->SetHoveredCatalogModule(Mid);
						OwnerPC->RefreshShipBuilderUi();
					}
				})
				.OnUnhovered_Lambda([this]()
				{
					if (OwnerPC.IsValid())
					{
						OwnerPC->SetHoveredCatalogModule(NAME_None);
						OwnerPC->RefreshShipBuilderUi();
					}
				})
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					.Text(FText::Format(
						LOCTEXT("ModLine", "{0} — {1} кг"),
						Def->DisplayName,
						FText::AsNumber(FMath::RoundToInt(Def->Mass))))
				]
			];
		}
	}
}

#undef LOCTEXT_NAMESPACE
