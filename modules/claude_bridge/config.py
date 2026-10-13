def can_build(env, platform):
    # Talewright: the Claude bridge is editor-only and must never be compiled into export templates
    # (plan/DECISIONS.md D-012). `env.editor_build` is true only for target=editor (SConstruct:533).
    return env.editor_build


def configure(env):
    pass
