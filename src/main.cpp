/*  by Alun Evans 2016 LaSalle (aevanss@salleurl.edu) */

//include some standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

//include OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
	GLuint texture_id = 0;
	vec3 position;
	vec3 scale; 
};

vector<bodie> bodies;

//global variables to help us do things
int g_ViewportWidth = 512; int g_ViewportHeight = 512; // Default window size, in pixels
double mouse_x, mouse_y; //variables storing mouse position
const vec3 g_backgroundColor(0.2f, 0.2f, 0.2f); // background colour - a GLM 3-component vector

GLuint g_simpleShader = 0;
GLuint g_phongShader = 0;

GLuint texture_skybox_id = 0; 

vec3 g_light_dir(100, 100, 100);

GLuint g_Vao = 0; //vao
GLuint g_NumTriangles = 0; //  Numbre of triangles we are painting.

vec3 eye(0, 0, 2), center(0.0, 0.0, 0.0), up(0, 1, 0);

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

	// Create the VAO where we store all geometry (stored in g_Vao)
	g_Vao = gl_createAndBindVAO();

	//create vertex buffer for positions, colors, and indices, and bind them to shader
	gl_createAndBindAttribute(&(shapes[0].mesh.positions[0]), shapes[0].mesh.positions.size() * sizeof(float), g_simpleShader, "a_vertex", 3);
	gl_createAndBindAttribute(&(shapes[0].mesh.normals[0]), shapes[0].mesh.normals.size() * sizeof(float), g_simpleShader, "a_normal", 3);
	gl_createAndBindAttribute(&(shapes[0].mesh.texcoords[0]), shapes[0].mesh.texcoords.size() * sizeof(float), g_simpleShader, "a_uv", 2);
	gl_createIndexBuffer(&(shapes[0].mesh.indices[0]), shapes[0].mesh.indices.size() * sizeof(unsigned int));

	//unbind everything
	gl_unbindVAO();

	//store number of triangles (use in draw())
	g_NumTriangles = shapes[0].mesh.indices.size() / 3;


	vector<string> names = { "Sun", "Mercury", "Venus", "Earth", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune" };
	vector<char*> textures = { "assets/sunmap.bmp", "assets/mercurymap.bmp", "assets/venusmap.bmp", "assets/earthmap1k.bmp", "assets/marsmap.bmp", "assets/jupitermap.bmp", "assets/saturnmap.bmp", "assets/uranusmap.bmp", "assets/mercurymap.bmp" };
	vector<string> type = { "sun", "planet", "planet", "planet", "planet", "planet", "planet", "planet", "planet" };

	for (int i = 0; i < names.size(); i++) {
		GLuint texture_id = 0;

		Image* image = loadBMP(textures[i]);

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

		bodie actualPlanet;
		actualPlanet.name = names[i];
		actualPlanet.position = vec3(0.0, 0.0, 0.0);
		actualPlanet.texture_id = texture_id;
		actualPlanet.type = type[i];

		bodies.push_back(actualPlanet);
	}

	Image* image = loadBMP("assets/milkyway.bmp");

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
}

// ------------------------------------------------------------------------------------------
// This function actually draws to screen and called non-stop, in a loop
// ------------------------------------------------------------------------------------------
void draw()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// activate shader
	glUseProgram(g_phongShader);

	mat4 projection_matrix = perspective(
		60.0f, // Field of view
		1.0f, // Aspect ratio
		0.1f, // near plane (distance from camera)
		50.0f // Far plane (distance from camera)
	);

	GLuint projection_loc = glGetUniformLocation(g_phongShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_phongShader, "u_view");
	mat4 view_matrix = glm::lookAt(
		eye, // the position of your camera, in world space
		center, // where you want to look at, in world space
		up // probably glm::vec3(0,1,0)
	);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_phongShader, "u_model");
	mat4 model = translate(mat4(1.0f), center);
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model)); 

	GLuint light_dir_loc = glGetUniformLocation(g_phongShader, "u_light_dir");
	glUniform3f(light_dir_loc, g_light_dir.x, g_light_dir.y, g_light_dir.z);

	GLuint light_color_loc = glGetUniformLocation(g_phongShader, "u_light_color");
	glUniform3f(light_color_loc, 1.0, 1.0, 1.0);

	GLuint eye_loc = glGetUniformLocation(g_phongShader, "u_eye");
	glUniform3f(eye_loc, eye.x, eye.y, eye.z);
	
	GLuint ambient_loc = glGetUniformLocation(g_phongShader, "u_ambient");
	glUniform3f(ambient_loc, 1.0, 1.0, 1.0);

	GLuint glossiness_loc = glGetUniformLocation(g_phongShader, "u_glossiness");
	glUniform1f(glossiness_loc, 50);

	GLuint u_texture = glGetUniformLocation(g_phongShader, "u_texture");
	glUniform1i(u_texture, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bodies[2].texture_id);

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

	mat4 projection_matrix = perspective(
		60.0f, // Field of view
		1.0f, // Aspect ratio
		0.1f, // near plane (distance from camera)
		50.0f // Far plane (distance from camera)
	);

	GLuint projection_loc = glGetUniformLocation(g_simpleShader, "u_projection");
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, glm::value_ptr(projection_matrix));

	GLuint view_loc = glGetUniformLocation(g_simpleShader, "u_view");
	mat4 view_matrix = glm::lookAt(
		eye, // the position of your camera, in world space
		center, // where you want to look at, in world space
		up // probably glm::vec3(0,1,0)
	);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view_matrix));

	GLuint model_loc = glGetUniformLocation(g_simpleShader, "u_model");
	mat4 trans = translate(mat4(1.0f), center);
	mat4 model = scale(trans, vec3(3.0f, 3.0f, 3.0f));
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, glm::value_ptr(model));

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
}

// ------------------------------------------------------------------------------------------
// This function is called every time you click the mouse
// ------------------------------------------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        cout << "Left mouse down at" << mouse_x << ", " << mouse_y << endl;
    }
}

int main(void)
{
	//setup window and boring stuff, defined in glfunctions.cpp
	GLFWwindow* window;
	if (!glfwInit())return -1;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(g_ViewportWidth, g_ViewportHeight, "Hello OpenGL!", NULL, NULL);
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//drawUniverse();
		draw();
        
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


