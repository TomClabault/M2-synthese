
//! \file tuto_mdi_count.cpp 

#include <chrono>

#include "mat.h"
#include "program.h"
#include "uniforms.h"

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"

#include "app.h"
#include "text.h"


// representation des parametres 
struct alignas(4) IndirectParam
{
    unsigned int vertex_count;
    unsigned int instance_count;
    unsigned int first_vertex;
    unsigned int first_instance;
};
// alignement GLSL correct...

struct alignas(16) Object
{
    Point pmin;
    unsigned int vertex_count;
    Point pmax;
    unsigned int vertex_base;
};
// alignement GLSL correct...

class TP : public App
{
public:
    TP( ) : App(1024, 640, 4,3) {}     // openGL version 4.3, ne marchera pas sur mac.
    
    int init( )
    {
        //  verifie l'existence des extensions
        if(!GLEW_ARB_indirect_parameters) 
            return -1;
        printf("GL_ARB_indirect_parameters ON\n");
        
        if(!GLEW_ARB_shader_draw_parameters)
            return -1;
        printf("GL_ARB_shader_draw_parameters ON\n");
        
        m_object= read_mesh("data/bigguy.obj");
        Point pmin, pmax;
        m_object.bounds(pmin, pmax);
        m_camera.lookat(pmin - Vector(200, 200,  0), pmax + Vector(200, 200, 0));
        
        // genere les parametres des draws et les transformations
        for(int y= -150; y <= 150; y++)
        for(int x= -150; x <= 150; x++)
        {
            m_multi_model.push_back( Translation(x *20, y *20, 0) );
            
            // calcule la bbox de chaque objet dans le repere du monde
            m_objects.push_back( {pmin + Vector(x *20, y *20, 0), unsigned(m_object.vertex_count()), pmax + Vector(x *20, y *20, 0), 0} );
        }
        // oui c'est la meme chose qu'un draw instancie, mais c'est juste pour comparer les 2 solutions...
        
        // transformations des objets
        glGenBuffers(1, &m_model_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_model_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Transform) * m_multi_model.size(), m_multi_model.data(), GL_DYNAMIC_DRAW);
        
        // objets a tester
        glGenBuffers(1, &m_object_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_object_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Object) * m_objects.size(), m_objects.data(), GL_DYNAMIC_DRAW);
        
        // re-indexation des objets visibles
        glGenBuffers(1, &m_remap_buffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_remap_buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int) * m_objects.size(), nullptr, GL_DYNAMIC_DRAW);
        
        // parametres du multi draw indirect
        glGenBuffers(1, &m_indirect_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectParam) * m_objects.size(), nullptr, GL_DYNAMIC_DRAW);   
        
        // nombre de draws de multi draw indirect count 
        glGenBuffers(1, &m_parameter_buffer);
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameter_buffer);
        glBufferData(GL_PARAMETER_BUFFER_ARB, sizeof(int), nullptr, GL_DYNAMIC_DRAW);
        
        // creation des vertex buffer, uniquement les positions
        m_vao= m_object.create_buffers(/* texcoord */ false, /* normal */ false, /* color */ false, /* material */ false);
        
        // shader program
        m_program_cull= read_program("tutos/M2/indirect_cull.glsl");    // tests de visibilite
        program_print_errors(m_program_cull);
        
        m_program= read_program("tutos/M2/indirect_remap.glsl");        // affichage des objets visibles
        program_print_errors(m_program);
        
        // affichage du temps cpu / gpu
        m_console= create_text();
        
        // mesure du temps gpu de glDraw
        glGenQueries(1, &m_time_query);
        
        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        
        return 0;   // ras, pas d'erreur
    }
    
    int quit( )
    {
        glDeleteQueries(1, &m_time_query);
        release_text(m_console);
        
        release_program(m_program);
        release_program(m_program_cull);
        
        m_object.release();
        
        glDeleteBuffers(1, &m_indirect_buffer);
        glDeleteBuffers(1, &m_parameter_buffer);
        glDeleteBuffers(1, &m_remap_buffer);
        glDeleteBuffers(1, &m_object_buffer);
        
        return 0;
    }
    
    int update( const float time, const float delta )
    {
        m_model= RotationY(time / 20);
        return 0;
    }
    
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // deplace la camera
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfonce
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfonce
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());
        
        // mesure le temps d'execution 
        glBeginQuery(GL_TIME_ELAPSED, m_time_query);    // pour le gpu
        std::chrono::high_resolution_clock::time_point cpu_start= std::chrono::high_resolution_clock::now();    // pour le cpu
        
        // etape 1: compute shader, tester l'inclusion des objets dans une boite
        glUseProgram(m_program_cull);
        
        // uniforms...
        program_uniform(m_program_cull, "bmin", Point(-60, -60, -10));
        program_uniform(m_program_cull, "bmax", Point(60, 60, 10));
        //~ program_uniform(m_program_cull, "bmin", Point(-120, -120, -10));
        //~ program_uniform(m_program_cull, "bmax", Point(120, 120, 10));
        
        // storage buffers...
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_object_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_remap_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_indirect_buffer);
        
        // compteur
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_parameter_buffer);
        // remet le compteur a zero
        unsigned int zero= 0;
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);
        // ou
        // glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int), &zero);
        
        // nombre de groupes de shaders
        int n= m_objects.size() / 256;
        if(m_objects.size() % 256)
            n= n +1;
        
        glDispatchCompute(n, 1, 1);
        
        // etape 2 : attendre le resultat
        glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        
        // etape 3 : afficher les objets visibles (resultat de l'etape 1) avec 1 seul appel a glMultiDrawArraysIndirectCount
        glBindVertexArray(m_vao);
        glUseProgram(m_program);
        
        // uniforms...
        program_uniform(m_program, "modelMatrix", m_model);
        program_uniform(m_program, "vpMatrix", m_camera.projection(window_width(), window_height(), 45) * m_camera.view());
        program_uniform(m_program, "viewMatrix", m_camera.view());
        
        // storage buffers...
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_model_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_remap_buffer);
        
        // parametres du multi draw...
        glBindBuffer(GL_PARAMETER_BUFFER_ARB, m_parameter_buffer);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirect_buffer);
        
        glMultiDrawArraysIndirectCountARB(m_object.primitives(), 0, 0, m_objects.size(), 0);
        
        // affiche le temps 
        glEndQuery(GL_TIME_ELAPSED);
        std::chrono::high_resolution_clock::time_point cpu_stop= std::chrono::high_resolution_clock::now();
        long long int cpu_time= std::chrono::duration_cast<std::chrono::nanoseconds>(cpu_stop - cpu_start).count();
        
        // recupere le resultat de la requete gpu
        GLint64 gpu_time= 0;
        glGetQueryObjecti64v(m_time_query, GL_QUERY_RESULT, &gpu_time);
        
        clear(m_console);
        printf(m_console, 0, 0, "cpu  %02dms %03dus", (int) (cpu_time / 1000000), (int) ((cpu_time / 1000) % 1000));
        printf(m_console, 0, 1, "gpu  %02dms %03dus", (int) (gpu_time / 1000000), (int) ((gpu_time / 1000) % 1000));
        
        draw(m_console, window_width(), window_height());
        
        printf("cpu    %02dms %03dus    ", (int) (cpu_time / 1000000), (int) ((cpu_time / 1000) % 1000));
        printf("gpu    %02dms %03dus\n", (int) (gpu_time / 1000000), (int) ((gpu_time / 1000) % 1000));
        
        return 1;
    }
    
protected:
    GLuint m_parameter_buffer;
    GLuint m_indirect_buffer;
    GLuint m_model_buffer;
    GLuint m_object_buffer;
    GLuint m_remap_buffer;
    
    GLuint m_vao;
    GLuint m_program;
    GLuint m_program_cull;

    GLuint m_time_query;
    Text m_console;

    Transform m_model;
    Mesh m_object;
    Orbiter m_camera;
    
    std::vector<Transform> m_multi_model;
    std::vector<Object> m_objects;

};


int main( int argc, char **argv )
{
    TP tp;
    tp.run();
    
    return 0;
}
