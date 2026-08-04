#include "ShipBlueprintSessionSubsystem.h"

#include "ShipBlueprintRegistry.h"
#include "ShipBlueprintSerializer.h"
#include "ShipModuleCatalog.h"

DEFINE_LOG_CATEGORY_STATIC(LogShipBlueprintSession, Log, All);

void UShipBlueprintSessionSubsystem::BeginNewShip(const FText& DisplayName)
{
	Session = FShipBlueprintSession();
	Session.EditSource = EShipBlueprintEditSource::New;
	Session.DisplayName = DisplayName;
	Session.bRequiresSaveAsOnWrite = false;
	Session.bDirty = false;
	Session.PendingDocument = FShipBlueprintDocument();
	Session.PendingDocument.SchemaVersion = FShipBlueprintDocument::CurrentSchemaVersion;
	if (!DisplayName.IsEmpty())
	{
		Session.PendingDocument.DisplayName = DisplayName;
	}
}

bool UShipBlueprintSessionSubsystem::BeginEditShip(const EShipBlueprintSource Source, const FName ShipId, FString& OutError)
{
	UShipBlueprintRegistry* Registry = GetGameInstance()->GetSubsystem<UShipBlueprintRegistry>();
	if (!Registry)
	{
		OutError = TEXT("ShipBlueprintRegistry недоступен.");
		return false;
	}

	FShipBlueprintDocument Doc;
	if (!Registry->LoadDocument(Source, ShipId, Doc, OutError))
	{
		return false;
	}

	if (UShipModuleCatalog* Catalog = GetGameInstance()->GetSubsystem<UShipModuleCatalog>())
	{
		TArray<FString> ModuleErrors;
		if (!FShipBlueprintSerializer::ValidateModuleReferences(Doc, *Catalog, ModuleErrors))
		{
			for (const FString& Line : ModuleErrors)
			{
				UE_LOG(LogShipBlueprintSession, Warning, TEXT("BeginEditShip: %s"), *Line);
			}
			OutError = FString::Printf(
				TEXT("Чертёж «%s» содержит неизвестные модули и не может быть открыт:\n%s"),
				*ShipId.ToString(),
				*FString::Join(ModuleErrors, TEXT("\n")));
			return false;
		}
	}
	else
	{
		UE_LOG(LogShipBlueprintSession, Warning, TEXT("BeginEditShip: каталог модулей недоступен, проверка ModuleId пропущена."));
	}

	Session = FShipBlueprintSession();
	Session.SourceShipId = ShipId;
	Session.DisplayName = Doc.DisplayName.IsEmpty() ? FText::FromName(ShipId) : Doc.DisplayName;
	Session.PendingDocument = Doc;
	Session.bDirty = false;

	if (Source == EShipBlueprintSource::Project)
	{
		Session.EditSource = EShipBlueprintEditSource::ProjectTemplate;
		Session.bRequiresSaveAsOnWrite = true;
	}
	else
	{
		Session.EditSource = EShipBlueprintEditSource::PlayerOwned;
		Session.bRequiresSaveAsOnWrite = false;
	}

	return true;
}

void UShipBlueprintSessionSubsystem::MarkDirty()
{
	Session.bDirty = true;
}

void UShipBlueprintSessionSubsystem::ClearDirty()
{
	Session.bDirty = false;
}

void UShipBlueprintSessionSubsystem::UpdatePendingLayout(const FShipBuilderDraftConfig& Draft, const int32 CreditCost)
{
	Session.PendingDocument.ModuleLayout = Draft;
	Session.PendingDocument.CachedCreditCost = CreditCost;
	if (Session.PendingDocument.DisplayName.IsEmpty() && !Session.DisplayName.IsEmpty())
	{
		Session.PendingDocument.DisplayName = Session.DisplayName;
	}
}

void UShipBlueprintSessionSubsystem::ApplyPendingToDraft(FShipBuilderDraftConfig& OutDraft) const
{
	OutDraft = Session.PendingDocument.ModuleLayout;
}

bool UShipBlueprintSessionSubsystem::CanSaveInPlace() const
{
	if (Session.bRequiresSaveAsOnWrite)
	{
		return false;
	}
	return Session.EditSource == EShipBlueprintEditSource::PlayerOwned
		|| (Session.EditSource == EShipBlueprintEditSource::New && !Session.SourceShipId.IsNone());
}

bool UShipBlueprintSessionSubsystem::SaveCurrent(FString& OutError)
{
	if (Session.bRequiresSaveAsOnWrite)
	{
		OutError = TEXT("Шаблон проекта можно сохранить только как новый корабль.");
		return false;
	}

	UShipBlueprintRegistry* Registry = GetGameInstance()->GetSubsystem<UShipBlueprintRegistry>();
	if (!Registry)
	{
		OutError = TEXT("ShipBlueprintRegistry недоступен.");
		return false;
	}

	FShipBlueprintDocument Doc = Session.PendingDocument;
	if (Session.EditSource == EShipBlueprintEditSource::New && Session.SourceShipId.IsNone())
	{
		OutError = TEXT("Укажите имя и сохраните как новый корабль.");
		return false;
	}

	if (!Session.SourceShipId.IsNone())
	{
		Doc.ShipId = Session.SourceShipId;
	}
	if (!Session.DisplayName.IsEmpty())
	{
		Doc.DisplayName = Session.DisplayName;
	}
	if (Doc.ShipId.IsNone())
	{
		OutError = TEXT("Пустой ShipId.");
		return false;
	}

	if (!Registry->SavePlayerBlueprint(Doc, OutError))
	{
		return false;
	}

	Session.EditSource = EShipBlueprintEditSource::PlayerOwned;
	Session.SourceShipId = Doc.ShipId;
	Session.bRequiresSaveAsOnWrite = false;
	Session.bDirty = false;
	UpdateSelectedForNewGameIfPlayReady(Doc);
	return true;
}

bool UShipBlueprintSessionSubsystem::SaveAs(const FName NewShipId, const FText& NewDisplayName, FString& OutError)
{
	if (NewShipId.IsNone())
	{
		OutError = TEXT("Идентификатор корабля не может быть пустым.");
		return false;
	}

	UShipBlueprintRegistry* Registry = GetGameInstance()->GetSubsystem<UShipBlueprintRegistry>();
	if (!Registry)
	{
		OutError = TEXT("ShipBlueprintRegistry недоступен.");
		return false;
	}

	FShipBlueprintDocument Doc = Session.PendingDocument;
	Doc.ShipId = NewShipId;
	Doc.DisplayName = NewDisplayName.IsEmpty() ? FText::FromName(NewShipId) : NewDisplayName;

	if (!Registry->SavePlayerBlueprint(Doc, OutError))
	{
		return false;
	}

	Session.EditSource = EShipBlueprintEditSource::PlayerOwned;
	Session.SourceShipId = NewShipId;
	Session.DisplayName = Doc.DisplayName;
	Session.bRequiresSaveAsOnWrite = false;
	Session.bDirty = false;
	Session.PendingDocument = Doc;
	UpdateSelectedForNewGameIfPlayReady(Doc);
	return true;
}

bool UShipBlueprintSessionSubsystem::TrySetSelectedShipForNewGame(
	const FShipBlueprintDocument& Document,
	FString& OutError)
{
	UShipModuleCatalog* Catalog = GetGameInstance()->GetSubsystem<UShipModuleCatalog>();
	if (!Catalog)
	{
		OutError = TEXT("Каталог модулей недоступен.");
		return false;
	}

	TArray<FString> PlayBlockers;
	if (!FShipBlueprintSerializer::ValidateDocumentForPlay(Document, *Catalog, PlayBlockers))
	{
		OutError = PlayBlockers.Num() > 0
			? FString::Join(PlayBlockers, TEXT("\n"))
			: TEXT("Корабль не готов к полёту.");
		return false;
	}

	SelectedForNewGame = Document;
	return true;
}

void UShipBlueprintSessionSubsystem::UpdateSelectedForNewGameIfPlayReady(const FShipBlueprintDocument& Document)
{
	FString Error;
	TrySetSelectedShipForNewGame(Document, Error);
}
