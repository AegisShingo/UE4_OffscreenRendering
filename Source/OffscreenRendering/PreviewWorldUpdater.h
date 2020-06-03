// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewWorldUpdater.generated.h"

UCLASS()
class OFFSCREENRENDERING_API APreviewWorldUpdater : public AActor
{
	GENERATED_BODY()
	
public:	
	APreviewWorldUpdater();

protected:
	virtual void BeginPlay() override;

	// ロード完了コールバック
	void HandleLoadCompleted();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:
	// プレビューするマップのアセットID
	UPROPERTY(EditAnywhere)
		FPrimaryAssetId PreviewWorldId;

	// プレビューするマップ
	UPROPERTY()
		UWorld* PreviewWorld;

	// 非同期ロードハンドル
	TSharedPtr<struct FStreamableHandle> LoadHandle;
};
