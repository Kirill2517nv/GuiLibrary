#pragma once
//
// Полный контроль над расположением окон – для тех, кому мало set_auto_layout.
//
// Вынесено из основного заголовка нарочно: здесь нужны лямбды, указатели и
// понятие идентификатора окна, а на первых занятиях этих понятий нет. Обычному
// заданию достаточно одной строки set_auto_layout() из gui_library.h.
//
// Пример:
//
//     set_default_layout([](ImGuiID id) {
//         ImGuiID left, right;
//         layout_split_left(id, 0.3f, &left, &right);
//         layout_dock("Parameters", left);
//         layout_dock("График",     right);
//     });
//
#include "gui_library.h"

#include <functional>
#include <string>

// Задать разметку. Функция получает идентификатор корневой области, внутри неё
// пользуйтесь layout_split_* и layout_dock.
//
// В браузере применяется при каждом запуске: сохранять расположение окон между
// сеансами там негде. В нативной сборке – только при первом, пока рядом нет
// файла imgui.ini с расположением, которое пользователь настроил мышью.
void set_default_layout(std::function<void(ImGuiID dockspace_id)> layout_fn);

// Разбить область вертикально: левая часть получает ratio от ширины.
void layout_split_left(ImGuiID node, float ratio, ImGuiID* out_left, ImGuiID* out_right);

// Разбить область горизонтально: верхняя часть получает ratio от высоты.
void layout_split_up(ImGuiID node, float ratio, ImGuiID* out_top, ImGuiID* out_bottom);

// Поместить окно с именем name в область node.
void layout_dock(const std::string& name, ImGuiID node);
