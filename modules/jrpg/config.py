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
        "JRPGCommandList",
        "JRPGCommandRegistry",
        "JRPGCommonEventData",
        "JRPGDatabase",
        "JRPGEnemyData",
        "JRPGEntry",
        "JRPGEquipData",
        "JRPGEvent",
        "JRPGEventAnchor",
        "JRPGEventCommand",
        "JRPGEventData",
        "JRPGEventPage",
        "JRPGEventRunner",
        "JRPGEventState",
        "JRPGFormula",
        "JRPGFormulaBattler",
        "JRPGGameState",
        "JRPGGoldWindow",
        "JRPGInterpreter",
        "JRPGItemData",
        "JRPGLocalPort",
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
        "JRPGPlayer",
        "JRPGPresentationPort",
        "JRPGProjectSetup",
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
        "JRPGUnknownCommand",
        "JRPGUsableData",
        "JRPGWeaponData",
        "JRPGWindow",
    ]


def get_doc_path():
    return "doc_classes"
