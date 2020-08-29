#include <cstdio>
#include <vector>
#include <nlohmann/json.hpp>
#include <assert.h>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <string>

using Json = nlohmann::json;

int n, m;
int max_value;
int win_width, win_height;
int base_width, base_height;
int margin_hor, margin_ver;
std::vector<std::vector<int>> a;

void InitDistance() {
	win_width = 1920; win_height = 1080;
	base_width = 1800; base_height = 900;
	margin_hor = (win_width - base_width) / 2;
	margin_ver = (win_height - base_height) / 2;
}

void Algorithm() {
	n = 200;
	m = 20;
	max_value = 100;
	a = std::vector<std::vector<int>>(m);
	for (int i = 0; i < m; i++) {
		a[i] = std::vector<int>(n);
		for (int j = 0; j < n; j++)
			a[i][j] = rand() % max_value + 1;
	}
}

void DrawLineFrame(Json &timeline, float time, std::string mix_mode, float x1, float y1, float x2, float y2) {
	Json f;
	f["ts"] = time;
	f["mix_mode"] = mix_mode;
	f["beg"] = Json::array({ x1,y1 });
	f["end"] = Json::array({ x2,y2 });
	timeline.push_back(f);
}

void DrawRectFrame(Json& timeline, float time, std::string mix_mode, float x1, float y1, float x2, float y2) {
	Json f;
	f["ts"] = time;
	f["mix_mode"] = mix_mode;
	f["pos"] = Json::array({ x1,y1 });
	f["siz"] = Json::array({ x2 - x1,y2 - y1 });
	timeline.push_back(f);
}

void DrawBottomLine(Json& ele_list) {
	Json ele;
	ele["type"] = "Line";
	ele["color"] = Json::array({ 230,230,230 });
	ele["width"] = 5.f;
	Json timeline;
	Json f;
	f.clear();
	f["ts"] = 0.f;
	f["mix_mode"] = "steep";
	f["beg"] = Json::array({ margin_hor, win_height - margin_ver });
	f["end"] = Json::array({ margin_hor + base_width, win_height - margin_ver });
	timeline.push_back(f);
	f.clear();
	f["ts"] = 10.f;
	f["mix_mode"] = "steep";
	f["beg"] = Json::array({ margin_hor, win_height - margin_ver });
	f["end"] = Json::array({ margin_hor + base_width, win_height - margin_ver });
	timeline.push_back(f);
	ele["frame"] = timeline;
	ele_list.push_back(ele);
}

void DrawStrip(Json& ele_list) {
	float width_ratio = 0.7f;
	float box_width = base_width / n;
	float str_width = box_width * width_ratio;
	for (int i = 0; i < n; i++) {
		Json ele;
		ele["type"] = "Rect";
		ele["color"] = Json::array({ 59,217,130 });
		ele["width"] = 1.f;
		ele["fill"] = true;
		float x = margin_hor + (i + 0.5f) * box_width;
		float y = win_height - margin_ver;
		Json timeline;
		DrawRectFrame(timeline, 0.f, "steep", x - str_width / 2, y, x + str_width / 2, y);
		for (int j = 0; j < m; j++)
			DrawRectFrame(timeline, (1.f + j) * 0.5f, "smoothstep", x - str_width / 2, y - base_height * (1.f * a[j][i] / max_value), x + str_width / 2, y);
		DrawRectFrame(timeline, 11.f, "smoothstep", x - str_width / 2, y, x + str_width / 2, y);
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}
}

void Draw() {
	Json data;
	data["virtual_width"] = 1920;
	data["virtual_height"] = 1080;
	data["duration"] = 15.f;
	Json ele_list;
	DrawBottomLine(ele_list);
	DrawStrip(ele_list);
	data["drawables"] = ele_list;
	std::ofstream out("output.json");
	out << data;
}

int main() {
	srand(time(NULL));
	InitDistance();
	Algorithm();
	Draw();
	return 0;
}
