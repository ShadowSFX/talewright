# Spec 04 — RNG, parameters, traits, formulas, battle math

Decisions: D-009 (formulas), D-010 (determinism, RNG), D-019 (battle systems), D-020 (legal), D-022 (undocumented
numbers are Talewright-defined, documented and data-driven). Inventory: RM-FRM-*, RM-BTL-*, RM-SKL-*, RM-TRT-*,
RM-EFF-*, RM-STA-*, RM-CLS-*.

**How defaults are chosen.** Where the official documentation states a behaviour, Talewright implements that
behaviour (the inventory row is cited). Where it is silent (`08_runtime_systems.md` "not verified" list, items 5–12),
Talewright picks its own rule on game-design grounds, deliberately not the community-known RPG Maker internals, and
records the reasoning in `talewright/docs/battle_defaults.md`. Every such number is a field of `JRPGBattleRules`
(part of `JRPGSystemData`), so projects can retune it.

## Numbers and determinism
- Game-logic quantities are `int64`. Rates and chances are **basis points** (bp): 10 000 bp = 100 %.
  `mul_bp(x, bp) = x · bp / 10 000`, rounded half away from zero; chains of rates are applied one factor at a time
  in the order given below, rounding after each.
- Trait and effect values are stored as integers (bp for rates, plain integers for flat amounts; spec 02).
- Floats appear only inside formula evaluation (`Expression` computes in double precision). The formula result is
  converted once to an integer (round half away from zero; NaN/inf → 0 plus one warning per formula).
- IEEE-754 `+ − × ÷` and `sqrt` give identical results on all targets because the engine already disables FP
  contraction everywhere: `-ffp-contract=off` (`platform/linuxbsd/detect.py:204`, `platform/macos/detect.py:118`,
  `platform/android/detect.py:247`, `platform/windows/detect.py:739` for MinGW) and `/fp:strict` for MSVC
  (`platform/windows/detect.py:408`). Transcendental functions (`pow`, `exp`, `sin`, …) go through libm and differ
  between platforms, so formulas may not use them (allow-list below).
- Simulation time is counted in fixed 60 Hz steps (spec 03); nothing in battle math reads wall-clock time.

## RNG — `JRPGRandom` (core)
- Wraps the PCG32 already vendored by Godot (`thirdparty/misc/pcg.h:12-14`, Apache-2.0, listed in
  `thirdparty/README.md:834-837`): `pcg32_random_r` for raw draws, `pcg32_boundedrand_r` for unbiased ranges.
  Only integer outputs are used (`RandomPCG`'s float paths are what D-010 avoids, `core/math/random_pcg.h:109-135`).
- API: `next_u32()`, `range(lo, hi)` (inclusive), `chance_bp(bp)` (true with probability bp/10 000; `bp ≥ 10 000`
  always true, `≤ 0` never), `pick_weighted(weights)`, `shuffle(array)`.
- **Streams:** the game state owns independent generators — `battle`, `map_ai` (random NPC movement), `encounter`,
  `loot` (drops, item rewards) and `misc` (events' Control Variables random) — each with its own saved state
  (spec 06). A battle's golden log therefore does not depend on how many NPC steps happened before it.

## Parameters
- 8 regular parameters: max HP, max MP, attack, defense, magic attack, magic defense, agility, luck (RM-FRM-011).
- Value = (class curve[level] + equipment flat bonuses) → × each Param trait rate (traits of the same parameter
  multiply, as the Primer documents, RM-FRM-011) → × buff factor → clamp.
- **Buffs** (documented, RM-FRM-014): each level changes the parameter by 25 % of its base value, at most 2 levels
  each way (Primer), a buff and a debuff cancel each other, and Remove Buff does nothing on a debuffed parameter.
  Buff factor = `10 000 + level · buff_step_bp` with `buff_step_bp` 2 500 and `buff_max_levels` 2 (fields, so a
  project can change them).
- **Clamp** (`JRPGBattleRules`): max HP 1 … 9 999 999; max MP 0 … 9 999 999; other parameters **0** … 999 999.
  The lower bound 0 follows the documented editor ranges (actors and enemies: "others 0–999", RM-FRM-011); the upper
  bounds are Talewright limits above the editor ranges so traits and buffs have headroom.
- Ex-parameters (hit, evasion, critical, critical evasion, magic evasion, magic reflection, counter, HP/MP/TP
  regeneration) are sums of trait values in bp. Sp-parameters (target rate, guard effect, recovery effect,
  pharmacology, MP cost rate, TP charge rate, physical/magic/floor damage rate, EXP rate) are products in bp.
- Stat cache with dirty flags per source (equipment, states, buffs, class, level), recomputed lazily
  (rathena_techniques C8).
- EXP curve (Talewright's own, D-022): total EXP to reach level L = `round(base · (L−1)^(1.5 + growth/100) +
  extra · (L−1))` with class fields `base`, `growth`, `extra` (defaults 25, 30, 10). It is evaluated once per class in
  the editor and stored as an integer table in the class data, so runtime never evaluates `pow`; projects can edit
  the table directly.

## Formulas — `JRPGFormula` (core)
- Godot `Expression` (`core/math/expression.h:253-254`), parsed once per formula string with input names
  `a` (user), `b` (target), `v` (game variables) and executed with `const_calls_only = true`, which routes every
  method call through `Variant::call_const` (`core/math/expression.cpp:1451-1452`).
- `a`/`b` are read-only proxies (`JRPGFormulaBattler`, RefCounted) whose properties `atk`, `def`, `mat`, `mdf`,
  `agi`, `luk`, `mhp`, `mmp`, `hp`, `mp`, `tp`, `level` are served by `_get`, plus const methods
  `is_state_affected(id)` and `is_actor()`.
- `v` is passed as the game state's variables **Array** (shared, copy-on-write; index = variable ID, slot 0 unused),
  so the documented `v[n]` syntax (RM-FRM-001) works: `Expression` parses `[]` (`core/math/expression.cpp:866-884`)
  and executes it on the Array (`:1287-1306`). (`v.get(5)` is not an option: `Object` already binds `get`,
  `core/object/object.cpp:1840`, and debug builds refuse to bind a method a parent class has,
  `core/object/class_db.cpp:1910`.)
- Names resolve in this order inside `Expression`: built-in types, then Godot **utility functions**
  (`core/math/expression.cpp:485`), then identifiers (inputs, base-instance methods). Utility functions therefore
  shadow helpers of the same name, so Talewright never defines helpers named like a utility function.
- **Allow-list** (checked lexically before parsing; any other identifier is a parse error shown in the editor):
  inputs `a`, `b`, `v`; proxy members above; utility functions `abs`, `min`, `max`, `clamp`, `floor`, `ceil`,
  `round`, `sqrt`, `sign`; helpers on the base instance `iff(cond, x, y)` and `roll(lo, hi)` (inclusive integer
  from the `battle` stream; named to avoid Godot's `randi`/`randf`, which use the global RNG and are not allowed);
  literals `true`, `false`. Formulas remain trusted project content; the allow-list keeps them deterministic and
  away from engine calls such as `instance_from_id` or `str_to_var`.
- Integer rules: `7 / 2` is integer division (3) when both operands are integers, as in Godot; writing `7.0 / 2`
  gives 3.5. The editor's formula field shows the evaluated result for sample battlers.
- Errors: parse error → shown in the editor; runtime error, NaN or inf → 0 and one warning per formula.

## Damage pipeline (Talewright order and numbers, D-022)
1. `base = formula(a, b)` converted to an integer (0 when empty or failing). For HP/MP **damage** types a negative
   base becomes 0 (a damage skill never heals by accident); recovery types use `|base|`.
2. **Variance** (range documented: ±variance % of the value, RM-FRM-003): the offset is the average of two uniform
   draws in `[−variance %, +variance %]`, so results stay inside the documented range but cluster near the middle.
3. **Element:** a skill with one element uses the target's element rate for it (several Element Rate traits for one
   element multiply, RM-FRM-007). A normal attack with several Attack Element traits uses the **average** of the
   target's rates for those elements (undocumented, `08_runtime_systems.md:1484` item 9); none → 100 %.
4. **Critical:** chance `cri × (1 − cev)` as documented (RM-FRM-006), in bp; only skills with Critical Hits = Yes.
   Multiplier `crit_multiplier_bp` (default 20 000 = ×2).
5. **Category rate:** physical or magic damage rate of the target (sp-param); recovery uses recovery effect, and
   items also pharmacology of the user.
6. **Guard** (qualitative rule documented, RM-FRM-008): a guarding target's damage is multiplied by
   `10 000 − guard_reduction_bp × GRD / 10 000` (default `guard_reduction_bp` 4 000, i.e. −40 % at GRD 100 %), floored
   at 0; only for damage > 0.
7. Clamp the amount to 0 … 9 999 999; HP/MP drain is limited by the target's current value.
Zero, negative and overflow cases are doctested (final sweep).

## Hit and evasion (single roll, Talewright model)
The documentation fixes which rates apply (RM-FRM-004/005: hit rate and evasion for physical, magic evasion for
magic, success rate for all) and that a miss and an evasion are distinct outcomes with their own sounds; the
combination is undocumented (`08_runtime_systems.md:685`). Talewright uses an **additive** model with one roll:
- `accuracy` = success × user hit (physical) or success (magic), in bp; `avoid` = target evasion (physical) or magic
  evasion (magic), in bp.
- Draw `r` in `[0, 10 000)`: `r < accuracy − avoid` → hit; `r < accuracy` → evaded (Evasion/Magic Evasion SE);
  otherwise → miss (Miss SE). Certain Hit skips the roll.
- Documented trigger rules (RM-FRM-004): Certain Hit never triggers counter, reflection or substitute; physical
  attacks can trigger counter and substitute; magic attacks can trigger reflection and substitute. Reflection is
  rolled (chance = target's magic reflection) before the hit roll; counter and substitute follow RM-SKL-013 /
  RM-TRT-007.

## Turn order and battle systems
- Turn-based (speed formula undocumented, RM-BTL-006 / item 6): `speed = agi + mul_bp(agi, r) + skill speed +
  attack-speed traits` (the last only for normal attacks, as documented), with `r` uniform in
  `[−speed_spread_bp, +speed_spread_bp]` (default 1 000 = ±10 %).
- Ties: each battle draws a random **tie order** of all battlers once at battle start (`battle` stream, `shuffle`);
  equal speeds act in that order. Deterministic given the seed, and fair to both sides.
- TPB (documented: fill speed from the square root of agility, relative to the fastest battler, RM-BTL-007):
  `tpb_speed = isqrt(agi · 10 000)`; the gauge (0 … 1 000 000) gains `1 000 000 · tpb_speed / (fastest ·
  tpb_fill_steps)` per 60 Hz step (`tpb_fill_steps` default 240, i.e. the fastest battler fills in 4 s). Casting time
  and the rest of TPB are specified in epic 27.
- Escape (undocumented, item 5; Talewright's own): chance % = clamp(50 + 50 · (party_avg_agi − troop_avg_agi) /
  max(1, troop_avg_agi) + 15 · failed_attempts, 5, 95), computed in integers; guaranteed after a pre-emptive start;
  impossible when the battle forbids escape. Constants live in `JRPGBattleRules`.
- Victory rewards: EXP (× EXP rate; reserve members per system option), gold (doubled by the gold-doubling party
  ability), drops (`1/N` each from the `loot` stream; the drop-doubling party ability doubles the chance, documented
  in `08_runtime_systems.md:543,879`).

## TP
- Documented: TP costs 0–100, TP Gain per skill/item on a successful hit, Gain TP effect, TP Charge Rate scales TP
  gained from damage and actions, TP resets each battle to a random value (the Primer gives 0–24), Preserve TP keeps
  it (RM-FRM-016). Talewright: `tp_max` 100 (field), battle start TP `range(0, tp_start_max)` with `tp_start_max` 24.
- Undocumented (item 11), Talewright's rule: a battler that takes HP damage gains `2 + 30 × damage / max HP` TP
  (integer, × TP Charge Rate, capped at `tp_max`), so small hits still feed TP and big hits feed it more.
- TP Regeneration applies at turn end in battle and every 20 steps on the map (documented,
  `08_runtime_systems.md:773`).

## States
Restriction, priority, removal (battle end, restriction, auto-removal timing with min/max turns — random in range
from the `battle` stream, removal by damage chance, walking steps), stacking (re-adding refreshes the turn count),
death state ID 1, revive.
