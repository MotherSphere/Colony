# Programs Module

This directory hosts self-contained program launchers that extend the Colony ecosystem. Each
program is designed to be initialized by the main SDL application and can ship its own services,
UI flow, and storage strategies.

Current layout:

- `archive/` – scaffolding for the **Archive Vault** secure storage experience.

Future programs should live next to `archive/`, sharing common patterns (state machines,
service locators, UI components) while remaining isolated from the core runtime until launched.
