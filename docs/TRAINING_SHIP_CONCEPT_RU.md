# Тренировочный корабль и сценарии классов

## Идея

Один **учебный корабль** (`TrainingVessel`) собирается из тех же `UShipModuleDefinition`, что и игровая сборка (Blender → FBX → VisualOverride → сетка **600×600×400**).  
Карта тренировки **не** строит комнату «из кубиков» и **не** скейлит меши. Она:

1. загружает чертёж корабля из `Content/Data/Ships/`;
2. собирает walkable hull тем же пайплайном, что превью конструктора (`RebuildFromDraft`);
3. включает **сценарий класса** (инженер / позже пилот, стрелок…) поверх этого корпуса.

Механики (occupy консоли, энергосеть, ремонт панели) отрабатываются на реальной геометрии модулей и потом переносятся в кампанию **как есть**.

## Слои

| Слой | Что | Где |
|------|-----|-----|
| Модули | Коридор, шлюз, инженерный отсек… | `/Game/Data/ShipModules/*`, меши Blender |
| Чертёж учебного корабля | JSON `ShipId=TrainingVessel` | `/Game/Data/Ships/TrainingVessel.json` |
| Спавн корпуса | `RebuildFromDraft` + collision | Training GameMode / future T04 play spawner |
| Сценарий | Шаги, objectives, привязка к станциям | `AEngineerTrainingScenario` (и аналоги) |
| Карта | Свет, небо, travel из меню | `/Game/Maps/Training/EngineerTraining` |

Карта общая: `EngineerTraining` (позже можно переименовать в `TrainingHangar`). Меняется только **сценарий** и опционально `InteriorPlacements` / точки интереса.

## Учебный корабль (MVP+)

```
[Шлюз 1×1] — [Груз 2×1] — [Инж. коридор 1×1] — [Мостик 2×1 нос]
Airlock_Training_01 → CargoHold_Training_01 → Corridor_CustomPanels_01 → Bridge_Training_01
```

Чертёж: `Content/Data/Ships/TrainingVessel.json`.  
Меши Blender: `Content/Meshes/TrainingShip/` (`TrainingShip_Modules.blend`).  
Импорт в UE: `py Content/Python/author_training_ship_modules.py`.

| ModuleId | CellSize | Назначение |
|----------|----------|------------|
| `Airlock_Training_01` | 1×1×1 | Шлюз, внешний проём Back |
| `Corridor_CustomPanels_01` | 1×1×1 | Инженерный отсек (станции) |
| `CargoHold_Training_01` | 2×2×1 | Грузовой, ящики |
| `Bridge_Training_01` | 2×1×1 | Нос, кресла пилота и командира |
| `EngineeringBay_01` | 2×2×1 | (опц.) отдельный инж. отсек — `author_engineering_bay_module.py` |

## Сценарии на одном корабле

```
TrainingVessel (чертёж)
    ├── Scenario_Engineer   ← сейчас
    ├── Scenario_Pilot      ← позже
    └── Scenario_Gunner     ← позже
```

GameMode параметр (или Options URL): `Scenario=Engineer`.  
Сценарий ищет станции по тегам / `InteriorPlacements` / ModuleInstanceId (`Engineering`), не по «магическим» координатам пустой карты.

## Что не делаем здесь

- Полный T04 campaign load / бюджет / найм.
- Отдельный «игрушечный» greybox вне модульной системы.
- Масштабирование корпуса под камеру.

## Связь с эпиком инженера

DoD эпика (`docs/ENGINEER_TRAINING_EPIC_PLAN_RU.md`) сохраняется: меню → Инженер → внутри **модульного** корабля → энергия → ремонт → Complete.  
Меняется носитель геометрии: **TrainingVessel**, а не ad-hoc 2×2 room в GameMode.
