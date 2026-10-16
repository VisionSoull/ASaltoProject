#include "CameraPIESaveLibrary.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "ScopedTransaction.h"
#endif

bool UCameraPIESaveLibrary::SavePIEActorToEditor(
    AActor* PIEActor,
    FString& OutMessage)
{
#if WITH_EDITOR

    if (!GEditor || !IsValid(PIEActor))
    {
        OutMessage = TEXT("Invalid actor or editor.");
        return false;
    }

    UWorld* PIEWorld = PIEActor->GetWorld();

    if (!PIEWorld ||
        PIEWorld->WorldType != EWorldType::PIE)
    {
        OutMessage = TEXT("Actor is not in PIE.");
        return false;
    }

    // Find the original editor-world actor.
    AActor* EditorActor =
        EditorUtilities::GetEditorWorldCounterpartActor(
            PIEActor
        );

    if (!IsValid(EditorActor))
    {
        OutMessage = TEXT(
            "Could not find the original editor actor."
        );
        return false;
    }

    if (EditorActor->GetClass() != PIEActor->GetClass())
    {
        OutMessage = TEXT("Actor classes do not match.");
        return false;
    }

    // Create an editor transaction for Undo support.
    const FScopedTransaction Transaction(
        NSLOCTEXT(
            "CameraRail",
            "SavePIECameraRail",
            "Save Camera Rail Changes from PIE"
        )
    );

    EditorActor->Modify();

    // Copy properties automatically instead of listing variables.
    const EditorUtilities::ECopyOptions::Type CopyFlags =
        static_cast<EditorUtilities::ECopyOptions::Type>(
            EditorUtilities::ECopyOptions::OnlyCopyEditOrInterpProperties |
            EditorUtilities::ECopyOptions::CallPostEditChangeProperty |
            EditorUtilities::ECopyOptions::CallPostEditMove |
            EditorUtilities::ECopyOptions::FilterBlueprintReadOnly
            );

    const EditorUtilities::FCopyOptions Options(CopyFlags);

    const int32 CopiedProperties =
        EditorUtilities::CopyActorProperties(
            PIEActor,
            EditorActor,
            Options
        );

    EditorActor->MarkPackageDirty();

    if (ULevel* Level = EditorActor->GetLevel())
    {
        Level->MarkPackageDirty();
    }

    OutMessage = FString::Printf(
        TEXT("Copied %d properties to %s. Save the level to persist changes."),
        CopiedProperties,
        *EditorActor->GetActorLabel()
    );

    return true;

#else

    OutMessage = TEXT("Editor-only functionality.");
    return false;

#endif
}