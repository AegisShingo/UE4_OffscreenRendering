// Fill out your copyright notice in the Description page of Project Settings.


#include "PreviewWorldUpdater.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Components/ReflectionCaptureComponent.h"

APreviewWorldUpdater::APreviewWorldUpdater()
{
	PrimaryActorTick.bCanEverTick = true;
}

void APreviewWorldUpdater::BeginPlay()
{
	Super::BeginPlay();

	// PreviewWorldを非同期ロード
	if (PreviewWorldId.IsValid())
	{
		if (UAssetManager* AssetManager = UAssetManager::GetIfValid())
		{
			LoadHandle = AssetManager->LoadPrimaryAsset(PreviewWorldId);

			if (LoadHandle.IsValid())
			{
				if (!LoadHandle->HasLoadCompleted())
				{
					LoadHandle->BindCompleteDelegate(FStreamableDelegate::CreateUObject(this, &ThisClass::HandleLoadCompleted));
				}
				else
				{
					HandleLoadCompleted();
				}
			}
		}
	}
}

void APreviewWorldUpdater::HandleLoadCompleted()
{
	// ロードしたPreviewWorldを受け取る
	UObject* AssetLoaded = LoadHandle->GetLoadedAsset();
	PreviewWorld = Cast<UWorld>(AssetLoaded);
	LoadHandle.Reset();

	// PreviewWorldを初期化
	if (IsValid(PreviewWorld))
	{
		PreviewWorld->WorldType = EWorldType::GamePreview;
		PreviewWorld->SetGameInstance(GetGameInstance());

		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(PreviewWorld->WorldType);
		WorldContext.SetCurrentWorld(PreviewWorld);
		WorldContext.OwningGameInstance = GetGameInstance();

		if (!PreviewWorld->bIsWorldInitialized)
		{
			PreviewWorld->InitWorld(UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false)
				.CreateFXSystem(false));
		}

		FURL URL;
		PreviewWorld->InitializeActorsForPlay(URL);
		PreviewWorld->BeginPlay();
	}
}

void APreviewWorldUpdater::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// PreviewWorldの後処理
	if (IsValid(PreviewWorld))
	{
		// 最後に念のため遅延シーンキャプチャの更新を呼んで、内部のリストをクリアする
		USceneCaptureComponent::UpdateDeferredCaptures(PreviewWorld->Scene);

		GEngine->DestroyWorldContext(PreviewWorld);
		PreviewWorld->DestroyWorld(false);
		PreviewWorld = nullptr;
	}
}

void APreviewWorldUpdater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// PreviewWorldのTickを呼ぶとcheckに引っかかるので、仕方なくアクターのTickをそれぞれ呼び出すことにする
	if (IsValid(PreviewWorld))
	{
		for (FConstLevelIterator LevelItr = PreviewWorld->GetLevelIterator(); LevelItr; ++LevelItr)
		{
			for (AActor* Actor : (*LevelItr)->Actors)
			{
				if (IsValid(Actor))
				{
					if (Actor->PrimaryActorTick.bCanEverTick)
					{
						Actor->TickActor(DeltaTime, ELevelTick::LEVELTICK_All, Actor->PrimaryActorTick);
					}
					for (UActorComponent* Component : Actor->GetComponents())
					{
						if (IsValid(Component) && Component->PrimaryComponentTick.bCanEverTick)
						{
							Component->TickComponent(DeltaTime, ELevelTick::LEVELTICK_All, &Component->PrimaryComponentTick);
						}
					}
				}
			}
		}

		// キャプチャの更新
		USkyLightComponent::UpdateSkyCaptureContents(PreviewWorld);
		UReflectionCaptureComponent::UpdateReflectionCaptureContents(PreviewWorld);
		USceneCaptureComponent::UpdateDeferredCaptures(PreviewWorld->Scene);
	}
}

