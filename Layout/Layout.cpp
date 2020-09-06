#include "Layout.hpp"
#include <graphviz/gvc.h>
#include <graphviz/gvplugin_render.h>

LayoutContext::LayoutContext() {
    m_context = gvContext();

    gvrender_engine_t eng;
    memset(eng, 0, sizeof(eng));

    eng.textspan = ;
    eng.polygon = ;
    eng.polyline = ;
    eng.beziercurve = ;
    eng.ellipse = [] (GVJ_t *job, pointf *A, int filled) {
        auto center = A[0];
        auto hw = A[1].x - A[0].x;
        auto hh = A[1].y - A[0].y;
        job->obj->u.n->base.tag.id;
    };

    gvrender_features_t features;

    features.color_type = color_type_t::RGBA_DOUBLE;
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

GVC_t *LayoutContext::context() {
    return m_context;
}

LayoutContext::~LayoutContext() {
    gvFreeContext(m_context);
}

Graph::Graph(LayoutContext &context, const std::string &name, bool directed) :m_context(context) {
    Agdesc_t desc;
    memset(desc, 0, sizeof(desc));
    desc.directed = directed;
    m_graph = agopen(name.c_str(), desc, nullptr);
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
    agset(obj, "color", col);
}



