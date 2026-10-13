# Spec 04 — RNG, parameters, traits, formulas, battle math

Decisions: D-009 (formulas), D-010 (RNG), D-019 (battle systems), D-022 (undocumented numbers are Talewright-defined,
documented and data-driven). Inventory: RM-FRM-*, RM-BTL-*, RM-SKL-*, RM-TRT-*, RM-EFF-*, RM-STA-*, RM-CLS-*.

## RNG — `JRPGRandom` (core)
PCG32 (O'Neill), 64-bit state + increment, integer-only API: `next_u32()`, `range(lo, hi)` (inclusive, rejection
sampling, no modulo bias), `chance(numerator, denominator)`, `percent(p)` (p in 0..100 as int), `pick(weights)`.
State is part of `JRPGGameState` and serialized. Floats are never drawn from the RNG for game logic (deterministic
across platforms; Godot's RandomPCG float paths differ by CPU, `core/math/random_pcg.h:109-135`).

## Parameters
- 8 regular parameters: max HP, max MP, attack, defense, magic attack, magic defense, agility, luck (RM-FRM-011).
- Value = class curve[level] + equipment params + param traits (rate, multiplicative) + buffs (+25%/level, ±2 levels
  default, configurable) → clamped to limits (HP 1..9 999 999, MP 0..9 999 999, others 1..999 999 by default;
  limits live in `JRPGSystemData`, D-022).
- Ex-parameters (rates, sum of traits): hit, evasion, critical, critical evasion, magic evasion, magic reflection,
  counter, HP/MP/TP regeneration. Sp-parameters (products of traits): target rate, guard effect, recovery effect,
  pharmacology, MP cost rate, TP charge rate, physical damage rate, magic damage rate, floor damage rate, EXP rate.
- Stat cache with dirty flags per source (equipment, states, buffs, class, level) — recomputed lazily
  (rathena_techniques C8).
- EXP curve (D-022, Talewright's own curve — not derived from any RPG Maker implementation): total EXP needed to
  reach level L is `round(base · (L−1)^(1.5 + growth/100) + extra · (L−1))` with class parameters `base`, `growth`,
  `extra` (defaults 25, 30, 10). Editors show the resulting table; games can override it with an explicit table.

## Formulas — `JRPGFormula` (core)
- Godot `Expression` (`core/math/expression.h:253`) parsed once per formula string with inputs `a` (user), `b`
  (target), `v` (game variables accessor) and a base instance exposing helpers: `iff(cond, x, y)`, `min`, `max`,
  `clamp`, `floor`, `ceil`, `round`, `rand(lo, hi)` (uses `JRPGRandom`).
- `a`/`b` are read-only proxy objects (`JRPGFormulaBattler`) exposing params (`atk`, `def`, `mat`, `mdf`, `agi`, `luk`,
  `mhp`, `mmp`, `hp`, `mp`, `tp`, `level`), `is_state_affected(id)`, `is_actor()`; `v` exposes `get(id)` and `[]`
  is not available in Expression, so `v.get(5)`.
- Safety (D-009): before parsing, a lexical check rejects identifiers outside the allow-list (inputs, helpers, proxy
  members, numeric literals, operators), which blocks utility functions like `instance_from_id` or `str_to_var`.
  Formulas remain trusted project content.
- Errors: parse error → editor shows it; runtime error / NaN / inf → result 0 and one warning per formula.
- Result → integer by rounding toward zero after all multipliers (see order below).

## Damage pipeline (Talewright order, D-022)
1. `base = formula(a, b)` (0 if none or error).
2. Element rate: attack-element skills use the attacker's attack elements (max of the target's rates); otherwise
   the skill's element rate on the target (traits multiply).
3. Physical/magic damage rate (target sp-param); recovery uses recovery effect (+ pharmacology for items).
4. Critical: if `chance(cri − cev)` → ×3 (multiplier in `JRPGSystemData`, default 3, D-022).
5. Variance: `±variance%` uniform integer from the RNG.
6. Guard: if the target guards and damage > 0 → ÷ (2 × guard effect).
7. Round toward zero; clamp to ±9 999 999; HP/MP drain limited by the target's current value.
Zero, negative (healing) and overflow cases are tested explicitly (final sweep).

## Hit and evasion
- Certain hit: always hits. Physical: hit if `chance(user.hit × skill.success)` and not `chance(target.eva)`.
  Magic: `chance(skill.success)` and not `chance(target.mev)`; magic reflection before applying.
- Counterattack (physical) and substitute (low-HP ally) per the inventory items RM-SKL-013, RM-TRT-007.

## Turn order and battle systems
- Turn-based: action speed = `agi + rand(0, agi/4) + skill speed + attack-speed traits`; ties broken by battler
  index (actors before enemies, then lower index) — deterministic.
- TPB (active/wait): gauge fills by `agi`-relative rate; cast time from skill speed (epic 27).
- Escape (D-022, Talewright's own rule): chance% = clamp(50 + 50 · (party_avg_agi − troop_avg_agi) / max(1,
  troop_avg_agi) + 15 · failed_attempts, 5, 95); guaranteed on pre-emptive; impossible when the battle forbids
  escape. Constants live in `JRPGSystemData`.
- Victory rewards: EXP (× EXP rate, reserve members per system option), gold, drops (`1/denominator` each, item-find
  party ability doubles).

## States
Restriction, priority, removal (battle end, restriction, auto-removal timing with min/max turns — random in range
via RNG, damage chance, walking steps), stacking (re-adding refreshes the turn count), death state id 1, revive.
