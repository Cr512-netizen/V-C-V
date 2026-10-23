# Phase B1 — воспроизводимая сборка

Статус: **proposed — awaiting review**. Код не пишется до утверждения.
Основание: решения ревью от 2026-10-22 (git baseline `707882f`, аудит `specs/00_audit.md`).

## 1. Задача и границы

Сделать так, чтобы на **чистой машине** проходило ровно это:

```
cmake --preset debug && cmake --build --preset debug && ctest --preset debug
```

Сегодня это не работает: нет `CMakePresets.json`, Lua не вендорится, CMake 4 останавливается на `FetchContent_Populate`. B1 закрывает только это. Сеть, физика и LOD в B1 не входят.

## 2. Что уже решено ревью

| Вопрос | Решение |
|---|---|
| Lua | вендорить через FetchContent, **5.4.6**; `option(USE_SYSTEM_LUA OFF)` по умолчанию; `find_package(Lua 5.4)` только при `ON` |
| Политика CMake | `CMAKE_POLICY_VERSION_MINIMUM=3.5` — в пресетах, не в README |
| Git | `git init` + baseline сделано (`707882f`), `.gitignore` дополнен `*.lib`, `*.obj`, `*.pdb` |

## 3. Что появится в дереве

| Файл | Назначение | Оценка строк |
|---|---|---|
| `CMakePresets.json` | пресеты конфигурации/сборки/тестов | ~90 |
| `cmake/lua.cmake` | вендоринг Lua 5.4.6 и target `Lua::Lua` | ~60 |
| `cmake/dependencies.cmake` | все `FetchContent_Declare` в одном месте | ~40 |
| `.clang-format` | стиль Google с поправками | ~20 |
| `CMakeLists.txt` | правки: язык `C`, `include(cmake/*.cmake)`, `Lua::Lua` вместо `${LUA_LIBRARIES}` | было 74 → ~70 |
| `README.md` | раздел «Сборка»: одна команда + требования | правка |
| `tools/build.sh`, `tools/test.sh` | тонкие обёртки над пресетами | правка |
| CI (`ci.yml`, `windows.yml`) | перевести на пресеты | правка |

## 4. `CMakePresets.json` — интерфейс

`"version": 6`, `cmakeMinimumRequired: 3.24`.

### Пресеты конфигурации

| Пресет | Назначение | generator | cacheVariables |
|---|---|---|---|
| `base` (hidden) | общее для всех | — | `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, `NUC_BUILD_TESTS=ON`, `USE_SYSTEM_LUA=OFF`, `CMAKE_POLICY_VERSION_MINIMUM=3.5` |
| `debug` | локальная разработка | `Ninja` | + `CMAKE_BUILD_TYPE=Debug`, `NUC_ENABLE_SANITIZERS=ON` |
| `release` | прогон производительности | `Ninja` | + `CMAKE_BUILD_TYPE=Release`, `NUC_ENABLE_SANITIZERS=OFF` |
| `windows-debug` | MSVC из Developer PowerShell | `Ninja` | наследует `debug` |
| `windows-release` | MSVC, релиз | `Ninja` | наследует `release` |

`binaryDir` — `${sourceDir}/build/${presetName}`, чтобы debug и release не затирали друг друга.

**Про ccache.** `CMAKE_CXX_COMPILER_LAUNCHER` в пресет **не** кладу: если ccache не установлен, CMake падает с ошибкой вместо того, чтобы просто собрать. Остаётся существующий `find_program(ccache)` в `CMakeLists.txt`, к нему добавляется `CMAKE_C_COMPILER_LAUNCHER` — иначе C-часть (Lua, GLFW) кэшируется хуже C++.

**Про санитайзеры.** `NUC_ENABLE_SANITIZERS` применяется только к non-MSVC (как сейчас): у MSVC нет UBSan, а `-fsanitize=address,undefined` — синтаксис GCC/Clang. В пресете `windows-debug` переменная остаётся `ON`, но на MSVC игнорируется; это записывается комментарием в `CMakeLists.txt`, чтобы не выглядело багом.

### Пресеты сборки и тестов

| Пресет | Тип | Параметры |
|---|---|---|
| `debug`, `release`, `windows-debug`, `windows-release` | build | `configurePreset` с тем же именем, `jobs` из окружения |
| `debug`, `release` | test | `output.outputOnFailure=true`, `execution.timeout=300` |

`ctest --preset debug` должен находить бинарь через `configurePreset`, без ручного `--test-dir`.

## 5. Вендоринг Lua (`cmake/lua.cmake`)

```
option(USE_SYSTEM_LUA "Use a system Lua 5.4 instead of the vendored copy" OFF)
```

- `USE_SYSTEM_LUA=ON` → `find_package(Lua 5.4 REQUIRED)`, target `Lua::Lua` = `INTERFACE` с `${LUA_INCLUDE_DIR}` и `${LUA_LIBRARIES}`.
- `USE_SYSTEM_LUA=OFF` → `FetchContent_Declare(lua URL https://www.lua.org/ftp/lua-5.4.6.tar.gz URL_HASH SHA256=7D5EA1B9CB6AA0B59CA3DDE1C6ADCB57EF83A1BA8E5432C0ECD06BF439B3AD88)`.
  Хэш посчитан мной 2026-10-22 на скачанном файле (363 329 байт) — это не выдуманное значение.
  В архиве **нет** `CMakeLists.txt`, поэтому `FetchContent_MakeAvailable` не применяется: только `FetchContent_Populate` и явный список из 32 файлов `src/*.c` (всё, кроме `lua.c`, `luac.c`, `onelua.c`).
  Цель: `add_library(lua_vendored STATIC ...)` + `add_library(Lua::Lua ALIAS lua_vendored)`, `PUBLIC` include на `${lua_SOURCE_DIR}/src`.
  Дефайны: `LUA_USE_WINDOWS` для MSVC, `LUA_USE_POSIX` для остальных (readline не нужен: он только в `lua.c`, который не собирается).

Следствие, которое надо внести в корневой `CMakeLists.txt`: `project(... LANGUAGES C CXX)` — сейчас указан только `CXX`, а Lua компилируется как C.

`nuc_script` переключается с `${LUA_INCLUDE_DIR}`/`${LUA_LIBRARIES}` на `Lua::Lua` — тогда файло-независимо от того, вендорная Lua или системная.

## 6. `.clang-format`

```
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 120
AllowShortFunctionsOnASingleLine: All
AllowShortIfStatementsOnASingleLine: WithoutElse
PointerAlignment: Left
SortIncludes: CaseSensitive
Standard: c++20
```

`ColumnLimit: 120`, а не 80: в коде уже есть длинные однострочные тела функций, и 80 превратит их в трудночитаемые обрубки. `IndentWidth: 4` — фактический отступ в репозитории.

## 7. Тест-кейсы чистой сборки

| № | Сценарий | Ожидание |
|---|---|---|
| T1 | пустой каталог сборки, Linux, `cmake --preset debug` | configure зелёный, Lua скачана и собрана как часть проекта |
| T2 | `cmake --build --preset debug` | сборка без ошибок и без сетевого доступа (зависимости уже в кэше) |
| T3 | `ctest --preset debug` | 115/115 (плюс новые тесты B1) |
| T4 | `touch world/raycast.cpp` + пересборка | **< 20 с** (правило 3) |
| T5 | пустой каталог сборки, Windows, Developer PowerShell, `cmake --preset windows-debug` | configure зелёный без флага `-DCMAKE_POLICY_VERSION_MINIMUM` на командной строке |
| T6 | `cmake --preset release` | зелёный, санитайзеры выключены |
| T7 | `cmake --preset debug -DUSE_SYSTEM_LUA=ON` при установленной Lua 5.4 | configure зелёный, вендорная копия не скачивается |
| T8 | повторный configure после смены пресета | пресеты не конфликтуют (разные `binaryDir`) |
| T9 | `ctest --preset debug` повторно | стабильно зелёный, тесты не зависят от порядка |

T4 и T1 — те самые критерии брифа (инкремент и «одна команда»).

## 8. Критерии готовности B1

1. `cmake --preset debug && cmake --build --preset debug && ctest --preset debug` проходит на чистой машине **без ручных `-D`**.
2. То же для `release`, и для Windows через `windows-*`.
3. Инкрементальная пересборка одного файла < 20 с.
4. `USE_SYSTEM_LUA=ON` тоже конфигурируется.
5. CI переведён на пресеты и зелёный (или локальный скрипт, если GitHub недоступен).
6. `ROADMAP.md` обновлён, коммит с описанием.

## 9. Риски

| Риск | Смягчение |
|---|---|
| Lua 5.4.6 как C-код в C++-проекте | `LANGUAGES C CXX`, отдельная статическая библиотека, никаких C++-обёрток |
| MSVC и `LUA_USE_WINDOWS` | дефайн только для MSVC; проверяется T5 |
| Порог 20 с на ccache-промахе | мерить после «холодной» сборки, порог применять ко второму прогону (как в текущем `ci.yml`) |
| GLFW на Linux требует X11/Wayland пакетов | список пакетов уже есть в `ci.yml`, переносится в README |

## 10. Три решения, которые прошу подтвердить

1. **Per-module `CMakeLists.txt`.** Бриф требует «верхний + per-module», но у модулей по 1–8 строк исходников: 8 файлов дадут boilerplate без выгоды. Предлагаю вместо этого `cmake/dependencies.cmake` + `cmake/lua.cmake` + один корневой файл. Если нужна буквальная структура — сделаю её.
2. **`ColumnLimit: 120`** против гугловых 80 (обоснование в §6).
3. **Разбиение `app/main.cpp` (338 строк) и `script/mod_loader.cpp` (221).** Это правило 2, но не буквально пункт B1. Предлагаю отдельной задачей `B1-refactor` сразу после пресетов, чтобы не смешивать механика-правки с форматированием.

## 11. Результат (2026-10-23)

| Проверка | Факт |
|---|---|
| `cmake --preset windows-release` + build + `ctest` | **115/115 passed**, 0 failed |
| `cmake --preset windows-debug` + build + `ctest` | **115/115 passed**, 0 failed |
| Инкрементальная пересборка одного файла | **0.14 с** после исправления отслеживания зависимостей (лимит 20 с); до исправления правка `.h` вообще не пересобирала зависимые файлы |
| Отслеживание заголовков | `ninja -t deps` показывает `#deps 2` вместо `#deps 0`; правка `render/texture_canvas.h` пересобирает `texture_gen.cpp` |
| `-DUSE_SYSTEM_LUA=ON` без системной Lua | понятная ошибка `Could NOT find Lua (missing: LUA_LIBRARIES LUA_INCLUDE_DIR)` на `cmake/lua.cmake:18`; путь с системной Lua на этой машине проверить нечем |
| Запуск из обычной оболочки | `tools/build-windows.ps1` сам находит `vcvars64`, `cmake`, `ninja`, `git` |

Что сломалось по дороге и почему:

1. `FetchContent_Populate` пишет `<name>_SOURCE_DIR` **в вызывающий scope**. Обёртка-функция `nuc_fetch_source()` молча давала пути вида `/src/lapi.c`. Убрана: populate вызывается прямо в файле, политика `CMP0169 OLD` ставится один раз на каталог.
2. `cmake_policy(PUSH)` без `POP`: когда скачивание падало, FetchContent делал ранний `return` из функции, POP не выполнялся, и CMake сообщал о «policy imbalance» вместо настоящей ошибки.
3. 15 тестов писали в `%TEMP%`, а дочерний процесс под песочницей ограничен workspace → `Access denied`. Введён `tests/test_scratch.h` с `NUC_TEST_TMP_DIR` внутри дерева сборки: тесты стали герметичными.
4. `%PATH%` внутри одной строки `cmd` раскрывается до `call vcvars64`, поэтому `set PATH=<см>` затирал PATH компилятора. Теперь PATH дополняется в родительском процессе, а в скрипте — до входа в `cmd`.
5. Пресет несёт `CMAKE_POLICY_VERSION_MINIMUM=3.5` не зря: сырой `cmake -S . -B ...` падает на `cmake_minimum_required` в enkiTS ещё до проверки Lua.
6. **Отслеживание заголовков было мертво, и это нашлось только в фазе T.** Русский MSVC печатает `Примечание: включение файла:`, CMake записывает эту строку в `CMakeFiles/rules.ninja` как `msvc_deps_prefix`, а под кодовой страницей консоли по умолчанию (866) строка превращается в мусор (`в•ЁРЇв•¤…`), Ninja перестаёт её узнавать и **не записывает ни одной зависимости**: `ninja -t deps` показывал `#deps 0`, а правка `.h` давала `ninja: no work to do`. `VSLANG=1033` в пресете не помогает — cl сообщения не переводит. Рабочее решение: конфигурировать под `chcp 65001`, тогда префикс записывается читаемым UTF-8 и зависимости восстанавливаются. Проверено на двух минимальных проектах (`build/depprobe`, после проверки удалены) в вариантах 866 / 1251 / 65001: пересборка по правке заголовка срабатывает только в двух последних, корректный префикс — только в 65001. Поэтому `tools/build-windows.ps1` теперь ставит `chcp 65001` перед конфигурацией и **падает**, если `ninja -t deps` для любого объекта `nuc_render` сообщает 0 зависимостей. Первая зелёная приёмка B1 (115/115, «1.6 с») измерялась при сломанном отслеживании, поэтому её цифра инкремента заменена на переснятую.
