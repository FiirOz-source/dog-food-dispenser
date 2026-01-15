# Distributeur Automatique de Croquettes pour Chiens

Système automatisé de distribution de nourriture pour chiens avec identification RFID, détection de mouvement infrarouge, capteur ultrasonique et serveur web.

**Plateforme :** ESP8266 (NodeMCU v2)  
**Langage :** C++ (Arduino)

---

## Table des matières

1. [Installation initiale](#installation-initiale)
2. [Commandes du Makefile](#commandes-du-makefile)
3. [Documentation Doxygen](#documentation-doxygen)
4. [Dépannage](#dépannage)

---

## Installation initiale

### 1. Première utilisation ou changement de carte

Si c'est votre première fois ou que vous changez de carte Arduino, installez le core ESP8266 :

```bash
make core
```

Cette commande télécharge et installe les outils Arduino CLI et le core ESP8266.

### 2. Détection du port série

Avant de compiler, vérifiez que votre ESP8266 est bien connecté et détectez le port automatiquement :

```bash
make port
```

Le Makefile détecte automatiquement le port, mais vous pouvez le spécifier manuellement en override :

```bash
make run PORT=/dev/cu.usbserial-0001
```

**Ports courants :**
- **macOS/Linux** : `/dev/cu.usbserial*`, `/dev/cu.SLAB_USBtoUART*`, `/dev/cu.wchusbserial*`, `/dev/ttyUSB*`
- **Windows** : `COM3`, `COM4`, etc.

---

## Commandes du Makefile

### Compilation

#### `make compile` ou `make`
Compile le projet sans flashing.

```bash
make compile
```

**Affiche :** Résultat de la compilation, erreurs et avertissements.

---

### Flashing

#### `make upload`
Compile et charge le sketch sur l'ESP8266.

```bash
make upload
```

Utilise le port détecté automatiquement (ou l'override `PORT=...`).

---

### Port série et monitoring

#### `make run`
Compile, charge le sketch ET lance le moniteur série en direct.

```bash
make run
```

**Affiche :** Tous les messages de la console série en temps réel (Serial.print, logs, etc.)  
**Arrêt :** Appuyez sur `Ctrl+C`

```bash
# Avec un port spécifique
make run PORT=/dev/cu.usbserial-A5069RR4
```

---

### Système de fichiers LittleFS

#### `make fs-image`
Crée une image binaire du système de fichiers LittleFS à partir du dossier `data/`.

```bash
make fs-image
```

Génère `build/littlefs.bin` qui sera uploadé sur la flash de l'ESP8266.

---

#### `make fs-upload`
Crée l'image LittleFS ET l'upload sur la flash.

```bash
make fs-upload
```

**Important :** Le système de fichiers reste intact après un upload de sketch.

---

#### `make upload-all`
Combine sketch et système de fichiers en un seul upload.

```bash
make upload-all
```

Équivalent à faire `make upload` + `make fs-upload`.

---

#### `make run-all`
Compile tout (sketch + LittleFS) et lance le moniteur série.

```bash
make run-all
```

**C'est la commande complète à utiliser en développement normal.**

---

### Documentation

#### `make docs`
Génère la documentation Doxygen au format HTML.

```bash
make docs
```

**Génère :** Dossier `docs/html/` contenant la documentation complète.

**Pour consulter la documentation :**
1. Générez d'abord : `make docs`
2. Ouvrez le fichier HTML :
   ```bash
   open docs/html/index.html
   ```
   Ou naviguez manuellement dans votre navigateur vers le fichier `docs/html/index.html`

**Contenu de la documentation :**
- Classes et structures documentées (Doxygen)
- Hiérarchie des classes avec diagrammes
- Fonctions et variables globales
- Namespaces et modules
- Graphiques de dépendances

---

#### `make docs-clean`
Supprime le dossier `docs/` complètement.

```bash
make docs-clean
```

---

### Nettoyage

#### `make clean`
Supprime le dossier `build/`.

```bash
make clean
```

---

#### `make rebuild`
Nettoie et recompile complètement.

```bash
make rebuild
```

Équivalent à `make clean && make compile`.

---

## Documentation Doxygen

### Génération

Pour générer la documentation, exécutez :

```bash
make docs
```

### Consultation

Une fois générée, ouvrez la documentation dans votre navigateur :

**macOS :**
```bash
open docs/html/index.html
```

**Linux :**
```bash
xdg-open docs/html/index.html
```

**Ou simplement :** Naviguez dans l'explorateur jusqu'à `docs/html/` et double-cliquez sur `index.html`.

### Navigation

La documentation contient :

- **Namespaces** → Browse `dispenser_lib` et ses sous-modules
- **Classes** → Voir la hiérarchie des classes avec diagrammes d'héritage
- **Files** → Lister tous les fichiers source
- **Graphiques** → Diagrammes de classes, d'inclusion, de collaboration

---

## Exemples d'utilisation

### Développement classique (cycle itératif)

```bash
# 1. Compiler et uploader le code + moniteur série
make run-all

# 2. Après modifications, recompiler et recharger
make run-all

# 3. Ou juste recompiler sans moniteur
make compile
```

### Avec port spécifique

```bash
# Déterminer le port
make port

# Utiliser un port spécifique
make run PORT=/dev/cu.usbserial-A5069RR4
```

### Complet avec documentation

```bash
# Compiler, uploader et générer la doc
make run-all
make docs

# Consulter la doc
open docs/html/index.html
```

### Réinitialisation complète

```bash
# Nettoyer et recompiler from scratch
make rebuild

# Avec upload
make upload
```

---

## Structure du projet

```
dog-food-dispenser/
├── dog-food-dispenser.ino      # Sketch principal (setup/loop)
├── Makefile                     # Automatisation
├── Doxyfile                     # Configuration documentation Doxygen
├── README.md                    # Ce fichier
├── build/                       # Dossier de compilation (généré)
├── docs/                        # Documentation générée (Doxygen)
├── data/                        # Fichiers système (images, config)
├── libraries/                   # Bibliothèques externes
│   ├── Grove_LCD_RGB_Backlight-master/
│   └── Seeed_Arduino_UltrasonicRanger-master/
└── dispenser_lib/              # Bibliothèque modulaire du projet
    ├── dispenser/              # Logique de distribution
    ├── actuators/              # Servo motor, LCD screen
    ├── sensors/                # RFID, ultrasonic, IR
    ├── dogs/                   # Gestion des chiens
    ├── logs/                   # Système de logging
    └── wifi_server/            # Serveur web
```

---

## Dépannage

### Carte non détectée

```bash
make port
```

Si aucun port n'est affiché, vérifiez :
- Câble USB (data, pas charging-only)
- Drivers USB installés (CH340, FTDI, etc.)

### Erreur de compilation

```bash
make rebuild
```

Recompile complètement depuis zéro, utile si build est corrompu.

### Upload échoue

Spécifiez manuellement le port :

```bash
make run PORT=/dev/cu.usbserial-0001
```

### Aucune sortie série

1. Vérifiez le baud rate (115200)
2. Vérifiez que l'ESP8266 est alimenté (LED rouge = OK)
3. Essayez `make run-all` avec moniteur intégré

---

## Configuration avancée

### Changer la taille de flash

Par défaut : 4M2M (4 MB total, 2 MB code, 2 MB filesystem)

```bash
make rebuild FLASH_SIZE=4M1M
```

### Changer le baud rate

Upload : 921600 (default), Monitor : 115200 (default)

Modifier dans le Makefile si nécessaire.

---

## Interaction avec le code

Cette section explique comment interagir avec le distributeur de croquettes en fonctionnement.

### Démarrage et initialisation

Lors du démarrage (après `make run-all`), l'ESP8266 exécute la fonction `setup()` :

1. **Initialisation des capteurs** → Capteurs RFID, infrarouge, ultrasonique prêts
2. **Initialisation des actionneurs** → Servo motor, écran LCD prêts
3. **Connexion WiFi** → L'ESP8266 se connecte au réseau WiFi
4. **Synchronisation NTP** → L'heure est mise à jour via Internet
5. **Affichage LCD** → L'écran affiche "Ready" ou le message d'accueil

---

### Séquence de fonctionnement normal

Voici ce qui se passe étape par étape quand un chien se présente au distributeur en mode automatique :

#### **Étape 1 : Le chien s'approche**

```
┌─────────────────────────────────────────────────────────┐
│ Capteur Infrarouge (GPIO 13) détecte un mouvement       │
│ → Génère une interruption matérielle (falling edge)     │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Fonction `on_IR_falling()` déclenchée par l'ISR

#### **Étape 2 : Vérification du stock de croquettes**

```
┌─────────────────────────────────────────────────────────┐
│ Capteur Ultrasonique (GPIO 12) mesure la distance       │
│ du bas du bac jusqu'aux croquettes                      │
│                                                         │
│ Distance < 200mm  → Bac presque plein  (OK)             │
│ Distance 200-400mm → Bac à moitié    (ATTENTION)        │
│ Distance > 400mm  → Bac presque vide  (ALERTE)          │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Lecture du capteur ultrasonique
#### **Étape 3 : Attente de la lecture RFID**

```
┌─────────────────────────────────────────────────────────┐
│ Le système attend une lecture RFID (GPIO 14 - RX)       │
│ Timeout : 200ms pour lire le numéro de la puce          │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Dans la fonction `handle_dog_detected()`

#### **Étape 4 : Identification du chien**

```
┌─────────────────────────────────────────────────────────┐
│ Comparaison du tag RFID avec la base de données         │
│                                                         │
│ Tag : 0080D552  → Correspond à JOP                      │
│ Tag : 002E2989  → Correspond à MANOUK                   │
│ Tag : autre     → Chien inconnu, aborter                │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Boucle de recherche sur les chiens enregistrés

#### **Étape 5 : Vérification de l'intervalle d'alimentation (12h minimum)**

```
┌─────────────────────────────────────────────────────────┐
│ Calcul du temps écoulé depuis le dernier repas          │
│                                                         │
│ Horodatage actuel (NTP) : 2026-01-15 14:30:00           │
│ Dernier repas de JOP   : 2026-01-15 02:15:00            │
│ Temps écoulé           : 12h 15min (PEUT MANGER)        │
│                                                         │
│ Si < 12h : Refuser l'accès (protection pour les chiens) │
│ Si ≥ 12h : Autoriser la distribution                    │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Vérification du statut du chien

#### **Étape 6 : Distribution (si autorisé)**

```
┌─────────────────────────────────────────────────────────┐
│ Actionnement du servomoteur pour ouvrir le distributeur │
│                                                         │
│ Position fermée  (0°)   → GPIO 15 à 0ms                 │
│ Position ouverte (45°)  → GPIO 15 à 2ms (pulse PWM)     │
│ Durée ouverture : 2000ms (2 secondes)                   │
│ Fermeture        : Retour à 0°                          │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Contrôle du servo motor

#### **Étape 7 : Enregistrement de l'événement**

```
┌─────────────────────────────────────────────────────────┐
│ Sauvegarde dans le journal avec horodatage NTP          │
│                                                         │
│ Timestamp NTP : 2026-01-15T14:32:45Z                    │
│ Action        : DOG_FED                                 │
│ Dog           : JOP (0080D552)                          │
│ Food level    : 380mm                                   │
│ Duration      : 2000ms                                  │
└─────────────────────────────────────────────────────────┘
```

**Code exécuté :** Logging du repas

#### **Étape 8 : Retour au repos**

```
┌─────────────────────────────────────────────────────────┐
│ Le système attend le prochain chien                     │
│ (attente minimum 1 secondes avant nouvelle détection)   │
└─────────────────────────────────────────────────────────┘
```

---

### Diagramme complet du flux

```
START (Mode normal)
      ↓
┌─────────────────────────┐
│ IR Motion detected?     │ ← Attente du mouvement du chien
└────────────┬────────────┘
             │ OUI
      ┌──────↓────────────┐
      │ Check food level  │ ← Capteur ultrasonique
      └──────┬────────────┘
             │ OUI
      ┌──────↓───────┐
      │ Read RFID    │ ← Essayer de lire la puce (200ms)
      └──────┬───────┘
             │ Tag trouvé?
      ┌──────↓──────────┐
      │ Unknown dog?    │ ← Chercher dans la base
      └──────┬──────────┘
             │ OUI (chien identifié)
      ┌──────↓────────────────┐
      │ Time since last meal? │ ← Vérification 12h minimum
      └──────┬────────────────┘
             │ ≥ 12h?
      ┌──────↓────────────┐
      │ Open servo motor  │ ← Durée 2s
      └──────┬────────────┘
             │
      ┌──────↓────────────┐
      │ Log event (NTP)   │ ← Sauvegarde avec timestamp
      └──────┬────────────┘
             │
      ┌──────↓────────────┐
      │ Back to ready     │
      └──────┬────────────┘
             │
      └──→ Attendre prochain chien
```

---

### Accès à l'interface web

L'ESP8266 crée un serveur web pour contrôler le distributeur à distance.

#### Trouver l'adresse IP

Lors du démarrage, l'adresse IP s'affiche dans le moniteur série :

```
WiFi connected
IP address: 192.168.1.XXX
```

Notez cette adresse (par exemple `192.168.1.42`).

#### Accès via navigateur

Ouvrez votre navigateur et allez à :

```
http://192.168.1.XXX
```

L'interface web affiche :
- État actuel du distributeur
- Historique des distributions
- Bouton pour déclencher manuellement l'alimentation
- Informations sur les chiens

---

### Interaction via page web

Le serveur web permet de lire les logs via une interface console. La page web affiche egalement la quantité de croquettes, les dernières dates de distribution de croquettes pour chaque chien, etc...
Il est egalement possible de déclencher manuellement une distribution de croquettes via le bouton distribuer.


---

### Interaction automatique via capteurs

Le système fonctionne automatiquement selon cet ordre :

#### 1. Détection d'un chien

```
Capteur infrarouge (GPIO 13) → Détecte la présence d'un chien
```

#### 2. Identification par RFID

```
Lecteur RFID (GPIO 14/RX) → Lit la puce RFID du collier
```

**Puces enregistrées :**
- **Jop** : `0080D552`
- **Manouk** : `002E2989`

#### 3. Vérification de l'intervalle d'alimentation

```
Historique de distribution → Vérifier si le chien a mangé il y a < 12h
```

Si le chien n'a pas mangé depuis 12h, il reçoit de la nourriture.

#### 4. Dispensing (Contrôle du servomoteur)

```
Servo motor (GPIO 15) → Ouvre le distributeur pendant 2 secondes
                     → Ferme le distributeur
```

#### 5. Enregistrement de l'événement

```
Système de logs → Enregistre :
                  - Heure précise (NTP)
                  - Chien identifié
                  - Niveau de stock restant
```

---

### Code d'exemple : Intégration personnalisée

Si vous voulez intégrer le code dans votre propre système, consultez la classe `Dispenser` :

```cpp
// Include la bibliothèque
#include "dispenser_lib/dispenser/dispenser.hpp"

// Dans setup()
init_dispenser();

// Dans votre code
// Déclencher manuellement l'alimentation
web_dispense_request = true;  // Flag lu dans la boucle principale

// Ou dans loop()
if (some_condition) {
    // Appeler directement la fonction de distribution
    handle_dog_detected();
}
```

### Lecture des capteurs

Accédez aux classes de capteurs directement :

```cpp
// Capteur ultrasonique (distance)
long distance_mm = ultrasonic_sensor.get_distance();
if (distance_mm < 400) {
    Serial.println("Bac presque vide !");
}

// Capteur RFID
String tag = rfid_sensor.read_rfid(100);  // Lecture avec timeout 100ms
if (tag == "0080D552") {
    Serial.println("Jop detected!");
}

// Vérification du temps depuis la dernière alimentation
dog jop = dogs[0];  // Le chien Jop
unsigned long ms_since_fed = jop.since_fed();
if (ms_since_fed > 12 * 3600000) {  // 12 heures
    jop.mark_fed();  // Marquer comme nourri
}
```

### Logging des événements

Le système enregistre automatiquement les événements avec horodatage NTP en mémoire :

```cpp
// Ajouter un événement custom au log
app_log += "Mon événement personnalisé";

// Les logs incluent automatiquement :
// - Timestamp (NTP si synchronisé, sinon uptime)
// - Niveau de stock restant
// - ID du chien détecté
// - État de la boucle principale
```

**Stockage des logs :**
- **En mémoire RAM** : Accessible via l'API REST `/logs`
- **Affichage temps réel** : Visible dans le moniteur série
- **Pas de persistance** : Logs perdus au redémarrage/interruption d'alimentation
- **Limité par la RAM** : ~200-300 entrées maximum selon les autres allocations

**Note :** Les logs ne sont stockés qu'en mémoire. Pour une persistance permanente sur LittleFS, il faudrait implémenter une sauvegarde (non implémenté actuellement).

### Affichage LCD

Contrôlez l'écran LCD RGB I2C :

```cpp
// Effacer l'écran
lcd_screen->clear();

// Afficher un message
lcd_screen->display_message("Feeding Jop!", 0, 0);  // Ligne 0, colonne 0
lcd_screen->display_message("Please wait...", 1, 0);  // Ligne 1

// Changer la couleur de fond
lcd_screen->setRGB(255, 0, 0);  // Rouge
```

### Monitoring en temps réel

Utilisez `make run-all` pour voir en direct tous les événements :

```
[10:25:34] WiFi connected - IP: 192.168.1.42
[10:25:45] NTP time synced
[10:26:12] IR sensor triggered
[10:26:13] RFID detected: 0080D552 (Jop)
[10:26:14] Jop eligible for feeding (last fed 14h ago)
[10:26:15] Opening servo...
[10:26:17] Closing servo (2000ms dispensed)
[10:26:17] Event logged: Jop fed at 2026-01-15 10:26:17
```

Appuyez sur **Ctrl+C** pour arrêter le monitoring.

---

## Auteurs

**Burgmeier Timothée & Louis-le-Denmat Raphaël**  
15 Janvier 2026
