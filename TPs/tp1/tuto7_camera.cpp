
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.


#include "wavefront.h"
#include "texture.h"

#include "app_camera.h"        // classe Application a deriver
#include "draw.h"        
#include "orbiter.h"
#include "uniforms.h"

const vec3 LIGHT_POSITION(2, 0, 11);

// utilitaire. creation d'une grille / repere.
Mesh make_grid(const int n = 10)
{
	Mesh grid = Mesh(GL_LINES);

	// grille
	grid.color(White());
	for (int x = 0; x < n; x++)
	{
		float px = float(x) - float(n) / 2 + .5f;
		grid.vertex(Point(px, 0, -float(n) / 2 + .5f));
		grid.vertex(Point(px, 0, float(n) / 2 - .5f));
	}

	for (int z = 0; z < n; z++)
	{
		float pz = float(z) - float(n) / 2 + .5f;
		grid.vertex(Point(-float(n) / 2 + .5f, 0, pz));
		grid.vertex(Point(float(n) / 2 - .5f, 0, pz));
	}

	// axes XYZ
	grid.color(Red());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(1, .1, 0));

	grid.color(Green());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(0, 1, 0));

	grid.color(Blue());
	grid.vertex(Point(0, .1, 0));
	grid.vertex(Point(0, .1, 1));

	glLineWidth(2);

	return grid;
}


class TP : public AppCamera
{
public:
	// constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
	TP() : AppCamera(1024, 640) {}

	// creation des objets de l'application
	int init()
	{
		// decrire un repere / grille 
		m_repere = make_grid(10);

		// charge un objet
		m_robot = read_mesh("data/robot.obj");
		//Extraction des couleurs du modele
		for (int i = 0; i < m_robot.materials().materials.size(); i++)
			m_robot_colors.push_back(m_robot.materials().materials.at(i).diffuse);

		// etat openGL par defaut
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre

		glClearDepth(1.f);                          // profondeur par defaut
		glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
		glEnable(GL_DEPTH_TEST);                    // activer le ztest


		Transform view = camera().view();
		Transform projection = camera().projection();
		m_vpMatrix = projection * view;

		m_shader_custom = read_program("data/shaders_persos/shader_obj_color.glsl");
		program_print_errors(m_shader_custom);

		return 0;   // pas d'erreur, sinon renvoyer -1
	}

	// destruction des objets de l'application
	int quit()
	{
		m_robot.release();
		m_repere.release();
		return 0;   // pas d'erreur
	}

	// dessiner une nouvelle image
	int render()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//On recalcule la nouvelle camera si l'utilisateur l'a deplace avec la souris par exemple
		Transform view = camera().view();
		Transform projection = camera().projection();
		m_vpMatrix = projection * view;

		draw(m_repere, /* model */ Identity(), camera());
		
		//Rouge
		glUseProgram(m_shader_custom);
		program_uniform(m_shader_custom, "light_position", LIGHT_POSITION);
		program_uniform(m_shader_custom, "robotMaterials", m_robot_colors);
		//On scale le bigguy en rajoutant la model matrix (le scale 0.1x du bigguy) a la matrice projection * view existante.
		//On obtient donc la matrice mvp complete puisqu'on a rajoute la matrice du model (scale 0.1x)
		program_uniform(m_shader_custom, "mvpMatrix", m_vpMatrix * Scale(0.3f));
		m_robot.draw(m_shader_custom, true, false, true, false, true);


		return 1;
	}

protected:
	Mesh m_robot;
	std::vector<Color> m_robot_colors;

	Mesh m_cube;
	Mesh m_repere;

	Transform m_vpMatrix;
	GLuint m_shader_custom;
};


int main(int argc, char** argv)
{
	// il ne reste plus qu'a creer un objet application et la lancer 
	TP tp;
	tp.run();

	return 0;
}
