# Notes sur le rebuild du package macOS

## ✅ Corrections appliquées aux templates

Les templates ont été corrigés pour utiliser le format **avec espaces** pour `hostArchitectures` :

- ✅ `CPack.distribution.dist.in` : `hostArchitectures="x86_64 arm64"` (espaces)
- ✅ `Distribution.xml.in` : `hostArchitectures="x86_64 arm64"` (espaces)

## Format correct

Le format **avec espaces** (`x86_64 arm64`) est celui recommandé par Apple et correctement reconnu par macOS Sonoma.

Le format avec virgules (`x86_64,arm64`) peut ne pas être reconnu par macOS Sonoma et causer la demande de Rosetta.

## Rebuild

Lors d'un rebuild complet, CPack utilisera les templates corrigés et générera automatiquement :

1. **Distribution.xml** avec `hostArchitectures="x86_64 arm64"` (format correct)
2. **PackageInfo** : pkgbuild peut générer avec des virgules, mais c'est moins critique car le Distribution.xml principal est lu en premier

## Vérification après rebuild

Pour vérifier que le package généré est correct :

```bash
# Extraire le package
xar -xf package.pkg -C /tmp/check

# Vérifier le format dans Distribution.xml
grep "hostArchitectures" /tmp/check/Distribution
# Devrait afficher : hostArchitectures="x86_64 arm64" (avec espaces)

# Valider le XML
xmllint --noout /tmp/check/Distribution
# Devrait être valide sans erreur
```

## Note importante

Le script `fix_pkg.sh` est une solution de contournement pour corriger des packages existants. 
**Un rebuild complet est préférable** car il génère le package avec le bon format dès le départ.

