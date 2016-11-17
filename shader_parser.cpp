//-----------------------------------------------------------------------------
// File: shader_parser.cpp
//
// Implementation of the shader parser
//-----------------------------------------------------------------------------

#include "util.h"
#include "pak.h"
#include "console.h"
#include "shader_parser.h"
#include "d3d.h"
#include <memory>

#include "mem.h"
#define new mem_new

using std::auto_ptr;

cvar_int_t verbose_shader_parsing("verbose_shader_parsing", 0, CVF_NONE, 0, 1);

#ifdef _DEBUG
// Uncomment this line to log very verbose parsing messages (always off in
// release builds)
//#define DEBUG_SHADER_PARSER
#endif

namespace {

	bool is_literal(token_type_t type) { return (type == SL_FLOAT || type == SL_STRING); }
	bool is_keyword(token_type_t type) { return (type >= SK_ADD && type <= SK_WAVE); }
	bool is_expression(token_type_t type) { return (type >= SE_ALPHAFUNC && type <= SE_WAVETYPE); }

	// Keywords just map a single keyword type to its text based equivalent. There
	// should be one keyword for each SK_xxx variable in the token_type_t enum and
	// they should be in the same order as they are listed there
	struct keyword_t {
		const char* name;
		token_type_t type;
	};

	// Array of names for the literal types and the token_type_t that they represent
	// These are used only for error reporting
	const keyword_t literals[] = {
		{ "<float>", SL_FLOAT },
		{ "<string>", SL_STRING },
	};
	const int num_literals = sizeof(literals) / sizeof(*literals);

	// Array of keywords and the token_type_t that they represent
	const keyword_t keywords[] = {
		{ "add",					SK_ADD						},
		{ "additive",				SK_ADDITIVE					},
		{ "alphafunc",				SK_ALPHAFUNC				},
		{ "alphagen",				SK_ALPHAGEN					},
		{ "alphashadow",			SK_ALPHASHADOW				},
		{ "animmap",				SK_ANIMMAP					},
		{ "areaportal",				SK_AREAPORTAL				},
		{ "autosprite",				SK_AUTOSPRITE				},
		{ "autosprite2",			SK_AUTOSPRITE2				},
		{ "back",					SK_BACK						},
		{ "banner",					SK_BANNER					},
		{ "base",					SK_BASE						},
		{ "blend",					SK_BLEND					},
		{ "blendfunc",				SK_BLENDFUNC				},
		{ "bulge",					SK_BULGE					},
		{ "clampmap",				SK_CLAMPMAP					},
		{ "clusterportal",			SK_CLUSTERPORTAL			},
		{ "cull",					SK_CULL						},
		{ "-",						SK_DASH						},
		{ "deformvertexes",			SK_DEFORMVERTEXES			},
		{ "depthfunc",				SK_DEPTHFUNC				},
		{ "depthwrite",				SK_DEPTHWRITE				},
		{ "disable",				SK_DISABLE					},
		{ "$lightmap",				SK_DOLLAR_LIGHTMAP			},
		{ "$whiteimage",			SK_DOLLAR_WHITEIMAGE		},
		{ "donotenter",				SK_DONOTENTER				},
		{ "entity",					SK_ENTITY					},
		{ "environment",			SK_ENVIRONMENT				},
		{ "equal",					SK_EQUAL					},
		{ "exactvertex",			SK_EXACTVERTEX				},
		{ "filter",					SK_FILTER					},
		{ "flesh",					SK_FLESH					},
		{ "fog",					SK_FOG						},
		{ "fogparms",				SK_FOGPARMS					},
		{ "front",					SK_FRONT					},
		{ "ge128",					SK_GE128					},
		{ "gl_dst_color",			SK_GL_DST_COLOR				},
		{ "gl_one",					SK_GL_ONE					},
		{ "gl_one_minus_dst_alpha",	SK_GL_ONE_MINUS_DST_ALPHA	},
		{ "gl_one_minus_dst_color",	SK_GL_ONE_MINUS_DST_COLOR	},
		{ "gl_one_minus_src_alpha",	SK_GL_ONE_MINUS_SRC_ALPHA	},
		{ "gl_one_minus_src_color",	SK_GL_ONE_MINUS_SRC_COLOR	},
		{ "gl_src_alpha",			SK_GL_SRC_ALPHA				},
		{ "gl_src_color",			SK_GL_SRC_COLOR				},
		{ "gl_zero",				SK_GL_ZERO					},
		{ "gt0",					SK_GT0						},
		{ "identity",				SK_IDENTITY					},
		{ "identitylighting",		SK_IDENTITYLIGHTING			},
		{ "inversesawtooth",		SK_INVERSESAWTOOTH			},
		{ "lava",					SK_LAVA						},
		{ "{",						SK_LEFT_BRACE				},
		{ "(",						SK_LEFT_BRACKET				},
		{ "lequal",					SK_LEQUAL					},
		{ "light",					SK_LIGHT					},
		{ "lightingdiffuse",		SK_LIGHTINGDIFFUSE			},
		{ "lightingspecular",		SK_LIGHTINGSPECULAR			},
		{ "lightmap",				SK_LIGHTMAP					},
		{ "lt128",					SK_LT128					},
		{ "map",					SK_MAP						},
		{ "metalsteps",				SK_METALSTEPS				},
		{ "move",					SK_MOVE						},
		{ "nearest",				SK_NEAREST					},
		{ "nodamage",				SK_NODAMAGE					},
		{ "nodlight",				SK_NODLIGHT					},
		{ "nodraw",					SK_NODRAW					},
		{ "nodrop",					SK_NODROP					},
		{ "noimpact",				SK_NOIMPACT					},
		{ "noise",					SK_NOISE					},
		{ "nolightmap",				SK_NOLIGHTMAP				},
		{ "nomarks",				SK_NOMARKS					},
		{ "nomipmaps",				SK_NOMIPMAPS				},
		{ "none",					SK_NONE						},
		{ "nonsolid",				SK_NONSOLID					},
		{ "nopicmip",				SK_NOPICMIP					},
		{ "normal",					SK_NORMAL					},
		{ "nosteps",				SK_NOSTEPS					},
		{ "opaque",					SK_OPAQUE					},
		{ "origin",					SK_ORIGIN					},
		{ "playerclip",				SK_PLAYERCLIP				},
		{ "portal",					SK_PORTAL					},
		{ "q3map_backshader",		SK_Q3MAP_BACKSHADER			},
		{ "q3map_globaltexture",	SK_Q3MAP_GLOBALTEXTURE		},
		{ "q3map_lightimage",		SK_Q3MAP_LIGHTIMAGE			},
		{ "q3map_lightsubdivide",	SK_Q3MAP_LIGHTSUBDIVIDE		},
		{ "q3map_sun",				SK_Q3MAP_SUN				},
		{ "q3map_surfacelight",		SK_Q3MAP_SURFACELIGHT		},
		{ "tesssize",				SK_Q3MAP_TESSSIZE			},
		{ "qer_editorimage",		SK_QER_EDITORIMAGE			},
		{ "qer_nocarve",			SK_QER_NOCARVE				},
		{ "qer_trans",				SK_QER_TRANS				},
		{ "rgbgen",					SK_RGBGEN					},
		{ "}",						SK_RIGHT_BRACE				},
		{ ")",						SK_RIGHT_BRACKET			},
		{ "rotate",					SK_ROTATE					},
		{ "sawtooth",				SK_SAWTOOTH					},
		{ "scale",					SK_SCALE					},
		{ "scroll",					SK_SCROLL					},
		{ "sin",					SK_SIN						},
		{ "sky",					SK_SKY						},
		{ "skyparms",				SK_SKYPARMS					},
		{ "slick",					SK_SLICK					},
		{ "slime",					SK_SLIME					},
		{ "sort",					SK_SORT						},
		{ "square",					SK_SQUARE					},
		{ "stretch",				SK_STRETCH					},
		{ "structural",				SK_STRUCTURAL				},
		{ "surfaceparm",			SK_SURFACEPARM				},
		{ "tcgen",					SK_TCGEN					},
		{ "tcmod",					SK_TCMOD					},
		{ "trans",					SK_TRANS					},
		{ "transform",				SK_TRANSFORM				},
		{ "triangle",				SK_TRIANGLE					},
		{ "turb",					SK_TURB						},
		{ "twosided",				SK_TWOSIDED					},
		{ "underwater",				SK_UNDERWATER				},
		{ "vertex",					SK_VERTEX					},
		{ "water",					SK_WATER					},
		{ "wave",					SK_WAVE						},
	};
	const int num_keywords = sizeof(keywords) / sizeof(*keywords);

	// Expression flags
	const int EXPR_REPEAT = 0x1;		// Expression can repeat multiple times
	const int EXPR_COMPONENT = 0x2;		// Set the remaining count for the parent expression

	// The entire file can be parsed using the expressions contained in the
	// expressions array
	struct expression_t {
		enum { MAX_PATTERNS = 26 };
		enum { MAX_PATTERN_TOKENS = 8 };

		const char*		name;	// Expression name (used for parse errors only)
		token_type_t	type;	// Shader token type for the expression SE_xxx
		int				flags;	// Expression flags
		token_type_t 	tokens[MAX_PATTERNS][MAX_PATTERN_TOKENS];	// Token types that make up an expression
	};

	// The complete set of shader expressions. The entire shader file is parsed as
	// a single SE_SHADERFILE expression. There must be one entry in here for each
	// SE_xxx variable in the token_type enum and they must be in the same order as
	// they are listed there, parsing is slightly faster if the keywords and literals
	// appear before the expressions in the match order
	const expression_t expressions[] = {
		{ 	"ALPHAFUNC", SE_ALPHAFUNC, EXPR_COMPONENT, {
	 			{ SK_GE128, SE_END },
	 			{ SK_GT0, SE_END },
	 			{ SK_LT128, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"ALPHAGEN", SE_ALPHAGEN, EXPR_COMPONENT, {
	 			{ SK_ENTITY, SE_END },
	 			{ SK_LIGHTINGSPECULAR, SE_END },
	 			{ SK_VERTEX, SE_END },
	 			{ SK_WAVE, SE_WAVEFUNC, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"ANIMMAP", SE_ANIMMAP, EXPR_REPEAT | EXPR_COMPONENT, {
	 			{ SL_STRING, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"BLENDFUNC", SE_BLENDFUNC, EXPR_COMPONENT, {
	 			{ SK_ADD, SE_END },
	 			{ SK_BLEND, SE_END },
	 			{ SK_FILTER, SE_END },
	 			{ SE_SRCBLEND, SE_DESTBLEND, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"CLOUDHEIGHT", SE_CLOUDHEIGHT, EXPR_COMPONENT, {
	 			{ SL_FLOAT, SE_END },
	 			{ SK_DASH, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"CULL", SE_CULL, EXPR_COMPONENT, {
	 			{ SK_BACK, SE_END },
	 			{ SK_DISABLE, SE_END },
	 			{ SK_FRONT, SE_END },
	 			{ SK_NONE, SE_END },
				{ SK_TWOSIDED, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"DEFORMVERTEXES", SE_DEFORMVERTEXES, EXPR_COMPONENT, {
	 			{ SK_AUTOSPRITE, SE_END },
	 			{ SK_AUTOSPRITE2, SE_END },
	 			{ SK_BULGE, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_MOVE, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_WAVEFUNC, SE_END },
	 			{ SK_NORMAL, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_WAVE, SL_FLOAT, SE_WAVEFUNC, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"DEPTHFUNC", SE_DEPTHFUNC, EXPR_COMPONENT, {
	 			{ SK_EQUAL, SE_END },
	 			{ SK_LEQUAL, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"DESTBLEND", SE_DESTBLEND, EXPR_COMPONENT, {
	 			{ SK_GL_SRC_COLOR, SE_END },
	 			{ SK_GL_ONE, SE_END },
	 			{ SK_GL_ONE_MINUS_DST_ALPHA, SE_END },
	 			{ SK_GL_ONE_MINUS_SRC_ALPHA, SE_END },
	 			{ SK_GL_ONE_MINUS_SRC_COLOR, SE_END },
	 			{ SK_GL_SRC_ALPHA, SE_END },
	 			{ SK_GL_ZERO, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"FOGPARMS", SE_FOGPARMS, EXPR_COMPONENT, {
	 			{ SK_LEFT_BRACKET, SL_FLOAT, SL_FLOAT, SL_FLOAT, SK_RIGHT_BRACKET, SL_FLOAT, SE_END },
	 			{ SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"MAPNAME", SE_MAPNAME, EXPR_COMPONENT, {
	 			{ SL_STRING, SE_END },
	 			{ SK_DOLLAR_LIGHTMAP, SE_END },
	 			{ SK_DOLLAR_WHITEIMAGE, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"RGBGEN", SE_RGBGEN, EXPR_COMPONENT, {
	 			{ SK_ENTITY, SE_END },
	 			{ SK_EXACTVERTEX, SE_END },
	 			{ SK_IDENTITY, SE_END },
	 			{ SK_IDENTITYLIGHTING, SE_END },
	 			{ SK_LIGHTINGDIFFUSE, SE_END },
	 			{ SK_VERTEX, SE_END },
	 			{ SK_WAVE, SE_WAVEFUNC, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SHADER", SE_SHADER, 0, {
	 			{ SL_STRING, SK_LEFT_BRACE, SE_SHADERBODY, SK_RIGHT_BRACE, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SHADERBODY", SE_SHADERBODY, EXPR_REPEAT, {
	 			{ SK_CULL, SE_CULL, SE_END },
	 			{ SK_DEFORMVERTEXES, SE_DEFORMVERTEXES, SE_END },
	 			{ SK_FOGPARMS, SE_FOGPARMS, SE_END },
	 			{ SK_LIGHT, SL_FLOAT, SE_END },
	 			{ SK_NOMIPMAPS, SE_END },
	 			{ SK_NOPICMIP, SE_END },
	 			{ SK_Q3MAP_BACKSHADER, SL_STRING, SE_END },
	 			{ SK_Q3MAP_GLOBALTEXTURE, SE_END },
	 			{ SK_Q3MAP_LIGHTIMAGE, SL_STRING, SE_END },
	 			{ SK_Q3MAP_LIGHTSUBDIVIDE, SL_FLOAT, SE_END },
	 			{ SK_Q3MAP_SUN, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_Q3MAP_SURFACELIGHT, SL_FLOAT, SE_END },
	 			{ SK_Q3MAP_TESSSIZE, SL_FLOAT, SE_END },
	 			{ SK_QER_EDITORIMAGE, SL_STRING, SE_END},
	 			{ SK_QER_NOCARVE, SE_END },
	 			{ SK_QER_TRANS, SL_FLOAT, SE_END },
	 			{ SK_SKYPARMS, SE_SKYBOX, SE_CLOUDHEIGHT, SE_SKYBOX, SE_END },
	 			{ SK_SORT, SE_SORT, SE_END },
				{ SK_SURFACEPARM, SE_SURFACEPARM, SE_END },
	 			{ SE_STAGE, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SKYBOX", SE_SKYBOX, EXPR_COMPONENT, {
	 			{ SL_STRING, SE_END },
	 			{ SK_DASH, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SORT", SE_SORT, EXPR_COMPONENT, {
	 			{ SK_ADDITIVE, SE_END },
	 			{ SK_BANNER, SE_END },
	 			{ SK_NEAREST, SE_END },
	 			{ SK_OPAQUE, SE_END },
	 			{ SK_PORTAL, SE_END },
	 			{ SK_SKY, SE_END },
	 			{ SK_UNDERWATER, SE_END },
				{ SL_FLOAT, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SRCBLEND", SE_SRCBLEND, EXPR_COMPONENT, {
	 			{ SK_GL_DST_COLOR, SE_END },
	 			{ SK_GL_ONE, SE_END },
	 			{ SK_GL_ONE_MINUS_DST_COLOR, SE_END },
	 			{ SK_GL_ONE_MINUS_SRC_ALPHA, SE_END },
	 			{ SK_GL_SRC_ALPHA, SE_END },
	 			{ SK_GL_ZERO, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"STAGE", SE_STAGE, 0, {
	 			{ SK_LEFT_BRACE, SE_STAGEBODY, SK_RIGHT_BRACE, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"STAGEBODY", SE_STAGEBODY, EXPR_REPEAT, {
	 			{ SK_ALPHAFUNC, SE_ALPHAFUNC, SE_END },
	 			{ SK_ALPHAGEN, SE_ALPHAGEN, SE_END },
	 			{ SK_ANIMMAP, SL_FLOAT, SE_ANIMMAP, SE_END },
	 			{ SK_BLENDFUNC, SE_BLENDFUNC, SE_END },
	 			{ SK_CLAMPMAP, SE_MAPNAME, SE_END },
	 			{ SK_DEPTHFUNC, SE_DEPTHFUNC, SE_END },
	 			{ SK_DEPTHWRITE, SE_END },
	 			{ SK_MAP, SE_MAPNAME, SE_END },
	 			{ SK_RGBGEN, SE_RGBGEN, SE_END },
	 			{ SK_TCGEN, SE_TCGEN, SE_END },
	 			{ SK_TCMOD, SE_TCMOD, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"SURFACEPARM", SE_SURFACEPARM, EXPR_COMPONENT, {
	 			{ SK_ALPHASHADOW, SE_END },
	 			{ SK_AREAPORTAL, SE_END },
	 			{ SK_CLUSTERPORTAL, SE_END },
	 			{ SK_DONOTENTER, SE_END },
	 			{ SK_FLESH, SE_END },
	 			{ SK_FOG, SE_END },
	 			{ SK_LAVA, SE_END },
	 			{ SK_METALSTEPS, SE_END },
	 			{ SK_NODAMAGE, SE_END },
	 			{ SK_NODLIGHT, SE_END },
	 			{ SK_NODRAW, SE_END },
	 			{ SK_NODROP, SE_END },
	 			{ SK_NOIMPACT, SE_END },
	 			{ SK_NOLIGHTMAP, SE_END },
	 			{ SK_NOMARKS, SE_END },
	 			{ SK_NONSOLID, SE_END },
	 			{ SK_NOSTEPS, SE_END },
	 			{ SK_ORIGIN, SE_END },
	 			{ SK_PLAYERCLIP, SE_END },
	 			{ SK_SKY, SE_END },
	 			{ SK_SLICK, SE_END },
	 			{ SK_SLIME, SE_END },
	 			{ SK_STRUCTURAL, SE_END },
	 			{ SK_TRANS, SE_END },
	 			{ SK_WATER, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"TCGEN", SE_TCGEN, EXPR_COMPONENT, {
	 			{ SK_BASE, SE_END },
	 			{ SK_ENVIRONMENT, SE_END },
	 			{ SK_LIGHTMAP, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"TCMOD", SE_TCMOD, EXPR_COMPONENT, {
	 			{ SK_ROTATE, SL_FLOAT, SE_END },
	 			{ SK_SCALE, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_SCROLL, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_STRETCH, SE_WAVEFUNC, SE_END },
	 			{ SK_TRANSFORM, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_TURB, SE_TURB, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"TURB", SE_TURB, EXPR_COMPONENT, {
	 			{ SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
	 			{ SK_SIN, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"WAVEFUNC", SE_WAVEFUNC, EXPR_COMPONENT, {
	 			{ SE_WAVETYPE, SL_FLOAT, SL_FLOAT, SL_FLOAT, SL_FLOAT, SE_END },
				{ SE_END },
	 		}
		},
		{ 	"WAVETYPE", SE_WAVETYPE, EXPR_COMPONENT, {
	 			{ SK_INVERSESAWTOOTH, SE_END },
	 			{ SK_NOISE, SE_END },
	 			{ SK_SAWTOOTH, SE_END },
	 			{ SK_SIN, SE_END },
	 			{ SK_SQUARE, SE_END },
	 			{ SK_TRIANGLE, SE_END },
				{ SE_END },
		 	}
		},
	};
	const int num_expressions = sizeof(expressions) / sizeof(*expressions);

	inline const keyword_t&
	get_literal(token_type_t type)
		// Returns the correct literal for type
	{
		u_assert(is_literal(type));
		return literals[type - SL_FLOAT];
	}

	inline const keyword_t&
	get_keyword(token_type_t type)
		// Returns the correct keyword for type
	{
		u_assert(is_keyword(type));
		return keywords[type - SK_ADD];
	}

	inline const expression_t&
	get_expression(token_type_t type)
		// Returns the correct expression for type
	{
		u_assert(is_expression(type));
		return expressions[type - SE_ALPHAFUNC];
	}

	bool
	validate_grammer()
		// Checks that the grammer is still correct (debugging aid)
		// Makes sure that keyword_index and expression_index return indexes that
		// are correct
	{
		// Check that get_literal returns the correct literal for all types
		for (int i = 0; i < num_literals; ++i)
			if (get_literal(literals[i].type).type != literals[i].type)
				return false;
		// Check that get_keyword returns the correct keyword for all types
		for (i = 0; i < num_keywords; ++i)
			if (get_keyword(keywords[i].type).type != keywords[i].type)
				return false;
		// Check that get_expression returns the correct expression for all types
		for (i = 0; i < num_expressions; ++i) {
			if (get_expression(expressions[i].type).type != expressions[i].type)
				return false;
		}
		return true;
	}

	token_type_t
	token_type(const char* token, float& fvalue)
		// Returns the token_type_t of token. If the type == SL_FLOAT then the
		// float value is also stored into fvalue, otherwise fvalue will be
		// unchanged
	{
		for (int i = 0; i < num_keywords; ++i)
			if (u_stricmp(keywords[i].name, token) == 0)
				return keywords[i].type;
		if (u_strtof(token, fvalue, fvalue))
			return SL_FLOAT;
		return SL_STRING;
	}

	class parse_t {
		// Implementation for the parser
	public:
		parse_t(const char* filename, const char* data, const int size) 
			: tokenizer(data, size), num_tokens(0), insert_pos(textbuf), 
			  token(0), ok_to_keep(false)
		{
			name = filename;
		}

		int parse(const expression_t& expr) { return parse(expr, true); }
		const token_t* get_tokens() { return tokens; }
	private:
		enum { MAX_TOKENS = 500 };
		enum { TEXT_BUF_SIZE = 10240 };

		int parse(const expression_t& expr, bool reset);
		void keep_token();
		void peek_token();
		void read_token();
		void setup_token(const char* text);

		void warn(const expression_t& parsing, const char* expected);
		const token_type_t* find_match(const expression_t& expr, token_type_t start);

		filename_t name;			// Input source name (just used for error messages)
		tokenizer_t tokenizer;		// Tokenizer 
		token_t* token;
		token_t tokens[MAX_TOKENS];
		char textbuf[TEXT_BUF_SIZE];

		bool ok_to_keep;	// Set true in read_token, false in peek_token

		int num_tokens;
		char* insert_pos;
	};

	void
	parse_t::keep_token()
		// The current token has been accepted as valid so keep it
	{
		u_assert(ok_to_keep);
		u_assert(token->type != SE_END);

		if (token->type == SL_STRING)
			insert_pos += u_strlen(insert_pos) + 1;
		++num_tokens;
	}
	
	void
	parse_t::peek_token()
		// Peeks the next token into the tokens array, and call setup_token
	{
		ok_to_keep = false;
		setup_token(tokenizer.peek_token(insert_pos, TEXT_BUF_SIZE - (insert_pos - textbuf)));
	}

	void
	parse_t::read_token()
		// Reads the next token into the tokens array, and call setup_token
	{
		ok_to_keep = true;
		setup_token(tokenizer.next_token(insert_pos, TEXT_BUF_SIZE - (insert_pos - textbuf)));
	}

	void
	parse_t::setup_token(const char* text)
		// Set the fields of token appropriately
	{
		token = &tokens[num_tokens];
		token->expr = SE_END;
		token->remaining = 0;	// Remaining is set at the end of parse
		if (text) {
			token->type = token_type(text, token->float_value);
			if (token->type == SL_STRING)
				token->string_value = text;
			else if (token->type != SL_FLOAT)
				token->string_value = get_keyword(token->type).name;
		} else {
			token->type = SE_END;
			token->string_value = 0;
		}
	}

	void
	parse_t::warn(const expression_t& parsing, const char* expected)
		// Print a warning message to the console
	{
		if (!*verbose_shader_parsing)
			return;
		char buffer[MAX_TOKEN];
		tokenizer_t::mark_t mark = tokenizer.mark();
		if (!ok_to_keep)
			tokenizer.next_token();
		console.warnf("File: \"%s\", line %d\n", name.str(), tokenizer.line_no() + 1);
		console.warnf("%s\n", tokenizer.current_line(buffer, MAX_TOKEN));
		for (int i = 0; i < tokenizer.line_pos(); ++i)
			console.warn(buffer[i] == '\t' ? "\t" : " ");
		console.warn("^\nshader parse error: ");
		console.warnf("parsing %s, expected %s, encountered %s\n",
				parsing.name, expected, tokenizer.current_token());
		if (!ok_to_keep)
			tokenizer.reset(mark);
	}

	const token_type_t*
	parse_t::find_match(const expression_t& expr, token_type_t start)
		// Returns a pointer to the first token of the first sub expression
		// of expr for which start is a valid starting token
	{
		u_assert(is_keyword(start) || is_literal(start));

		for (int i = 0; expr.tokens[i][0]; ++i) {
			if (expr.tokens[i][0] == start)
				return expr.tokens[i];
			if (is_expression(expr.tokens[i][0])) {
				if (find_match(get_expression(expr.tokens[i][0]), start))
					return expr.tokens[i];
			}
		}
		return 0;
	}

	int
	parse_t::parse(const expression_t& expr, bool reset)
		// Attempts to parse an expression of type expr from the tokenizer
		// Returns the number of tokens consumed, zero means that the requested
		// expression cannot be parsed from the current location in the text
		// buffer
	{
#ifdef DEBUG_SHADER_PARSER
		static char tabbuffer[50];
		static int recurse_depth = 0;
		console.printf("%sstarted parsing %s\n", tabbuffer, expr.name);
		tabbuffer[recurse_depth++] = '\t';
#endif
		if (reset) {
			num_tokens = 0;
			insert_pos = textbuf;
		}

		peek_token();
		// Check that the end of the file hasn't been reached
		if (token->type == SE_END) {
#ifdef DEBUG_SHADER_PARSER
			tabbuffer[recurse_depth--] = 0;
			console.printf("%sfinished parsing %s - end of file reached\n", tabbuffer, expr.name);
#endif
			return 0;
		}

		// Find the matching expression
		const token_type_t* subexpr = find_match(expr, token->type);
		if (subexpr == 0) {	// No match found
			warn(expr, expr.name);
#ifdef DEBUG_SHADER_PARSER
			tabbuffer[--recurse_depth] = 0;
			console.printf("%sfinished parsing %s - 0 tokens consumed\n", tabbuffer, expr.name);
#endif
			return 0;
		}

		// Save the position in the tokenizer
		tokenizer_t::mark_t mark = tokenizer.mark();

		int consumed = 0;	// Tokens consumed
		int discarded = 0;	// Tokens discarded

		// Match found so parse the expression. Whenever there is a repeatable
		// expression then a recursive call is made to parse it again.
		for (int i = 0; subexpr[i] != SE_END; ++i) {
			if (is_expression(subexpr[i])) {
				// Attempt to parse the expression
				const expression_t& subexpression = get_expression(subexpr[i]);
				if (subexpression.flags & EXPR_REPEAT) {
					// Parsing an expression that repeats, read as many as possible
//					int num;
//					while ((num = parse(subexpression, false)) != 0) {
//						consumed += num;
//					}
					while (find_match(subexpression, token->type)) {
						int num = parse(subexpression, false);
						consumed += num;
						if (num == 0)
							break;
						peek_token();
					}

					// If at the end of a repeating expression the next token is not
					// the beginning of the next expression then just dump the token
					// with a printed warning and continue. This is not the ideal
					// situation but it is the best place to ignore unexpected tokens
					if (subexpr[i + 1] != SE_END) {
						// Two repeating expressions cannot follow each other
						u_assert(!is_expression(subexpr[i + 1]));
						peek_token();
						if (token->type != subexpr[i + 1]) {
							char expected[256];
							const keyword_t& kw = (is_literal(subexpr[i + 1]) 
								? get_literal(subexpr[i + 1]) 
								: get_keyword(subexpr[i + 1]));
							u_snprintf(expected, 256, "%s or %s", subexpression.name, kw.name);
							read_token();
							warn(expr, expected);
#ifdef DEBUG_SHADER_PARSER
							if (token->type == SL_FLOAT)
								console.printf("%sdiscarded %f\n", tabbuffer, token->float_value);
							else
								console.printf("%sdiscarded %s\n", tabbuffer, token->string_value);
#endif
							++consumed;		// token was consumed
							++discarded;	// token was discarded
							--i;
						}
					}
				} else {
					int num = parse(subexpression, false);
					if (num)
						consumed += num;
					else 
						break;
				}
			} else if (token->type == subexpr[i]) {
				read_token();
				keep_token();
#ifdef DEBUG_SHADER_PARSER
				if (token->type == SL_FLOAT)
					console.printf("%sconsumed %f\n", tabbuffer, token->float_value);
				else
					console.printf("%sconsumed %s\n", tabbuffer, token->string_value);
#endif
				++consumed;
			} else {
				break;
			}
			peek_token();
		}
		if (subexpr[i] != SE_END) {
			tokenizer.reset(mark);
			num_tokens -= (consumed - discarded);
#ifdef DEBUG_SHADER_PARSER
			tabbuffer[--recurse_depth] = 0;
			console.printf("%sfinished parsing %s - 0 tokens consumed\n", tabbuffer, expr.name);
#endif
			return 0;
		}

		if (!(expr.flags & EXPR_COMPONENT))
			for (i = 0; i < (consumed - discarded); ++i) {
				if (tokens[num_tokens - i - 1].remaining == 0)
					tokens[num_tokens - i - 1].remaining = i + 1;
				if (tokens[num_tokens - i - 1].expr == SE_END)
					tokens[num_tokens - i - 1].expr = expr.type;
			}

#ifdef DEBUG_SHADER_PARSER
		tabbuffer[--recurse_depth] = 0;
		console.printf("%sfinished parsing %s - %d tokens consumed\n", tabbuffer, expr.name, consumed);

		if (expr.type == SE_SHADER) {
			console.print("[---------- PARSED SHADER ---------]\n");
			// Print completed shaders to the console
			for (int i = 0; i < num_tokens; ++i) {
				if (tokens[i].type == SK_RIGHT_BRACE) 
					tabbuffer[--recurse_depth] = 0;
				console.print(tabbuffer);
				if (tokens[i].expr != SE_SHADER && tokens[i].expr != SE_STAGE) {
					while (tokens[i].remaining != 1) {
						if (tokens[i].type == SL_FLOAT)
							console.printf("%f (%d) ", tokens[i].float_value, tokens[i].remaining);
						else
							console.printf("%s (%d) ", tokens[i].string_value, tokens[i].remaining);
						++i;
					}
				}
				if (tokens[i].type == SL_FLOAT)
					console.printf("%f (%d)\n", tokens[i].float_value, tokens[i].remaining);
				else
					console.printf("%s (%d)\n", tokens[i].string_value, tokens[i].remaining);
				if (tokens[i].type == SK_LEFT_BRACE)
					tabbuffer[recurse_depth++] = '\t';
			}
		}
#endif
		return consumed - discarded;
	}
}

void 
shader_parser_t::parse_file(const char* filename)
	// Parses an entire shader file
{
	u_assert(validate_grammer());

	auto_ptr<file_t> file(pak.open_file(filename));
	if (file.get() == 0) {
		console.warnf("Failed to open shader file %s\n", filename);
		return;
	}
	parse_t parse(filename, reinterpret_cast<const char*>(file->data()), file->size());

#ifdef DEBUG_SHADER_PARSER
	console.print("[=========== BEGIN FILE ===========]\n");
	console.print("[---------- BEGIN SHADER ----------]\n");
	int num_shaders = 0;
#endif
	while (parse.parse(get_expression(SE_SHADER))) {
		d3d.upload_shader(parse.get_tokens());
#ifdef DEBUG_SHADER_PARSER
		console.print("[----------- END SHADER -----------]\n");
		console.print("[---------- BEGIN SHADER ----------]\n");
		++num_shaders;
#endif
	}
#ifdef DEBUG_SHADER_PARSER
	console.print("[----------- END SHADER -----------]\n");
	console.printf("[Total: %d shaders in file %s]\n", num_shaders, filename);
	console.print("[============ END FILE ============]\n");
#endif
}
