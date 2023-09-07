
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

		m_robot_mesh = read_mesh("data/robot_smooth.obj");

		// etat openGL par defaut
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
		glClearDepth(1.f);                          // profondeur par defaut

		glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
		glEnable(GL_DEPTH_TEST);                    // activer le ztest

		//Lecture du shader
		m_shader_from_buffer = read_program("data/shaders_persos/shader_from_buffer.glsl");
		program_print_errors(m_shader_from_buffer);

		glUseProgram(m_shader_from_buffer);
		GLint light_position_location = glGetUniformLocation(m_shader_from_buffer, "light_position");
		glUniform3f(light_position_location, 2, 0, 10);

		GLuint diffuse_colors_location = glGetUniformLocation(m_shader_from_buffer, "diffuse_colors");
		std::vector<Color> diffuse_colors_buffer;
		for (const Material& mat : m_robot_mesh.materials().materials)
			diffuse_colors_buffer.push_back(mat.diffuse);
		glUniform4fv(diffuse_colors_location, m_robot_mesh.materials().materials.size(), (GLfloat*)diffuse_colors_buffer.data());

		//Creation du VAO
		glGenVertexArrays(1, &m_robot_vao);
		//Selection du VAO pour le configurer apres
		glBindVertexArray(m_robot_vao);

		//Creation du position buffer
		GLuint position_buffer;
		glGenBuffers(1, &position_buffer);
		//On selectionne le position buffer
		glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
		//On remplit le buffer selectionne (le position buffer)
		glBufferData(GL_ARRAY_BUFFER, m_robot_mesh.vertex_buffer_size(), m_robot_mesh.vertex_buffer(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut position du vertex shader "in vec3 position"
		GLint position_attribute = glGetAttribLocation(m_shader_from_buffer, "position");
		glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(position_attribute);





		//Creation du normal buffer
		GLuint normal_buffer;
		glGenBuffers(1, &normal_buffer);
		//On selectionne le normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		//On remplit le buffer selectionne (le normal buffer)
		glBufferData(GL_ARRAY_BUFFER, m_robot_mesh.normal_buffer_size(), m_robot_mesh.normal_buffer(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
		GLint normal_attribute = glGetAttribLocation(m_shader_from_buffer, "normal");
		glVertexAttribPointer(normal_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(normal_attribute);





		std::vector<unsigned int> material_index_buffer;
		for (unsigned int index : m_robot_mesh.material_indices())
		{
			material_index_buffer.push_back(index);
			material_index_buffer.push_back(index);
			material_index_buffer.push_back(index);
		}

		//Creation du buffer des materiaux
		GLuint material_index_buffer_id;
		glGenBuffers(1, &material_index_buffer_id);
		//On selectionne le normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, material_index_buffer_id);
		//On remplit le buffer selectionne (le material index buffer). On divise par 3 parce qu'on a un material par TRIANGLE, pas par vertex
		glBufferData(GL_ARRAY_BUFFER, m_robot_mesh.vertex_buffer_size(), material_index_buffer.data(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
		GLint material_index_attribute = glGetAttribLocation(m_shader_from_buffer, "material_index");
		glVertexAttribIPointer(material_index_attribute, 1, GL_UNSIGNED_INT, 0, 0);
		glEnableVertexAttribArray(material_index_attribute);



		//Nettoyage (on repositionne les buffers selectionnes sur les valeurs par defaut)
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return 0;   // pas d'erreur, sinon renvoyer -1
	}

	// destruction des objets de l'application
	int quit()
	{
		m_repere.release();
		return 0;   // pas d'erreur
	}

	// dessiner une nouvelle image
	int render()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw(m_repere, /* model */ Identity(), camera());
		
		//On selectionne notre shader
		glUseProgram(m_shader_from_buffer);

		//On update l'uniform mvpMatrix de notre shader
		Transform mvpMatrix = camera().projection() * camera().view() * Identity();
		GLuint mvpMatrixLocation = glGetUniformLocation(m_shader_from_buffer, "mvpMatrix");
		glUniformMatrix4fv(mvpMatrixLocation, 1, GL_TRUE, mvpMatrix.data());

		//On selectionne le vao du robot
		glBindVertexArray(m_robot_vao);
		//On draw le robot
		glDrawArrays(GL_TRIANGLES, 0, m_robot_mesh.vertex_count());

		return 1;
	}

protected:
	Mesh m_repere;
	Mesh m_robot_mesh;

	GLuint m_robot_vao;
	GLuint m_shader_from_buffer;
};


int main(int argc, char** argv)
{
	// il ne reste plus qu'a creer un objet application et la lancer 
	TP tp;
	tp.run();

	return 0;
}
