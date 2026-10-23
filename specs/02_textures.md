# Фаза T — индустриальные текстуры 32×32

Статус: спецификация на ревью. Следующая после B1.

## 1. Цель

Заменить плоскую заливку граней (`MeshVertex::material` → константный цвет в шейдере) на процедурные текстуры 32×32 пикселя в индустриальном стиле: ржавый металл, клёпаные плиты, краска по металлу, бетон, техжидкость. Никаких PNG на диске: текстуры генерируются в рантайме из детерминированного хэша, поэтому их нельзя «потерять», они не весят в репозитории и одинаковы на всех машинах.

Ограничения, которые формируют дизайн:

- правило 2 (файлы < 200 строк) — генератор, упаковка и загрузка в GL разнесены по трём модулям;
- правило 3 — генерация покрыта юнит-тестами без GPU: байты, палитра, бесшовность;
- правило про детерминизм — один и тот же `material id` даёт одни и те же пиксели на любой платформе (никакого `rand()`, только целочисленный хэш).

## 2. Визуальный язык (стиль)

Все тайлы рисуются в одной палитре: приглушённый металл, тёмные швы, высветленные фаски, редкие яркие акценты (медь, ржавчина, предупреждающий жёлтый). Свет всегда «сверху-слева», поэтому фаски верхней и левой кромки светлее, нижней и правой — темнее: плитка читается как объёмная даже при плоском освещении.

| Материал | id | Рецепт тайла |
|---|---|---|
| `stone` | 1 | клёпаная стальная плита: тёмный шов по периметру, 4 заклёпки в углах, диагональные царапины, лёгкая зернистость |
| `soil` | 2 | ржавый лист: пятна коррозии, потёки вниз, отслоившаяся краска по краям |
| `grass` | 3 | крашеная антискользящая плита: зелёная краска, ромбический насечка-рисунок, стёртости до металла по кромкам |
| `water` | 4 | техжидкость в смотровом стекле: тёмный сине-зелёный объём, каустические блики, вертикальные пузырьки; альфа 170 |
| `copper_ore` | 10 | бетонная матрица с вкраплениями медного цвета и окисленной зеленью |
| `iron_ore` | 11 | тёмная порода со стальными блёстками и тонкими прожилками |
| `air` | 0 | не рисуется, слоя нет |

Каждый тайл дополнительно получает общий «индустриальный» слой: тонкая сетка швов 8×8 пикселей с вероятностью 1/3 и слабый вертикальный градиент, чтобы тайлы не выглядели как независимые картинки.

## 3. Модули

### 3.1 `render/texture_gen.h/.cpp` — рисование тайлов

```cpp
namespace nuc::render {
struct TileRequest { std::uint32_t material; TilePattern pattern; };
enum class TilePattern : std::uint8_t { riveted_plate, rusted_sheet, painted_grate, coolant, ore_in_concrete, dark_rock };
struct RgbaTile { std::array<std::uint8_t, 32 * 32 * 4> pixels; };   // row-major, RGBA8
[[nodiscard]] RgbaTile generate_tile(TilePattern pattern, std::uint32_t material);
[[nodiscard]] TilePattern pattern_for(std::uint32_t material) noexcept;  // id -> рецепт
}
```

- Внутри — только целочисленная арифметика и `splitmix64`-хэш от `(material, x, y, salt)`; функция чистая, без состояния и без глобальных таблиц.
- Один тайл генерируется < 1 мс (32·32 = 1024 пикселя, десяток проходов).

### 3.2 `render/texture_atlas.h/.cpp` — упаковка

```cpp
namespace nuc::render {
struct TextureAtlas {
    static constexpr int tile_size = 32;
    int layers{};
    std::vector<std::uint8_t> rgba;                       // layers * 32 * 32 * 4
    std::unordered_map<std::uint32_t, int> layer_of;      // material id -> слой
    [[nodiscard]] const std::uint8_t* tile(int layer) const noexcept;
};
[[nodiscard]] TextureAtlas build_atlas(const std::vector<std::uint32_t>& materials);
}
```

Слои — это `GL_TEXTURE_2D_ARRAY`: 32×32×N без атласных швов и без bleed между тайлами. Порядок слоёв = порядок `materials`, поэтому `layer_of` стабилен и проверяется тестом.

### 3.3 `render/gl/texture_upload.h/.cpp` — GPU

```cpp
namespace nuc::render::gl {
class TextureArray {
public:
    TextureArray() = default;
    ~TextureArray();
    TextureArray(TextureArray&&) noexcept;
    TextureArray& operator=(TextureArray&&) noexcept;
    TextureArray(const TextureArray&) = delete;
    bool upload(const ::nuc::render::TextureAtlas& atlas);
    void bind(std::uint32_t unit) const noexcept;
    [[nodiscard]] bool ready() const noexcept { return texture_ != 0; }
private:
    void reset() noexcept;
    std::uint32_t texture_{};
};
}
```

В `render/gl/gl_api.h` добавляются ровно нужные указатели: `gen_textures`, `delete_textures`, `bind_texture`, `tex_image_3d`, `tex_parameteri`, `active_texture`, `pixel_storei`, `generate_mipmap`, `uniform1i`. Параметры: `GL_NEAREST_MIPMAP_NEAREST` для минификации (стиль voxel-art, без мыла), `GL_NEAREST` для магнификации, `GL_REPEAT` по S/T.

### 3.4 Меш и шейдер

- `MeshVertex` получает `float u, v;` (0..1 на грань) и сохраняет `material`; `GpuVertex` становится `{position[3], normal[3], uv[2], layer}` = 9 float.
- Вершинный шейдер: `layout(location=2) in vec2 a_uv; layout(location=3) in float a_layer;` → `flat out int v_layer; out vec2 v_uv;`.
- Фрагментный шейдер: `uniform sampler2DArray u_atlas; vec4 t = texture(u_atlas, vec3(v_uv, v_layer));` затем то же направленное освещение `0.35 + 0.65·max(dot(n, L), 0)`.
- UV не масштабируются: один тайл на грань. Позже (LOD) — тайлинг по размеру грани.

## 4. Тест-кейсы (Catch2, без GPU)

| # | Тест | Что доказывает |
|---|---|---|
| T1 | `atlas covers every registry material and 32×32 per layer` | нет «пустых» блоков: каждому материалу из `data/materials.json` соответствует слой |
| T2 | `tile generation is deterministic` | два вызова дают побайтово равные тайлы |
| T3 | `tiles are not flat noise` | stddev по каналам выше порога (иначе регресс «всё серое») |
| T4 | `tile palettes match the industrial style` | оттенок/насыщенность в ожидаемом диапазоне: сталь серая, ржавчина 15–35°, краска 90–150°, вода синяя |
| T5 | `tiles wrap seamlessly at the edges` | средняя разница левой/правой и верхней/нижней кромок меньше порога |
| T6 | `atlas dimension stays inside GL limits` | `32 * layers <= 256` (запас под будущие материалы) |
| T7 | `generation stays under five milliseconds` | тайминг на релизе, без флаки: порог с запасом в 10× |

## 5. Порядок работ

1. `texture_gen` + тесты T2–T5 (без GL).
2. `texture_atlas` + T1, T6.
3. `gl_api` + `TextureArray` + правки `chunk_mesh`/`mesh_upload`/шейдеров.
4. Прогон приложения, скриншот в `specs/img/textures_32.png`, запись в `specs/00_audit.md`.
5. Обновление `ROADMAP.md`, коммит.

## 6. Риски

- **Вода**: сейчас материал 4 полупрозрачный, но рендер не сортирует прозрачные грани. Решение фазы T: рисуем воду как непрозрачную с альфой 1.0 в тайле, а настоящую прозрачность отдаём в фазу J (порядок отрисовки, back-to-front по чанкам).
- **Стиль против читаемости**: мелкий 32×32 тайл на дальней дистанции превращается в шум; мипмапы обязательны, иначе FPS-картинка хуже плоских цветов. Проверяется скриншотом с высоты.
- **`glTexImage3D` на старых драйверах**: OpenGL 3.3 гарантирует массивы текстур, но при отказе создавать массив рендер обязан не падать, а откатиться на текущую заливку цветом (флаг `TextureArray::ready() == false`).
