#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>

using Json = nlohmann::json;

enum OptType { add, que };

const float INF = 1e10;

float dur;
float frame_dur = 0.02f;
float end_time;

float win_width, win_height;
float base_width, base_height;
float margin_hor, margin_ver;
float row_height, fill_ratio;

int n, log_n;
int max_value;

struct RGB {
	int r, g, b;
};

struct Node {
	Node *fa, *lc, *rc;
	int dep;
	int l, r;
	int sum, tag;
	float x, y, w, h; // center
	Json body, body_timeline;
	Json bodyl, bodyl_timeline;
	Json edge, edge_timeline;
	Json edgel, edgel_timeline;
	Json text, text_timeline;
	Node() {
		fa = lc = rc = nullptr;
		dep = -1;
		l = r = 0;
		sum = tag = 0;
		x = y = w = h = 0.f;

		body["type"] = "Rect";
		body["fill"] = false;
		body["color"] = Json::array({ 235,235,235 });
		body["width"] = 4.f;
		body["layer"] = 2;
		bodyl["type"] = "Rect";
		bodyl["fill"] = false;
		bodyl["color"] = Json::array({ 252,245,36 });
		bodyl["width"] = 7.f;
		bodyl["layer"] = 2.5f;

		edge["type"] = "Line";
		edge["color"] = Json::array({ 150,150,150 });
		edge["width"] = 4.f;
		edge["layer"] = 0;
		edgel["type"] = "Line";
		edgel["color"] = Json::array({ 255,77,0 });
		edgel["width"] = 7.f;
		edgel["layer"] = 1;

		text["type"] = "Text";
		text["color"] = Json::array({ 0, 255,136 });
		text["layer"] = 3;
		drawTextFrame(INF, "steep", 0.f, 0.f);
	}

	void locate() {
		if (dep < log_n) {
			x = margin_hor + (l / (1 << (log_n - dep)) + .5f) * (base_width / (1 << dep));
			y = 100.f + margin_ver + (dep + .5f) * row_height;
			w = (base_width / (1 << dep)) * fill_ratio * fill_ratio;
			h = std::min(row_height * fill_ratio * fill_ratio, w);
		} else {
			x = margin_hor + (l / (1 << (log_n - dep)) + .5f) * (base_width / (1 << dep));
			y = 100.f + margin_ver + (dep + (l % 2 == 0 ? 0.7f : 0.f) + .5f) * row_height;
			w = h = 40.f;
		}
	}

	void appear() {
		dur += frame_dur;
		drawBodyFrame(dur - 0.2f, "smoothstep", x, y, 0, 0);
		drawBodyFrame(dur, "smoothstep", x, y, w * 1.1f, h * 1.1f);
		drawBodyFrame(dur + 0.1f, "smoothstep", x, y, w, h);
		drawBodyFrame(INF, "smoothstep", x, y, w, h);
		if (fa != nullptr) {
			drawEdgeFrame(dur - 0.2f, "smoothstep", fa->x, fa->y + fa->h / 2, fa->x, fa->y + fa->h / 2);
			drawEdgeFrame(dur + 0.1f, "smoothstep", x, y - h / 2, fa->x, fa->y + fa->h / 2);
			drawEdgeFrame(INF, "smoothstep", x, y - h / 2, fa->x, fa->y + fa->h / 2);
		}
	}

	void updateText(float tim) {
		drawTextFrame(tim, "steep", x, y);
	}


	void bodyHL(float beg, float end, RGB col = { -1,-1,-1 }) {
		drawBodyLFrame(beg, "steep", x, y, w, h, col);
		drawBodyLFrame(end, "steep", -1, -1, 0, 0, col);
	}
	
	void edgeHL(float beg, float end, int dir, RGB col = { -1,-1,-1 }) {
		if (dir == 0) { // pushup
			float arr = 0.f;
			drawEdgeLFrame(beg - 0.1f, "steep", x, y - h / 2, x, y - h / 2, 0.f, col);
			drawEdgeLFrame(beg, "smoothstep", x, y - h / 2, fa->x, fa->y + fa->h / 2, arr,col);
			drawEdgeLFrame(end, "smoothstep", x, y - h / 2, fa->x, fa->y + fa->h / 2, arr,col);
			drawEdgeLFrame(end + 0.1f, "smoothstep", fa->x, fa->y + fa->h / 2, fa->x, fa->y + fa->h / 2, 0.f,col);
		} else {
			float arr = 20.f;
			drawEdgeLFrame(beg - 0.1f, "steep", fa->x, fa->y + fa->h / 2, fa->x, fa->y + fa->h / 2, 0.f, col);
			drawEdgeLFrame(beg, "smoothstep", fa->x, fa->y + fa->h / 2, x, y - h / 2, arr, col);
			drawEdgeLFrame(end, "smoothstep", fa->x, fa->y + fa->h / 2, x, y - h / 2, arr, col);
			drawEdgeLFrame(end + 0.1f, "smoothstep", x, y - h / 2, x, y - h / 2, 0.f, col);
		}
	}

	void drawBodyFrame(float time, std::string mix_mode, float x, float y, float w, float h, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["pos"] = Json::array({ x - w / 2,y - h / 2 });
		f["siz"] = Json::array({ w,h });
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		body_timeline.push_back(f);
	}

	void drawBodyLFrame(float time, std::string mix_mode, float x, float y, float w, float h, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["pos"] = Json::array({ x - w / 2,y - h / 2 });
		f["siz"] = Json::array({ w,h });
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		bodyl_timeline.push_back(f);
	}

	void drawEdgeLFrame(float time, std::string mix_mode, float x1, float y1, float x2, float y2, float arrow, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["beg"] = Json::array({ x1,y1 });
		f["end"] = Json::array({ x2,y2 });
		f["end_arrow"] = arrow;
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		edgel_timeline.push_back(f);
	}

	void drawEdgeFrame(float time, std::string mix_mode, float x1, float y1, float x2, float y2, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["beg"] = Json::array({ x1,y1 });
		f["end"] = Json::array({ x2,y2 });
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		edge_timeline.push_back(f);
	}

	void drawTextFrame(float time, std::string mix_mode, float x, float y, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["text"] = Json::array({ std::to_string(sum), std::to_string(tag) });
		f["center"] = Json::array({ x,y + 10.f });
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		f["size"] = 20.f;
		text_timeline.push_back(f);
	}

	void addElement(Json &data) {
		body["frame"] = body_timeline;
		bodyl["frame"] = bodyl_timeline;
		edge["frame"] = edge_timeline;
		edgel["frame"] = edgel_timeline;
		text["frame"] = text_timeline;
		data.push_back(body);
		data.push_back(bodyl);
		data.push_back(edge);
		data.push_back(edgel);
		data.push_back(text);
	}

};

struct Operation {
	int id;
	OptType type;
	std::vector<int> para;
};

std::vector<int> arr;
std::vector<Node*> node;
int m;
std::vector<Operation> opr;

void InitPara() {
	n = 64; log_n = 6;
	max_value = 100;
	win_width = 1920.f; win_height = 1080.f;
	base_width = 1800.f; base_height = 900.f;
	margin_hor = (win_width - base_width) / 2;
	margin_ver = (win_height - base_height) / 2;
	row_height = 100.f;
	fill_ratio = 0.90f;
	dur = 1.f;
}

std::pair<int, int> RandSeg(int n) {
	int l = rand() % n;
	int r = l + rand() % (n - l);
	return std::make_pair(l, r);
}

void Pushup(Node* u) {
	u->sum = u->lc->sum + u->rc->sum;
	dur += frame_dur;
	u->lc->edgeHL(dur, end_time > 0 ? end_time : dur + .2f, 0);
	u->rc->edgeHL(dur, end_time > 0 ? end_time : dur + .2f, 0);
	u->updateText(dur);
}

void Pushdown(Node* u) {
	dur += frame_dur;
	u->lc->sum += u->tag * (u->lc->r - u->lc->l + 1);
	u->rc->sum += u->tag * (u->rc->r - u->rc->l + 1);
	u->lc->tag += u->tag;
	u->rc->tag += u->tag;
	u->tag = 0;
}

void Build(Node* u) {
	node.push_back(u);
	u->locate();
	u->appear();
	if (u->l == u->r) {
		u->sum = arr[u->l];
		dur += frame_dur;
		u->updateText(dur);
		return;
	}
	int l=u->l, r=u->r, mid = (u->l + u->r) >> 1;
	u->lc = new Node();
	u->lc->fa = u; u->lc->l = l; u->lc->r = mid;
	u->lc->dep = u->dep + 1;
	u->rc = new Node();
	u->rc->fa = u; u->rc->l = mid+1; u->rc->r = r;
	u->rc->dep = u->dep + 1;
	Build(u->lc);
	Build(u->rc);
	Pushup(u);
}

void SegAdd(Node* u, int l, int r, int x) {
	if (l <= u->l && u->r <= r) {
		u->bodyHL(dur, end_time, { 255,166,0 });
		u->sum += (u->r - u->l + 1) * x;
		u->tag += x;
		u->updateText(dur);
		return;
	}
	Pushdown(u);
	int mid = (u->l + u->r) >> 1;
	int res = 0;
	if (l <= mid)
		SegAdd(u->lc, l, mid, x);
	if (mid < r)
		SegAdd(u->rc, mid + 1, r, x);
	Pushup(u);
}

int SegQue(Node* u, int l, int r) {
	if (u->fa != nullptr)
		u->edgeHL(dur, end_time, 1, { 252,245,36 });
	if (l <= u->l && u->r <= r) {
		u->bodyHL(dur, end_time);
		return u->sum;
	}
	Pushdown(u);
	int mid = (u->l + u->r) >> 1;
	int res = 0;
	if (l <= mid)
		res += SegQue(u->lc, l, mid);
	if (mid < r)
		res += SegQue(u->rc, mid+1, r);
	return res;
}

Json Run() {
	/* Generate array */
	arr = std::vector<int>(n);
	for (auto& x : arr)
		x = rand() % max_value;
	/* Generate operations */
	m = 50;
	for (int i = 0; i < m; i++) {
		Operation op;
		op.id = i;
		switch (rand() % 2) {
		case 0: {
			op.type = add;
			auto [l, r] = RandSeg(n);
			op.para.push_back(l);
			op.para.push_back(r);
			op.para.push_back(rand() % max_value);
			break;
		}
		case 1: {
			op.type = que;
			auto [l, r] = RandSeg(n);
			op.para.push_back(l);
			op.para.push_back(r);
			break;
		}
		}
		opr.push_back(op);
	}
	/* Build segment tree */
	Node* root = new Node();
	root->l = 0; root->r = n - 1;
	root->dep = 0;
	Build(root);

	/* Process operations*/
	Json res;
	dur += 1.f;
	frame_dur = .05f;
	for (auto& q : opr) {
		switch (q.type) {
		case add: {
			Json t, t_timeline;
			t["type"] = "Text";
			t["color"] = Json::array({ 255,255,255 });
			t["layer"] = 0;
			Json f;
			f["ts"] = dur;
			f["mix_mode"] = "steep";
			f["size"] = 50.f;
			f["text"] = "Segment [" + std::to_string(q.para[0]) + ", " + std::to_string(q.para[1]) + "] adding " + std::to_string(q.para[2]);
			f["center"] = Json::array({ win_width / 2, 50.f });
			t_timeline.push_back(f);
			f.clear();
			f["ts"] = dur + 1.f;
			f["mix_mode"] = "steep";
			f["size"] = 50.f;
			f["text"] = "";
			f["center"] = Json::array({ win_width / 2, 50.f });
			t_timeline.push_back(f);
			t["frame"] = t_timeline;
			res.push_back(t);

			end_time = dur + 1.f;
			SegAdd(root, q.para[0], q.para[1], q.para[2]);
			dur = end_time;

			break;
		}
		case que: {
			float tmpd = dur;
			end_time = dur + 1.f;
			int ans = SegQue(root, q.para[0], q.para[1]);
			dur = end_time;

			Json t, t_timeline;
			t["type"] = "Text";
			t["color"] = Json::array({ 255,255,255 });
			t["layer"] = 0;
			Json f;
			f["ts"] = tmpd;
			f["mix_mode"] = "steep";
			f["size"] = 50.f;
			f["text"] = "Querying sum of  [" + std::to_string(q.para[0]) + ", " + std::to_string(q.para[1]) + "] = " + std::to_string(ans);
			f["center"] = Json::array({ win_width / 2, 50.f });
			t_timeline.push_back(f);
			f.clear();
			f["ts"] = tmpd + 1.f;
			f["mix_mode"] = "steep";
			f["size"] = 50.f;
			f["text"] = "";
			f["center"] = Json::array({ win_width / 2, 50.f });
			t_timeline.push_back(f);
			t["frame"] = t_timeline;
			res.push_back(t);

			break;
		}
		}
		dur += 0.2f;
	}

	for (auto& u : node)
		u->addElement(res);
	return res;
}

int main() {
	srand(time(NULL));
	InitPara();
	Json data;	
	data["virtual_width"] = win_width;
	data["virtual_height"] = win_height;
	data["drawables"] = Run();
	data["duration"] = dur + 3.f;
    std::ofstream out("output.json");
    out << data;
	return 0;
}
