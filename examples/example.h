#define glfw_include_glu

#include <glfw/glfw3.h>

#define stb_truetype_implementation
#include "third_party/stb_truetype.h"

#include "stdio.h"

struct font
{
	gluint tex;
	stbtt_bakedchar cdata[96]; // ascii 32..126 is 95 glyph
};

static struct font ttf_create(file * f)
{
	int buffer_size;
	void * buffer;
	unsigned char temp_bitmap[512*512];
	struct font fn;

	fseek(f, 0, seek_end);
	buffer_size = ftell(f);

	buffer = malloc(buffer_size);
	fseek(f, 0, seek_set);
	fread(buffer, 1, buffer_size, f);

	stbtt_bakefontbitmap((unsigned char*)buffer,0, 20.0, temp_bitmap, 512,512, 32,96, fn.cdata); // no guarantee this fits!
	free(buffer);

	glgentextures(1, &fn.tex);
	glbindtexture(gl_texture_2d, fn.tex);
	glteximage2d(gl_texture_2d, 0, gl_alpha, 512,512, 0, gl_alpha, gl_unsigned_byte, temp_bitmap);
	gltexparameteri(gl_texture_2d, gl_texture_mag_filter, gl_linear);
	gltexparameteri(gl_texture_2d, gl_texture_min_filter, gl_linear);
	gltexparameteri(gl_texture_2d, gl_texture_wrap_s, gl_clamp);
	gltexparameteri(gl_texture_2d, gl_texture_wrap_t, gl_clamp);

	return fn;
}

static void ttf_print(struct font * font, float x, float y, const char *text)
{
	glpushattrib(gl_all_attrib_bits);
	glenable(gl_texture_2d);
	glbindtexture(gl_texture_2d, font->tex);
	glenable(gl_blend);
	glblendfunc(gl_src_alpha, gl_one_minus_src_alpha);
	glbegin(gl_quads);
	for(; *text; ++text)
	{
		if (*text >= 32 && *text < 128)
		{
			stbtt_aligned_quad q;
			stbtt_getbakedquad(font->cdata, 512,512, *text-32, &x,&y,&q,1);
			gltexcoord2f(q.s0,q.t0); glvertex2f(q.x0,q.y0);
			gltexcoord2f(q.s1,q.t0); glvertex2f(q.x1,q.y0);
			gltexcoord2f(q.s1,q.t1); glvertex2f(q.x1,q.y1);
			gltexcoord2f(q.s0,q.t1); glvertex2f(q.x0,q.y1);
		}
	}
	glend();
	glpopattrib();
}

static float ttf_len(struct font * font, const char *text)
{
	float x=0, y=0;
	for(; *text; ++text)
	{
		if (*text >= 32 && *text < 128)
		{
			stbtt_aligned_quad q;
			stbtt_getbakedquad(font->cdata, 512,512, *text-32, &x,&y,&q,1);
		}
	}
	return x;
}

static void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
	uint32_t histogram[0x10000] = {};
	uint8_t rgb_image[640*480*3];
	int i, d, f;

	for(i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
	for(i = 2; i < 0x10000; ++i) histogram[i] += histogram[i-1]; // build a cumulative histogram for the indices in [1,0xffff]
	for(i = 0; i < width*height; ++i)
	{
		if(d = depth_image[i])
		{
			f = histogram[d] * 255 / histogram[0xffff]; // 0-255 based on histogram location
			rgb_image[i*3 + 0] = 255 - f;
			rgb_image[i*3 + 1] = 0;
			rgb_image[i*3 + 2] = f;
		}
		else
		{
			rgb_image[i*3 + 0] = 20;
			rgb_image[i*3 + 1] = 5;
			rgb_image[i*3 + 2] = 0;
		}
	}
	gldrawpixels(width, height, gl_rgb, gl_unsigned_byte, rgb_image);
}
