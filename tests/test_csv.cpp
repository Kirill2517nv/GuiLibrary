// Проверка экспорта CSV.
//
// Зачем: история точек лежит в кольцевом буфере, и после переполнения самая
// старая точка стоит не в начале массива. Ошибка в вычислении смещения даёт
// правильный по виду файл с перепутанным порядком времени – ученик унесёт его в
// отчёт и ничего не заметит. Поэтому у этой логики есть отдельная проверка.
//
// Окно и OpenGL не нужны: create_plot, add_plot_history_point и save_plot_csv
// работают с обычными контейнерами, без графического контекста.
//
// Запуск:
//     cmake -S . -B build -DBUILD_TESTS=ON
//     cmake --build build --config Release --target test_csv
//     build/Release/test_csv

#include "gui_library.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Своя проверка вместо assert: в Release-сборке MSVC определяет NDEBUG, и
// assert превращается в пустое место – тест проходил бы всегда, ничего не
// проверяя. Требовать ради этого Debug-сборку не хочется.
static int g_failed = 0;

static void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("PROVAL: %s\n", what);
        ++g_failed;
    }
}

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

static std::vector<std::string> split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

int main() {
    const std::string path = "test_csv_output.csv";
    std::remove(path.c_str());

    // Пустой график: сохранять нечего, функция обязана вернуть false,
    // а не создать файл с одним заголовком.
    create_plot("Пустой", 0.f, 1.f, 0.f, 1.f);
    check(!save_plot_csv("Пустой", path), "pustoy grafik ne dolzhen sohranyatsya");
    check(!save_plot_csv("Takogo net", path), "neizvestnyy grafik ne dolzhen sohranyatsya");

    // Кольцевой буфер на 4 точки, кладём 7: остаться должны последние четыре,
    // в порядке возрастания времени.
    create_plot("Тест", 0.f, 10.f, -1.f, 1.f);
    const size_t capacity = 4;
    for (int i = 1; i <= 7; ++i) {
        add_plot_history_point("Тест", (float)i, (float)i * 10.f,
                               "серия, с запятой", BLUE, 1.f, capacity);
    }
    check(save_plot_csv("Тест", path), "sohranenie dolzhno proyti");

    const std::string csv = read_file(path);
    // BOM: без него Excel читает файл в системной кодировке и портит кириллицу.
    check(csv.size() > 3 && csv.compare(0, 3, "\xEF\xBB\xBF") == 0, "net BOM");
    if (g_failed) { std::printf("test_csv: provaleno %d\n", g_failed); return 1; }

    const std::vector<std::string> lines = split_lines(csv.substr(3));
    check(lines.size() == 1 + capacity, "ozhidalis zagolovok i chetyre tochki");
    if (g_failed) { std::printf("test_csv: provaleno %d\n", g_failed); return 1; }

    check(lines[0] == "серия,x,y", "zagolovok");

    // Запятая внутри подписи не должна разваливать строку на лишние колонки.
    for (size_t i = 1; i < lines.size(); ++i) {
        check(lines[i].rfind("\"серия, с запятой\",", 0) == 0, "podpis ne ekranirovana");
    }

    // Главное: порядок. Ожидаем x = 4, 5, 6, 7 – первые три вытеснены.
    const char* expected[] = {
        "\"серия, с запятой\",4,40",
        "\"серия, с запятой\",5,50",
        "\"серия, с запятой\",6,60",
        "\"серия, с запятой\",7,70",
    };
    for (size_t i = 0; i < capacity; ++i) {
        if (lines[i + 1] != expected[i]) {
            std::printf("PROVAL: stroka %d\n", (int)i + 1);
            ++g_failed;
        }
    }

    // Буфер ещё не заполнен: точки лежат подряд с начала, смещение не нужно.
    create_plot("Неполный", 0.f, 10.f, -1.f, 1.f);
    for (int i = 1; i <= 2; ++i) {
        add_plot_history_point("Неполный", (float)i, (float)i, "s", BLUE, 1.f, 10);
    }
    check(save_plot_csv("Неполный", path), "nepolnyy bufer dolzhen sohranyatsya");
    const std::vector<std::string> partial = split_lines(read_file(path).substr(3));
    check(partial.size() == 3, "ozhidalis zagolovok i dve tochki");
    if (partial.size() == 3) {
        check(partial[1] == "\"s\",1,1", "pervaya tochka nepolnogo bufera");
        check(partial[2] == "\"s\",2,2", "vtoraya tochka nepolnogo bufera");
    }

    std::remove(path.c_str());
    if (g_failed) {
        std::printf("test_csv: provaleno proverok: %d\n", g_failed);
        return 1;
    }
    std::printf("test_csv: vse proverki proydeny\n");
    return 0;
}
