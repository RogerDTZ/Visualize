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
float width_ratio;
float box_width;
float str_width;
std::vector<int> val, who;
std::vector<std::vector<int>> pos;
std::vector < std::pair<std::pair<int, int>, std::pair<int, int>>> arrow;
float frame_dur;
float total_time;
int tot;

void InitPara() {
	n = 80;
	max_value = 100;
	win_width = 1920; win_height = 1080;
	base_width = 1800; base_height = 900;
	margin_hor = (win_width - base_width) / 2;
	margin_ver = (win_height - base_height) / 2;
	width_ratio = 0.7f;
	box_width = base_width / n;
	str_width = box_width * width_ratio;
	frame_dur = 0.05f;
}

void Record() {
	for (int i = 0; i < who.size(); i++)
		pos[who[i]].push_back(i);
}

void Algorithm() {
	/* Init data */
	val = std::vector<int>(n);
	pos = std::vector<std::vector<int>>(n);
	who = std::vector<int>(n);
	for (int i = 0; i < n; i++) {
		val[i] = rand() % max_value + 1;
		pos[i].push_back(i);
		who[i] = i;
	}
	std::vector<int> a = val;
	/* Bubble sort */
	for (int i = 0; i < n; i++) 
		for (int j = 0; j < n - 1; j++) {
			if (a[j] > a[j + 1]) {
				std::swap(who[j], who[j + 1]);
				std::swap(a[j], a[j + 1]);
				Record();
				arrow.push_back(std::make_pair(std::make_pair(pos[0].size() - 1, j), std::make_pair(a[j + 1], a[j])));
			}
		}
	Record();
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
	tot++;
	Json f;
	f["ts"] = time;
	f["mix_mode"] = mix_mode;
	f["pos"] = Json::array({ x1,y1 });
	f["siz"] = Json::array({ x2 - x1,y2 - y1 });
	timeline.push_back(f);
}

void DrawArrowFrame(Json& timeline, float time, std::string mix_mode, float x1, float y1, float x2, float y2, float x3, float y3) {
	tot++;
	Json f;
	f["ts"] = time;
	f["mix_mode"] = mix_mode;
	f["beg"] = Json::array({ x1, y1 });
	f["ctrl"] = Json::array({ x2, y2 });
	f["end"] = Json::array({ x3, y3 });
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
	f["ts"] = 1000.f;
	f["mix_mode"] = "steep";
	f["beg"] = Json::array({ margin_hor, win_height - margin_ver });
	f["end"] = Json::array({ margin_hor + base_width, win_height - margin_ver });
	timeline.push_back(f);
	ele["frame"] = timeline;
	ele_list.push_back(ele);
}

void DrawStrip(Json& ele_list) {
	unsigned int cnt = pos[0].size();
	total_time = cnt * frame_dur + 1.f;
	std::vector<Json> timeline(n);
	std::vector<Json> ele(n);
	std::vector<int> last(n, -1);
	for (unsigned int t = 0; t < cnt; t++) {
		for (int i = 0; i < n; i++) {
			if (t > 0 && pos[i][t - 1] != pos[i][t]) {
				int p = pos[i][t - 1];
				float x, y;
				x = margin_hor + (p + 0.5f) * box_width;
				y = win_height - margin_ver;
				DrawRectFrame(timeline[i], (t - 1) * frame_dur, "smoothstep", x - str_width / 2, y - base_height * (1.f * val[i] / max_value), x + str_width / 2, y);
				p = pos[i][t];
				x = margin_hor + (p + 0.5f) * box_width;
				y = win_height - margin_ver;
				DrawRectFrame(timeline[i], t * frame_dur - 0.000001f, "smoothstep", x - str_width / 2, y - base_height * (1.f * val[i] / max_value), x + str_width / 2, y);
			} else if (t == 0) {
				int p = pos[i][t];
				float x = margin_hor + (p + 0.5f) * box_width;
				float y = win_height - margin_ver;
	DrawBottomLine(ele_list);
				DrawRectFrame(timeline[i], t * frame_dur, "smoothstep", x - str_width / 2, y - base_height * (1.f * val[i] / max_value), x + str_width / 2, y);
			}
		}
	}
	for (int i = 0; i < n; i++) {
		int p = pos[i][cnt - 1];
		float x = margin_hor + (p + 0.5f) * box_width;
		float y = win_height - margin_ver;
		DrawRectFrame(timeline[i], cnt * frame_dur + 1.f, "smoothstep", x - str_width / 2, y - base_height * (1.f * val[i] / max_value), x + str_width / 2, y);
	}
	for (int i = 0; i < n; i++) {
		ele[i]["type"] = "Rect";
		ele[i]["color"] = Json::array({ 59,217,130 });
		ele[i]["width"] = 1.f;
		ele[i]["fill"] = true;
		ele[i]["frame"] = timeline[i];
		ele_list.push_back(ele[i]);
	DrawBottomLine(ele_list);
	}
}

void DrawArrow(Json& ele_list) {
	for (auto& a : arrow) {
		int t = a.first.first;
		int p = a.first.second;
		int v1 = a.second.first;
		int v2 = a.second.second;
		float x1, y1, x2, y2;
		x1 = margin_hor + (p + 0.5f) * box_width;
		y1 = win_height - margin_ver - (1.f * v1 / max_value * base_height) - 10.f;
		x2 = margin_hor + (p + 1.5f) * box_width;
		y2 = win_height - margin_ver - (1.f * v2 / max_value * base_height) - 10.f;
		Json timeline;
		DrawArrowFrame(timeline, (t - 1) * frame_dur, "smoothstep", x1, y1, (x1 + x2) / 2, -100, x2, y2);
		DrawArrowFrame(timeline, t * frame_dur, "smoothstep", x2, y1, (x1 + x2) / 2, -100, x1, y2);
		Json ele;
		ele["type"] = "Curve";
		ele["color"] = Json::array({ 252,107,23 });
		ele["width"] = 5.f;
		ele["frame"] = timeline;
		ele_list.push_back(ele);
	}
}

void Draw() {
	Json data;
	data["virtual_width"] = 1920;
	data["virtual_height"] = 1080;
	total_time = 0.f;
	Json ele_list;
	DrawStrip(ele_list);
	DrawBottomLine(ele_list);
	DrawArrow(ele_list);
	data["drawables"] = ele_list;
	data["duration"] = total_time;
	std::ofstream out("output.json");
	out << data;
}

int main() {
	srand(time(NULL));
	InitPara();
	Algorithm();
	Draw();
	return 0;
}
