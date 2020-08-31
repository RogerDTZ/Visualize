#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <chrono>
using Json = nlohmann::json;

constexpr size_t num = 100, pat = 10, setSize = 2;
constexpr auto width = 1000.0f, height = 500.0f, step = 0.5f;
constexpr auto rw = 30.0f, woff = 31.0f, rh = 30.0f, hoff = 60.0f;
std::mt19937_64 eng;

std::string genString(size_t cnt) {
    static_assert(setSize <= 26);
    std::vector<char> res(cnt);
    std::uniform_int_distribution<int> uid('A', 'A' + setSize - 1);
    std::generate(res.begin(), res.end(), [&] {return uid(eng); });
    return { res.begin(), res.end() };
}

struct Rect final {
    size_t idx, type;
    int val;
    bool active;
    Json tkf, rkf;
};

void initRect(Rect &rect) {
    rect.rkf["type"] = "Rect";
    rect.rkf["color"] = Json::array({ 255, 255, 255 });

    rect.tkf["type"] = "Text";
    rect.tkf["fill"] = true;
    rect.tkf["color"] = Json::array({ 255, 255, 255 });

    rect.active = false;
}

void recordRect(Rect &rect, float ts, bool ch) {
    Json rk;
    rk["pos"] = Json::array({ rect.idx * woff, rect.type * hoff });
    rk["siz"] = { rw, rh };
    rk["ts"] = ts;
    rk["mix_mode"] = "steep";
    rk["color"] = (rect.active ? Json::array({ 255, 0, 0 }) : Json::array({ 255, 255, 255 }));
    rect.rkf["frame"].push_back(rk);

    Json tk;
    tk["center"] = Json::array({ (rect.idx + 0.5f) * woff, rect.type * hoff + 20.0f });
    tk["size"] = 15.0f;
    tk["text"] = (ch ? std::string(1, rect.val) : std::to_string(rect.val));
    tk["mix_mode"] = "steep";
    tk["ts"] = ts;
    rk["color"] = (rect.active ? Json::array({ 255, 0, 0 }) : Json::array({ 255, 255, 255 }));
    rect.tkf["frame"].push_back(tk);
}

void recordPointer(Json &curve, size_t i, int it, size_t j, int jt, float ts) {
    Json ck;
    ck["beg"] = { (i + 0.5f) * woff, it * hoff };
    ck["end"] = { (j + 0.5f) * woff, jt * hoff };
    ck["ctrl"] = { ((i + j) * 0.5f + 0.5f) * woff, ((it + jt) * 0.5f - 1.0f) * hoff };
    ck["ts"] = ts;
    ck["mix_mode"] = "steep";
    curve["frame"].push_back(ck);
}

void recordArrow(Json &line, size_t i, int it, float ts) {
    Json lk;
    lk["beg"] = { (i + 0.5f) * woff, it * hoff - rh };
    lk["end"] = { (i + 0.5f) * woff, it * hoff };
    lk["end_arrow"] = rh * 0.7f;
    lk["ts"] = ts;
    lk["mix_mode"] = "steep";
    line["frame"].push_back(lk);
}


std::pair<float, Json> run() {
    std::string patstr = "ABABAABABA";
    std::vector<Rect> pattern(pat);
    for (size_t i = 0; i < pat; ++i) {
        Rect &rect = pattern[i];
        rect.idx = i;
        rect.type = 1;
        initRect(rect);
        rect.val = patstr[i];
        recordRect(rect, 0.0f, true);
    }

    Json drawables;

    Json arrow1;
    arrow1["type"] = "Curve";
    arrow1["color"] = { 255, 255, 255 };
    arrow1["layer"] = 1;

    recordPointer(arrow1, 0, 1, 0, 1, 0.0f);

    Json arrow2;
    arrow2["type"] = "Line";
    arrow2["color"] = { 255, 255, 255 };
    arrow2["layer"] = 1;
    recordArrow(arrow2, 0, 1, 0.0f);

    float dur = 0.0f;

    std::vector<Rect> nxt(pat + 1);
    for (size_t i = 1; i <= pat; ++i) {
        Rect &rect = nxt[i];
        rect.idx = i - 1;
        rect.type = 2;
        initRect(rect);
        rect.val = -1;
        recordRect(rect, 0.0f, false);
    }

    auto test = [&] (size_t p, size_t i) {
        dur += step;
        pattern[p].active = true;
        pattern[i].active = true;
        recordRect(pattern[i], dur, true);
        recordRect(pattern[p], dur, true);
        recordPointer(arrow1, i, 1, p, 1, dur);
        dur += step;
        pattern[p].active = false;
        pattern[i].active = false;
        recordRect(pattern[i], dur, true);
        recordRect(pattern[p], dur, true);
        
        return patstr[p] != patstr[i];
    };

    size_t p = 0;
    nxt[1].val = 0;
    dur += step;
    recordRect(nxt[1], dur, false);
    recordArrow(arrow2, p, 1, 0.0f);

    for (size_t i = 1; i < pat; ++i) {
        while (p && test(p, i)) {
            p = nxt[p].val;
            recordArrow(arrow2, p, 1, dur);
        }
        if (patstr[p] == patstr[i]) {
            ++p;
            dur += step;
            recordArrow(arrow2, p, 1, dur);
        }
        nxt[i + 1].val = static_cast<int>(p);
        dur += step;
        recordRect(nxt[i + 1], dur, false);
    }

    dur += 2.0f;
    for (auto &&pat : pattern) {
        recordRect(pat, dur, true);
        drawables.push_back(pat.tkf);
        drawables.push_back(pat.rkf);
    }
    for (size_t i = 1; i <= pat; ++i) {
        recordRect(nxt[i], dur, false);
        drawables.push_back(nxt[i].tkf);
        drawables.push_back(nxt[i].rkf);
    }
    drawables.push_back(arrow1);
    drawables.push_back(arrow2);

    return { dur, drawables };
}
int main() {
    {
        std::random_device src;
        if (src.entropy() != 0.0)
            eng.seed(src());
        else {
            eng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        }
    }
    Json json;
    json["virtual_width"] = width;
    json["virtual_height"] = height;
    auto [dur, ele] = run();
    json["duration"] = dur;
    json["drawables"] = ele;
    std::ofstream out("output.json");
    out << json;
    return 0;
}
