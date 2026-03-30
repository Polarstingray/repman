#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$PROJECT_DIR/out"

# 1. Compile
echo "Building project..."
make -C "$PROJECT_DIR" clean
make -C "$PROJECT_DIR"

# 2. Create output directory structure
echo "Creating output directory structure..."
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR/bin"
mkdir -p "$OUT_DIR/data/build"
mkdir -p "$OUT_DIR/data/cli"
mkdir -p "$OUT_DIR/data/sig/index"
mkdir -p "$OUT_DIR/data/index"
mkdir -p "$OUT_DIR/data/cache"
mkdir -p "$OUT_DIR/data/packages"
mkdir -p "$OUT_DIR/data/tmp"

# 3. Copy build artifacts
echo "Copying build artifacts..."
cp "$PROJECT_DIR/build/"*.o "$OUT_DIR/data/build/"
cp "$PROJECT_DIR/build/librepman.so" "$OUT_DIR/data/build/"

# 4. Copy CLI sources
echo "Copying CLI files..."
cp "$PROJECT_DIR/cli/repcli.py" "$OUT_DIR/data/cli/"
cp "$PROJECT_DIR/cli/repman.py" "$OUT_DIR/data/cli/"

# 5. Copy config
echo "Setting up configuration..."
if [ -f "$PROJECT_DIR/config.env" ]; then
    cp "$PROJECT_DIR/config.env" "$OUT_DIR/data/config.env"
else
    cp "$PROJECT_DIR/config.env.example" "$OUT_DIR/data/config.env"
fi

# 6. Copy public key
if [ -f "$PROJECT_DIR/sig/ci.pub" ]; then
    cp "$PROJECT_DIR/sig/ci.pub" "$OUT_DIR/data/sig/ci.pub"
elif [ -f "$PROJECT_DIR/ci.pub" ]; then
    cp "$PROJECT_DIR/ci.pub" "$OUT_DIR/data/sig/ci.pub"
else
    echo "Warning: No public key found (sig/ci.pub or ci.pub)."
fi

# 7. Set up Python venv
echo "Setting up Python virtual environment..."
python3 -m venv "$OUT_DIR/data/cli/venv"
"$OUT_DIR/data/cli/venv/bin/pip" install python-dotenv --quiet

# 8. Write entrypoint script
cat > "$OUT_DIR/bin/repman" << 'EOF'
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DATA_DIR="$SCRIPT_DIR/../data"
exec "$DATA_DIR/cli/venv/bin/python3" "$DATA_DIR/cli/repcli.py" "$@"
EOF
chmod +x "$OUT_DIR/bin/repman"

echo "Done! Run: $OUT_DIR/bin/repman"
