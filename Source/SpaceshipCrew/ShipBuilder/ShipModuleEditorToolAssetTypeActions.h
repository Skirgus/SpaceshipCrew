#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FShipModuleEditorToolAssetTypeActions final : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
};

#endif // WITH_EDITOR

