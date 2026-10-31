# DaftSynth : synthé VST3 pour FL Studio (Windows)

Synthé + boîte à rythmes inspirés du matériel de Daft Punk
(Juno-106, Minimoog, TB-303, MC-202, Prophet, Super Jupiter, TR-808/909, LinnDrum, DMX, SP-1200).

## Obtenir le plugin (aucune installation sur ton PC)

1. Sur github.com : bouton **+** en haut à droite > **New repository**.
   Nom : `DaftSynth`, laisse en Public ou Private, clique **Create repository**.
2. Sur la page du dépôt vide : lien **uploading an existing file**.
3. Extrais le zip, puis **glisse tout le contenu du dossier `DaftSynth`**
   (CMakeLists.txt, le dossier Source, le dossier .github, LISEZMOI.md) dans la page.
   Clique **Commit changes**.
   - Si le dossier `.github` n'a pas été envoyé (dossier caché) : **Add file > Create new file**,
     tape comme nom `.github/workflows/build.yml` puis colle le contenu du fichier build.yml.
4. Onglet **Actions** : la compilation démarre toute seule (5 à 10 minutes la 1re fois).
5. Quand c'est vert : clique sur la compilation, descends à **Artifacts**,
   télécharge **DaftSynth-Windows**.
   Si c'est rouge : ouvre le job, copie les dernières lignes du log et envoie-les moi.

## Installer dans FL Studio

1. Extrais le zip téléchargé.
2. Copie le dossier **DaftSynth.vst3** dans `C:\Program Files\Common Files\VST3`.
3. FL Studio : **Options > Manage plugins > Find installed plugins** (Verify plugins),
   puis cherche **DaftSynth** dans l'onglet Plugin database > Installed > Generators.

`DaftSynth.exe` (dans le zip) est une version autonome pour tester sans FL Studio.

## Utilisation

- Le bandeau animé réagit au son (spectre, oscilloscope, soleil, grille). Les bordures néon et le logo pulsent avec le volume.
- Menu en haut : **presets** (flèches < > pour les parcourir) et **Mode** :
  Poly (Juno), Mono (Moog), Acid (303), Drums.
- Mode **Drums** : notes MIDI (FL Studio affiche C5 pour la note 60) :
  C3 grosse caisse, C#3 rim, D3 caisse claire, D#3 clap, F3/G3 toms graves, F#3 charley fermé,
  A#3 charley ouvert, A3/B3 toms moyens, C4/D4 toms aigus, C#4 crash, G#4 cowbell.
- **Accent (mode Acid)** : une note jouée avec une vélocité >= 100 est accentuée.
  Des notes qui se chevauchent créent un glissando (slide), comme sur une 303.
- **Pump** : effet de sidechain synchronisé au tempo (fonctionne quand FL Studio joue).
- Double-clic sur un potard = valeur par défaut. Molette de modulation = vibrato.

## Remarque de licence

JUCE est gratuit pour un usage personnel (un écran "Made with JUCE" peut s'afficher).
Pour vendre ou distribuer le plugin, vérifie les conditions de licence de JUCE.
