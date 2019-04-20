#version 450

out vec4 frag_color;
uniform vec2 spotlight_center;

void main(void)
{
    float c = (1.25 - .0015*distance(gl_FragCoord.xy, spotlight_center))*(.9 + .1*step(20., mod(gl_FragCoord.x + gl_FragCoord.y, 40.)));
    // frag_color = vec4(.75*c, .75*c, c, 1);
    frag_color = vec4(.75*c, .75*c, .75*c, 1);
}
