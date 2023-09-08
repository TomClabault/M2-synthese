
//! \file tuto7_camera.cpp reprise de tuto7.cpp mais en derivant AppCamera, avec gestion automatique d'une camera.


#include "wavefront.h"
#include "texture.h"

#include "app_camera.h"        // classe Application a deriver
#include "draw.h"        
#include "orbiter.h"
#include "uniforms.h"

#include "imgui.h"
#include "imgui_impl_sdl_gl3.h"

#include "application_settings.h"

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
	TP() : AppCamera(1280, 720) {}

	void update_ambient_uniforms()
	{
		GLuint use_ambient_location = glGetUniformLocation(m_custom_shader, "u_use_ambient");
		glUniform1i(use_ambient_location, m_application_settings.use_ambient);

		GLuint ambient_color_location = glGetUniformLocation(m_custom_shader, "u_ambient_color");
		glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);
	}

	void setup_light_position_uniform(const vec3& light_position)
	{
		glUseProgram(m_custom_shader);
		GLint light_position_location = glGetUniformLocation(m_custom_shader, "u_light_position");
		glUniform3f(light_position_location, light_position.x, light_position.y, light_position.z);
	}

	void setup_diffuse_color_uniform()
	{
		GLuint diffuse_colors_location = glGetUniformLocation(m_custom_shader, "u_diffuse_colors");
		std::vector<Color> diffuse_colors_buffer;
		for (const Material& mat : m_mesh.materials().materials)
			diffuse_colors_buffer.push_back(mat.diffuse);
		glUniform4fv(diffuse_colors_location, m_mesh.materials().materials.size(), (GLfloat*)diffuse_colors_buffer.data());
	}

	// creation des objets de l'application
	int init()
	{
		//Setting ImGUI up
		ImGui::CreateContext();

		m_imgui_io = ImGui::GetIO();
		ImGui_ImplSdlGL3_Init(m_window);

		//Positioning the camera to a default state
		m_camera.read_orbiter("TPs/from_scratch/start_camera.txt");


		//Creating a basic grid
		m_repere = make_grid(10);

		//Reading the mesh displayed
		m_mesh = read_mesh("data/robot.obj");

		// etat openGL par defaut
		glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
		glClearDepth(1.f);                          // profondeur par defaut

		glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
		glEnable(GL_DEPTH_TEST);                    // activer le ztest

		//Lecture du shader
		m_custom_shader = read_program("TPs/from_scratch/shader_custom.glsl");
		program_print_errors(m_custom_shader);
		
		setup_light_position_uniform(vec3(2, 0, 10));
		setup_diffuse_color_uniform();
		

		GLuint use_ambient_location = glGetUniformLocation(m_custom_shader, "u_use_ambient");
		glUniform1i(use_ambient_location, m_application_settings.use_ambient);

		GLuint ambient_color_location = glGetUniformLocation(m_custom_shader, "u_ambient_color");
		glUniform4f(ambient_color_location, m_application_settings.ambient_color.r, m_application_settings.ambient_color.g, m_application_settings.ambient_color.b, m_application_settings.ambient_color.a);

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
		glBufferData(GL_ARRAY_BUFFER, m_mesh.vertex_buffer_size(), m_mesh.vertex_buffer(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut position du vertex shader "in vec3 position"
		GLint position_attribute = glGetAttribLocation(m_custom_shader, "position");
		glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(position_attribute);





		//Creation du normal buffer
		GLuint normal_buffer;
		glGenBuffers(1, &normal_buffer);
		//On selectionne le normal buffer
		glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
		//On remplit le buffer selectionne (le normal buffer)
		glBufferData(GL_ARRAY_BUFFER, m_mesh.normal_buffer_size(), m_mesh.normal_buffer(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
		GLint normal_attribute = glGetAttribLocation(m_custom_shader, "normal");
		glVertexAttribPointer(normal_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(normal_attribute);





		std::vector<unsigned int> material_index_buffer;
		for (unsigned int index : m_mesh.material_indices())
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
		glBufferData(GL_ARRAY_BUFFER, m_mesh.vertex_buffer_size(), material_index_buffer.data(), GL_STATIC_DRAW);

		//On recupere l'id de l'attribut normal du vertex shader "in vec3 normal"
		GLint material_index_attribute = glGetAttribLocation(m_custom_shader, "material_index");
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

	void draw_lighting_window()
	{
		if(ImGui::Checkbox("Use ambient", &m_application_settings.use_ambient))
			update_ambient_uniforms();

		if (m_application_settings.use_ambient)
			if(ImGui::ColorPicker4("Ambient color", (float*)(&m_application_settings.ambient_color)))
				update_ambient_uniforms();
	}

	void draw_materials_window()
	{
		static ImGuiTableFlags materials_table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
			| ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersInnerH
			| ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersInnerV
			| ImGuiTableFlags_BordersOuter
			| ImGuiTableFlags_BordersInner;

		if (ImGui::BeginTable("MaterialsTable", /* columns */ 2, materials_table_flags))
		{
			//Headers of the table
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Diffuse color");
			ImGui::TableHeadersRow();

			for (int i = 0; i < m_mesh.materials().materials.size(); i++)
			{
				const Material& mat = m_mesh.materials().materials.at(i);
				std::string& material_name = m_mesh.materials().names.at(i);

				ImGui::TableNextRow();
				for (int column = 0; column < 2; column++)
				{
					ImGui::TableSetColumnIndex(column);

					if (column == 0)
						//Name of the material
						ImGui::TextUnformatted(material_name.c_str());
					else if (column == 1)
					{
						//Diffuse color

						//We're using color button here instead of ColorEdit only to be able to change the size of the color "square"
						//Because we still want to retain the color picker feature of the ColorEdit, we're opening a popup if the ColorButton
						//is clicked
						if (ImGui::ColorButton(std::string("DiffuseColor#" + material_name).c_str(), ImVec4(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b, mat.diffuse.a), 0, ImVec2(32, 32)))
							ImGui::OpenPopup(std::string("DiffuseColor#" + material_name + " picker").c_str());

						if (ImGui::BeginPopup(std::string("DiffuseColor#" + material_name + " picker").c_str()))
						{
							//Adding a color picker to the popup

							if (ImGui::ColorPicker4("##picker", (float*)&mat.diffuse, ImGuiColorEditFlags_None, NULL))
							{
								//If the user interacted with the color of the material
								//We're updating the uniform of the shader
								setup_diffuse_color_uniform();
							}
							ImGui::EndPopup();
						}
					}
				}
			}

			ImGui::EndTable();
		}
	}

	void draw_imgui()
	{
		ImGui_ImplSdlGL3_NewFrame(m_window);

		ImGui::ShowDemoWindow();

		ImGui::Begin("Lighting");
		draw_lighting_window();
		ImGui::End();

		ImGui::Begin("Materials");
		draw_materials_window();
		ImGui::End();


		ImGui::Render();
		ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
	}

	// dessiner une nouvelle image
	int render()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw(m_repere, /* model */ Identity(), camera());
		
		//On selectionne notre shader
		glUseProgram(m_custom_shader);

		//On update l'uniform mvpMatrix de notre shader
		Transform mvpMatrix = camera().projection() * camera().view() * Identity();
		GLuint mvpMatrixLocation = glGetUniformLocation(m_custom_shader, "mvpMatrix");
		glUniformMatrix4fv(mvpMatrixLocation, 1, GL_TRUE, mvpMatrix.data());

		//On selectionne le vao du robot
		glBindVertexArray(m_robot_vao);
		//On draw le robot
		glDrawArrays(GL_TRIANGLES, 0, m_mesh.vertex_count());




		////////// ImGUI //////////
		draw_imgui();
		////////// ImGUI //////////


		return 1;
	}

protected:
	Mesh m_repere;
	Mesh m_mesh;

	GLuint m_robot_vao;
	GLuint m_custom_shader;

	ImGuiIO m_imgui_io;

	ApplicationSettings m_application_settings;
};


int main(int argc, char** argv)
{
	// il ne reste plus qu'a creer un objet application et la lancer 
	TP tp;
	tp.run();

	ImGui_ImplSdlGL3_Shutdown();
	ImGui::DestroyContext();

	return 0;
}
