# 07. CI без ручных флагов (фаза B2)

## 1. Что было

Когда `CMakePresets.json` только появился, воркфлоу остались от предыдущей фазы:

| Файл | Проблема |
|---|---|
| `windows.yml` | Шаг «Build pinned Lua 5.4.8» сам скачивает tarball и собирает `lua54.lib` через `cl.exe`, затем передаёт `-DLUA_INCLUDE_DIR=` / `-DLUA_LIBRARY=`. Дерево давно вендорит **5.4.6** через `cmake/lua.cmake`, поэтому библиотека собиралась, линковалась и ни разу не использовалась — минуты работы раннера на мусор. |
| `windows.yml` | Конфигурация руками (`-G "Visual Studio 17 2022" -A x64 -DNUC_ENABLE_SANITIZERS=OFF ...`), то есть CI собирал не то, что собирает `tools/build-windows.ps1`. |
| `ci.yml` | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DNUC_ENABLE_SANITIZERS=ON` — те же флаги, что в пресете `debug`, но выписанные второй раз. Правка пресета не меняла CI. |
| оба | Пути к артефактам (`build/...`, `build-windows/...`) не связаны с `binaryDir` пресета, поэтому перенос сборки на пресеты ломал бы упаковку молча. |

## 2. Требования

1. Оба воркфлоу вызывают **только пресеты**: `cmake --preset X`, `cmake --build --preset X`, `ctest --preset X`. Никаких `-D` в YAML.
2. Ручная сборка Lua удаляется; источник Lua — единственный, `cmake/lua.cmake`.
3. Проверка инкрементальной пересборки (<20 с) остаётся и работает от каталога пресета.
4. Упаковка (`package-windows`, ZIP, установщик) читает бинарник из `build/windows-release`, а не из `build-windows/Release`.
5. Если на раннере нет `ninja`, шаг конфигурации ставит его, а не падает с невнятной ошибкой.
6. Поведение, которое проверяли раньше (ASan/UBSan на Linux, CTest, артефакты), не меняется.

## 3. Реализация

`ci.yml`:

```yaml
- run: cmake --preset debug
- run: cmake --build --preset debug --parallel 2
- run: touch world/raycast.cpp && time cmake --build --preset debug --parallel 2   # тот же порог 20 с
- run: xvfb-run -a ctest --preset debug
```

`windows.yml`:

```yaml
- uses: ilammy/msvc-dev-cmd@v1        # даёт cl.exe, ninja приходит из образа или из choco
- run: cmake --preset windows-release
- run: cmake --build --preset windows-release --parallel 2
- run: ctest --preset windows-release
```

Упаковка берёт `build/windows-release/nuc_app.exe`; установщик и артефакты не меняются.

## 4. Проверка

| Что | Как |
|---|---|
| Linux-джоба использует пресет | в логе есть `--preset debug`, нет `-DCMAKE_BUILD_TYPE` |
| Windows-джоба больше не собирает Lua | в логе нет `lua-5.4.8`, есть скачивание 5.4.6 от FetchContent |
| Тесты те же | 144/144 в обеих джобах |
| Инкремент | строка `Incremental … ms` и порог 20 000 мс |
| Артефакты | `Nuc-lVuc-l-Windows-x64` и `V-C-V-Installer-Windows-x64` на месте |
| Установщик | версия `0.1.0.<run_number>`, smoke-install зелёный |

## 5. Риски

| Риск | Что делаем |
|---|---|
| На `windows-2022` нет Ninja | шаг конфигурации проверяет `Get-Command ninja` и при отсутствии ставит через choco |
| FetchContent качает Lua 5.4.6 на каждом прогоне | принимаем: ~1 с, зато один источник зависимости |
| Пресет `windows-release` не покрывает то, что делал VS-генератор | если сборка упадёт, diagnose по логу, а не откат на флаги |

## 6. Результат

Заполняется после прогона CI.
