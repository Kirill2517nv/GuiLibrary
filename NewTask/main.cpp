#include "gui_library.h"
#include <cmath>
#include <vector>
#include <cstdlib>

// Частица (приведённые единицы: m=1, b=1, ε=1, kB=1)
// Потенциал: U(r) = ε * ((b/r)^12 - 2*(b/r)^6)
// Минимум при r = b, U(b) = -ε
struct Particle {
    double x, y;
    double vx, vy;
    double ax, ay;
    double ax_new, ay_new;
};

// Глобальные переменные
std::vector<Particle> prtcls;
int N = 2;
double Lx = 10.0, Ly = 10.0;
double rc = 2.5;
double dt = 0.002;
double t_sim = 0;
int step_count = 0;

// Физические величины
double E_kin = 0, U_pot = 0;
double T_inst = 0, P_inst = 0;
double virial_sum = 0;
double U_shift = 0;

// Термостат Берендсена
double T_target = 0.5;
double tau_T = 0.1;

// Усреднение T и P (кумулятивное)
double T_accum = 0, P_accum = 0;
int avg_samples = 0;
double T_avg = 0, P_avg = 0;

// Графики — простые векторы
std::vector<float> plot_time;
std::vector<float> plot_ekin, plot_u, plot_etotal;
std::vector<float> plot_T, plot_P;
std::vector<float> plot_T_avg, plot_P_avg;
const size_t MAX_PLOT_POINTS = 4000;

// EOS: записанные точки (ρ, P_avg, T_avg)
struct EOSPoint { float rho, P, T; };
std::vector<EOSPoint> eos_data;

void trim_vectors() {
    if (plot_time.size() >= MAX_PLOT_POINTS) {
        size_t half = MAX_PLOT_POINTS / 2;
        plot_time.erase(plot_time.begin(), plot_time.begin() + half);
        plot_ekin.erase(plot_ekin.begin(), plot_ekin.begin() + half);
        plot_u.erase(plot_u.begin(), plot_u.begin() + half);
        plot_etotal.erase(plot_etotal.begin(), plot_etotal.begin() + half);
        plot_T.erase(plot_T.begin(), plot_T.begin() + half);
        plot_P.erase(plot_P.begin(), plot_P.begin() + half);
        plot_T_avg.erase(plot_T_avg.begin(), plot_T_avg.begin() + half);
        plot_P_avg.erase(plot_P_avg.begin(), plot_P_avg.begin() + half);
    }
}

void clear_plot_data() {
    plot_time.clear();
    plot_ekin.clear(); plot_u.clear(); plot_etotal.clear();
    plot_T.clear(); plot_P.clear();
    plot_T_avg.clear(); plot_P_avg.clear();
    T_accum = 0; P_accum = 0; avg_samples = 0;
    T_avg = 0; P_avg = 0;
}

// Изотермы P(ρ) по уравнению Ван-дер-Ваальса: P = ρT/(1−bρ) − aρ²
// и поверх — записанные точки MD
void update_eos_plot() {
    clear_plot("EOS");

    double a = get_float_param("a VdW");
    double b = get_float_param("b VdW");

    struct IsoT { const char* param; ImVec4 color; };
    IsoT isotherms[] = {
        {"T VdW 1", BLUE},
        {"T VdW 2", GREEN},
        {"T VdW 3", YELLOW},
        {"T VdW 4", ImVec4(1.f, 0.5f, 0.f, 1.f)},
        {"T VdW 5", RED},
        {"T VdW 6", ImVec4(0.8f, 0.f, 0.8f, 1.f)},
    };

    const int n_pts = 300;
    const double rho_max = 0.92 / b;  // не доходим до сингулярности 1/b
    for (auto& iso : isotherms) {
        double T = get_float_param(iso.param);
        char label[32];
        snprintf(label, sizeof(label), "VdW T=%.2f", T);
        std::vector<double> rho_v, P_v;
        for (int i = 1; i <= n_pts; i++) {
            double rho = (double)i / n_pts * rho_max;
            double denom = 1.0 - b * rho;
            if (denom < 1e-6) break;
            double P = rho * T / denom - a * rho * rho;
            rho_v.push_back(rho);
            P_v.push_back(P);
        }
        add_plot_line("EOS", rho_v, P_v, label, iso.color, 1.5f);
    }

    // Записанные точки MD поверх кривых
    if (!eos_data.empty()) {
        std::vector<float> rho_pts, P_pts;
        for (auto& pt : eos_data) {
            rho_pts.push_back(pt.rho);
            P_pts.push_back(pt.P);
        }
        add_plot_points("EOS", rho_pts, P_pts, "MD", WHITE, 7.0f);
    }
}

// Минимальный образ (периодические ГУ)
inline void min_image(double xi, double yi, double xj, double yj, double& dx, double& dy) {
    dx = xj - xi;  dx -= Lx * round(dx / Lx);
    dy = yj - yi;  dy -= Ly * round(dy / Ly);
}

void compute_U_shift() {
    double brc2 = 1.0 / (rc * rc);
    double brc6 = brc2 * brc2 * brc2;
    U_shift = brc6 * brc6 - 2.0 * brc6;
}

// Вычисление сил + вириал + потенциальная энергия
void compute_forces() {
    for (int i = 0; i < N; i++) {
        prtcls[i].ax_new = 0;
        prtcls[i].ay_new = 0;
    }
    U_pot = 0;
    virial_sum = 0;
    double rc2 = rc * rc;

    for (int i = 0; i < N - 1; i++) {
        for (int j = i + 1; j < N; j++) {
            double dx, dy;
            min_image(prtcls[i].x, prtcls[i].y, prtcls[j].x, prtcls[j].y, dx, dy);
            double r2 = dx * dx + dy * dy;
            if (r2 < rc2 && r2 > 1e-6) {
                double r2inv = 1.0 / r2;
                double br6 = r2inv * r2inv * r2inv;

                double f_over_r = -12.0 * r2inv * (br6 * br6 - br6);
                prtcls[i].ax_new += f_over_r * dx;
                prtcls[i].ay_new += f_over_r * dy;
                prtcls[j].ax_new -= f_over_r * dx;
                prtcls[j].ay_new -= f_over_r * dy;

                U_pot += (br6 * br6 - 2.0 * br6) - U_shift;
                virial_sum += 12.0 * (br6 * br6 - br6);
            }
        }
    }
}

void update_positions() {
    for (int i = 0; i < N; i++) {
        prtcls[i].x += prtcls[i].vx * dt + 0.5 * prtcls[i].ax * dt * dt;
        prtcls[i].y += prtcls[i].vy * dt + 0.5 * prtcls[i].ay * dt * dt;
        prtcls[i].x -= Lx * floor(prtcls[i].x / Lx);
        prtcls[i].y -= Ly * floor(prtcls[i].y / Ly);
    }
}

void update_velocities() {
    for (int i = 0; i < N; i++) {
        prtcls[i].vx += 0.5 * dt * (prtcls[i].ax + prtcls[i].ax_new);
        prtcls[i].vy += 0.5 * dt * (prtcls[i].ay + prtcls[i].ay_new);
        prtcls[i].ax = prtcls[i].ax_new;
        prtcls[i].ay = prtcls[i].ay_new;
    }
}

double compute_ekin() {
    double ek = 0;
    for (int i = 0; i < N; i++)
        ek += 0.5 * (prtcls[i].vx * prtcls[i].vx + prtcls[i].vy * prtcls[i].vy);
    return ek;
}

// T = Σ m_i (v_i - v_cm)² / (2N - 3) в 2D, m=1, kB=1
double measure_temperature() {
    double vx_cm = 0, vy_cm = 0;
    for (int i = 0; i < N; i++) {
        vx_cm += prtcls[i].vx;
        vy_cm += prtcls[i].vy;
    }
    vx_cm /= N;
    vy_cm /= N;

    double sum_v2 = 0;
    for (int i = 0; i < N; i++) {
        double dvx = prtcls[i].vx - vx_cm;
        double dvy = prtcls[i].vy - vy_cm;
        sum_v2 += dvx * dvx + dvy * dvy;
    }
    return sum_v2 / (2.0 * N - 3.0);
}

// P = N*T/A + W/(2*A) в 2D
double compute_pressure() {
    double A = Lx * Ly;
    return (N * T_inst) / A + virial_sum / (2.0 * A);
}

// Запуск N частиц: задаём L (размер бокса) и N, плотность = N / L²
void init_N_particles() {
    N = get_int_param("N");
    Lx = get_float_param("L");
    Ly = Lx;
    double T0 = get_float_param("T0");
    rc = 2.5; dt = 0.002;
    compute_U_shift();
    prtcls.resize(N);
    t_sim = 0; step_count = 0;

    int cols = (int)ceil(sqrt((double)N));
    int rows_grid = (N + cols - 1) / cols;
    double dx_grid = Lx / cols;
    double dy_grid = Ly / rows_grid;

    srand(42);
    for (int i = 0; i < N; i++) {
        prtcls[i].x = (i % cols + 0.5) * dx_grid;
        prtcls[i].y = (i / cols + 0.5) * dy_grid;
        prtcls[i].vx = (rand() / (double)RAND_MAX - 0.5) * 2.0;
        prtcls[i].vy = (rand() / (double)RAND_MAX - 0.5) * 2.0;
        prtcls[i].ax = 0; prtcls[i].ay = 0;
        prtcls[i].ax_new = 0; prtcls[i].ay_new = 0;
    }

    double vx_cm = 0, vy_cm = 0;
    for (int i = 0; i < N; i++) { vx_cm += prtcls[i].vx; vy_cm += prtcls[i].vy; }
    vx_cm /= N; vy_cm /= N;
    for (int i = 0; i < N; i++) { prtcls[i].vx -= vx_cm; prtcls[i].vy -= vy_cm; }

    double sum_v2 = 0;
    for (int i = 0; i < N; i++)
        sum_v2 += prtcls[i].vx * prtcls[i].vx + prtcls[i].vy * prtcls[i].vy;
    double T_curr = sum_v2 / (2.0 * N);
    if (T_curr > 1e-12) {
        double sc = sqrt(T0 / T_curr);
        for (int i = 0; i < N; i++) { prtcls[i].vx *= sc; prtcls[i].vy *= sc; }
    }

    compute_forces();
    for (int i = 0; i < N; i++) { prtcls[i].ax = prtcls[i].ax_new; prtcls[i].ay = prtcls[i].ay_new; }
    clear_plot_data();

    E_kin = 0; U_pot = 0; T_inst = 0; P_inst = 0;
    T_avg = 0; P_avg = 0;
    set_float_param("Time",    0.0f);
    set_float_param("E_kin",   0.0f);
    set_float_param("U",       0.0f);
    set_float_param("E_total", 0.0f);
    set_float_param("T inst",  0.0f);
    set_float_param("T avg",   0.0f);
    set_float_param("P inst",  0.0f);
    set_float_param("P avg",   0.0f);
    set_float_param("Density", static_cast<float>(N / (Lx * Ly)));
}

void calculation_function() {
    if (get_bool_param("Pause")) {
        update_eos_plot();
        return;
    }

    int steps_per_frame = get_int_param("Steps/frame");

    for (int s = 0; s < steps_per_frame; s++) {
        update_positions();
        compute_forces();
        update_velocities();

        if (get_bool_param("Thermostat")) {
            T_target = get_float_param("T0");
            tau_T    = get_float_param("tau");
            double T_now = measure_temperature();
            if (T_now > 1e-12) {
                double lambda = sqrt(1.0 + (dt / tau_T) * (T_target / T_now - 1.0));
                for (int i = 0; i < N; i++) {
                    prtcls[i].vx *= lambda;
                    prtcls[i].vy *= lambda;
                }
            }
        }

        t_sim += dt;
        step_count++;

        E_kin  = compute_ekin();
        T_inst = measure_temperature();
        P_inst = compute_pressure();

        if (std::isnan(E_kin) || std::isnan(T_inst)) {
            set_bool_param("Pause", true);
            return;
        }

        avg_samples++;
        T_accum += T_inst;
        P_accum += P_inst;
        T_avg = T_accum / avg_samples;
        P_avg = P_accum / avg_samples;

        trim_vectors();
        float tf = static_cast<float>(t_sim);
        plot_time.push_back(tf);
        plot_ekin.push_back(static_cast<float>(E_kin));
        plot_u.push_back(static_cast<float>(U_pot));
        plot_etotal.push_back(static_cast<float>(E_kin + U_pot));
        plot_T.push_back(static_cast<float>(T_inst));
        plot_P.push_back(static_cast<float>(P_inst));
        plot_T_avg.push_back(static_cast<float>(T_avg));
        plot_P_avg.push_back(static_cast<float>(P_avg));
    }

    clear_plot("Energy");
    if (!plot_time.empty()) {
        add_plot_line("Energy", plot_time, plot_ekin,   "E_kin",   RED,    2.0f);
        add_plot_line("Energy", plot_time, plot_u,      "U",       YELLOW, 2.0f);
        add_plot_line("Energy", plot_time, plot_etotal, "E_total", WHITE,  2.0f);
    }

    clear_plot("Temperature");
    if (!plot_time.empty()) {
        add_plot_line("Temperature", plot_time, plot_T,     "T inst", GREEN, 1.0f);
        add_plot_line("Temperature", plot_time, plot_T_avg, "T avg",  RED,   2.0f);
    }

    clear_plot("Pressure");
    if (!plot_time.empty()) {
        add_plot_line("Pressure", plot_time, plot_P,     "P inst", BLUE, 1.0f);
        add_plot_line("Pressure", plot_time, plot_P_avg, "P avg",  RED,  2.0f);
    }

    // Автоскролл: скользящее окно шириной 10 единиц
    if (!plot_time.empty()) {
        float t_now  = plot_time.back();
        float window = 10.0f;
        float x_min  = t_now - window / 2.0f;
        float x_max  = t_now + window / 2.0f;

        float e_min = 1e30f, e_max = -1e30f;
        for (size_t i = 0; i < plot_time.size(); i++) {
            if (plot_time[i] >= x_min && plot_time[i] <= x_max) {
                e_min = std::min({e_min, plot_ekin[i], plot_u[i], plot_etotal[i]});
                e_max = std::max({e_max, plot_ekin[i], plot_u[i], plot_etotal[i]});
            }
        }
        float e_margin = (e_max - e_min) * 0.1f + 0.1f;
        set_plot_scale("Energy", x_min, x_max, e_min - e_margin, e_max + e_margin);

        float t_min_v = 1e30f, t_max_v = -1e30f;
        for (size_t i = 0; i < plot_time.size(); i++) {
            if (plot_time[i] >= x_min && plot_time[i] <= x_max) {
                t_min_v = std::min({t_min_v, plot_T[i], plot_T_avg[i]});
                t_max_v = std::max({t_max_v, plot_T[i], plot_T_avg[i]});
            }
        }
        float t_margin = (t_max_v - t_min_v) * 0.1f + 0.01f;
        set_plot_scale("Temperature", x_min, x_max, t_min_v - t_margin, t_max_v + t_margin);

        float p_min_v = 1e30f, p_max_v = -1e30f;
        for (size_t i = 0; i < plot_time.size(); i++) {
            if (plot_time[i] >= x_min && plot_time[i] <= x_max) {
                p_min_v = std::min({p_min_v, plot_P[i], plot_P_avg[i]});
                p_max_v = std::max({p_max_v, plot_P[i], plot_P_avg[i]});
            }
        }
        float p_margin = (p_max_v - p_min_v) * 0.1f + 0.01f;
        set_plot_scale("Pressure", x_min, x_max, p_min_v - p_margin, p_max_v + p_margin);
    }

    // Частицы
    clear_plot("Particles");
    for (int i = 0; i < N; i++) {
        add_plot_disk("Particles",
            static_cast<float>(prtcls[i].x),
            static_cast<float>(prtcls[i].y),
            0.5f,  // радиус частицы = b/2 = 0.5 (минимум потенциала Леннарда-Джонса при r=b=1)
            "p" + std::to_string(i), WHITE);
    }

    update_eos_plot();

    set_float_param("Time",    static_cast<float>(t_sim));
    set_float_param("E_kin",   static_cast<float>(E_kin));
    set_float_param("U",       static_cast<float>(U_pot));
    set_float_param("E_total", static_cast<float>(E_kin + U_pot));
    set_float_param("T inst",  static_cast<float>(T_inst));
    set_float_param("T avg",   static_cast<float>(T_avg));
    set_float_param("P inst",  static_cast<float>(P_inst));
    set_float_param("P avg",   static_cast<float>(P_avg));
    set_float_param("Density", static_cast<float>(N / (Lx * Ly)));
}

int main() {
    if (!init_gui_library("MD Test", 1600, 1200)) return -1;

    add_int_param("Steps/frame", 20, 1, 200, 5);
    add_bool_param("Pause", false);

    add_int_param("N", 50, 2, 500, 1);
    add_float_param("L", 13.0f, 3.0f, 50.0f, 0.5f);
    add_float_param("T0", 0.5f, 0.05f, 3.0f, 0.05f);

    add_bool_param("Thermostat", true);
    add_float_param("tau", 0.1f, 0.01f, 5.0f, 0.01f);

    add_button_param("Run N particles", []() { init_N_particles(); });

    // VdW EOS: параметры и кнопки записи
    add_float_param("a VdW", 1.58f, 0.01f, 10.0f, 0.01f);
    add_float_param("b VdW", 0.801f, 0.01f, 5.0f, 0.005f);
    add_float_param("T VdW 1", 0.30f, 0.01f, 5.0f, 0.01f);
    add_float_param("T VdW 2", 0.40f, 0.01f, 5.0f, 0.01f);
    add_float_param("T VdW 3", 0.50f, 0.01f, 5.0f, 0.01f);
    add_float_param("T VdW 4", 0.60f, 0.01f, 5.0f, 0.01f);
    add_float_param("T VdW 5", 0.70f, 0.01f, 5.0f, 0.01f);
    add_float_param("T VdW 6", 0.80f, 0.01f, 5.0f, 0.01f);
    add_button_param("Write Point EOS", []() {
        if (avg_samples > 0) {
            EOSPoint pt;
            pt.rho = static_cast<float>(N / (Lx * Ly));
            pt.P   = static_cast<float>(P_avg);
            pt.T   = static_cast<float>(T_avg);
            eos_data.push_back(pt);
        }
    });
    add_button_param("Clear EOS", []() { eos_data.clear(); });

    add_float_param("Time",    0.0f, 0.0f,  1e6f, 0.0f);
    add_float_param("Density", 0.0f, 0.0f,  10.f, 0.0f);
    add_float_param("T inst",  0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("T avg",   0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("P inst",  0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("P avg",   0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("E_kin",   0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("U",       0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("E_total", 0.0f, -1e6f, 1e6f, 0.0f);

    create_plot("Particles",   Scale(0.f,  15.f, 0.f,   15.f),  500, 500);
    create_plot("Energy",      Scale(0.f,  5.f,  -100.f, 100.f), 500, 220);
    create_plot("Temperature", Scale(0.f,  5.f,  0.f,    2.f),   500, 220);
    create_plot("Pressure",    Scale(0.f,  5.f,  -2.f,   2.f),   500, 220);
    create_plot("EOS",         Scale(0.f,  1.0f, -0.5f,  0.5f),  735, 350);

    init_N_particles();
    set_calculation_function(calculation_function);
    set_default_layout([](ImGuiID id) {
        // Воспроизводит разметку из imgui.ini (1600×1200):
        //   Parameters (297px) | Particles (728px) | Energy/Temperature/Pressure (571px)

        ImGuiID left, rest;
        layout_split_left(id,   297.f / 1600.f,           &left,    &rest);

        ImGuiID center, right_col;
        layout_split_left(rest, 728.f / (728.f + 571.f),  &center,  &right_col);

        ImGuiID energy, lower;
        layout_split_up(right_col, 400.f / (400.f + 400.f + 400.f), &energy, &lower);

        ImGuiID temp, pressure;
        layout_split_up(lower, 400.f / (400.f + 400.f), &temp, &pressure);

        ImGuiID particles, eos_node;
        layout_split_up(center, 700.f / (1200.f), &particles, &eos_node);

        layout_dock("Parameters",   left);
        layout_dock("Particles", particles);
        layout_dock("Energy",       energy);
        layout_dock("Temperature",  temp);
        layout_dock("Pressure",     pressure);
        layout_dock("EOS",          eos_node);
    });

    run_gui_library();
    return 0;
}
