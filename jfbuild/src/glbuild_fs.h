/*#glbuild(ES2) #version 100
#glbuild(2)   #version 110
#glbuild(3)   #version 140

#ifdef GL_ES
#  define o_fragcolour gl_FragColor
#elif __VERSION__ < 140
#  define lowp
#  define mediump
#  define o_fragcolour gl_FragColor
#else
#  define varying in
#  define texture2D texture
out vec4 o_fragcolour;
#endif

varying mediump vec2 v_texcoord;

uniform sampler2D u_palette;
uniform sampler2D u_frame;

void main(void)
{
  lowp float pixelvalue;
  lowp vec3 palettevalue;
  pixelvalue = texture2D(u_frame, v_texcoord).r;
  palettevalue = texture2D(u_palette, vec2(pixelvalue, 0.5)).rgb;
  o_fragcolour = vec4(palettevalue, 1.0);
}
*/

const char *default_glbuild_fs_glsl =
R"(float4 main(
	float2 v_texcoord : TEXCOORD0,
	uniform sampler2D u_frame,
	uniform sampler2D u_palette)
{
	float pixelvalue = tex2D(u_frame, v_texcoord).r;
	float3 palettevalue = tex2D(u_palette, float2(pixelvalue, 0.5f)).rgb;
	return float4(palettevalue, 1.0f);
}
)";