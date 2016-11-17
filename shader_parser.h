//-----------------------------------------------------------------------------
// File: shader_parser.h
//
// Defines the shader parser, the shader parser is used to read *.shader files
// and convert the shader definitions in them into parsed_shader structs. These
// structs are in turn passed on to the renderer which is then responsible for
// dealing with them. 
//-----------------------------------------------------------------------------

#ifndef SHADER_PARSER_H
#define SHADER_PARSER_H

#include "exec.h"

extern cvar_int_t verbose_shader_parsing;

/////////////////////////////////////
// SL_* - Shader Literals. Only two types, floating point values and strings
// SK_* - Shader Keywords. Each corresponds to a single case-insensitive keyword
// SE_* - Shader Expressions. Expressions represent a valid sequence of literals,
//        keywords and other expressions
// Important: If you add any more values here make sure to update the
//    is_literal, is_keyword and is_expression functions below
enum token_type_t {
	SE_END,		// 0 is reserved for the end of expressions

	// Shader literals
	SL_FLOAT,
	SL_STRING,

	// Shader keywords
	SK_ADD,
	SK_ADDITIVE,
	SK_ALPHAFUNC,
	SK_ALPHAGEN,
	SK_ALPHASHADOW,
	SK_ANIMMAP,
	SK_AREAPORTAL,
	SK_AUTOSPRITE,
	SK_AUTOSPRITE2,
	SK_BACK,
	SK_BANNER,
	SK_BASE,
	SK_BLEND,
	SK_BLENDFUNC,
	SK_BULGE,
	SK_CLAMPMAP,
	SK_CLUSTERPORTAL,
	SK_CULL,
	SK_DASH,
	SK_DEFORMVERTEXES,
	SK_DEPTHFUNC,
	SK_DEPTHWRITE,
	SK_DISABLE,
	SK_DOLLAR_LIGHTMAP,
	SK_DOLLAR_WHITEIMAGE,
	SK_DONOTENTER,
	SK_ENTITY,
	SK_ENVIRONMENT,
	SK_EQUAL,
	SK_EXACTVERTEX,
	SK_FILTER,
	SK_FLESH,
	SK_FOG,
	SK_FOGPARMS,
	SK_FRONT,
	SK_GE128,
	SK_GL_DST_COLOR,
	SK_GL_ONE,
	SK_GL_ONE_MINUS_DST_ALPHA,
	SK_GL_ONE_MINUS_DST_COLOR,
	SK_GL_ONE_MINUS_SRC_ALPHA,
	SK_GL_ONE_MINUS_SRC_COLOR,
	SK_GL_SRC_ALPHA,
	SK_GL_SRC_COLOR,
	SK_GL_ZERO,
	SK_GT0,
	SK_IDENTITY,
	SK_IDENTITYLIGHTING,
	SK_INVERSESAWTOOTH,
	SK_LAVA,
	SK_LEFT_BRACE,
	SK_LEFT_BRACKET,
	SK_LEQUAL,
	SK_LIGHT,
	SK_LIGHTINGDIFFUSE,
	SK_LIGHTINGSPECULAR,
	SK_LIGHTMAP,
	SK_LT128,
	SK_MAP,
	SK_METALSTEPS,
	SK_MOVE,
	SK_NEAREST,
	SK_NODAMAGE,
	SK_NODLIGHT,
	SK_NODRAW,
	SK_NODROP,
	SK_NOIMPACT,
	SK_NOISE,
	SK_NOLIGHTMAP,
	SK_NOMARKS,
	SK_NOMIPMAPS,
	SK_NONE,
	SK_NONSOLID,
	SK_NOPICMIP,
	SK_NORMAL,
	SK_NOSTEPS,
	SK_OPAQUE,
	SK_ORIGIN,
	SK_PLAYERCLIP,
	SK_PORTAL,
	SK_Q3MAP_BACKSHADER,
	SK_Q3MAP_GLOBALTEXTURE,
	SK_Q3MAP_LIGHTIMAGE,
	SK_Q3MAP_LIGHTSUBDIVIDE,
	SK_Q3MAP_SUN,
	SK_Q3MAP_SURFACELIGHT,
	SK_Q3MAP_TESSSIZE,
	SK_QER_EDITORIMAGE,
	SK_QER_NOCARVE,
	SK_QER_TRANS,
	SK_RGBGEN,
	SK_RIGHT_BRACE,
	SK_RIGHT_BRACKET,
	SK_ROTATE,
	SK_SAWTOOTH,
	SK_SCALE,
	SK_SCROLL,
	SK_SIN,
	SK_SKY,
	SK_SKYPARMS,
	SK_SLICK,
	SK_SLIME,
	SK_SORT,
	SK_SQUARE,
	SK_STRETCH,
	SK_STRUCTURAL,
	SK_SURFACEPARM,
	SK_TCGEN,
	SK_TCMOD,
	SK_TRANS,
	SK_TRANSFORM,
	SK_TRIANGLE,
	SK_TURB,
	SK_TWOSIDED,
	SK_UNDERWATER,
	SK_VERTEX,
	SK_WATER,
	SK_WAVE,

	// Shader Expressions (technically these arent tokens but using the
	// same enum for them makes defining the valid expressions a lot easier
	SE_ALPHAFUNC,
	SE_ALPHAGEN,
	SE_ANIMMAP,
	SE_BLENDFUNC,
	SE_CLOUDHEIGHT,
	SE_CULL,
	SE_DEFORMVERTEXES,
	SE_DEPTHFUNC,
	SE_DESTBLEND,
	SE_FOGPARMS,
	SE_MAPNAME,
	SE_RGBGEN,
	SE_SHADER,
	SE_SHADERBODY,
	SE_SKYBOX,
	SE_SORT,
	SE_SRCBLEND,
	SE_STAGE,
	SE_STAGEBODY,
	SE_SURFACEPARM,
	SE_TCGEN,
	SE_TCMOD,
	SE_TURB,
	SE_WAVEFUNC,
	SE_WAVETYPE,
};

struct token_t {
	// A single shader token, the renderer is passed an array of these tokens
	// that corresponds to a single valid shader for each shader. The remaining
	// field for the first token in that array will specify the size of the 
	// array. Note that the expr and remaining variables are for the lowest
	// level expression of which the token represents. For example if a token
	// is part of a SE_BLENDFUNC, in a SE_STAGEBODY, in a SE_STAGE, inside a
	// SE_SHADERBODY, inside a SE_SHADER then expr will be SE_BLENDFUNC and
	// remaining will be the number of tokens remaining in the blendfunc
public:
	token_type_t type;	// What type of token this is: SL_xxx or SK_xxx
	token_type_t expr;	// The type of expression this token is part of SE_xxx
	int remaining;		// The number of tokens left in the current expression

	union {
		const char* string_value;
		float float_value;
	};
};

class shader_parser_t {
	// Parses the shader files to produce parsed shaders, then passes them to
	// the renderer in a parsed form
public:
	void parse_file(const char* file);
};

#endif
