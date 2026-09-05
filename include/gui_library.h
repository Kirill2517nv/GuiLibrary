#pragma once
//
// Публичный интерфейс учебной библиотеки.
//
// Здесь только то, что нужно коду задачи: цвета, параметры пульта, графики,
// экспорт и запуск. Внутренние структуры (PlotData, PlotHistory, Heatmap и
// прочее) живут в src/gui_library.cpp – ученик, открывший «документацию», не
// должен читать реализацию.
//
// Продвинутая разметка окон вынесена в gui_library_layout.h: она требует
// понятий, которых на первых занятиях нет, а обычному заданию хватает
// set_auto_layout().
//
#include "imgui.h"

#include <functional>
#include <string>
#include <vector>

// ============================================================
// Цвета серий
// ============================================================
//
// Раньше это были макросы с чистыми RGB: BLUE = (0,0,1), GREEN = (0,1,0). На
// тёмном фоне графика чистый синий почти не виден, а чистый зелёный режет глаз.
// Значения взяты из палитры сайта и проверены на различимость по всем парам, а
// не только по соседним: при протанопии и дейтеранопии худшая пара сохраняет
// запас ΔE 8.1, при обычном зрении – 19.8.
//
// Жёлтый светлее «полосы яркости», которую предписывает методика: тёмный
// жёлтый – это коричневый, и он сливается с красным (ΔE падает до 11.8 даже
// для обычного зрения). Из двух зол выбрано отклонение по яркости.
//
// Константы, а не макросы: макрос с именем RED рано или поздно столкнётся с
// чужим объявлением, и ошибка будет невразумительной.
inline const ImVec4 BLUE   = ImVec4(0.231f, 0.510f, 0.965f, 1.0f);  // brand-500 #3b82f6
inline const ImVec4 RED    = ImVec4(0.937f, 0.267f, 0.267f, 1.0f);  // red-500   #ef4444
inline const ImVec4 YELLOW = ImVec4(0.961f, 0.620f, 0.043f, 1.0f);  // amber-500 #f59e0b
inline const ImVec4 GREEN  = ImVec4(0.063f, 0.725f, 0.506f, 1.0f);  // emerald-500 #10b981
inline const ImVec4 WHITE  = ImVec4(0.945f, 0.961f, 0.976f, 1.0f);  // slate-100 #f1f5f9
inline const ImVec4 BLACK  = ImVec4(0.059f, 0.090f, 0.165f, 1.0f);  // slate-900 #0f172a

// ============================================================
// Инициализация и запуск
// ============================================================

// Создать окно. Вызывается первой; вернула false – дальше идти нельзя.
bool init_gui_library(const std::string& window_title = "Численное моделирование",
                      int widthWindow = 1200, int heightWindow = 800);

// Запустить главный цикл. Работает и в нативной сборке, и в браузере –
// разницу между ними библиотека прячет.
void run_gui_library();

// Установить функцию, которую библиотека будет звать каждый кадр. Ваша функция
// делает ОДИН шаг расчёта и заканчивается; повторит её библиотека сама.
void set_calculation_function(std::function<void()> calc_func);

// ============================================================
// Пульт: ручки и показания
// ============================================================
//
// Имя параметра – это ключ, по которому его потом читают. Опечатка в имени не
// ошибка компиляции: get_* вернёт ноль и напишет предупреждение.

// Ручки – их крутит пользователь.
void add_float_param(const std::string& name,
                     float initial_value = 0.0f, float min = 0.0f, float max = 100.0f,
                     float step = 0.2f, bool use_slider = false);

void add_int_param(const std::string& name,
                   int initial_value = 0, int min = 0, int max = 100,
                   int step = 1, bool use_slider = false);

void add_bool_param(const std::string& name, bool initial_value = false);

void add_string_param(const std::string& name, const std::string& initial_value = "");

// Кнопка: по нажатию вызывает переданную функцию.
void add_button_param(const std::string& name, std::function<void()> function);

// Показания – их пишет расчёт. Рисуются такой же рамкой, что и ручки, но
// только для чтения: число можно выделить и скопировать, изменить нельзя.
// Читаются и пишутся теми же get_/set_, что и ручки, поэтому превратить ручку
// в показание – правка одного слова.
void add_output_float(const std::string& name, float initial_value = 0.0f);
void add_output_int(const std::string& name, int initial_value = 0);

float       get_float_param(const std::string& name);
void        set_float_param(const std::string& name, float value);
int         get_int_param(const std::string& name);
void        set_int_param(const std::string& name, int value);
bool        get_bool_param(const std::string& name);
void        set_bool_param(const std::string& name, bool value);
std::string get_string_param(const std::string& name);
void        set_string_param(const std::string& name, const std::string& value);

// ============================================================
// Графики
// ============================================================

// Создать график: границы осей, затем размер полотна в пикселях.
void create_plot(const std::string& name,
                 float x_min, float x_max, float y_min, float y_max,
                 int width = 600, int height = 400);

// Подписать оси вместе с единицами: set_plot_axes("Маятник", "t, с", "x, м").
// График без единиц на уроке физики читать нельзя, поэтому подписывать стоит всё.
void set_plot_axes(const std::string& plot_name,
                   const std::string& x_label, const std::string& y_label);

// Обновить границы осей (применяется на следующем кадре).
void set_plot_scale(const std::string& name,
                    float x_min, float x_max, float y_min, float y_max);

// ── Что рисуется каждый кадр заново ─────────────────────────
//
// Объекты кадра (точка, линия, набор точек, круг, тепловая карта) живут до
// следующего вызова clear_plot. История – наоборот, копится сама.

// Одна точка.
void add_plot_point(const std::string& plot_name, float x, float y,
                    const std::string& label = "Точка",
                    const ImVec4& color = RED, float size = 4.0f);

// Набор точек.
void add_plot_points(const std::string& plot_name,
                     const std::vector<float>& x, const std::vector<float>& y,
                     const std::string& label = "Данные",
                     const ImVec4& color = BLUE, float size = 1.0f);

void add_plot_points(const std::string& plot_name,
                     const std::vector<double>& x, const std::vector<double>& y,
                     const std::string& label = "Данные",
                     const ImVec4& color = BLUE, float size = 1.0f);

// Сплошная линия через заданные точки.
void add_plot_line(const std::string& plot_name,
                   const std::vector<float>& x, const std::vector<float>& y,
                   const std::string& label = "Данные",
                   const ImVec4& color = BLUE, float size = 1.0f);

void add_plot_line(const std::string& plot_name,
                   const std::vector<double>& x, const std::vector<double>& y,
                   const std::string& label = "Данные",
                   const ImVec4& color = BLUE, float size = 1.0f);

// Закрашенный круг радиусом в координатах графика. Если масштабы осей разные,
// круг выглядит эллипсом – это физически верно: форма сохраняется в координатах
// данных, а не на экране.
void add_plot_disk(const std::string& plot_name, float x, float y, float radius,
                   const std::string& label = "Круг",
                   const ImVec4& color = BLUE);

// Тепловая карта: значения построчно, rows строк на cols столбцов.
void add_plot_heatmap(const std::string& plot_name,
                      const std::vector<float>& values,
                      int rows, int cols,
                      const std::string& label = "Тепловая карта",
                      double scale_min = 0.0, double scale_max = 0.0,
                      int colormap = 4,
                      const std::string& label_fmt = "");

// Очистить объекты текущего кадра. Историю не трогает.
void clear_plot(const std::string& plot_name);

// ── История: копится сама ───────────────────────────────────

// Добавить точку в историю именованной серии. Библиотека хранит не более
// max_points последних значений и рисует их в хронологическом порядке –
// собственный кольцевой буфер коду задачи не нужен.
void add_plot_history_point(const std::string& plot_name,
                            float x, float y,
                            const std::string& label = "Данные",
                            const ImVec4& color = BLUE, float size = 1.0f,
                            size_t max_points = 2000);

// Очистить всю накопленную историю графика (например, по кнопке «Заново»).
void clear_plot_history(const std::string& plot_name);

// Очистить только одну именованную серию.
void clear_plot_history(const std::string& plot_name, const std::string& label);

// ============================================================
// Экспорт данных
// ============================================================

// Сохранить накопленную историю графика в CSV: по строке на точку, колонки
// «серия, x, y». Пустое имя файла – «<имя графика>.csv».
//
// Нативно файл пишется в текущий каталог, и путь печатается в консоль. В
// браузере файловой системы нет, поэтому файл отдаётся на скачивание.
//
// У каждого графика есть кнопка «Сохранить CSV» – звать эту функцию из кода
// задачи нужно, только если сохранение должно случиться само, без нажатия.
bool save_plot_csv(const std::string& plot_name, const std::string& filename = "");

// ============================================================
// Разметка окон
// ============================================================

// Пульт слева (params_ratio от ширины), все графики справа вкладками.
// Этого хватает почти всегда; полный контроль – в gui_library_layout.h.
void set_auto_layout(float params_ratio = 0.25f);

// ============================================================
// Утилиты
// ============================================================

// Время в секундах с момента init_gui_library.
double get_time();

void sleep_ms(int milliseconds);

// ============================================================
// Служебное
// ============================================================

// Один кадр вручную. Нужна, только если вы пишете свой главный цикл вместо
// run_gui_library. Возвращает false, когда окно закрыли.
bool gui_main_loop();

void shutdown_gui_library();
