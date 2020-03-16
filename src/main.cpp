/*  by Alun Evans 2016 LaSalle (aevanss@salleurl.edu) */

//include some standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <time.h> 

//include OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

//include some custom code files
#include "glfunctions.h" //include all OpenGL stuff
#include "Shader.h" // class to compile shaders

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "imageloader.h"

std::string basepath = "assets/";
std::string inputfile = basepath + "sphere.obj";
std::vector< tinyobj::shape_t > shapes;
std::vector< tinyobj::material_t > materials;
std::string err;
bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str(), basepath.c_str());

using namespace std;
using namespace glm;

struct bodie {
	string name;
	string type; 
	GLuint texture_id;
	GLuint texture_spec_id;
	GLuint normal_map_id;
	GLuint texture_trans_id;
	float clouds_rotation;
	vec3 position;
	vec3 scale; 
	float orbit_angle; 
	float rotacion;
	float orbit_speed;
	float dist_to_sun;
};

vector<bodie> bodies;

//global variables to help us do things
int g_ViewportWidth = 800; int g_ViewportHeight = 800; // Default window size, in pixels
double mouse_x, mouse_y; //variables storing mouse position
const vec3 g_backgroundColor(0.2f, 0.2f, 0.2f); // background colour - a GLM 3-component vector
int camera_mode = 0; 
float g_NumPlanets = 0; 
float dist_to_sun0 = 10; 

GLuint g_simpleShader = 0;
GLuint g_transparencyShader = 0;
GLuint g_phongShader = 0;
GLuint g_phongEarthShader = 0; 

GLuint texture_skybox_id = 0; 
GLuint texture_cloud_id = 0;

vec3 g_light_dir(0, 0, 0);

GLuint g_Vao = 0; //vao
GLuint g_NumTriangles = 0; //  Numbre of triangles we are painting.

vec3 eye(0, 0, 50), center(0.0, 0.0, 0.0), up(0, 1, 0);


mat4 projection_matrix = perspective(
	60.0f, // Field of view
	1.0f, // Aspect ratio
	0.1f, // near plane (distance from camera)
	600.0f // Far plane (distance from camera)
);

mat4 view_matrix = glm::lookAt(
	eye, // the position of your camera, in world space
	center, // where you want to look at, in world space
	up // probably glm::vec3(0,1,0)
);

// ------------------------------------------------------------------------------------------
// This function manually creates a square geometry (defined in the array vertices[])
// ------------------------------------------------------------------------------------------
void load()
{
	//test it loaded correctly
	if (!err.empty()) { // `err` may contain warning message.
		std::cerr << err << std::endl;
	}
	//print out number of meshes described in file
	std::cout << "# of shapes : " << shapes.size() << std::endl;

	//load the shader
	Shader simpleShader("src/shader.vert", "src/shader_simple.frag");
	g_simpleShader = simpleShader.program;

	Shader phongShader("src/shader.vert", "src/shader_phong.frag");
	g_phongShader = phongShader.program;

	Shader phongEarthShader("src/shader.vert", "src/shader_phong_earth.frag");
	g_phongEarthShader = phongEarthShader.program;

	Shader transparencyShader("src/shader.vert", "src/shader_transparency.frag");
	g_transparencyShader = transparencyShader.program;

	// Create the VAO where we store all geometry (stored in g_Vao)
	g_Vao = gl_createAndBindVAO();

	//create vertex buffer for positions, colors, and indices, and bind them to shader
	gl_createAndBindAttribute(&(shapes[0].mesh.positions[0]), shapes[0].mesh.positions.size() * sizeof(float), g_simpleShader, "a_vertex", 3);
	gl_createAndBindAttribute(&(shapes[0].mesh.normals[0]), shapes[0].mesh.normals.size() * sizeof(float), g_simpleShader, "a_normal", 3);
	gl_createAndBindAttribute(&(shapes[0].mesh.normals[0]), shapes[0].mesh.normals.size() * sizeof(float), g_phongShader, "a_normal", 3);
	gl_createAndBindAttribute(&(shapes[0].mesh.texcoords[0]), shapes[0].mesh.texcoords.size() * sizeof(float), g_simpleShader, "a_uv", 2);
	gl_createIndexBuffer(&(shapes[0].mesh.indices[0]), shapes[0].mesh.indices.size() * sizeof(unsigned int));

	//unbind everything
	gl_unbindVAO();

	//store number of triangles (use in draw())
	g_NumTriangles = shapes[0].mesh.indices.size() / 3;

	vector<float> scales = { 10, 0.38, 0.95, 1, 0.53,  1.12, 9.45, 4, 3.88 };
	vector<string> names = { "Sun", "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };
	vector<char*> textures = { "assets/textures/sunmap.bmp", "assets/textures/mercurymap.bmp", "assets/textures/venusmap.bmp", "assets/textures/earth/earthmap1k.bmp", "assets/textures/marsmap.bmp", "assets/textures/jupitermap.bmp", "assets/textures/saturnmap.bmp", "assets/textures/uranusmap.bmp", "assets/textures/mercurymap.bmp" };
	vector<string> type = { "sun", "planet", "planet", "planet", "planet", "planet", "planet", "planet", "planet" };

	g_NumPlanets = names.size();

	for (int i = 0; i < g_NumPlanets; i++) {
		bodie actualPlanet;
		actualPlanet.name = names[i];

		GLuint texture_id = 0;
		Image* image;

		if (actualPlanet.name == "Earth") {

			image = loadBMP("assets/textures/earth/earthspec.bmp");

			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D,
				0,
				GL_RGB,
				image->width,
				image->height,
				0,
				GL_RGB,
				GL_UNSIGNED_BYTE,
				image->pixels);

			actualPlanet.texture_spec_id = texture_id;

			image = loadBMP("assets/textures/earth/earthnormal.bmp");

			glGenTextures(1, &texture_id);
			glBindTexture(GL_TEXTURE_2D, texture_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D,
				0,
				GL_RGB,
				image->width,
				image->height,
				0,
				GL_RGB,
				GL_UNSIGNED_BYTE,
				image->pixels);

			actualPlanet.normal_map_id = texture_id;
		}
		else {
			actualPlanet.texture_spec_id = 0;
			actualPlanet.normal_map_id = 0;
		}

		image = loadBMP(textures[i]);

		glGenTextures(1, &texture_id);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB,
			image->width,
			image->height,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			image->pixels);


		if (i == 0) {
			actualPlanet.dist_to_sun = 0;
		}
		else {
			actualPlanet.dist_to_sun = 5 * i;
		}
		actualPlanet.position = vec3(dist_to_sun0 + actualPlanet.dist_to_sun, 0.0, 0.0);
		actualPlanet.scale = vec3(scales[i], scales[i], scales[i]);
		actualPlanet.texture_id = texture_id;
		actualPlanet.type = type[i];
		actualPlanet.clouds_rotation = 0;
		actualPlanet.orbit_speed = rand() % 10 + 1;
		actualPlanet.orbit_angle = rand() % 10 + 1;
		actualPlanet.rotacion = 0;
		bodies.push_back(actualPlanet);
	}

	Image* image = loadBMP("assets/textures/milkyway.bmp");

	glGenTextures(1, &texture_skybox_id);
	glBindTexture(GL_TEXTURE_2D, texture_skybox_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGB,
		image->width,
		image->height,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		image->pixels);

	image = loadBMP("assets/textures/earth/clouds.bmp");

	glGenTextures(1, &texture_cloud_id);
	glBindTexture(GL_TEXTURE_2D, texture_cloud_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGB,
		image->width,
		image->height,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		image->pixels);


}

void drawEarth(bodie Earth) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// activate shader
	glUseProgram(g_phongEarthShader);

	GLuint projection_loc = glGetUniformLocation(g_phongEarthShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_phongEarthShader, "u_view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_phongEarthShader, "u_model");
	mat4 model = translate(scale(mat4(1.0f), Earth.scale), Earth.position);
	model = glm::rotate(model, 10.0f, vec3(0.0f, 0.0f, 1.0f));
	model = glm::rotate(model, Earth.rotacion, vec3(0.3f, 1.0f, 0.0f));
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	GLuint u_normal_matrix = glGetUniformLocation(g_phongEarthShader, "u_normal_matrix");
	mat3 normal_matrix = inverseTranspose((mat3(model)));
	glUniformMatrix3fv(u_normal_matrix, 1, GL_FALSE, glm::value_ptr(normal_matrix));

	GLuint light_dir_loc = glGetUniformLocation(g_phongEarthShader, "u_light_dir");
	glUniform3f(light_dir_loc, g_light_dir.x - Earth.position.x, g_light_dir.y - Earth.position.y, g_light_dir.z - Earth.position.z);

	GLuint light_color_loc = glGetUniformLocation(g_phongEarthShader, "u_light_color");
	glUniform3f(light_color_loc, 1.0, 1.0, 1.0);

	GLuint eye_loc = glGetUniformLocation(g_phongEarthShader, "u_eye");
	glUniform3f(eye_loc, eye.x, eye.y, eye.z);

	GLuint ambient_loc = glGetUniformLocation(g_phongEarthShader, "u_ambient");
	glUniform3f(ambient_loc, 0.1, 0.1, 0.1);

	GLuint glossiness_loc = glGetUniformLocation(g_phongEarthShader, "u_glossiness");
	glUniform1f(glossiness_loc, 50);

	GLuint u_texture = glGetUniformLocation(g_phongEarthShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Earth.texture_id);

	GLuint u_texture_spec = glGetUniformLocation(g_phongEarthShader, "u_texture_spec");
	glUniform1i(u_texture_spec, 1);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, Earth.texture_spec_id);

	GLuint u_normal_map = glGetUniformLocation(g_phongEarthShader, "u_normal_map");
	glUniform1i(u_normal_map, 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, Earth.normal_map_id);

	//bind the geometry
	gl_bindVAO(g_Vao);

	// Draw to screen
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles, GL_UNSIGNED_INT, 0);



	//----------------------
	//--CLOUDS--------------
	//----------------------
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(g_transparencyShader);

	projection_loc = glGetUniformLocation(g_transparencyShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	view_loc = glGetUniformLocation(g_transparencyShader, "u_view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	model_loc = glGetUniformLocation(g_transparencyShader, "u_model");
	model = translate(mat4(1.0f), Earth.position);
	model = glm::scale(model, vec3(1.03f, 1.03f, 1.03f));
	model = glm::rotate(model, Earth.clouds_rotation, vec3(0.0f, 1.0f, 0.0f));
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	GLuint u_transparency_loc = glGetUniformLocation(g_transparencyShader, "u_transparency");
	glUniform1f(u_transparency_loc, 0.3f);

	u_texture = glGetUniformLocation(g_transparencyShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_cloud_id);

	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles, GL_UNSIGNED_INT, 0);
	glDisable(GL_BLEND);
}


void drawSun(vec3 position, GLuint texture_id, vec3 bodie_scale) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// activate shader
	glUseProgram(g_simpleShader);


	GLuint projection_loc = glGetUniformLocation(g_simpleShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_simpleShader, "u_view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_simpleShader, "u_model");
	mat4 trans = translate(mat4(1.0f), vec3(0.0,0.0,0.0));
	mat4 model = scale(trans, bodie_scale);
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	GLuint u_normal_matrix = glGetUniformLocation(g_simpleShader, "u_normal_matrix");
	mat3 normal_matrix = inverseTranspose((mat3(model)));
	glUniformMatrix3fv(u_normal_matrix, 1, GL_FALSE, glm::value_ptr(normal_matrix));

	GLuint u_texture = glGetUniformLocation(g_simpleShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	//bind the geometry
	gl_bindVAO(g_Vao);

	// Draw to screen
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles, GL_UNSIGNED_INT, 0);
}

void drawPlanet(vec3 position, GLuint texture_id, vec3 bodie_scale)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// activate shader
	glUseProgram(g_phongShader);

	GLuint projection_loc = glGetUniformLocation(g_phongShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_phongShader, "u_view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_phongShader, "u_model");
	mat4 model = translate(scale(mat4(1.0f), bodie_scale), position);
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model)); 

	GLuint u_normal_matrix = glGetUniformLocation(g_phongShader, "u_normal_matrix");
	mat3 normal_matrix = inverseTranspose((mat3(model)));
	glUniformMatrix3fv(u_normal_matrix, 1, GL_FALSE, glm::value_ptr (normal_matrix));

	GLuint light_dir_loc = glGetUniformLocation(g_phongShader, "u_light_dir");
	glUniform3f(light_dir_loc, g_light_dir.x - position.x, g_light_dir.y - position.y, g_light_dir.z - position.z);

	GLuint light_color_loc = glGetUniformLocation(g_phongShader, "u_light_color");
	glUniform3f(light_color_loc, 1.0, 1.0, 1.0);

	GLuint eye_loc = glGetUniformLocation(g_phongShader, "u_eye");
	glUniform3f(eye_loc, eye.x, eye.y, eye.z);
	
	GLuint ambient_loc = glGetUniformLocation(g_phongShader, "u_ambient");
	glUniform3f(ambient_loc, 0.1, 0.1, 0.1);

	GLuint glossiness_loc = glGetUniformLocation(g_phongShader, "u_glossiness");
	glUniform1f(glossiness_loc, 50);

	GLuint u_texture = glGetUniformLocation(g_phongShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	//bind the geometry
	gl_bindVAO(g_Vao);

	// Draw to screen
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles, GL_UNSIGNED_INT, 0);

}

void drawUniverse()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	// activate shader
	glUseProgram(g_simpleShader);


	GLuint projection_loc = glGetUniformLocation(g_simpleShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_simpleShader, "u_view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_simpleShader, "u_model");
	mat4 trans = translate(mat4(1.0f), eye);
	mat4 model = scale(trans, vec3(30.0f, 30.0f, 30.0f));
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

	GLuint u_normal_matrix = glGetUniformLocation(g_simpleShader, "u_normal_matrix");
	mat3 normal_matrix = inverseTranspose((mat3(model)));
	glUniformMatrix3fv(u_normal_matrix, 1, GL_FALSE, glm::value_ptr(normal_matrix));

	GLuint u_texture = glGetUniformLocation(g_simpleShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_skybox_id);

	//bind the geometry
	gl_bindVAO(g_Vao);

	// Draw to screen
	glDrawElements(GL_TRIANGLES, 3 * g_NumTriangles, GL_UNSIGNED_INT, 0);
}

// ------------------------------------------------------------------------------------------
// This function is called every time you press a screen
// ------------------------------------------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //quit
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
	//reload
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		load();
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) camera_mode = 0;
	if (key == GLFW_KEY_E && action == GLFW_PRESS) camera_mode = 1;
}

// ------------------------------------------------------------------------------------------
// This function is called every time you click the mouse
// ------------------------------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        cout << "Left mouse down at" << mouse_x << ", " << mouse_y << endl;
    }
}

void update() {
	for (int i = 1; i < g_NumPlanets; i++) {
		bodies[i].position = vec3((dist_to_sun0 + i)*bodies[i].dist_to_sun*cos(bodies[i].orbit_angle), 0, (dist_to_sun0 + i)*bodies[i].dist_to_sun*sin(bodies[i].orbit_angle));
		bodies[i].orbit_angle += bodies[i].orbit_speed * 0.001;
		bodies[i].clouds_rotation +=  0.1f;
		if (bodies[i].clouds_rotation > 360) bodies[i].clouds_rotation = 0;

		bodies[i].rotacion += -0.1f;
		if (bodies[i].rotacion > 360) bodies[i].rotacion = 0;
	}

	switch (camera_mode)
	{
	case 1: 
		eye = vec3(bodies[3].position.x, bodies[3].position.y, bodies[3].position.z + 5);
		center = bodies[3].position;
		
		break;

	default:
		eye = vec3(0, 0, 50);
		center = vec3(0.0, 0.0, 0.0);
		up = vec3 (0, 1, 0);
	}

	view_matrix = glm::lookAt(eye, center, up); 
}


int main(void)
{
	srand(time(NULL));

	//setup window and boring stuff, defined in glfunctions.cpp
	GLFWwindow* window;
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(g_ViewportWidth, g_ViewportHeight, "Solar System: Phong Test", NULL, NULL);
	if (!window) {glfwTerminate();	return -1;}
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();

	//input callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

	//load all the resources
	load();

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
		update();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawUniverse();
		drawSun(bodies[0].position, bodies[0].texture_id, bodies[0].scale);

		for (int i = 1; i < g_NumPlanets; i++) {
			if (bodies[i].name == "Earth") {
				drawEarth(bodies[i]);
			}
			else {
				drawPlanet(bodies[i].position, bodies[i].texture_id, bodies[i].scale);
			}
		}
        
        // Swap front and back buffers
        glfwSwapBuffers(window);
        
        // Poll for and process events
        glfwPollEvents();
        
        //mouse position must be tracked constantly (callbacks do not give accurate delta)
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
    }

    //terminate glfw and exit
    glfwTerminate();
    return 0;
}


