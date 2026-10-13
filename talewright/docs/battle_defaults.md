# Battle defaults and why they were chosen

Talewright's battle rules come from two places:

- **Documented behavior**: where the official RPG Maker MZ documentation describes how something works, Talewright
  behaves the same way, so designers' expectations carry over. The inventory row that documents it is given below.
- **Talewright rules**: where the documentation is silent, Talewright does not copy anyone's engine internals
  (`plan/DECISIONS.md` D-020, D-022). It picks its own rule for a stated design reason.

Every number here is a field of the project's battle rules (`JRPGBattleRules` in the system data), so a project can
retune it. The engine-side specification is `plan/specs/04_formulas_battle.md`. Rates are written in percent here;
the engine stores them as basis points (100 % = 10 000).

| Topic | Source | Default | Field(s) | Reason |
|---|---|---|---|---|
| Damage formula language (`a.atk * 4 - b.def * 2`, `v[n]`) | documented (RM-FRM-001) | — | per skill/item | Same authoring model; evaluated by Godot's `Expression` with an allow-list |
| Variance range | documented (RM-FRM-003) | skill field, 0–100 % | per skill/item | Range as documented. **Shape (Talewright):** the offset is the average of two uniform draws, so most hits land near the expected value and extremes are rare, which makes balance testing less noisy |
| Order of damage steps | Talewright | formula → variance → element → critical → physical/magic rate → guard → clamp | — | Variance is applied to the raw value so every later multiplier scales an already-varied number, so a critical or a weakness always feels like a fixed multiple of a "normal" hit |
| Negative result of a damage formula | Talewright | treated as 0 | — | A typo in a formula (e.g. `b.def` larger than `a.atk`) should make a weak attack, not silently heal the target |
| Element rate stacking for one element | documented (RM-FRM-007) | rates multiply | traits | As documented |
| Several attack elements on one normal attack | Talewright | average of the target's rates | — | A weapon with two elements is partly effective against a target weak to one and resistant to the other; taking the best rate would make multi-element weapons strictly dominant |
| Critical chance | documented (RM-FRM-006) | user crit × (1 − target crit evasion) | traits | As documented |
| Critical multiplier | Talewright | ×2 | `crit_multiplier` | Doubling is easy to read in damage numbers and keeps a critical decisive without letting one lucky hit end a boss fight |
| Guard | documented qualitatively (RM-FRM-008: fixed reduction, scaled by Guard Effect) | −40 % at Guard Effect 100 % | `guard_reduction` | Guarding should be clearly worthwhile without halving everything; Guard Effect 250 % reaches full immunity, a deliberate design ceiling |
| Which rates decide hit/miss | documented (RM-FRM-004, RM-FRM-005) | physical: success, hit rate, evasion; magic: success, magic evasion | traits | As documented, including which outcomes can trigger counter, reflection and substitute |
| How those rates combine | Talewright | additive, one roll: hit if roll < accuracy − evasion; evaded if roll < accuracy; otherwise miss | — | Designers can read evasion as "points of accuracy removed" (20 % evasion against 95 % accuracy hits 75 % of the time); one roll still separates "missed" from "dodged" for the different sounds |
| Turn-based action speed | documented parts (RM-BTL-006): agility, skill Speed, Attack Speed for normal attacks | agility ± 10 % random | `speed_spread` | Symmetric spread keeps the average order equal to the agility order while letting near-equal battlers swap |
| Speed ties | Talewright | random tie order drawn once per battle | — | Neither side wins ties by rule; deterministic for replays because it comes from the battle's seeded random stream |
| TPB fill speed | documented (RM-BTL-007: from √agility, relative to the fastest battler) | fastest battler fills in 4 s | `tpb_fill_steps` (240 steps at 60 Hz) | Square-root scaling as documented; 4 s is a readable pace for a first battle |
| Escape chance | Talewright | 50 % + agility difference, +15 % per failed try, clamped 5–95 % | `escape_*` | Fast parties escape more easily, repeated tries eventually succeed, and escape is never certain or impossible unless the battle says so |
| Buffs and debuffs | documented (RM-FRM-014) | ±25 % of base per level, up to 2 levels, buff and debuff cancel | `buff_step`, `buff_max_levels` | As documented |
| Parameter limits | lower bounds documented (RM-FRM-011: "others 0–999") | Max HP ≥ 1, others ≥ 0; upper limits 9 999 999 (HP/MP) and 999 999 | `param_limits` | Upper limits leave headroom above the editor ranges for traits and buffs |
| TP at battle start | documented (RM-FRM-016, Primer: random 0–24) | random 0–24 | `tp_start_max` | As documented |
| TP maximum | implied by documented ranges (RM-FRM-016) | 100 | `tp_max` | As implied |
| TP from taking damage | Talewright | 2 + 30 × damage / max HP, × TP Charge Rate | `tp_on_damage_*` | Every hit gives a little TP, so chip damage still builds toward a TP skill, and a heavy hit gives a meaningful boost |
| EXP curve | Talewright | `base · (L−1)^(1.5 + growth/100) + extra · (L−1)`, defaults 25 / 30 / 10 | class fields | A smooth power curve with one "growth" knob; the editor stores the resulting table, which projects can edit freely |
| Steps between random encounters | average documented (RM-MPR-008: the map's encounter steps are an average) | uniform between half and one and a half times the map value | per map | Keeps the documented average while avoiding encounters on consecutive steps; a flat distribution is easy to reason about |
| Drops | documented (`08_runtime_systems.md:879`: 1/N per drop, doubled by the drop-doubling party ability) | as documented | per enemy | As documented |

Values were chosen from first principles for readability and balance, not tuned to match any other engine; the
"not verified" list in `plan/research/rpgmaker/08_runtime_systems.md` shows which numbers the documentation leaves
open.
