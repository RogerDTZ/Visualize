#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <queue>

#define TIME_INF 1e8
#define SCALE_X (margin_hor + 20.f)
#define SCALE_Y0 (margin_ver + 30.f)
#define SCALE_Y1 (win_height - margin_ver - 30.f)
#define SCALE_STEP 5.f
#define SCALE_LENGTH 10.f
#define PTR_RECT_W 100.f
#define PTR_RECT_H 50.f
#define PTR_TEXT_SIZE 25.f
#define PTR_ARROW_LENGTH 20.f
#define FONT_SIZE 20.f

const int DX[4] = { -1,0,1,0 };
const int DY[4] = { 0,-1,0,1 };

using Json = nlohmann::json;

struct RGB {
	int r, g, b;
};

struct D_Rect {

	Json ele;
	Json timeline;

	D_Rect(RGB color, bool fill, float width, float layer) {
		ele.clear();
		ele["type"] = "Rect";
		ele["color"] = Json::array({ color.r, color.g, color.b });
		ele["fill"] = fill;
		ele["width"] = width;
		ele["layer"] = layer;
		timeline.clear();
	}

	void drawFrame(float time, std::string mix_mode, float x, float y, float w, float h, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["pos"] = Json::array({ x - w / 2,y - h / 2 });
		f["siz"] = Json::array({ w,h });
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		timeline.push_back(f);
	}

	void flush(Json &ele_list) {
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}

};

struct D_Line {

	Json ele;
	Json timeline;

	D_Line(RGB color, float width, float layer) {
		ele.clear();
		ele["type"] = "Line";
		ele["color"] = Json::array({ color.r, color.g, color.b });
		ele["width"] = width;
		ele["layer"] = layer;
		timeline.clear();
	}

	void drawFrame(float time, std::string mix_mode, float x1, float y1, float x2, float y2, float arrow_beg, float arrow_end, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["beg"] = Json::array({ x1,y1 });
		f["end"] = Json::array({ x2,y2 });
		f["beg_arrow"] = arrow_beg;
		f["end_arrow"] = arrow_end;
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		timeline.push_back(f);
	}

	void flush(Json &ele_list) {
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}

};

struct D_Circle {

	Json ele;
	Json timeline;

	D_Circle(RGB color, bool fill, float width, float layer) {
		ele.clear();
		ele["type"] = "Circle";
		ele["color"] = Json::array({ color.r, color.g, color.b });
		ele["fill"] = fill;
		ele["width"] = width;
		ele["layer"] = layer;
		timeline.clear();
	}

	void drawFrame(float time, std::string mix_mode, float x, float y, float r, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["center"] = Json::array({ x,y });
		f["radius"] = r;
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		timeline.push_back(f);
	}

	void flush(Json &ele_list) {
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}

};

struct D_Text {

	Json ele;
	Json timeline;

	D_Text(RGB color, float layer) {
		ele.clear();
		ele["type"] = "Text";
		ele["fill"] = true;
		ele["color"] = Json::array({ color.r, color.g, color.b });
		ele["layer"] = layer;
		timeline.clear();
	}

	void drawFrame(float time, std::string mix_mode, std::string msg, float x, float y, float size, RGB col = { -1,-1,-1 }) {
		Json f;
		f["ts"] = time;
		f["mix_mode"] = mix_mode;
		f["text"] = msg;
		f["center"] = Json::array({ x,y });
		f["size"] = size;
		if (col.r != -1)
			f["color"] = Json::array({ col.r,col.g,col.b });
		timeline.push_back(f);
	}

	void flush(Json &ele_list) {
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}

};

struct Arrow {

	float m_x, m_y, m_l;
	D_Line* line;
	D_Rect* rect;
	D_Text* text;

	Arrow(float x, float y, float len, float appear_time, std::string msg, float layer) :m_x(x), m_y(y), m_l(len){
		line = new D_Line({ 235,235,235 }, 5.f, layer);
		rect = new D_Rect({ 235,235,235 }, true, 10.f, layer);
		text = new D_Text({ 0,100,235 }, layer + 0.01f);
		move(appear_time, x, y, "steep", msg);
		move(TIME_INF, 0, 0, "steep", msg);
	}

	void move(float time, float x, float y, std::string mix_mode, std::string msg) {
		m_x = x; m_y = y;
		line->drawFrame(time, mix_mode, x, y, x + m_l, y, PTR_ARROW_LENGTH, 0);
		rect->drawFrame(time, mix_mode, x + m_l + PTR_RECT_W / 2, y, PTR_RECT_W, PTR_RECT_H);
		text->drawFrame(time, mix_mode, msg, x + m_l + PTR_RECT_W / 2, y + 10.f, PTR_TEXT_SIZE);
	}

	void flush(Json& ele_list) {
		line->flush(ele_list);
		rect->flush(ele_list);
		text->flush(ele_list);
	}

};


float win_width, win_height;
float base_width, base_height;
float margin_hor, margin_ver;
float grid_len;
int n;
int s_x, s_y, e_x, e_y;
int max_value;
std::vector<std::vector<int>> mat;
std::vector<std::vector<D_Rect*>> hlg;
std::vector<std::vector<std::pair<int, int>>> coo;
std::vector<std::vector<bool>> vis;
std::vector<std::vector<int>> from;

Json data;
Json ele_list;
float timer;

struct Route {

	float m_x1, m_y1, m_x2, m_y2;
	float r;
	bool active;
	D_Circle* cir;
	D_Line* line;

	Route(float x1, float y1, float x2, float y2, float layer) :m_x1(x1), m_y1(y1), m_x2(x2), m_y2(y2) {
		active = false;
		r = grid_len / 2 * 0.5f;
		cir = new D_Circle({ 252,248,61 }, true, r, layer);
		line = new D_Line({ 252,248,61 }, 8.f, layer);
	}

	void appear(float time, RGB col, float animation_time = 0.08f) {
		active = true;
		cir->drawFrame(time - animation_time, "steep", m_x1, m_y1, 0, col);
		cir->drawFrame(time, "smoothstep", m_x1, m_y1, r, col);
		line->drawFrame(time - animation_time, "steep", m_x1, m_y1, m_x1, m_y1, 0, 0, col);
		line->drawFrame(time, "smoothstep", m_x1, m_y1, m_x2, m_y2, 0, 0, col);
	}

	void disappear(float time) {
		if (!active)
			return;
		active = false;
		cir->drawFrame(time - 0.08f, "steep", m_x1, m_y1, r);
		cir->drawFrame(time, "smoothstep", m_x1, m_y1, 0);
		cir->drawFrame(time + 0.01f, "steep", -100.f, -100.f, 0);
		line->drawFrame(time - 0.05f, "steep", m_x1, m_y1, m_x2, m_y2, 0, 0);
		line->drawFrame(time, "smoothstep", m_x1, m_y1, m_x1, m_y1, 0, 0);
		line->drawFrame(time + 0.01f, "steep", -100.f, -100.f, -100.f, -100.f, 0, 0);
	}

	void flush(Json& ele_list) {
		cir->flush(ele_list);
		line->flush(ele_list);
	}

};

std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, Route*>>> rt;

void InitPara() {
	win_width = 1920.f; win_height = 1080.f;
	base_width = 1900.f; base_height = 1000.f;
	margin_hor = (win_width - base_width) / 2; margin_ver = (win_height - base_height) / 2;
	n = 20;
	max_value = 500;
	s_x = rand() % n; s_y = rand() % n;
	while (s_x == 0 || s_x == n - 1 || s_y == 0 || s_y == n - 1) {
		s_x = rand() % n; s_y = rand() % n;
	}
	e_x = s_x; e_y = s_y;
	while ((e_x == s_x && e_y == s_y) || (e_x == 0 || e_x == n - 1 || e_y == 0 || e_y == n - 1)) {
		e_x = rand() % n; e_y = rand() % n;
	}
}

void InitMatrix() {
	mat = std::vector<std::vector<int>>(n);
	hlg = std::vector<std::vector<D_Rect*>>(n);
	coo = std::vector<std::vector<std::pair<int, int>>>(n);
	vis = std::vector<std::vector<bool>>(n);
	from = std::vector<std::vector<int>>(n);
	for (int i = 0; i < n; i++) {
		mat[i] = std::vector<int>(n);
		hlg[i] = std::vector<D_Rect*>(n);
		coo[i] = std::vector<std::pair<int, int>>(n);
		vis[i] = std::vector<bool>(n);
		from[i] = std::vector<int>(n);
		for (int j = 0; j < n; j++) 
			mat[i][j] = rand() % max_value + 1;
	}
	mat[s_x][s_y] = mat[e_x][e_y] = max_value + 1;
}

void InitBasic() {
	/* Scale Line */
	D_Line* line = new D_Line({ 200,200,200 }, 4.f, 0);
	line->drawFrame(0.f, "smoothstep", SCALE_X, (SCALE_Y0 + SCALE_Y1) / 2, SCALE_X, (SCALE_Y0 + SCALE_Y1) / 2, 0.f, 0.f);
	line->drawFrame(0.4f, "smoothstep", SCALE_X, SCALE_Y0 - 10.f, SCALE_X, SCALE_Y1 + 10.f, 0.f, 0.f);
	line->drawFrame(0.5f, "smoothstep", SCALE_X, SCALE_Y0, SCALE_X, SCALE_Y1, 0.f, 0.f);
	line->drawFrame(TIME_INF, "smoothstep", SCALE_X, SCALE_Y0, SCALE_X, SCALE_Y1, 0.f, 0.f);
	line->flush(ele_list);
	delete line;
	for (float p = 0.f; p <= max_value; p += SCALE_STEP) {
		static int t = 0;
		line = new D_Line({ 200,200,200 }, 3.f, 0);
		float y = SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * SCALE_STEP * t;
		line->drawFrame(0.f, "smoothstep", SCALE_X, y, SCALE_X, y, 0, 0);
		line->drawFrame(0.4f, "smoothstep", SCALE_X, y, SCALE_X, y, 0, 0);
		line->drawFrame(0.6f, "smoothstep", SCALE_X, y, SCALE_X + SCALE_LENGTH, y, 0, 0);
		line->drawFrame(TIME_INF, "smoothstep", SCALE_X, y, SCALE_X + SCALE_LENGTH, y, 0, 0);
		line->flush(ele_list);
		delete line;
		t++;
	}

	/* Grid */
	D_Rect* r;
	D_Text* txt;
	float len = 0.95f * base_height;
	grid_len = len / n;
	float sx = win_width - margin_hor - len;
	float sy = (win_height - len) / 2;
	for(int i = 0; i < n; i++)
		for (int j = 0; j < n; j++) {
			float x = sx + j * grid_len;
			float y = sy + i * grid_len;
			float appear_time = 0.1f + (0.6f - 0.1f) / (n * n) * (i * n + j);
			coo[i][j] = std::make_pair(x + grid_len / 2, y + grid_len / 2);
			for (int k = 0; k < 4; k++) {
				int nx = i + DX[k], ny = j + DY[k];
				if (nx < 0 || nx >= n || ny < 0 || ny >= n)
					continue;
				rt[i][j][k] = new Route(x + grid_len / 2, y + grid_len / 2, x + grid_len / 2 + DY[k] * grid_len, y + grid_len / 2 + DX[k] * grid_len, 3.f);
			}
			if(i == s_x && j == s_y)
				r = new D_Rect({ 0,0,240 }, true, 8.f, 1.f - 0.0001f);
			else if(i==e_x&&j==e_y)
				r = new D_Rect({ 240,0,0 }, true, 8.f, 1.f - 0.0001f);
			else
				r = new D_Rect({ 240,240,240 }, false, 5.f, 1.f);
			r->drawFrame(0.f, "smoothstep", x + grid_len / 2, -100.f, grid_len, grid_len);
			r->drawFrame(appear_time, "smoothstep", x + grid_len / 2, y + grid_len / 2, grid_len, grid_len);
			r->drawFrame(TIME_INF, "smoothstep", x + grid_len / 2, y + grid_len / 2, grid_len, grid_len);
			r->flush(ele_list);
			hlg[i][j] = new D_Rect({ 52,177,71 }, true, 1.f, 1 - 0.0002f);
			txt = new D_Text({ 200,200,200 }, 1 - 0.0003f);
			txt->drawFrame(appear_time, "steep", std::to_string(mat[i][j]), x + grid_len / 2, y + grid_len / 2 + 10.f, FONT_SIZE);
			txt->drawFrame(TIME_INF, "steep", "", x + grid_len / 2, y + grid_len / 2 + 10.f, FONT_SIZE);
			txt->flush(ele_list);
			delete txt;
		}
	timer = 1.f;
}

bool Check(int thr) {
	/* Mark available square */
	timer += 0.2f;
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++) {
			vis[i][j] = false;
			from[i][j] = 0;
			if (mat[i][j] >= thr) {
				hlg[i][j]->drawFrame(timer, "steep", coo[i][j].first, coo[i][j].second, grid_len, grid_len);
			}
		}
	timer += 0.2f;
	/* Search */
	bool success = false;
	std::queue<std::pair<int, int>> q;
	std::vector<Route*> path;
	vis[s_x][s_y] = true;
	q.push(std::make_pair(s_x, s_y));
	while (!q.empty() && !success) {
		auto u = q.front();
		q.pop();
		int x = u.first, y = u.second;
		for (int k = 0; k < 4; k++) {
			int nx = x + DX[k], ny = y + DY[k];
			if (nx < 0 || nx >= n || ny < 0 || ny >= n || vis[nx][ny] || mat[nx][ny] < thr)
				continue;
			vis[nx][ny] = true;
			q.push(std::make_pair(nx, ny));
			timer += 0.002f;
			rt[x][y][k]->appear(timer, { -1,-1,-1 });
			from[nx][ny] = (k + 2) % 4;
			if (nx == e_x && ny == e_y) {
				timer -= 0.002f;
				for (int ux = e_x, uy = e_y; !(ux == s_x && uy == s_y);) {
					int d = from[ux][uy];
					int tux = ux + DX[d];
					int tuy = uy + DY[d];
					ux = tux;
					uy = tuy;
					d = (d + 2) % 4;
					Route* nr = new Route(rt[ux][uy][d]->m_x1, rt[ux][uy][d]->m_y1, rt[ux][uy][d]->m_x2, rt[ux][uy][d]->m_y2, 3.1f);
					nr->cir->ele["color"] = Json::array({ 255,0,0 });
					nr->cir->ele["layer"] = 3.1f;
					nr->line->ele["color"] = Json::array({ 255,0,0 });
					nr->line->ele["layer"] = 3.1f;
					nr->appear(timer, { 255,0,0 }, 0.02f);
					path.push_back(nr);
				}
				success = true;
			}
		}
	}
	timer += 0.5f;
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++)
			for (int k = 0; k < 4; k++) {
				if (rt[i][j][k] != nullptr && rt[i][j][k]->active)
					rt[i][j][k]->disappear(timer);
				if (mat[i][j] >= thr) 
					hlg[i][j]->drawFrame(timer, "steep", -100.f, -100.f, 0, 0);
			}
	for (auto& x : path) {
		x->disappear(timer);
		x->flush(ele_list);
		delete x;
	}
	return success;
}

void BinarySearchAnswer() {
	D_Text* title = new D_Text({ 240,240,240 }, 0);
	title->drawFrame(0, "steep", "Find a path that the minimum on it is maximized", 30, 30, 40.f);
	title->drawFrame(TIME_INF, "steep", "Find a path that the minimum on it is maximized", 30, 30, 40.f);
	title->flush(ele_list);
	int l = 1, r = max_value, mid;
	Arrow arrow_l(-300.f, SCALE_Y1, 100.f, 0.8f, std::to_string(1), 1.3f);
	Arrow arrow_r(-300.f, SCALE_Y0, 400.f, 0.8f, std::to_string(max_value), 1.1f);
	Arrow arrow_m(-300.f, (SCALE_Y0 + SCALE_Y1) / 2, 250.f, 0.8f, std::to_string((1 + max_value) / 2), 1.2f);
	while (l <= r) {
		mid = (l + r) >> 1;
		arrow_l.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * l, "smoothstep", "L: " + std::to_string(l));
		arrow_r.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * r, "smoothstep", "R: " + std::to_string(r));
		arrow_m.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * mid, "smoothstep", "MID: " + std::to_string(mid));
		int tl = l, tr = r, tm = mid;
		if (Check(mid))
			l = mid + 1;
		else
			r = mid - 1;
		arrow_l.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * tl, "smoothstep", "L: " + std::to_string(l));
		arrow_r.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * tr, "smoothstep", "R: " + std::to_string(r));
		arrow_m.move(timer, SCALE_X, SCALE_Y1 - (SCALE_Y1 - SCALE_Y0) / max_value * tm, "smoothstep", "MID: " + std::to_string(mid));
		timer += 0.2f;
	}
	arrow_l.flush(ele_list);
	arrow_r.flush(ele_list);
	arrow_m.flush(ele_list);
	for (int i = 0; i < n; i++)
		for (int j = 0; j < n; j++) {
			hlg[i][j]->flush(ele_list);
			for (int k = 0; k < 4; k++)
				if (rt[i][j][k] != nullptr)
					rt[i][j][k]->flush(ele_list);
		}
}

void Output() {
	data["virtual_width"] = win_width;
	data["virtual_height"] = win_height;
	data["drawables"] = ele_list;
	data["duration"] = timer + 5.f;
	std::ofstream out("output.json");
	out << data;
}

int main() {
	srand(time(NULL));
	InitPara();
	InitMatrix();
	InitBasic();
	BinarySearchAnswer();
	Output();
	return 0;
}
