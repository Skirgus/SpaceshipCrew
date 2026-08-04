#include "SSpaceshipBlueprintPicker.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Menu/SpaceshipCrewLevelTravel.h"
#include "ShipBuilder/ShipBlueprintRegistry.h"
#include "ShipBuilder/ShipBlueprintSessionSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SpaceshipBlueprintPicker"

void SSpaceshipBlueprintPicker::Construct(const FArguments& InArgs)
{
	World = InArgs._WorldContext;
	OwnerPC = InArgs._OwnerPC;
	OnBack = InArgs._OnBack;
	OnConfirmed = InArgs._OnConfirmed;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 16.0f))
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
			.Text(LOCTEXT("PickerTitle", "Выбор корабля"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 0.0f, 0.0f, 12.0f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(28.0f, 14.0f))
			.OnClicked(this, &SSpaceshipBlueprintPicker::OnNewShipClicked)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
				.Text(LOCTEXT("NewShip", "Создать новый корабль"))
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 8.0f, 0.0f, 4.0f))
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.Text(LOCTEXT("ProjectHdr", "Шаблоны проекта"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(180.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(ProjectListBox, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 12.0f, 0.0f, 4.0f))
		[
			SNew(STextBlock)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
			.Text(LOCTEXT("PlayerHdr", "Мои корабли"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.MaxHeight(220.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(PlayerListBox, SVerticalBox)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 20.0f, 0.0f, 0.0f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(28.0f, 14.0f))
			.OnClicked(this, &SSpaceshipBlueprintPicker::OnBackClicked)
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
				.Text(LOCTEXT("Back", "Назад"))
			]
		]
	];

	RefreshLists();
}

void SSpaceshipBlueprintPicker::RefreshLists()
{
	if (!ProjectListBox.IsValid() || !PlayerListBox.IsValid())
	{
		return;
	}

	ProjectListBox->ClearChildren();
	PlayerListBox->ClearChildren();

	UGameInstance* GI = World.IsValid() ? World->GetGameInstance() : nullptr;
	UShipBlueprintRegistry* Registry = GI ? GI->GetSubsystem<UShipBlueprintRegistry>() : nullptr;
	if (!Registry)
	{
		ProjectListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoRegistry", "Каталог недоступен"))
		];
		return;
	}

	Registry->EnsureProjectCatalogFresh();
	Registry->EnsurePlayerCatalogFresh();
	BuildEntryList(Registry->GetProjectBlueprints(), ProjectListBox.ToSharedRef());
	BuildEntryList(Registry->GetPlayerBlueprints(), PlayerListBox.ToSharedRef());

	if (Registry->GetProjectBlueprints().Num() == 0)
	{
		ProjectListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.75f))
			.Text(LOCTEXT("NoProject", "— нет шаблонов —"))
		];
	}
	if (Registry->GetPlayerBlueprints().Num() == 0)
	{
		PlayerListBox->AddSlot()
		.AutoHeight()
		[
			SNew(STextBlock)
			.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.75f))
			.Text(LOCTEXT("NoPlayer", "— пока нет сохранений —"))
		];
	}
}

void SSpaceshipBlueprintPicker::BuildEntryList(
	const TArray<FShipBlueprintListEntry>& Entries,
	const TSharedRef<SVerticalBox> TargetBox)
{
	for (const FShipBlueprintListEntry& Entry : Entries)
	{
		const FName ShipId = Entry.ShipId;
		const EShipBlueprintSource Source = Entry.Source;
		const FText Label = Entry.bPlayReady
			? Entry.DisplayName
			: FText::Format(LOCTEXT("DraftLabel", "{0} (черновик)"), Entry.DisplayName);

		TargetBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.0f, 4.0f, 0.0f, 0.0f))
		[
			SNew(SButton)
			.ContentPadding(FMargin(20.0f, 10.0f))
			.OnClicked_Lambda([this, Source, ShipId]()
			{
				return OnEntryClicked(Source, ShipId);
			})
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				.Text(Label)
			]
		];
	}
}

FReply SSpaceshipBlueprintPicker::OnBackClicked()
{
	if (OnBack.IsBound())
	{
		OnBack.Execute();
	}
	return FReply::Handled();
}

void SSpaceshipBlueprintPicker::ShowPickerError(const FText& Message) const
{
	FMessageDialog::Open(EAppMsgType::Ok, Message);
}

FReply SSpaceshipBlueprintPicker::OnNewShipClicked()
{
	UGameInstance* GI = World.IsValid() ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		ShowPickerError(LOCTEXT("NoGI", "Игровая сессия недоступна."));
		return FReply::Handled();
	}

	if (!GI->GetSubsystem<UShipBlueprintSessionSubsystem>())
	{
		ShowPickerError(LOCTEXT("NoSession", "Подсистема сессии конструктора недоступна."));
		return FReply::Handled();
	}

	if (UShipBlueprintSessionSubsystem* Session = GI->GetSubsystem<UShipBlueprintSessionSubsystem>())
	{
		Session->BeginNewShip();
	}

	if (UWorld* W = World.Get())
	{
		UGameplayStatics::OpenLevel(
			W,
			FName(SpaceshipCrewLevelTravel::GetPlayMapPackagePath()),
			false,
			SpaceshipCrewLevelTravel::GetShipBuilderGameOptions());
	}

	if (OnConfirmed.IsBound())
	{
		OnConfirmed.Execute();
	}
	return FReply::Handled();
}

FReply SSpaceshipBlueprintPicker::OnEntryClicked(const EShipBlueprintSource Source, const FName ShipId)
{
	UGameInstance* GI = World.IsValid() ? World->GetGameInstance() : nullptr;
	if (!GI)
	{
		ShowPickerError(LOCTEXT("NoGI2", "Игровая сессия недоступна."));
		return FReply::Handled();
	}

	UShipBlueprintSessionSubsystem* Session = GI->GetSubsystem<UShipBlueprintSessionSubsystem>();
	if (!Session)
	{
		ShowPickerError(LOCTEXT("NoSession2", "Подсистема сессии конструктора недоступна."));
		return FReply::Handled();
	}

	if (!GI->GetSubsystem<UShipBlueprintRegistry>())
	{
		ShowPickerError(LOCTEXT("NoRegistry2", "Каталог чертежей недоступен."));
		return FReply::Handled();
	}

	FString Error;
	if (!Session->BeginEditShip(Source, ShipId, Error))
	{
		ShowPickerError(FText::FromString(Error));
		return FReply::Handled();
	}

	if (UWorld* W = World.Get())
	{
		UGameplayStatics::OpenLevel(
			W,
			FName(SpaceshipCrewLevelTravel::GetPlayMapPackagePath()),
			false,
			SpaceshipCrewLevelTravel::GetShipBuilderGameOptions());
	}

	if (OnConfirmed.IsBound())
	{
		OnConfirmed.Execute();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
