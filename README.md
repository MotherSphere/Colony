# Colony

Colony est un prototype de plateforme modulaire en C++ qui simule un App Center capable de gérer l'installation de modules applicatifs, leur sécurité et la publication sur une place de marché. Le dépôt propose une bibliothèque statique `colony` accompagnée d'un exécutable de démonstration `colony_demo`.

## Fonctionnalités principales

- **Catalogue local** : chargement de manifestes JSON décrivant des applications (nom, version, permissions, dépendances) via `AppCenter`.
- **Registre de modules** : enregistrement de fabriques de modules, suivi des manifestes installés et instanciation à la volée avec `ModuleRegistry`.
- **Sécurité** : traçage des permissions accordées à chaque module grâce à `SecurityManager`.
- **Installation** : résolution basique des dépendances et gestion de l'état installé via `ModuleInstaller`.
- **Marketplace** : publication de packages et détection de mises à jour disponibles avec `MarketplaceClient`.

Le programme de démonstration (`src/main.cpp`) illustre un flux complet :
1. Chargement des manifestes de `manifests/` dans l'App Center.
2. Installation des applications en utilisant le registre et le gestionnaire de sécurité.
3. Publication des applications locales sur la marketplace et détection d'éventuelles mises à jour.

## Structure du dépôt

```
.
├── include/colony/   # Interfaces publiques de la bibliothèque (AppCenter, ModuleRegistry, etc.)
├── src/              # Implémentations de la bibliothèque et programme de démonstration
├── manifests/        # Manifestes JSON d'exemple pour les modules fournis
└── CMakeLists.txt    # Configuration CMake pour construire la bibliothèque et l'exécutable
```

## Construire le projet

Les instructions suivantes utilisent CMake et un compilateur C++17 :

```bash
cmake -S . -B build
cmake --build build
```

L'exécutable résultant `build/colony_demo` peut ensuite être lancé pour visualiser le catalogue et le processus d'installation simulé.

```bash
./build/colony_demo
```

## Ajouter un nouveau module

1. Créez une classe dérivée de `colony::core::Module` dans `src/modules/` et exposez son en-tête dans `include/colony/modules/`.
2. Enregistrez une fabrique pour ce module dans `main.cpp` (ou dans votre application hôte) via `ModuleRegistry::registerFactory`.
3. Ajoutez un manifeste JSON décrivant le module dans `manifests/`.
4. Ajoutez le fichier source au `CMakeLists.txt` si nécessaire.

Après recompilation, le nouveau module apparaîtra dans le catalogue et pourra être installé par le démonstrateur.

## Licence

Aucune licence explicite n'est fournie dans ce dépôt. Veillez à clarifier la licence avant toute utilisation en production.
