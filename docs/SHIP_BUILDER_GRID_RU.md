# Сетка конструктора и размещение модулей

Конвенции runtime-сборки корабля: сетка панелей, `GridPos`, стыковка, проёмы и версия схемы чертежа.

Связанные документы:

- [SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md](SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md) — авторинг модулей и визуал
- [SHIP_VALIDATION.md](SHIP_VALIDATION.md) — готовность к полёту

---

## 1. Единичная панель сетки

| Ось | Размер (см) | Константа в коде |
|-----|-------------|------------------|
| X, Y | 400 | `ShipBuilderGrid::PanelUnitXY` |
| Z (этаж) | 300 | `ShipBuilderGrid::PanelUnitZ` |

Модуль занимает прямоугольный **footprint** из целых панелей по каждой оси.

---

## 2. CellSize и Size на Definition

| Поле | Смысл |
|------|--------|
| **`CellSize`** | Размер модуля в **панелях** (`FIntVector`, минимум 1 по каждой оси). **Источник истины** в редакторе. |
| **`Size`** | Габарит bbox в см, **производное** от `CellSize`: `Size = CellSize × (400, 400, 300)`. Только для чтения в Details. |

Примеры:

| CellSize | Size (см) |
|----------|-----------|
| 1×1×1 | 400 × 400 × 300 |
| 1×3×1 | 400 × 1200 × 300 |
| 2×3×2 | 800 × 1200 × 600 |

При изменении **Cell Size** в редакторе автоматически пересчитываются `Size` и **Contact Points** (если нет `ContactPointsOverride` в Visual Override).

---

## 3. GridPos — угол footprint

В чертеже (`FShipBuilderPlacedModule.GridPos`) хранится **min-угол** footprint в ячейках сетки `(min X, min Y, min Z)`, а не центр модуля.

Центр модуля в мире:

```
Center = CornerWorld + CellSizeToWorldSize(CellSize) × 0.5
CornerWorld = GridPos × (400, 400, 300)   // по осям XY и Z
```

Новый модуль в цепочку добавляется **сразу после** правого края уже размещённых модулей (`max GridPos.X + CellSize.X`), а не по индексу «номер модуля в списке».

---

## 4. Версия схемы чертежа (SchemaVersion)

| Версия | Семантика `GridPos` |
|--------|---------------------|
| **1** (legacy) | Центр модуля в индексах ячеек |
| **2** (текущая) | Min-угол footprint |

**`FShipBlueprintDocument::CurrentSchemaVersion = 2`**

При открытии чертежа v1 в конструкторе выполняется миграция:

```
GridPos_corner = GridPos_center − CellSize / 2   // покомponentно, целочисленно
```

После миграции связи (`Connections`) пересобираются по геометрическому соседству; чертёж помечается dirty — **сохраните**, чтобы записать schema v2.

---

## 5. Стыковочные точки (panel-sockets)

Каждая внешняя панель грани имеет сокет с именем:

```
{Face}_X{ix}_Y{iy}_Z{iz}
```

Примеры: `Front_X0_Y0_Z0`, `Back_X0_Y1_Z0`, `Top_X0_Y0_Z1`.

Legacy-имена (`Front`, `Back`, …) в старых ассетах по-прежнему сопоставляются с panel-именами при стыковке и отрисовке проёмов.

**Стыковка:** panel-socket к panel-socket по совпадению мировых позиций (snap в конструкторе). На одном этаже вертикальные сокеты (Top/Bottom) не участвуют в горизонтальном snap; между этажами — наоборот.

---

## 6. Проёмы в превью конструктора

| Ситуация | Поведение |
|----------|-----------|
| Два модуля с **`bHasInterior = true`** | Проём на **обеих** сторонах стыка (по panel-socket из `Draft.Connections`) |
| Модуль **с** объёмом + модуль **без** (`bHasInterior = false`, сплошной блок) | Проём **не** режется на стороне модуля с объёмом; сплошной блок остаётся закрытым |
| **Airlock** + `ForcedOpeningSide` | Принудительный проём на выбранной стороне (независимо от соседа) |

Модули без внутреннего объёма рисуются **цельным мешем** по `Size`, без panel-shell.

---

## 7. Вертикальные стыки

- Snap между этажами по Top/Bottom panel-sockets работает.
- Люки на полу/потолке (Top/Bottom panels) отображаются при вертикальной связи.
- **Пандус внутри модуля временно отключён** в превью конструктора (будет доработан позже). Ghost пандуса в **Module Editor Tool** для авторинга сокетов сохранён.

---

## 8. Ключевые файлы (C++)

| Область | Файлы |
|---------|--------|
| Константы сетки | `ShipBuilder/ShipBuilderGridConstants.h` |
| Snap, connections, миграция GridPos | `ShipBuilder/ShipBuilderDomainGlue.cpp` |
| Definition: CellSize, contact points | `ShipModule/ShipModuleDefinition.cpp` |
| Превью, проёмы | `ShipBuilder/ShipBuilderModulePreviewActor.cpp` |
| Ввод, drag, загрузка чертежа | `SpaceshipShipBuilderPlayerController.cpp` |
| SchemaVersion | `ShipBuilder/ShipBlueprintTypes.h` |

---

## 9. Частые проблемы

| Симптом | Причина / решение |
|---------|-------------------|
| Меш 1×1, сокеты «в воздухе» на 1×3 | `CellSize` изменён, а `Size`/Contact Points не синхронизированы — переоткройте Definition или снова задайте Cell Size |
| Чертёж «уехал» после обновления | Старый schema v1 — откройте в конструкторе (миграция), сохраните |
| Проём только с одной стороны | Проверьте `bHasInterior` у обоих модулей; перестройте connections (перетащите модуль) |
| Нет проёма к сплошному блоку | Ожидаемо: interior не режет стену к `bHasInterior=false` |
| Сохранённые connections пропали | Каталог временно недоступен — connections больше **не сбрасываются**; пересборка при следующем Rebuild |

---

*При расхождении с кодом приоритет у исходников.*
