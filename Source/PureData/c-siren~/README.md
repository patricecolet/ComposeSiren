# c-siren~

External Pure Data mono-voix pour ComposeSiren.

## Objectif

`c-siren~` joue un seul modele de sirene par instance, selectionne par argument:

- `alto` (S1)
- `bass` (S3)
- `tenor` (S4)
- `soprano` (S5)
- `piccolo` (S7)

La logique MIDI interne reproduit les controles de ComposeSiren sans routage de canal, puisque l'objet ne pilote qu'une seule sirene.

## Build

Depuis `Source/PureData/c-siren~`:

```sh
make
```

Le build utilise la copie vendored de `pd-lib-builder` dans `pd-lib-builder/`.

## Usage

Creation:

```pd
[c-siren~ alto]
```

Optionnel: second argument = chemin explicite des ressources:

```pd
[c-siren~ bass /custom/path/to/Resources]
```

## Messages supportes

- `note <pitch> <vel>`: note on/off (vel=0 => off)
- `ctl <valeur> <numCTL>`: Control Change (valeur 0..127, puis numéro de contrôleur)
- `bend <value14>`: pitch bend 14-bit (0..16383)
- `bend <lsb> <msb>`: pitch bend en deux octets
- `reset`: reset de l'etat interne
- `resources <path>`: surcharge dynamique du dossier de ressources
- `model <name>`: change de modele (`alto|bass|tenor|soprano|piccolo`)

## Resolution des ressources

Par defaut, l'objet tente ce chemin selon l'OS:

- macOS: `/Library/Audio/Plug-Ins/Mecanique Vivante/ComposeSiren_Orchestra/Resources/`
- Linux: `/usr/share/ComposeSiren/Resources/`
- Windows: `C:\Program Files\Common Files\Mecanique Vivante\ComposeSiren_Orchestra\Resources\`

Priorite:

1. Message `resources <path>`
2. Chemin par defaut de l'OS
