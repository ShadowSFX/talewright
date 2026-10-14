# Golden images

Images the checks compare new renders with, pixel for pixel (`talewright/tools/img_compare.py`: per-channel tolerance
2/255, at most 0.1 % of pixels differing).

| Image | Made by | What it shows |
|---|---|---|
| `jrpg_map_strokes.png` | `talewright/tools/jrpg_bridge_checks.py` (`jrpg_map_capture`) | A 10 x 8 map drawn with each map canvas tool: a grass fill, a rectangle, an ellipse with two cells erased, a pencil path (task 8.4.2), and a column of left-half shadows (task 8.4.3.1; regions are not drawn on the map) |

Goldens are regenerated only on purpose, when the rendering is meant to change:
`python talewright/tools/jrpg_bridge_checks.py --update-goldens`, then look at the new image and say why in the commit.
They come from the editor's Forward+ renderer on the development machine; another renderer or GPU may differ.
