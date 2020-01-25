#include <stdlib.h>
#include "Random.h"

static void Seed(int seed) { srand(seed); }

static int Int() { return rand(); }

static Scalar Float() { return (Scalar)rand() / (Scalar)RAND_MAX; }

static Scalar FloatRange(Scalar min, Scalar max) { return (max - min) * Random.Scalar() + min; }

static int IntRange(int min, int max) { return Random.Int() % (max - min) + min; }

int _SimplexHash[256] =
{
	0x97,0xA0,0x89,0x5B,0x5A,0x0F,0x83,0x0D,0xC9,0x5F,0x60,0x35,0xC2,0xE8,0x07,0xE1,
	0x8C,0x24,0x67,0x1E,0x45,0x8E,0x08,0x63,0x25,0xF0,0x15,0x0A,0x17,0xBE,0x06,0x94,
	0xF7,0x78,0xEA,0x4B,0x00,0x1A,0xC5,0x3E,0x5E,0xFC,0xDB,0xCB,0x75,0x23,0x0B,0x20,
	0x39,0x75,0x21,0x58,0xED,0x95,0x38,0x57,0xAE,0x14,0x7D,0x88,0xAB,0xA8,0x44,0xAF,
	0x4A,0xA5,0x47,0x86,0x8B,0x30,0x1B,0xA6,0x4D,0x92,0x9E,0xE7,0x53,0x6F,0xE5,0x7A,
	0x3C,0xD3,0x85,0xE6,0xDC,0x69,0x5C,0x29,0x37,0x2E,0xF5,0x28,0xF4,0x66,0x8F,0x36,
	0x41,0x19,0x3F,0xA1,0x01,0xD8,0x50,0x49,0xD1,0x4C,0x84,0xBB,0xD0,0x59,0x12,0xA9,
	0xC8,0xC4,0x87,0x82,0x74,0xBC,0x9F,0x56,0xA4,0x64,0x6D,0xC6,0xAD,0xBA,0x03,0x40,
	0x34,0xD9,0xE2,0xFA,0x7C,0x7B,0x05,0xCA,0x26,0x93,0x76,0x7E,0xFF,0x52,0x55,0xD4,
	0xCF,0xCE,0x38,0xE3,0x2F,0x10,0x3A,0x11,0xB6,0xBD,0x1C,0x2A,0xDF,0xB7,0xAA,0xD5,
	0x77,0xF8,0x98,0x02,0x2C,0x9A,0xA3,0x46,0xDD,0x99,0x65,0x9B,0xA7,0x2B,0xAC,0x09,
	0x81,0x16,0x27,0xFD,0x13,0x62,0x6C,0x6E,0x4F,0x71,0xE0,0xE8,0xB2,0xB9,0x70,0x68,
	0xDA,0xF6,0x61,0xE4,0xFB,0x22,0xF2,0xC1,0xEE,0xD2,0x90,0x0C,0xBF,0xB3,0xA2,0xF1,
	0x51,0x33,0x91,0xEB,0xF9,0x0E,0xEF,0x6B,0x31,0xC0,0xD6,0x1F,0xB5,0xC7,0x6A,0x9D,
	0xB8,0x54,0xCC,0xB0,0x73,0x79,0x32,0x2D,0x7F,0x04,0x96,0xFE,0x8A,0xEC,0xCD,0x5D,
	0xDE,0x72,0x43,0x1D,0x18,0x48,0xF3,0x8D,0x80,0xC3,0x4E,0x42,0xD7,0x3D,0x9C,0xB4
};

static int P(int i) { return _SimplexHash[i % 256]; }
static Scalar Fade(Scalar t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
static Scalar Grad(int hash, Scalar x, Scalar y, Scalar z)
{
	int h = hash & 15;
	Scalar u = h < 8 ? x : y;
	Scalar v;
	if (h < 4) { v = y; }
	else if (h == 12 || h == 14) { v = x; }
	else { v = z; }
	return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}
static Scalar Lerp(Scalar a, Scalar b, Scalar t) { return a + (b - a) * t; }
static Scalar Simplex3(Vector3 xyz)
{
	Scalar x = xyz.X + 5000.0, y = xyz.Y + 5000.0, z = xyz.Z + 5000.0;
	int xi = (int)x & 255; int yi = (int)y & 255; int zi = (int)z & 255;
	double xf = x - (int)x; double yf = y - (int)y; double zf = z - (int)z;
	double u = Fade(xf); double v = Fade(yf); double w = Fade(zf);
	int aaa, aba, aab, abb, baa, bba, bab, bbb;
	aaa = P(P(P(xi) + yi) + zi);
	aba = P(P(P(xi) + yi + 1) + zi);
	aab = P(P(P(xi) + yi) + zi + 1);
	abb = P(P(P(xi) + yi + 1) + zi + 1);
	baa = P(P(P(xi + 1) + yi) + zi);
	bba = P(P(P(xi + 1) + yi + 1) + zi);
	bab = P(P(P(xi + 1) + yi) + zi + 1);
	bbb = P(P(P(xi + 1) + yi + 1) + zi + 1);
	double x1, x2, y1, y2;
	x1 = Lerp(Grad(aaa, xf, yf, zf), Grad(baa, xf - 1, yf, zf), u);
	x2 = Lerp(Grad(aba, xf, yf - 1, zf), Grad(bba, xf - 1, yf - 1, zf), u);
	y1 = Lerp(x1, x2, v);
	x1 = Lerp(Grad(aab, xf, yf, zf - 1), Grad(bab, xf - 1, yf, zf - 1), u);
	x2 = Lerp(Grad(abb, xf, yf - 1, zf - 1), Grad(bbb, xf - 1, yf - 1, zf - 1), u);
	y2 = Lerp(x1, x2, v);
	return Lerp(y1, y2, w);
}

struct RandomInterface Random =
{
	.Seed = Seed,
	.Int = Int,
	.Scalar = Float,
	.ScalarRange = FloatRange,
	.IntRange = IntRange,
	.Simplex3 = Simplex3,
};
