
#version 330

#ifdef VERTEX_SHADER
out vec2 vertex_texcoord;

void main(void)
{
    vec3 positions[3]= vec3[3]( vec3(-1, 1, -1), vec3( -1, -3, -1), vec3( 3,  1, -1) );
    
    gl_Position= vec4(positions[gl_VertexID], 1.0);
    vertex_texcoord= positions[gl_VertexID].xy / 2.0 + 0.5;
}

#endif


#ifdef FRAGMENT_SHADER
uniform float compression= 1;
uniform float saturation= 1;
uniform vec4 channels= vec4(1, 1, 1, 1);
uniform float gray= 0;
uniform float difference= 0;

uniform int split;
uniform sampler2D image;
uniform sampler2D image_next;

uniform float zoom;
uniform vec2 center;

uniform int graph;
uniform vec2 line;

in vec2 vertex_texcoord;
out vec4 fragment_color;

void main(void)
{
    const vec3 rgby= vec3(0.3, 0.59, 0.11);
    float k1= 1.0 / pow(saturation, 1.0 / compression); // normalisation : saturation == blanc
    
    // zoom
    vec2 zoom_texcoord= vertex_texcoord;
    if(zoom != 1)
        zoom_texcoord= (vertex_texcoord - center) / zoom + center;
    
    // split
    vec4 color= texture(image, zoom_texcoord);
    vec4 color_next= texture(image_next, zoom_texcoord);
    
    if(gl_FragCoord.x >= split)
    {
        if(difference == 0)
            color= color_next;
        else
            color= abs(color_next - color);
    }

    // nan
    if(any(isnan(color)) || any(isinf(color)))
    {
        fragment_color= vec4(1, 0, 1, 1);
        return;
    }
    
    // graph
    vec4 gcolor= texture(image, vec2(zoom_texcoord.x, line.x));
    vec4 gcolor_next= texture(image, vec2(zoom_texcoord.x + dFdx(zoom_texcoord.x), line.x));
    
    if(gl_FragCoord.x >= split)
    {
        if(difference == 0)
        {
            gcolor= texture(image_next, vec2(zoom_texcoord.x, line.x));
            gcolor_next= texture(image_next, vec2(zoom_texcoord.x + dFdx(zoom_texcoord.x), line.x));
        }
        else
        {
            gcolor= abs(texture(image_next, vec2(zoom_texcoord.x, line.x)) - gcolor);
            gcolor_next= abs(texture(image_next, vec2(zoom_texcoord.x + dFdx(zoom_texcoord.x), line.x)) - gcolor_next);
        }
    }
    
    // channels
    // applique la compression (gamma)
    if(channels != vec4(0, 0, 0, 1))
        color.rgb= channels.rgb * color.rgb;
    else
        // visualisation du canal alpha seul
        color.rgb= color.aaa;
    
    if(gray > 0.0)
    {
        float g= max(color.r, max(color.g, color.b));
        color.rgb= vec3(g, g, g);
    }
    
    float y= dot(color.rgb, rgby);  // normalisation de la couleur : (color / y) == teinte
    color= (color / y) * k1 * pow(y, 1.0 / compression);
    
    // graph
    if(graph > 0)
    {
        float gy= dot(gcolor.rgb, rgby);
        gcolor= (gcolor / gy) * k1 * pow(gy, 1.0 / compression);
        float gyn= dot(gcolor_next.rgb, rgby);
        gcolor_next= (gcolor_next / gyn) * k1 * pow(gyn, 1.0 / compression);
        
        // intervalle
        vec4 ymin= min(gcolor, gcolor_next) * 0.99;
        vec4 ymax= max(gcolor, gcolor_next);
        
        // intersection intervalle
        vec4 gline= vec4(0, 0, 0, 0);
        if((channels.r > 0) && (vertex_texcoord.y >= ymin.r) && (vertex_texcoord.y <= ymax.r))
            gline.r= 1.0;
        if((channels.g > 0) && (vertex_texcoord.y >= ymin.g) && (vertex_texcoord.y <= ymax.g))
            gline.g= 1.0;
        if((channels.b > 0) && (vertex_texcoord.y >= ymin.b) && (vertex_texcoord.y <= ymax.b))
            gline.b= 1.0;

        if(gline.r > 0 || gline.g > 0 || gline.b > 0)
            color= gline;
            
        // draw graph
        if(abs(gl_FragCoord.y - line.y) < 1)
            color= vec4(0.8, 0.5, 0, 1);
    }

    // draw split
    if(abs(gl_FragCoord.x - split) < 1)
        color= vec4(0.8, 0.5, 0, 1);
    
    fragment_color= vec4(clamp(color.rgb, vec3(0.0), vec3(1.0)), 1.0);
}

#endif
