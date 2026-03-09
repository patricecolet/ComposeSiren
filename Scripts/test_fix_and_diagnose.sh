#!/bin/bash
# Script pour tester la correction et le diagnostic

PACKAGE_DIR="${1:-build/Packaging/ComposeSiren_Orchestra_Installer_artefacts}"

echo "🔧 Étape 1: Correction des PackageInfo"
echo "========================================"
./scripts/fix_pkginfo_after_build.sh "$PACKAGE_DIR"

echo ""
echo ""
echo "🔍 Étape 2: Diagnostic du DMG"
echo "========================================"
./scripts/diagnose_rosetta_issue.sh

echo ""
echo ""
echo "✅ Test terminé"

