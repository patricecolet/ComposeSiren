#!/usr/bin/env python3
"""
Script pour ajouter automatiquement une Build Phase "Copy App Icon" au projet Xcode
Usage: python3 add_icon_build_phase.py
"""

import os
import re
import uuid

PROJECT_PATH = "ComposeSiren.xcodeproj/project.pbxproj"
SCRIPT_CONTENT = '''ICON_SOURCE="${PROJECT_DIR}/Assets/ComposeSiren.icns"
ICON_DEST="${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources/ComposeSiren.icns"

if [ -f "$ICON_SOURCE" ]; then
    mkdir -p "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app/Contents/Resources"
    cp "$ICON_SOURCE" "$ICON_DEST"
    touch "${BUILT_PRODUCTS_DIR}/${PRODUCT_NAME}.app"
    echo "✅ Icône copiée avec succès!"
else
    echo "⚠️  Icône source introuvable: $ICON_SOURCE"
fi'''

def generate_uuid():
    """Génère un UUID au format Xcode (24 caractères hexadécimaux)"""
    return uuid.uuid4().hex[:24].upper()

def add_build_phase(project_content):
    """Ajoute une Build Phase au projet Xcode"""
    
    # Générer des UUIDs uniques
    script_phase_id = generate_uuid()
    
    # Échapper le script pour l'insérer dans le projet
    escaped_script = SCRIPT_CONTENT.replace('\n', '\\n').replace('"', '\\"')
    
    # Créer la section PBXShellScriptBuildPhase
    script_phase_section = f'''/* Begin PBXShellScriptBuildPhase section */
		{script_phase_id} /* Copy App Icon */ = {{
			isa = PBXShellScriptBuildPhase;
			buildActionMask = 2147483647;
			files = (
			);
			inputPaths = (
			);
			name = "Copy App Icon";
			outputPaths = (
			);
			runOnlyForDeploymentPostprocessing = 0;
			shellPath = /bin/sh;
			shellScript = "{escaped_script}";
		}};
/* End PBXShellScriptBuildPhase section */'''
    
    # Trouver où insérer la section
    if '/* Begin PBXShellScriptBuildPhase section */' not in project_content:
        # Insérer avant la section PBXSourcesBuildPhase
        pattern = r'(\/\* Begin PBXSourcesBuildPhase section \*\/)'
        project_content = re.sub(pattern, script_phase_section + '\n\n\\1', project_content)
    else:
        print("⚠️  Une section PBXShellScriptBuildPhase existe déjà")
        return None
    
    # Trouver la target "ComposeSiren - Standalone Plugin"
    # et ajouter la phase dans buildPhases
    pattern = r'(name = "ComposeSiren - Standalone Plugin";.*?buildPhases = \(\s*)'
    replacement = f'\\1{script_phase_id} /* Copy App Icon */,\n\t\t\t\t'
    project_content = re.sub(pattern, replacement, project_content, flags=re.DOTALL)
    
    return project_content

def main():
    """Point d'entrée principal"""
    
    if not os.path.exists(PROJECT_PATH):
        print(f"❌ Projet Xcode introuvable : {PROJECT_PATH}")
        print("   Exécutez ce script depuis Builds/MacOSX/")
        return 1
    
    # Backup du projet
    backup_path = PROJECT_PATH + ".backup"
    print(f"📦 Création d'un backup : {backup_path}")
    
    with open(PROJECT_PATH, 'r') as f:
        original_content = f.read()
    
    with open(backup_path, 'w') as f:
        f.write(original_content)
    
    # Modifier le projet
    print("🔧 Ajout de la Build Phase 'Copy App Icon'...")
    modified_content = add_build_phase(original_content)
    
    if modified_content is None:
        print("❌ Échec : Build Phase déjà existante ou erreur")
        return 1
    
    # Sauvegarder le projet modifié
    with open(PROJECT_PATH, 'w') as f:
        f.write(modified_content)
    
    print("✅ Build Phase ajoutée avec succès!")
    print(f"   Backup sauvegardé : {backup_path}")
    print("\n🎯 Prochaines étapes :")
    print("   1. Ouvrir le projet dans Xcode")
    print("   2. Vérifier que la phase 'Copy App Icon' est présente")
    print("   3. Compiler et tester")
    print(f"\n   Pour restaurer le backup : cp {backup_path} {PROJECT_PATH}")
    
    return 0

if __name__ == "__main__":
    exit(main())

