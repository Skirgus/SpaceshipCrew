# VisualOverride: стыковка со сменой стены на проём / раздвижную дверь

Связано: [SHIP_BUILDER_GRID_RU.md](SHIP_BUILDER_GRID_RU.md), [SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md](SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md).

## Проблема

Раньше `VisualOverride` с непустым `VisualParts` **полностью** заменял procedural shell: проёмы по `Draft.Connections` не появлялись. Двери, зашитые в VisualParts, были всегда.

## Решение (MVP)

На `FShipModuleVisualPart`:

| Поле | Смысл |
|------|--------|
| `WallSocketName` | Panel-socket стены (`Front_X0_Y0_Z0` …). `None` — не заменяется (пол, декор). |
| `OpeningKind` | `Passage` или `SlidingDoor` |
| `OpeningMesh` | Меш при **открытом** сокете (рамка / открытая дверь) |
| `OpeningActorClass` | BP раздвижной двери (геймплей / Module Editor; в билдере MVP — меш) |
| `bAirtightWhenClosed` | Задел: закрытая SlidingDoor герметична |
| `bAffectsOxygenVolume` | Задел: участвует в графе O₂-объёмов |

В `AShipBuilderModulePreviewActor::RebuildFromDraft` для override-модуля:

1. Собираются открытые сокеты так же, как у procedural shell (`Draft.Connections` + interior-сосед + `ForcedOpeningSide`).
2. Часть с `WallSocketName`:
   - сокет **закрыт** → `Mesh` (глухая стена);
   - сокет **открыт** → `OpeningMesh` (если пусто — дыра без меша).

## Авторство коридора

`Corridor_CustomPanels_01_VisualOverride`:

- Пол / потолок — без `WallSocketName`.
- Left/Right — `SM_Wall_Solid` + Opening `SM_Wall_Door` (thin Y), yaw 0/180.
- Front/Back — `SM_Wall_Solid_FB` + Opening `SM_Wall_Door_FB` (thin X), yaw 0/180.
- Не крутить LR-меш yaw ±90 на Front/Back: при сохранении Transform часто теряет yaw→pitch, и панель встаёт поперёк прохода.

Скрипт: `Content/Python/author_corridor_wall_slots.py`.

## Дальше (кислород / двери)

1. Spawn `OpeningActorClass` в runtime (не только ISM-меш) с состоянием Open/Closed.
2. Раздвижная анимация + input экипажа.
3. Граф hermetic volumes: модули = узлы, `SlidingDoor` с `bAirtightWhenClosed` = рёбра; открытая дверь смешивает O₂, закрытая — нет.
4. Airlock: внешняя сторона всегда `ForcedOpening` / шлюзовая логика.

Пока поля life support **не влияют на симуляцию** — только данные для будущей системы.
