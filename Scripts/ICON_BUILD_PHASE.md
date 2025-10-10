# Intégration automatique de l'icône dans les builds Xcode

## Méthode 1 : Ajouter manuellement dans Xcode (Recommandé)

1. **Ouvrir le projet Xcode**
   ```bash
   open Builds/MacOSX/ComposeSiren.xcodeproj
   ```

2. **Sélectionner la target "ComposeSiren - Standalone Plugin"**
   - Dans le navigateur de projet (panneau de gauche)
   - Cliquer sur le projet "ComposeSiren"
   - Sélectionner la target "ComposeSiren - Standalone Plugin"

3. **Ajouter une Build Phase**
   - Aller dans l'onglet "Build Phases"
   - Cliquer sur le "+" en haut à gauche
   - Choisir "New Run Script Phase"

4. **Configurer le script**
   - Nommer la phase : "Copy App Icon"
   - Dans la zone de script, ajouter :
   ```bash
   "${PROJECT_DIR}/Scripts/copy_icon.sh"
   ```
   - Ou directement le contenu du script :
   ```bash
   ICON_SOURCE="${PROJECT_DIR}/Assets/ComposeSiren.icns"
   ICON_DEST="${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/ComposeSiren.icns"
   
   if [ -f "$ICON_SOURCE" ]; then
       mkdir -p "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources"
       cp "$ICON_SOURCE" "$ICON_DEST"
       touch "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app"
       echo "✅ Icône copiée avec succès!"
   fi
   ```

5. **Ordre des phases**
   - Déplacer "Copy App Icon" **après** "Copy Files" ou "Embed Frameworks"
   - Mais **avant** "Code Sign"

6. **Tester**
   ```bash
   xcodebuild -project Builds/MacOSX/ComposeSiren.xcodeproj \
              -target "ComposeSiren - Standalone Plugin" \
              -configuration Release clean build
   ```

---

## Méthode 2 : Script automatique (Expérimental)

Utiliser le script Python pour modifier automatiquement le projet :

```bash
python3 Builds/MacOSX/add_icon_build_phase.py
```

⚠️ **Attention** : Cette méthode modifie le fichier `.xcodeproj`. Faites un backup avant !

---

## Méthode 3 : Via Projucer (Recommandé si vous utilisez Projucer)

1. **Ouvrir ComposeSiren.jucer** dans Projucer
2. **File → Open in IDE → Xcode (macOS)**
3. Projucer régénère le projet avec la bonne configuration d'icône

---

## Vérification

L'icône devrait maintenant apparaître :
- ✅ Dans le Finder (icône de l'app)
- ✅ Dans le Dock (quand l'app est lancée)
- ✅ Dans le dossier Applications

Pour rafraîchir l'icône dans le Finder :
```bash
/System/Library/Frameworks/CoreServices.framework/Versions/Current/Frameworks/LaunchServices.framework/Versions/Current/Support/lsregister -f -R ~/Applications/ComposeSiren.app
```

