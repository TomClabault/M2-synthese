
//! \file tuto_cubemap.cpp reflets miroirs cubemap / envmap. 

#include <memory>

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "program.h"
#include "uniforms.h"
#include "draw.h"

#include "app_camera.h"        // classe Application a deriver

//! charge une image, decoupe les 6 faces et renvoie une texture cubemap.
GLuint read_cubemap( const int unit, const char *filename,  const GLenum texel_type = GL_RGBA )
{
    // les 6 faces sur une croix
    ImageData image= read_image_data(filename);
    if(image.pixels.empty()) 
        return 0;
    
    int w= image.width / 4;
    int h= image.height / 3;
    assert(w == h);
    
    GLenum data_format;
    GLenum data_type= GL_UNSIGNED_BYTE;
    if(image.channels == 3)
        data_format= GL_RGB;
    else // par defaut
        data_format= GL_RGBA;
    
    // creer la texture
    GLuint texture= 0;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    // creer les 6 faces
    // chaque face de la cubemap est un rectangle [image.width/4 x image.height/3] dans l'image originale 
    struct { int x, y; } faces[]= {
        {0, 1}, // X+
        {2, 1}, // X-
        {1, 2}, // Y+
        {1, 0}, // Y- 
        {1, 1}, // Z+
        {3, 1}, // Z-
    };
    
    for(int i= 0; i < 6; i++)
    {
        ImageData face= flipX(flipY(copy(image, faces[i].x*w, faces[i].y*h, w, h)));
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X +i, 0,
            texel_type, w, h, 0,
            data_format, data_type, face.data());
    }
    
    // parametres de filtrage
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);    
    
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    
    // filtrage "correct" sur les bords du cube...
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    //~ glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    printf("  cubemap faces %dx%d\n", w, h);
    
    return texture;
}
    
class TP : public AppCamera
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppCamera(1024, 640) {}
    
    // creation des objets de l'application
    int init( )
    {
        //~ m_objet= read_mesh("data/cube.obj");
        m_objet= read_mesh("data/bigguy.obj");
        // m_objet= read_mesh("data/ccbigguy.obj");
        
        m_texture= read_cubemap(0, "tutos/cubemap_debug_cross.png");
        // m_texture= read_cubemap(0, "canyon2.jpg");
        
        m_program_draw= read_program("tutos/draw_cubemap.glsl");
        program_print_errors(m_program_draw);
        m_program= read_program("tutos/cubemap.glsl");
        program_print_errors(m_program);
        
        // init camera
        Point pmin, pmax;
        m_objet.bounds(pmin, pmax);
        camera().lookat(pmin, pmax);
        
        // etat openGL par defaut
        glGenVertexArrays(1, &m_vao);
        
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LEQUAL);                     // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        
        return 0;   // ras, pas d'erreur
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        m_objet.release();
        release_program(m_program_draw);
        release_program(m_program);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteTextures(1, &m_texture);
        return 0;
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        Transform model= Identity();
        Transform view= camera().view();
        Transform projection= camera().projection();
        Transform viewport= camera().viewport();
        Transform mvp= projection * view * model;
        
        Transform viewInv= Inverse(view);
        Point camera_position= viewInv(Point(0, 0, 0));  // coordonnees de la camera, dans le repere camera... c'est l'origine
        
        // etape 1 : affiche l'objet, utilise la cubemap pour calculer les reflets
        glUseProgram(m_program);
        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "modelMatrix", model);
        program_uniform(m_program, "camera_position", camera_position);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
        program_uniform(m_program, "texture0", int(0));
        
        // dessine l'objet, les attributs position et normale sont necessaires a l'execution du shader.
        m_objet.draw(m_program, /* position */ true, /* texcoord */ false, /* normal */ true, /* color */ false, /* material */ false);
        
        // etape 2 : affiche la cube map
        // inverse de la composition des transformations repere monde vers repere image
        Transform inv= Inverse(viewport * projection * view);
        
        glUseProgram(m_program_draw);
        glBindVertexArray(m_vao);
        program_uniform(m_program_draw, "invMatrix", inv);
        program_uniform(m_program_draw, "camera_position", camera_position);
        
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
        program_uniform(m_program_draw, "texture0", int(0));
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        // nettoyage
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glUseProgram(0);
        glBindVertexArray(0);
        
        if(key_state('r'))
        {
            clear_key_state('r');
            
            reload_program(m_program_draw, "tutos/draw_cubemap.glsl");
            program_print_errors(m_program_draw);
            reload_program(m_program, "tutos/cubemap.glsl");
            program_print_errors(m_program);
        }
        
        if(key_state('s'))
        {
            clear_key_state('s');
            static int calls= 0;
            screenshot("cubemap_brdf", ++calls);
            printf("screenshot %d\n", calls);
        }
        
        return 1;
    }

protected:
    Mesh m_objet;
    GLuint m_texture;
    GLuint m_program_draw;
    GLuint m_program;
    GLuint m_vao;
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}
