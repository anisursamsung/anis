# anis

My personal Rofi-inspired application launcher and menu system for Hyprland.

**anis** is built from scratch using the native Hyprtoolkit to exclusively target Hyprland, providing a seamless and native experience.

## Features

- **Native Hyprland integration** - Built with Hyprtoolkit, no X11 compatibility layer
- **Multiple modes** - Application launcher, file browser, and custom menus
- **Dual views** - List and grid layouts for different workflows
- **Real-time search** - Filter items as you type
- **Keyboard-first navigation** - Full keyboard control
- **Icon support** - System icons, custom icons, and image thumbnails
- **Theme aware** - Automatically follows your system theme (light/dark)
- **Quiet mode** - `HT_QUIET=1` environment variable for clean script integration
- **Custom actions** - `-onclick` parameter with placeholders for maximum flexibility

## Screenshots

### Dark Theme

| List View | Grid View |
|-----------|-----------|
| ![Dark List Launcher](screenshots/dark_list.png) | ![Dark Grid Launcher](screenshots/dark_grid.png) |

### Light Theme

| List View | Grid View |
|-----------|-----------|
| ![Light List Launcher](screenshots/light_list.png) | ![Light Grid Launcher](screenshots/light_grid.png) |

---

## Installation

### Prerequisites
- Hyprland (Wayland compositor)
- Hyprtoolkit development libraries

### Build from source
```bash
git clone https://github.com/yourusername/anis.git
cd anis
chmod +x make.sh
./make.sh
```

### Post-build
Move the executable to your PATH for system-wide use:
```bash
cp build/anis ~/bin/  # or /usr/local/bin/
```

---

## Usage

```
anis [options]
```

### Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| `-mode` | `apps`, `files`, `options` | `apps` | Operation mode |
| `-view` | `list`, `grid` | `list` | Display style |
| `-source` | path or string | (varies) | Data source |
| `-onclick` | command string | - | Custom command to run on click |
| `-h`, `-help` | - | - | Show this help |

### Environment Variables

| Variable | Description |
|----------|-------------|
| `HT_QUIET=1` | Silence all debug output (essential for scripting) |
| `HT_NO_LOGS=1` | Alternative method to silence logs |

### Modes

| Mode | Description | Source Required | Default Output |
|------|-------------|-----------------|----------------|
| `apps` | Application launcher - scans `.desktop` files | Optional directory | None (launches apps) |
| `files` | File browser - shows images from directory | Directory path | File path |
| `options` | Custom menu - creates items from string | Menu definition | Selected item name |

### Views

| View | Description |
|------|-------------|
| `list` | Vertical list with icons and text |
| `grid` | Grid layout with larger previews (thumbnails for files) |

---

## OPTIONS MODE FORMATS

### Format 1: Simple List (Echo on Click)
```
"Item1,Item2,Item3"
```
Each item echoes its name when clicked.

### Format 2: With Icons (Echo on Click)
```
"icon.png,Name;icon2.png,Name2"
```
Shows icons, still echoes the name when clicked.

### Format 3: With Commands (Execute on Click)
```
"icon.png,Name,command;icon2.png,Name2,command2"
```
Executes the command when clicked. No output captured.

### Icon Resolution Priority
1. **File path** - If icon string is a valid file path (PNG, JPG, SVG)
2. **System icon theme** - If no file found, tries as system icon name (Papirus, Adwaita, etc.)
3. **Placeholder** - Falls back to generic icon

---

## -ONCLICK PARAMETER

Override default behavior with custom commands:

| Placeholder | Replaced With | Available In |
|-------------|---------------|--------------|
| `%f` | File path | Files mode only |
| `%n` | Display name | All modes |
| `%a` | Action/command | Options mode only |

### Priority Rules
- **`-onclick` ALWAYS takes priority** over default behavior
- If `-onclick` contains `%a`, the original command is inserted
- If `-onclick` has no `%a`, the original command is ignored

---

## COMPLETE EXAMPLES

### 1. APPS MODE (Launcher)

```bash
# Default launcher
anis

# Explicit apps mode
anis -mode apps

# Grid view
anis -mode apps -view grid

# Custom desktop directory
anis -mode apps -source /custom/applications

# No output capture needed - apps launch directly
```

---

### 2. FILES MODE (File Browser)

#### Basic Browsing
```bash
# List view (default)
anis -mode files -source ~/Pictures

# Grid view with thumbnails
anis -mode files -source ~/Wallpapers -view grid

# Relative path
anis -mode files -source ./downloads
```

#### Capturing File Path (in scripts)
```bash
#!/usr/bin/env bash
export HT_QUIET=1

# Capture selected file path
SELECTED_FILE=$(anis -mode files -source ~/Pictures -view grid)
echo "You selected: $SELECTED_FILE"

# Use the file path
if [[ -n "$SELECTED_FILE" ]]; then
    cp "$SELECTED_FILE" ~/backup/
fi
```

#### Custom Actions with -onclick
```bash
# Open with specific editor
anis -mode files -source ~/Documents -onclick "gedit %f"

# Copy to backup
anis -mode files -source ~/Downloads -onclick "cp %f ~/backup/"

# Process with custom script
anis -mode files -source ~/Pictures -onclick "~/scripts/process-image.sh %f"
```

---

### 3. OPTIONS MODE (Custom Menus)

#### Format 1: Simple List (Echo)

```bash
# Basic menu
anis -mode options -source "Save,Load,Quit"

# With grid view
anis -mode options -source "Yes,No,Cancel" -view grid
```

**Capturing selection:**
```bash
#!/usr/bin/env bash
export HT_QUIET=1

SELECTION=$(anis -mode options -source "Save,Load,Quit")
echo "User selected: $SELECTION"

case "$SELECTION" in
    "Save") save_file ;;
    "Load") load_file ;;
    "Quit") exit 0 ;;
esac
```

#### Format 2: With Icons (Echo)

```bash
# Menu with icons
anis -mode options -source "firefox.png,Firefox;chrome.png,Chrome;terminal.png,Terminal"

# Grid view with icons
anis -mode options -source "lock.png,Lock;suspend.png,Suspend" -view grid
```

**Capturing selection:**
```bash
#!/usr/bin/env bash
export HT_QUIET=1

BROWSER=$(anis -mode options -source "firefox.png,Firefox;chrome.png,Chrome")
echo "Selected browser: $BROWSER"
```

#### Format 3: With Commands (Execute)

```bash
# Direct command execution
anis -mode options -source "firefox.png,Firefox,/usr/bin/firefox;code.png,VS Code,/usr/bin/code"

# Power menu
anis -mode options -source \
"system-lock-screen,Lock,loginctl lock-session;\
system-suspend,Suspend,systemctl suspend;\
system-reboot,Reboot,systemctl reboot;\
system-shutdown,Power Off,systemctl poweroff" \
-view grid

# No capture needed - commands run directly
```

---

### 4. ADVANCED -ONCLICK EXAMPLES

#### Override Echo with Custom Command
```bash
# Instead of echoing, run custom command
anis -mode options -source "Apple,Banana,Orange" -onclick "echo You picked: %n"

# Output: "You picked: Apple" (when clicked)
```

#### Wrap Existing Command
```bash
# Add notification before running command
anis -mode options -source "firefox.png,Firefox,/usr/bin/firefox" \
  -onclick "notify-send 'Launching' '%n' && %a"

# Runs: notify-send 'Launching' 'Firefox' && /usr/bin/firefox
```

#### Log Selections
```bash
# Log all selections to file
anis -mode options -source "Save,Load,Quit" \
  -onclick "echo \"$(date): Selected %n\" >> ~/selection.log"
```

#### Complex File Operations
```bash
#!/usr/bin/env bash
export HT_QUIET=1

# First pick a file
FILE=$(anis -mode files -source ~/Documents -view grid)

if [[ -n "$FILE" ]]; then
    # Then pick action with custom onclick
    anis -mode options -source "edit.png,Edit;copy.png,Copy;delete.png,Delete" \
      -onclick "case '%n' in
                  'Edit')   gedit '$FILE' ;;
                  'Copy')   cp '$FILE' '$FILE.bak' ;;
                  'Delete') rm '$FILE' ;;
                esac"
fi
```

---

## 5. PRACTICAL SCRIPT EXAMPLES

### Power Menu Script
```bash
#!/usr/bin/env bash
# Save as: ~/bin/power.sh
# Bind to: $mainMod + Shift + P

export HT_QUIET=1
~/bin/anis -mode options -source \
"system-lock-screen,Lock,loginctl lock-session;\
system-suspend,Suspend,systemctl suspend;\
system-reboot,Reboot,systemctl reboot;\
system-shutdown,Power Off,systemctl poweroff" \
-view grid
```

### Theme Switcher
```bash
#!/usr/bin/env bash
# Save as: ~/bin/theme.sh

export HT_QUIET=1
SELECTION=$(~/bin/anis -mode options -source \
"preferences-system,Dark Mode;\
preferences-desktop-wallpaper,Light Mode")

case "$SELECTION" in
    "Dark Mode")
        gsettings set org.gnome.desktop.interface color-scheme 'prefer-dark'
        notify-send "Theme" "Dark mode enabled"
        ;;
    "Light Mode")
        gsettings set org.gnome.desktop.interface color-scheme 'prefer-light'
        notify-send "Theme" "Light mode enabled"
        ;;
esac
```

### Wallpaper Picker (2-Step)
```bash
#!/usr/bin/env bash
# Save as: ~/bin/wallpaper.sh

export HT_QUIET=1
CONFIG_DIR="$HOME/.config/theme"
WALLPAPER_DIR="$HOME/Pictures/Wallpapers"

# Step 1: Pick wallpaper
WALLPAPER=$(~/bin/anis -mode files -source "$WALLPAPER_DIR" -view grid)

if [[ -n "$WALLPAPER" ]]; then
    # Step 2: Pick fit type
    FIT=$(~/bin/anis -mode options -source "cover,Cover;fill,Fill;tile,Tile;contain,Contain" -view grid)
    
    if [[ -n "$FIT" ]]; then
        FIT=$(echo "$FIT" | tr '[:upper:]' '[:lower:]')
        "$CONFIG_DIR/wallpapers/set-wallpaper.sh" "$WALLPAPER" "$FIT"
        notify-send "Wallpaper Changed" "$(basename "$WALLPAPER") ($FIT)"
    fi
fi
```

### Quick Note Taker
```bash
#!/usr/bin/env bash
# Save as: ~/bin/note.sh

export HT_QUIET=1
NOTE=$(~/bin/anis -mode options -source "Work,Idea,Todo,Bug,Meeting")

if [[ -n "$NOTE" ]]; then
    echo "$(date): $NOTE" >> ~/notes.txt
    notify-send "Note Saved" "$NOTE"
fi
```

### File Action Menu
```bash
#!/usr/bin/env bash
# Save as: ~/bin/file-actions.sh

export HT_QUIET=1

# Pick a file
FILE=$(~/bin/anis -mode files -source ~/Downloads -view grid)

if [[ -n "$FILE" ]]; then
    # Choose action
    ACTION=$(~/bin/anis -mode options -source \
        "document-open,Open;edit-copy,Copy;edit-delete,Delete;document-send,Send")
    
    case "$ACTION" in
        "Open")   xdg-open "$FILE" ;;
        "Copy")   cp "$FILE" "$FILE.bak" && notify-send "Copied" "$(basename "$FILE")" ;;
        "Delete") rm "$FILE" && notify-send "Deleted" "$(basename "$FILE")" ;;
        "Send")   ~/scripts/email-file.sh "$FILE" ;;
    esac
fi
```

### Application Launcher with Categories
```bash
#!/usr/bin/env bash
# Save as: ~/bin/app-categories.sh

export HT_QUIET=1

CATEGORY=$(~/bin/anis -mode options -source \
"internet,Internet;office,Office;games,Games;development,Development;graphics,Graphics")

case "$CATEGORY" in
    "Internet")     ~/bin/anis -mode apps -source /usr/share/applications -onclick "grep -i 'web\|browser\|mail' %f" ;;
    "Office")       ~/bin/anis -mode apps -source /usr/share/applications -onclick "grep -i 'office\|word\|excel\|pdf' %f" ;;
    "Games")        ~/bin/anis -mode apps -source /usr/share/applications -onclick "grep -i 'game\|steam\|lutris' %f" ;;
    "Development")  ~/bin/anis -mode apps -source /usr/share/applications -onclick "grep -i 'code\|idea\|eclipse\|android' %f" ;;
    "Graphics")     ~/bin/anis -mode apps -source /usr/share/applications -onclick "grep -i 'gimp\|inkscape\|blender' %f" ;;
esac
```

---

## KEYBOARD CONTROLS

| Key | Action |
|-----|--------|
| `↑` `↓` `←` `→` | Navigate items |
| `Page Up` / `Page Down` | Jump 10 items |
| `Home` / `End` | Go to first/last item |
| `Enter` | Activate selected item |
| `Ctrl` + `Esc` | Toggle between list and grid view |
| `Esc` | Close application |

---

## TIPS & TRICKS

### Quiet Mode for Scripts
Always use `export HT_QUIET=1` at the beginning of scripts to get clean output:
```bash
#!/usr/bin/env bash
export HT_QUIET=1
SELECTION=$(anis -mode options -source "A,B,C")
# Only "A", "B", or "C" is captured, no debug messages
```

### Combining Modes
You can chain anis calls for complex workflows:
```bash
FILE=$(anis -mode files -source ~/Pictures)
[ -n "$FILE" ] && ACTION=$(anis -mode options -source "Edit,Print,Delete")
# ... process both selections
```

### Custom Icons
Place your icons in `~/.local/share/icons/` or use absolute paths:
```bash
anis -mode options -source "/home/user/icons/custom.png,My App,/usr/bin/myapp"
```

### System Theme Icons
Use icon names without paths for system theme icons:
```bash
anis -mode options -source "firefox,Firefox,/usr/bin/firefox"
```

---

## TROUBLESHOOTING

### Debug output in scripts
**Problem:** Script captures debug messages instead of selection
**Solution:** Use `export HT_QUIET=1` before running anis

### SVG icons not showing
**Problem:** SVG files fail to load
**Solution:** anis automatically converts SVGs to PNGs in `/tmp/anis_cache/`

### Files mode opens files instead of returning path
**Problem:** Clicking a file opens it instead of returning the path
**Solution:** This is the default behavior. Use `-onclick` for custom actions or capture the output in scripts

---

## ACKNOWLEDGMENTS

- Built with [Hyprtoolkit](https://github.com/hyprwm/hyprtoolkit)
- Inspired by [Rofi](https://github.com/davatorium/rofi)

---

## LICENSE

MIT License - feel free to use and modify!
