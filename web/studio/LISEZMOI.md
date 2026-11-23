# FM‑1 Groove Studio

Groovebox web pour le **M‑VAVE FM‑1** : vue FM‑1 (machine virtuelle), éditeur complet, USB MIDI et bibliothèque de sons.

## Lancer

Le MIDI USB et l'installation de l'app ont besoin d'une adresse **https** (ou de `localhost`) dans **Chrome ou Edge**.

- Sur ton ordinateur : `python -m http.server 8000` dans ce dossier, puis ouvre <http://localhost:8000/>
- En ligne : dépose le dossier sur GitHub Pages, Netlify ou ton hébergement (tout est statique)
- Dans Deskosia : le projet reste compatible (`window.deskosia`)

Le piano (son de base) est dans la bibliothèque, dossier Synth, et dans le menu Son de chaque piste mélodique.

`install.html` installe l'app, teste la connexion au FM‑1 et lance l'assistant des knobs.

## Vue FM‑1 : la logique des commandes

| Commande | Rôle |
| --- | --- |
| PRESETS | choisir la piste (8 lignes de batterie + 6 pistes mélodiques) |
| ALGORITHM | rythme de la banque (change à la mesure suivante) |
| SELECT | curseur de la page · clic = action principale |
| MASTER | volume général |
| KNOB 1‑4 | les 4 réglages affichés en bas de l'écran |
| 11 touches du haut | jouent la piste choisie (batterie : les 8 sons · mélodie : la gamme, OCT ±) |
| 16 touches du bas | rôle selon la page, étiquette au-dessus de chaque touche |

Pages : appuie plusieurs fois sur le même bouton pour passer à la page suivante.

| Bouton | Pages |
| --- | --- |
| HOME | Live général (KNOB2 = filtre DJ) → Live rythme → Live mélodie · **HOME maintenu 2 s = 🎲 nouvelle génération de la piste choisie** |

**FX LIVE (HOME, 4e page)** : 16 effets sur 4 pages de 4 knobs, ALGORITHM passe de l'une à l'autre (A → D) :
- A : filtre, résonance, disto, écho
- B : reverb, wobble (LFO du filtre), vitesse du wobble, volume
- C : chorus, roll (×2 ×3 ×4), pas sautés (probabilité), reverse
- D : loop (1, 2, 4, 8 pas), bitcrush, gate (hachage au 1/32), demi-tempo

SELECT (ou les touches blanches 1-8) choisit où ils agissent : rythme, toute la mélodie, basse, basse 2, synthé, synthé 2, nappe ou fx. Les touches 9-16 coupent/rétablissent les 8 effets des pages A+B (ou C+D quand tu y es). Chaque effet a son animation, et les mouvements de knobs s'enregistrent avec REC.

**Mode JEU (pages LIVE)** : ALGORITHM à droite, les 27 touches jouent l'instrument choisi (mélodie : la gamme qui monte de gauche à droite ; batterie : 1-8 les sons, 9-16 accentués). ALGORITHM à gauche : retour aux coupures.

**ARP maintenu 2 s** : valide le style choisi et regénère le rythme et toutes les mélodies.

**ALGORITHM** : par défaut il fait défiler toutes les pages de la machine. Dans GLO (page Tempo, touches 3 à 8) tu peux lui donner un autre rôle : BPM, filtre DJ, volume de la piste, rythmes de la banque ou swing. Son rôle s'affiche sous le knob.
| SEQ | Rythme → Mélodie |
| ARP | Styles (liste + aperçu animé) → Visu (animation du style joué) → Gen rythme → Gen mélodie (mêmes touches : 🎲, varier, 🎲 tout, vider, vider tout, garder/enregistrer, dupliquer/charger, supprimer) |
| FX | Récap (tous les effets actifs, une touche coupe chacun, 16 = tout couper) → Filtre → Espace → Batterie → Perf · **FX maintenu 2 s = l'effet de la page à zéro** |
| SCL | Gamme, avec une scène animée par gamme |
| ENV | Env (attaque, decay, enveloppe, glide) → Filtre (passe-bas / haut / bande) → LFO (vitesse, profondeur, forme, calage tempo) → Osc (oscilloscope en direct) |
| LFO | Mix → Loop / Roll |
| EDIT | Son (bibliothèque, piano compris, ＋ mes sons) → Réglages du son → Éditer (couper, supprimer, reverse, normaliser, fondus, silence, gain, annuler, copie) → MIDI pistes (canal, sortie, note, horloge pour les 14 pistes) |
| GLO | Tempo → MIDI |
| SAVE | Banque (＋ rythme, ＋ mélodie, liste à lancer) → Rythmes → Mélodies → Nom |

## Fichiers

`app.js`, `audio.js`, `data.js` : ton moteur d'origine (avec des points d'accroche pour le MIDI) · `fm1.js` / `fm1.css` : la vue FM‑1 · `midi.js` : USB MIDI · `lib.js` : bibliothèque · `install.html`, `manifest.webmanifest`, `sw.js` : installation et mode hors ligne.

## FM‑1 branché en USB

Avec le firmware d'origine du FM‑1 (et Felucca), seules les **touches** sont envoyées en USB ; les knobs et les boutons n'envoient rien. Les touches pilotent toute la vue FM‑1. Les knobs marcheront avec le firmware Groove.

## REC

REC : chaque note va sur le pas le plus proche (quantisée) avec la durée pendant laquelle tu la tiens ; un bandeau rouge « REC … PAS n » confirme. Pendant REC, les auto-randoms sont suspendus pour ne pas effacer ce que tu enregistres. REC enregistre à la fois les notes jouées (touches du haut) et les mouvements de knobs (pages LIVE, FX LIVE, FX, ENV, MIX, SON). Les mouvements rejouent en boucle (1, 2, 4 ou 8 mesures, réglable dans FX) ; un point cyan marque un knob enregistré.

## Bibliothèque

Onglet **Bibliothèque** dans la barre du haut : les dossiers sont de grandes cases, chacune avec son icône dessinée et sa couleur (Kick rouge, Snare orange, Clap rose, Hi-hat jaune, Open hat vert citron, Perc vert, Tom corail, Basse bleu, Synth cyan, Piano ivoire, Nappe violet, FX magenta, Voix bleu ciel, Boucles vert pomme, Samples gris). La même couleur marque chaque son dans les listes, les menus et la page SON du FM‑1. Glisse tes fichiers ou un dossier entier directement sur une case : ils y sont rangés.

## Scènes des gammes (SCL)

Mineur : ville la nuit sous la pluie · Phrygien : danseur égyptien devant les pyramides · Dorien : club funk et boule disco · Mineur harmonique : charmeur de serpent dans le désert · Penta mineure : mont Fuji et cerisiers · Majeur : plage au soleil. Elles bougent sur le tempo pendant la lecture.

## MIDI

Chaque piste a sa sortie (interne / FM‑1 / les deux), son canal (1‑16) et, pour la batterie, sa note MIDI : page EDIT · MIDI PISTES sur le FM‑1, ou section FM‑1 USB de l'éditeur. Horloge : **interne** (l'app mène, et peut envoyer son horloge), **externe** (l'app suit l'horloge reçue : tempo, départ, arrêt), **auto** (suit une horloge si elle arrive, sinon interne).

## Couleurs

Chaque piste a la couleur de sa catégorie partout : Kick rouge, Snare orange, Clap rose, Hat F. jaune, Hat O. vert citron, Perc et Rim vert, Low corail, Basse bleu, Synthé cyan, Nappe violet, FX magenta.

## Base de rythmes (MIDI)

Onglet **Rythme** → **BASE DE RYTHMES** :
- **Importer MIDI / ZIP** : des fichiers `.mid` de batterie (carte General MIDI) ou un `.zip` qui en contient. Chaque mesure différente devient un motif, rangé par genre (le nom du dossier). Les notes fortes deviennent des accents, les très douces des notes fantômes.
- **Ajouter ce rythme** : met le rythme affiché dans la base. **Exporter ce rythme / la base (.mid)** : fichier MIDI, canal 10.
- **Random pioche dans** : *Génération* (comme avant), *Base de rythmes*, ou *Les deux*. Ce réglage vaut pour « Rythme random » et pour l'auto-random. Avec un genre choisi dans la liste, la pioche reste dans ce genre ; sinon elle prend les motifs du style en cours, ou toute la base.
- La base est gardée dans ce navigateur, à part du projet. Rien n'est envoyé ni publié.
