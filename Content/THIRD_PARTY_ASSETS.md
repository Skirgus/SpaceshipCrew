# Сторонние ассеты (Fab / Marketplace)

В репозиторий **не коммитятся** сырые файлы купленных паков (лицензия Fab/Epic и размер).  
В git хранятся только **собственные** data-ассеты проекта (`Content/Data/...`) и ссылки на паки ниже.

Каждый разработчик устанавливает паки **локально** в `Content/ThirdParty/<ИмяПака>/`.

---

## Git LFS

Крупные бинарники Unreal (`.uasset`, текстуры и т.д.) настроены через [`.gitattributes`](../.gitattributes) в корне репозитория.

### Один раз на машине

```powershell
git lfs install
```

### После клонирования репозитория

```powershell
git lfs pull
```

Если LFS-файлы не подтягиваются (маленькие pointer-файлы вместо ассетов) — проверьте, что установлен [Git LFS](https://git-lfs.com/) и выполнен `git lfs install`.

### Квота GitHub

На бесплатном аккаунте LFS ограничен по **хранилищу** и **трафику** в месяц. Не добавляйте в git целые Fab-паки — только проектные ассеты.

---

## Big Modular SciFi Interior

| | |
|---|---|
| **Назначение** | Модульный sci-fi интерьер: коридоры, комнаты, панели — визуал модулей конструктора (`UShipModuleVisualOverride`) |
| **Fab** | [Big Modular SciFi Interior Model Pack](https://www.fab.com/listings/76891a02-69fa-494e-a2c3-ea76e66cf641) |
| **Папка в проекте** | `Content/ThirdParty/BigModularSciFi/` |
| **UE** | 5.0–5.7 (проверить импорт в 5.7 после добавления) |
| **В git** | ❌ не коммитить содержимое папки |

### Установка (новый разработчик)

1. Купить / добавить пак в Epic Games Launcher (Fab → Library → Add to Project).
2. Добавить в проект **SpaceshipCrew** или импортировать вручную.
3. Переместить (или оставить с symlink) контент под:
   ```
   Content/ThirdParty/BigModularSciFi/
   ```
   Рекомендуется не смешивать с `Content/Data/` — там только игровые data-ассеты (`ShipModuleDefinition` и т.д.).
4. Открыть проект в UE 5.7, дождаться компиляции шейдеров.
5. Убедиться, что папка **не** попала в git:
   ```powershell
   git status
   ```
   Пути под `Content/ThirdParty/BigModularSciFi/` не должны быть в staged.

### Использование в конструкторе

- Меши из пака → `UShipModuleVisualOverride.VisualParts` (поля `Mesh`, `RelativeTransform`).
- Стыковочные точки → panel-sockets (`Front_X0_Y0_Z0`, …) в `ContactPoints` / `ContactPointsOverride`.
- Размер модуля → **`CellSize`** на Definition (см. [docs/SHIP_BUILDER_GRID_RU.md](../docs/SHIP_BUILDER_GRID_RU.md)).
- **Пошаговое руководство:** [docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md](../docs/SHIP_MODULE_BIG_MODULAR_GUIDE_RU.md)
- Лицензия: только для сборки **вашей** игры; не публиковать сырые `.uasset` пака в открытом репозитории.

### При добавлении нового пака

Скопируйте блок «Big Modular…» выше, заполните таблицу и при необходимости добавьте исключение в [`.gitignore`](../.gitignore) (или оставьте общее правило `Content/ThirdParty/**`).

---

## Чеклист перед commit

- [ ] Нет файлов из `Content/ThirdParty/` в `git add` (кроме `README.md` / `.gitkeep`)
- [ ] `git lfs status` без неожиданных больших файлов вне LFS
- [ ] Новые `.uasset` проекта лежат в `Content/Data/` (или другом согласованном пути), не в ThirdParty
