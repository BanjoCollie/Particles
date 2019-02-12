// Includes and defs --------------------------

// openGL functionality
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// shader helper
#include "shader.h"
// math
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
// image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


// Functions ---------------------------------

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int findUnusedParticle();
void sortParticles();

// Global variables ---------------------------

// window
const int SCR_WIDTH = 1280;
const int SCR_HEIGHT = 720;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = 0.0f, pitch = 0.0f;
bool firstMouse = true;
float lastX = 400, lastY = 300;

// time
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame
float elapsedTime = 0.0f; // Total time of simulation thus far

// particle spawner
glm::vec3 spawnerPos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 spawnerDim = glm::vec3(10.0f, 10.0f, 10.0f);
glm::vec3 spawnerDir = glm::vec3(0.0f, 0.0f, 0.0f);

// particles
struct Particle { // Struct for cpu - data is pushed into buffers for gpu to use
	glm::vec3 pos, vel;
	float r, g, b, a;
	float size;
	float life  = -1.0f; // Remaining life of the particle. < 0 = dead/unused.
	float cameraDist = -INFINITY;

	bool operator<(Particle& that) 
	{
		// Sort in reverse order : far particles drawn first.
		return this->cameraDist > that.cameraDist;
	}
};
const int maxParticles = 100000;
Particle particleContainer[maxParticles];
const int particleRate = 1000; // number of particles spawned each second
int lastUsedParticle = 0;
float particleLifetime = 120.0;
//glm::vec3 startVel = spawnerDir * 4.0f;
glm::vec3 startVel = glm::vec3(10.0f, 0.0f, 0.0f);
float minSize = 0.1;
float maxSize = 0.5;


// physics
//glm::vec3 grav = glm::vec3(0.0f, -9.80, 0.0f);
glm::vec3 grav = glm::vec3(0.0f, 0.0f, 0.0f);

// Display
const bool ADDITIVE = true;

int main()
{
	// Before loop starts ---------------------
	// glfw init
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "5611 HW1", NULL, NULL);
	glfwMakeContextCurrent(window);

	// register callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	//Register mouse movement callback
	glfwSetCursorPosCallback(window, mouse_callback);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize glad
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	if (ADDITIVE)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
	}
	else
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
	}

	// Things to render -----------------------

		// Particles

		// allocate mem for particle data
		static glm::vec4* particlePositionData = new glm::vec4[maxParticles];
		static glm::vec4* particleColorData = new glm::vec4[maxParticles];

		// VBO for particle position and size
		unsigned int particle_position_buffer;
		glGenBuffers(1, &particle_position_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * maxParticles, NULL, GL_STREAM_DRAW); // will add data in update
		// VBO for particle color
		unsigned int particle_color_buffer;
		glGenBuffers(1, &particle_color_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * maxParticles, NULL, GL_STREAM_DRAW); // will add data in update

		float particle_vertices[] = {
			-0.5f, -0.5f, 0.0f,
			 0.5f, -0.5f, 0.0f,
			-0.5f,  0.5f, 0.0f,
			 0.5f,  0.5f, 0.0f
		};
		// 1st create and bind VAO
		unsigned int particle_VAO;
		glGenVertexArrays(1, &particle_VAO);
		glBindVertexArray(particle_VAO);
		// VBO for particle vertex data
		unsigned int particle_vertex_buffer;
		glGenBuffers(1, &particle_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, particle_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(particle_vertices), particle_vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0); // pos
		glEnableVertexAttribArray(0);

		// VBO for particle position and size
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, particle_position_buffer);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glVertexAttribDivisor(1, 1);
		// VBO for particle color
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, particle_color_buffer);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glVertexAttribDivisor(2, 1);

		// particle shader
		Shader particleShader("particle.vert", "particle.frag");

		// particle texture
		unsigned int particleTexture;
		glGenTextures(1, &particleTexture);
		glBindTexture(GL_TEXTURE_2D, particleTexture);
		int width, height, nrChannels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load("particle.png", &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		stbi_image_free(data);
		particleShader.use();
		particleShader.setInt("texture1", 0);

	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop ----------------------------
	while (!glfwWindowShouldClose(window))
	{
		// Set deltaT
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		elapsedTime += deltaTime;

		// input
		processInput(window);

		// processing

		// determine # of particles to spawn
		int toSpawn = (int)(deltaTime*particleRate);
		toSpawn *= elapsedTime / 10.0f;

		float r = ((float)rand() / RAND_MAX);
		if (r < (deltaTime*particleRate - (float)toSpawn))
		{ // use non-integers to determine chance of spawning particle
			toSpawn++;
		}
		// spawn new particles
		if (elapsedTime < 100)
		{
			for (int i = 0; i < toSpawn; i++)
			{
				int index = findUnusedParticle();
				if (index >= 0)
				{
					Particle& p = particleContainer[index];

					p.life = particleLifetime;
					float offX = spawnerDim[0] * ((float)rand() / RAND_MAX);
					float posX = spawnerPos[0] - (spawnerDim[0] / 2.0f) + offX;
					float offY = spawnerDim[1] * ((float)rand() / RAND_MAX);
					float posY = spawnerPos[1] - (spawnerDim[1] / 2.0f) + offY;
					float offZ = spawnerDim[2] * ((float)rand() / RAND_MAX);
					float posZ = spawnerPos[2] - (spawnerDim[2] / 2.0f) + offZ;
					p.pos = glm::vec3(posX, posY, posZ);

					float rTheta = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
					float rPhi = ((float)rand() / RAND_MAX) * 4.0f - 2.0f;
					float rMag = ((float)rand() / RAND_MAX) * 2.0f;

					glm::mat4 rot = glm::mat4(1.0f);
					rot = glm::rotate(rot, glm::radians(rTheta), glm::vec3(0.0f, 1.0f, 0.0f));
					rot = glm::rotate(rot, glm::radians(rPhi), glm::vec3(0.0f, 0.0f, 1.0f));

					p.vel = glm::vec3(rot * glm::vec4(rMag * startVel, 1.0f));

					p.r = ((float)rand() / RAND_MAX);
					p.g = ((float)rand() / RAND_MAX);
					p.b = ((float)rand() / RAND_MAX);
					p.a = ((float)rand() / RAND_MAX);

					p.size = (maxSize - minSize) * ((float)rand() / RAND_MAX);

					//p.cameraDist = glm::dot(p.pos, cameraFront);
				}
			}
		}

		// simulate particles on cpu
		int numParticles = 0; // number of particles actually existing right now
		for (int i = 0; i < maxParticles; i++)
		{
			Particle& p = particleContainer[i];
			if (p.life > 0.0f)
			{ // For each currently alive particle
				//numParticles++;
				p.life -= deltaTime;
				if (p.life > 0.0f)
				{ // If the particle didn't die this frame
					if (elapsedTime < 88.0)
					{ //Spin and compress for 90 seconds
						// Integrate acceleration and velocity using eularian integration
						//p.vel += (-glm::normalize(p.pos) * 15.0f * deltaTime / glm::length(p.pos));
						//p.pos += p.vel * deltaTime;

						// Convert to polar
						float r = sqrtf(p.pos[0] * p.pos[0] + p.pos[2] * p.pos[2]);
						float theta = atan2f(p.pos[2], p.pos[0]);

						float tMod = 5.0f / (r*r + 1) * elapsedTime / 20.0;
						float rMod = 3.0f / (r + 1) * elapsedTime / 20.0;

						theta += deltaTime * tMod;
						r += deltaTime * -0.1f * exp(-0.1f * theta) * rMod;

						// Convert back to cartesian
						float x = r * cos(theta);
						float z = r * sin(theta);

						float yMod = 0.15 * elapsedTime / 15.0;

						float y = p.pos[1] + (r - p.pos[1]) * deltaTime * yMod;//y = p.pos[1] blended with z = r

						p.pos = glm::vec3(x, y, z);

						if (p.r < 1.0f)
						{
							p.r += deltaTime / 88.0f;
						}
						if (p.g < 1.0f)
						{
							p.g += deltaTime / 88.0f;
						}
						if (p.b < 0.4f)
						{
							p.b += deltaTime / 88.0f;
						}
						else
						{
							p.b -= deltaTime / 88.0f;
						}
					}
					else
					{
						if (p.life < elapsedTime*elapsedTime - 8070.0f)
						{
							p.pos += p.vel * deltaTime;
						}
					}


					p.cameraDist = glm::dot(p.pos, cameraFront);
				}
				else
				{
					// This particle just died
					p.cameraDist = -INFINITY;
				}
				numParticles++;
			}
		}

		std::cout << numParticles << std::endl;

		// Sort particles by distance to camera
		//sortParticles();

		// put info into arrays for gpu
		for (int i = 0; i < numParticles; i++)
		{
			Particle& p = particleContainer[i];
			if (p.life > 0.0f)
			{ // For each currently alive particle
				particlePositionData[i] = glm::vec4(p.pos, p.size);
				particleColorData[i] = glm::vec4(p.r, p.g, p.b, p.a);
			}
		}
		

		// Update openGl buffers for particles
		glBindBuffer(GL_ARRAY_BUFFER, particle_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 4, particlePositionData);

		glBindBuffer(GL_ARRAY_BUFFER, particle_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 4, particleColorData);

		// rendering commands here
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		if (ADDITIVE)
		{
			glClear(GL_COLOR_BUFFER_BIT);
		}
		else
		{
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		// set up transformation matrices
		glm::mat4 view;
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		//projection = glm::ortho(0.0f, (float)SCR_WIDTH/10.0f, 0.0f, (float)SCR_HEIGHT/10.0f, 0.1f, 100.0f);
		

		// particles
		glBindVertexArray(particle_VAO);
		particleShader.setMat4("view", view);
		particleShader.setMat4("projection", projection);

		glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
		glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
		glVertexAttribDivisor(2, 1); // color : one per quad                                  -> 1

		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);

		// check and call events and swap the buffers
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	free(particlePositionData);
	free(particleColorData);

	glfwTerminate();
	
	return 0;
}

// This function is called whenever window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

// Process all ketboard input here
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	float cameraSpeed = 2.5f * deltaTime;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.2;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

// Finds a particle in particleContainer which isn't used yet.
int findUnusedParticle()
{
	for (int i = lastUsedParticle; i < maxParticles; i++)
	{
		if (particleContainer[i].life < 0)
		{
			lastUsedParticle = i;
			return i;
		}
	}

	for (int i = 0; i < lastUsedParticle; i++)
	{
		if (particleContainer[i].life < 0)
		{
			lastUsedParticle = i;
			return i;
		}
	}

	return -1; // All particles are taken, return -1
}

void sortParticles() 
{
	std::sort(&particleContainer[0], &particleContainer[maxParticles]);
}