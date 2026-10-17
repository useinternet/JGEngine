// boldlab — ER-019 검토용 시험 프로그램(스크래치, 엔진 코드 아님).
// 합성 굵게 방법 비교: 지금(엔진 GameUIFont.cpp 의 dilateCoverage 그대로) · 여러 배로 그려 넓힌 뒤 줄이기 · 윤곽선 넓히기 · 가로 정수 픽셀.
// 출력: 크기 · 방법별 측정(표준 출력) + 비교 그림 PNG(4배 확대, 흰 바탕 검은 글).
#define STB_TRUETYPE_IMPLEMENTATION
#include "C:/Develop/JGEngine/Source/ThirdParty/stb/stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "C:/Develop/JGEngine/Source/ThirdParty/stb/stb_image_write.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

typedef unsigned char u8;

struct Glyph
{
	int   w = 0, h = 0, ox = 0, oy = 0;   // 비트맵 크기, 펜(기준선) 기준 왼쪽 위 자리(픽셀, y 아래로 +)
	float advance = 0.0f;
	std::vector<u8> a;                    // 커버리지 0~255
};

enum Method { Regular, Current, Supersample, Outline, HorizontalInt, MethodCount };
const char* MethodNames[MethodCount] = { "regular", "current", "supersample", "outline", "horizontal" };

// ---- 지금 엔진 방식(GameUIFont.cpp boldPadding · dilateLine · dilateCoverage 그대로)
int boldPadding(float radius)
{
	return (int)std::ceil(radius - 0.0001f);
}

void dilateLine(const u8* source, int count, int sourceStride, u8* out, int outStride, float radius, int padding)
{
	const int   full     = (int)radius;
	const float fraction = radius - (float)full;
	const int   outCount = count + padding * 2;
	for (int i = 0; i < outCount; ++i)
	{
		const int center = i - padding;
		float value = 0.0f;
		for (int s = center - full; s <= center + full; ++s)
		{
			if (s >= 0 && s < count)
			{
				value = std::max(value, (float)source[s * sourceStride]);
			}
		}
		if (fraction > 0.0f)
		{
			const int ring[2] = { center - full - 1, center + full + 1 };
			for (int s : ring)
			{
				if (s >= 0 && s < count)
				{
					value = std::max(value, (float)source[s * sourceStride] * fraction);
				}
			}
		}
		out[i * outStride] = (u8)(value + 0.5f);
	}
}

void dilateCoverage(const std::vector<u8>& source, int width, int height, float radius, std::vector<u8>& outCoverage)
{
	const int padding   = boldPadding(radius);
	const int outWidth  = width + padding * 2;
	const int outHeight = height + padding * 2;
	std::vector<u8> horizontal((size_t)outWidth * height, 0);
	for (int y = 0; y < height; ++y)
	{
		dilateLine(&source[(size_t)y * width], width, 1, &horizontal[(size_t)y * outWidth], 1, radius, padding);
	}
	outCoverage.assign((size_t)outWidth * outHeight, 0);
	for (int x = 0; x < outWidth; ++x)
	{
		dilateLine(&horizontal[(size_t)x], height, outWidth, &outCoverage[(size_t)x], outWidth, radius, padding);
	}
}

// ---- 공통
float scaleFor(const stbtt_fontinfo& font, int px)
{
	return stbtt_ScaleForMappingEmToPixels(&font, (float)px);
}

Glyph rasterRegular(const stbtt_fontinfo& font, int cp, int px)
{
	Glyph g;
	const float scale = scaleFor(font, px);
	int adv = 0, lsb = 0;
	stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
	g.advance = adv * scale;
	int x0, y0, x1, y1;
	stbtt_GetCodepointBitmapBox(&font, cp, scale, scale, &x0, &y0, &x1, &y1);
	g.w = x1 - x0; g.h = y1 - y0; g.ox = x0; g.oy = y0;
	if (g.w > 0 && g.h > 0)
	{
		g.a.resize((size_t)g.w * g.h);
		stbtt_MakeCodepointBitmap(&font, g.a.data(), g.w, g.h, g.w, scale, scale, cp);
	}
	return g;
}

Glyph rasterCurrent(const stbtt_fontinfo& font, int cp, int px, float strength)
{
	Glyph g = rasterRegular(font, cp, px);
	const float radius  = (float)px * strength * 0.5f;
	const int   padding = boldPadding(radius);
	g.advance += (float)px * strength;
	if (g.w > 0 && padding > 0)
	{
		std::vector<u8> out;
		dilateCoverage(g.a, g.w, g.h, radius, out);
		g.a.swap(out);
		g.w += padding * 2; g.h += padding * 2; g.ox -= padding; g.oy -= padding;
	}
	return g;
}

int floorDiv(int a, int b)
{
	return (a >= 0) ? a / b : -((-a + b - 1) / b);
}

int ceilDiv(int a, int b)
{
	return -floorDiv(-a, b);
}

// 여러 배(S)로 그려 둥근 붓(반지름 r×S 부분 픽셀)으로 넓힌 뒤 S×S 평균으로 줄인다. 픽셀 격자는 1배 래스터와 같다.
Glyph rasterSupersample(const stbtt_fontinfo& font, int cp, int px, float strength, int S)
{
	Glyph g;
	const float scale = scaleFor(font, px);
	int adv = 0, lsb = 0;
	stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
	g.advance = adv * scale + (float)px * strength;
	const float radius = (float)px * strength * 0.5f;
	const int   k      = (int)std::lround(radius * (float)S);
	const float sscale = scale * (float)S;
	int X0, Y0, X1, Y1;
	stbtt_GetCodepointBitmapBox(&font, cp, sscale, sscale, &X0, &Y0, &X1, &Y1);
	const int W = X1 - X0, H = Y1 - Y0;
	if (W <= 0 || H <= 0)
	{
		return g;
	}
	std::vector<u8> hi((size_t)W * H);
	stbtt_MakeCodepointBitmap(&font, hi.data(), W, H, W, sscale, sscale, cp);

	const int px0 = floorDiv(X0 - k, S), py0 = floorDiv(Y0 - k, S);
	const int px1 = ceilDiv(X1 + k, S), py1 = ceilDiv(Y1 + k, S);
	const int CW = (px1 - px0) * S, CH = (py1 - py0) * S;
	std::vector<u8> canvas((size_t)CW * CH, 0);
	const int offX = X0 - px0 * S, offY = Y0 - py0 * S;
	for (int y = 0; y < H; ++y)
	{
		for (int x = 0; x < W; ++x)
		{
			canvas[(size_t)(y + offY) * CW + x + offX] = hi[(size_t)y * W + x];
		}
	}
	std::vector<u8> grown = canvas;
	if (k > 0)
	{
		for (int y = 0; y < CH; ++y)
		{
			for (int x = 0; x < CW; ++x)
			{
				u8 best = 0;
				for (int dy = -k; dy <= k; ++dy)
				{
					const int sy = y + dy;
					if (sy < 0 || sy >= CH)
					{
						continue;
					}
					for (int dx = -k; dx <= k; ++dx)
					{
						const int sx = x + dx;
						if (sx < 0 || sx >= CW || dx * dx + dy * dy > k * k)
						{
							continue;
						}
						best = std::max(best, canvas[(size_t)sy * CW + sx]);
					}
				}
				grown[(size_t)y * CW + x] = best;
			}
		}
	}
	g.w = px1 - px0; g.h = py1 - py0; g.ox = px0; g.oy = py0;
	g.a.assign((size_t)g.w * g.h, 0);
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			int sum = 0;
			for (int sy = 0; sy < S; ++sy)
			{
				for (int sx = 0; sx < S; ++sx)
				{
					sum += grown[(size_t)(y * S + sy) * CW + x * S + sx];
				}
			}
			g.a[(size_t)y * g.w + x] = (u8)((sum + S * S / 2) / (S * S));
		}
	}
	return g;
}

// 윤곽선 넓히기(FreeType FT_Outline_EmboldenXY 와 같은 생각): 윤곽의 점마다 양옆 변의 바깥 법선 이등분선으로 r 만큼 옮긴 뒤 그린다.
Glyph rasterOutline(const stbtt_fontinfo& font, int cp, int px, float strength)
{
	Glyph g;
	const float scale = scaleFor(font, px);
	int adv = 0, lsb = 0;
	stbtt_GetCodepointHMetrics(&font, cp, &adv, &lsb);
	g.advance = adv * scale + (float)px * strength;
	const float radius = (float)px * strength * 0.5f;
	const float ru     = radius / scale;   // 글꼴 단위

	stbtt_vertex* vertices = nullptr;
	const int count = stbtt_GetCodepointShape(&font, cp, &vertices);
	if (count <= 0)
	{
		return g;
	}

	// 점 목록: 윤곽마다 [시작, (조절점), 끝, ...]. 각 점이 어느 정점의 어느 자리(끝 · 조절 1 · 조절 2)인지 기억한다.
	struct Ref { int vertex; int slot; };
	struct Contour { std::vector<float> x, y; std::vector<Ref> refs; };
	std::vector<Contour> contours;
	for (int i = 0; i < count; ++i)
	{
		const stbtt_vertex& v = vertices[i];
		if (v.type == STBTT_vmove)
		{
			contours.emplace_back();
		}
		Contour& c = contours.back();
		if (v.type == STBTT_vcurve)
		{
			c.x.push_back(v.cx); c.y.push_back(v.cy); c.refs.push_back({ i, 1 });
		}
		else if (v.type == STBTT_vcubic)
		{
			c.x.push_back(v.cx); c.y.push_back(v.cy); c.refs.push_back({ i, 1 });
			c.x.push_back(v.cx1); c.y.push_back(v.cy1); c.refs.push_back({ i, 2 });
		}
		c.x.push_back(v.x); c.y.push_back(v.y); c.refs.push_back({ i, 0 });
	}

	// 전체 방향(신발끈 면적): 음수면 트루타입식(바깥 시계 방향, 잉크가 진행 방향 오른쪽) → 바깥 = 왼쪽 법선
	double area = 0.0;
	for (const Contour& c : contours)
	{
		const size_t n = c.x.size();
		for (size_t i = 0; i < n; ++i)
		{
			const size_t j = (i + 1) % n;
			area += (double)c.x[i] * c.y[j] - (double)c.x[j] * c.y[i];
		}
	}
	const float side = (area < 0.0) ? 1.0f : -1.0f;

	std::vector<stbtt_vertex> moved(vertices, vertices + count);
	for (const Contour& c : contours)
	{
		const int n = (int)c.x.size();
		for (int i = 0; i < n; ++i)
		{
			// 같은 자리 점(닫는 점 등)을 건너뛰고 이웃을 찾는다
			int prev = i, next = i;
			for (int step = 1; step < n; ++step)
			{
				prev = (i - step + n) % n;
				if (c.x[prev] != c.x[i] || c.y[prev] != c.y[i])
				{
					break;
				}
			}
			for (int step = 1; step < n; ++step)
			{
				next = (i + step) % n;
				if (c.x[next] != c.x[i] || c.y[next] != c.y[i])
				{
					break;
				}
			}
			float inX = c.x[i] - c.x[prev], inY = c.y[i] - c.y[prev];
			float outX = c.x[next] - c.x[i], outY = c.y[next] - c.y[i];
			const float inLen = std::sqrt(inX * inX + inY * inY), outLen = std::sqrt(outX * outX + outY * outY);
			if (inLen <= 0.0f || outLen <= 0.0f)
			{
				continue;
			}
			inX /= inLen; inY /= inLen; outX /= outLen; outY /= outLen;
			const float nInX = -inY * side, nInY = inX * side;
			const float nOutX = -outY * side, nOutY = outX * side;
			float d = 1.0f + nInX * nOutX + nInY * nOutY;
			d = std::max(d, 0.25f);   // 아주 뾰족한 모서리는 너무 멀리 나가지 않게
			const float shiftX = (nInX + nOutX) * ru / d;
			const float shiftY = (nInY + nOutY) * ru / d;
			stbtt_vertex& target = moved[c.refs[i].vertex];
			if (c.refs[i].slot == 0)
			{
				target.x = (stbtt_vertex_type)std::lround(c.x[i] + shiftX); target.y = (stbtt_vertex_type)std::lround(c.y[i] + shiftY);
			}
			else if (c.refs[i].slot == 1)
			{
				target.cx = (stbtt_vertex_type)std::lround(c.x[i] + shiftX); target.cy = (stbtt_vertex_type)std::lround(c.y[i] + shiftY);
			}
			else
			{
				target.cx1 = (stbtt_vertex_type)std::lround(c.x[i] + shiftX); target.cy1 = (stbtt_vertex_type)std::lround(c.y[i] + shiftY);
			}
		}
	}

	int x0, y0, x1, y1;
	stbtt_GetCodepointBitmapBox(&font, cp, scale, scale, &x0, &y0, &x1, &y1);
	const int pad = (int)std::ceil(radius) + 1;
	x0 -= pad; y0 -= pad; x1 += pad; y1 += pad;
	g.w = x1 - x0; g.h = y1 - y0; g.ox = x0; g.oy = y0;
	g.a.assign((size_t)g.w * g.h, 0);
	stbtt__bitmap bitmap;
	bitmap.w = g.w; bitmap.h = g.h; bitmap.stride = g.w; bitmap.pixels = g.a.data();
	stbtt_Rasterize(&bitmap, 0.35f, moved.data(), count, scale, scale, 0.0f, 0.0f, x0, y0, 1, nullptr);
	stbtt_FreeShape(&font, vertices);
	return g;
}

// 가로로만 정수 n 픽셀 넓히기(오른쪽으로 n 번 겹쳐 찍기와 같음). n = 반올림(em × 세기), 최소 1.
Glyph rasterHorizontal(const stbtt_fontinfo& font, int cp, int px, float strength)
{
	Glyph g = rasterRegular(font, cp, px);
	const int n = std::max(1, (int)std::lround((float)px * strength));
	g.advance += (float)n;
	if (g.w <= 0)
	{
		return g;
	}
	std::vector<u8> out((size_t)(g.w + n) * g.h, 0);
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w + n; ++x)
		{
			u8 best = 0;
			for (int j = 0; j <= n; ++j)
			{
				const int sx = x - j;
				if (sx >= 0 && sx < g.w)
				{
					best = std::max(best, g.a[(size_t)y * g.w + sx]);
				}
			}
			out[(size_t)y * (g.w + n) + x] = best;
		}
	}
	g.a.swap(out);
	g.w += n;
	return g;
}

Glyph raster(Method method, const stbtt_fontinfo& font, int cp, int px, float strength)
{
	switch (method)
	{
	case Current:       return rasterCurrent(font, cp, px, strength);
	case Supersample:   return rasterSupersample(font, cp, px, strength, 8);
	case Outline:       return rasterOutline(font, cp, px, strength);
	case HorizontalInt: return rasterHorizontal(font, cp, px, strength);
	default:            return rasterRegular(font, cp, px);
	}
}

// ---- 측정: 잉크 픽셀(커버리지 ≥ 80, 화면 루마 < 175) 중 아주 진한(커버리지 ≥ 196, 루마 < 60) 비율, 가장 진한 값(루마), 잉크 합
struct Stats { int ink = 0; int veryDark = 0; int darkest = 0; double sum = 0.0; int gaps = 0; int closed = 0; };

void accumulate(const Glyph& g, Stats& s)
{
	for (u8 v : g.a)
	{
		s.sum += v / 255.0;
		if (v >= 80)
		{
			++s.ink;
			if (v >= 196)
			{
				++s.veryDark;
			}
		}
		s.darkest = std::max(s.darkest, (int)v);
	}
}

// 획 사이 빈 곳(보통 글리프에서 가로 · 세로로 양끝이 잉크(≥128)인 빈 칸(<64) 연속 구간)이 굵게 뒤 모두 ≥192 로 칠해졌나
void countGaps(const Glyph& normal, const Glyph& bold, Stats& s)
{
	auto nAt = [&](int x, int y) { return (int)normal.a[(size_t)y * normal.w + x]; };
	auto bAt = [&](int x, int y)
	{
		const int bx = x + normal.ox - bold.ox, by = y + normal.oy - bold.oy;
		if (bx < 0 || by < 0 || bx >= bold.w || by >= bold.h) return 0;
		return (int)bold.a[(size_t)by * bold.w + bx];
	};
	for (int pass = 0; pass < 2; ++pass)
	{
		const bool horizontal = pass == 0;
		const int lines = horizontal ? normal.h : normal.w, length = horizontal ? normal.w : normal.h;
		for (int line = 0; line < lines; ++line)
		{
			int p = 0;
			while (p < length)
			{
				auto at = [&](int q) { return horizontal ? nAt(q, line) : nAt(line, q); };
				if (at(p) >= 64) { ++p; continue; }
				const int b = p;
				while (p < length && at(p) < 64) ++p;
				const int e = p;
				if (b == 0 || e >= length || at(b - 1) < 128 || at(e) < 128) continue;
				++s.gaps;
				bool closed = true;
				for (int q = b; q < e; ++q)
				{
					const int v = horizontal ? bAt(q, line) : bAt(line, q);
					if (v < 192) { closed = false; break; }
				}
				if (closed) ++s.closed;
			}
		}
	}
}

// ---- 그림: 흰 바탕에 검은 글(커버리지는 겹치면 큰 값), 4배 확대
struct Canvas { int w, h; std::vector<u8> cov; };

void drawGlyph(Canvas& canvas, const Glyph& g, int penX, int baseline)
{
	for (int y = 0; y < g.h; ++y)
	{
		for (int x = 0; x < g.w; ++x)
		{
			const int cx = penX + g.ox + x, cy = baseline + g.oy + y;
			if (cx < 0 || cy < 0 || cx >= canvas.w || cy >= canvas.h) continue;
			u8& d = canvas.cov[(size_t)cy * canvas.w + cx];
			d = std::max(d, g.a[(size_t)y * g.w + x]);
		}
	}
}

int main(int argc, char** argv)
{
	const char* fontPath  = (argc > 1) ? argv[1] : "C:/Develop/ProjectAH/Content/CardTemplates/fonts/act_body.ttf";
	const char* outPrefix = (argc > 2) ? argv[2] : "boldlab";
	const float strength  = (argc > 3) ? (float)atof(argv[3]) : 1.0f / 16.0f;
	// 방법마다 세기(같은 굵기로 맞춰 비교할 때): 인자 4 = 여러 배 그리기, 5 = 윤곽선. 없으면 모두 같은 세기
	float strengthFor[MethodCount] = { 0.0f, strength, strength, strength, strength };
	if (argc > 4) strengthFor[Supersample] = (float)atof(argv[4]);
	if (argc > 5) strengthFor[Outline]     = (float)atof(argv[5]);

	FILE* file = fopen(fontPath, "rb");
	if (file == nullptr) { printf("no font %s\n", fontPath); return 1; }
	fseek(file, 0, SEEK_END); const long size = ftell(file); fseek(file, 0, SEEK_SET);
	std::vector<u8> data((size_t)size); fread(data.data(), 1, (size_t)size, file); fclose(file);
	stbtt_fontinfo font;
	if (stbtt_InitFont(&font, data.data(), stbtt_GetFontOffsetForIndex(data.data(), 0)) == 0) { printf("bad font\n"); return 1; }

	const int measured[] = { 0xD3ED, 0xB85C, 0xAC15, 0xC81C, 0xC804, 0xD22C, 0xD611, 0xC0C1 };   // 폭 로 강 제 전 투 협 상
	const int sizes[] = { 14, 16, 20 };
	printf("font %s, strength current %.4f / supersample %.4f / outline %.4f / horizontal %.4f em\n", fontPath, strengthFor[Current], strengthFor[Supersample], strengthFor[Outline], strengthFor[HorizontalInt]);
	printf("size method      ink  veryDark%%  darkestLuma  inkSum(+vs regular)  gapsClosed\n");
	for (int px : sizes)
	{
		double regularSum = 0.0;
		for (int m = 0; m < MethodCount; ++m)
		{
			Stats s;
			for (int cp : measured)
			{
				const Glyph normal = rasterRegular(font, cp, px);
				const Glyph g      = raster((Method)m, font, cp, px, strengthFor[m]);
				accumulate(g, s);
				if (m != Regular && normal.w > 0)
				{
					countGaps(normal, g, s);
				}
			}
			if (m == Regular) regularSum = s.sum;
			printf("%4d %-11s %4d  %6.1f%%    %4d        %7.1f (%+6.1f)      %d/%d\n", px, MethodNames[m], s.ink, s.ink ? 100.0 * s.veryDark / s.ink : 0.0,
				255 - s.darkest, s.sum, s.sum - regularSum, s.closed, s.gaps);
		}
	}

	// 비교 그림: 크기마다 방법별 한 줄 — 굵게 "폭로 – 강제" + 보통 " 둡니다 전투"
	const int boldPart[]    = { 0xD3ED, 0xB85C, 0x20, 0x2D, 0x20, 0xAC15, 0xC81C };
	const int regularPart[] = { 0x20, 0xB461, 0xB2C8, 0xB2E4, 0x20, 0xC804, 0xD22C };
	for (int px : sizes)
	{
		const int lineHeight = px + px / 2 + 4;
		Canvas canvas{ px * 14, lineHeight * MethodCount, {} };
		canvas.cov.assign((size_t)canvas.w * canvas.h, 0);
		const float scale = scaleFor(font, px);
		int ascent = 0, descent = 0, lineGap = 0;
		stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);
		for (int m = 0; m < MethodCount; ++m)
		{
			const int baseline = m * lineHeight + 2 + (int)std::lround(ascent * scale * 0.9f);
			float pen = 4.0f;
			int previous = 0;
			auto put = [&](int cp, bool bold)
			{
				if (previous != 0) pen += stbtt_GetCodepointKernAdvance(&font, previous, cp) * scale;
				const Glyph g = bold ? raster((Method)m, font, cp, px, strengthFor[m]) : rasterRegular(font, cp, px);
				drawGlyph(canvas, g, (int)std::floor(pen + 0.5f), baseline);
				pen += g.advance;
				previous = cp;
			};
			for (int cp : boldPart) put(cp, true);
			for (int cp : regularPart) put(cp, false);
		}
		const int zoom = 4;
		std::vector<u8> rgb((size_t)canvas.w * zoom * canvas.h * zoom * 3);
		for (int y = 0; y < canvas.h * zoom; ++y)
		{
			for (int x = 0; x < canvas.w * zoom; ++x)
			{
				const u8 c = canvas.cov[(size_t)(y / zoom) * canvas.w + x / zoom];
				const u8 luma = (u8)(255 - c);
				u8* p = &rgb[((size_t)y * canvas.w * zoom + x) * 3];
				p[0] = p[1] = p[2] = luma;
			}
		}
		const std::string path = std::string(outPrefix) + "_" + std::to_string(px) + "px.png";
		stbi_write_png(path.c_str(), canvas.w * zoom, canvas.h * zoom, 3, rgb.data(), canvas.w * zoom * 3);
		printf("wrote %s (rows: %s / %s / %s / %s / %s)\n", path.c_str(), MethodNames[0], MethodNames[1], MethodNames[2], MethodNames[3], MethodNames[4]);
	}
	return 0;
}
