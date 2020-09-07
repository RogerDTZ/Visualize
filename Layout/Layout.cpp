#include "Layout.hpp"
#include <graphviz/gvc.h>
#include <graphviz/gvplugin_render.h>
#include <graphviz/gvplugin_device.h>
#include <graphviz/gvplugin_textlayout.h>
#include <graphviz/gvconfig.h>
#include <climits>

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
        frame["center"] = { cpos.x, cpos.y };
        frame["rx"] = m_size.x * scale;
        frame["ry"] = m_size.y * scale;
    }
public:
    Ellipse(obj_state_t *state, const glm::vec2 &pos, const glm::vec2 &size) :Primitive(state, "Ellipse"), m_pos(pos), m_size(size) {}
};
class Bezierline final :public Primitive {
private:
    std::vector<glm::vec2> m_vert;
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto &&ps = frame["verts"];
        for (auto &&p : m_vert) {
            auto cp = cast(p);
            ps.push_back(Json{ cp.x, cp.y });
        }
    }
public:
    Bezierline(obj_state_t *state, const std::vector<glm::vec2> &vert) :Primitive(state, "Bezierline"), m_vert(vert) {}
};
class Polyline final :public Primitive {
private:
    std::vector<glm::vec2> m_vert;
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto &&ps = frame["verts"];
        for (auto &&p : m_vert) {
            auto cp = cast(p);
            ps.push_back(Json{ cp.x, cp.y });
        }
    }
public:
    Polyline(obj_state_t *state, const std::vector<glm::vec2> &vert) :Primitive(state, "Polyline"), m_vert(vert) {}
};
class Polygon final :public Primitive {
private:
    std::vector<glm::vec2> m_vert;
    void record(Json &frame, const PointCast &cast, float scale) const override {
        auto &&ps = frame["verts"];
        for (auto &&p : m_vert) {
            auto cp = cast(p);
            ps.push_back(Json{ cp.x, cp.y });
        }
    }
public:
    Polygon(obj_state_t *state, const std::vector<glm::vec2> &vert) :Primitive(state, "Polygon"), m_vert(vert) {}
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
        case NODE_OBJTYPE:return job->obj->u.n->base.tag.id * 3; break;
        case EDGE_OBJTYPE:return job->obj->u.e->base.tag.id * 3 + 1; break;
        case ROOTGRAPH_OBJTYPE:return job->obj->u.g->base.tag.id * 3 + 2; break;
        default:throw;
            break;
    }
}

extern "C" {
    extern __declspec(dllimport) gvplugin_library_t gvplugin_core_LTX_library;
    extern __declspec(dllimport) gvplugin_library_t gvplugin_neato_layout_LTX_library;
    extern __declspec(dllimport) gvplugin_library_t gvplugin_dot_layout_LTX_library;
}
boolean textlayout(textspan_t *span, char **fontpath) {
    span->layout = nullptr;
    span->free_layout = nullptr;
    span->size.x = span->font->size * strlen(span->str);
    span->size.y = span->font->size;
    span->yoffset_layout = 0;
    span->yoffset_centerline = 0;
    return TRUE;
}
void textspan(GVJ_t *job, pointf p, textspan_t *text) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    ctx->pris[getID(job)][PrimitiveType::text].push_back(std::make_shared<Text>(job->obj, text->str, castVec2(p), static_cast<float>(text->font->size)));
}
void polygon(GVJ_t *job, pointf *A, int n, int filled) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    std::vector<glm::vec2> vert(n);
    for (int i = 0; i < n; ++i)
        vert[i] = castVec2(A[i]);
    ctx->pris[getID(job)][PrimitiveType::polygon].push_back(std::make_shared<Polygon>(job->obj, vert));
}
void resolve_color(GVJ_t *job, gvcolor_t *col) {
    switch (col->type) {
        case RGBA_BYTE:break;
        default:throw;
            break;
    }
    col->type = RGBA_BYTE;
}
void polyline(GVJ_t *job, pointf *A, int n) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    std::vector<glm::vec2> vert(n);
    for (int i = 0; i < n; ++i)
        vert[i] = castVec2(A[i]);
    ctx->pris[getID(job)][PrimitiveType::polyline].push_back(std::make_shared<Polyline>(job->obj, vert));
}
void beziercurve(GVJ_t *job, pointf *A, int n, int arrow_beg, int arrow_end, int) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    std::vector<glm::vec2> vert(n);
    for (int i = 0; i < n; ++i)
        vert[i] = castVec2(A[i]);
    ctx->pris[getID(job)][PrimitiveType::curve].push_back(std::make_shared<Bezierline>(job->obj, vert));
}
void ellipse(GVJ_t *job, pointf *A, int filled) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    auto center = A[0];
    auto hw = A[1].x - A[0].x;
    auto hh = A[1].y - A[0].y;
    ctx->pris[getID(job)][PrimitiveType::ellipse].push_back(std::make_shared<Ellipse>(job->obj, castVec2(center), castVec2({ hw, hh })));
}
void begin_graph(GVJ_t *job) {
    auto ctx = reinterpret_cast<Frame *>(job->context);
    ctx->size = castVec2(job->pageSize);
}

gvrender_engine_t engine = {
    nullptr,				/* begin_job */
    nullptr,				/* end_job */
    begin_graph,/*begin_graph*/
    nullptr,/*end_graph*/
    nullptr,				/* begin_layer */
    nullptr,				/* end_layer */
    nullptr,				/* begin_page */
    nullptr,				/* end_page */
    nullptr,				/* begin_cluster */
    nullptr,				/* end_cluster */
    nullptr,				/* begin_nodes */
    nullptr,				/* end_nodes */
    nullptr,				/* begin_edges */
    nullptr,				/* end_edges */
    nullptr,				/* begin_node */
    nullptr,				/* end_node */
    nullptr,				/* begin_edge */
    nullptr,				/* end_edge */
    nullptr,				/* begin_anchor */
    nullptr,				/* end_anchor */
    nullptr,				/* begin_label */
    nullptr,				/* end_label */
    textspan,				/* textspan */
    resolve_color,				/* resolve_color */
    ellipse,				/* ellipse */
    polygon,				/* polygon */
    beziercurve,				/* bezier */
    polyline,				/* polyline */
    nullptr,				/* comment */
    nullptr,				/* library_shape */
};

gvrender_features_t render_features = {
    GVRENDER_Y_GOES_DOWN,	/* not really - uses raw graph coords */  /* flags */
    10.0,                         /* default pad - graph units */
    nullptr,			/* knowncolors */
    0,				/* sizeof knowncolors */
    RGBA_BYTE		/* color_type */
};

gvplugin_installed_t gvrender_types[] = {
    { 12364984, "layout", 1000, &engine, &render_features },
    { 0, nullptr, 0, nullptr, nullptr }
};

gvdevice_features_t device_features = {
    GVDEVICE_NO_WRITER,				/* flags */
    { 10.0, 10.0 },			/* default margin - points */
    { 0., 0. },			/* default page width, height - points */
    { 72.0, 72.0 },			/* default dpi */
};

gvplugin_installed_t gvdevice_types[] = {
    { 12364984, "layout:layout", 10000, nullptr, &device_features },
    { 0, nullptr, 0, nullptr, nullptr }
};

gvtextlayout_engine_t textlayout_engine = {
    textlayout
};

gvplugin_installed_t gvtextlayout_types[] = {
    { 0, "textlayout", 8, &textlayout_engine, nullptr },
    { 0, nullptr, 0, nullptr, nullptr }
};

gvplugin_api_t apis[] = {
    { API_device, gvdevice_types },
    { API_render, gvrender_types },
    { API_textlayout, gvtextlayout_types },
    { static_cast<api_t>(0), 0 }
};

gvplugin_library_t lib = { "layout", apis };

LayoutContext::LayoutContext() {
    m_context = gvContext();
    gvAddLibrary(m_context, &lib);
    gvAddLibrary(m_context, &gvplugin_core_LTX_library);
    gvAddLibrary(m_context, &gvplugin_dot_layout_LTX_library);
    gvAddLibrary(m_context, &gvplugin_neato_layout_LTX_library);

    /*
    int sz;
    char **list = gvPluginList(m_context, "textlayout", &sz, nullptr);
    for (int i = 0; i < sz; ++i)
        std::cout << list[i] << std::endl;
        */
}

Frame LayoutContext::render(Agraph_t *g) {
    gvLayout(m_context, g, "dot");
    Frame frame;
    gvRenderContext(m_context, g, "layout", &frame);
    gvFreeLayout(m_context, g);
    return frame;
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
                auto &&buk = buk2.second;

                const std::remove_reference_t<decltype(buk)> *cmp2 = &empty;
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
    gvFinalize(m_context);
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

Primitive::Primitive(obj_state_t *style, const std::string &type) :m_width(static_cast<float>(style->penwidth)), m_type(type), jsonIdx(invalidIdx) {
    assert(style->pencolor.type == color_type_t::RGBA_BYTE);
    static_assert(sizeof(RGBA) == 4);
    memcpy(&m_col, style->pencolor.u.rgba, sizeof(m_col));
}

void Primitive::record(Json &drawables, size_t idx, float ts, const PointCast &cast, float scale) const {
    if (idx == invalidIdx) {
        idx = drawables.size();
        drawables.push_back(init());
    }

    jsonIdx = idx;

    Json frame;
    frame["ts"] = ts;
    record(frame, cast, scale);
    drawables[idx]["frame"].push_back(frame);
}

Primitive::~Primitive() {}
