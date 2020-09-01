#include <fstream>
#include <nlohmann/json.hpp>
#include <random>
#include <chrono>
using Json = nlohmann::json;

constexpr auto width = 1000.0f, height = 500.0f, step = 0.2f;
constexpr auto rw = 30.0f, woff = 31.0f, rh = 30.0f, hoff = 60.0f;
std::mt19937_64 eng;

struct Rect final {
    int idx, type;
    int val;
    int status;
    Json tkf, rkf;
};

void initRect(Rect &rect) {
    rect.rkf["type"] = "Rect";
    rect.rkf["color"] = Json::array({ 255, 255, 255 });

    rect.tkf["type"] = "Text";
    rect.tkf["fill"] = true;
    rect.tkf["color"] = Json::array({ 255, 255, 255 });

    rect.status = 0;
}

Json getColor(int status) {
    if (status == 0)
        return Json{ 255, 255, 255 };
    return status == 1 ? Json{ 0, 255, 0 } : Json{ 255, 0, 0 };
}

void recordRect(Rect &rect, float ts, bool ch, bool smooth) {
    Json rk;
    rk["pos"] = Json::array({ rect.idx * woff, rect.type * hoff });
    rk["siz"] = { rw, rh };
    rk["ts"] = ts;
    rk["mix_mode"] = (smooth ? "smoothstep" : "steep");
    rk["color"] = getColor(rect.status);
    rect.rkf["frame"].push_back(rk);

    Json tk;
    tk["center"] = Json::array({ (rect.idx + 0.5f) * woff, rect.type * hoff + 20.0f });
    tk["size"] = 15.0f;
    tk["text"] = (ch ? std::string(1, rect.val) : std::to_string(rect.val));
    tk["mix_mode"] = rk["mix_mode"];
    tk["ts"] = ts;
    tk["color"] = rk["color"];
    rect.tkf["frame"].push_back(tk);
}

void recordArrow(Json &line, int i, int it, float ts) {
    Json lk;
    lk["beg"] = { (i + 0.5f) * woff, it * hoff + rh };
    lk["end"] = { (i + 0.5f) * woff, (it + 1) * hoff };
    lk["end_arrow"] = rh * 0.2f;
    lk["beg_arrow"] = rh * 0.2f;
    lk["ts"] = ts;
    lk["mix_mode"] = "smoothstep";
    line["frame"].push_back(lk);
}

std::pair<float, Json> run() {
    std::string patstr = "ABAABABABBABAAB";

    int pat = static_cast<int>(patstr.size());

    std::vector<Rect> pattern(pat);
    for (int i = 0; i < pat; ++i) {
        Rect &rect = pattern[i];
        rect.idx = i;
        rect.type = 1;
        initRect(rect);
        rect.val = patstr[i];
        recordRect(rect, 0.0f, true, false);
    }

    std::vector<Rect> comp(pat);
    for (int i = 0; i < pat; ++i) {
        Rect &rect = comp[i];
        rect.idx = i;
        rect.type = 2;
        initRect(rect);
        rect.val = patstr[i];
        recordRect(rect, 0.0f, true, true);
    }

    float dur = 0.0f;

    Json arrow;
    arrow["type"] = "Line";
    arrow["color"] = { 255, 255, 255 };
    arrow["layer"] = 1;
    recordArrow(arrow, 0, 1, dur);

    std::vector<Rect> nxt(pat + 1);
    for (int i = 1; i <= pat; ++i) {
        Rect &rect = nxt[i];
        rect.idx = i - 1;
        rect.type = 3;
        initRect(rect);
        rect.val = -1;
        recordRect(rect, 0.0f, false, false);
    }

    auto equal = [&] (int p, int cur) {

        for (int k = 0; k < 2; ++k) {
            dur += step;
            for (int i = 0; i < pat; ++i) {
                Rect &rect = comp[i];
                rect.idx = i + cur - p;
                recordRect(rect, dur, true, true);
            }
            recordArrow(arrow, cur, 1, dur);
        }

        bool eq = (patstr[p] == patstr[cur]);

        pattern[cur].status = (eq ? 1 : 2);
        recordRect(pattern[cur], dur, true, false);
        comp[p].status = (eq ? 1 : 2);
        recordRect(comp[p], dur, true, false);
        dur += step;
        pattern[cur].status = 0;
        recordRect(pattern[cur], dur, true, false);
        comp[p].status = 0;
        for (int i = 0; i < pat; ++i) {
            recordRect(comp[i], dur, true, false);
        }

        return eq;
    };

    int p = 0;
    nxt[1].val = 0;
    recordRect(nxt[1], dur, false, false);
    recordArrow(arrow, p, 1, dur);

    for (int i = 1; i < pat; ++i) {

        dur += step;
        recordArrow(arrow, i, 1, dur);

        while (p && !equal(p, i)) {
            p = nxt[p].val;
        }

        if (p ? (patstr[p] == patstr[i]) : equal(p, i)) {
            ++p;
        }

        nxt[i + 1].val = static_cast<int>(p);
        dur += step;
        recordRect(nxt[i + 1], dur, false, false);
        recordArrow(arrow, i, 1, dur);
    }

    Json drawables;
    recordArrow(arrow, pat - 1, 1, dur);
    for (auto &&pat : comp) {
        recordRect(pat, dur, true, true);
        drawables.push_back(pat.tkf);
        drawables.push_back(pat.rkf);
    }
    dur += step;
    for (auto &&pat : pattern) {
        recordRect(pat, dur, true, false);
    }
    for (size_t i = 1; i <= pat; ++i) {
        recordRect(nxt[i], dur, false, false);
    }

    dur += step;

    for (auto &&pat : pattern) {
        pat.type = 2;
        recordRect(pat, dur, true, true);
    }

    //std::string patstr = "ABAABABABBABAAB";
    std::string scan = "ABAABAABABABBABAABABABABBABAABABAB";

    int num = static_cast<int>(scan.size());

    std::vector<Rect> scanStr(num);

    for (int i = 0; i < num; ++i) {
        Rect &rect = scanStr[i];
        rect.idx = i;
        rect.type = 1;
        initRect(rect);
        rect.val = scan[i];
        recordRect(rect, dur, true, true);
    }

    dur += 1.0f;

    for (auto &&pat : pattern) {
        recordRect(pat, dur, true, false);
    }
    for (int i = 1; i <= pat; ++i) {
        recordRect(nxt[i], dur, false, false);
    }
    recordArrow(arrow, 0, 1, dur);
    for (auto &&ch : scanStr) {
        recordRect(ch, dur, true, false);
    }

    struct Line final {
        int beg, end;
        float height;
        Json lkf;
        Line() {
            lkf["type"] = "Line";
            lkf["color"] = { 0, 255, 0 };
        }
        void record(int offset, float ts) {
            Json lk;
            lk["beg"] = { (beg + offset) * woff, hoff + rh + height };
            lk["end"] = { (end + offset) * woff, hoff + rh + height };
            lk["ts"] = ts;
            lk["mix_mode"] = "smoothstep";
            lkf["frame"].push_back(lk);
        }
    };

    std::vector<Line> lines;

    int coff = 0, lpoff = -1, lcoff = -1, lcur = -1;

    auto equal2 = [&] (int p, int cur) {

        int delta = cur - p;
        int poff = std::min(delta, 8);
        coff = poff - delta;

        for (int k = 0; k < 2; ++k) {
            dur += step;
            if (lpoff != poff) {
                for (int i = 0; i < pat; ++i) {
                    Rect &rect = pattern[i];
                    rect.idx = i + poff;
                    recordRect(rect, dur, true, true);
                }
                for (int i = 1; i <= pat; ++i) {
                    Rect &rect = nxt[i];
                    rect.idx = i - 1 + poff;
                    recordRect(rect, dur, false, true);
                }
            }
            recordArrow(arrow, cur + coff, 1, dur);
            if (lcoff != coff) {
                for (int i = 0; i < num; ++i) {
                    Rect &rect = scanStr[i];
                    rect.idx = i + coff;
                    recordRect(rect, dur, true, true);
                }
                for (auto &&l : lines)
                    l.record(coff, dur);
            }
        }

        lpoff = poff, lcoff = coff, lcur = cur;

        bool eq = (patstr[p] == scan[cur]);

        scanStr[cur].status = (eq ? 1 : 2);
        recordRect(scanStr[cur], dur, true, false);
        pattern[p].status = (eq ? 1 : 2);
        recordRect(pattern[p], dur, true, false);
        dur += step;
        scanStr[cur].status = 0;
        for (int i = 0; i < num; ++i) {
            Rect &rect = scanStr[i];
            recordRect(rect, dur, true, true);
        }
        pattern[p].status = 0;
        for (int i = 0; i < pat; ++i) {
            recordRect(pattern[i], dur, true, false);
        }
        for (int i = 1; i <= pat; ++i) {
            recordRect(nxt[i], dur, false, false);
        }
        for (auto &&l : lines)
            l.record(coff, dur);

        return eq;
    };

    p = 0;
    for (int i = 0; i < num; ++i) {
        while (p && !equal2(p, i))
            p = nxt[p].val;
        if (p ? (patstr[p] == scan[i]) : equal2(p, i))
            ++p;
        if (p == pat) {
            Line line;
            line.beg = i - pat + 1;
            line.end = i + 1;
            std::uniform_real_distribution<float> urd(2.0f, 7.0f);
            line.height = urd(eng);
            dur += step;
            line.record(coff, dur);
            dur += step;
            line.record(coff, dur);
            lines.push_back(line);
            p = nxt[p].val;
        }
    }
    recordArrow(arrow, scanStr.back().idx, 1, dur);

    dur += 2.0f;

    for (int i = 0; i < pat; ++i) {
        pattern[i].idx = i;
        recordRect(pattern[i], dur, true, true);
        drawables.push_back(pattern[i].tkf);
        drawables.push_back(pattern[i].rkf);
    }
    for (int i = 1; i <= pat; ++i) {
        nxt[i].idx = i - 1;
        recordRect(nxt[i], dur, false, true);
        drawables.push_back(nxt[i].tkf);
        drawables.push_back(nxt[i].rkf);
    }
    for (int i = 0; i < num; ++i) {
        scanStr[i].idx = i;
        recordRect(scanStr[i], dur, true, true);
        drawables.push_back(scanStr[i].tkf);
        drawables.push_back(scanStr[i].rkf);
    }

    for (auto &&l : lines) {
        l.record(0, dur);
        drawables.push_back(l.lkf);
    }

    drawables.push_back(arrow);

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
