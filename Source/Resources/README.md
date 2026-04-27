# Resources

App-bundle assets baked into the b33p binary by `juce_add_gui_app`.

| File | What it is |
| --- | --- |
| `icon_big.png` | 1024 × 1024 placeholder app icon (`ICON_BIG`) |
| `icon_small.png` | 64 × 64 placeholder app icon (`ICON_SMALL`) |
| `make_icon.py` | One-shot Pillow script that regenerates both PNGs |

The icon is a placeholder — bold lowercase **b33p** in pale cyan on a
dark navy panel. Regenerate via:

```sh
python3 -m venv /tmp/iconvenv
/tmp/iconvenv/bin/pip install Pillow
/tmp/iconvenv/bin/python make_icon.py
```

Replace the PNGs with real artwork whenever a designer is available;
no code changes required as long as filenames stay the same.
