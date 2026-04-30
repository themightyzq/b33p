# Resources

App-bundle assets baked into the b33p binary by `juce_add_plugin`.

| File | What it is |
| --- | --- |
| `icon_big.png` | 1024 × 1024 app icon source (`ICON_BIG`); drives Windows .ico + Linux freestanding PNG. |
| `icon_small.png` | 64 × 64 app icon (`ICON_SMALL`). |
| `Icon.icns` | macOS .icns containing the full Apple-recommended size set (16 / 32 / 64 / 128 / 256 / 512 / 1024 with @2x retina pairs). CMake's macOS post-build step copies this over JUCE's juceaide-generated default. |
| `make_icon.py` | Pillow-based generator that regenerates all three outputs from the bundled wordmark / palette constants. |

The icon is a placeholder — bold lowercase **b33p** in pale cyan on a
dark navy panel. Regenerate via:

```sh
python3 -m venv /tmp/iconvenv
/tmp/iconvenv/bin/pip install Pillow
/tmp/iconvenv/bin/python make_icon.py
```

The `.icns` step requires macOS's `iconutil`; on Linux / Windows the
script writes the PNGs and the iconset directory but skips the .icns.

Replace the assets with real artwork whenever a designer is available;
no code changes required as long as filenames stay the same.
