# Ðespiтe Tater

Открытый клиент DDNet на базе [TaterClient/TClient](https://github.com/TaterClient/TClient).
Это изменённая версия, самостоятельный проект, не официальный релиз TClient или DDNet.

## Антипинг

Открой **Настройки → TClient → Ðespiтe** и нажми **«Включить антипинг»**.
Выбери один из трёх профилей:

- **Отзывчивый** — быстрее возвращается к полному предсказанию при стабильном соединении.
- **Баланс** — компромисс между отзывчивостью и плавностью коррекций.
- **Плавный** — более осторожное восстановление при скачках задержки.

Адаптивный алгоритм дополняет improved antiping TClient: измеряет изменения времени
предсказания, быстрее реагирует на нестабильность и постепенно возвращается к обычному
режиму. Замеры выполняются один раз за игровой тик, отдельно для основного соединения
и dummy. История сбрасывается после переподключения и больших разрывов тиков.
Ограничения предсказания TClient для игровых режимов сохраняются.

### Настройка для задержки около 100 мс

Нажми **«Настроить для 100 мс»** во вкладке **TClient → Ðespiтe**. Кнопка
включает адаптивный режим, сбалансированный профиль, плавность 80, быстрый ввод,
предсказание dummy, событий и точное прицеливание внутри тика. Устраняются
конфликтующие режимы призраков и снятия предсказания во freeze; сетевой запас
предсказания (`cl_prediction_margin`) сохраняется. Предсказанные звуки/эффекты
работают только при поддержке этого режима сервером.

В 0.3.0 добавлено сглаживание небольших поправок позиции: сравниваются старое
и новое предсказание на одном игровом тике, а визуальное смещение постепенно
убирается. Обычная скорость движения не фильтруется. Смещение ограничено
12 игровыми единицами, проверяется относительно стен и предсказанного пути.
После пропуска тиков, телепортации, смерти или freeze оно сбрасывается.

На близкой дистанции и при взаимодействии хуком приоритет получает точная
предсказанная позиция, согласованная с физикой клиента. Дополнительное смещение
в этой ситуации убирается. Это уменьшает визуальное расхождение при ударе или
зацепе; решение о попадании, уроне и серверном движении остаётся за сервером.
Клиент не выполняет удары или наведение автоматически.

Ползунок плавности сам по себе не включает адаптивный режим. В статусе должно
быть указано, что режим активен. Результат при реальных 100 мс зависит от jitter,
потерь пакетов и действий других игроков; онлайн-сравнение пока не проведено.

В версии 0.2.0 добавлено плавное изменение веса предсказания других игроков.
Экспоненциальный фильтр учитывает прошедшее игровое время, поэтому результат
не зависит от FPS. Ползунок **«Плавность»** регулирует смягчение коррекций:
`0` отключает дополнительный фильтр, `65` — исходное значение, `100` — максимум.
Телепортации, переподключения и пропавшие персонажи сбрасывают историю.
Позиции по-прежнему ограничиваются допустимой областью предсказанного пути;
движение своего персонажа и dummy не пропускается через этот фильтр.

Рекомендуемая настройка включает предсказание игроков, снарядов и pre-inputs,
improved smoothing и быстрый ввод TClient на 20 мс. Старое сглаживание отключается,
чтобы алгоритмы не конфликтовали. Сетевой prediction margin сохраняется.
Кнопка отключения выключает адаптивное восстановление; остальные применённые
настройки TClient остаются доступными в обычных разделах.

**80 мс не превращаются в настоящие 40 мс.** Предсказание может сделать управление
визуально отзывчивее, но не сокращает путь пакетов. На экране показывается исходный
пинг от сервера. Потери пакетов, резкие действия других игроков и серверные коррекции
могут оставаться заметными. Улучшение ощущений требует проверки на реальном сервере;
численного обещания «в два раза быстрее» здесь нет.

## Интерфейс

Главное меню использует тему присланного референса: графитовый фон, мягкие
контуры панелей, небольшие иконки и серебристую кнопку «Играть». Доступны
настройки, демки, редактор и локальный сервер; сохранены горячие клавиши.
В версии Windows сервер включён в комплект.

В 0.2.1 доработаны поверхности кнопок: тонкий светлый контур, мягкая тень,
сдержанный блик и более объёмная серебристая кнопка «Играть» с иконкой.
При наведении кнопки плавно увеличиваются вместе с надписью и иконкой,
при уходе курсора возвращаются назад. Нажатие слегка продавливает кнопку.
Реакция продолжается от текущего состояния даже при быстрых движениях мыши;
длительность не зависит от FPS. Кнопки не сдвигают соседей, область клика
остаётся стабильной. Анимация есть у навигации, профилей, включения антипинга
и нижних ссылок.

Подвкладка антипинга упрощена: включение, три профиля, один ползунок и короткий
статус. Показатели вариации и восстановления раскрываются через «Подробнее».
Нижний HUD выполнен в той же нейтральной палитре и учитывает status bar.
Новые элементы переведены на русский. Визуальная проверка была прервана Escape;
подробности проверок и ограничения — в [отчёте](docs/DESPITE_TESTING.md).

Консольные параметры:

| Команда | Значение |
| --- | --- |
| `tc_despite_antiping` | `0` / `1`, адаптивное восстановление (по умолчанию выключено) |
| `tc_despite_profile` | `0` отзывчивый, `1` баланс, `2` плавный |
| `tc_despite_smoothness` | `0`–`100`, смягчение визуальных коррекций (исходно `65`) |
| `tc_despite_hud` | `0` / `1`, индикатор в игре |

Для первой настройки используй кнопку в меню: один `tc_despite_antiping 1` не
включает необходимые параметры предсказания автоматически.

## Сборка и проверки

Windows: Visual Studio с C++, CMake, Python 3, Rust и Git.
Распаковывай сборку целиком вместе с `data` и DLL.

```powershell
git clone --recurse-submodules https://github.com/privetpixel-sudo/despite-tater.git
cd despite-tater
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DDOWNLOAD_GTEST=ON -DAUTOUPDATE=OFF -DVULKAN=OFF -DVIDEORECORDER=OFF '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' -DCMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE=.
cmake --build build --config Release --target game-client game-server --parallel 4
cmake --build build --config Release --target run_tests --parallel 4
```

Автообновление TClient отключено в сборках этого проекта, чтобы обновление не
заменяло клиент исходным TClient. Инструкции для других платформ и сведения об
исходном проекте приведены ниже. Используй `-DAUTOUPDATE=OFF` и для них.

Исходный код распространяется по исходной zlib-лицензии: [license.txt](license.txt).
У ресурсов, шрифтов и библиотек остаются собственные лицензии. Авторство DDNet,
Teeworlds и TClient сохранено. База: TClient `087d4ae5ab7266f3afc4fe6d46a579eff83234f6`.

---

## Документация исходного TClient

Ссылки и функции ниже относятся к TClient; его загрузки не содержат изменений Ðespiтe.

[![DDraceNetwork](docs/assets/TClient_Logo_Horizontal.svg)](https://tclient.app)

[![Build status](https://github.com/TaterClient/TClient/workflows/Build/badge.svg)](https://github.com/TaterClient/TClient/actions/workflows/build.yml)

### Taters custom ddnet client with some modifications

Not guaranteed to be bug free, but I will try to fix them.

If ddnet devs are reading this and want to steal my changes please feel free.

Thanks to tela for the logo design, and solly for svg <3

### Links

[Discord](https://discord.gg/BgPSapKRkZ)
[Website](https://tclient.app)

### Installation

* Download the latest [release](https://github.com/sjrc6/TaterClient-ddnet/releases)
* Download a [nightly (dev/unstable) build](https://github.com/sjrc6/TaterClient-ddnet/actions/workflows/fast-build.yml?query=branch%3Amaster)
* [Clone](https://docs.github.com/en/repositories/creating-and-managing-repositories/cloning-a-repository) this repo and build using the [guide from DDNet](https://github.com/ddnet/ddnet?tab=readme-ov-file#cloning)

### Translation

FTAPI (a simple wrapper for Google translate) will work out of the box, however it will quickly become overloaded

This is a guide for setting up [libretranslate](https://docs.libretranslate.com/guides/installation/)

First you need an old version of python (3.8, 3.9 or 3.10), along with `pip`

If you do not have this you can use [conda](https://www.anaconda.com/docs/getting-started/miniconda/install#quickstart-install-instructions) to install it

```sh
conda create -n libretranslate python=3.9
conda activate libretranslate
```

Then you can install and run libretranslate, do note that this requires large libraries like `torch` so it's a couple of gigs

```sh
pip install libretranslate
libretranslate
```

You can then set `tc_translate_backend libretranslate`, the port is automatically 5000

### Scripting

TClient supports the [ChaiScript](https://chaiscript.com/) language for simple tasks

Add scripts to your config dir then run them with `chai [scriptname] [args]`

> [!CAUTION]
> There are no runtime restrictions, you can easily `while (true) {}` yourself or run out of memory, be careful!

```js
var a // Declare a variable
a = 1 // Set it
var b = 2 // Do both at once
var c = "strings"
var d = ["lists", 2] // not strongly typed
// var e, f = d // no list deconstruction
print(d[0] + to_string(d[1])) // explicit to_string required for string concat
var bass = "ba" + "s" + "s"
var ass = bass.substr(1, -1) // both indices required, use -1 for end
if (a == b) { // brackets required
	print("this will never happen") // output
} else if (c == "strings") { // string comparison
	exec("echo hello world") // run console stuff
}
var current_game_mode = state("game_mode") // Get the current game mode, all states you can get are listed below
def myfunc(a, b, c) { // yeah it uses def for function definition idk
	print(a, b, c)
	if (a == b) { return "early" }
	c // last statement returns like in rust
}
print(myfunc(1, 2, 3)) // prints "early"
for (var i = 0; i < 10; i += 1) { // for loops (c style)
	print(i) // auto converts to string, will throw if it cant
}
return "top level return"
```

Here is a list of states which are available:

| Return type | Call | Description |
| --- | -- | --- |
| `string` | `state("game_mode")` | Returns the current game mode name (e.g., “DM”, “TDM”, “CTF”). |
| `bool` | `state("game_mode_pvp")` | Whether the current mode is PvP. |
| `bool` | `state("game_mode_race")` | Whether the current mode is a race mode. |
| `bool` | `state("eye_wheel_allowed")` | Whether the “eye wheel” feature is allowed on this server. |
| `bool` | `state("zoom_allowed")` | Whether camera zoom is allowed. |
| `bool` | `state("dummy_allowed")` | Whether using a dummy client is allowed. |
| `bool` | `state("dummy_connected")` | Whether the dummy client is currently connected. |
| `bool` | `state("rcon_authed")` | Whether the client is authenticated with RCON (admin access). |
| `int` | `state("team")` | The player’s current team number. |
| `int` | `state("ddnet_team")` | The player’s DDNet team number. |
| `string` | `state("map")` | The name of the current or connecting map. |
| `string` | `state("server_ip")` | The IP address of the connected or connecting server. |
| `int` | `state("players_connected")` | Number of currently connected players. |
| `int` | `state("players_cap")` | Maximum number of players the server supports. |
| `string` | `state("server_name")` | The server’s name. |
| `string` | `state("community")` | The server’s community identifier. |
| `string` | `state("location")` | The player’s approximate map location (“NW”, “C”, “SE”, etc.). |
| `string` | `state("state")` | The client’s connection state (e.g., “online”, “offline”, “loading”, “demo”). |
| `int` | `state("id", string Name)` | Finds and returns a client ID by player name (exact or case-insensitive match). |
| `string` | `state("name", int Id)` | Returns the name of a player given their client ID. |
| `string` | `state("clan", int Id)` | Returns the clan name of a player given their client ID. |

```js
var what = include("thatscript.chai") // you can include other scripts, they use absolute paths from config dir
print(what) // prints "top level return"
if (!file_exists("file")) { // check if a file exists, also absolute from config dir
	throw("why doesn't this file exist")
}
```

There is also `math` and `re` modules

```js
import("math")
math.pi
math.e
math.pow(1, 2)
math.sqrt(3)
math.sin(1)
math.cos(1)
math.tan(1)
math.asin(1)
math.acos(1)
math.atan(1)
math.atan2(1, 1)
math.log(1)
math.log10(1)
math.log2(1)
math.ceil(1)
math.floor(1)
math.round(1)
math.abs(1)
```

```js
import("re")

if(re.test(re.compile(".+?ello.+?"), "hello")) { // re.test(r, string)
	print("hi")
}
re.match(re.compile("\\d"), "h3ll0", false, fun[](str, match, group) { // re.match(r, string, global, callback)
	print("not global: " + to_string(match) + " " + str)
})
re.match(re.compile("\\d"), "h3ll0", true, fun[](str, match, group) {
	print("global: " + to_string(match) + " " + str)
})
re.match(re.compile("(h3)l(l0)"), "h3ll0", false, fun[](str, match, group) {
	print("groups: " + to_string(match) + " " + to_string(group) + " " + str)
})
print(re.replace(re.compile("\\d"), "h3ll0", true, fun[](str, match, group) { // re.replace(r, string, global, callback)
	if (str == "3") {
		return "e"
	} else if (str == "0") {
		return "o"
	}
	return str
}))
```

### Settings Page

> [!NOTE]
> This is out of date

![image](https://github.com/user-attachments/assets/a6ccb206-9fed-48be-a2d2-8fc50a6be882)
![image](https://github.com/user-attachments/assets/9251509a-d852-41ac-bf6b-9a610db08945)
![image](https://github.com/user-attachments/assets/47dab977-1311-4963-a11a-81b78005b12b)
![image](https://github.com/user-attachments/assets/29bddfd9-fcf1-420c-b7e0-958493051a3c)
![image](https://github.com/user-attachments/assets/efe3528f-a962-4dc0-aa8c-9ca963c246e5)
![image](https://github.com/user-attachments/assets/9f15023d-2a27-44ee-8157-e76da53c875a)

![image](https://user-images.githubusercontent.com/22122579/182528700-4c8238c3-836e-49c3-9996-68025e7f5d58.png)

### Config List

```
tc_allow_any_res
tc_show_chat_client
tc_frozen_tees_text
tc_frozen_tees_hud
tc_frozen_tees_hud_skins
tc_frozen_tees_size
tc_frozen_tees_max_rows
tc_frozen_tees_only_inteam
tc_nameplate_ping_circle
tc_nameplate_country
tc_nameplate_skins
tc_fake_ctf_flags
tc_limit_mouse_to_screen
tc_scale_mouse_distance
tc_hammer_rotates_with_cursor
tc_mini_vote_hud
tc_remove_anti
tc_remove_anti_ticks
tc_remove_anti_delay_ticks
tc_unpred_others_in_freeze
tc_pred_margin_in_freeze
tc_pred_margin_in_freeze_amount
tc_show_others_ghosts
tc_swap_ghosts
tc_hide_frozen_ghosts
tc_pred_ghosts_alpha
tc_unpred_ghosts_alpha
tc_render_ghost_as_circle
tc_show_center
tc_show_center_width
tc_show_center_color
tc_fast_input
tc_fast_input_amount
tc_fast_input_others
tc_fast_input_repredict_hook
tc_antiping_improved
tc_antiping_negative_buffer
tc_antiping_stable_direction
tc_antiping_uncertainty_scale
tc_color_freeze
tc_color_freeze_darken
tc_color_freeze_feet
tc_prediction_margin_smooth
tc_frozen_katana
tc_old_team_colors
tc_revert_hook_line
tc_outline
tc_outline_in_entities
tc_outline_solid
tc_outline_freeze
tc_outline_unfreeze
tc_outline_kill
tc_outline_tele
tc_outline_width_solid
tc_outline_width_freeze
tc_outline_width_unfreeze
tc_outline_width_kill
tc_outline_width_tele
tc_outline_color_solid
tc_outline_color_freeze
tc_outline_color_unfreeze
tc_outline_color_kill
tc_outline_color_tele
tc_indicator_alive
tc_indicator_freeze
tc_indicator_dead
tc_indicator_offset
tc_indicator_offset_max
tc_indicator_variable_distance
tc_indicator_variable_max_distance
tc_indicator_radius
tc_indicator_opacity
tc_player_indicator
tc_player_indicator_freeze
tc_indicator_inteam
tc_indicator_tees
tc_indicator_hide_visible_tees
tc_reset_bindwheel_mouse
tc_regex_chat_ignore
tc_white_feet
tc_white_feet_skin
tc_render_weapons_as_gun
tc_moving_tiles_entities
tc_mini_debug
tc_last_notify
tc_last_notify_text
tc_last_notify_color
tc_last_notify_x
tc_last_notify_y
tc_last_notify_size
tc_cursor_in_spec
tc_cursor_in_spec_alpha
tc_tiny_tees
tc_indicator_tees_size
tc_tiny_tees_others
tc_cursor_scale
tc_profile_skin
tc_profile_name
tc_profile_clan
tc_profile_flag
tc_profile_colors
tc_profile_emote
tc_profile_overwrite_clan_with_empty
tc_rainbow_tees
tc_rainbow_hook
tc_rainbow_weapon
tc_rainbow_others
tc_rainbow_mode
tc_rainbow_speed
tc_warlist
tc_warlist_show_clan_if_war
tc_warlist_reason
tc_warlist_chat
tc_warlist_scoreboard
tc_warlist_allow_duplicates
tc_warlist_spectate
tc_warlist_indicator
tc_warlist_indicator_colors
tc_warlist_indicator_all
tc_warlist_indicator_enemy
tc_warlist_indicator_team
tc_statusbar
tc_statusbar_12_hour_clock
tc_statusbar_local_time_seconds
tc_statusbar_height
tc_statusbar_color
tc_statusbar_text_color
tc_statusbar_alpha
tc_statusbar_text_alpha
tc_statusbar_labels
tc_statusbar_scheme
tc_tee_trail
tc_tee_trail_others
tc_tee_trail_width
tc_tee_trail_length
tc_tee_trail_alpha
tc_tee_trail_color
tc_tee_trail_taper
tc_tee_trail_fade
tc_tee_trail_color_mode
tc_auto_reply_muted
tc_auto_reply_muted_message
tc_auto_reply_minimized
tc_auto_reply_minimized_message
tc_auto_vote_when_far
tc_auto_vote_when_far_message
tc_auto_vote_when_far_time
tc_custom_font
tc_bg_draw_width
tc_bg_draw_fade_time
tc_bg_draw_max_items
tc_bg_draw_color
tc_bg_draw_auto_save_load
tc_translate_backend
tc_translate_target
tc_translate_endpoint
tc_translate_key
tc_translate_auto
tc_translate_outgoing
tc_translate_outgoing_target
tc_animate_wheel_time
tc_pet_show
tc_pet_skin
tc_pet_size
tc_pet_alpha
tc_change_name_near_finish
tc_finish_name
tc_tclient_settings_tabs
tc_volleyball_better_ball
tc_volleyball_better_ball_skin
tc_show_player_hit_boxes
tc_hide_chat_bubbles
tc_mod_weapon
tc_mod_weapon_command
tc_execute_on_connect
tc_execute_on_join
tc_execute_on_join_delay
tc_custom_communities_url
tc_discord_rpc
tc_show_local_time_seconds
tc_ui_show_ddnet
tc_ui_show_tclient
tc_ui_only_modified
tc_ui_compact_list
tc_showhud_dummy_position
tc_showhud_dummy_speed
tc_showhud_dummy_angle
```

Additional commands:

```
add_profile
add_bindwheel
remove_bindwheel
delete_all_bindwheel_binds
+bindwheel_execute_hover
+bindwheel
```
