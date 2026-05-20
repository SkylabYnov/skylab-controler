# controller — Firmware manette ESP32

Firmware de la **radiocommande** Aerisys. Lit 2 joysticks analogiques
+ 2 boutons, et envoie l'état au drone via **ESP-NOW** à ~100 Hz.

## Stack

- **Plateforme** : ESP32 DevKit (`esp32dev`)
- **Framework** : ESP-IDF via PlatformIO (`espressif32 @ 6.10.0`)
- **Langage** : C++ (FreeRTOS, classes avec tâche dédiée)
- **Communication** : ESP-NOW (2.4 GHz, Wi-Fi peer-to-peer)
- **Dépendances** (cf. [platformio.ini](platformio.ini)) :
  - `github.com/Aerisys/esp-lib` (DTOs partagés)
  - `github.com/Aerisys/imu-lib` (présent mais pas utilisé ici)

## Commandes utiles

```powershell
pio run                       # build
pio run -t upload             # flash USB
pio device monitor -b 115200  # logs série
pio run -t clean              # clean
```

## Architecture

```
app_main (main.cpp)
  ├── nvs/netif/event_loop init
  ├── EspNowHandler  ── tâche "espNowTask"      (gère pairing, send/recv)
  │
  ├─ mode normal (modeComputer = false) :
  │    ├── JoysticksManager ── tâche "joystickManagerTask"
  │    │      lit ADC, moyenne glissante (NBR_INCR_JOYSTICK=10),
  │    │      construit un ControllerRequestDTO et appelle send_data()
  │    └── ButtonsManager   ── tâche "buttonManagerTask"
  │           ISR + debounce timer pour arming + motor state
  │
  └─ mode computer (modeComputer = true) :
       └── ReadComputer ── tâche "read_pc_task"
              lit UART0 un struct ControllerPacket binaire et l'envoie
              tel quel via EspNowHandler. Utile pour tests automatisés.
```

Le toggle entre modes se fait dans [src/main.cpp](src/main.cpp) avec la variable `modeComputer`.

## Pins matérielles

| Fonction              | GPIO        | Notes                                |
| --------------------- | ----------- | ------------------------------------ |
| Joystick gauche X     | GPIO35 (ADC1_CH6) | input only                     |
| Joystick gauche Y     | GPIO34 (ADC1_CH7) | input only                     |
| Joystick droit X      | GPIO32 (ADC1_CH4) |                                |
| Joystick droit Y      | GPIO33 (ADC1_CH5) |                                |
| Bouton arming         | GPIO14      | ISR + debounce (esp_timer)           |
| Bouton motor state    | GPIO12      | ISR + debounce (esp_timer)           |
| Bouton pairing        | GPIO16      | appui long 5 s → reset MAC peer NVS  |
| LED pairing           | GPIO2       | clignote quand mode association      |

## ESP-NOW & pairing

- Le MAC du drone est persisté en **NVS** (clé interne, voir
  `loadPeerMacFromNvs` / `savePeerMacToNvs` dans [include/EspNowHandler.h](include/EspNowHandler.h)).
- Mode association déclenché par un appui long (`LONG_PRESS_MS = 5000`)
  sur GPIO16. La LED clignote tant qu'on n'a pas reçu un `PAIR_CONFIRM`.
- Format pairing : `PairingPacket { char magic[20]; }` avec `REQ_MAGIC =
  "AERISYS_DRONE_PAIR"` / `RESP_MAGIC = "PAIR_CONFIRM"`. Défini dans
  `esp-lib/include/PairingPacket.h`.
- Une fois pairé, on envoie un `ControllerRequestDTO` sérialisé via
  `EspNowHandler::send_data()`.

## Conventions / pièges

- **Buffer joystick** : `NBR_INCR_JOYSTICK = 10`, moyenne glissante
  par sommes courantes (pas de recalcul complet). Si tu changes la
  taille, mets aussi à jour `TIME_MS_BETWEEN`.
- **DTOs** : `ControllerRequestDTO` et `JoystickModel` vivent dans
  `esp-lib`, **ne pas les redéfinir** ici.
- **Mode computer** : `ReadComputer` redéfinit `NBR_INCR_JOYSTICK` —
  garde les deux valeurs alignées.
- **`LimiteJoystick`** : `-1` désactive le bridage, sinon ratio max
  (ex. `0.4f` = 40 %). Toujours signé float.
- Les libs `esp-lib` / `imu-lib` viennent de GitHub. Pour itérer
  localement, utiliser `lib_extra_dirs = ../esp-lib ../imu-lib` ou
  remplacer le `lib_deps` par un chemin file://.

## Fichiers clés

- [src/main.cpp](src/main.cpp) — point d'entrée, choix du mode
- [include/EspNowHandler.h](include/EspNowHandler.h) + [src/feature/EspNowHandler.cpp](src/feature/EspNowHandler.cpp) — pairing + envoi
- [include/JoysticksManager.h](include/JoysticksManager.h) + [src/feature/JoysticksManager.cpp](src/feature/JoysticksManager.cpp) — lecture ADC + lissage
- [include/ButtonsManager.h](include/ButtonsManager.h) + [src/feature/ButtonsManager.cpp](src/feature/ButtonsManager.cpp) — ISR + debounce
- [include/ReadComputer.h](include/ReadComputer.h) + [src/feature/ReadComputer.cpp](src/feature/ReadComputer.cpp) — bridge UART en mode computer

## Tests

Pas de tests unitaires. Le dossier [test/](test/) ne contient qu'un README placeholder PlatformIO.
