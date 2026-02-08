#!/bin/bash

# Build script for TrollSpeed
# This script mimics the GitHub Actions build process for local development

set -e

echo "🚀 Starting TrollSpeed build process..."

# Clean problematic directories
if [ -d "READM1E.md" ]; then
    echo "🧹 Removing problematic READM1E.md directory..."
    rm -rf READM1E.md
fi

# Check if Theos is installed
if [ -z "$THEOS" ]; then
    echo "❌ THEOS environment variable not set"
    echo "Please install Theos and set THEOS environment variable"
    echo "Example: export THEOS=/opt/theos"
    exit 1
fi

if [ ! -d "$THEOS" ]; then
    echo "❌ Theos directory not found at: $THEOS"
    exit 1
fi

echo "✅ Using Theos at: $THEOS"

# Check for required tools
if ! command -v make &> /dev/null; then
    echo "❌ make command not found"
    exit 1
fi

if ! command -v ldid &> /dev/null; then
    echo "⚠️  ldid not found - code signing may fail"
    echo "Install with: brew install ldid"
fi

# Clean previous builds
echo "🧹 Cleaning previous builds..."
make clean || true
rm -rf packages/
rm -rf Payload/

# Build the project
echo "🔨 Building project..."
make package FINALPACKAGE=1 FOR_RELEASE=1

# Check if build was successful
if [ ! -d ".theos/_/Applications/crepware.app" ]; then
    echo "❌ Build failed - crepware.app not found"
    echo "Available files in .theos/_/Applications/:"
    ls -la .theos/_/Applications/ || echo "Directory not found"
    exit 1
fi

echo "✅ Build successful!"

# Create .tipa file
echo "📦 Creating .tipa file..."

# Create directories
mkdir -p packages
mkdir -p Payload

# Copy app to Payload
cp -r .theos/_/Applications/crepware.app Payload/

# Create .tipa file
cd Payload
zip -r ../packages/crepware.tipa . -q
cd ..

# Clean up Payload directory
rm -rf Payload

# Verify .tipa file
if [ -f "packages/crepware.tipa" ]; then
    TIPA_SIZE=$(ls -lh packages/crepware.tipa | awk '{print $5}')
    echo "✅ .tipa file created successfully!"
    echo "📁 File: packages/crepware.tipa"
    echo "📏 Size: $TIPA_SIZE"
    
    # Generate checksum
    if command -v shasum &> /dev/null; then
        CHECKSUM=$(shasum -a 256 packages/crepware.tipa | awk '{print $1}')
        echo "🔐 SHA256: $CHECKSUM"
        echo "$CHECKSUM  crepware.tipa" > packages/checksum.txt
    fi
else
    echo "❌ Failed to create .tipa file"
    exit 1
fi

echo ""
echo "🎉 Build completed successfully!"
echo "📦 Your .tipa file is ready at: packages/crepware.tipa"
echo ""
echo "To install:"
echo "1. Copy crepware.tipa to your iOS device"
echo "2. Install using TrollStore or similar tool"
echo "3. Make sure you have the required entitlements"