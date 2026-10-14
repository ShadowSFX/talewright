def can_build(env, platform):
    # Talewright: the JRPG runtime ships in export templates; editor-only code lives in editor/ and is compiled only
    # when env.editor_build (see SCsub). Layout and boundaries: plan/specs/01_module_layout.md (D-004).
    return True


def configure(env):
    pass


def get_doc_classes():
    return [
        "JRPGActorData",
        "JRPGActorState",
        "JRPGAnimationData",
        "JRPGArmorData",
        "JRPGAudioCue",
        "JRPGAutotile",
        "JRPGAutotileImporter",
        "JRPGAutotileResolver",
        "JRPGBattleRules",
        "JRPGCharacterSheet",
        "JRPGCharacterSprite",
        "JRPGChoiceWindow",
        "JRPGClassData",
        "JRPGCommonEventData",
        "JRPGDatabase",
        "JRPGEnemyData",
        "JRPGEntry",
        "JRPGEquipData",
        "JRPGEventState",
        "JRPGFormula",
        "JRPGFormulaBattler",
        "JRPGGameState",
        "JRPGGoldWindow",
        "JRPGItemData",
        "JRPGManagedAddon",
        "JRPGMap",
        "JRPGMapCellCache",
        "JRPGMapData",
        "JRPGMapEditing",
        "JRPGMapFiles",
        "JRPGMapList",
        "JRPGMapShapes",
        "JRPGMessageText",
        "JRPGMessageWindow",
        "JRPGMover",
        "JRPGNumberInputWindow",
        "JRPGProjectSetup",
        "JRPGPlayer",
        "JRPGRandom",
        "JRPGRandomStreams",
        "JRPGReferenceScanner",
        "JRPGSceneManager",
        "JRPGSelectableList",
        "JRPGSimClock",
        "JRPGSkillData",
        "JRPGStatBlock",
        "JRPGStateData",
        "JRPGSystemData",
        "JRPGTable",
        "JRPGTermsData",
        "JRPGTileFlags",
        "JRPGTilesetData",
        "JRPGTroopData",
        "JRPGTroopPage",
        "JRPGTypesData",
        "JRPGUsableData",
        "JRPGWeaponData",
        "JRPGWindow",
    ]


def get_doc_path():
    return "doc_classes"
