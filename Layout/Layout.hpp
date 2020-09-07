#pragma once
#include <nlohmann/json.hpp>
#include <map>
#include <vector>
#include <glm/glm.hpp>
using Json = nlohmann::json;

using ObjectHandle = void *;
typedef struct GVC_s GVC_t;
typedef struct Agraph_s Agraph_t;
typedef struct Agnode_s Agnode_t;
typedef struct Agedge_s Agedge_t;
typedef struct obj_state_s obj_state_t;

enum class PrimitiveType {
    text, curve, polygon, polyline, ellipse
};

using PointCast = std::function<glm::vec2(const glm::vec2 &pos)>;

struct RGBA {
    unsigned char r, g, b, a;
};

class Primitive {
public:
    RGBA m_col;
    float m_width;
    std::string m_type;
protected:
    virtual Json init() const;
    Primitive(obj_state_t *style, const std::string &type);
    virtual void record(Json &frames, const PointCast &cast, float scale) const = 0;
public:
    mutable size_t jsonIdx;
    void record(Json &drawables, size_t idx, float ts, const PointCast &cast, float scale) const;
    virtual ~Primitive();
};

struct Frame final {
    glm::vec2 size;
    std::map < uint64_t, std::map<PrimitiveType, std::vector< std::shared_ptr<Primitive>>>> pris;
};

class LayoutContext final {
private:
    GVC_t *m_context;
public:
    explicit LayoutContext();
    Frame render(Agraph_t *g);
    void renderFrames(Json &drawables, const std::vector<std::pair<float, Frame>> &frames, float width, float height) const;
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

    Frame recordKeyFrame();
};
