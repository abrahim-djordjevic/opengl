#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h> // 	need to include glew before glfw or project won't build
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "loadShader.cpp"
#include "controls.cpp"
#include <glm/gtc/matrix_transform.hpp>
#include <time.h>
#include "point.cpp"

int HEIGHT = 100;
int WIDTH = 100;

GLFWwindow* window;
using namespace glm;

typedef struct {
	float x, y;
} vector2;

vector2 randomGradient(int ix, int iy) {
	// No precomputed gradients mean this works for any number of grid coordinates
	const unsigned w = 8 * sizeof(unsigned);
	const unsigned s = w / 2;
	unsigned a = ix, b = iy;
	a *= 3284157443;

	b ^= a << s | a >> w - s;
	b *= 1911520717;

	a ^= b << s | b >> w - s;
	a *= 2048419325;
	float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
	//std::cout << random << std::endl;

	// Create the vector from the angle
	vector2 v;
	v.x = sin(random);
	v.y = cos(random);

	return v;
}

float randomFloat()
{
	float f = (float)rand()/(float)RAND_MAX;
	return f;
}

void genColourArray(GLfloat colour_array[])
{
	for(int i = 0; i < (2 * HEIGHT * WIDTH * 3); i++)
	{
		colour_array[3 * i + 0] = randomFloat();
		colour_array[3 * i + 1] = randomFloat();
		colour_array[3 * i + 2] = randomFloat();
	}
}

int main(void)
{
	srand(time(NULL));
	//	initialise GLFW
	if(!glfwInit())
	{
		printf("Failed to initialise GLFW\n");
		return -1;
	}

	//	open a window and create OPENGL Context
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

	window = glfwCreateWindow(1024, 768, "Window 1", NULL, NULL);
	if(window == NULL)
	{
		printf("Failed to open window\n");
		glfwTerminate();
		return -1;
	}

	//	initialise GLEW
	glfwMakeContextCurrent(window);
	if(glewInit() != GLEW_OK)
	{
		printf("Failed to initialise GLEW\n");
		glfwTerminate();
		return -1;
	}

	//	get input and set background
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glClearColor(0.0f, 0.0f, 0.f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_CULL_FACE);
	//	vao
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	//vao data
	std::vector<Point> g_vertex_buffer_data;

	//ebo data
	std::vector<unsigned int> g_indices;

	for(int i = 0; i < WIDTH+1; i++)
	{
		for(int j = 0; j < WIDTH+1; j++)
		{
			vector2 grad = randomGradient(i, j);
			g_vertex_buffer_data.push_back({0.0f + (i), 1.0f * grad.y,  0.0f + j});
		}
	}

	for(int i = 0; i < g_vertex_buffer_data.size() - (WIDTH + 1); i++)
	{


		g_indices.push_back(i);
		g_indices.push_back(i + 1);
		g_indices.push_back(i + WIDTH + 1);

		g_indices.push_back(i + WIDTH + 1);
		g_indices.push_back(i + WIDTH);
		g_indices.push_back(i);

	}

	// vertex buffer
	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer_data.size() * sizeof(Point), &g_vertex_buffer_data[0], GL_STATIC_DRAW);

	// element buffer
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, g_indices.size() * sizeof(unsigned int), &g_indices[0], GL_STATIC_DRAW);

	GLuint programID = LoadShaders( "SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader" );
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");

	//colour buffer
	GLfloat g_color_buffer_data[2 * HEIGHT * WIDTH * 3 * 3];
	genColourArray(g_color_buffer_data);
	GLuint colorbuffer;
	glGenBuffers(1, &colorbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	// wireframe
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//	render loop
	do
	{
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if ( currentTime - lastTime >= 1.0 ){ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0/double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		//	clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// One color for each vertex. They were generated randomly.
		// Use our shader
		glUseProgram(programID);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		//genColourArray(g_color_buffer_data);

		glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,		// attr 0 no reason for 0 but must match shader
			3,		// size
			GL_FLOAT,	// type
			GL_FALSE,	// normalized
			0,		// stride
			(void*)0	// array buffer offset
		);

		// 2nd attribute buffer : colors
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
		glVertexAttribPointer(
			1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);


		//	draw the triangle
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glDrawElements(
			GL_TRIANGLES,
			g_indices.size(),
			GL_UNSIGNED_INT,
			(void*)0
		);
		//glDrawArrays(GL_POINTS, 0, WIDTH * HEIGHT);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		//	swap buffer
		glfwSwapBuffers(window);
		glfwPollEvents();


	}
	while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);
	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &colorbuffer);
	glDeleteBuffers(1, &elementbuffer);
	glDeleteProgram(programID);
	glDeleteVertexArrays(1, &VertexArrayID);
	//	terminate
	glfwTerminate();
	return 0;
}
