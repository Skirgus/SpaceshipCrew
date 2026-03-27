// Copyright Epic Games, Inc. All Rights Reserved.


#include "SideScrollingCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

void ASideScrollingCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// убедиться view цель is a pawn
	APawn* TargetPawn = Cast<APawn>(OutVT.Target);

	// is our цель валидный?
	if (IsValid(TargetPawn))
	{
 // установить view цель FOV и rotation
		OutVT.POV.Rotation = FRotator(0.0f, -90.0f, 0.0f);
		OutVT.POV.FOV = 65.0f;

 // cache текущий location
		FVector CurrentActorLocation = OutVT.Target->GetActorLocation();

 // copy текущий камера location
		FVector CurrentCameraLocation = GetCameraLocation();

 // calculate "zoom distance" - в reality дистанция we want для сохранить для цель
		float CurrentY = CurrentZoom + CurrentActorLocation.Y;

 // do first-time setup
		if (bSetup)
		{
 // lower setup flag
			bSetup = false;

 // инициализировать камера viewpoint и вернуть
			OutVT.POV.Location.X = CurrentActorLocation.X;
			OutVT.POV.Location.Y = CurrentY;
			OutVT.POV.Location.Z = CurrentActorLocation.Z + CameraZOffset;

 // сохранить текущий камера height
			CurrentZ = OutVT.POV.Location.Z;

 // skip rest из calculations
			return;
		}

 // проверить если камера needs для обновить its height
		bool bZUpdate = false;

 // is персонаж движущаяся vertiвызватьy?
		if (FMath::IsNearlyZero(TargetPawn->GetVelocity().Z))
		{
 // determine если we need для do a height обновить
			bZUpdate = FMath::IsNearlyEqual(CurrentZ, CurrentCameraLocation.Z, 25.0f);

		} else {

 // run a trace below персонаж для determine если we need для do a height обновить
			FHitResult OutHit;

			const FVector End = CurrentActorLocation + FVector(0.0f, 0.0f, -1000.0f);

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(TargetPawn);

 // только обновить height если мы not about для попадание ground
			bZUpdate = !GetWorld()->LineTraceSingleByChannel(OutHit, CurrentActorLocation, End, ECC_Visibility, QueryParams);

		}

 // do we need для do a height обновить?
		if (bZUpdate)
		{

 // установить height goal из актор location
			CurrentZ = CurrentActorLocation.Z;

		} else {

 // are we close enough для цель height?
			if (FMath::IsNearlyEqual(CurrentZ, CurrentActorLocation.Z, 100.0f))
			{
 // установить height goal из актор location
				CurrentZ = CurrentActorLocation.Z;

			} else {

 // blend height towards актор location
				CurrentZ = FMath::FInterpTo(CurrentZ, CurrentActorLocation.Z, DeltaTime, 2.0f);
				
			}

		}

 // clamp X axis для минимальный и максимальный камера bounds
		float CurrentX = FMath::Clamp(CurrentActorLocation.X, CameraXMinBounds, CameraXMaxBounds);

 // blend towards new камера позиция и обновить output
		FVector TargetCameraLocation(CurrentX, CurrentY, CurrentZ);

		OutVT.POV.Location = FMath::VInterpTo(CurrentCameraLocation, TargetCameraLocation, DeltaTime, 2.0f);
	}
}



