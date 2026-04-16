#include "SSpaceshipShipBuilderRoot.h"

#include "SpaceshipShipBuilderPlayerController.h"
#include "ShipBuilderParameterEvaluator.h"
#include "ShipModuleCatalog.h"
#include "ShipModuleDefinition.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Types/SlateEnums.h"
#include "Styling/CoreStyle.h"
#include "ShipModuleTypes.h"

#define LOCTEXT_NAMESPACE "SpaceshipShipBuilderRoot"

namespace SpaceshipShipBuilderUiPrivate
{
	static constexpr int32 GaugeSegments = 10;

	static FSlateFontInfo CapsFont(const int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	static FSlateFontInfo BodyFont(const int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	static FLinearColor PanelBg()
	{
		return FLinearColor(0.035f, 0.055f, 0.1f, 0.78f);
	}

	static FLinearColor PanelBgStrong()
	{
		return FLinearColor(0.025f, 0.042f, 0.085f, 0.86f);
	}

	static const FSlateBrush* PanelTintBrush()
	{
		return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	}

	static FLinearColor TextHi()
	{
		return FLinearColor(0.94f, 0.96f, 1.0f, 1.0f);
	}

	static FLinearColor TextMutedHi()
	{
		return FLinearColor(0.74f, 0.78f, 0.85f, 1.0f);
	}

	/** Вертикальная колонка «сегментов» снизу вверх (как на референсе). */
	static TSharedRef<SWidget> MakeVerticalGaugeColumn(const float Fill01)
	{
		const int32 Lit = FMath::Clamp(FMath::RoundToInt(Fill01 * static_cast<float>(GaugeSegments)), 0, GaugeSegments);
		TSharedRef<SScrollBox> Col = SNew(SScrollBox);
		for (int32 Seg = 0; Seg < GaugeSegments; ++Seg)
		{
			const int32 FromBottom = GaugeSegments - 1 - Seg;
			const bool bOn = FromBottom < Lit;
			Col->AddSlot()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 2.0f))
			[
				SNew(SBox)
				.HeightOverride(5.0f)
				.WidthOverride(22.0f)
				[
					SNew(SBorder)
					.BorderImage(PanelTintBrush())
					.BorderBackgroundColor(bOn
						? FLinearColor(0.88f, 0.9f, 0.95f, 1.0f)
						: FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
				]
			];
		}
		return Col;
	}

	static TSharedRef<SWidget> MakeStatColumn(
		const FText& Label,
		const FText& Value,
		const FLinearColor& ValueColor)
	{
		return SNew(SBorder)
			.Padding(FMargin(10.0f, 6.0f, 10.0f, 6.0f))
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Font(BodyFont(9))
					.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.88f))
					.Justification(ETextJustify::Center)
					.Text(Label)
				]
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Font(CapsFont(14))
					.ColorAndOpacity(ValueColor)
					.Justification(ETextJustify::Center)
					.Text(Value)
				]
			];
	}

	static TSharedRef<SWidget> MakeThinSeparator()
	{
		return SNew(SBox)
			.WidthOverride(1.0f)
			.HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.35f, 0.4f, 0.48f, 0.98f))
			];
	}

	static bool TryGetSnapshotValue(
		const FShipBuilderParameterSnapshot& Snap,
		const FName Id,
		double& OutValue,
		bool& bHigherBetter)
	{
		for (const FShipBuilderParameterEntry& E : Snap.Entries)
		{
			if (E.Id == Id)
			{
				OutValue = E.Value;
				bHigherBetter = E.bHigherIsBetter;
				return true;
			}
		}
		return false;
	}

	static FText TypeDisplayName(const EShipModuleType T)
	{
		if (UEnum* E = StaticEnum<EShipModuleType>())
		{
			return E->GetDisplayNameTextByValue(static_cast<int64>(T));
		}
		return FText::GetEmpty();
	}

	static TSharedRef<SWidget> MakeKeyHint(const TCHAR* KeyLabel)
	{
		return SNew(SBox)
			.WidthOverride(26.0f)
			.HeightOverride(26.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(PanelTintBrush())
				.Padding(FMargin(0.0f))
				.BorderBackgroundColor(FLinearColor(0.14f, 0.16f, 0.22f, 1.0f))
				[
					SNew(STextBlock)
					.Font(CapsFont(11))
					.ColorAndOpacity(TextHi())
					.Justification(ETextJustify::Center)
					.Text(FText::FromString(KeyLabel))
				]
			];
	}
}

SSpaceshipShipBuilderRoot::~SSpaceshipShipBuilderRoot() = default;

FText SSpaceshipShipBuilderRoot::GetModuleInspectTitleText() const
{
	if (!OwnerPC.IsValid())
	{
		return FText::GetEmpty();
	}
	const FName H = OwnerPC->GetHoveredCatalogModule();
	if (H.IsNone())
	{
		return LOCTEXT("ModInspectTitleIdle", "Модуль");
	}
	if (UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog())
	{
		if (UShipModuleDefinition* Def = Cat->FindModuleById(H))
		{
			return Def->DisplayName.IsEmpty() ? FText::FromName(H) : Def->DisplayName;
		}
	}
	return LOCTEXT("ModInspectTitleUnknown", "Модуль");
}

FText SSpaceshipShipBuilderRoot::GetModuleInspectBodyText() const
{
	if (!OwnerPC.IsValid())
	{
		return LOCTEXT("ModInspectHint", "Откройте каталог (G) и наведите курсор на строку модуля.");
	}
	const FName H = OwnerPC->GetHoveredCatalogModule();
	if (H.IsNone())
	{
		return LOCTEXT("ModInspectHint", "Откройте каталог (G) и наведите курсор на строку модуля.");
	}
	UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
	if (!Cat)
	{
		return FText::GetEmpty();
	}
	UShipModuleDefinition* Def = Cat->FindModuleById(H);
	if (!Def)
	{
		return FText::GetEmpty();
	}
	const int32 C = Def->GetEffectiveCreditCost();
	return FText::Format(
		LOCTEXT("ModInspectFmt", "{0}\n\n{1} кг  ·  {2} кр."),
		Def->Description.IsEmpty() ? FText::FromString(TEXT("—")) : Def->Description,
		FText::AsNumber(FMath::RoundToInt(Def->Mass)),
		FText::AsNumber(C));
}

void SSpaceshipShipBuilderRoot::Construct(const FArguments& InArgs)
{
	OwnerPC = InArgs._OwnerPC;

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(32.0f, 0.0f, 32.0f, 20.0f))
		[
			SNew(SBorder)
			.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
			.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBg())
			.Padding(FMargin(14.0f, 12.0f, 14.0f, 12.0f))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
				[
					SNew(SBox)
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Font(SpaceshipShipBuilderUiPrivate::BodyFont(10))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
						.Justification(ETextJustify::Right)
						.Text(LOCTEXT(
							"HintsRow",
							"[CTRL] НАСТРОЙКИ   [M3] КАМЕРА   [G] ДОБАВИТЬ   [C] ЧЕК-ЛИСТ   [TAB] ВЫХОД"))
					]
				]
			+ SScrollBox::Slot()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
			[
				SNew(SBox)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::CapsFont(14))
					.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
					.Justification(ETextJustify::Right)
					.Text_Lambda([this]()
					{
						if (!OwnerPC.IsValid())
						{
							return FText::GetEmpty();
						}
						const FShipBuildValidationResult V = OwnerPC->ComputeValidation();
						if (V.bIsValid && V.Errors.Num() == 0 && V.Warnings.Num() == 0)
						{
							return LOCTEXT("Nominal", "ВСЕ СИСТЕМЫ В НОРМЕ");
						}
						return LOCTEXT("NotNominal", "ТРЕБУЕТСЯ ВНИМАНИЕ");
					})
				]
			]
			+ SScrollBox::Slot()
			.Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(8.0f, 8.0f, 8.0f, 8.0f))
				.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBgStrong())
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::BodyFont(10))
					.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
					.Text_Lambda([this]()
					{
						if (!OwnerPC.IsValid())
						{
							return FText::GetEmpty();
						}
						const FShipBuildValidationResult V = OwnerPC->ComputeValidation();
						return FText::Format(
							LOCTEXT("ValTiny", "T02b: {0}  ·  ошибок: {1}  ·  предупреждений: {2}"),
							V.bIsValid ? LOCTEXT("Ok2", "OK") : LOCTEXT("Err2", "ОШИБКИ"),
							FText::AsNumber(V.Errors.Num()),
							FText::AsNumber(V.Warnings.Num()));
					})
				]
			]
			+ SScrollBox::Slot()
			.Padding(FMargin(4.0f, 2.0f, 4.0f, 0.0f))
			[
				BuildFooterStatsRow()
			]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(28.0f, 28.0f, 0.0f, 0.0f))
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(16.0f, 14.0f, 16.0f, 14.0f))
				.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBg())
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 10.0f))
					[
						SNew(SScrollBox)
						.Orientation(EOrientation::Orient_Horizontal)
						+ SScrollBox::Slot()
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.55f)]
						+ SScrollBox::Slot()
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.42f)]
						+ SScrollBox::Slot()
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.38f)]
						+ SScrollBox::Slot()
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.62f)]
						+ SScrollBox::Slot()
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.48f)]
						+ SScrollBox::Slot()
						.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
						[SpaceshipShipBuilderUiPrivate::MakeVerticalGaugeColumn(0.51f)]
					]
					+ SScrollBox::Slot()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
					[
						SNew(STextBlock)
						.Font(SpaceshipShipBuilderUiPrivate::BodyFont(8))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
						.Text(FText::FromString(TEXT("В0    В1    В2    ДВГ    ЩИТ    ГРВ")))
					]
					+ SScrollBox::Slot()
					.Padding(FMargin(0.0f, 0.0f, 0.0f, 10.0f))
					[
						SNew(SBox)
						.HeightOverride(6.0f)
						[
							SNew(SBorder)
							.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
							.BorderBackgroundColor(FLinearColor(0.16f, 0.72f, 0.82f, 0.88f))
						]
					]
					+ SScrollBox::Slot()
					[
						SNew(STextBlock)
						.Font(SpaceshipShipBuilderUiPrivate::CapsFont(13))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
						.Text_Lambda([this]()
						{
							if (!OwnerPC.IsValid())
							{
								return FText::GetEmpty();
							}
							UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
							if (!Cat)
							{
								return LOCTEXT("NoCatShort", "Каталог —");
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
							const FString ClassLetter = Reactor > 0.5 ? TEXT("A") : TEXT("—");
							return FText::Format(
								LOCTEXT("ReactorRow", "{0}  РЕАКТОР {1}     ПИТАНИЕ {2}"),
								FText::FromString(ClassLetter),
								FText::AsNumber(FMath::RoundToInt(Reactor)),
								FText::AsNumber(FMath::RoundToInt(Power)));
						})
					]
					+ SScrollBox::Slot()
					.Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
					[
						SNew(STextBlock)
						.Font(SpaceshipShipBuilderUiPrivate::BodyFont(10))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
						.Text_Lambda([this]()
						{
							if (!OwnerPC.IsValid())
							{
								return FText::GetEmpty();
							}
							return FText::Format(
								LOCTEXT("DraftTiny", "Черновик: {0} мод."),
								FText::AsNumber(OwnerPC->AccessDraft().ModuleIds.Num()));
						})
					]
				]
			]
			+ SScrollBox::Slot()
			.Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(14.0f, 12.0f, 14.0f, 12.0f))
				.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBgStrong())
				.Visibility_Lambda([this]()
				{
					return OwnerPC.IsValid() && !OwnerPC->GetHoveredCatalogModule().IsNone()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SNew(STextBlock)
						.Font(SpaceshipShipBuilderUiPrivate::CapsFont(12))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
						.Text_Lambda([this]()
						{
							return GetModuleInspectTitleText();
						})
					]
					+ SScrollBox::Slot()
					.Padding(FMargin(0.0f, 8.0f, 0.0f, 0.0f))
					[
						SNew(STextBlock)
						.WrapTextAt(300.0f)
						.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
						.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
						.Text_Lambda([this]()
						{
							return GetModuleInspectBodyText();
						})
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(FMargin(0.0f, 28.0f, 28.0f, 0.0f))
		[
			SNew(SBorder)
			.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
			.Padding(FMargin(14.0f, 10.0f, 14.0f, 10.0f))
			.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBgStrong())
			[
				SNew(STextBlock)
				.Font(SpaceshipShipBuilderUiPrivate::CapsFont(12))
				.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
				.Justification(ETextJustify::Right)
				.Text_Lambda([this]()
				{
					if (!OwnerPC.IsValid())
					{
						return FText::GetEmpty();
					}
					return FText::Format(
						LOCTEXT("ShipCostLine", "СТОИМОСТЬ: {0}"),
						FText::AsNumber(OwnerPC->GetDraftTotalCreditCost()));
				})
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 80.0f, 120.0f, 120.0f))
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(14.0f))
				.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBgStrong())
				.Visibility_Lambda([this]()
				{
					return OwnerPC.IsValid() && OwnerPC->IsChecklistOpen()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SBox)
					.WidthOverride(420.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(STextBlock)
							.Font(SpaceshipShipBuilderUiPrivate::CapsFont(14))
							.ColorAndOpacity(FLinearColor(1.0f, 0.45f, 0.35f))
							.Text(LOCTEXT("ErrHdr", "ОШИБКИ"))
						]
						+ SScrollBox::Slot()
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 12.0f))
						[
							SNew(STextBlock)
							.WrapTextAt(400.0f)
							.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
							.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
							.Text_Lambda([this]()
							{
								if (!OwnerPC.IsValid())
								{
									return FText::GetEmpty();
								}
								return FText::FromString(FString::Join(OwnerPC->ComputeValidation().Errors, TEXT("\n")));
							})
						]
						+ SScrollBox::Slot()
						[
							SNew(STextBlock)
							.Font(SpaceshipShipBuilderUiPrivate::CapsFont(14))
							.ColorAndOpacity(FLinearColor(1.0f, 0.85f, 0.35f))
							.Text(LOCTEXT("WarnHdr", "ПРЕДУПРЕЖДЕНИЯ"))
						]
						+ SScrollBox::Slot()
						.Padding(FMargin(0.0f, 6.0f, 0.0f, 0.0f))
						[
							SNew(STextBlock)
							.WrapTextAt(400.0f)
							.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
							.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
							.Text_Lambda([this]()
							{
								if (!OwnerPC.IsValid())
								{
									return FText::GetEmpty();
								}
								return FText::FromString(FString::Join(OwnerPC->ComputeValidation().Warnings, TEXT("\n")));
							})
						]
					]
				]
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Fill)
		.Padding(FMargin(0.0f, 96.0f, 40.0f, 200.0f))
		[
			SNew(SBox)
			.HAlign(HAlign_Right)
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(12.0f))
				.BorderBackgroundColor(SpaceshipShipBuilderUiPrivate::PanelBg())
				.Visibility_Lambda([this]()
				{
					return OwnerPC.IsValid() && OwnerPC->IsCatalogOpen()
						? EVisibility::Visible
						: EVisibility::Collapsed;
				})
				[
					SNew(SBox)
					.WidthOverride(380.0f)
					[
						SAssignNew(CatalogPanelRoot, SVerticalBox)
					]
				]
			]
		]
	];
	RebuildCatalogPanel();
}

TSharedRef<SWidget> SSpaceshipShipBuilderRoot::BuildFooterStatsRow() const
{
	const auto DynCol = [this](const FName Id, const FText& Label) -> TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.Padding(FMargin(10.0f, 6.0f, 10.0f, 6.0f))
			.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::BodyFont(9))
					.ColorAndOpacity(FLinearColor(0.78f, 0.82f, 0.88f))
					.Justification(ETextJustify::Center)
					.Text(Label)
				]
				+ SScrollBox::Slot()
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::CapsFont(14))
					.ColorAndOpacity_Lambda([this, Id]()
					{
						return GetFooterValueColor(Id);
					})
					.Justification(ETextJustify::Center)
					.Text_Lambda([this, Id]()
					{
						return GetFooterValueText(Id, LOCTEXT("PlaceholderDash", "—"));
					})
				]
			];
	};

	return SNew(SScrollBox)
		.Orientation(EOrientation::Orient_Horizontal)
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_LAS", "ЛАЗ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_BAL", "БАЛ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_RKT", "РКТ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[DynCol(ShipBuilderParameterIds::Hull, LOCTEXT("L_HULL", "КОРПУС"))]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_SH", "ЩИТ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_CARGO", "ГРУЗ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_CREW", "МАКС. ЭКИПАЖ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_JUMP", "Д. ПРЫЖКА"),
				LOCTEXT("PlaceholderDashLy", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[DynCol(ShipBuilderParameterIds::Mobility, LOCTEXT("L_MOB", "МОБИЛЬНОСТЬ"))]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[
			SpaceshipShipBuilderUiPrivate::MakeStatColumn(
				LOCTEXT("L_SPD", "МАКС. СКОРОСТЬ"),
				LOCTEXT("PlaceholderDash", "—"),
				FLinearColor::White)
		]
		+ SScrollBox::Slot()
		[SpaceshipShipBuilderUiPrivate::MakeThinSeparator()]
		+ SScrollBox::Slot()
		[DynCol(ShipBuilderParameterIds::MassKg, LOCTEXT("L_MASS", "ВЕС"))];
}

FText SSpaceshipShipBuilderRoot::GetFooterValueText(const FName ParameterId, const FText& Placeholder) const
{
	if (!OwnerPC.IsValid())
	{
		return Placeholder;
	}
	UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
	if (!Cat)
	{
		return Placeholder;
	}
	FShipBuilderParameterSnapshot Base;
	FShipBuilderParameterEvaluator::ComputeSnapshot(OwnerPC->AccessDraft(), *Cat, Base);
	double Bv = 0.0;
	bool Hb = true;
	if (!SpaceshipShipBuilderUiPrivate::TryGetSnapshotValue(Base, ParameterId, Bv, Hb))
	{
		return Placeholder;
	}
	const FName Hover = OwnerPC->GetHoveredCatalogModule();
	if (Hover.IsNone())
	{
		return FText::AsNumber(FMath::RoundToInt(Bv));
	}
	FShipBuilderParameterSnapshot Preview;
	FShipBuilderParameterEvaluator::ComputeSnapshotWithPreviewAppend(
		OwnerPC->AccessDraft(),
		Hover,
		*Cat,
		Preview);
	double Pv = 0.0;
	bool Hb2 = true;
	if (!SpaceshipShipBuilderUiPrivate::TryGetSnapshotValue(Preview, ParameterId, Pv, Hb2))
	{
		return FText::AsNumber(FMath::RoundToInt(Bv));
	}
	const int32 Bi = FMath::RoundToInt(Bv);
	const int32 Pi = FMath::RoundToInt(Pv);
	const int32 D = Pi - Bi;
	if (D == 0)
	{
		return FText::AsNumber(Bi);
	}
	return FText::Format(
		LOCTEXT("FooterDelta", "{0} ({1}{2})"),
		FText::AsNumber(Bi),
		D > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(),
		FText::AsNumber(D));
}

FSlateColor SSpaceshipShipBuilderRoot::GetFooterValueColor(const FName ParameterId) const
{
	if (!OwnerPC.IsValid())
	{
		return FSlateColor(FLinearColor::White);
	}
	UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
	if (!Cat)
	{
		return FSlateColor(FLinearColor::White);
	}
	const FName Hover = OwnerPC->GetHoveredCatalogModule();
	if (Hover.IsNone())
	{
		return FSlateColor(FLinearColor::White);
	}
	FShipBuilderParameterSnapshot Base;
	FShipBuilderParameterSnapshot Preview;
	FShipBuilderParameterEvaluator::ComputeSnapshot(OwnerPC->AccessDraft(), *Cat, Base);
	FShipBuilderParameterEvaluator::ComputeSnapshotWithPreviewAppend(
		OwnerPC->AccessDraft(),
		Hover,
		*Cat,
		Preview);
	double Bv = 0.0;
	double Pv = 0.0;
	bool Hb = true;
	bool Hb2 = true;
	if (!SpaceshipShipBuilderUiPrivate::TryGetSnapshotValue(Base, ParameterId, Bv, Hb)
		|| !SpaceshipShipBuilderUiPrivate::TryGetSnapshotValue(Preview, ParameterId, Pv, Hb2))
	{
		return FSlateColor(FLinearColor::White);
	}
	const double Delta = Pv - Bv;
	if (FMath::IsNearlyZero(Delta))
	{
		return FSlateColor(FLinearColor::White);
	}
	const bool bBetter = (Delta > 0.0 && Hb) || (Delta < 0.0 && !Hb);
	return FSlateColor(bBetter ? FLinearColor(0.35f, 0.78f, 1.0f) : FLinearColor(1.0f, 0.38f, 0.4f));
}

void SSpaceshipShipBuilderRoot::RequestRefresh()
{
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SSpaceshipShipBuilderRoot::RebuildCatalogPanel()
{
	RebuildCatalogList();
}

void SSpaceshipShipBuilderRoot::RebuildCatalogList()
{
	if (!CatalogPanelRoot.IsValid() || !OwnerPC.IsValid())
	{
		return;
	}
	CatalogPanelRoot->ClearChildren();
	CatalogList.Reset();

	UShipModuleCatalog* Cat = OwnerPC->GetModuleCatalog();
	if (!Cat)
	{
		CatalogPanelRoot->AddSlot().AutoHeight()
		[
			SNew(STextBlock)
			.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
			.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
			.Text(LOCTEXT("CatNoCatalog", "Каталог недоступен."))
		];
		return;
	}

	const TArray<EShipModuleType> Types = OwnerPC->GetCatalogModuleTypesSorted();
	const int32 TotalTypes = Types.Num();
	int32 Idx = 0;
	if (TotalTypes > 0)
	{
		Idx = FMath::Clamp(OwnerPC->GetCatalogCategoryIndex(), 0, TotalTypes - 1);
	}

	CatalogPanelRoot->AddSlot().AutoHeight().Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
	[
		SNew(STextBlock)
		.Font(SpaceshipShipBuilderUiPrivate::CapsFont(12))
		.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
		.Text(LOCTEXT("CatHdr", "КАТАЛОГ (G) · Q/T — тип · наведите / клик"))
	];

	EShipModuleType PrevType = EShipModuleType::Bridge;
	EShipModuleType CurrType = EShipModuleType::Bridge;
	EShipModuleType NextType = EShipModuleType::Bridge;
	if (TotalTypes > 0)
	{
		PrevType = Types[(Idx - 1 + TotalTypes) % TotalTypes];
		CurrType = Types[Idx];
		NextType = Types[(Idx + 1) % TotalTypes];
	}

	const FText PrevName = TotalTypes > 0 ? SpaceshipShipBuilderUiPrivate::TypeDisplayName(PrevType) : LOCTEXT("CatTypeNone", "—");
	const FText CurrName = TotalTypes > 0 ? SpaceshipShipBuilderUiPrivate::TypeDisplayName(CurrType) : LOCTEXT("CatTypeNone", "—");
	const FText NextName = TotalTypes > 0 ? SpaceshipShipBuilderUiPrivate::TypeDisplayName(NextType) : LOCTEXT("CatTypeNone", "—");

	CatalogPanelRoot->AddSlot().AutoHeight()
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
		[
			SpaceshipShipBuilderUiPrivate::MakeKeyHint(TEXT("Q"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Font(SpaceshipShipBuilderUiPrivate::CapsFont(10))
				.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
				.Justification(ETextJustify::Center)
				.WrapTextAt(100.0f)
				.Text(PrevName)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.2f)
			.Padding(FMargin(4.0f, 0.0f, 4.0f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
				.Padding(FMargin(8.0f, 6.0f, 8.0f, 6.0f))
				.BorderBackgroundColor(FLinearColor(0.22f, 0.24f, 0.3f, 1.0f))
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::CapsFont(11))
					.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
					.Justification(ETextJustify::Center)
					.WrapTextAt(120.0f)
					.Text(CurrName)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(STextBlock)
				.Font(SpaceshipShipBuilderUiPrivate::CapsFont(10))
				.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
				.Justification(ETextJustify::Center)
				.WrapTextAt(100.0f)
				.Text(NextName)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(FMargin(8.0f, 0.0f, 0.0f, 0.0f))
		[
			SpaceshipShipBuilderUiPrivate::MakeKeyHint(TEXT("T"))
		]
	];

	// Сегменты: по одному на каждый тип в каталоге (как полоска прогресса на референсе)
	{
		TSharedRef<SHorizontalBox> SegRow = SNew(SHorizontalBox);
		if (TotalTypes <= 0)
		{
			SegRow->AddSlot()
			[
				SNew(SBox)
				.HeightOverride(5.0f)
				[
					SNew(SBorder)
					.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
					.BorderBackgroundColor(FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
				]
			];
		}
		else
		{
			for (int32 Si = 0; Si < TotalTypes; ++Si)
			{
				const bool bLit = Si == Idx;
				SegRow->AddSlot()
				.FillWidth(1.0f)
				.Padding(FMargin(1.5f, 0.0f, 1.5f, 0.0f))
				[
					SNew(SBox)
					.HeightOverride(5.0f)
					[
						SNew(SBorder)
						.BorderImage(SpaceshipShipBuilderUiPrivate::PanelTintBrush())
						.BorderBackgroundColor(bLit
							? FLinearColor(0.92f, 0.94f, 0.98f, 1.0f)
							: FLinearColor(0.12f, 0.14f, 0.18f, 1.0f))
					]
				];
			}
		}
		CatalogPanelRoot->AddSlot().AutoHeight().Padding(FMargin(0.0f, 6.0f, 0.0f, 2.0f))
		[
			SegRow
		];
	}

	CatalogPanelRoot->AddSlot().AutoHeight().Padding(FMargin(0.0f, 0.0f, 0.0f, 8.0f))
	[
		SNew(STextBlock)
		.Font(SpaceshipShipBuilderUiPrivate::BodyFont(10))
		.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
		.Justification(ETextJustify::Center)
		.Text(FText::Format(
			LOCTEXT("CatTypeCount", "{0} / {1}"),
			FText::AsNumber(TotalTypes > 0 ? Idx + 1 : 0),
			FText::AsNumber(TotalTypes)))
	];

	CatalogPanelRoot->AddSlot().AutoHeight().Padding(FMargin(0.0f, 0.0f, 0.0f, 6.0f))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Font(SpaceshipShipBuilderUiPrivate::CapsFont(11))
			.ColorAndOpacity(FLinearColor(0.45f, 0.95f, 1.0f))
			.Text(CurrName)
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.Font(SpaceshipShipBuilderUiPrivate::CapsFont(11))
			.ColorAndOpacity(FLinearColor(0.45f, 0.95f, 1.0f))
			.Text(LOCTEXT("CatPriceCol", "ЦЕНА"))
		]
	];

	CatalogPanelRoot->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.MaxDesiredHeight(420.0f)
		[
			SAssignNew(CatalogList, SScrollBox)
		]
	];

	if (TotalTypes <= 0)
	{
		CatalogList->AddSlot().Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			SNew(STextBlock)
			.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
			.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
			.Text(LOCTEXT("CatNoModules", "В каталоге нет модулей."))
		];
		return;
	}

	for (UShipModuleDefinition* Def : Cat->GetAllModules())
	{
		if (!Def || Def->ModuleType != CurrType)
		{
			continue;
		}
		const FName Mid = Def->ModuleId;
		const int32 Price = Def->GetEffectiveCreditCost();
		CatalogList->AddSlot()
		.Padding(FMargin(0.0f, 2.0f, 0.0f, 2.0f))
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
				}
			})
			.OnUnhovered_Lambda([this]()
			{
				if (OwnerPC.IsValid())
				{
					OwnerPC->SetHoveredCatalogModule(NAME_None);
				}
			})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				.Padding(FMargin(4.0f, 2.0f, 8.0f, 2.0f))
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
					.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextHi())
					.WrapTextAt(220.0f)
					.Text(Def->DisplayName.IsEmpty() ? FText::FromName(Mid) : Def->DisplayName)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 2.0f, 4.0f, 2.0f))
				[
					SNew(STextBlock)
					.Font(SpaceshipShipBuilderUiPrivate::BodyFont(11))
					.ColorAndOpacity(SpaceshipShipBuilderUiPrivate::TextMutedHi())
					.Text(FText::AsNumber(Price))
				]
			]
		];
	}
}

#undef LOCTEXT_NAMESPACE
