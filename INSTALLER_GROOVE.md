# Groove pour M-VAVE FM-1 — compiler et installer

Groove est un firmware pour le FM-1 : 8 pistes de batterie + 6 pistes mélodiques (basse, synthé, basse 2,
synthé 2, nappe, fx), les générateurs de rythmes et de mélodies de FM-1 Groove Studio (17 styles, 6 gammes,
accords), et les pages LIVE / FX LIVE / ARP / SEQ / ENV… sur l'écran de la machine.
Il est construit sur **Hortator** (DEADACTIVE), lui-même construit sur **Felucca** (Leo Kuroshita), sous licence
GPL-3.0 : le code source reste libre et doit le rester.

> ⚠️ **À tes risques.** Groove n'a pas encore été essayé sur un vrai FM-1. Il passe tous les tests du simulateur
> (le même code C qu'en vrai), mais le premier essai sur la machine reste un premier essai. Le retour au firmware
> officiel est expliqué tout en bas.

## 1. Mettre le projet sur GitHub (une seule fois, ~5 min)

Le projet complet a plus de 500 fichiers, trop pour l'envoi depuis le navigateur (100 maximum). On part donc
d'une copie d'Hortator sur ton compte, puis on y dépose seulement les 15 fichiers de Groove.

1. Crée un compte sur <https://github.com> si tu n'en as pas, et connecte-toi.
2. Ouvre <https://github.com/DeadActive/hortator> → bouton **Fork** (en haut à droite) → nomme-le `groove-fm1`
   → **Create fork**. Tu as maintenant `https://github.com/TON-NOM/groove-fm1`.
3. Dézippe **groove-fm1-patch.zip** sur ton PC. Sur la page de ton fork : **Add file → Upload files**, puis glisse
   **le contenu** du dossier dézippé (les dossiers `firmware`, `tests`, `tools`, `.github` et les fichiers `.md`).
   GitHub garde les dossiers et remplace les fichiers du même nom. En bas : **Commit changes**.
   - Sous Windows, le dossier `.github` est caché : dans l'Explorateur, **Affichage → Éléments masqués**.
   - S'il ne passe pas : **Add file → Create new file**, nom `.github/workflows/build.yml`, colle le contenu
     du fichier `build.yml`, puis **Commit changes**.
4. Onglet **Actions** : un fork a les Actions coupées au départ, clique sur le bouton vert
   **I understand my workflows, go ahead and enable them**.
5. **Settings → Pages → Source : GitHub Actions** (pour l'installateur dans le navigateur).

(Groove a été fait sur Hortator au commit `44c9ad4` du 5 novembre 2026. Si Hortator a beaucoup changé depuis et
que la compilation échoue, envoie-moi le message d'erreur.)

## 2. La compilation automatique

Dès que les fichiers sont envoyés (ou en relançant : **Actions → Build firmware → Run workflow**), GitHub compile tout seul :

1. Onglet **Actions** → **Build firmware** → la ligne du haut (un rond jaune = en cours, environ 5 à 15 min).
2. Coche verte : en bas de la page, **Artifacts → groove-firmware**. Tu télécharges un zip qui contient
   `felucca-UNTESTED.fwsc` : c'est le firmware Groove (« UNTESTED » parce qu'il n'a pas encore été essayé sur une vraie machine).
3. Croix rouge : ouvre l'étape en rouge, ou télécharge l'artifact **build-log**, et envoie-moi le message
   d'erreur : je corrige.

Pour relancer à la main : **Actions → Build firmware → Run workflow**. Chaque modification envoyée sur GitHub
relance aussi la compilation.

## 3. L'essayer d'abord dans le simulateur

Le site publié contient un simulateur : le vrai code C de Groove compilé en WebAssembly, qui tourne dans le
navigateur sur un FM-1 en 3D (souris, écran tactile ou clavier de l'ordinateur). Ouvre
`https://TON-NOM.github.io/groove-fm1/`, clique l'interrupteur **Power** au-dessus de la machine, puis joue.
Ce que tu y vois et entends, c'est ce que fera la machine (au processeur près).

Le même site contient aussi la **version web** (Groove Studio) : `https://TON-NOM.github.io/groove-fm1/studio/`
(bouton **Studio** en haut du simulateur). Sa page « Installer » mène au simulateur et à l'installateur.

## 4. Installer sur le FM-1

### Le plus simple : l'installateur dans le navigateur (Chrome ou Edge)

Quand le job **installer** est vert, ouvre :

`https://TON-NOM.github.io/groove-fm1/webapp/installer/`

Branche le FM-1 en USB, clique sur connecter, autorise l'accès MIDI, puis installe. Ne débranche pas pendant
l'installation. Le FM-1 redémarre sur Groove.

### Sinon : en ligne de commande

```
pip install mido python-rtmidi
python tools/fm1_install.py felucca.fwsc
```

## 5. Les commandes de Groove sur la machine

| | |
|---|---|
| **HOME** | LIVE → LIVE RYTHME → LIVE MÉLODIE → **FX LIVE** (appuie encore pour la page suivante) |
| **HOME maintenu 2 s** | une nouvelle génération pour la piste choisie |
| **SEQ** | RYTHME (les 16 pas de la ligne de batterie) → MÉLODIE (le piano-roll de la piste) |
| **ARP** | STYLES → GÉN. RYTHME → GÉN. MÉLODIE · **maintenu 2 s** : valide le style et régénère tout |
| **FX** | délais et reverb généraux → effets de la batterie · **maintenu 2 s** : remet les effets à zéro |
| **SCL** | la gamme, la tonique, la suite d'accords |
| **ENV** | ENV → FILTRE → LFO → OSC de la piste choisie |
| **LFO** | MIX → LOOP ROLL |
| **EDIT** | SON → SON RÉGLAGES → ÉDITER → MIDI PISTES |
| **GLO** | TEMPO → MIDI |
| **SAVE** | BANQUE → RYTHMES → MÉLODIES |
| **PRESETS** | choisit la piste (8 batteries + 6 mélodiques) |
| **SELECT** | se déplace dans la page (le pas, le style, la cible des effets…) |
| **KNOB 1-4** | les 4 réglages affichés en bas de l'écran |
| **ALGORITHM** | sur LIVE : à droite **mode JEU** (tout le clavier joue la piste), à gauche les coupures · sur FX LIVE : pages A → D · ailleurs : toutes les pages |
| **Touches noires** | jouent la piste choisie (batterie : les 8 sons) |
| **Touches blanches** | changent avec la page, leurs LED montrent ce qu'elles font |
| **REC** | les notes jouées s'écrivent dans le motif pendant la lecture |
| **GLO → touche blanche 16** | les pages d'Hortator (retour à Groove : maintiens **ENV** et appuie sur **ARP**) |

**FX LIVE**, 16 effets sur 4 pages (ALGORITHM), sur la cible choisie par SELECT ou les touches 1-8 (rythme,
toute la mélodie, basse, basse 2, synthé, synthé 2, nappe, fx). Les touches 9-16 coupent et rétablissent un effet.

- A : filtre, réso, disto, écho
- B : reverb, wobble, vitesse, volume (pour la batterie : enveloppe et decay du filtre)
- C : chorus, roll ×2/×3/×4, pas sautés, reverse
- D : loop 1-8 pas, bitcrush, gate, demi-tempo

## 6. Ce que le firmware ne fait pas (encore)

- La **banque SAVE** (8 rythmes, 8 mélodies) reste en mémoire jusqu'à l'extinction : elle n'est pas encore
  gardée dans la flash. La batterie se sauve aussi avec les projets d'Hortator.
- Pas d'échantillons (samples) ni de piano, pas d'éditeur de sons, pas d'enregistrement des mouvements de knobs,
  pas de canaux MIDI par piste : ces parties sont dans la version web.
- Les décors des styles et des gammes sont en pixel art, plus simples que ceux de la version web.

## 7. Revenir au firmware officiel

Utilise **M-UPGRADE** de M-VAVE (<https://www.m-vave.com/download> → PC Software → M-UPGRADE) avec le firmware
officiel du FM-1. Si le FM-1 ne répond plus du tout : voir `docs/RECOVERY.md`.

## Licence et crédits

GPL-3.0-only, comme Hortator et Felucca. Groove : `firmware/src/groove.c`, `groove_ui.c`, `gv_data.h`
(généré par `tools/gen_gvdata.py` depuis les données de FM-1 Groove Studio), `tests/sim_groove.c`.
Hortator © 2026 DEADACTIVE · Felucca © 2026 Leo Kuroshita, Hügelton Instruments. Le paquet contient trois fichiers
du SDK JieLi AC79 (Apache-2.0), voir `LICENSING.md`.
