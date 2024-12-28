#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureY;
uniform sampler2D textureU;
uniform sampler2D textureV;

void main()
{
    float y = texture(textureY, TexCoord).r;
    float u = texture(textureU, TexCoord).r - 0.5;
    float v = texture(textureV, TexCoord).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;

    FragColor = vec4(r, g, b, 1.0);
}

