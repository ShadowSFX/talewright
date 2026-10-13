# Upstream test baseline (task 1.3)

- Tree: `talewright` @ 4.7.2-stable (ed1daf0bf0), no Talewright modules yet.
- Binary: `bin/godot.windows.editor.x86_64.exe` built with `python talewright/tools/build.py editor`
  (`dev_mode=yes`, i.e. `tests=yes`, `d3d12=no`), 2026-10-13.
- Command: `bin/godot.windows.editor.x86_64.console.exe --headless --test` (Linux CI also uses `--headless --test`,
  `.github/workflows/linux_builds.yml:237-241`; Windows CI omits `--headless`, `.github/workflows/windows_builds.yml:137-141`).

| Run | Test cases | Passed | Failed | Skipped | Assertions | Time |
|---|---|---|---|---|---|---|
| 1 | 1380 | 1380 | 0 | 3 | 419763 passed, 0 failed | 27.5 s |
| 2 | 1380 | 1380 | 0 | 3 | 419763 passed, 0 failed | ~27 s |

Runs 1 and 2 produce identical summaries (`diff` of the doctest summary lines is empty).

- Full list of upstream test case names (including the 3 skipped): `plan/research/baseline_test_cases.txt`
  (1383 entries, from `--test --list-test-cases --no-skip`). Regression rule: after every epic, the upstream
  suite must still pass with 0 failures and every name in that list must still exist.
- Raw outputs are kept locally in `talewright/build_logs/baseline_tests_run{1,2}.txt` (git-ignored).
