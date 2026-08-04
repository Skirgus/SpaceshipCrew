#include "ShipBlueprintDefinition.h"

FPrimaryAssetId UShipBlueprintDefinition::GetPrimaryAssetId() const
{
	if (ShipId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(TEXT("ShipBlueprint"), ShipId);
}

FShipBlueprintDocument UShipBlueprintDefinition::BuildDocument() const
{
	FShipBlueprintDocument Out = Document;
	Out.SchemaVersion = FShipBlueprintDocument::CurrentSchemaVersion;
	if (!ShipId.IsNone())
	{
		Out.ShipId = ShipId;
	}
	if (!DisplayName.IsEmpty())
	{
		Out.DisplayName = DisplayName;
	}
	else if (Out.DisplayName.IsEmpty() && !ShipId.IsNone())
	{
		Out.DisplayName = FText::FromName(ShipId);
	}
	return Out;
}
