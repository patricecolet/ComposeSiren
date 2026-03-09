# Correction du problème Rosetta

## Problème identifié

Le DMG demande Rosetta sur macOS Sonoma alors que les binaires sont universels. Le diagnostic a révélé que :

1. ✅ **Distribution.xml** : Format correct (`hostArchitectures="x86_64 arm64"`)
2. ❌ **PackageInfo** : Les 4 sous-packages n'avaient **pas** l'attribut `hostArchitectures`

## Solution

Le script `fix_pkginfo_after_build.sh` :
1. Trouve le PKG principal (dans `_CPack_Packages` ou `dmgContent`)
2. Extrait le PKG
3. Ajoute `hostArchitectures="x86_64 arm64"` dans les 4 PackageInfo manquants
4. Reconstruit le PKG avec `xar`
5. Recrée le DMG avec le PKG corrigé

## Utilisation

### Option 1 : Correction manuelle d'un package existant

```bash
chmod +x scripts/fix_pkginfo_after_build.sh
./scripts/fix_pkginfo_after_build.sh build/Packaging/ComposeSiren_Orchestra_Installer_artefacts
```

### Option 2 : Test complet (correction + diagnostic)

```bash
chmod +x scripts/test_fix_and_diagnose.sh
./scripts/test_fix_and_diagnose.sh
```

### Option 3 : Diagnostic uniquement

```bash
chmod +x scripts/diagnose_rosetta_issue.sh
./scripts/diagnose_rosetta_issue.sh
```

## Vérification

Après correction, le diagnostic devrait afficher :
- ✅ Distribution.xml: Format correct (espaces)
- ✅ PackageInfo: **4/4 avec format correct** (au lieu de 0/4)

Le DMG ne devrait plus demander Rosetta.

## Intégration dans le build

Le script `fix_pkginfo_after_build.sh` est automatiquement appelé dans `MakePackage.cmake` **avant** la création du DMG, donc les futurs builds devraient être corrects automatiquement.

