#pragma once
#include <nlohmann/json.hpp>
using Json = nlohmann::json;

using ObjectHandle = void *;
struct GVC_t;
struct Agraph_t;
struct Agnode_t;
struct Agedge_t;

class LayoutContext final {
private:
    GVC_t *m_context;
public:
    explicit LayoutContext();
    GVC_t *context();
    ~LayoutContext();
};

class Graph final {
private:
    LayoutContext &m_context;
    Agraph_t *m_graph;
public:
    explicit Graph(LayoutContext &context, const std::string &name, bool directed);
    ~Graph();
    Agnode_t *allocNode();
    Agedge_t *linkEdge(Agnode_t *a, Agnode_t *b);
    void removeObject(ObjectHandle obj);

    void setColor(ObjectHandle obj, const char *col);

    Json recordKeyFrame() const;
};
