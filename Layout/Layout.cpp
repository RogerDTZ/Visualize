#include "Layout.hpp"
#include <graphviz/gvc.h>
#include <graphviz/gvplugin_render.h>
#include <cstdint>

constexpr auto invalidIdx = std::numeric_limits<size_t>::max();

Json rgba2Json(const RGBA &col) {
    return { col.r, col.g, col.b, col.a };
}

class Ellipse final :public Primitive {
private:
    glm::vec2 m_pos;
    glm::vec2 m_size;
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto cpos = cast(m_pos);
        frame["pos"] = { cpos.x, cpos.y };
        frame["rx"] = m_size.x * scale;
        frame["ry"] = m_size.y * scale;
    }
public:
    Ellipse(obj_state_t *state, const glm::vec2 &pos, const glm::vec2 &size) :Primitive(state, "Ellipse"), m_pos(pos), m_size(size) {}
};

class Polyline final :public Primitive {
private:
    std::vector<glm::vec2> m_vert;
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto ps = frame["verts"];
        for (auto &&p : m_vert) {
            auto cp = cast(p);
            ps.push_back(Json{ cp.x, cp.y });
        }
    }
public:
    Polyline(obj_state_t *state, const std::vector<glm::vec2> &vert) :Primitive(state, "Polyline"), m_vert(vert) {}
};

class Text final :public Primitive {
private:
    std::string m_text;
    glm::vec2 m_pos;
    float m_size;
    Json init() const {
        auto res = Primitive::init();
        res["fill"] = true;
        return res;
    }
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto cpos = cast(m_pos);
        frame["center"] = { cpos.x, cpos.y };
        frame["size"] = m_size * scale;
        frame["text"] = m_text;
    }
public:
    Text(obj_state_t *state, const std::string &text, const glm::vec2 &pos, float size) :Primitive(state, "Text"), m_text(text), m_pos(pos), m_size(size) {}
};

static glm::vec2 castVec2(const pointf &p) {
    return { static_cast<float>(p.x), static_cast<float>(p.y) };
}
static IDTYPE getID(GVJ_t *job) {
    switch (job->obj->type) {
        case NODE_OBJTYPE:return job->obj->u.n->base.tag.id; break;
        case EDGE_OBJTYPE:return job->obj->u.e->base.tag.id; break;
        default:throw;
            break;
    }
}
LayoutContext::LayoutContext() {
    m_context = gvContext();

    gvrender_engine_t eng;
    memset(&eng, 0, sizeof(eng));

    eng.textspan = [] (GVJ_t *job, pointf p, textspan_t *text) {
        auto ctx = reinterpret_cast<LayoutContext *>(job->context);
        ctx->m_frame.pris[getID(job)][PrimitiveType::text].push_back(std::make_shared<Text>(job->obj, text->str, castVec2(p), static_cast<float>(text->font->size)));
    };
    eng.polygon = [] (GVJ_t *job, pointf *A, int n, int filled) {
        throw;
    };
    eng.resolve_color = [] (GVJ_t *job, gvcolor_t *col) {
        throw;
    };
    eng.polyline = [] (GVJ_t *job, pointf *A, int n) {
        auto ctx = reinterpret_cast<LayoutContext *>(job->context);
        std::vector<glm::vec2> vert(n);
        for (int i = 0; i < n; ++i)
            vert[i] = castVec2(A[i]);
        ctx->m_frame.pris[getID(job)][PrimitiveType::polyline].push_back(std::make_shared<Polyline>(job->obj, vert));
    };
    eng.beziercurve = [] (GVJ_t *job, pointf *A, int n, int arrow_beg, int arrow_end, int) {
        throw;
    };
    eng.ellipse = [] (GVJ_t *job, pointf *A, int filled) {
        auto ctx = reinterpret_cast<LayoutContext *>(job->context);
        auto center = A[0];
        auto hw = A[1].x - A[0].x;
        auto hh = A[1].y - A[0].y;
        ctx->m_frame.pris[getID(job)][PrimitiveType::ellipse].push_back(std::make_shared<Ellipse>(job->obj, castVec2(center), castVec2({ hw, hh })));
    };
    eng.begin_graph = [] (GVJ_t *job) {
        auto ctx = reinterpret_cast<LayoutContext *>(job->context);
        ctx->m_frame.size = castVec2(job->pageSize);
    };

    gvrender_features_t features;

    features.color_type = color_type_t::RGBA_BYTE;
    features.default_pad = 10.0;
    features.flags = GVRENDER_Y_GOES_DOWN;
    features.knowncolors = nullptr;
    features.sz_knowncolors = 0;

    gvplugin_installed_t renderer;
    renderer.engine = &eng;
    renderer.features = &features;
    renderer.id = 12364984;
    renderer.quality = 1000;
    renderer.type = "layout";

    gvplugin_api_t api;
    api.api = api_t::API_render;
    api.types = &renderer;

    gvplugin_library_t lib;
    lib.packagename = "layout";
    lib.apis = &api;
    gvAddLibrary(m_context, &lib);
}

Frame LayoutContext::render(Agraph_t *g) {
    m_frame.pris.clear();
    gvRenderContext(m_context, g, "layout", this);
    return m_frame;
}

void LayoutContext::renderFrames(Json &drawables, const std::vector<std::pair<float, Frame>> &frames, float width, float height) const {
    float r1 = width / height;

    const Frame *lastFrame = nullptr;
    std::vector<std::shared_ptr<Primitive>> empty;
    for (auto &&fr : frames) {
        auto ts = fr.first;
        const Frame &frame = fr.second;

        float dw = frame.size.x, dh = frame.size.y;
        float r2 = frame.size.x / frame.size.y;

        float scale = (r2 > r1 ? (width / dw) : height / dh);
        dw *= scale; dh *= scale;
        glm::vec2 offset = { (width - dw) * 0.5f, (height - dh) * 0.5f };

        PointCast cast = [offset, scale] (const glm::vec2 &p) {return p * scale + offset; };

        for (auto &&buk1 : frame.pris) {
            auto id = buk1.first;
            auto &&pris = buk1.second;

            std::remove_reference_t< decltype(pris)> *cmp1 = nullptr;
            if (lastFrame) {
                auto iter = lastFrame->pris.find(id);
                if (iter != lastFrame->pris.cend())
                    cmp1 = &iter->second;
            }

            for (auto &&buk2 : pris) {
                auto t = buk2.first;
                auto buk = buk2.second;


                const decltype(buk) *cmp2 = &empty;
                if (cmp1) {
                    auto iter = cmp1->find(t);
                    if (iter != cmp1->cend())
                        cmp2 = &iter->second;
                }

                for (size_t i = 0; i < std::min(buk.size(), cmp2->size()); ++i)
                    buk[i]->record(drawables, (*cmp2)[i]->jsonIdx, ts, cast, scale);

                for (size_t i = cmp2->size(); i < buk.size(); ++i)
                    buk[i]->record(drawables, invalidIdx, ts, cast, scale);
            }
        }

        lastFrame = &frame;
    }
}

LayoutContext::~LayoutContext() {
    gvFreeContext(m_context);
}

Graph::Graph(LayoutContext &context, const std::string &name, bool directed) :m_context(context) {
    Agdesc_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.directed = directed;
    m_graph = agopen(const_cast<char *>(name.c_str()), desc, nullptr);
}

Graph::~Graph() {
    agclose(m_graph);
}

Agnode_t *Graph::allocNode() {
    return agnode(m_graph, nullptr, 1);
}

Agedge_t *Graph::linkEdge(Agnode_t *a, Agnode_t *b) {
    return agedge(m_graph, a, b, nullptr, 1);
}

void Graph::removeObject(ObjectHandle obj) {
    agdelete(m_graph, obj);
}

void Graph::setColor(ObjectHandle obj, const char *col) {
    agset(obj, "color", const_cast<char *>(col));
}

Frame Graph::recordKeyFrame() {
    return m_context.render(m_graph);
}

Json Primitive::init() const {
    Json base;
    base["color"] = rgba2Json(m_col);
    base["width"] = m_width;
    base["type"] = m_type;
    return base;
}

Primitive::Primitive(obj_state_t *style, const std::string &type) :m_width(static_cast<float>(style->penwidth)), m_type(type) {
    assert(style->pencolor.type == color_type_t::RGBA_BYTE);
    memcpy(&m_col, style->pencolor.u.rgba, sizeof(m_col));
}

void Primitive::record(Json &drawables, size_t idx, float ts, const PointCast &cast, float scale) const {
    if (idx == invalidIdx) {
        jsonIdx = idx = drawables.size();
        drawables.push_back(init());
    }
    Json frame;
    frame["ts"] = ts;
    record(frame, cast, scale);
    drawables[idx]["frame"] = frame;
}

Primitive::~Primitive() {}
