#include "memory.h"
#include <thread>

namespace offsets {
	constexpr auto dwLocalPlayer = 0xDA544C;
	constexpr auto m_fFlags = 0x104;
	constexpr auto dwForceJump = 0x5269570;
	constexpr auto dwEntityList = 0x4DBF75C;
	constexpr auto dwViewMatrix = 0x4DB1074;
	constexpr auto m_iTeamNum = 0xF4;
	constexpr auto m_iHealth = 0x100;
	constexpr auto m_vecOrigin = 0x138;
}

#define EnemyPen 0x000000FF
HBRUSH EnemyBrush = CreateSolidBrush(0x000000FF);

HDC hdc = GetDC(FindWindowA(NULL, "Counter-Strike: Global Offensive"));

int screenX = GetSystemMetrics(SM_CXSCREEN);	
int screenY = GetSystemMetrics(SM_CYSCREEN);


struct view_matrix_t {
	float* operator[ ](int index) {
		return matrix[index];
	}

	float matrix[4][4];
};

struct Vector3 {
	float x, y, z;
};

Vector3 WorldToScreen(const Vector3 pos, view_matrix_t matrix) {
	float _x = matrix[0][0] * pos.x + matrix[0][1] * pos.y + matrix[0][2] * pos.z + matrix[0][3];
	float _y = matrix[1][0] * pos.x + matrix[1][1] * pos.y + matrix[1][2] * pos.z + matrix[1][3];

	float w = matrix[3][0] * pos.x + matrix[3][1] * pos.y + matrix[3][2] * pos.z + matrix[3][3];

	float inv_w = 1.f / w;
	_x *= inv_w;
	_y *= inv_w;

	float x = screenX * .5f;
	float y = screenY * .5f;

	x += 0.5f * _x * screenX + 0.5f;
	y -= 0.5f * _y * screenY + 0.5f;

	return { x,y,w };
}

void DrawFilledRect(int x, int y, int w, int h) {
	RECT rect = { x, y, x + w, y + h };
	FillRect(hdc, &rect, EnemyBrush);
}

void DrawBorderBox(int x, int y, int w, int h, int thickness) {
	DrawFilledRect(x, y, w, thickness); //Top horiz line
	DrawFilledRect(x, y, thickness, h); //Left vertical line
	DrawFilledRect((x + w), y, thickness, h); //right vertical line
	DrawFilledRect(x, y + h, w + thickness, thickness); //bottom horiz line
}

void DrawLine(float startX, float startY, float endX, float endY) {
	int a, b = 0;
	HPEN hOpen;
	HPEN HNPen = CreatePen(PS_SOLID, 2, EnemyPen);
	hOpen = (HPEN)SelectObject(hdc, HNPen);
	MoveToEx(hdc, startX, startY, NULL);
	a = LineTo(hdc, endX, endY);
	DeleteObject(SelectObject(hdc, hOpen));
}

int main() {
	std::cout << screenX << std::endl;
	std::cout << screenY << std::endl;
	auto mem = Memory("csgo.exe");

	const auto client = mem.GetModuleAddress("client.dll");

	while (true) {
		view_matrix_t vm = mem.Read<view_matrix_t>(client + offsets::dwViewMatrix);
		int localteam = mem.Read<int>(mem.Read<DWORD>(client + offsets::dwEntityList) + offsets::m_iTeamNum);

		for (int i = 1; i < 64; i++) {
			uintptr_t pEnt = mem.Read<DWORD>(client + offsets::dwEntityList + (i * 0x10));

			int health = mem.Read<int>(pEnt + offsets::m_iHealth);
			int team = mem.Read<int>(pEnt + offsets::m_iTeamNum);

			Vector3 pos = mem.Read<Vector3>(pEnt + offsets::m_vecOrigin);
			Vector3 head;
			head.x = pos.x;
			head.y = pos.y;
			head.z = pos.z + 75.f;
			Vector3 screenpos = WorldToScreen(pos, vm);
			Vector3 screenhead = WorldToScreen(head, vm);
			float height = screenhead.y - screenpos.y;
			float width = height / 2.4f;
			if (screenpos.z >= 0.01f && team != localteam && health > 0 && health < 101) {
				DrawBorderBox(screenpos.x - (width / 2), screenpos.y, width, height, 1);
				DrawLine(screenX / 2, screenY, screenpos.x, screenpos.y);
			}
		}

		const auto dwLocalPlayer = mem.Read<uintptr_t>(client + offsets::dwLocalPlayer);

		if (dwLocalPlayer) {
			const auto onGround = mem.Read<bool>(dwLocalPlayer + offsets::m_fFlags);

			if (GetAsyncKeyState(VK_SPACE) && onGround & (1 << 0))
				mem.Write<BYTE>(client + offsets::dwForceJump, 6);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

}