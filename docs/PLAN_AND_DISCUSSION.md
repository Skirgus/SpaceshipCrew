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
- **Станции / точки взаимодействия** — **внутри `BP_Ship`** (дочерние акторы или компоненты), а не разбросаны по уровню: корабль = единый переносимый объект, `ShipSystems` и посты живут в одном месте. На уровне — сам инстанс `BP_Ship` (и окружение).
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

## Точка продолжения (зафиксировано: 2026-03-29)

**Этап закрыт:** объекты-станции расставлены (в т.ч. в `BP_Ship`), к ним можно подойти и нажать interact; **параметры `UShipSystemsComponent`** меняются на сервере и **отображаются в HUD** (`WBP_ShipStatus`). Цепочка: `UCrewInteractionComponent` → `ServerTryInteract` → `AShipInteractableBase::ExecuteInteract` → `ApplyAuthorizedAction`. Подсказка на экране согласована с тем же набором проверок, что и перед RPC.

**Ввод:** `IMC_Default` с **`IA_Interact`** (клавиша E), на **`BP_ShipCrewCharacter`** в Class Defaults — **Auto Receive Input = Player 0** (или эквивалент), поле **Interact Action** = тот же IA. Привязка Interact в C++ — отложенно после `BeginPlay` / из `SetupPlayerInputComponent`, триггер **Triggered**.

**Git checkpoint:** аннотированный тег **`checkpoint/ship-stations-hud`** (текущий хэш: `git rev-parse checkpoint/ship-stations-hud`; основной функционал в коммите **`a245008`**). Вернуться к снимку: `git checkout checkpoint/ship-stations-hud` или ветка от тега: `git switch -c имя-ветки checkpoint/ship-stations-hud`.

**Дальше по чеклисту:** `docs/EDITOR_NEXT_STEPS.md` — пункты **§6–8** (уточнение Game Mode / ролей / PIE с ботами у станций), затем развитие ИИ и миссий по плану этапов.

### Предыдущая точка (2026-03-28)

PIE с ходьбой, камерой, IMC и HUD без станций; спавн через **Crew Spawn Marker**; см. историю коммитов до тега выше.

## Ссылка на репозиторий

- Remote: `https://github.com/Skirgus/SpaceshipCrew.git`  
- История: initial `.gitignore`, MVP ship crew, локализация комментариев, фиксы GameMode/HUD/ввод/камера; push с машины разработчика при наличии доступа к GitHub.
