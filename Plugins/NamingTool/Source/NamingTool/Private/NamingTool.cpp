// Copyright Epic Games, Inc. All Rights Reserved.

#include "NamingTool.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "AssetThumbnail.h"
#include "Styling/AppStyle.h"
#include "Brushes/SlateRoundedBoxBrush.h"

#define LOCTEXT_NAMESPACE "FNamingToolModule"

void FNamingToolModule::StartupModule()
{
#if WITH_EDITOR
	FEditorDelegates::OnAssetPostImport.AddRaw(this, &FNamingToolModule::OnAssetPostImport);
#endif
}

void FNamingToolModule::ShutdownModule()
{
#if WITH_EDITOR
	FEditorDelegates::OnAssetPostImport.RemoveAll(this);
#endif
}

void FNamingToolModule::OnAssetPostImport(UFactory* Factory, UObject* ImportedObject)
{

#if WITH_EDITOR
	if (ImportedObject)
	{
		PendingImportedAssets.Add(ImportedObject);
		if (GEditor && GEditor->GetEditorWorldContext().World())
		{
			GEditor->GetEditorWorldContext().World()->GetTimerManager().ClearTimer(ImportBatchTimer);
			GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(ImportBatchTimer, [this]() { HandleImportBatch(); }, 0.2f, false);
		}
	}
#endif
}

void FNamingToolModule::HandleImportBatch()
{
	if (PendingImportedAssets.Num() > 0)
	{
		ShowAssetNamingPopup(PendingImportedAssets);
		PendingImportedAssets.Empty();
	}
}

class SAssetNamingPopup : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetNamingPopup) {}
		SLATE_ARGUMENT(TArray<UObject*>, ImportedAssets)
	SLATE_END_ARGS()

	static TArray<TPair<FString, UClass*>> AssetTypePrefixes;

	void Construct(const FArguments& InArgs)
	{
		ImportedAssets = InArgs._ImportedAssets;
		CollectAvailableFolders();

		// Prepare asset rows
		for (UObject* Asset : ImportedAssets)
		{
			TSharedPtr<FAssetRow> Row = MakeShareable(new FAssetRow(Asset, AvailableFolders));
			AssetRows.Add(Row);
		}

		ChildSlot
		[
			SNew(SBorder)
			.Padding(24)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
			.BorderBackgroundColor(FLinearColor(0.08f, 0.09f, 0.12f, 1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("Brushes.Header"))
					.Padding(FMargin(8,4))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Preview"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Type Prefix"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Name"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Descriptor"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Variant"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(2)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Destination Folder"))
							.Font(FCoreStyle::Get().GetFontStyle("BoldFont"))
							.ColorAndOpacity(FLinearColor(0.85f,0.85f,0.95f,1.0f))
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0,0,0,12)
				[
					SAssignNew(AssetListView, SListView<TSharedPtr<FAssetRow>>)
					.ListItemsSource(&AssetRows)
					.OnGenerateRow(this, &SAssetNamingPopup::OnGenerateRow)
					.SelectionMode(ESelectionMode::None)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).HAlign(HAlign_Right)
					[
						SNew(SButton)
						.Text(FText::FromString("Confirm"))
						.ToolTipText(FText::FromString("Apply all changes and move/rename assets."))
						.ContentPadding(FMargin(14, 6))
						.ButtonColorAndOpacity(FLinearColor(0.2f,0.7f,0.4f,1.0f))
						.ForegroundColor(FLinearColor(1,1,1,1))
						.OnClicked(this, &SAssetNamingPopup::OnConfirmClicked)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
					[
						SNew(SButton)
						.Text(FText::FromString("Cancel"))
						.ToolTipText(FText::FromString("Close this window without making changes."))
						.ContentPadding(FMargin(14, 6))
						.ButtonColorAndOpacity(FLinearColor(0.7f,0.2f,0.2f,1.0f))
						.ForegroundColor(FLinearColor(1,1,1,1))
						.OnClicked(this, &SAssetNamingPopup::OnCancelClicked)
					]
				]
			]
		];
	}

private:
	struct FAssetRow
	{
		UObject* Asset;
		FString AssetTypePrefix;
		FString AssetName;
		FString Descriptor;
		FString Variant;
		FString DestinationFolder;
		TArray<TSharedPtr<FString>> AvailableFolders;
		TSharedPtr<FString> SelectedFolderPtr;

		FAssetRow(UObject* InAsset, const TArray<FString>& InAvailableFolders)
			: Asset(InAsset)
		{
			for (const auto& Pair : SAssetNamingPopup::AssetTypePrefixes)
			{
				if (Asset->IsA(Pair.Value))
				{
					AssetTypePrefix = Pair.Key;
					break;
				}
			}
			if (AssetTypePrefix.IsEmpty()) AssetTypePrefix = "A_";
			AssetName = Asset->GetName();

			// Automatic descriptor detection for textures
			if (Asset->IsA(UTexture::StaticClass()))
			{
				Descriptor = DetectTextureDescriptor(AssetName);
			}
			else
			{
				Descriptor = "";
			}
			Variant = "";

			// Convert InAvailableFolders to TArray<TSharedPtr<FString>>
			for (const FString& Folder : InAvailableFolders)
			{
				AvailableFolders.Add(MakeShareable(new FString(Folder)));
			}

			// Set default folder to imported asset's folder if possible
			FString ImportedPackage = Asset->GetOutermost()->GetName();
			int32 LastSlashIdx;
			FString ImportedFolder;
			if (ImportedPackage.FindLastChar('/', LastSlashIdx))
			{
				ImportedFolder = ImportedPackage.Left(LastSlashIdx);
			}
			else
			{
				ImportedFolder = InAvailableFolders.Num() > 0 ? InAvailableFolders[0] : TEXT("");
			}
			DestinationFolder = ImportedFolder;
			// Set SelectedFolderPtr
			for (const TSharedPtr<FString>& FolderPtr : AvailableFolders)
			{
				if (FolderPtr.IsValid() && *FolderPtr == ImportedFolder)
				{
					SelectedFolderPtr = FolderPtr;
					break;
				}
			}
			if (!SelectedFolderPtr.IsValid() && AvailableFolders.Num() > 0)
			{
				SelectedFolderPtr = AvailableFolders[0];
			}
		}

		static FString DetectTextureDescriptor(const FString& Name)
		{
			FString LowerName = Name.ToLower();
			if (LowerName.Contains("_n") || LowerName.Contains("_normal")) return "Normal";
			if (LowerName.Contains("_a") || LowerName.Contains("_albedo") || LowerName.Contains("_d") || LowerName.Contains("_diffuse")) return "Albedo";
			if (LowerName.Contains("_o") || LowerName.Contains("_opacity")) return "Opacity";
			if (LowerName.Contains("_m") || LowerName.Contains("_metallic")) return "Metallic";
			if (LowerName.Contains("_r") || LowerName.Contains("_roughness")) return "Roughness";
			if (LowerName.Contains("_s") || LowerName.Contains("_specular")) return "Specular";
			if (LowerName.Contains("_ao") || LowerName.Contains("_ambientocclusion")) return "AO";
			if (LowerName.Contains("_emissive")) return "Emissive";
			if (LowerName.Contains("_height")) return "Height";
			if (LowerName.Contains("_cavity")) return "Cavity";
			if (LowerName.Contains("_mask")) return "Mask";
			return "";
		}
	};

	TArray<UObject*> ImportedAssets;
	TArray<TSharedPtr<FAssetRow>> AssetRows;
	TSharedPtr<SListView<TSharedPtr<FAssetRow>>> AssetListView;
	TArray<FString> AvailableFolders;

	void CollectAvailableFolders()
	{
		AvailableFolders.Empty();
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FString> Paths;
		AssetRegistryModule.Get().GetAllCachedPaths(Paths);
		for (const FString& Path : Paths)
		{
			// Only include folders under /Game/ASaltoProject/
			if (Path.StartsWith("/Game/ASaltoProject/"))
			{
				AvailableFolders.Add(Path);
			}
		}
	}

	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FAssetRow> Row, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<FAssetThumbnail> AssetThumbnail;
		bool bValidAsset = Row->Asset && Row->Asset->IsValidLowLevel();
		UE_LOG(LogTemp, Warning, TEXT("Asset: %s, Class: %s, Valid: %d"),
			Row->Asset ? *Row->Asset->GetName() : TEXT("NULL"),
			Row->Asset ? *Row->Asset->GetClass()->GetName() : TEXT("NULL"),
			bValidAsset ? 1 : 0);
		if (bValidAsset)
		{
			AssetThumbnail = MakeShareable(new FAssetThumbnail(Row->Asset, 64, 64, nullptr));
		}
		return SNew(STableRow<TSharedPtr<FAssetRow>>, OwnerTable)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			.Padding(FMargin(6,4))
			[
				SNew(SHorizontalBox)
				// Texture preview column
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(SBox)
					.WidthOverride(64)
					.HeightOverride(64)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
						.Padding(2)
						[
							(AssetThumbnail.IsValid() && bValidAsset) ? AssetThumbnail->MakeThumbnailWidget() : SNew(STextBlock).Text(FText::FromString("No Preview"))
						]
					]
				]
				// Prefix
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row->AssetTypePrefix))
					.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
					.ToolTipText(FText::FromString("Asset type prefix (e.g. T_, SM_, BP_)."))
					.MinDesiredWidth(40)
				]
				// Name
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(Row->AssetName))
					.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
					.ToolTipText(FText::FromString("Main asset name."))
					.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type){ Row->AssetName = NewText.ToString(); })
					.MinDesiredWidth(120)
				]
				// Descriptor
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(Row->Descriptor))
					.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
					.ToolTipText(FText::FromString("Descriptor (e.g. Normal, Albedo, Mask, etc.)."))
					.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type){ Row->Descriptor = NewText.ToString(); })
					.MinDesiredWidth(80)
				]
				// Variant
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(SEditableTextBox)
					.Text(FText::FromString(Row->Variant))
					.Font(FCoreStyle::Get().GetFontStyle("NormalFont"))
					.ToolTipText(FText::FromString("Optional variant letter or number."))
					.OnTextCommitted_Lambda([Row](const FText& NewText, ETextCommit::Type){ Row->Variant = NewText.ToString(); })
					.MinDesiredWidth(40)
				]
				// Destination Folder
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
				[
					SNew(SBox)
					.MinDesiredWidth(120)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&Row->AvailableFolders)
						.InitiallySelectedItem(Row->SelectedFolderPtr)
						.ToolTipText(FText::FromString("Select the destination folder for this asset."))
						.OnSelectionChanged_Lambda([Row](TSharedPtr<FString> NewSelection, ESelectInfo::Type) {
							if (NewSelection.IsValid()) {
							 Row->DestinationFolder = *NewSelection;
							 Row->SelectedFolderPtr = NewSelection;
							}
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem) {
							return SNew(STextBlock).Text(FText::FromString(InItem.IsValid() ? *InItem : TEXT("")));
						})
						.Content()
						[
							SNew(STextBlock)
							.Text_Lambda([Row]() { return FText::FromString(Row->SelectedFolderPtr.IsValid() ? *Row->SelectedFolderPtr : TEXT("")); })
						]
					]
				]
			]
		];
	}

	FReply OnConfirmClicked()
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		IAssetTools& AssetTools = AssetToolsModule.Get();

		TArray<FAssetRenameData> RenameDataArray;
		for (TSharedPtr<FAssetRow>& Row : AssetRows)
		{
			if (!Row.IsValid() || !Row->Asset) continue;

			FString NewName = Row->AssetTypePrefix + Row->AssetName;
			if (!Row->Descriptor.IsEmpty()) NewName += "_" + Row->Descriptor;
			if (!Row->Variant.IsEmpty()) NewName += "_" + Row->Variant;

			FString NewPackagePath = Row->DestinationFolder;
			RenameDataArray.Add(FAssetRenameData(Row->Asset, NewPackagePath, NewName));
		}
		AssetTools.RenameAssets(RenameDataArray);

		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Assets renamed and moved!"));

		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWindow.IsValid())
		{
			ParentWindow->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	FReply OnCancelClicked()
	{
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (ParentWindow.IsValid())
		{
			ParentWindow->RequestDestroyWindow();
		}
		return FReply::Handled();
	}
};

TArray<TPair<FString, UClass*>> SAssetNamingPopup::AssetTypePrefixes = {
	TPair<FString, UClass*>(TEXT("T_"), UTexture::StaticClass()),
	TPair<FString, UClass*>(TEXT("SM_"), UStaticMesh::StaticClass()),
	TPair<FString, UClass*>(TEXT("SK_"), USkeletalMesh::StaticClass()),
	TPair<FString, UClass*>(TEXT("M_"), UMaterial::StaticClass()),
	TPair<FString, UClass*>(TEXT("MI_"), UMaterialInstance::StaticClass()),
	TPair<FString, UClass*>(TEXT("S_"), USoundBase::StaticClass()),
	TPair<FString, UClass*>(TEXT("A_"), UAnimSequence::StaticClass()),
	TPair<FString, UClass*>(TEXT("BP_"), UBlueprint::StaticClass())
	// Add more pairs here for future asset types
};

void FNamingToolModule::ShowAssetNamingPopup(const TArray<UObject*>& ImportedAssets)
{
   
    float totalWidth = 750; 
    float rowHeight = 64 + 16;
    float totalHeight = 500; 

    TSharedRef<SWindow> PopupWindow = SNew(SWindow)
        .Title(FText::FromString("Asset Naming Tool"))
        .ClientSize(FVector2D(totalWidth, totalHeight))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        .IsTopmostWindow(true)
        .FocusWhenFirstShown(true)
        .ActivationPolicy(EWindowActivationPolicy::Always)
        [
            SNew(SAssetNamingPopup)
            .ImportedAssets(ImportedAssets)
        ];

    FSlateApplication::Get().AddModalWindow(PopupWindow, nullptr, false);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNamingToolModule, NamingTool)