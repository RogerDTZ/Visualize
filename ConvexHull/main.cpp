#include <fstream>
#include <random>
#include <chrono>
#include <glm/glm.hpp>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <stack>
using Json = nlohmann::json;

constexpr auto count = 200;
constexpr auto width = 1000.0f, height = 800.0f, step = 0.1f;
std::mt19937_64 eng;

struct Point final {
    glm::vec2 pos;
    float angle;
    bool inStack;
    size_t id;
    Json pkf, tkf;
};

struct Edge final {
    glm::vec2 beg;
    glm::vec2 end;
    float bt;
};

void initPoint(Point &point) {
    point.pkf["type"] = "Circle";
    point.pkf["fill"] = true;
    point.pkf["color"] = Json::array({ 255, 255, 255 });

    point.tkf["type"] = "Text";
    point.tkf["fill"] = true;
    point.tkf["color"] = Json::array({ 255, 255, 255 });
}

void recordPoint(Point &point, float ts) {
    Json pk;
    pk["center"] = Json::array({ point.pos.x, height - point.pos.y });
    pk["radius"] = (point.inStack ? 5.0f : 3.0f);
    pk["ts"] = ts;
    pk["mix_mode"] = "steep";
    point.pkf["frame"].push_back(pk);
}

void recordPointText(Point &point, float ts) {
    Json tk;
    tk["center"] = Json::array({ point.pos.x, height - (point.pos.y + 10.0f) });
    tk["size"] = 15.0f;
    tk["text"] = std::to_string(point.id);
    tk["mix_mode"] = "steep";
    tk["ts"] = ts;
    point.tkf["frame"].push_back(tk);
}

Json genLine(glm::vec2 beg, glm::vec2 end, float bt, float et) {
    Json json;
    json["type"] = "Line";
    json["color"] = Json::array({ 255, 255, 255 });

    Json kf;
    kf["beg"] = Json::array({ beg.x, height - beg.y });
    kf["end"] = Json::array({ end.x, height - end.y });
    kf["ts"] = bt;
    json["frame"].push_back(kf);
    kf["ts"] = et;
    json["frame"].push_back(kf);
    return json;
}

Json genRay(glm::vec2 beg, glm::vec2 end, float bt, float et) {
    Json json;
    json["type"] = "Ray";
    json["color"] = Json::array({ 255, 255, 255 });

    Json kf;
    kf["origin"] = Json::array({ beg.x, height - beg.y });
    auto delta = end - beg;
    kf["angle"] = atan2(-delta.y, delta.x);
    kf["arrow"] = 10.0f;
    kf["arrow_offset"] = 20.0f;
    kf["ts"] = bt;
    json["frame"].push_back(kf);
    kf["ts"] = et;
    json["frame"].push_back(kf);
    return json;
}
//Graham

bool test(Json &drawables, std::vector<size_t> &stack, std::vector<Point> &points, size_t i, float &dur) {
    auto &&p1 = points[stack[stack.size() - 2]];
    auto &&p2 = points[stack[stack.size() - 1]];
    auto &&p3 = points[i];

    drawables.push_back(genRay(p1.pos, p2.pos, dur, dur + step));
    drawables.push_back(genRay(p2.pos, p3.pos, dur, dur + step));
    dur += step;

    auto delta1 = p2.pos - p1.pos;
    auto delta2 = p3.pos - p2.pos;

    return delta1.x * delta2.y - delta2.x * delta1.y <= 0.0f;
}

std::pair<float, Json> run() {
    float dur = 0.0f;

    std::vector<Point> points(count);
    std::uniform_real_distribution<float> urd(0.0f, 500.0f);
    std::generate(points.begin(), points.end(), [&] {
        Point res;
        float ang = urd(eng), rad = urd(eng);
        res.pos = glm::vec2{ 0.95f * cos(ang) * rad + 500.0f, sin(ang) * 0.75f * rad + 400.0f };
        res.inStack = false;
        return res;
        });

    while (true) {
        std::vector<Point> keep;
        bool flag = false;
        for (size_t i = 0; i < points.size(); ++i) {
            for (size_t j = i + 1; j < points.size(); ++j) {
                if (glm::length(points[i].pos - points[j].pos) < 20.0f) {
                    flag = true;
                    points.erase(points.begin() + i);
                    break;
                }
            }
            if (flag)break;
        }
        if (!flag)break;
    }

    for (size_t i = 0; i < points.size(); ++i) {
        points[i].id = i;
        initPoint(points[i]);
        recordPoint(points[i], 0.0f);
        recordPointText(points[i], 0.0f);
    }

    std::sort(points.begin(), points.end(), [] (const Point &lhs, const Point &rhs) {return lhs.pos.y == rhs.pos.y ? lhs.pos.x < rhs.pos.x : lhs.pos.y < rhs.pos.y; });
    std::stack<Edge> edges;
    std::vector<size_t> stack;

    stack.push_back(0);
    points[0].inStack = true;
    recordPoint(points[0], 1.0f);

    Json drawables;

    for (size_t i = 1; i < points.size(); ++i) {
        auto delta = points[i].pos - points[0].pos;
        points[i].angle = atan2(delta.y, delta.x);
        drawables.push_back(genLine(points[0].pos, points[i].pos, 1.1f, 2.0f));
    }

    std::sort(points.begin() + 1, points.end(), [] (const Point &lhs, const Point &rhs) {return lhs.angle < rhs.angle; });

    for (size_t i = 0; i < points.size(); ++i) {
        points[i].id = i;
        recordPointText(points[i], 2.0f);
    }
    dur = 2.0f;

    for (size_t i = 1; i < points.size(); ++i) {
        while (stack.size() >= 2 && test(drawables, stack, points, i, dur)) {
            points[stack.back()].inStack = false;
            recordPoint(points[stack.back()], dur);
            stack.pop_back();

            auto &&edge = edges.top();
            drawables.push_back(genLine(edge.beg, edge.end, edge.bt, dur));
            edges.pop();
        }

        dur += step;
        points[i].inStack = true;
        recordPoint(points[i], dur);

        edges.push(Edge{ points[stack.back()].pos, points[i].pos, dur });
        stack.push_back(i);
    }

    {
        dur += step;
        edges.push(Edge{ points[stack.back()].pos, points[0].pos, dur });
    }

    dur += 2.0f;
    for (auto &&p : points) {
        recordPoint(p, dur);
        recordPointText(p, dur);
    }

    for (auto &&p : points) {
        drawables.push_back(p.pkf);
        drawables.push_back(p.tkf);
    }

    while (edges.size()) {
        auto edge = edges.top();
        edges.pop();
        drawables.push_back(genLine(edge.beg, edge.end, edge.bt, dur));
    }

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
