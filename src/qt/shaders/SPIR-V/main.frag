#version 440

layout(location = 0) in vec4 v_color;
layout(location = 1) in vec2 v_texcoord;

layout(location = 0) out vec4 o_color;

layout(binding = 1) uniform sampler2D tex;

void main()
{
  vec4 _color = v_color;

  if (v_texcoord.x > 0.0 || v_texcoord.y > 0.0)
  {
    vec4 _texColor = texture(tex, v_texcoord.xy);

    if (_texColor.a == 0.0) discard;

    _color *= _texColor;
  }

  o_color = _color;
}
