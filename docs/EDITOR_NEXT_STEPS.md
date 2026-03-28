# Что сделать дальше в редакторе Unreal

Чеклист без привязки к версии движка — после открытия проекта `SpaceshipCrew.uproject`.

## Прогресс (чтобы продолжить с того же места)

| П. | Тема | Статус |
|----|------|--------|
| 1 | Сборка C++ | Сделано |
| 2 | Пешка `BP_ShipCrewCharacter` (mesh, anim), `Crew Pawn Class` в Game Mode | Сделано |
| 3 | Ввод: IMC на `BP_ShipPlayerController` (в т.ч. мышь для Look), IA на пешке, Interact | Сделано (уточнять Interact на уровне) |
| 4 | Уровень **`Lvl_Ship`**: `ShipActor`, NavMesh, `Crew Spawn Transforms` | **Текущий шаг** (§4.0–4.5) |
| 5–8 | Станции, Game Mode / слот, роли DA, проверка PIE с ботами | Дальше |
| 9 | Git | Актуально при новой машине / remote |

Подробный контекст сессии и коммит: **`docs/PLAN_AND_DISCUSSION.md`** → раздел **«Точка продолжения»**.

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

## 4. Уровень (подробно)

Цель: отдельная карта корабля (**`Lvl_Ship`**), на ней **один** `ShipActor`, **навигация** для ботов, **точки спавна** экипажа (индекс слота = индекс в массиве).

В `Config/DefaultEngine.ini` уже выставлено: **`GameDefaultMap`** и **`EditorStartupMap`** = `/Game/Content/Maps/Lvl_Ship.Lvl_Ship`. Папка контента: **`Content/Content/Maps/`** (в редакторе путь `/Game/Content/Maps`).

**Если проект открылся без файла уровня** (первый раз после pull): открой любой существующий уровень из проекта или пустой шаблон, выполни §4.0 и сохрани — либо временно верни в `DefaultEngine.ini` старый `GameDefaultMap` на `Lvl_ThirdPerson`, пока не создашь `Lvl_Ship`.

### 4.0. Создать и сохранить новый уровень `Lvl_Ship`

1. **File** → **New Level…** → выбери **Open World** или **Empty Level** (для закрытого интерьера корабля чаще удобнее **Empty**).
2. Добавь **базовое освещение** при необходимости: **Place Actors** → **Light** → **Directional Light** + **Sky Atmosphere** / **Sky Light** (или **Environment Light Mixer** — по вкусу).
3. Добавь простой **пол** для теста: **Place Actors** → **Geometry** → **Box** (масштаб по X/Y большой, по Z тонкий), включи у него **collision** (пресет **BlockAll**), чтобы персонажи и nav стояли на поверхности.
4. **File** → **Save Current As…** → папка **`Content/Content/Maps`** (в браузере: `All` → `Content` → `Content` → `Maps`), имя файла **`Lvl_Ship`**. Должен получиться ассет **`/Game/Content/Maps/Lvl_Ship`**.
5. **Edit** → **Project Settings** → **Maps & Modes** (опционально): убедись, что **Default Maps** совпадают с `DefaultEngine.ini` (или не трогай — ini уже задаёт стартовую карту).
6. **Window** → **World Settings** → **GameMode Override** = **`BP_ShipGameMode`** (если ещё не стоит на этом уровне).

Дальше на этой же карте выполняй **§4.1–4.5**.

### 4.1. Корабль: `BP_Ship` (рекомендуется)

После **компиляции C++** у `Ship Actor` есть корневой компонент **`HullMesh`** (`Static Mesh`) и **`ShipSystems`**. Удобнее собирать корабль как Blueprint.

1. **Content Browser** → папка **`Content/Content/Ships`** (создай при необходимости) → ПКМ → **Blueprint Class** → **All Classes** → родитель **`Ship Actor`**.
2. Имя: **`BP_Ship`**, сохрани в эту папку (`/Game/Content/Ships/BP_Ship`).
3. Открой **`BP_Ship`** → выдели **`HullMesh`**:
   - **Static Mesh** — любой меш корпуса / палубы / куб-заглушка.
   - **Collision** — **Use Complex Collision as Simple** или **BlockAll** на простых мешах; чтобы персонажи стояли на палубе и Nav строился по ней.
4. При необходимости подправь **Location / Scale** `HullMesh` (корень актора = этот компонент).
5. На уровне **`Lvl_Ship`**: удали старый голый `Ship Actor`, если был → перетащи **`BP_Ship`** из Content Browser на сцену.
6. Выставь **Transform** корабля относительно **пола** уровня (капсула персонажа ~**92** см над поверхностью палубы — ориентир из кода спавна).
7. Компонент **`ShipSystems`** — без обязательных правок; реплицируемые показатели реактора, O₂, корпуса, огня.

**Без Blueprint:** можно по-прежнему класть на уровень **`Ship Actor`** (C++) и назначить меш на **`HullMesh`** у инстанса в **Details**.

### 4.1a. Только нативный `Ship Actor`

Если BP не нужен: **Place Actors** → **`Ship Actor`** → в **Details** на **`HullMesh`** назначь **Static Mesh** и коллизию, как выше.

### 4.2. Nav Mesh Bounds Volume (боты)

1. **Place Actors** → **Volumes** → **`Nav Mesh Bounds Volume`**.
2. Растянуть **Brush** так, чтобы он **полностью покрывал** зону, где ходят персонажи (палуба + коридоры к станциям). Объём должен **охватывать воздух над полом** — nav строится по walkable поверхностям внутри бокса.
3. Меню **Build** → **Build Paths** (или полный **Build**). Во вьюпорте: **P** — показать **navigation mesh** (зелёная заливка на полу).
4. Если зелени нет: проверьте **RecastNavMesh** в **World Outliner**, коллизию пола (**BlockAll** / пресет с **NavMesh**), что volume не слишком маленький.

### 4.3. `Crew Spawn Marker` на `BP_Ship` (визуальные точки)

На корабле собираются все **`Crew Spawn Marker`**. Порядок спавна:

1. Маркеры **с заданным `Role Id`** (как у **`Crew Role Definition`**) — по одному на подходящий слот, в порядке обхода компонентов на корабле.
2. Маркеры **без `Role Id`** (пусто) — на **любые** ещё не занятые слоты, по возрастанию индекса слота (0, 1, 2…).
3. Слоты без маркера — **`ShipLocation + (150 * i, 0, 92)`** в **мировых** осях и поворот как у корабля.

1. Открой **`BP_Ship`** (или другой блюпринт корабля).
2. **Add** → **`Crew Spawn Marker`**. Повесь на **`HullMesh`** (или корень), чтобы точка ехала с кораблём.
3. В **Viewport** расставь маркеры на палубе; **Rotation** задаёт направление взгляда пешки.
4. **`Role Id`**: либо точное имя роли (`Captain`, `Engineer`, … из манифеста / Game Mode), либо **оставь пустым** — туда попадёт любой оставшийся по слотам персонаж после раздачи точечных маркеров.

**Высота:** центр капсулы персонажа окажется в **точке маркера** — подними маркер так, чтобы капсула не застревала в полу (часто чуть выше видимой палубы).

В редакторе у маркера включена **визуализация** компонента (иконка/ось), чтобы проще целиться в сцене.

### 4.4. `PlayerStart` (резерв)

Если **нет** `ShipActor` в мире, fallback-позиция спавна берётся от **первого `PlayerStart`**. Держите на уровне **хотя бы один** `PlayerStart` в безопасной точке — на случай пустой карты или отладки.

### 4.5. Проверка перед пунктом 5

- **PIE**: игрок и боты появляются **на палубе**, не в воздухе и не в геометрии.
- **P**: nav виден там, где должны ходить боты.
- **HUD** (если уже настроен): показатели корабля читаются — значит цепочка до `ShipActor` / `ShipSystems` жива.

Дальше по чеклисту — **§5: станции** (`ShipInteractableBase`) внутри зоны NavMesh и согласованные **Action Id** / права.

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
- В **`BP_ShipGameMode` → Class Defaults** поле **`Crew Pawn Class`** должно указывать на **`BP_ShipCrewCharacter`** (не *None*). Если оно пустое, спавн экипажа не сработает.
- В **Graph** у `BP_ShipGameMode` не должно быть переопределённых **Event Spawn Default Pawn** / **Restart Player** без вызова **Parent** — иначе движок не получит пешку слота игрока из C++ и останется «голая» камера.
- **`Player Role Slot Index`** — какой слот (0…N−1) занимает человек; остальные слоты — боты. Если в **Crew Manifest** только **3** роли, будет **3** пешки и слоты **0…2**; индекс игрока не должен выходить за этот диапазон.
- При необходимости создать **`Ship Crew Manifest`** (Data Asset) со списком `Mandatory Roles` и назначить его в Game Mode вместо ролей по умолчанию из кода.

## 7. Роли (Data Assets, опционально)

- Создать **`Crew Role Definition`** ассеты: `Role Id`, `Display Name`, набор **`Allowed Station Permissions`** — согласовать с **`Required Permission`** на interactable.

## 8. Проверка в PIE

- Запустить игру: должен спавниться игрок в выбранном слоте и боты в остальных.
- Подойти к станции, нажать interact — изменяются показатели на `Ship Systems` (реактор, O₂, корпус, огонь).
- Убедиться, что бот при пожаре / низком корпусе идёт к соответствующей станции (нужны корректные **Action Id** и права роли).

## 9. Git

- Если `git push` не выполнялся: `git remote add origin https://github.com/Skirgus/SpaceshipCrew.git` (если ещё нет), затем `git push -u origin main` с учётом аутентификации GitHub.
