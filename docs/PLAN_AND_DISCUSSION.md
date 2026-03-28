# План и обсуждение: SpaceshipCrew (UE5)

Файл для возврата к контексту позже. Не заменяет официальный план в Cursor — фиксирует договорённости из переписки и реализацию.

## Идея игры

- В духе **Barotrauma**: кооперативное выживание на корабле, системы (энергия, атмосфера, повреждения), роли экипажа.
- Отличие: **вид от третьего лица**, космический корабль, **Unreal Engine 5**.

## Решения по MVP (уточнённые)

1. **Один игрок** в сессии выбирает **одну роль** (слот `PlayerRoleSlotIndex` на Game Mode).
2. **Остальные обязательные роли** закрывают **боты** (упрощённый ИИ: приоритеты, станции, реакция на аварии).
3. **Кооп позже**: те же слоты экипажа; при подключении человека — занятие свободного слота (бот отключается / передаёт управление), без переписывания логики корабля.
4. **Dedicated Server для первого MVP не обязателен**; старт с offline / listen, но код закладывается под репликацию и слоты.

## Архитектура (кратко)

- **Слот экипажа**: роль, тип контроллера (Human / Bot), ссылка на пешку. Реплицируемые данные в `AShipGameState::CrewSlots`.
- **Правило**: системы корабля **не различают** игрока и бота — команды идут через **авторизацию роли** (`UCrewRoleComponent` + `AllowedStationPermissions` на `UCrewRoleDefinition`).
- **Взаимодействия**: `AShipInteractableBase` → `UShipSystemsComponent::ApplyAuthorizedAction(Controller, StationPermission, ActionId)`.
- **Боты**: `UCrewBotBrainComponent` + `ACrewAIController`, те же interactable, что и у игрока.
- **Заглушка коопа**: `IShipCrewSlotPossession::PossessCrewSlot`, `UCrewLobbySubsystem`.

## Этапы разработки (из плана)

| Этап | Содержание |
|------|------------|
| 0 | Проект UE5 (C++ + BP), папки, DataAssets для ролей и миссий. |
| 1 | Слоты экипажа + выбор роли + спавн игрока и ботов. |
| 2 | 3rd-person, взаимодействия, общий путь команд на корабль. |
| 3 | Системы корабля (power / O2 / damage / fire) + UI тревог. |
| 4 | ИИ ботов на 2–3 типа задач + приоритеты при авариях. |
| 5 | 1 карта-корабль, 2–3 простые миссии, директор событий. |
| 6 (позже) | Listen/dedicated, лобби, замена бота на игрока в слоте. |

## Открытые решения (зафиксировать при продолжении)

1. **Один персонаж на игрока** vs переключение между несколькими — для barotrauma-подобного ощущения чаще **одно тело, одна роль**.
2. **Offline-first** до появления второго игрока в сети.

## Критерии готовности MVP (из плана)

- Игрок выбирает роль до старта (или в безопасной зоне).
- Все обязательные роли закрыты ботами без софтлоков.
- Аварии решаемы совместно игроком и ботами.
- Слоты и команды к кораблю не требуют ломки ядра для коопа.

## Что сделано в коде (референс)

- Папка `Source/SpaceshipCrew/Ship/`: роли, манифест, Game Mode / Game State, пешка `AShipCrewCharacter`, системы корабля, interactable, ИИ бота, интерфейс слота, подсистема-заглушка лобби, **`UCrewSpawnMarkerComponent`** + спавн через `BuildCrewSpawnTransforms`.
- `AShipPlayerController`: HUD таймер, **`OnPossess` + `SetViewTarget`** (камера на предзаспавненную пешку), **`ApplyShipViewAndInputDefaults`** (игровой ввод, без курсора, снят ignore look/move — обзор мышью после HUD).
- `Config/DefaultEngine.ini`: **`GlobalDefaultGameMode=/Game/Content/GameModes/BP_ShipGameMode.BP_ShipGameMode_C`**, **`GameDefaultMap` / `EditorStartupMap`** = **`/Game/Content/Maps/Lvl_Ship.Lvl_Ship`** — `Lvl_Ship.umap` создаётся в редакторе (см. `docs/EDITOR_NEXT_STEPS.md` §4.0).

## Точка продолжения (зафиксировано: 2026-03-28, вечер)

**PIE:** персонаж ходит, **камера за пешкой** (`SetViewTarget`), **мышь — Look**, Enhanced Input (`IMC_Default` на `BP_ShipPlayerController`, IA на `BP_ShipCrewCharacter`). HUD `WBP_ShipStatus` на контроллере. **Нюансы (уже починены):** IMC на контроллере; `SetViewTarget` после предспавна; game-only input без курсора.

**Спавн экипажа:** `CrewSpawnTransforms` убран. **`Crew Spawn Marker`** (`UCrewSpawnMarkerComponent`) на `BP_Ship`: вьюпорт + **`Role Id`** или пусто (wildcard). Логика: `BuildCrewSpawnTransforms` в `ShipGameMode`. См. `docs/EDITOR_NEXT_STEPS.md` §4.3.

**Дальше:** по чеклисту редактора — NavMesh, маркеры (если ещё не все роли), затем **§5** станции `ShipInteractableBase`, PIE с ботами.

**Git:** `main` **на 3 коммита впереди `origin`** (не запушено). Верх стека — коммит **`docs: checkpoint — spawn markers, PLAN_AND_DISCUSSION`** (этот файл + `Lvl_Ship.umap`); ниже **`646f97d`** (маркеры спавна), **`889aba9`** (спавн Game Mode). Перед сменой машины: `git push`; точный хэш: `git log -1 --oneline`.

## Ссылка на репозиторий

- Remote: `https://github.com/Skirgus/SpaceshipCrew.git`  
- История: initial `.gitignore`, MVP ship crew, локализация комментариев, фиксы GameMode/HUD/ввод/камера; push с машины разработчика при наличии доступа к GitHub.
