//-----------------------------------------------------------------------------
// File: shader.cpp
//
// Shader related class implementations
//-----------------------------------------------------------------------------

#include "shader.h"
#include "util.h"
#include "pak.h"
#include "console.h"
#include "d3dlookup.h"

#include <memory>

#include "mem.h"
#define new mem_new

shader_manager_t& 
shader_manager_t::get_instance()
{
	static std::auto_ptr<shader_manager_t> instance(new shader_manager_t());
	return *instance;
}

shader_stage_t::~shader_stage_t()
{
	for (int i = 0; i < map_names.size(); i++)
		delete [] map_names[i];
	free_textures();
} 

void
shader_stage_t::load_textures(int flags)
	// Causes the stage to load any textures requred
{
	for (int i = 0; i < textures.size(); i++)
		textures[i] = d3d.get_texture(map_names[i], flags);
}

void
shader_stage_t::free_textures()
	// Causes the stage to release any textures it has acquired
{
	for (int i = 0; i < textures.size(); i++)
		if (textures[i])
			textures[i] = 0;
}

shader_t::~shader_t()
	// Destructor, free all the stages
{
	for (int i = 0; i < stages.size(); i++)
		delete stages[i];
	delete [] name;
}

void
shader_manager_t::destroy()
	// Delete the shader manager
{
	for (int i = 0; i < shaders.size(); i++)
		delete shaders[i];
}

shader_t*
shader_manager_t::get_shader(const char* name)
	// Return a reference to the named shader
{
	for (int i = 0; i < shaders.size(); i++)
		if (!u_strcmp(name, shaders[i]->name))
			if (ref_counts[i]++) {
				return shaders[i];
			} else {
				shaders[i]->activate();
				return shaders[i];
			}
	console.warnf("failed to find shader: \"%s\"\n", name);
	return 0;
}

void
shader_manager_t::release_shader(shader_t* shader)
{
	for (int i = 0; i < shaders.size(); i++)
		if (shaders[i] == shader)
			if ((--ref_counts[i]) == 0)
				shaders[i]->passivate();
}

static enum shader_token_type_t {
	STT_UNKNOWN = 0,
	STT_LEFT_BRACE,
	STT_RIGHT_BRACE,
	STT_LEFT_BRACKET,
	STT_RIGHT_BRACKET,
	STT_DASH,

	STT_MAP,
	STT_CLAMPMAP,
	STT_ANIMMAP,
	STT_DOLLAR_LIGHTMAP,
	STT_DOLLAR_WHITEIMAGE,

	STT_ALPHAFUNC,
	STT_GT0,
	STT_LT128,
	STT_GE128,

	STT_DEPTHFUNC,
	STT_EQUAL,
	STT_LEQUAL,

	STT_DEPTHWRITE,

	STT_RGBGEN,
	STT_ALPHAGEN,
	STT_ENTITY,
	STT_EXACTVERTEX,
	STT_IDENTITY,
	STT_IDENTITYLIGHTING,
	STT_LIGHTINGDIFFUSE,
	STT_VERTEX,
	STT_WAVE,
	STT_LIGHTINGSPECULAR,

	STT_SIN,
	STT_TRIANGLE,
	STT_SQUARE,
	STT_SAWTOOTH,
	STT_INVERSESAWTOOTH,
	STT_NOISE,

	STT_TCGEN,
	STT_BASE,
	STT_LIGHTMAP,
	STT_ENVIRONMENT,

	STT_TCMOD,
	STT_ROTATE,
	STT_SCALE,
	STT_SCROLL,
	STT_STRETCH,
	STT_TURB,
	STT_TRANSFORM,

	STT_BLENDFUNC,
	STT_ADD,
	STT_BLEND,
	STT_FILTER,
	STT_GL_DST_COLOR,
	STT_GL_SRC_COLOR,
	STT_GL_ZERO,
	STT_GL_ONE,
	STT_GL_SRC_ALPHA,
	STT_GL_ONE_MINUS_DST_ALPHA,
	STT_GL_ONE_MINUS_SRC_ALPHA,
	STT_GL_ONE_MINUS_DST_COLOR,
	STT_GL_ONE_MINUS_SRC_COLOR,

	STT_NOMIPMAPS,
	STT_NOPICMIP,

	STT_LIGHT,

	STT_FOGPARMS,
	STT_SKYPARMS,

	STT_CULL,
	STT_FRONT,
	STT_BACK,
	STT_DISABLE,
	STT_NONE,

	STT_SORT,
	STT_PORTAL,
	STT_SKY,
	STT_OPAQUE,
	STT_BANNER,
	STT_UNDERWATER,
	STT_ADDITIVE,
	STT_NEAREST,

	STT_DEFORMVERTEXES,
	STT_NORMAL,
	STT_BULGE,
	STT_MOVE,
	STT_AUTOSPRITE,
	STT_AUTOSPRITE2,

	STT_SURFACEPARM,
	STT_ALPHASHADOW,
	STT_AREAPORTAL,
	STT_CLUSTERPORTAL,
	STT_DONOTENTER,
	STT_FLESH,
	STT_FOG,
	STT_LAVA,
	STT_METALSTEPS,
	STT_NODAMAGE,
	STT_NODLIGHT,
	STT_NODRAW,
	STT_NODROP,
	STT_NOIMPACT,
	STT_NOMARKS,
	STT_NOLIGHTMAP,
	STT_NOSTEPS,
	STT_NONSOLID,
	STT_ORIGIN,
	STT_PLAYERCLIP,
	STT_SLICK,
	STT_SLIME,
	STT_STRUCTURAL,
	STT_TRANS,
	STT_WATER,

	// q3map specific values, unimportant here, just parse and discard them
	STT_Q3MAP_BACKSHADER,
	STT_Q3MAP_GLOBALTEXTURE,
	STT_Q3MAP_SUN,
	STT_Q3MAP_SURFACELIGHT,
	STT_Q3MAP_TESSSIZE,
	STT_Q3MAP_LIGHTIMAGE,
	STT_Q3MAP_LIGHTSUBDIVIDE,

	// Quake Editor Radiant specific keyworde
	STT_QER_EDITORIMAGE,
	STT_QER_NOCARVE,
	STT_QER_TRANS,
};

static shader_token_type_t
token_type(const char* token)
	// Return a token type for token
{
	if (!u_stricmp(token, "{"))
		return STT_LEFT_BRACE;
	if (!u_stricmp(token, "}"))
		return STT_RIGHT_BRACE;
	if (!u_stricmp(token, "("))
		return STT_LEFT_BRACKET;
	if (!u_stricmp(token, ")"))
		return STT_RIGHT_BRACKET;
	if (!u_stricmp(token, "-"))
		return STT_DASH;

	if (!u_stricmp(token, "map"))
		return STT_MAP;
	if (!u_stricmp(token, "clampmap"))
		return STT_CLAMPMAP;
	if (!u_stricmp(token, "animmap"))
		return STT_ANIMMAP;
	if (!u_stricmp(token, "$lightmap"))
		return STT_DOLLAR_LIGHTMAP;
	if (!u_stricmp(token, "$whiteimage"))
		return STT_DOLLAR_WHITEIMAGE;

	if (!u_stricmp(token, "alphafunc"))
		return STT_ALPHAFUNC;
	if (!u_stricmp(token, "gt0"))
		return STT_GT0;
	if (!u_stricmp(token, "lt128"))
		return STT_LT128;
	if (!u_stricmp(token, "ge128"))
		return STT_GE128;

	if (!u_stricmp(token, "depthfunc"))
		return STT_DEPTHFUNC;
	if (!u_stricmp(token, "equal"))
		return STT_EQUAL;
	if (!u_stricmp(token, "lequal"))
		return STT_LEQUAL;

	if (!u_stricmp(token, "depthwrite"))
		return STT_DEPTHWRITE;

	if (!u_stricmp(token, "rgbgen"))
		return STT_RGBGEN;
	if (!u_stricmp(token, "alphagen"))
		return STT_ALPHAGEN;
	if (!u_stricmp(token, "entity"))
		return STT_ENTITY;
	if (!u_stricmp(token, "exactvertex"))
		return STT_EXACTVERTEX;
	if (!u_stricmp(token, "identity"))
		return STT_IDENTITY;
	if (!u_stricmp(token, "identitylighting"))
		return STT_IDENTITYLIGHTING;
	if (!u_stricmp(token, "lightingdiffuse"))
		return STT_LIGHTINGDIFFUSE;
	if (!u_stricmp(token, "vertex"))
		return STT_VERTEX;
	if (!u_stricmp(token, "wave"))
		return STT_WAVE;
	if (!u_stricmp(token, "lightingspecular"))
		return STT_LIGHTINGSPECULAR;

	if (!u_stricmp(token, "sin"))
		return STT_SIN;
	if (!u_stricmp(token, "triangle"))
		return STT_TRIANGLE;
	if (!u_stricmp(token, "square"))
		return STT_SQUARE;
	if (!u_stricmp(token, "sawtooth"))
		return STT_SAWTOOTH;
	if (!u_stricmp(token, "inversesawtooth"))
		return STT_INVERSESAWTOOTH;
	if (!u_stricmp(token, "noise"))
		return STT_NOISE;

	if (!u_stricmp(token, "tcgen"))
		return STT_TCGEN;
	if (!u_stricmp(token, "base"))
		return STT_BASE;
	if (!u_stricmp(token, "lightmap"))
		return STT_LIGHTMAP;
	if (!u_stricmp(token, "environment"))
		return STT_ENVIRONMENT;

	if (!u_stricmp(token, "tcmod"))
		return STT_TCMOD;
	if (!u_stricmp(token, "rotate"))
		return STT_ROTATE;
	if (!u_stricmp(token, "scale"))
		return STT_SCALE;
	if (!u_stricmp(token, "scroll"))
		return STT_SCROLL;
	if (!u_stricmp(token, "stretch"))
		return STT_STRETCH;
	if (!u_stricmp(token, "turb"))
		return STT_TURB;
	if (!u_stricmp(token, "transform"))
		return STT_TRANSFORM;

	if (!u_stricmp(token, "blendfunc"))
		return STT_BLENDFUNC;
	if (!u_stricmp(token, "add"))
		return STT_ADD;
	if (!u_stricmp(token, "blend"))
		return STT_BLEND;
	if (!u_stricmp(token, "filter"))
		return STT_FILTER;
	if (!u_stricmp(token, "gl_dst_color"))
		return STT_GL_DST_COLOR;
	if (!u_stricmp(token, "gl_src_color"))
		return STT_GL_SRC_COLOR;
	if (!u_stricmp(token, "gl_zero"))
		return STT_GL_ZERO;
	if (!u_stricmp(token, "gl_one"))
		return STT_GL_ONE;
	if (!u_stricmp(token, "gl_src_alpha"))
		return STT_GL_SRC_ALPHA;
	if (!u_stricmp(token, "gl_one_minus_dst_alpha"))
		return STT_GL_ONE_MINUS_DST_ALPHA;
	if (!u_stricmp(token, "gl_one_minus_src_alpha"))
		return STT_GL_ONE_MINUS_SRC_ALPHA;
	if (!u_stricmp(token, "gl_one_minus_dst_color"))
		return STT_GL_ONE_MINUS_DST_COLOR;
	if (!u_stricmp(token, "gl_one_minus_src_color"))
		return STT_GL_ONE_MINUS_SRC_COLOR;

	if (!u_stricmp(token, "nomipmaps"))
		return STT_NOMIPMAPS;
	if (!u_stricmp(token, "nopicmip"))
		return STT_NOPICMIP;

	if (!u_stricmp(token, "light"))
		return STT_LIGHT;

	if (!u_stricmp(token, "fogparms"))
		return STT_FOGPARMS;
	if (!u_stricmp(token, "skyparms"))
		return STT_SKYPARMS;

	if (!u_stricmp(token, "cull"))
		return STT_CULL;
	if (!u_stricmp(token, "front"))
		return STT_FRONT;
	if (!u_stricmp(token, "back"))
		return STT_BACK;
	if (!u_stricmp(token, "disable"))
		return STT_DISABLE;
	if (!u_stricmp(token, "none"))
		return STT_NONE;

	if (!u_stricmp(token, "sort"))
		return STT_SORT;
	if (!u_stricmp(token, "portal"))
		return STT_PORTAL;
	if (!u_stricmp(token, "sky"))
		return STT_SKY;
	if (!u_stricmp(token, "opaque"))
		return STT_OPAQUE;
	if (!u_stricmp(token, "banner"))
		return STT_BANNER;
	if (!u_stricmp(token, "underwater"))
		return STT_UNDERWATER;
	if (!u_stricmp(token, "additive"))
		return STT_ADDITIVE;
	if (!u_stricmp(token, "nearest"))
		return STT_NEAREST;

	if (!u_stricmp(token, "deformvertexes"))
		return STT_DEFORMVERTEXES;
	if (!u_stricmp(token, "normal"))
		return STT_NORMAL;
	if (!u_stricmp(token, "bulge"))
		return STT_BULGE;
	if (!u_stricmp(token, "move"))
		return STT_MOVE;
	if (!u_stricmp(token, "autosprite"))
		return STT_AUTOSPRITE;
	if (!u_stricmp(token, "autosprite2"))
		return STT_AUTOSPRITE2;

	if (!u_stricmp(token, "surfaceparm"))
		return STT_SURFACEPARM;
	if (!u_stricmp(token, "alphashadow"))
		return STT_ALPHASHADOW;
	if (!u_stricmp(token, "areaportal"))
		return STT_AREAPORTAL;
	if (!u_stricmp(token, "clusterportal"))
		return STT_CLUSTERPORTAL;
	if (!u_stricmp(token, "donotenter"))
		return STT_DONOTENTER;
	if (!u_stricmp(token, "flesh"))
		return STT_FLESH;
	if (!u_stricmp(token, "fog"))
		return STT_FOG;
	if (!u_stricmp(token, "lava"))
		return STT_LAVA;
	if (!u_stricmp(token, "metalsteps"))
		return STT_METALSTEPS;
	if (!u_stricmp(token, "nodamage"))
		return STT_NODAMAGE;
	if (!u_stricmp(token, "nodlight"))
		return STT_NODLIGHT;
	if (!u_stricmp(token, "nodraw"))
		return STT_NODRAW;
	if (!u_stricmp(token, "nodrop"))
		return STT_NODROP;
	if (!u_stricmp(token, "noimpact"))
		return STT_NOIMPACT;
	if (!u_stricmp(token, "nomarks"))
		return STT_NOMARKS;
	if (!u_stricmp(token, "nolightmap"))
		return STT_NOLIGHTMAP;
	if (!u_stricmp(token, "nosteps"))
		return STT_NOSTEPS;
	if (!u_stricmp(token, "nonsolid"))
		return STT_NONSOLID;
	if (!u_stricmp(token, "origin"))
		return STT_ORIGIN;
	if (!u_stricmp(token, "playerclip"))
		return STT_PLAYERCLIP;
	if (!u_stricmp(token, "slick"))
		return STT_SLICK;
	if (!u_stricmp(token, "slime"))
		return STT_SLIME;
	if (!u_stricmp(token, "structural"))
		return STT_STRUCTURAL;
	if (!u_stricmp(token, "trans"))
		return STT_TRANS;
	if (!u_stricmp(token, "water"))
		return STT_WATER;

	if (!u_stricmp(token, "q3map_backshader"))
		return STT_Q3MAP_BACKSHADER;
	if (!u_stricmp(token, "q3map_globaltexture"))
		return STT_Q3MAP_GLOBALTEXTURE;
	if (!u_stricmp(token, "q3map_sun"))
		return STT_Q3MAP_SUN;
	if (!u_stricmp(token, "q3map_surfacelight"))
		return STT_Q3MAP_SURFACELIGHT;
	if (!u_stricmp(token, "tesssize"))	// This is q3map specific
		return STT_Q3MAP_TESSSIZE;
	if (!u_stricmp(token, "q3map_lightimage"))
		return STT_Q3MAP_LIGHTIMAGE;
	if (!u_stricmp(token, "q3map_lightsubdivide"))
		return STT_Q3MAP_LIGHTSUBDIVIDE;

	if (!u_stricmp(token, "qer_editorimage"))
		return STT_QER_EDITORIMAGE;
	if (!u_stricmp(token, "qer_nocarve"))
		return STT_QER_NOCARVE;
	if (!u_stricmp(token, "qer_trans"))
		return STT_QER_TRANS;

	return STT_UNKNOWN;
}

static inline shader_token_type_t
next_token_type(const char* &token, shader_token_type_t& type, tokenizer_t& tokenizer)
	// Get the next token from tokenizer, store it in token, get the token type and
	// store it in type then return type. throws an exception if the next token is NULL
{
	token = tokenizer.next_token();
	if (!token)
		throw "Error: Unexpected end of token stream";
	type = token_type(token);
	return type;
}

static inline shader_token_type_t
peek_token_type(const char* &token, shader_token_type_t& type, tokenizer_t& tokenizer)
	// Get the next token from tokenizer, store it in token, get the token type and
	// store it in type then return type. throws an exception if the next token is NULL
{
	token = tokenizer.peek_token();
	if (!token)
		throw "Error: Unexpected end of token stream";
	type = token_type(token);
	return type;
}

void
shader_manager_t::warn(const char* filename, const tokenizer_t& tok, const char* message)
	// Prints a warning message showing the current line number, where the error is
	// and highlighting the position of the error. The message string may include a
	// single %s directive and if it does then the current token from tokenizer will
	// be printed at that position
{
	char buffer[MAX_TOKEN];
	console.warnf("File: \"%s\", line %d\n%s\n", filename, tok.line_no() + 1, tok.current_line(buffer, MAX_TOKEN));
	for (int i = 0; i < tok.line_pos(); i++)
		if (buffer[i] == '\t')
			console.warn("\t");
		else
			console.warn(" ");
	console.warnf("^\nshader parse error: ");
	console.warnf(message, tok.current_token(buffer, MAX_TOKEN));
	console.warn("\n");
	shader_manager.errors++;
}

int
shader_manager_t::num_stages() const
	// Return the total number of shader stages
{
	int num = 0;
	for (int i = 0; i < shaders.size(); i++)
		num += shaders[i]->stages.size();
	return num;
}

void
shader_manager_t::parse_file(const char* filename)
{
	file_t* file = pak.open_file(filename);
	if (!file) {
		console.warnf("Warning: could not open %s\n", filename);
		return;
	}
	console.debugf("Parsing %s\n", filename);

	tokenizer_t tokenizer(reinterpret_cast<const char*>(file->data()), file->size());

	try {

	const char* token;
	shader_token_type_t type;
	// Read shaders, each run through this loop reads a complete shader
	for (token = tokenizer.next_token(); token; token = tokenizer.next_token()) {
		// Get the shader name
		if ((type = token_type(token)) != STT_UNKNOWN)
			warn(filename, tokenizer, "keyword as shader name: %s");
		shader_t* shader = new shader_t(token);
		console.debugf("parsing shader: %s\n", shader->name);

		// Read the shader body
		if (next_token_type(token, type, tokenizer) == STT_LEFT_BRACE) {
			int nstage = 0;
			int nvdeform = 0;
			vert_deform_t vdeform;
			while (next_token_type(token, type, tokenizer) != STT_RIGHT_BRACE) {
				shader_stage_t* stage = 0; // declared here to appease the compiler
				int ntcmod;	// declared here to appease the compiler
				int i; // declared here to appease the compiler
				switch (type) {
				case STT_LEFT_BRACE:	// Read a stage
					stage = new shader_stage_t();
					ntcmod = 0;
					while (next_token_type(token, type, tokenizer) != STT_RIGHT_BRACE) {
						bool read_destblend; // declared here to appease the compiler
						tcmod_t tcmod;	// declared here to appease the compiler
						switch (type) {
						case STT_CLAMPMAP:	// DELIBERATE FALLTHROUGH
							stage->flags |= SHADER_STAGE_CLAMP;
							console.debugf(" - stage[%d].flag SHADER_STAGE_CLAMP set\n", nstage);
						case STT_MAP:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_DOLLAR_LIGHTMAP:
							case STT_DOLLAR_WHITEIMAGE:
							case STT_UNKNOWN:
								stage->add_map(token);
								console.debugf(" - stage[%d].map_names[%d] = %s\n", nstage, stage->map_names.size() - 1, token);
								break;
							default:
								warn(filename, tokenizer, "keyword as stage map name: %s");
							};
							break;
						case STT_ANIMMAP:
							next_token_type(token, type, tokenizer);
							if (u_strtof(token, stage->anim_frequency))
								console.debugf(" - stage[%d].anim_frequency = %f\n", nstage, stage->anim_frequency);
							else
								warn(filename, tokenizer, "invalid anim_frequency for stage: %s");
							while (peek_token_type(token, type, tokenizer) == STT_UNKNOWN) {
								stage->add_map(token);
								console.debugf(" - stage[%d].map_names[%d] = %s\n", nstage, stage->map_names.size() - 1, token);
								token = tokenizer.next_token();
							}
							break;
						case STT_ALPHAFUNC:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_GT0:
								stage->alpha_func = D3DCMP_GREATER;
								stage->alpha_ref = 0;
								console.debugf(" - stage[%d].alpha_func = D3DCMP_GREATER\n", nstage);
								console.debugf(" - stage[%d].alpha_ref = 0\n", nstage);
								break;
							case STT_LT128:
								stage->alpha_func = D3DCMP_LESS;
								stage->alpha_ref = 128;
								console.debugf(" - stage[%d].alpha_func = D3DCMP_LESS\n", nstage);
								console.debugf(" - stage[%d].alpha_ref = 128\n", nstage);
								break;
							case STT_GE128:
								stage->alpha_func = D3DCMP_GREATEREQUAL;
								stage->alpha_ref = 128;
								console.debugf(" - stage[%d].alpha_func = D3DCMP_GREATEREQUAL\n", nstage);
								console.debugf(" - stage[%d].alpha_ref = 128\n", nstage);
								break;
							default:
								warn(filename, tokenizer, "unrecognized alphafunc for stage: %s");
							};
							break;
						case STT_DEPTHFUNC:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_EQUAL:
								stage->depth_func = D3DCMP_EQUAL;
								console.debugf(" - stage[%d].depth_func = D3DCMP_EQUAL\n", nstage);
								break;
							case STT_LEQUAL:
								stage->depth_func = D3DCMP_LESSEQUAL;
								console.debugf(" - stage[%d].depth_func = D3DCMP_LESSEQUAL\n", nstage);
								break;
							default:
								warn(filename, tokenizer, "unrecognized depthfunc: %s");
							};
							break;
						case STT_DEPTHWRITE:
							stage->flags |= SHADER_STAGE_DEPTHWRITE;
							console.debugf(" - stage[%d].flag SHADER_STAGE_DEPTHWRITE set\n", nstage);
							break;
						case STT_RGBGEN:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_ENTITY:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_ENTITY\n", nstage);
								stage->rgbgen.method = RGBGEN_ENTITY;
								break;
							case STT_EXACTVERTEX:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_EXACTVERTEX\n", nstage);
								stage->rgbgen.method = RGBGEN_EXACTVERTEX;
								break;
							case STT_IDENTITY:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_IDENTITY\n", nstage);
								stage->rgbgen.method = RGBGEN_IDENTITY;
								break;
							case STT_IDENTITYLIGHTING:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_IDENTITYLIGHTING\n", nstage);
								stage->rgbgen.method = RGBGEN_IDENTITYLIGHTING;
								break;
							case STT_LIGHTINGDIFFUSE:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_LIGHTINGDIFFUSE\n", nstage);
								stage->rgbgen.method = RGBGEN_LIGHTINGDIFFUSE;
								break;
							case STT_VERTEX:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_VERTEX\n", nstage);
								stage->rgbgen.method = RGBGEN_VERTEX;
								break;
							case STT_WAVE:
								console.debugf(" - stage[%d].rgbgen.method = RGBGEN_WAVE\n", nstage);
								stage->rgbgen.method = RGBGEN_WAVE;
								switch (next_token_type(token, type, tokenizer)) {
								case STT_SIN:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_SIN\n", nstage);
									break;
								case STT_TRIANGLE:
									stage->rgbgen.wavefunc = WF_TRIANGLE;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_TRIANGLE\n", nstage);
									break;
								case STT_SQUARE:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_SQUARE\n", nstage);
									break;
								case STT_SAWTOOTH:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_SAWTOOTH\n", nstage);
									break;
								case STT_INVERSESAWTOOTH:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_INVERSESAWTOOTH\n", nstage);
									break;
								case STT_NOISE:
									stage->rgbgen.wavefunc = WF_NOISE;
									console.debugf(" - stage[%d].rgbgen.wavefunc = WF_NOISE\n", nstage);
									break;
								default:
									warn(filename, tokenizer, "unrecognized rgbgen wave: %s");
								};
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.base))
									console.debugf(" - stage[%d].rgbgen.base = %f\n", nstage, stage->rgbgen.base);
								else
									warn(filename, tokenizer, "invalid base for wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.amplitude))
									console.debugf(" - stage[%d].rgbgen.amplitude = %f\n", nstage, stage->rgbgen.amplitude);
								else
									warn(filename, tokenizer, "invalid amplitude for wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.phase))
									console.debugf(" - stage[%d].rgbgen.phase = %f\n", nstage, stage->rgbgen.phase);
								else
									warn(filename, tokenizer, "invalid phase for wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.frequency))
									console.debugf(" - stage[%d].rgbgen.frequency = %f\n", nstage, stage->rgbgen.frequency);
								else
									warn(filename, tokenizer, "invalid frequency for wave: %s");
								break;
							default:
								warn(filename, tokenizer, "unrecognized rgbgen: %s");
							};
							break;
						case STT_ALPHAGEN:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_ENTITY:
								console.debugf(" - stage[%d].alphagen.method = ALPHAGEN_ENTITY\n", nstage);
								stage->alphagen.method = ALPHAGEN_ENTITY;
								break;
							case STT_VERTEX:
								console.debugf(" - stage[%d].alphagen.method = ALPHAGEN_VERTEX\n", nstage);
								stage->alphagen.method = ALPHAGEN_VERTEX;
								break;
							case STT_WAVE:
								console.debugf(" - stage[%d].alphagen.method = ALPHAGEN_WAVE\n", nstage);
								stage->alphagen.method = ALPHAGEN_WAVE;
								switch (next_token_type(token, type, tokenizer)) {
								case STT_SIN:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_SIN\n", nstage);
									break;
								case STT_TRIANGLE:
									stage->rgbgen.wavefunc = WF_TRIANGLE;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_TRIANGLE\n", nstage);
									break;
								case STT_SQUARE:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_SQUARE\n", nstage);
									break;
								case STT_SAWTOOTH:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_SAWTOOTH\n", nstage);
									break;
								case STT_INVERSESAWTOOTH:
									stage->rgbgen.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_INVERSESAWTOOTH\n", nstage);
									break;
								case STT_NOISE:
									stage->rgbgen.wavefunc = WF_NOISE;
									console.debugf(" - stage[%d].alphagen.wavefunc = WF_NOISE\n", nstage);
									break;
								default:
									warn(filename, tokenizer, "unrecognized wavefunc for alphagen wave: %s");
								};
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.base))
									console.debugf(" - stage[%d].alphagen.base = %f\n", nstage, stage->rgbgen.base);
								else
									warn(filename, tokenizer, "invalid base for alphagen wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.amplitude))
									console.debugf(" - stage[%d].alphagen.amplitude = %f\n", nstage, stage->rgbgen.amplitude);
								else
									warn(filename, tokenizer, "invalid amplitude for alphagen wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.phase))
									console.debugf(" - stage[%d].alphagen.phase = %f\n", nstage, stage->rgbgen.phase);
								else
									warn(filename, tokenizer, "invalid phase for alphagen wave: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, stage->rgbgen.frequency))
									console.debugf(" - stage[%d].alphagen.frequency = %f\n", nstage, stage->rgbgen.frequency);
								else
									warn(filename, tokenizer, "invalid frequency for alphagen wave: %s");
								break;
							case STT_LIGHTINGSPECULAR:
								console.debugf(" - stage[%d].rgbgen.method = ALPHAGEN_LIGHTINGSPECULAR\n", nstage);
								stage->alphagen.method = ALPHAGEN_LIGHTINGSPECULAR;
								break;
							default:
								warn(filename, tokenizer, "unrecognized method for alphagen: %s");
							};
							break;
						case STT_TCGEN:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_BASE:
								stage->tcgen.method = TCGEN_BASE;
								console.debugf(" - stage[%d].tcgen.method = TCGEN_BASE\n", nstage);
								break;
							case STT_LIGHTMAP:
								stage->tcgen.method = TCGEN_LIGHTMAP;
								console.debugf(" - stage[%d].tcgen.method = TCGEN_LIGHTMAP\n", nstage);
								break;
							case STT_ENVIRONMENT:
								stage->tcgen.method = TCGEN_ENVIRONMENT;
								console.debugf(" - stage[%d].tcgen.method = TCGEN_ENVIRONMENT\n", nstage);
								break;
							default:
								warn(filename, tokenizer, "unrecognized method for tcgen: %s");
							}
							break;
						case STT_TCMOD:
							switch (next_token_type(token, type, tokenizer)) {
							case STT_ROTATE:
								tcmod.method = TCMOD_ROTATE;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_ROTATE\n", nstage, ntcmod);
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.angle))
									console.debugf(" - stage[%d].tcmod[%d].angle = %f\n", nstage, ntcmod, tcmod.angle);
								else
									warn(filename, tokenizer, "invalid angle for tcmod rotate: %s");
								break;
							case STT_SCALE:
								tcmod.method = TCMOD_SCALE;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_SCALE\n", nstage, ntcmod);
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.sscale))
									console.debugf(" - stage[%d].tcmod[%d].sscale = %f\n", nstage, ntcmod, tcmod.sscale);
								else
									warn(filename, tokenizer, "invalid sscale for tcmod scale: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.tscale))
									console.debugf(" - stage[%d].tcmod[%d].tscale = %f\n", nstage, ntcmod, tcmod.tscale);
								else
									warn(filename, tokenizer, "invalid tscale for tcmod scale: %s");
								break;
							case STT_SCROLL:
								tcmod.method = TCMOD_SCROLL;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_SCROLL\n", nstage, ntcmod);
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.sspeed))
									console.debugf(" - stage[%d].tcmod[%d].sspeed = %f\n", nstage, ntcmod, tcmod.sspeed);
								else
									warn(filename, tokenizer, "invalid sspeed for tcmod scroll: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.tspeed))
									console.debugf(" - stage[%d].tcmod[%d].tspeed = %f\n", nstage, ntcmod, tcmod.tspeed);
								else
									warn(filename, tokenizer, "invalid tspeed for tcmod scroll: %s");
								break;
							case STT_STRETCH:
								tcmod.method = TCMOD_STRETCH;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_STRETCH\n", nstage, ntcmod);
								switch (next_token_type(token, type, tokenizer)) {
								case STT_SIN:
									tcmod.wavefunc = WF_SIN;
									console.debugf(" - stage[%d].tcmod[%d].wavefunc = WF_SIN\n", nstage, ntcmod);
									break;
								case STT_TRIANGLE:
									tcmod.wavefunc = WF_TRIANGLE;
									console.debugf(" - stage[%d].tcmod[%d].wavefunc = WF_TRIANGLE\n", nstage, ntcmod);
									break;
								case STT_SQUARE:
									tcmod.wavefunc = WF_SQUARE;
									console.debugf(" - stage[%d].tcmod[%d].wavefunc = WF_SQUARE\n", nstage, ntcmod);
									break;
								case STT_SAWTOOTH:
									tcmod.wavefunc = WF_SAWTOOTH;
									console.debugf(" - stage[%d].tcmod[%d].wavefunc = WF_SAWTOOTH\n", nstage, ntcmod);
									break;
								case STT_INVERSESAWTOOTH:
									tcmod.wavefunc = WF_INVERSESAWTOOTH;
									console.debugf(" - stage[%d].tcmod[%d].wavefunc = WF_INVERSESAWTOOTH\n", nstage, ntcmod);
									break;
								default:
									warn(filename, tokenizer, "unrecognized wavefunc for tcmod stretch: %s");
								};
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.base))
									console.debugf(" - stage[%d].tcmod[%d].base = %f\n", nstage, ntcmod, tcmod.base);
								else
									warn(filename, tokenizer, "invalid base for tcmod stretch: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.amplitude))
									console.debugf(" - stage[%d].tcmod[%d].amplitude = %f\n", nstage, ntcmod, tcmod.amplitude);
								else
									warn(filename, tokenizer, "invalid amplitude for tcmod stretch: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.phase))
									console.debugf(" - stage[%d].tcmod[%d].phase = %f\n", nstage, ntcmod, tcmod.phase);
								else
									warn(filename, tokenizer, "invalid phase for tcmod stretch: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.frequency))
									console.debugf(" - stage[%d].tcmod[%d].frequency = %f\n", nstage, ntcmod, tcmod.frequency);
								else
									warn(filename, tokenizer, "invalid frequency for tcmod stretch: %s");
								break;
							case STT_TURB:
								tcmod.method = TCMOD_TURB;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_TURB\n", nstage, ntcmod);
								if (next_token_type(token, type, tokenizer) == STT_SIN) {
									console.debug(" - ignoring sin in tcmod turb\n");
									next_token_type(token, type, tokenizer);
								}
								if (u_strtof(token, tcmod.base))
									console.debugf(" - stage[%d].tcmod[%d].base = %f\n", nstage, ntcmod, tcmod.base);
								else
									warn(filename, tokenizer, "invalid base for tcmod turb: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.amplitude))
									console.debugf(" - stage[%d].tcmod[%d].amplitude = %f\n", nstage, ntcmod, tcmod.amplitude);
								else
									warn(filename, tokenizer, "invalid amplitude for tcmod turb: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.phase))
									console.debugf(" - stage[%d].tcmod[%d].phase = %f\n", nstage, ntcmod, tcmod.phase);
								else
									warn(filename, tokenizer, "invalid phase for tcmod turb: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.frequency))
									console.debugf(" - stage[%d].tcmod[%d].frequency = %f\n", nstage, ntcmod, tcmod.frequency);
								else
									warn(filename, tokenizer, "invalid frequency for tcmod turb: %s");
								break;
							case STT_TRANSFORM:
								tcmod.method = TCMOD_TRANSFORM;
								console.debugf(" - stage[%d].tcmod[%d].method = TCMOD_TRANSFORM\n", nstage, ntcmod);
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.m00))
									console.debugf(" - stage[%d].tcmod[%d].m00 = %f\n", nstage, ntcmod, tcmod.m00);
								else
									warn(filename, tokenizer, "invalid m00 for tcmod transform: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.m01))
									console.debugf(" - stage[%d].tcmod[%d].m01 = %f\n", nstage, ntcmod, tcmod.m01);
								else
									warn(filename, tokenizer, "invalid m01 for tcmod transform: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.m10))
									console.debugf(" - stage[%d].tcmod[%d].m10 = %f\n", nstage, ntcmod, tcmod.m10);
								else
									warn(filename, tokenizer, "invalid m10 for tcmod transform: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.m11))
									console.debugf(" - stage[%d].tcmod[%d].m11 = %f\n", nstage, ntcmod, tcmod.m11);
								else
									warn(filename, tokenizer, "invalid m11 for tcmod transform: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.t0))
									console.debugf(" - stage[%d].tcmod[%d].t0 = %f\n", nstage, ntcmod, tcmod.t0);
								else
									warn(filename, tokenizer, "invalid t0 for tcmod transform: %s");
								next_token_type(token, type, tokenizer);
								if (u_strtof(token, tcmod.t1))
									console.debugf(" - stage[%d].tcmod[%d].t1 = %f\n", nstage, ntcmod, tcmod.t1);
								else
									warn(filename, tokenizer, "invalid t1 for tcmod transform: %s");
								break;
							default:
								warn(filename, tokenizer, "unrecognized tcmod method: %s");
							}
							stage->tcmods.append(tcmod);
							++ntcmod;
							break;
						case STT_BLENDFUNC:
							read_destblend = true;
							switch (next_token_type(token, type, tokenizer)) {
							case STT_ADD:
								stage->srcblend = D3DBLEND_ONE;
								stage->destblend = D3DBLEND_ONE;
								read_destblend = false;
								break;
							case STT_BLEND:
								stage->srcblend = D3DBLEND_SRCALPHA;
								stage->destblend = D3DBLEND_INVSRCALPHA;
								read_destblend = false;
								break;
							case STT_FILTER:
								stage->srcblend = D3DBLEND_DESTCOLOR;
								stage->destblend = D3DBLEND_ZERO;
								read_destblend = false;
								break;
							case STT_GL_DST_COLOR:
								stage->srcblend = D3DBLEND_DESTCOLOR;
								break;
							case STT_GL_ZERO:
								stage->srcblend = D3DBLEND_ZERO;
								break;
							case STT_GL_ONE:
								stage->srcblend = D3DBLEND_ONE;
								break;
							case STT_GL_SRC_ALPHA:
								stage->srcblend = D3DBLEND_SRCALPHA;
								break;
							case STT_GL_ONE_MINUS_SRC_ALPHA:
								stage->srcblend = D3DBLEND_INVSRCALPHA;
								break;
							case STT_GL_ONE_MINUS_DST_COLOR:
								stage->srcblend = D3DBLEND_INVDESTCOLOR;
								break;
							default:
								warn(filename, tokenizer, "unrecognized blendfunc: %s");
								read_destblend = false;
							};
							if (read_destblend) {
								switch(next_token_type(token, type, tokenizer)) {
								case STT_GL_SRC_COLOR:
									stage->destblend = D3DBLEND_SRCCOLOR;
									break;
								case STT_GL_ZERO:
									stage->destblend = D3DBLEND_ZERO;
									break;
								case STT_GL_ONE:
									stage->destblend = D3DBLEND_ONE;
									break;
								case STT_GL_SRC_ALPHA:
									stage->destblend = D3DBLEND_SRCALPHA;
									break;
								case STT_GL_ONE_MINUS_DST_ALPHA:
									stage->destblend = D3DBLEND_INVDESTALPHA;
									break;
								case STT_GL_ONE_MINUS_SRC_ALPHA:
									stage->destblend = D3DBLEND_INVSRCALPHA;
									break;
								case STT_GL_ONE_MINUS_SRC_COLOR:
									stage->destblend = D3DBLEND_INVSRCCOLOR;
									break;
								default:
									warn(filename, tokenizer, "unrecognized destination blend: %s");
								}
							}
							console.debugf(" - stage[%d].srcblend = %s\n", nstage, D3DBLEND_to_str(stage->srcblend));
							console.debugf(" - stage[%d].destblend = %s\n", nstage, D3DBLEND_to_str(stage->destblend));
							break;
						default:
							warn(filename, tokenizer, "unrecognized token: %s");
						}
					}
					if (nstage != 0 && stage->srcblend == D3DBLEND_ONE && stage->destblend == D3DBLEND_ZERO)
						console.debugf(" - stage[%d] has a blend func that disables it\n", nstage);
					shader->stages.append(stage);
					stage = 0;
					++nstage;
					break;
				case STT_NOMIPMAPS:
					shader->flags |= SHADER_NOMIPMAPS;
					console.debug(" - flag SHADER_NOMIPMAPS set\n");
					break;
				case STT_NOPICMIP:
					shader->flags |= SHADER_NOPICMIP;
					console.debug(" - flag SHADER_NOPICMIP set\n");
					break;
				case STT_LIGHT:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: light value: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as light: %s\n", token);
					break;
				case STT_FOGPARMS:
					if (peek_token_type(token, type, tokenizer) == STT_LEFT_BRACKET)
						token = tokenizer.next_token();
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: fogparms.r: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as fogparms.r: %s\n", token);
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: fogparms.g: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as fogparms.g: %s\n", token);
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: fogparms.g: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as fogparms.b: %s\n", token);
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: fogparms.b: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as fogparms.distance: %s\n", token);
					if (peek_token_type(token, type, tokenizer) == STT_RIGHT_BRACKET)
						token = tokenizer.next_token();
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: fogparms.distance: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as fogparms.distance: %s\n", token);
					break;
				case STT_SKYPARMS:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN || type == STT_DASH)
						console.debugf(" - ignoring: skyparms.farbox: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as skyparms.farbox: %s\n", token);
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN || type == STT_DASH)
						console.debugf(" - ignoring: skyparms.cloudheight: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as skyparms.cloudheight: %s\n", token);
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN || type == STT_DASH)
						console.debugf(" - ignoring: skyparms.nearbox: %s\n", token);
					else
						console.debugf(" - shader parse error: keyword as skyparms.nearbox: %s\n", token);
					break;
				case STT_CULL:
					switch (next_token_type(token, type, tokenizer)) {
					case STT_FRONT:
						shader->cull = D3DCULL_CCW;
						console.debugf(" - cull = D3DCULL_CCW\n");
						break;
					case STT_BACK:
						shader->cull = D3DCULL_CW;
						console.debugf(" - cull = D3DCULL_CW\n");
						break;
					case STT_DISABLE:
					case STT_NONE:
						shader->cull = D3DCULL_NONE;
						console.debugf(" - cull = D3DCULL_NONE\n");
						break;
					default:
						warn(filename, tokenizer, "unrecognized cull: %s");
					};
					break;
				case STT_SORT:
					switch (next_token_type(token, type, tokenizer)) {
					case STT_PORTAL:
						shader->sort = SORT_PORTAL;
						console.debugf(" - sort = SORT_PORTAL\n");
						break;
					case STT_SKY:
						shader->sort = SORT_SKY;
						console.debugf(" - sort = SORT_SKY\n");
						break;
					case STT_OPAQUE:
						shader->sort = SORT_OPAQUE;
						console.debugf(" - sort = SORT_OPAQUE\n");
						break;
					case STT_BANNER:
						shader->sort = SORT_BANNER;
						console.debugf(" - sort = SORT_BANNER\n");
						break;
					case STT_UNDERWATER:
						shader->sort = SORT_UNDERWATER;
						console.debugf(" - sort = SORT_UNDERWATER\n");
						break;
					case STT_ADDITIVE:
						shader->sort = SORT_ADDITIVE;
						console.debugf(" - sort = SORT_ADDITIVE\n");
						break;
					case STT_NEAREST:
						shader->sort = SORT_NEAREST;
						console.debugf(" - sort = SORT_NEAREST\n");
						break;
					default:
						warn(filename, tokenizer, "unrecognized sort: %s");
					};
					break;
				case STT_DEFORMVERTEXES:
					switch (next_token_type(token, type, tokenizer)) {
					case STT_WAVE:
						console.debugf(" - vdeforms[%d].method = VDM_WAVE\n", nvdeform);
						vdeform.method = VDM_WAVE;
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.div))
							console.debugf(" - vdeforms[%d].div = %f\n", nvdeform, vdeform.div);
						else
							warn(filename, tokenizer, "invalid div for deformvertexes: %s");
						switch (next_token_type(token, type, tokenizer)) {
						case STT_SIN:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SIN\n", nvdeform);
							break;
						case STT_TRIANGLE:
							vdeform.wavefunc = WF_TRIANGLE;
							console.debugf(" - vdeforms[%d].wavefunc = WF_TRIANGLE\n", nvdeform);
							break;
						case STT_SQUARE:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SQUARE\n", nvdeform);
							break;
						case STT_SAWTOOTH:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SAWTOOTH\n", nvdeform);
							break;
						case STT_INVERSESAWTOOTH:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_INVERSESAWTOOTH\n", nvdeform);
							break;
						case STT_NOISE:
							vdeform.wavefunc = WF_NOISE;
							console.debugf(" - vdeforms[%d].wavefunc = WF_NOISE\n", nvdeform);
							break;
						default:
							warn(filename, tokenizer, "unrecognized wavefunc for deformvertexes: %s");
						};
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.base))
							console.debugf(" - vdeforms[%d].base = %f\n", nvdeform, vdeform.base);
						else
							warn(filename, tokenizer, "invalid base for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.amplitude))
							console.debugf(" - vdeforms[%d].amplitude = %f\n", nvdeform, vdeform.amplitude);
						else
							warn(filename, tokenizer, "invalid amplitude for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.phase))
							console.debugf(" - vdeforms[%d].phase = %f\n", nvdeform, vdeform.phase);
						else
							warn(filename, tokenizer, "invalid phase for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.frequency))
							console.debugf(" - vdeforms[%d].frequency = %f\n", nvdeform, vdeform.frequency);
						else
							warn(filename, tokenizer, "invalid frequency for deformvertexes: %s");
						break;
					case STT_NORMAL:
						console.debugf(" - vdeforms[%d].method = VDM_NORMAL\n", nvdeform);
						vdeform.method = VDM_NORMAL;
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.amplitude))
							console.debugf(" - vdeforms[%d].amplitude = %f\n", nvdeform, vdeform.amplitude);
						else
							warn(filename, tokenizer, "invalid amplitude for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.frequency))
							console.debugf(" - vdeforms[%d].frequency = %f\n", nvdeform, vdeform.frequency);
						else
							warn(filename, tokenizer, "invalid frequency for deformvertexes: %s");
						break;
					case STT_BULGE:
						console.debugf(" - vdeforms[%d].method = VDM_BULGE\n", nvdeform);
						vdeform.method = VDM_BULGE;
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.width))
							console.debugf(" - vdeforms[%d].width = %f\n", nvdeform, vdeform.width);
						else
							warn(filename, tokenizer, "invalid width for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.height))
							console.debugf(" - vdeforms[%d].height = %f\n", nvdeform, vdeform.height);
						else
							warn(filename, tokenizer, "invalid height for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.speed))
							console.debugf(" - vdeforms[%d].speed = %f\n", nvdeform, vdeform.speed);
						else
							warn(filename, tokenizer, "invalid speed for deformvertexes: %s");
						break;
					case STT_MOVE:
						console.debugf(" - vdeforms[%d].method = VDM_MOVE\n", nvdeform);
						vdeform.method = VDM_MOVE;
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.x))
							console.debugf(" - vdeforms[%d].x = %f\n", nvdeform, vdeform.x);
						else
							warn(filename, tokenizer, "invalid x for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.y))
							console.debugf(" - vdeforms[%d].y = %f\n", nvdeform, vdeform.y);
						else
							warn(filename, tokenizer, "invalid y for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.z))
							console.debugf(" - vdeforms[%d].z = %f\n", nvdeform, vdeform.z);
						else
							warn(filename, tokenizer, "warning: invalid z for deformvertexes: %s");
						switch (next_token_type(token, type, tokenizer)) {
						case STT_SIN:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SIN\n", nvdeform);
							break;
						case STT_TRIANGLE:
							vdeform.wavefunc = WF_TRIANGLE;
							console.debugf(" - vdeforms[%d].wavefunc = WF_TRIANGLE\n", nvdeform);
							break;
						case STT_SQUARE:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SQUARE\n", nvdeform);
							break;
						case STT_SAWTOOTH:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_SAWTOOTH\n", nvdeform);
							break;
						case STT_INVERSESAWTOOTH:
							vdeform.wavefunc = WF_SIN;
							console.debugf(" - vdeforms[%d].wavefunc = WF_INVERSESAWTOOTH\n", nvdeform);
							break;
						case STT_NOISE:
							vdeform.wavefunc = WF_NOISE;
							console.debugf(" - vdeforms[%d].wavefunc = WF_NOISE\n", nvdeform);
							break;
						default:
							warn(filename, tokenizer, "unrecognized wavefunc for deformvertexes: %s");
						};
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.base))
							console.debugf(" - vdeforms[%d].base = %f\n", nvdeform, vdeform.base);
						else
							warn(filename, tokenizer, "invalid base for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.amplitude))
							console.debugf(" - vdeforms[%d].amplitude = %f\n", nvdeform, vdeform.amplitude);
						else
							warn(filename, tokenizer, "invalid amplitude for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.phase))
							console.debugf(" - vdeforms[%d].phase = %f\n", nvdeform, vdeform.phase);
						else
							warn(filename, tokenizer, "invalid phase for deformvertexes: %s");
						next_token_type(token, type, tokenizer);
						if (u_strtof(token, vdeform.frequency))
							console.debugf(" - vdeforms[%d].frequency = %f\n", nvdeform, vdeform.frequency);
						else
							warn(filename, tokenizer, "invalid frequency for deformvertexes: %s");
						break;
					case STT_AUTOSPRITE:
						console.debugf(" - vdeforms[%d].method = VDM_AUTOSPRITE\n", nvdeform);
						vdeform.method = VDM_AUTOSPRITE;
						break;
					case STT_AUTOSPRITE2:
						console.debugf(" - vdeforms[%d].method = VDM_AUTOSPRITE2\n", nvdeform);
						vdeform.method = VDM_AUTOSPRITE2;
						break;
					default:
						warn(filename, tokenizer, "unrecognized method for deformvertexes: %s");
					};
					shader->vdeforms.append(vdeform);
					nvdeform++;
					break;
				case STT_SURFACEPARM:
					switch (next_token_type(token, type, tokenizer)) {
					case STT_ALPHASHADOW:
					case STT_AREAPORTAL:
					case STT_CLUSTERPORTAL:
					case STT_DONOTENTER:
					case STT_FLESH:
					case STT_FOG:
					case STT_LAVA:
					case STT_METALSTEPS:
					case STT_NODAMAGE:
					case STT_NODLIGHT:
					case STT_NODRAW:
					case STT_NODROP:
					case STT_NOIMPACT:
					case STT_NOMARKS:
					case STT_NOLIGHTMAP:
					case STT_NOSTEPS:
					case STT_NONSOLID:
					case STT_ORIGIN:
					case STT_PLAYERCLIP:
					case STT_SKY:
					case STT_SLICK:
					case STT_SLIME:
					case STT_STRUCTURAL:
					case STT_TRANS:
					case STT_WATER:
						console.debugf(" - ignoring: surfaceparm %s\n", token);
						break;
					default:
						console.debugf(" - unrecognized surfaceparm: %s\n", token);
					}
					break;
				case STT_Q3MAP_BACKSHADER:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: q3map_backshader: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as q3map_backshader: %s");
					break;
				case STT_Q3MAP_GLOBALTEXTURE:
					console.debugf(" - ignoring: q3map_globaltexture: %s\n", token);
					break;
				case STT_Q3MAP_SUN:
					for (i = 0; i < 6; i++)
						if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
							console.debugf(" - ignoring: q3map_sun param %d: %s\n", i, token);
						else
							warn(filename, tokenizer, "keyword as q3map_sun param: %s");
					break;
				case STT_Q3MAP_SURFACELIGHT:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: q3map_surfacelight: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as q3map_surfacelight: %s");
					break;
				case STT_Q3MAP_TESSSIZE:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: tesssize: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as q3map_tesssize: %s");
					break;
				case STT_Q3MAP_LIGHTIMAGE:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: q3map_lightimage: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as q3map_lightimage: %s");
					break;
				case STT_Q3MAP_LIGHTSUBDIVIDE:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: q3map_lightsubdivide: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as q3map_lightsubdivide: %s");
					break;
				case STT_QER_EDITORIMAGE:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: qer_editorimage: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as qer_editorimage: %s");
					break;
				case STT_QER_NOCARVE:
					console.debug(" - ignoring: qer_nocarve\n");
					break;
				case STT_QER_TRANS:
					if (next_token_type(token, type, tokenizer) == STT_UNKNOWN)
						console.debugf(" - ignoring: qer_trans: %s\n", token);
					else
						warn(filename, tokenizer, "keyword as qer_trans: %s");
					break;
				default:
					warn(filename, tokenizer, "unrecognized token: %s");
				}
			}
			shaders.append(shader);
			ref_counts.append(0);
		} else {
			warn(filename, tokenizer, "shader name not followed by opening brace");
			delete shader;
		}
	}
	int max_stages = 0;
	int max_vdeforms = 0;
	int max_tcmods = 0;
	int max_textures = 0;
	for (int i = 0; i < num_shaders(); ++i) {
		if (max_vdeforms < shaders[i]->vdeforms.size())
			max_vdeforms = shaders[i]->vdeforms.size();
		if (max_stages < shaders[i]->stages.size())
			max_stages = shaders[i]->stages.size();
		for (int j = 0; j < shaders[i]->stages.size(); ++j) {
			if (max_tcmods < shaders[i]->stages[j]->tcmods.size())
				max_tcmods = shaders[i]->stages[j]->tcmods.size();
			if (max_tcmods < shaders[i]->stages[j]->map_names.size())
				max_textures = shaders[i]->stages[j]->map_names.size();
		}
	}
	console.printf("Max Shader Stages: %d\n", max_stages);
	console.printf("Max Vertex Deforms: %d\n", max_vdeforms);
	console.printf("Max Texture Co-ord Mods: %d\n", max_tcmods);
	console.printf("Max Textures: %d\n", max_textures);

	} catch (const char* error) {
		console.print(error);
	}
	delete file;
}
