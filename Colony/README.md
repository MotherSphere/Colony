# Colony

Colony est un simple éditeur de texte sous tkinter (non complet)

## Structure du projet

```
Colony
├── src
│   ├── main.py          # Point d'entrée de l'application
│   └── app.py           # Classe principale de l'application
├── requirements.txt     # Dépendances (facultatif)
├── pyproject.toml       # Configuration du projet (facultatif)
└── README.md            # Documentation (ce fichier)
```

## Prérequis

- Python 3.8+ installé.
- (Optionnel) créer un environnement virtuel :
  - python -m venv .venv
  - .venv\Scripts\activate

Installer les dépendances si nécessaire :
```
pip install -r requirements.txt
```

## Exécution

Lancer l'application :
```
python src/Colony.py
```
ou
```
python -m src.Colony
```

## Tests

Exécuter les tests unitaires :
```
python -m unittest discover -s tests
```

## Licence

Projet open‑source — Mais a créditer