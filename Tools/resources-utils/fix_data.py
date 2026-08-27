#!/usr/bin/env python3
"""
Correction de l'artefact de crossfade dans les fichiers data (+ réduction optionnelle).

Non destructif : les fichiers de Resources/ (source de vérité) ne sont jamais
modifiés ; les fichiers corrigés sont écrits dans un dossier de sortie séparé.

Correction (cf. CORRECTIFS_ARTEFACT_CROSSFADE.md, session du 25 mai 2026) :
  1. Réindexation des partiels par numéro d'harmonique physique :
     le slot h contient l'harmonique h+1 de la fondamentale de grille.
     En cas de collision, l'harmonique la plus forte est conservée.
  2. Fréquences forcées aux harmoniques exactes f0×(h+1), amplitudes
     préservées. Chaque transition entre notes adjacentes devient exactement
     un demi-ton pour toutes les harmoniques → plus de fréquences fantômes
     dans le crossfade.

Format de sortie :
  - défaut : format plein, identique à l'original (drop-in : remplacer le
    contenu de Resources/ par celui du dossier de sortie pour tester, sans
    aucune modification du code).
  - --reduit (EXPÉRIMENTAL, 603 Mo → ~28 Mo) : dataAmp packé par note (seules
    les nbFrames réelles de dureTabs[note][1] sont stockées), dataFreq omis
    (les fréquences sont déterministes après correction : f0 est écrite dans
    dureTabs[note][2], champ inutilisé par le code). Ce format nécessite un
    loader C++ adapté — il n'est PAS lisible par le code actuel.

Grille harmonique : auto-détectée par sirène. La fréquence nominale du code
(pitchSchift) est 440·2^((note-69)/12) ; les données S1/S3/S4/S5 sont
échantillonnées sur cette grille (f0/2 par rapport à la formule du correctif
S7), S7 une octave au-dessus. La grille détectée est conservée pour ne pas
transposer les sirènes.

Usage :
    python3 fix_data.py [--resources <dossier>] [--output <dossier>]
                        [--reduit] [--dry-run]
"""

import argparse
import shutil
from pathlib import Path

import numpy as np

MAX_TAB = 1000
MAX_PARTIEL = 200
SEUIL_AMP = 1e-7  # en dessous, un slot est considéré vide

# fichiers data par sirène ; S2 réutilise S1, S6 réutilise S5 (cf. sirenProperties.h)
SIRENES = ["S1", "S3", "S4", "S5", "S7"]


def f0_theorique(note):
    """Formule du correctif S7 : fondamentale théorique de l'index de table."""
    return 440.0 * 2.0 ** ((note - 57) / 12.0)


def detecter_grille(freq, amp, dure, n_notes):
    """Détecte la grille harmonique : la plus grande fraction de f0_theorique
    (1.0, 0.5, 0.25) sur laquelle les fréquences actives s'alignent en
    multiples entiers. Vote amplitude-pondéré sur l'ensemble des notes.
    Retourne 1.0 (S7) ou 0.5 (S1/S3/S4/S5)."""
    votes = {}
    for note in range(n_notes):
        nb = int(dure[note][1])
        if nb == 0:
            continue
        avg_f = freq[note, :nb, :].mean(axis=0)
        peak_a = amp[note, :nb, :].max(axis=0)
        masque = (peak_a > 1e-4) & (avg_f > 20.0)
        f_act, a_act = avg_f[masque], peak_a[masque]
        if len(f_act) < 4:
            continue
        f0 = f0_theorique(note)
        for g in (1.0, 0.5, 0.25):  # de la plus grande à la plus petite
            n_harm = f_act / (f0 * g)
            # écart à l'entier le plus proche, pondéré par l'amplitude
            err = np.abs(n_harm - np.round(n_harm))
            err_moyen = float(np.average(err, weights=a_act))
            if err_moyen < 0.15:
                votes[g] = votes.get(g, 0.0) + float(a_act.sum())
                break
    if not votes:
        raise ValueError("aucune note exploitable pour détecter la grille")
    return max(votes, key=votes.get)


def corriger_sirene(freq, amp, dure, n_notes, grille):
    """Réindexation par harmonique physique + fréquences exactes f0×(h+1).
    Écrit aussi f0 dans dureTabs[note][2] (utile au format réduit, sans effet
    sur le code actuel qui ignore ce champ).
    Retourne (newAmp, newFreq, dure modifié, stats)."""
    new_amp = np.zeros_like(amp)
    new_freq = np.zeros_like(freq)
    collisions = 0
    perdues = 0  # harmoniques au-delà du slot 200 (notes graves)
    perdues_audibles = 0  # idem, mais avec une amplitude > -60 dB
    harm_max = 0

    for note in range(n_notes):
        f0 = f0_theorique(note) * grille
        dure[note][2] = f0
        # fréquences exactes pour tous les slots, même vides (comme le
        # correctif S7 : les slots vides gardent la valeur théorique attendue)
        new_freq[note, :, :] = f0 * np.arange(1, MAX_PARTIEL + 1,
                                              dtype=np.float32)
        nb = int(dure[note][1])
        if nb == 0:
            continue
        avg_f = freq[note, :nb, :].mean(axis=0)
        avg_a = amp[note, :nb, :].mean(axis=0)
        for h in range(MAX_PARTIEL):
            if avg_f[h] <= 0 or avg_a[h] <= SEUIL_AMP:
                continue
            harm_num = max(1, round(float(avg_f[h]) / f0))
            cible = harm_num - 1
            if cible >= MAX_PARTIEL:
                perdues += 1
                if amp[note, :nb, h].max() > 1e-3:
                    perdues_audibles += 1
                continue
            harm_max = max(harm_max, harm_num)
            # en cas de collision, garder l'harmonique la plus forte
            existante = new_amp[note, :nb, cible].mean()
            if existante > 0:
                collisions += 1
            if avg_a[h] > existante:
                new_amp[note, :nb, cible] = amp[note, :nb, h]

    return new_amp, new_freq, dure, {
        "collisions": collisions,
        "perdues": perdues,
        "perdues_audibles": perdues_audibles,
        "harm_max": harm_max,
    }


def packer_amp(new_amp, dure, n_notes):
    """Concatène les frames réellement utilisées : [note][0..nbFrames-1][partiel]."""
    blocs = []
    for note in range(n_notes):
        nb = int(dure[note][1])
        if nb > 0:
            blocs.append(new_amp[note, :nb, :])
    return np.concatenate(blocs, axis=0).astype(np.float32)


def traiter(resources, sortie, sirene, reduit, dry_run):
    chemin_amp = resources / f"dataAmp{sirene}"
    chemin_freq = resources / f"dataFreq{sirene}"
    chemin_dure = resources / f"datadureTabs{sirene}"

    taille = chemin_amp.stat().st_size
    n_notes = taille // (4 * MAX_TAB * MAX_PARTIEL)
    assert n_notes * 4 * MAX_TAB * MAX_PARTIEL == taille, \
        f"{sirene}: taille de fichier inattendue ({taille})"

    amp = np.fromfile(chemin_amp, dtype=np.float32).reshape(
        (n_notes, MAX_TAB, MAX_PARTIEL))
    freq = np.fromfile(chemin_freq, dtype=np.float32).reshape(
        (n_notes, MAX_TAB, MAX_PARTIEL))
    dure = np.fromfile(chemin_dure, dtype=np.float32).reshape(
        (n_notes, 3)).copy()

    grille = detecter_grille(freq, amp, dure, n_notes)
    new_amp, new_freq, dure, stats = corriger_sirene(
        freq, amp, dure, n_notes, grille)

    if reduit:
        amp_sortie = packer_amp(new_amp, dure, n_notes)
        taille_apres = amp_sortie.nbytes
    else:
        amp_sortie = new_amp
        taille_apres = new_amp.nbytes + new_freq.nbytes

    print(f"{sirene}: {n_notes} notes, grille=f0x{grille}, "
          f"harmonique max={stats['harm_max']}, "
          f"collisions={stats['collisions']}, "
          f"harmoniques>{MAX_PARTIEL} perdues={stats['perdues']} "
          f"(dont audibles >-60dB : {stats['perdues_audibles']}), "
          f"{taille * 2 / 1e6:.0f} Mo -> {taille_apres / 1e6:.1f} Mo")

    if dry_run:
        return

    amp_sortie.astype(np.float32).tofile(sortie / f"dataAmp{sirene}")
    dure.astype(np.float32).tofile(sortie / f"datadureTabs{sirene}")
    if not reduit:
        new_freq.astype(np.float32).tofile(sortie / f"dataFreq{sirene}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[1])
    racine = Path(__file__).resolve().parent / "../.."
    defaut_resources = (racine / "Resources").resolve()
    defaut_sortie = (racine / "Resources-fixed").resolve()
    parser.add_argument("--resources", type=Path, default=defaut_resources,
                        help=f"fichiers data originaux (défaut : {defaut_resources})")
    parser.add_argument("--output", type=Path, default=defaut_sortie,
                        help=f"dossier de sortie (défaut : {defaut_sortie})")
    parser.add_argument("--reduit", action="store_true",
                        help="format packé expérimental (nécessite un loader adapté)")
    parser.add_argument("--dry-run", action="store_true",
                        help="affiche les stats sans écrire")
    args = parser.parse_args()

    if args.output.resolve() == args.resources.resolve():
        parser.error("le dossier de sortie doit être différent de --resources "
                     "(les originaux sont la source de vérité)")

    if not args.dry_run:
        args.output.mkdir(parents=True, exist_ok=True)

    for sirene in SIRENES:
        traiter(args.resources, args.output, sirene, args.reduit, args.dry_run)

    if not args.dry_run and not args.reduit:
        # compléter le dossier de sortie pour qu'il soit un drop-in complet
        for f in sorted(args.resources.glob("dataVectorInterval*")):
            shutil.copy2(f, args.output / f.name)
        print(f"\nSortie drop-in complète dans : {args.output}")
        print("Test A/B : échanger temporairement Resources/ et ce dossier "
              "(les originaux restent la source de vérité).")


if __name__ == "__main__":
    main()
