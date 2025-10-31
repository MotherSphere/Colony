# Colony Launcher Skeleton

Ce dépôt propose une base Qt 6 minimale pour afficher un lanceur "Colony" vide :
- Navigation verticale à gauche listant des catégories d’applications.
- Barre d’onglets en haut permettant de garder ouvertes les applications lancées.
- Vue centrale affichant soit la grille d’applications d’une catégorie, soit un placeholder pour l’application sélectionnée.

## Prérequis
- Qt 6 (module Widgets) installé et disponible dans le `PATH`/`CMAKE_PREFIX_PATH`.
- CMake ≥ 3.16
- Un compilateur C++17 (Clang, GCC, MSVC…)

## Compilation rapide
```bash
cmake -S . -B build
cmake --build build
```

## Exécution
```bash
./build/colony_launcher
```

Le binaire ouvre une fenêtre avec la structure du launcher ; aucun module fonctionnel n’est encore implémenté.
