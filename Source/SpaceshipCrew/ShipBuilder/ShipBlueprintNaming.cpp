#include "ShipBlueprintNaming.h"

#include "Misc/Crc.h"

namespace ShipBlueprintNamingPrivate
{
	static FString TransliterateSingleChar(const TCHAR Ch)
	{
		switch (Ch)
		{
		case TEXT('а'): case TEXT('А'): return TEXT("a");
		case TEXT('б'): case TEXT('Б'): return TEXT("b");
		case TEXT('в'): case TEXT('В'): return TEXT("v");
		case TEXT('г'): case TEXT('Г'): return TEXT("g");
		case TEXT('д'): case TEXT('Д'): return TEXT("d");
		case TEXT('е'): case TEXT('Е'): return TEXT("e");
		case TEXT('ё'): case TEXT('Ё'): return TEXT("e");
		case TEXT('ж'): case TEXT('Ж'): return TEXT("zh");
		case TEXT('з'): case TEXT('З'): return TEXT("z");
		case TEXT('и'): case TEXT('И'): return TEXT("i");
		case TEXT('й'): case TEXT('Й'): return TEXT("y");
		case TEXT('к'): case TEXT('К'): return TEXT("k");
		case TEXT('л'): case TEXT('Л'): return TEXT("l");
		case TEXT('м'): case TEXT('М'): return TEXT("m");
		case TEXT('н'): case TEXT('Н'): return TEXT("n");
		case TEXT('о'): case TEXT('О'): return TEXT("o");
		case TEXT('п'): case TEXT('П'): return TEXT("p");
		case TEXT('р'): case TEXT('Р'): return TEXT("r");
		case TEXT('с'): case TEXT('С'): return TEXT("s");
		case TEXT('т'): case TEXT('Т'): return TEXT("t");
		case TEXT('у'): case TEXT('У'): return TEXT("u");
		case TEXT('ф'): case TEXT('Ф'): return TEXT("f");
		case TEXT('х'): case TEXT('Х'): return TEXT("h");
		case TEXT('ц'): case TEXT('Ц'): return TEXT("ts");
		case TEXT('ч'): case TEXT('Ч'): return TEXT("ch");
		case TEXT('ш'): case TEXT('Ш'): return TEXT("sh");
		case TEXT('щ'): case TEXT('Щ'): return TEXT("sch");
		case TEXT('ъ'): case TEXT('Ъ'): return TEXT("");
		case TEXT('ы'): case TEXT('Ы'): return TEXT("y");
		case TEXT('ь'): case TEXT('Ь'): return TEXT("");
		case TEXT('э'): case TEXT('Э'): return TEXT("e");
		case TEXT('ю'): case TEXT('Ю'): return TEXT("yu");
		case TEXT('я'): case TEXT('Я'): return TEXT("ya");
		case TEXT('ї'): case TEXT('Ї'): return TEXT("i");
		case TEXT('і'): case TEXT('І'): return TEXT("i");
		case TEXT('є'): case TEXT('Є'): return TEXT("e");
		case TEXT('ґ'): case TEXT('Ґ'): return TEXT("g");
		default: return FString(1, &Ch);
		}
	}

	static void AppendSanitizedChar(FString& Out, const TCHAR Ch)
	{
		if (FChar::IsAlnum(Ch))
		{
			Out.AppendChar(FChar::ToLower(Ch));
		}
		else if (Ch == TEXT(' ') || Ch == TEXT('-') || Ch == TEXT('_'))
		{
			if (!Out.IsEmpty() && Out[Out.Len() - 1] != TEXT('_'))
			{
				Out.AppendChar(TEXT('_'));
			}
		}
	}

	static void CollapseUnderscores(FString& InOut)
	{
		while (InOut.Contains(TEXT("__")))
		{
			InOut.ReplaceInline(TEXT("__"), TEXT("_"));
		}
		bool bTrimmed = false;
		InOut.TrimCharInline(TEXT('_'), &bTrimmed);
	}
}

FString ShipBlueprintNaming::MakeReadableSlug(const FString& DisplayName)
{
	FString Slug;
	Slug.Reserve(DisplayName.Len() * 2);

	for (const TCHAR Ch : DisplayName)
	{
		if (FChar::IsAlnum(Ch) && Ch < 128)
		{
			ShipBlueprintNamingPrivate::AppendSanitizedChar(Slug, Ch);
			continue;
		}

		const FString Transliterated = ShipBlueprintNamingPrivate::TransliterateSingleChar(Ch);
		for (const TCHAR TCh : Transliterated)
		{
			ShipBlueprintNamingPrivate::AppendSanitizedChar(Slug, TCh);
		}
	}

	ShipBlueprintNamingPrivate::CollapseUnderscores(Slug);

	if (Slug.IsEmpty())
	{
		Slug = TEXT("ship");
	}

	return Slug;
}

FString ShipBlueprintNaming::MakeStableHashSuffix(const FString& DisplayName)
{
	const uint32 Hash = FCrc::StrCrc32(*DisplayName);
	return FString::Printf(TEXT("%08x"), Hash);
}

FName ShipBlueprintNaming::MakeShipIdFromDisplayName(const FString& DisplayName)
{
	const FString Slug = MakeReadableSlug(DisplayName);
	const FString Hash = MakeStableHashSuffix(DisplayName);
	const FString Combined = FString::Printf(TEXT("%s_%s"), *Slug, *Hash);
	return FName(*Combined);
}

FName ShipBlueprintNaming::MakeUniquePlayerShipId(const FString& BaseDisplayName, const int32 NumericSuffix)
{
	FString Id = MakeShipIdFromDisplayName(BaseDisplayName).ToString();
	if (NumericSuffix > 0)
	{
		Id += FString::Printf(TEXT("_%d"), NumericSuffix);
	}
	return FName(*Id);
}

bool ShipBlueprintNaming::TrySanitizeShipIdForFilename(const FName ShipId, FString& OutFileBaseName)
{
	OutFileBaseName.Reset();
	const FString Raw = ShipId.ToString();
	if (Raw.IsEmpty())
	{
		return false;
	}
	if (Raw.Contains(TEXT("..")) || Raw.Contains(TEXT("/")) || Raw.Contains(TEXT("\\"))
		|| Raw.Contains(TEXT(":")))
	{
		return false;
	}

	FString Sanitized;
	Sanitized.Reserve(Raw.Len());
	for (const TCHAR Ch : Raw)
	{
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
		{
			Sanitized.AppendChar(Ch);
		}
		else
		{
			return false;
		}
	}

	if (Sanitized.IsEmpty())
	{
		return false;
	}

	OutFileBaseName = Sanitized;
	return true;
}
