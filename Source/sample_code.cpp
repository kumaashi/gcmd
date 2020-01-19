/*
 *
 * Copyright (c) 2020 gyabo <gyaboyan@gmail.com>
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <windows.h>
#include <map>
#include <vector>
#include <string>
#include <vector>
#include <algorithm>
#include <DirectXMath.h>

#include "oden_util.h"

struct matrix4x4 {
	float data[16];
};

struct vector4 {
	union {
		struct {
			float x, y, z, w;
		};
		float data[4];
	};
	void print()
	{
		printf("%.5f, %.5f, %.5f, %.5f\n", x, y, z, w);
	}
};

struct vector3 {
	float x, y, z;
	void print()
	{
		printf("%.5f, %.5f\n", x, y, z);
	}
};

struct vector2 {
	float x, y;
	void print()
	{
		printf("%.5f, %.5f\n", x, y);
	}
};

struct vertex_format {
	vector4 pos;
	vector3 nor;
	vector2 uv;
};

//Simple Matrix struct for windows.
struct MatrixStack {
	enum {
		Max = 32,
	};
	unsigned index = 0;
	DirectX::XMMATRIX data[Max];

	MatrixStack()
	{
		Reset();
	}

	auto & Get(int i)
	{
		return data[i];
	}

	auto & GetTop()
	{
		return Get(index);
	}

	void GetTop(float *a)
	{
		XMStoreFloat4x4((DirectX::XMFLOAT4X4 *) a, XMMatrixTranspose(GetTop()));
	}

	void Reset()
	{
		index = 0;
		for (int i = 0 ; i < Max; i++) {
			auto & m = Get(i);
			m = DirectX::XMMatrixIdentity();
		}
	}

	void Push()
	{
		auto & mb = GetTop();
		index = (index + 1) % Max;
		auto & m = GetTop();
		m = mb;
		printf("%s:index=%d\n", __FUNCTION__, index);
	}

	void Pop()
	{
		index = (index - 1) % Max;
		if (index < 0)
			printf("%s : under flow\n", __FUNCTION__);
		printf("%s:index=%d\n", __FUNCTION__, index);
	}

	void Load(DirectX::XMMATRIX a)
	{
		auto & m = GetTop();
		m = a;
	}

	void Load(float *a)
	{
		Load(* (DirectX::XMMATRIX *) a);
	}

	void Mult(DirectX::XMMATRIX a)
	{
		auto & m = GetTop();
		m *= a;
	}

	void Mult(float *a)
	{
		Mult(* (DirectX::XMMATRIX *) a);
	}

	void LoadIdentity()
	{
		Load(DirectX::XMMatrixIdentity());
	}

	void LoadLookAt(
		float px, float py, float pz,
		float ax, float ay, float az,
		float ux, float uy, float uz)
	{
		Load(DirectX::XMMatrixLookAtLH({px, py, pz}, {ax, ay, az}, {ux, uy, uz}));
	}

	void LoadPerspective(float ffov, float faspect, float fnear, float ffar)
	{
		Load(DirectX::XMMatrixPerspectiveFovLH(ffov, faspect, fnear, ffar));
	}

	void Translation(float x, float y, float z)
	{
		Mult(DirectX::XMMatrixTranslation(x, y, z));
	}

	void RotateAxis(float x, float y, float z, float angle)
	{
		Mult(DirectX::XMMatrixRotationAxis({x, y, z}, angle));
	}

	void RotateX(float angle)
	{
		Mult(DirectX::XMMatrixRotationX(angle));
	}

	void RotateY(float angle)
	{
		Mult(DirectX::XMMatrixRotationY(angle));
	}

	void RotateZ(float angle)
	{
		Mult(DirectX::XMMatrixRotationZ(angle));
	}

	void Scaling(float x, float y, float z)
	{
		Mult(DirectX::XMMatrixScaling(x, y, z));
	}

	void Transpose()
	{
		Load(DirectX::XMMatrixTranspose(GetTop()));
	}

	void Print(DirectX::XMMATRIX m)
	{
		int i = 0;
		for (auto & v : m.r) {
			for (auto & e : v.m128_f32) {
				if ((i % 4) == 0) printf("\n");
				printf("[%02d]%.4f, ", i++, e);
			}
		}
		printf("\n");
	}

	void Print()
	{
		Print(GetTop());
	}

	void PrintAll()
	{
		for (int i = 0; i < Max; i++) {
			Print(Get(i));
		}
	}
};

static LRESULT WINAPI
MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_SYSCOMMAND:
		switch ((wParam & 0xFFF0)) {
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			return 0;
		default:
			break;
		}
		break;
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_IME_SETCONTEXT:
		lParam &= ~ISC_SHOWUIALL;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

HWND
InitWindow(const char *name, int w, int h)
{
	HINSTANCE instance = GetModuleHandle(NULL);
	auto style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	auto ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

	RECT rc = {0, 0, w, h};
	WNDCLASSEX twc = {
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, instance,
		LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, IDC_ARROW),
		(HBRUSH) GetStockObject(BLACK_BRUSH), NULL, name, NULL
	};

	RegisterClassEx(&twc);
	AdjustWindowRectEx(&rc, style, FALSE, ex_style);
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	HWND hwnd = CreateWindowEx(
			ex_style, name, name, style,
			(GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2,
			(GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2,
			rc.right, rc.bottom, NULL, NULL, instance, NULL);
	ShowWindow(hwnd, SW_SHOW);
	SetFocus(hwnd);
	return hwnd;
};

int
Update()
{
	MSG msg;
	int is_active = 1;

	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
		if (msg.message == WM_QUIT) {
			is_active = 0;
			break;
		} else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return is_active;
}

void
GenerateMipmap(std::vector<cmd> & vcmd, std::string name, int w, int h)
{
	using namespace odenutil;
	//Generate Mipmap
	SetShader(vcmd, "genmipmap.hlsl", false, false, false);
	int miplevel = oden_get_mipmap_max(w, h);
	for (int i = 1; i < miplevel; i++) {
		SetTextureUav(vcmd, name, 0, 0, 0, i - 1, nullptr, 0);
		SetTextureUav(vcmd, name, 1, 0, 0, i - 0, nullptr, 0);
		Dispatch(vcmd, "mip" + name, (w >> i), (h >> i), 1);
	}
}

int main()
{
	using namespace odenutil;
	enum {
		Width = 1280,
		Height = 720,
		BloomWidth = Width >> 2,
		BloomHeight = Height >> 2,

		BufferMax = 2,
		ShaderSlotMax = 8,
		ResourceMax = 1024,

		TextureHeight = 256,
		TextureWidth = 256,
	};

	vertex_format vtx_rect[] = {
		{{-1, 1, 0, 1}, {0,  1,  1}, { 0, 1}},
		{{-1,-1, 0, 1}, {0,  1,  1}, { 0, 0}},
		{{ 1, 1, 0, 1}, {0,  1,  1}, { 1, 1}},
		{{ 1,-1, 0, 1}, {0,  1,  1}, { 1, 0}},
	};
	uint32_t idx_rect[] = {
		0, 1, 2,
		2, 1, 3
	};

	vertex_format vtx_cube[] = {
		{{-1, -1,  1, 1}, {0, 0, -1}, {-1, -1}},
		{{ 1, -1,  1, 1}, {0, 0, -1}, { 1, -1}},
		{{ 1,  1,  1, 1}, {0, 0, -1}, { 1,  1}},
		{{-1,  1,  1, 1}, {0, 0, -1}, {-1,  1}},
		{{-1, -1, -1, 1}, {0, 0,  1}, {-1, -1}},
		{{ 1, -1, -1, 1}, {0, 0,  1}, { 1, -1}},
		{{ 1,  1, -1, 1}, {0, 0,  1}, { 1,  1}},
		{{-1,  1, -1, 1}, {0, 0,  1}, {-1,  1}},
	};
	uint32_t idx_cube[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		3, 2, 6,
		6, 7, 3,
		// back
		7, 6, 5,
		5, 4, 7,
		// bottom
		4, 5, 1,
		1, 0, 4,
		// left
		4, 0, 3,
		3, 7, 4,
		// right
		1, 5, 6,
		6, 2, 1,
	};

	auto app_name = "oden_sample_code";
	auto hwnd = InitWindow(app_name, Width, Height);
	int index = 0;
	static std::vector<uint32_t> vtex;
	for (int y = 0  ; y < TextureHeight; y++) {
		for (int x = 0  ; x < TextureWidth; x++) {
			vtex.push_back((x ^ y) * 1110);
		}
	}

	struct constdata {
		vector4 time;
		vector4 color;
		matrix4x4 world;
		matrix4x4 proj;
		matrix4x4 view;
	};
	struct bloominfo {
		vector4 direction;
	};
	constdata cdata {};
	bloominfo binfoX {};
	bloominfo binfoY {};

	MatrixStack stack;
	std::vector<cmd> vcmd;

	auto tex_name = "testtex";
	uint64_t frame = 0;
	while (Update()) {
		auto buffer_index = frame % BufferMax;
		auto index_name = std::to_string(buffer_index);
		auto backbuffer_name = oden_get_backbuffer_name(buffer_index);
		auto offscreen_name = "offscreen" + index_name;
		auto constant_name = "constcommon" + index_name;
		auto constmip_name = "constmip" + index_name;
		auto constbloom_name = "constbloom" + index_name;
		auto bloomscreen_name = "bloomtexture" + index_name;
		auto bloomscreen_nameX = bloomscreen_name + index_name + "_X";

		bool is_update = false;

		if (GetAsyncKeyState(VK_F5) & 0x0001) {
			is_update = true;
		}

		cdata.time.data[0] = float (frame) / 1000.0f;
		cdata.time.data[1] = 0.0;
		cdata.time.data[2] = 1.0;
		cdata.time.data[3] = 1.0;

		cdata.color.data[0] = 1.0;
		cdata.color.data[1] = 0.0;
		cdata.color.data[2] = 1.0;
		cdata.color.data[3] = 1.0;

		//matrix : world
		stack.Reset();
		stack.Scaling(16, 16, 16);
		stack.GetTop(cdata.world.data);

		//matrix : view
		stack.Reset();
		float tm = float (frame) * 0.01;
		float rad = 64.0f;
		float height = 64.0f;
		stack.LoadLookAt(
			rad * cos(tm), height * sin(tm * 0.3), rad * sin(tm * 0.8),
			0, 0, 0,
			0, 1, 0);
		stack.GetTop(cdata.view.data);

		//matrix : proj
		stack.Reset();
		stack.LoadPerspective((3.141592653f / 180.0f) * 90.0f, float (Width) / float (Height), 0.5f, 1024.0f);
		stack.GetTop(cdata.proj.data);

		//Clear offscreenbuffer.
		float clear_color[] = {0, 1, 1, 1};
		SetRenderTarget(vcmd, offscreen_name, Width, Height);
		ClearRenderTarget(vcmd, offscreen_name, clear_color);
		ClearDepthRenderTarget(vcmd, offscreen_name, 1.0f);
		SetConstant(vcmd, constant_name, 0, &cdata, sizeof(cdata));
		SetShader(vcmd, "clear.hlsl", is_update, false, false);
		SetVertex(vcmd, "clear_vb", vtx_rect, sizeof(vtx_rect), sizeof(vertex_format));
		SetIndex(vcmd, "clear_ib", idx_rect, sizeof(idx_rect));
		DrawIndex(vcmd, "clear_draw", 0, _countof(idx_rect));

		//Draw Cube to offscreenbuffer.
		SetRenderTarget(vcmd, offscreen_name, Width, Height);
		ClearDepthRenderTarget(vcmd, offscreen_name, 1.0f);
		SetConstant(vcmd, constant_name, 0, &cdata, sizeof(cdata));
		SetTexture(vcmd, tex_name, 0, TextureWidth, TextureHeight, vtex.data(), vtex.size() * sizeof(uint32_t), 256 * sizeof(uint32_t));
		SetShader(vcmd, "model.hlsl", is_update, false, true);
		SetVertex(vcmd, "cube_vb", vtx_cube, sizeof(vtx_cube), sizeof(vertex_format));
		SetIndex(vcmd, "cube_ib", idx_cube, sizeof(idx_cube));
		DrawIndex(vcmd, "cube_draw", 0, _countof(idx_cube));
		GenerateMipmap(vcmd, offscreen_name, Width, Height);

		//Create Bloom X
		SetRenderTarget(vcmd, bloomscreen_nameX, BloomWidth, BloomHeight);
		SetTexture(vcmd, offscreen_name, 0);
		SetShader(vcmd, "bloom.hlsl", is_update, false, false);
		SetVertex(vcmd, "present_vb", vtx_rect, sizeof(vtx_rect), sizeof(vertex_format));
		SetIndex(vcmd, "present_ib", idx_rect, sizeof(idx_rect));
		binfoX.direction.x = BloomWidth;
		binfoX.direction.y = BloomHeight;
		binfoX.direction.z = 1.0;
		binfoX.direction.w = 0.0;
		SetConstant(vcmd, constbloom_name + "X", 0, &binfoX, sizeof(binfoX));
		DrawIndex(vcmd, "bloomX", 0, _countof(idx_rect));
		GenerateMipmap(vcmd, bloomscreen_nameX, BloomWidth, BloomHeight);

		//Create Bloom Y
		SetRenderTarget(vcmd, bloomscreen_name, BloomWidth, BloomHeight);
		SetTexture(vcmd, bloomscreen_nameX, 0);
		SetShader(vcmd, "bloom.hlsl", is_update, false, false);
		SetVertex(vcmd, "present_vb", vtx_rect, sizeof(vtx_rect), sizeof(vertex_format));
		SetIndex(vcmd, "present_ib", idx_rect, sizeof(idx_rect));
		binfoY.direction.x = BloomWidth;
		binfoY.direction.y = BloomHeight;
		binfoY.direction.z = 0.0;
		binfoY.direction.w = 1.0;
		SetConstant(vcmd, constbloom_name + "Y", 0, &binfoY, sizeof(binfoY));
		DrawIndex(vcmd, "bloomY", 0, _countof(idx_rect));
		GenerateMipmap(vcmd, bloomscreen_name, BloomWidth, BloomHeight);

		//Draw offscreen buffer to present buffer.
		SetRenderTarget(vcmd, backbuffer_name, Width, Height);
		ClearRenderTarget(vcmd, backbuffer_name, clear_color);
		ClearDepthRenderTarget(vcmd, backbuffer_name, 1.0f);
		SetTexture(vcmd, offscreen_name, 0);
		SetTexture(vcmd, bloomscreen_name, 1);
		SetShader(vcmd, "present.hlsl", is_update, false, false);
		SetVertex(vcmd, "present_vb", vtx_rect, sizeof(vtx_rect), sizeof(vertex_format));
		SetIndex(vcmd, "present_ib", idx_rect, sizeof(idx_rect));
		DrawIndex(vcmd, "present_draw", 0, _countof(idx_rect));

		//Present CMD to ODEN.
		SetBarrierToPresent(vcmd, backbuffer_name);
		oden_present_graphics(app_name, vcmd, hwnd, Width, Height, BufferMax, ResourceMax, ShaderSlotMax);

		vcmd.clear();
		frame++;
	}

	//Terminate Oden.
	oden_present_graphics(app_name, vcmd, nullptr, Width, Height, BufferMax, ResourceMax, ShaderSlotMax);
	return 0;
}

