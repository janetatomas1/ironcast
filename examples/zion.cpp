
#include <cfloat>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <pwd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <ironcast/ironcast.hpp>
#include <ironcast/tabwidget.hpp>
#include <ironcast/pane.hpp>
#include <ironcast/theme.hpp>
#include <ironcast/window.hpp>
#include <ironcast/font.hpp>
#include <ironcast/image.hpp>
#include <ironcast/dockarea.hpp>
#include <ironcast/table.hpp>
#include <ironcast/button.hpp>
#include <ironcast/checkbox.hpp>
#include <ironcast/dropdown.hpp>
#include <ironcast/extras/chartwidget.hpp>
#include <ironcast/text.hpp>
#include <ironcast/textinput.hpp>
#include <ironcast/logwidget.hpp>

#include "ironcast/sdlgpu/backend.hpp"

using namespace ironcast;
using namespace literals;

constexpr char glyphs[] = "0123456789ABCDEFXYZ:.=*+-<>|$#@";

// Animated "digital rain" background painter for Window::set_background_painter(). Falling columns of
// glyphs in the current font, coloured with the theme's text colour. Keeps the loop drawing while set.
class MatrixRain {
    struct Column {
        float head;   // row of the leading glyph, may be negative while above the window
        float speed;  // rows per second
    };

    std::vector<Column> columns_;
    std::minstd_rand rng_{std::random_device{}()};
    int rows_ = 0;

    Column spawn(bool initial);
    [[nodiscard]] char glyph_at(int column, int row, double time) const;

public:
    float opacity = 0.3f;
    // Average fall speed in rows per second; each column varies around it.
    float speed = 10.0f;
    // Glyphs per column, fading from the head upwards.
    int trail = 22;
    // How often a glyph changes, per second.
    float glyphChangeRate = 6.0f;

    void operator()(ImDrawList& list, ImVec2 min, ImVec2 max);
};

MatrixRain::Column MatrixRain::spawn(bool initial) {
    std::uniform_real_distribution<float> start(initial ? -static_cast<float>(rows_) : -0.5f * rows_,
                                                initial ? static_cast<float>(rows_) : 0.0f);
    std::uniform_real_distribution<float> variation(0.5f, 1.5f);
    return {start(rng_), speed * variation(rng_)};
}

char MatrixRain::glyph_at(int column, int row, double time) const {
    // Stateless flicker: each cell changes at its own pace, so nothing per cell has to be stored.
    const auto tick = static_cast<std::uint32_t>(time * glyphChangeRate * (0.5 + (column % 7) / 7.0));
    const std::uint32_t hash = static_cast<std::uint32_t>(column) * 73856093u
        ^ static_cast<std::uint32_t>(row) * 19349663u
        ^ tick * 83492791u;
    return glyphs[hash % (sizeof(glyphs) - 1)];
}

void MatrixRain::operator()(ImDrawList& list, ImVec2 min, ImVec2 max) {
    ImFont* font = ImGui::GetFont();
    const float size = ImGui::GetFontSize();
    const float cellWidth = font->CalcTextSizeA(size, FLT_MAX, 0.0f, "0").x;

    if (cellWidth <= 0.0f || size <= 0.0f || trail <= 0) {
        return;
    }
    const int columns = static_cast<int>(std::ceil((max.x - min.x) / cellWidth));
    rows_ = static_cast<int>(std::ceil((max.y - min.y) / size));

    // New window width: start over, scattered so the screen isn't empty on the first frames.
    if (static_cast<int>(columns_.size()) != columns) {
        columns_.clear();

        for (int i = 0; i < columns; i++) {
            columns_.push_back(spawn(true));
        }
    }
    const float dt = ImGui::GetIO().DeltaTime;
    const double time = ImGui::GetTime();
    const ImVec4 text = ImGui::GetStyleColorVec4(ImGuiCol_Text);
    // The leading glyph is the text colour pushed most of the way to white.
    const ImVec4 head{
        text.x + (1.0f - text.x) * 0.7f,
        text.y + (1.0f - text.y) * 0.7f,
        text.z + (1.0f - text.z) * 0.7f,
        text.w
    };

    for (int c = 0; c < columns; c++) {
        Column& column = columns_[c];
        column.head += column.speed * dt;

        if (column.head - static_cast<float>(trail) > static_cast<float>(rows_)) {
            column = spawn(false);
        }
        const int headRow = static_cast<int>(std::floor(column.head));

        for (int k = 0; k < trail; k++) {
            const int row = headRow - k;

            if (row < 0) {
                break;
            }
            if (row >= rows_) {
                continue;
            }
            const float fade = 1.0f - static_cast<float>(k) / static_cast<float>(trail);
            ImVec4 color = k == 0 ? head : text;
            color.w *= opacity * fade * fade;
            const char glyph[2] = {glyph_at(c, row, time), '\0'};
            list.AddText(
                font,
                size,
                ImVec2(min.x + static_cast<float>(c) * cellWidth, min.y + static_cast<float>(row) * size),
                ImGui::ColorConvertFloat4ToU32(color),
                glyph,
                glyph + 1
            );
        }
    }

    if (Ironcast::has_current()) {
        Ironcast::current().request_redraw();
    }
}

static std::string read_line(const std::string& path) {
    std::ifstream stream(path);
    std::string line;
    std::getline(stream, line);
    return line;
}

// One logical CPU: name, a usage sparkline over the last minute, current usage and frequency.
struct CoreRow : public Widget {
    static constexpr std::size_t capacity = 60;

    Text name;
    ChartWidget chart;
    LineSeries& usage = chart.add_series<LineSeries>("##usage", SeriesRing(capacity), SeriesRing(capacity));
    Text value = Text("", this);

    CoreRow(int index, Widget* parent): Widget("", parent),
        name(std::format("CPU {:<2}", index), this),
        chart(std::format("##core{}", index), this) {
        flex_direction(YGFlexDirectionRow);
        align_items(YGAlignCenter);
        // gap(2);
        chart.height(70).max_width(45_pcts).flex_shrink(1);
        chart.set_flags(ImPlotFlags_CanvasOnly).set_legend({});
        chart.set_x_axis({.flags = ImPlotAxisFlags_NoDecorations});
        chart.set_y_axis({.flags = ImPlotAxisFlags_NoDecorations, .limits = ImPlotRange(0, 100)});
    }

    void sample(double time, float percent, float frequencyMHz) {
        usage.xs<SeriesRing>().add(time);
        usage.ys<SeriesRing>().add(percent);
        chart.x_axis().limits = ImPlotRange(time - static_cast<double>(capacity), time);
        value.set_title(std::format("{:3.0f}%  {:4.0f} MHz", percent, frequencyMHz));
    }
};

struct CPUTab : public Widget {
    struct CPUInfo {
        std::string model;
        std::string vendor;
        std::string architecture;
        int cores = 0;
        int threads = 0;
        float minFrequencyMHz = 0;
        float maxFrequencyMHz = 0;
        std::size_t cacheL1d = 0;
        std::size_t cacheL2 = 0;
        std::size_t cacheL3 = 0;
    };

    static CPUInfo read_cpu_info() {
        CPUInfo info;

        std::ifstream stream("/proc/cpuinfo");
        std::string line;
        while (std::getline(stream, line) && !line.empty()) {
            const auto colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            const auto key = line.substr(0, line.find_last_not_of(" \t", colon - 1) + 1);
            const auto value = colon + 2 <= line.size() ? line.substr(colon + 2) : std::string{};

            if (key == "model name") {
                info.model = value;
            } else if (key == "vendor_id") {
                info.vendor = value;
            } else if (key == "cpu cores") {
                info.cores = std::stoi(value);
            } else if (key == "siblings") {
                info.threads = std::stoi(value);
            }
        }

        utsname name{};
        if (uname(&name) == 0) {
            info.architecture = name.machine;
        }

        const std::string cpu0 = "/sys/devices/system/cpu/cpu0/";

        // cpufreq reports kHz.
        if (auto value = read_line(cpu0 + "cpufreq/cpuinfo_min_freq"); !value.empty()) {
            info.minFrequencyMHz = std::stof(value) / 1000.0f;
        }
        if (auto value = read_line(cpu0 + "cpufreq/cpuinfo_max_freq"); !value.empty()) {
            info.maxFrequencyMHz = std::stof(value) / 1000.0f;
        }

        // Sizes as "32K"; Instruction caches are skipped.
        for (int index = 0;; ++index) {
            const auto dir = cpu0 + "cache/index" + std::to_string(index) + "/";
            const auto level = read_line(dir + "level");
            if (level.empty()) {
                break;
            }
            const auto type = read_line(dir + "type");
            const std::size_t size = std::stoul(read_line(dir + "size")) * 1024;

            if (level == "1" && type == "Data") {
                info.cacheL1d = size;
            } else if (level == "2") {
                info.cacheL2 = size;
            } else if (level == "3") {
                info.cacheL3 = size;
            }
        }

        spdlog::info("cpu info fetched");
        return info;
    }

    // Package temperature input: temp1 of k10temp or zenpower (AMD Tctl), or the coretemp
    // input labelled "Package id 0" (Intel). Empty for other vendors.
    static std::string find_temperature_path() {
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon", error)) {
            const auto dir = entry.path().string() + "/";
            const auto name = read_line(dir + "name");
            if (name == "k10temp" || name == "zenpower") {
                return dir + "temp1_input";
            }
            if (name == "coretemp") {
                for (int index = 1;; ++index) {
                    const auto label = read_line(std::format("{}temp{}_label", dir, index));
                    if (label.empty()) {
                        break;
                    }
                    if (label == "Package id 0") {
                        return std::format("{}temp{}_input", dir, index);
                    }
                }
            }
        }
        return {};
    }

    // Jiffies from one "cpuN" line of /proc/stat.
    struct CoreTimes {
        unsigned long long idle = 0;
        unsigned long long total = 0;
    };

    static std::vector<CoreTimes> read_core_times() {
        std::vector<CoreTimes> times;
        std::ifstream stream("/proc/stat");
        std::string label;

        while (stream >> label && label.starts_with("cpu")) {
            // user nice system idle iowait irq softirq steal
            unsigned long long fields[8] = {};
            for (auto& field : fields) {
                stream >> field;
            }
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            if (label == "cpu") {
                continue;
            }
            CoreTimes core{.idle = fields[3] + fields[4]};
            for (auto field : fields) {
                core.total += field;
            }
            times.push_back(core);
        }
        return times;
    }

    // Left column: the info table with the load toggle under it.
    Widget side {"", this};
    Table cpu {2, &side};
    Widget coreList {"", this};
    std::jthread burner;

    void set_burn(bool on) {
        if (on) {
            burner = std::jthread([](std::stop_token stop) {
                while (!stop.stop_requested()) {}
            });
            spdlog::info("cpu load started");
        } else {
            // Assigning an empty jthread stops and joins the running one.
            burner = std::jthread();
            spdlog::info("cpu load stopped");
        }
    }
    std::vector<std::unique_ptr<CoreRow>> cores;
    std::vector<CoreTimes> lastTimes = read_core_times();
    double lastSample = 0;
    const std::string temperaturePath = find_temperature_path();
    Text* temperature = nullptr;

    bool show() override {
        const double time = ImGui::GetTime();
        if (time - lastSample >= 1.0) {
            lastSample = time;
            auto times = read_core_times();

            for (std::size_t i = 0; i < cores.size() && i < times.size(); ++i) {
                const auto total = times[i].total - lastTimes[i].total;
                const auto idle = times[i].idle - lastTimes[i].idle;
                const float percent = total > 0 ? 100.0f * static_cast<float>(total - idle) / static_cast<float>(total) : 0.0f;

                const auto freq = read_line(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/scaling_cur_freq", i));
                cores[i]->sample(time, percent, freq.empty() ? 0.0f : std::stof(freq) / 1000.0f);
            }
            lastTimes = std::move(times);

            // hwmon reports millidegrees.
            if (auto value = read_line(temperaturePath); !value.empty()) {
                temperature->set_title(std::format("{:.1f} °C", std::stof(value) / 1000.0f));
            }
        }
        return Widget::show();
    }

    CPUTab(): Widget("CPU") {
        flex_direction(YGFlexDirectionRow);
        side.margin(2_pcts).width(100_pcts).max_width(20_pcts).gap(10);
        coreList.margin(1_pcts).flex_basis(0).flex_grow(1).flex_direction(YGFlexDirectionRow);
        coreList.layout().flex_wrap(YGWrapWrap);
        coreList.layout().align_content(YGAlignFlexStart);

        for (std::size_t i = 0; i < lastTimes.size(); ++i) {
            auto& core = *cores.emplace_back(std::make_unique<CoreRow>(static_cast<int>(i), &coreList));
            core.width(47_pcts).margin(1_pcts, YGEdgeHorizontal).margin(4, YGEdgeVertical);
        }

        const auto info = read_cpu_info();

        auto row = [this](const std::string& name, const std::string& value) {
            cpu.add_cell<Text>(name);
            cpu.add_cell<Text>(value);
        };

        row("Model", info.model);
        row("Vendor", info.vendor);
        row("Architecture", info.architecture);
        row("Cores", std::to_string(info.cores));
        row("Threads", std::to_string(info.threads));
        row("Min frequency", std::format("{:.0f} MHz", info.minFrequencyMHz));
        row("Max frequency", std::format("{:.0f} MHz", info.maxFrequencyMHz));
        row("L1d cache", std::format("{} KB", info.cacheL1d / 1024));
        row("L2 cache", std::format("{} KB", info.cacheL2 / 1024));
        row("L3 cache", std::format("{} KB", info.cacheL3 / 1024));

        cpu.add_cell<Text>("Temperature");
        temperature = &cpu.add_cell<Text>(temperaturePath.empty() ? "Unsupported vendor" : "n/a");

        cpu.add_cell<Text>("Burn CPU");
        cpu.add_cell<Checkbox>("", [this](const bool on) { set_burn(on); });
    }
};

// One /sys/class/drm/cardN: static info read once, live rows refreshed by sample().
// amdgpu reports everything; i915 and xe (Intel) only clocks, and temperature and power on
// discrete cards. Other drivers show "Unsupported vendor" instead of the table.
struct GPUCard : public Widget {
    struct GPUInfo {
        std::string card;          // "card2"
        std::string vendor;        // from device/vendor
        std::string deviceId;      // device/device, e.g. "0x15bf"
        std::string driver;        // device/driver symlink: "amdgpu", "nouveau", "i915"
        std::string pciAddress;    // device symlink, e.g. "0000:c4:00.0"
        std::size_t vramTotal = 0; // bytes, amdgpu only
        std::size_t gttTotal = 0;  // bytes, amdgpu only
        float maxCoreClockMHz = NAN;
        float maxMemClockMHz = NAN;
    };

    // Current ("*") and highest level of a pp_dpm_* table with lines like "1: 800Mhz *".
    struct Clock {
        float current = NAN;
        float max = NAN;
    };

    static Clock read_clock(const std::string& path) {
        Clock clock;
        std::ifstream stream(path);
        std::string line;
        while (std::getline(stream, line)) {
            const auto colon = line.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            // Levels are listed lowest first.
            clock.max = std::stof(line.substr(colon + 1));
            if (line.ends_with('*')) {
                clock.current = clock.max;
            }
        }
        return clock;
    }

    // value / scale, NAN if the file is missing.
    static float read_float(const std::string& path, float scale = 1.0f) {
        const auto value = read_line(path);
        return value.empty() ? NAN : std::stof(value) / scale;
    }

    static std::size_t read_size(const std::string& path) {
        const auto value = read_line(path);
        return value.empty() ? 0 : std::stoull(value);
    }

    static std::string vendor_name(const std::string& id) {
        if (id == "0x1002") return "AMD";
        if (id == "0x10de") return "NVIDIA";
        if (id == "0x8086") return "Intel";
        return id;
    }

    static std::string mib(std::size_t bytes) {
        return std::format("{} MiB", bytes >> 20);
    }

    // text, or "n/a" when value is NAN.
    static std::string or_na(float value, std::string text) {
        return std::isnan(value) ? "n/a" : std::move(text);
    }

    // Driver-specific: amdgpu has a pp_dpm_sclk level table, i915 and xe plain MHz files.
    [[nodiscard]] Clock core_clock() const {
        if (info.driver == "amdgpu") {
            return read_clock(device + "pp_dpm_sclk");
        }
        if (info.driver == "i915") {
            return {read_float(cardDir + "gt_cur_freq_mhz"), read_float(cardDir + "gt_max_freq_mhz")};
        }
        if (info.driver == "xe") {
            return {read_float(device + "tile0/gt0/freq0/cur_freq"), read_float(device + "tile0/gt0/freq0/max_freq")};
        }
        return {};
    }

    [[nodiscard]] bool supported() const {
        return info.driver == "amdgpu" || info.driver == "i915" || info.driver == "xe";
    }

    GPUInfo info;
    std::string cardDir; // ".../cardN/"
    std::string device;  // ".../cardN/device/"
    std::string hwmon;   // ".../device/hwmon/hwmonM/", empty if the driver has none
    Table table {2, this};
    Text unsupported {"", this};
    Text* usage = nullptr;
    Text* vram = nullptr;
    Text* gtt = nullptr;
    Text* coreClock = nullptr;
    Text* memClock = nullptr;
    Text* temperature = nullptr;
    Text* power = nullptr;

    GPUCard(const std::filesystem::path& card, Widget* parent): Widget("", parent),
        cardDir(card.string() + "/"),
        device(card.string() + "/device/") {
        std::error_code error;
        info.card = card.filename();
        info.vendor = vendor_name(read_line(device + "vendor"));
        info.deviceId = read_line(device + "device");
        info.driver = std::filesystem::read_symlink(device + "driver", error).filename();
        info.pciAddress = std::filesystem::canonical(device, error).filename();
        info.vramTotal = read_size(device + "mem_info_vram_total");
        info.gttTotal = read_size(device + "mem_info_gtt_total");
        info.maxCoreClockMHz = core_clock().max;
        info.maxMemClockMHz = read_clock(device + "pp_dpm_mclk").max;

        if (!supported()) {
            table.display(YGDisplayNone);
            unsupported.set_title(std::format("Unsupported vendor: {} ({})", info.vendor, info.driver));
            spdlog::info("gpu unsupported: {} {}", info.card, info.driver);
            return;
        }
        unsupported.display(YGDisplayNone);

        for (const auto& entry : std::filesystem::directory_iterator(device + "hwmon", error)) {
            hwmon = entry.path().string() + "/";
            break;
        }

        auto row = [this](const std::string& name, const std::string& value) {
            table.add_cell<Text>(name);
            return &table.add_cell<Text>(value);
        };

        row("Card", info.card);
        row("Vendor", info.vendor);
        row("Device", info.deviceId);
        row("Driver", info.driver);
        row("PCI", info.pciAddress);
        row("Max core clock", or_na(info.maxCoreClockMHz, std::format("{:.0f} MHz", info.maxCoreClockMHz)));
        row("Max memory clock", or_na(info.maxMemClockMHz, std::format("{:.0f} MHz", info.maxMemClockMHz)));
        usage = row("Usage", "n/a");
        vram = row("VRAM", "n/a");
        gtt = row("GTT", "n/a");
        coreClock = row("Core clock", "n/a");
        memClock = row("Memory clock", "n/a");
        temperature = row("Temperature", "n/a");
        power = row("Power", "n/a");

        spdlog::info("gpu info fetched: {} {}", info.card, info.driver);
    }

    // True when the driver reports usage, i.e. the live rows are worth showing.
    [[nodiscard]] bool reports_usage() const {
        return !read_line(device + "gpu_busy_percent").empty();
    }

    void sample() {
        if (!supported()) {
            return;
        }
        const float busy = read_float(device + "gpu_busy_percent");
        usage->set_title(or_na(busy, std::format("{:.0f}%", busy)));

        if (info.vramTotal > 0) {
            vram->set_title(std::format("{} / {}", mib(read_size(device + "mem_info_vram_used")), mib(info.vramTotal)));
        }
        if (info.gttTotal > 0) {
            gtt->set_title(std::format("{} / {}", mib(read_size(device + "mem_info_gtt_used")), mib(info.gttTotal)));
        }

        const float core = core_clock().current;
        coreClock->set_title(or_na(core, std::format("{:.0f} MHz", core)));
        const float mem = read_clock(device + "pp_dpm_mclk").current;
        memClock->set_title(or_na(mem, std::format("{:.0f} MHz", mem)));

        if (hwmon.empty()) {
            return;
        }
        // hwmon reports millidegrees and microwatts.
        const float celsius = read_float(hwmon + "temp1_input", 1000.0f);
        temperature->set_title(or_na(celsius, std::format("{:.1f} °C", celsius)));
        float watts = read_float(hwmon + "power1_average", 1e6f);
        if (std::isnan(watts)) {
            watts = read_float(hwmon + "power1_input", 1e6f);
        }
        power->set_title(or_na(watts, std::format("{:.1f} W", watts)));
    }
};

// System memory from /proc/meminfo, refreshed by sample().
struct RAMTable : public Table {
    // /proc/meminfo in bytes, keyed by field name ("MemTotal", ...). Values are in kB there.
    static std::unordered_map<std::string, std::size_t> read_meminfo() {
        std::unordered_map<std::string, std::size_t> fields;
        std::ifstream stream("/proc/meminfo");
        std::string key;
        std::size_t value = 0;
        while (stream >> key >> value) {
            key.pop_back();  // trailing ':'
            fields[key] = value * 1024;
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        return fields;
    }

    static std::string gib(std::size_t bytes) {
        return std::format("{:.1f} GiB", static_cast<double>(bytes) / (1 << 30));
    }

    // The memory modules from SMBIOS type 17, as decoded by udev at boot: lines like
    // "E:MEMORY_DEVICE_0_MANUFACTURER=Micron Technology". The raw DMI table is root-only, this
    // copy isn't. Keyed without the "E:" prefix; empty if the file doesn't exist (no systemd-udev).
    static std::unordered_map<std::string, std::string> read_memory_devices() {
        std::unordered_map<std::string, std::string> fields;
        std::ifstream stream("/run/udev/data/+dmi:id");
        std::string line;
        while (std::getline(stream, line)) {
            const auto equals = line.find('=');
            if (line.starts_with("E:MEMORY_DEVICE_") && equals != std::string::npos) {
                fields[line.substr(2, equals - 2)] = line.substr(equals + 1);
            }
        }
        return fields;
    }

    Text* total = nullptr;
    Text* used = nullptr;
    Text* available = nullptr;
    Text* cached = nullptr;
    Text* swap = nullptr;

    RAMTable(Widget* parent): Table(2, parent) {
        auto row = [this](const std::string& name) {
            add_cell<Text>(name);
            return &add_cell<Text>("n/a");
        };

        total = row("Total");
        used = row("Used");
        available = row("Available");
        cached = row("Cached");
        swap = row("Swap");

        // Three rows per module: slot with size and type, then vendor and part number.
        // Empty slots have a locator but no size.
        auto modules = read_memory_devices();
        if (modules.empty()) {
            add_cell<Text>("Modules");
            add_cell<Text>("n/a");
        }
        for (int index = 0; modules.contains(std::format("MEMORY_DEVICE_{}_LOCATOR", index)); ++index) {
            auto field = [&](const char* name) -> const std::string& {
                return modules[std::format("MEMORY_DEVICE_{}_{}", index, name)];
            };
            if (field("SIZE").empty()) {
                continue;
            }
            add_cell<Text>(field("BANK_LOCATOR"));
            add_cell<Text>(std::format("{} {}-{} {}", gib(std::stoull(field("SIZE"))),
                                       field("TYPE"), field("SPEED_MTS"), field("FORM_FACTOR")));
            add_cell<Text>("  Vendor");
            add_cell<Text>(field("MANUFACTURER"));
            add_cell<Text>("  Part");
            add_cell<Text>(field("PART_NUMBER"));
        }

        sample();
    }

    void sample() {
        auto fields = read_meminfo();
        const auto memTotal = fields["MemTotal"];
        const auto memAvailable = fields["MemAvailable"];
        const auto usedBytes = memTotal - memAvailable;

        total->set_title(gib(memTotal));
        used->set_title(std::format("{} ({:.0f}%)", gib(usedBytes),
                                    memTotal > 0 ? 100.0 * static_cast<double>(usedBytes) / static_cast<double>(memTotal) : 0.0));
        available->set_title(gib(memAvailable));
        cached->set_title(gib(fields["Cached"]));
        swap->set_title(std::format("{} / {}", gib(fields["SwapTotal"] - fields["SwapFree"]), gib(fields["SwapTotal"])));
    }
};

// GPU on the left: a dropdown of the DRM cards, only the selected card's table is displayed
// and sampled. System RAM on the right.
struct GPUTab : public Widget {
    Widget side {"", this};
    Dropdown picker {"Card", &side};
    std::vector<std::unique_ptr<GPUCard>> cards;
    std::size_t current = 0;
    RAMTable ram {this};
    double lastSample = 0;

    void select(std::size_t index) {
        cards[current]->display(YGDisplayNone);
        current = index;
        cards[current]->display(YGDisplayFlex);
        cards[current]->sample();
    }

    bool show() override {
        const double time = ImGui::GetTime();
        if (time - lastSample >= 1.0) {
            lastSample = time;
            ram.sample();
            if (!cards.empty()) {
                cards[current]->sample();
            }
        }
        return Widget::show();
    }

    GPUTab(): Widget("GPU & RAM") {
        flex_direction(YGFlexDirectionRow);
        side.margin(2_pcts).width(100_pcts).max_width(30_pcts).gap(10);
        ram.margin(2_pcts).width(100_pcts).max_width(30_pcts);

        // "cardN" only; "card1-DP-1" and the like are connectors.
        std::vector<std::filesystem::path> paths;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm", error)) {
            const auto name = entry.path().filename().string();
            if (name.starts_with("card") && name.size() > 4 &&
                name.find_first_not_of("0123456789", 4) == std::string::npos) {
                paths.push_back(entry.path());
            }
        }
        std::ranges::sort(paths);

        for (const auto& path : paths) {
            auto& card = *cards.emplace_back(std::make_unique<GPUCard>(path, &side));
            card.display(YGDisplayNone);
            picker.add_option(std::format("{} · {} ({})", card.info.card, card.info.driver, card.info.vendor));
        }

        if (cards.empty()) {
            picker.set_placeholder("No GPU found");
            return;
        }

        // Prefer a card that reports usage (amdgpu), then any supported one (Intel).
        auto preferred = std::ranges::find_if(cards, [](const auto& card) { return card->reports_usage(); });
        if (preferred == cards.end()) {
            preferred = std::ranges::find_if(cards, [](const auto& card) { return card->supported(); });
        }
        const auto index = preferred == cards.end() ? 0 : static_cast<std::size_t>(preferred - cards.begin());
        picker.set_selected(static_cast<int>(index));
        select(index);
        picker.set_on_change([this](int selected) { select(static_cast<std::size_t>(selected)); });
    }
};

// One /sys/block disk: static info read once, live throughput and temperature from sample().
struct DiskCard : public Widget {
    // sysfs pads model, vendor and serial with spaces.
    static std::string trim(std::string text) {
        text.erase(text.find_last_not_of(' ') + 1);
        return text.empty() ? "n/a" : text;
    }

    // Sectors read and written, fields 3 and 7 of /sys/block/X/stat. Always 512-byte units.
    static std::pair<std::size_t, std::size_t> read_sectors(const std::string& dir) {
        std::ifstream stream(dir + "stat");
        std::size_t fields[7] = {};
        for (auto& field : fields) {
            stream >> field;
        }
        return {fields[2], fields[6]};
    }

    // NVMe drives have device/hwmonN, SATA drives with drivetemp device/hwmon/hwmonN.
    static std::string find_hwmon(const std::string& device) {
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(device + "hwmon", error)) {
            return entry.path().string() + "/";
        }
        for (const auto& entry : std::filesystem::directory_iterator(device, error)) {
            if (entry.path().filename().string().starts_with("hwmon")) {
                return entry.path().string() + "/";
            }
        }
        return {};
    }

    std::string name;  // "nvme0n1"
    std::string dir;   // "/sys/block/nvme0n1/"
    std::string hwmon;
    Table table {2, this};
    Text* readSpeed = nullptr;
    Text* writeSpeed = nullptr;
    Text* temperature = nullptr;
    std::size_t lastRead = 0;
    std::size_t lastWritten = 0;
    double lastTime = NAN;

    DiskCard(const std::filesystem::path& path, Widget* parent): Widget("", parent),
        name(path.filename()),
        dir("/sys/block/" + name + "/"),
        hwmon(find_hwmon(dir + "device/")) {
        auto row = [this](const std::string& label, const std::string& value) {
            table.add_cell<Text>(label);
            return &table.add_cell<Text>(value);
        };

        const auto transport = read_line(dir + "device/transport");
        const double size = static_cast<double>(GPUCard::read_size(dir + "size")) * 512;

        row("Disk", name);
        row("Model", trim(read_line(dir + "device/model")));
        row("Vendor", trim(read_line(dir + "device/vendor")));
        row("Serial", trim(read_line(dir + "device/serial")));
        row("Firmware", trim(read_line(dir + "device/firmware_rev")));
        row("Transport", transport.empty() ? "n/a" : transport);
        row("Type", read_line(dir + "queue/rotational") == "1" ? "HDD" : "SSD");
        row("Size", std::format("{:.1f} GB", size / 1e9));
        row("Removable", read_line(dir + "removable") == "1" ? "yes" : "no");
        readSpeed = row("Read", "n/a");
        writeSpeed = row("Write", "n/a");
        temperature = row("Temperature", "n/a");

        spdlog::info("disk info fetched: {}", name);
    }

    // Throughput over the time since the previous call; the first call only sets the baseline.
    void sample(double time) {
        const auto [read, written] = read_sectors(dir);
        if (!std::isnan(lastTime) && time > lastTime) {
            const double seconds = time - lastTime;
            readSpeed->set_title(std::format("{:.1f} MB/s", static_cast<double>(read - lastRead) * 512 / seconds / 1e6));
            writeSpeed->set_title(std::format("{:.1f} MB/s", static_cast<double>(written - lastWritten) * 512 / seconds / 1e6));
        }
        lastRead = read;
        lastWritten = written;
        lastTime = time;

        // hwmon reports millidegrees.
        if (!hwmon.empty()) {
            const float celsius = GPUCard::read_float(hwmon + "temp1_input", 1000.0f);
            temperature->set_title(GPUCard::or_na(celsius, std::format("{:.1f} °C", celsius)));
        }
    }
};

// Mounted /dev filesystems from /proc/mounts, with usage from statvfs refreshed by sample().
// The list is read once; mounts made later don't show up.
struct FilesystemTable : public Table {
    struct Mount {
        std::string path;
        Text* usage = nullptr;
    };

    std::vector<Mount> mounts;

    FilesystemTable(Widget* parent): Table({"Mount", "Device", "Type", "Used"}, parent) {
        std::ifstream stream("/proc/mounts");
        std::string device, path, type;
        while (stream >> device >> path >> type) {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (!device.starts_with("/dev/")) {
                continue;
            }
            add_cell<Text>(path);
            add_cell<Text>(std::filesystem::path(device).filename().string());
            add_cell<Text>(type);
            mounts.push_back({path, &add_cell<Text>("n/a")});
        }
        sample();
    }

    void sample() {
        for (auto& mount : mounts) {
            struct statvfs stats{};
            if (statvfs(mount.path.c_str(), &stats) != 0) {
                continue;
            }
            const double total = static_cast<double>(stats.f_blocks) * stats.f_frsize;
            const double used = total - static_cast<double>(stats.f_bfree) * stats.f_frsize;
            mount.usage->set_title(std::format("{:.1f} / {:.1f} GB ({:.0f}%)", used / 1e9, total / 1e9,
                                               total > 0 ? 100.0 * used / total : 0.0));
        }
    }
};

// Disks on the left: a dropdown of the /sys/block devices, only the selected disk's table is
// displayed. Mounted filesystems on the right.
struct DiskTab : public Widget {
    Widget side {"", this};
    Dropdown picker {"Disk", &side};
    std::vector<std::unique_ptr<DiskCard>> disks;
    std::size_t current = 0;
    FilesystemTable filesystems {this};
    double lastSample = 0;

    void select(std::size_t index) {
        disks[current]->display(YGDisplayNone);
        current = index;
        disks[current]->display(YGDisplayFlex);
    }

    // Every disk is sampled, not just the shown one, so switching shows a fresh rate.
    bool show() override {
        const double time = ImGui::GetTime();
        if (time - lastSample >= 1.0) {
            lastSample = time;
            filesystems.sample();
            for (auto& disk : disks) {
                disk->sample(time);
            }
        }
        return Widget::show();
    }

    DiskTab(): Widget("Disk") {
        flex_direction(YGFlexDirectionRow);
        side.margin(2_pcts).width(100_pcts).max_width(30_pcts).gap(10);
        filesystems.margin(2_pcts).width(100_pcts).max_width(50_pcts);

        // Physical disks only: loop, zram and device-mapper devices are virtual, and an empty
        // card reader reports size 0.
        std::vector<std::filesystem::path> paths;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/block", error)) {
            const auto name = entry.path().filename().string();
            if (name.starts_with("loop") || name.starts_with("zram") || name.starts_with("dm-") ||
                name.starts_with("ram") || read_line(entry.path().string() + "/size") == "0") {
                continue;
            }
            paths.push_back(entry.path());
        }
        std::ranges::sort(paths);

        for (const auto& path : paths) {
            auto& disk = *disks.emplace_back(std::make_unique<DiskCard>(path, &side));
            disk.display(YGDisplayNone);
            picker.add_option(disk.name);
        }

        if (disks.empty()) {
            picker.set_placeholder("No disk found");
            return;
        }

        picker.set_selected(0);
        select(0);
        picker.set_on_change([this](int selected) { select(static_cast<std::size_t>(selected)); });
    }
};

// Every process in /proc as a sortable, filterable ImGui table. Rows are drawn directly with a
// list clipper, so hundreds of processes refreshing every second stay cheap.
// No intrinsic size: give it one (flex_grow).
struct ProcessList : public Widget {
    struct Process {
        int pid = 0;
        std::string name;
        std::string user;
        char state = '?';
        int threads = 0;
        std::size_t memory = 0;        // resident, bytes
        float cpu = 0;                 // %, 100 = one core fully busy, like top
        unsigned long long ticks = 0;  // utime + stime, in clock ticks
    };

    std::vector<Process> processes;
    // Indices into processes, filtered and sorted.
    std::vector<int> visible;
    std::string filter;
    int selected = -1;  // pid
    // Set when processes or the filter changed; the table rebuilds visible on its next show().
    bool resort = true;
    std::unordered_map<uid_t, std::string> users;
    double lastTime = NAN;

    ProcessList(Widget* parent): Widget("", parent) {}

    const std::string& user_name(uid_t uid) {
        auto [it, inserted] = users.try_emplace(uid);
        if (inserted) {
            const passwd* entry = getpwuid(uid);
            it->second = entry ? entry->pw_name : std::to_string(uid);
        }
        return it->second;
    }

    // Rereads /proc. CPU % is the tick delta since the previous call; 0 on the first one.
    void sample(double time) {
        static const long clockTicks = sysconf(_SC_CLK_TCK);
        static const long pageSize = sysconf(_SC_PAGESIZE);
        const double seconds = time - lastTime;

        std::unordered_map<int, unsigned long long> lastTicks;
        for (const auto& process : processes) {
            lastTicks[process.pid] = process.ticks;
        }
        processes.clear();

        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("/proc", error)) {
            const auto pid = entry.path().filename().string();
            if (pid.find_first_not_of("0123456789") != std::string::npos) {
                continue;
            }

            // "1234 (name) S 1 ...": the name may contain spaces and parentheses, so split at the last ')'.
            const auto line = read_line(entry.path().string() + "/stat");
            const auto open = line.find('(');
            const auto close = line.rfind(')');
            if (open == std::string::npos || close == std::string::npos) {
                continue;  // exited meanwhile
            }
            std::istringstream stream(line.substr(close + 1));
            const std::vector<std::string> fields{std::istream_iterator<std::string>(stream), {}};
            // fields[0] is field 3 of proc_pid_stat(5): state; utime 14, stime 15, num_threads 20, rss 24.
            if (fields.size() < 22) {
                continue;
            }

            Process process{
                .pid = std::stoi(pid),
                .name = line.substr(open + 1, close - open - 1),
                .state = fields[0][0],
                .threads = std::stoi(fields[17]),
                .memory = std::stoull(fields[21]) * pageSize,
                .ticks = std::stoull(fields[11]) + std::stoull(fields[12]),
            };
            struct stat owner{};
            if (::stat(entry.path().c_str(), &owner) == 0) {
                process.user = user_name(owner.st_uid);
            }
            if (const auto last = lastTicks.find(process.pid); last != lastTicks.end() && seconds > 0) {
                process.cpu = static_cast<float>(100.0 * static_cast<double>(process.ticks - last->second) / clockTicks / seconds);
            }
            processes.push_back(std::move(process));
        }

        lastTime = time;
        resort = true;
    }

    // Filter: case-insensitive part of the name, or the start of the PID.
    void update_visible(const ImGuiTableSortSpecs& specs) {
        auto lower = [](std::string text) {
            std::ranges::transform(text, text.begin(), [](unsigned char c) { return std::tolower(c); });
            return text;
        };
        const auto needle = lower(filter);

        visible.clear();
        for (int i = 0; i < static_cast<int>(processes.size()); ++i) {
            const auto& process = processes[i];
            if (needle.empty() || lower(process.name).contains(needle) || std::to_string(process.pid).starts_with(needle)) {
                visible.push_back(i);
            }
        }

        if (specs.SpecsCount == 0) {
            return;
        }
        const auto& spec = specs.Specs[0];
        auto less = [&](const Process& a, const Process& b) {
            switch (spec.ColumnIndex) {
                case 0: return a.pid < b.pid;
                case 1: return a.name < b.name;
                case 2: return a.user < b.user;
                case 3: return a.state < b.state;
                case 4: return a.threads < b.threads;
                case 5: return a.cpu < b.cpu;
                case 6: return a.memory < b.memory;
                default: return false;
            }
        };
        std::ranges::stable_sort(visible, [&](int a, int b) {
            return spec.SortDirection == ImGuiSortDirection_Ascending
                ? less(processes[a], processes[b])
                : less(processes[b], processes[a]);
        });
    }

    bool show() override {
        constexpr ImGuiTableFlags flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit;

        ImGui::PushID(static_cast<int>(id()));
        if (ImGui::BeginTable("##processes", 7, flags, size())) {
            ImGui::TableSetupColumn("PID");
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("User");
            ImGui::TableSetupColumn("State");
            ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_PreferSortDescending);
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableHeadersRow();

            if (auto* specs = ImGui::TableGetSortSpecs(); specs && (specs->SpecsDirty || resort)) {
                update_visible(*specs);
                specs->SpecsDirty = false;
                resort = false;
            }

            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(visible.size()));
            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                    const auto& process = processes[visible[row]];
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    const auto pid = std::to_string(process.pid);
                    if (ImGui::Selectable(pid.c_str(), process.pid == selected, ImGuiSelectableFlags_SpanAllColumns)) {
                        selected = process.pid;
                    }
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(process.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(process.user.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%c", process.state);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", process.threads);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.1f", process.cpu);
                    ImGui::TableNextColumn();
                    ImGui::Text("%.1f MiB", static_cast<double>(process.memory) / (1 << 20));
                }
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
        return false;
    }
};

// A filter and an "End process" button above the process table.
struct ProcessesTab : public Widget {
    Widget toolbar {"", this};
    TextInput filter {"##filter", &toolbar};
    Text count {"", &toolbar};
    Button end {"End process", [this] { end_selected(); }, &toolbar};
    ProcessList list {this};
    double lastSample = 0;

    // SIGTERM, so the process can clean up. Fails with EPERM for other users' processes.
    void end_selected() {
        if (kill(list.selected, SIGTERM) == 0) {
            spdlog::info("sent SIGTERM to {}", list.selected);
        } else {
            spdlog::error("can't end {}: {}", list.selected, std::strerror(errno));
        }
    }

    bool show() override {
        const double time = ImGui::GetTime();
        if (time - lastSample >= 1.0) {
            lastSample = time;
            list.sample(time);
            count.set_title(std::format("{} processes", list.processes.size()));
        }
        end.set_enabled(list.selected > 0);
        return Widget::show();
    }

    ProcessesTab(): Widget("Processes") {
        column(4.0f);
        margin(1_pcts);
        toolbar.row(8.0f).align_items(YGAlignCenter);
        filter.set_hint("Filter by name or PID");
        filter.flex_grow(1);
        filter.set_on_change([this](const std::string& text) {
            list.filter = text;
            list.resort = true;
        });
        list.flex_grow(1);
        list.sample(0.0);
    }
};

// Machine, OS and session info read once; uptime, load, processes and battery from sample().
struct SystemTab : public Widget {
    // PRETTY_NAME from /etc/os-release, e.g. "Fedora Linux 44 (KDE Plasma Desktop Edition)".
    static std::string read_os_name() {
        std::ifstream stream("/etc/os-release");
        std::string line;
        while (std::getline(stream, line)) {
            if (line.starts_with("PRETTY_NAME=")) {
                auto value = line.substr(12);
                if (value.size() >= 2 && value.front() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                return value;
            }
        }
        return "n/a";
    }

    // First power supply of type "Battery", e.g. "/sys/class/power_supply/BAT0/"; empty if none.
    static std::string find_battery() {
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/power_supply", error)) {
            const auto dir = entry.path().string() + "/";
            if (read_line(dir + "type") == "Battery") {
                return dir;
            }
        }
        return {};
    }

    static std::string or_na(const std::string& value) {
        return value.empty() ? "n/a" : value;
    }

    static std::string env_or_na(const char* name) {
        const char* value = std::getenv(name);
        return value ? value : "n/a";
    }

    Table table {2, this};
    std::string battery = find_battery();
    Text* uptime = nullptr;
    Text* load = nullptr;
    Text* processes = nullptr;
    Text* batteryLevel = nullptr;
    Text* batteryPower = nullptr;
    double lastSample = 0;

    bool show() override {
        const double time = ImGui::GetTime();
        if (time - lastSample >= 1.0) {
            lastSample = time;
            sample();
        }
        return Widget::show();
    }

    void sample() {
        const auto seconds = static_cast<long>(GPUCard::read_float("/proc/uptime"));
        uptime->set_title(std::format("{}d {}h {:02}m", seconds / 86400, seconds / 3600 % 24, seconds / 60 % 60));

        // "1.47 1.04 0.93 1/1806 13547": three load averages, then running/total tasks.
        std::ifstream stream("/proc/loadavg");
        float load1 = 0, load5 = 0, load15 = 0;
        std::string tasks;
        stream >> load1 >> load5 >> load15 >> tasks;
        load->set_title(std::format("{:.2f}  {:.2f}  {:.2f}", load1, load5, load15));
        processes->set_title(tasks.substr(tasks.find('/') + 1));

        if (!battery.empty()) {
            batteryLevel->set_title(std::format("{}% ({})", read_line(battery + "capacity"), read_line(battery + "status")));
            // power_now is in microwatts.
            const float watts = GPUCard::read_float(battery + "power_now", 1e6f);
            batteryPower->set_title(GPUCard::or_na(watts, std::format("{:.1f} W", watts)));
        }
    }

    SystemTab(): Widget("System") {
        table.margin(2_pcts).width(100_pcts).max_width(40_pcts);

        auto row = [this](const std::string& name, const std::string& value) {
            table.add_cell<Text>(name);
            return &table.add_cell<Text>(value);
        };

        utsname name{};
        uname(&name);
        const std::string dmi = "/sys/class/dmi/id/";

        row("Hostname", name.nodename);
        row("OS", read_os_name());
        row("Kernel", name.release);
        row("Architecture", name.machine);
        row("Desktop", env_or_na("XDG_CURRENT_DESKTOP"));
        row("Session", env_or_na("XDG_SESSION_TYPE"));
        row("Manufacturer", or_na(read_line(dmi + "sys_vendor")));
        row("Model", or_na(read_line(dmi + "product_name")));
        row("Board", or_na(read_line(dmi + "board_name")));
        row("BIOS", std::format("{} ({})", or_na(read_line(dmi + "bios_version")), or_na(read_line(dmi + "bios_date"))));
        uptime = row("Uptime", "n/a");
        load = row("Load", "n/a");
        processes = row("Processes", "n/a");
        if (!battery.empty()) {
            batteryLevel = row("Battery", "n/a");
            batteryPower = row("Battery power", "n/a");
        }

        sample();
        spdlog::info("system info fetched");
    }
};

struct Zion: TabWidget {
    Zion(Widget* parent = nullptr): TabWidget(parent) {
        add_tab<SystemTab>();
        add_tab<CPUTab>();
        add_tab<GPUTab>();
        add_tab<DiskTab>();
        add_tab<ProcessesTab>();
    }
};

struct TabsPane : public Pane {
    Zion tabs = Zion(this);

    TabsPane(): Pane("Monitor") {
        set_dock_flags(ImGuiDockNodeFlags_NoTabBar)
        .set_window_flags(ImGuiWindowFlags_NoBackground);
        tabs.flex_grow(1);
    }
};

struct LogPane : public Pane {
    LogWidget log = LogWidget(this);

    LogPane(): Pane("Log") {
        log.flex_grow(1);
        set_closable(false);
        set_dock_flags(ImGuiDockNodeFlags_NoTabBar)
        .set_window_flags(ImGuiWindowFlags_NoBackground);
    }
};

struct Root : public DockArea {
    Root() {
        add_pane<TabsPane>();
        add_pane<LogPane>().set_dock(ImGuiDir_Down).set_ratio(0.3f);
    }
};

int main(const int argc, char** argv) {
    auto app = Ironcast(argc, argv);

    Theme theme = Theme::matrix();
    theme.font = &Font::jetbrains_mono();

    app.set_theme(theme);
    app.set_background_painter([] {
        return BackgroundPainter(MatrixRain{});
    });

    app.set_backend_factory([] (const WindowConfig &config) {
       return std::make_unique<sdlgpu::Backend>(config);
    });

    if (auto icon = Image::from_file(IRONCAST_EXAMPLES_ASSETS "/zion-icon.png")) {
        app.set_icon(&app.add_image(std::move(*icon)));
    }

    app.add_window<Root>();
    spdlog::info("zion started, F12 opens the widget inspector");

    return app.exec();
}
