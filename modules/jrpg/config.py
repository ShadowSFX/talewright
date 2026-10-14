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
        "JRPGBattleRules",
        "JRPGClassData",
        "JRPGCommonEventData",
        "JRPGDatabase",
        "JRPGEnemyData",
        "JRPGEntry",
        "JRPGEquipData",
        "JRPGFormula",
        "JRPGFormulaBattler",
        "JRPGGameState",
        "JRPGItemData",
        "JRPGRandom",
        "JRPGRandomStreams",
        "JRPGSimClock",
        "JRPGSkillData",
        "JRPGStatBlock",
        "JRPGStateData",
        "JRPGSystemData",
        "JRPGTable",
        "JRPGTermsData",
        "JRPGTilesetData",
        "JRPGTroopData",
        "JRPGTroopPage",
        "JRPGTypesData",
        "JRPGUsableData",
        "JRPGWeaponData",
    ]


def get_doc_path():
    return "doc_classes"
