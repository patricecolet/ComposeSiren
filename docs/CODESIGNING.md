# Signature et Notarisation macOS

Guide complet pour signer et notariser ComposeSiren pour une distribution professionnelle sur macOS.

---

## 📋 Table des matières

- [Pourquoi signer et notariser ?](#pourquoi-signer-et-notariser-)
- [Workaround temporaire (utilisateurs finaux)](#workaround-temporaire-utilisateurs-finaux)
- [Configuration professionnelle](#configuration-professionnelle)
- [Script d'automatisation](#script-dautomatisation)
- [Vérification](#vérification)
- [Dépannage](#dépannage)

---

## 🔐 Pourquoi signer et notariser ?

macOS Gatekeeper bloque l'installation des DMG non signés, affichant :

> ⚠️ "Impossible de vérifier si le logiciel contient un malware"

### Comparaison

| Aspect | Sans signature | Avec signature & notarisation |
|--------|----------------|------------------------------|
| **Coût** | Gratuit | $99/an (Apple Developer) |
| **Expérience utilisateur** | ⚠️ Avertissements, manipulation manuelle | ✅ Installation fluide |
| **Professionnalisme** | ❌ Amateur | ✅ Professionnel |
| **Temps de build** | Rapide | +5-30 min (notarisation) |
| **Distribution** | Via lien direct uniquement | Mac App Store possible aussi |

---

## 🆘 Workaround temporaire (utilisateurs finaux)

Si vous distribuez un DMG non signé, vos utilisateurs doivent :

### Méthode 1 : Clic droit

1. **Télécharger le DMG** normalement
2. **Clic droit** (ou Ctrl+clic) sur le fichier DMG
3. Choisir **"Ouvrir"** dans le menu contextuel
4. Confirmer l'ouverture dans la boîte de dialogue

### Méthode 2 : Terminal

```bash
xattr -d com.apple.quarantine /chemin/vers/ComposeSiren-v1.5.0-custom-mix-macOS.dmg
```

⚠️ **Cette solution n'est pas professionnelle** et nécessite que chaque utilisateur contourne manuellement Gatekeeper.

---

## 🏢 Configuration professionnelle

### Prérequis

1. **Compte Apple Developer** ($99/an)
   - Inscrivez-vous sur https://developer.apple.com/programs/

2. **Certificats de développeur**
   - Developer ID Application (pour signer les .app)
   - Developer ID Installer (pour signer les .pkg)

---

## 📝 Étape 1 : Obtenir les certificats

### 1.1 Créer une demande de certificat

1. Ouvrez **Keychain Access** (Trousseaux d'accès)
2. Menu **Keychain Access > Certificate Assistant > Request a Certificate from a Certificate Authority**
3. Entrez votre email Apple Developer ID
4. Choisissez "Saved to disk"
5. Sauvegardez le fichier `.certSigningRequest`

### 1.2 Télécharger les certificats

1. Allez sur https://developer.apple.com/account/resources/certificates/list
2. Créez deux certificats :
   - **Developer ID Application** (pour signer le code)
   - **Developer ID Installer** (pour signer les installers)
3. Téléchargez et double-cliquez pour les installer dans votre Keychain

### 1.3 Vérifier les certificats installés

```bash
# Lister les certificats Developer ID
security find-identity -v -p codesigning

# Vous devriez voir :
# 1) XXXXXXXXXX "Developer ID Application: Votre Nom (XXXXXXXXXX)"
# 2) YYYYYYYYYY "Developer ID Installer: Votre Nom (XXXXXXXXXX)"
```

**⚠️ Notez les noms complets entre guillemets**, vous en aurez besoin.

---

## 🔑 Étape 2 : Configurer la notarisation

### 2.1 Créer un App-Specific Password

1. Allez sur https://appleid.apple.com
2. Section **"Sign-In and Security"** > **"App-Specific Passwords"**
3. Générez un nouveau mot de passe (ex: "ComposeSiren-Notarization")
4. **⚠️ Copiez ce mot de passe**, il ne sera affiché qu'une fois

### 2.2 Stocker les credentials

```bash
xcrun notarytool store-credentials "notarytool-profile" \
  --apple-id "votre.email@example.com" \
  --team-id "VOTRE_TEAM_ID" \
  --password "xxxx-xxxx-xxxx-xxxx"
```

**Remplacez** :
- `votre.email@example.com` : votre Apple ID
- `VOTRE_TEAM_ID` : trouvez-le sur https://developer.apple.com/account (onglet Membership)
- `xxxx-xxxx-xxxx-xxxx` : le App-Specific Password généré à l'étape 2.1

### 2.3 Vérifier le profil

```bash
xcrun notarytool history --keychain-profile "notarytool-profile"
```

---

## ⚙️ Étape 3 : Configurer le projet

### 3.1 Créer LocalConfig.cmake

Créez le fichier `/Users/patricecolet/repo/ComposeSiren-fixed/LocalConfig.cmake` :

```cmake
# Configuration de signature et notarisation Apple

# Certificat pour signer les applications (.app)
set(APPLE_DEVELOPER_ID_APPLICATION "Developer ID Application: Votre Nom (XXXXXXXXXX)")

# Certificat pour signer les installers (.pkg)
set(APPLE_DEVELOPER_ID_INSTALLER "Developer ID Installer: Votre Nom (XXXXXXXXXX)")

# Profil keychain pour la notarisation
set(APPLE_NOTARIZATION_KEYCHAIN_PROFILE "notarytool-profile")

# Activer la notarisation
set(ENABLE_NOTARIZATION ON)
```

**⚠️ Remplacez les valeurs** par vos vrais noms de certificats (copiés depuis `security find-identity`).

**💡 Astuce** : Un template est fourni dans `LocalConfig.cmake.template`. Copiez-le et remplissez vos valeurs :

```bash
cp LocalConfig.cmake.template LocalConfig.cmake
nano LocalConfig.cmake  # Éditer avec vos valeurs
```

### 3.2 Vérifier le .gitignore

Le fichier `LocalConfig.cmake` **ne doit jamais être commité** (il contient vos credentials).

Vérifiez que `.gitignore` contient :

```
# Configuration locale de signature
LocalConfig.cmake
```

---

## 🤖 Script d'automatisation

Un script interactif est fourni pour simplifier la configuration.

### Utilisation

```bash
cd /Users/patricecolet/repo/ComposeSiren-fixed
./scripts/setup-codesigning.sh
```

### Ce que fait le script

1. ✅ Détecte automatiquement vos certificats Developer ID
2. ✅ Crée `LocalConfig.cmake` avec la configuration
3. ✅ Guide pour configurer la notarisation Apple
4. ✅ Vérifie que tout est prêt pour un build signé

---

## 🏗️ Étape 4 : Builder avec signature

### Avec CMake (recommandé)

```bash
cd /Users/patricecolet/repo/ComposeSiren-fixed

# Nettoyer le build précédent
rm -rf build

# Configurer avec CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Builder (cela prendra du temps à cause de la notarisation)
cmake --build build --config Release

# Le DMG signé et notarisé sera dans :
# build/Packaging/ComposeSiren_Installer_artefacts/
```

⏱️ **La notarisation peut prendre 5 à 30 minutes**. Pendant ce temps, Apple analyse votre DMG pour vérifier qu'il ne contient pas de malware.

### Avec Xcode

Si vous utilisez Xcode au lieu de CMake :

1. Ouvrez le projet : `open Builds/MacOSX/ComposeSiren.xcodeproj`
2. Allez dans **Signing & Capabilities** de chaque target
3. Cochez **"Automatically manage signing"**
4. Sélectionnez votre **Team** (votre Apple Developer account)
5. Buildez normalement

---

## ✅ Vérification

### Vérifier la signature du DMG

```bash
# Vérifier la signature
codesign -dv --verbose=4 build/Packaging/ComposeSiren_Installer_artefacts/*.dmg

# Devrait afficher :
# Authority=Developer ID Application: Votre Nom (XXXXXXXXXX)
# ...
```

### Vérifier la notarisation

```bash
# Vérifier la notarisation
spctl -a -t open --context context:primary-signature -v \
  build/Packaging/ComposeSiren_Installer_artefacts/*.dmg

# Si tout est OK, vous devriez voir :
# accepted
# source=Notarized Developer ID
```

### Test final

1. **Télécharger le DMG** depuis un autre Mac (ou simuler en ajoutant l'attribut quarantine)
2. **Double-cliquer** sur le DMG
3. Si tout fonctionne sans avertissement ➔ ✅ Succès !

```bash
# Simuler un téléchargement (ajoute la quarantine)
xattr -w com.apple.quarantine "0000;00000000;Safari;|com.apple.Safari" \
  build/Packaging/ComposeSiren_Installer_artefacts/*.dmg

# Puis tester l'ouverture
open build/Packaging/ComposeSiren_Installer_artefacts/*.dmg
```

---

## 🐛 Dépannage

### "No identity found" lors de la signature

```bash
# Vérifier que les certificats sont bien installés
security find-identity -v -p codesigning

# Si vide, réimporter les certificats depuis developer.apple.com
```

### La notarisation échoue

```bash
# Voir les logs détaillés
xcrun notarytool history --keychain-profile "notarytool-profile"

# Obtenir les détails d'une soumission spécifique
xcrun notarytool log <submission-id> --keychain-profile "notarytool-profile"
```

Causes fréquentes :
- **Hardened Runtime** non activé → Vérifier CMakeLists.txt
- **Code non signé** dans les frameworks → Signer tous les binaires
- **Entitlements** incorrects → Vérifier les entitlements

### Erreur "altool has been deprecated"

✅ Vous utilisez la bonne méthode (`notarytool`), pas l'ancienne (`altool`).

### CMake ne trouve pas les certificats

Vérifiez que :
1. `LocalConfig.cmake` existe et contient les bons noms
2. Les noms de certificats correspondent **exactement** à ceux de `security find-identity`
3. Le fichier est inclus dans `CMakeLists.txt`

```bash
# Vérifier que CMake trouve la config
cmake -B build -DCMAKE_BUILD_TYPE=Release | grep "Configuration locale"

# Devrait afficher :
# -- Configuration locale chargée : signature et notarisation activées
```

---

## 📚 Ressources

### Documentation officielle Apple

- [Code Signing Guide](https://developer.apple.com/library/archive/documentation/Security/Conceptual/CodeSigningGuide/Introduction/Introduction.html)
- [Notarization Guide](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution)
- [notarytool Documentation](https://developer.apple.com/documentation/security/notarizing_macos_software_before_distribution/customizing_the_notarization_workflow)

### Outils utiles

```bash
# Vérifier la signature d'une app
codesign -dvv /path/to/App.app

# Vérifier les entitlements
codesign -d --entitlements - /path/to/App.app

# Voir l'historique de notarisation
xcrun notarytool history --keychain-profile "notarytool-profile"

# Obtenir les logs d'une soumission
xcrun notarytool log <submission-id> --keychain-profile "notarytool-profile"
```

---

## 💡 Conseils pour la distribution

### Distribution GitHub Releases

Une fois votre DMG signé et notarisé :

```bash
# Créer un release GitHub
gh release create v1.5.0 \
  build/Packaging/ComposeSiren_Installer_artefacts/ComposeSiren-v1.5.0-macOS.dmg \
  --title "ComposeSiren v1.5.0" \
  --notes-file Releases/RELEASE_NOTES_v1.5.0.md
```

### Distribution par serveur web

Si vous hébergez le DMG sur votre propre serveur :

1. **Utilisez HTTPS** (requis par macOS pour la sécurité)
2. **Servez avec le bon MIME type** : `application/x-apple-diskimage`
3. Fournissez un **checksum SHA256** pour vérification :

```bash
# Générer le checksum
shasum -a 256 ComposeSiren-v1.5.0-macOS.dmg > checksum.txt
```

---

**Dernière mise à jour** : 21 Octobre 2025

