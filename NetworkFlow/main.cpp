#include <fstream>
#include <nlohmann/json.hpp>
#include "../Layout/Layout.hpp"
using Json = nlohmann::json;


constexpr auto width = 1000.0f, height = 500.0f, step = 1.0f;
int main() {
    std::ifstream in("netflow.in");
    size_t n, m, s, t;
    in >> n >> m >> s >> t;

    LayoutContext ctx;
    Graph g(ctx, "netflow", true);
    float dur = 0.0f;
    std::vector<std::pair<float, Frame>> frames;
    auto render = [&] {
        dur += step;
        frames.emplace_back(dur, g.recordKeyFrame());
    };

    std::vector<Agnode_t *> nodes(n);
    auto getNode = [&] (size_t id) {
        render();
        if (!nodes[id])
            nodes[id] = g.allocNode();
        render();
        return nodes[id];
    };

    for (size_t i = 0; i < m; ++i) {
        size_t u, v, w;
        in >> u >> v >> w;
        auto un = getNode(u);
        auto vn = getNode(v);
        render();
        g.linkEdge(un, vn);
        render();
    }

    Json json;
    json["virtual_width"] = width;
    json["virtual_height"] = height;
    json["duration"] = dur;
    ctx.renderFrames(json["drawables"], frames, width, height);
    std::ofstream out("output.json");
    out << json;
    return 0;
}
