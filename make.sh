#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "==> [1/5] Configuring CMake (incremental)..."
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

echo "==> [2/5] Building hlmenu with $(nproc) core(s)..."
cmake --build build -j"$(nproc)"

echo "==> [3/5] Installing binary to ~/.local/bin..."
mkdir -p "$HOME/.local/bin"
cp -f build/hlmenu "$HOME/.local/bin/hlmenu"
chmod +x "$HOME/.local/bin/hlmenu"

echo "==> [4/5] Installing desktop entry and assets..."
mkdir -p "$HOME/.local/share/applications"
cp -f resources/hlmenu.desktop "$HOME/.local/share/applications/hlmenu.desktop"

mkdir -p "$HOME/.local/share/hlmenu"
if [ -f "assets/placeholderappicon.png" ]; then
    cp -f assets/placeholderappicon.png "$HOME/.local/share/hlmenu/placeholderappicon.png"
fi

mkdir -p "$HOME/.config/hlmenu"
if [ ! -f "$HOME/.config/hlmenu/hlmenu.conf" ]; then
    if [ -f "resources/hlmenu.conf" ]; then
        cp resources/hlmenu.conf "$HOME/.config/hlmenu/hlmenu.conf"
    fi
fi
if [ ! -f "$HOME/.config/hlmenu/custom.conf" ]; then
    if [ -f "resources/custom.conf" ]; then
        cp resources/custom.conf "$HOME/.config/hlmenu/custom.conf"
    fi
fi

echo "==> [5/5] Build and installation completed successfully!"

# If arguments were provided or asked to run, execute the binary
if [ "$1" == "--no-run" ]; then
    exit 0
elif [ "$1" == "run" ] || [ "$1" == "--run" ]; then
    shift
    echo "==> Running hlmenu $@..."
    exec "$HOME/.local/bin/hlmenu" "$@"
else
    echo "==> Running hlmenu..."
    exec "$HOME/.local/bin/hlmenu" "$@"
fi
