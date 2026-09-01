# Existing system-menu styling

`PolishSystemMenu.py` edits the eleven existing menu Widget Blueprints under
`/Game/ProjectHunter/UI/Widgets/Menus/`. It does not create replacement widgets or
change gameplay owners, Blueprint graphs, input settings, or the in-game HUD.

The palette and cell dimensions are grouped at the top of the script. Colors are
written as sRGB hex and converted to Unreal linear colors. The existing Rajdhani
font and texture assets are not modified. Panel brushes use native rounded boxes
with thin outlines; all original widget identities are retained.

The menu's four existing vital instances receive the visual colors and bar-height
settings. Their resource types, attributes, event bindings, interpolation, and
visibility policies stay unchanged. This does not restyle `WBP_BaseProgressBar`
or the independently sized Stamina widget in `WBP_HunterHUD`.

## Explicit application

Build `ALS_ProjectHunterEditor` first. Save your own menu edits before applying;
the script refuses to run over dirty menu packages. From Unreal's Python console:

```python
import sys, unreal
sys.path.insert(0, unreal.Paths.project_dir() + "Tools/Editor")
import PolishSystemMenu
PolishSystemMenu.main(["--apply"])
```

Verified backups, before/after contracts, and a result file go into a new dated
folder under `Saved/MenuPolish/`. An explicit `--output` directory must not already
exist. The script compiles every target and refuses to save if compilation,
protected Blueprint contracts, or the menu's interaction checks fail. Saves are
sequential; the backup manifest lists originals if an individual save fails.

For unattended use, close other editors of this project and run a wrapper script
with `-ExecutePythonScript` and `-RenderOffscreen`. Do not use
`-run=pythonscript`: that commandlet does not initialize Slate. Omit `--apply` only
inside a disposable editor process; this exercises changes in memory and does not
save them, but it intentionally leaves that process's loaded packages modified.

## Verification and previews

The editor-only `PHMenuPreviewEditorLibrary` can inspect contracts, validate
existing tab/selection/binding paths, and render the actual menu at viewport DPI:

```python
menu_out = unreal.Paths.project_saved_dir() + "MenuPreview/"
unreal.PHMenuPreviewEditorLibrary.inspect_menu_contracts(menu_out + "Contracts.json")
unreal.PHMenuPreviewEditorLibrary.validate_system_menu()
unreal.PHMenuPreviewEditorLibrary.render_system_menu(menu_out + "Menu.png", 1920, 1080)
```

Rendering requires a real RHI. It uses a transient character's normal stats and
inventory owners, with values from the existing `DA_BaseStats`; no live save or
player is changed. The PNG is UI-only: the transparent center normally reveals the
world character, and no stand-in character or camera is invented for the image.
The sibling JSON records viewport DPI, slot counts, and the source of its values.

Automation: `ProjectHunter.Menu.Presentation.AuthoredBindingsTabsAndInventorySelection`.
Rendered output and native automation do not certify actual mouse gestures or
camera framing in a live game session; those still need a hands-on menu pass.

## Panel textures

`MakeSystemPanelTextures.py` generates `T_SystemPanel_Fill` and `T_SystemPanel_Frame`. It runs on
plain Python with Pillow and needs no editor:

```bash
python Tools/Editor/MakeSystemPanelTextures.py
```

Two rules it exists to enforce, both from DECISION-PH-20260831-04:

- **256x256, power of two.** The originals were 160x160. Unreal cannot mip a non-power-of-two
  texture and handles it as a special case, which sheared the 9-slice corners. `PolishSystemMenu.py`
  refuses to build a panel brush from a texture that is not this size, so a stale import fails
  loudly rather than rendering bent. Never compensate for a bad texture size with brush margins.
- **No corner accents baked in.** They come from the authored `M_SquareFlicker` material, which
  animates them and is reusable by any menu.

The script prints the `SlateBrush` BOX margin that matches its geometry; `SYSTEM_MARGIN` must equal
it. Body translucency is baked into the Fill texture's alpha rather than applied as a brush tint,
because a tint alpha fades the white double line along with the body.

`PolishSystemMenu.py` reimports both PNGs at apply time and pins LOD group UI, UserInterface2D
compression, NoMipmaps and sRGB, then saves them with the Blueprints. Both are backed up first.
