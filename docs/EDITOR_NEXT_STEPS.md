# Что сделать дальше в редакторе Unreal

Чеклист без привязки к версии движка — после открытия проекта `SpaceshipCrew.uproject`.

## 1. Сборка C++

- Сгенерировать файлы проекта и **скомпилировать** модуль `SpaceshipCrew` (Editor).
- Если ошибка по интерфейсу `IShipCrewSlotPossession`: в `ShipCrewSlotPossession.h` при необходимости заменить `GENERATED_IINTERFACE_BODY()` на `GENERATED_BODY()` в I-классе (зависит от версии UE).

## 2. Пешка игрока

- Создать **Blueprint** от `ShipCrewCharacter` (например `BP_ShipCrewCharacter`).
- Назначить **Skeletal Mesh** и **Anim Blueprint** (как у шаблона Third Person: `ThirdPersonCharacter` или свои ассеты).
- В **Class Defaults** Game Mode указать `Crew Pawn Class` = этот Blueprint **или** оставить C++ класс и настроить меш в дочернем BP (предпочтительно BP).

## 3. Ввод: взаимодействие (Interact)

- В BP пешки назначить **`Interact Action`** (`UInputAction`) — отдельный IA для взаимодействия (или продублировать существующий из Third Person).
- Убедиться, что **Input Mapping Context** подключены в `ShipPlayerController` / родительском PC (как в шаблоне Third Person).

## 4. Уровень

- Добавить на сцену **`Ship Actor`** (`AShipActor`) — носитель `UShipSystemsComponent`.
- Добавить **`Nav Mesh Bounds Volume`** и **построить навигацию** (P — Navigation), иначе боты не дойдут до станций.
- При необходимости задать **`Crew Spawn Transforms`** на `Ship Actor` — иначе спавн идёт смещением от актора корабля.

## 5. Станции (interactable)

- Разместить **`Ship Interactable Base`** (`AShipInteractableBase`) или дочерние BP у точек взаимодействия.
- Для каждой станции выставить:
  - **`Owning Ship Actor`** — ссылка на ваш `AShipActor` (или оставить пустым: берётся первый `AShipActor` в мире).
  - **`Required Permission`** — тег права роли, например `Helm`, `Reactor`, `Medical`, `Extinguisher` (совпадает с `Allowed Station Permissions` на роли).
  - **`Action Id`** — строка, обрабатываемая в `UShipSystemsComponent`, например:
    - `AdjustReactor`
    - `RepairHull`
    - `FightFire`
    - `VentAtmosphere`
    - `StartFireEvent` (для теста аварий)
  - **`Magnitude`** — множитель эффекта (по умолчанию 1).

## 6. Game Mode и роль игрока

- В **World Settings** или в дефолтном классе Game Mode проверить **`Ship Game Mode`** (`AShipGameMode`).
- **`Player Role Slot Index`** — какой слот (0…N−1) занимает человек; остальные слоты — боты.
- При необходимости создать **`Ship Crew Manifest`** (Data Asset) со списком `Mandatory Roles` и назначить его в Game Mode вместо ролей по умолчанию из кода.

## 7. Роли (Data Assets, опционально)

- Создать **`Crew Role Definition`** ассеты: `Role Id`, `Display Name`, набор **`Allowed Station Permissions`** — согласовать с **`Required Permission`** на interactable.

## 8. Проверка в PIE

- Запустить игру: должен спавниться игрок в выбранном слоте и боты в остальных.
- Подойти к станции, нажать interact — изменяются показатели на `Ship Systems` (реактор, O₂, корпус, огонь).
- Убедиться, что бот при пожаре / низком корпусе идёт к соответствующей станции (нужны корректные **Action Id** и права роли).

## 9. Git

- Если `git push` не выполнялся: `git remote add origin https://github.com/Skirgus/SpaceshipCrew.git` (если ещё нет), затем `git push -u origin main` с учётом аутентификации GitHub.
