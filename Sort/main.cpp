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
std::vector<int> val, who;
std::vector<std::vector<int>> pos;
float total_time;

void InitDistance() {
	win_width = 1920; win_height = 1080;
	base_width = 1800; base_height = 900;
	margin_hor = (win_width - base_width) / 2;
	margin_ver = (win_height - base_height) / 2;
}

void Record() {
	for (int i = 0; i < who.size(); i++)
		pos[who[i]].push_back(i);
}

void Algorithm() {
	/* Init data */
	n = 50;
	max_value = 100;
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
	f["ts"] = 1000.f;
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
	unsigned int cnt = pos[0].size();
	float frame_dur = 0.02f;
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
			} else {
				int p = pos[i][t];
				float x = margin_hor + (p + 0.5f) * box_width;
				float y = win_height - margin_ver;
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
	}
}

void Draw() {
	Json data;
	data["virtual_width"] = 1920;
	data["virtual_height"] = 1080;
	total_time = 0.f;
	Json ele_list;
	DrawBottomLine(ele_list);
	DrawStrip(ele_list);
	data["drawables"] = ele_list;
	data["duration"] = total_time;
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
