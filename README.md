# image_converter

Швидкий стартовий Win32 GUI-проєкт під Visual Studio 2022 (CMake + vcpkg manifest mode).

## Вимоги
- Windows 10/11 x64
- Visual Studio 2022 з компонентом C++ Desktop
- CMake підтримка у VS (File → Open → Folder)
- vcpkg (manifest mode)

## Підготовка vcpkg
1. Клонувати vcpkg (якщо ще не встановлено):
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
   ```
2. Запустити bootstrap (один раз):
   ```powershell
   C:\vcpkg\bootstrap-vcpkg.bat
   ```
3. Увімкнути інтеграцію з Visual Studio:
   ```powershell
   C:\vcpkg\vcpkg integrate install
   ```
   Це дозволить VS автоматично використовувати manifest (`vcpkg.json`) із цього репозиторію.

## Відкриття у Visual Studio 2022
1. `git clone https://github.com/Pvitaly91/image_convertor.git`
2. Запустити Visual Studio 2022.
3. Обрати **File → Open → Folder** і вказати корінь репозиторію.
4. Дочекатися, поки VS зчитає CMakeLists.txt та підтягне залежності через vcpkg.
   > Назва репозиторію лишилась `image_convertor` (legacy), але CMake-проєкт і таргет називаються `image_converter`.

### Якщо конфігуруєте cmake вручну (не через VS)
- Переконайтесь, що задано шлях до toolchain vcpkg:
  ```powershell
  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
  ```
  або задайте `VCPKG_ROOT` у середовищі — тоді CMakeLists.txt підхопить toolchain автоматично.
  Якщо vcpkg стоїть у типовоvому шляху `C:\vcpkg`, CMakeLists.txt також спробує підхопити його автоматично.

## Збірка
1. У верхній панелі VS обрати **x64-Debug** або **x64-Release**.
2. Натиснути **Build → Build All** (або CMake → Build All).
3. Має зібратися виконуваний Win32 GUI застосунок без додаткових ручних налаштувань.

## Запуск
1. Обрати потрібну конфігурацію (наприклад, x64-Debug).
2. В меню **Debug → Start Without Debugging** (Ctrl+F5) або **Start Debugging** (F5).
3. У вікні:
   - Вставити URL (http/https).
   - Натиснути **Convert**.
   - Статусне поле покаже етапи «Downloading…», «Decoding…», «Saving…», «Done» та виведе шлях до створеного файлу `test.jpg` у папці Pictures.

## Структура
```
/
  CMakeLists.txt
  vcpkg.json
  README.md
  src/
    main.cpp
```

## TODO (реальна конвертація)
- Додати реальне завантаження через WinHTTP (з урахуванням потоків і обробки помилок).
- Декодувати вміст як AVIF за допомогою libavif.
- Закодувати результат у JPEG через libjpeg-turbo і зберегти у Pictures.
- Додати обробку розширення/імені файлу та базову перевірку типу контенту.
