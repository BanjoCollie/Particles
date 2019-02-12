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
glm::vec3 cameraPos = glm::vec3(0.0f, 3.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = 0.0f, pitch = 0.0f;
bool firstMouse = true;
float lastX = 400, lastY = 300;

// time
float deltaTime = 0.0f;	// Time between current frame and last frame
float lastFrame = 0.0f; // Time of last frame

// particles
struct Particle { // Struct for cpu - data is pushed into buffers for gpu to use
	glm::vec3 pos, vel;
	glm::vec4 col, startCol, endCol;
	float size, maxLife;
	float life = -1.0f; // Remaining life of the particle. < 0 = dead/unused.
	float cameraDist = -INFINITY;
	int type; // water or fire

	bool operator<(Particle& that)
	{
		// Sort in reverse order : far particles drawn first.
		return this->cameraDist > that.cameraDist;
	}
};


const int maxParticles = 10000; // This is across all spawners
Particle particleContainer[maxParticles];
int lastUsedParticle = 0;

Particle waterContainer[1000];
int lastUsedWater = 0;

struct ParticleSpawner {
	glm::vec3 pos, dim, startVel;
	glm::vec4 startCol, endCol;
	int particleRate;
	float particleLifetime;
	float size, velRange;

	float wetness = 0.0f;
};

int numSpawners = 1;
const int maxSpawners = 100;
ParticleSpawner spawnerContainer[maxSpawners];

glm::vec3 grav = glm::vec3(0.0f, -9.8f, 0.0f);

bool spaceHeld = false;

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

	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Setup ----------------------------------

	// Spawners
	// one fire on grill
	spawnerContainer[0].pos = glm::vec3(2.0, 0.5f, 2.0f);
	spawnerContainer[0].dim = glm::vec3(0.5f, 0.0f, 0.5f);
	spawnerContainer[0].startVel = glm::vec3(0.25, 1.0f, 0.0f);
	spawnerContainer[0].velRange = 3.0f;
	spawnerContainer[0].particleRate = 50;
	spawnerContainer[0].particleLifetime = 3.0f;
	spawnerContainer[0].size = 1.0f;
	spawnerContainer[0].startCol = glm::vec4(0.682f, 0.306f, 0.0f, 0.8f);
	spawnerContainer[0].endCol = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

	spawnerContainer[1].pos = glm::vec3(2.0, 0.5f, 2.0f);
	spawnerContainer[1].dim = glm::vec3(0.5f, 0.0f, 0.5f);
	spawnerContainer[1].startVel = glm::vec3(0.25, 1.0f, 0.0f);
	spawnerContainer[1].velRange = 1.0f;
	spawnerContainer[1].particleRate = 200;
	spawnerContainer[1].particleLifetime = 0.5f;
	spawnerContainer[1].size = 0.5f;
	spawnerContainer[1].startCol = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	spawnerContainer[1].endCol = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
	/*
	for (int i = 2; i < numSpawners*2; i += 2)
	{
		float rX, rY, rZ;
		rX = ((float)rand() / RAND_MAX) * 14.0f - 7.0f;
		rZ = ((float)rand() / RAND_MAX) * 14.0f - 7.0f;
		spawnerContainer[i].pos = glm::vec3(rX, -0.5f, rZ);
		spawnerContainer[i].dim = glm::vec3(0.5f, 0.0f, 0.5f);
		spawnerContainer[i].startVel = glm::vec3(0.25, 1.0f, 0.0f);
		spawnerContainer[i].velRange = 3.0f;
		spawnerContainer[i].particleRate = 50;
		spawnerContainer[i].particleLifetime = 3.0f;
		spawnerContainer[i].size = 1.0f;
		spawnerContainer[i].startCol = glm::vec4(0.682f, 0.306f, 0.0f, 0.8f);
		spawnerContainer[i].endCol = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

		spawnerContainer[i + 1].pos = glm::vec3(rX, -0.5f, rZ);
		spawnerContainer[i + 1].dim = glm::vec3(0.5f, 0.0f, 0.5f);
		spawnerContainer[i + 1].startVel = glm::vec3(0.25, 1.0f, 0.0f);
		spawnerContainer[i + 1].velRange = 1.0f;
		spawnerContainer[i + 1].particleRate = 200;
		spawnerContainer[i + 1].particleLifetime = 0.5f;
		spawnerContainer[i + 1].size = 0.5f;
		spawnerContainer[i + 1].startCol = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
		spawnerContainer[i + 1].endCol = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
	}
	//*/

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
	unsigned int textures[5];
	glGenTextures(4, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load("smoke1.png", &width, &height, &nrChannels, 0);
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
	//glUniform1i(glGetUniformLocation(particleShader.ID, "texture1"), 0);
	particleShader.setInt("texture1", 0);

	// Floor
	float floor_vertices[] = {
		-15.0f, -1.0f, -15.0f, -3.0f, -3.0f,
		15.0f, -1.0f, -15.0f, 3.0f, -3.0f,
		-15.0f, -1.0f, 15.0f, -3.0f, 3.0f,
		15.0f, -1.0f, 15.0f, 3.0f, 3.0f,
	};
	// 1st create and bind VAO
	unsigned int floor_VAO;
	glGenVertexArrays(1, &floor_VAO);
	glBindVertexArray(floor_VAO);
	// VBO for particle vertex data
	unsigned int floor_VBO;
	glGenBuffers(1, &floor_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, floor_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(floor_vertices), floor_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0); // pos
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float))); // uv
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// shader
	Shader texturedShader("textured.vert", "textured.frag");
	// texture
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	data = stbi_load("wood1.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	stbi_image_free(data);

	// Wall
	float wall_vertices[] = {
		-15.0f, -1.0f, 0.0f, -3.0f, -2.0f,
		15.0f, -1.0f, 0.0f, 3.0f, -2.0f,
		-15.0f, 10.0f, 0.0f, -3.0f, 2.0f,
		15.0f, 10.0f, 0.0f, 3.0f, 2.0f,
	};
	// 1st create and bind VAO
	unsigned int wall_VAO;
	glGenVertexArrays(1, &wall_VAO);
	glBindVertexArray(wall_VAO);
	// VBO for particle vertex data
	unsigned int wall_VBO;
	glGenBuffers(1, &wall_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, wall_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(wall_vertices), wall_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)0); // pos
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(3 * sizeof(float))); // uv
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// texture
	glBindTexture(GL_TEXTURE_2D, textures[2]);
	data = stbi_load("brickwall.jpg", &width, &height, &nrChannels, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	stbi_image_free(data);

	// grill
	const int xSegments = 25;
	const int ySegments = 50;
	float sphereVertices[xSegments * ySegments * 3];

	float PI = 3.14159265f;
	for (unsigned int y = 0; y < ySegments; y++)
	{
		for (unsigned int x = 0; x < xSegments; x++)
		{
			float xSegment = (float)x / (float)xSegments;
			float ySegment;
			if (y == 0)
			{
				ySegment = -0.5f;
			}
			else
			{
				ySegment = (float)y / (float)ySegments + 0.5f;
			}

			int index = (y*xSegments + x) * 3;
			sphereVertices[index] = std::cos(xSegment * PI*2.0f) * std::sin(ySegment * PI); // x pos
			sphereVertices[index + 1] = std::cos(ySegment * PI); // y pos
			sphereVertices[index + 2] = std::sin(xSegment * PI*2.0f) * std::sin(ySegment * PI); // z pos
		}
	}
	int sphereIndices[(xSegments) * (ySegments) * 6]; // Make sure this is sized correctly
	for (unsigned int y = 0; y < (ySegments); y++) // Dont go below last row
	{
		for (unsigned int x = 0; x < (xSegments); x++) // Dont go after last column
		{
			int index = (y*xSegments + x) * 6;
			sphereIndices[index] = ((y)*xSegments) + x;				// 0
			sphereIndices[index + 1] = ((y + 1)*xSegments) + x;		// |
			sphereIndices[index + 2] = ((y + 1)*xSegments) + x + 1;	// 0--0

			sphereIndices[index + 3] = ((y)*xSegments) + x;			// 0--0
			sphereIndices[index + 4] = ((y)*xSegments) + x + 1;		//  \ |
			sphereIndices[index + 5] = ((y + 1)*xSegments) + x + 1;	//    0
		}
	}
	// Buffer stuff for sphere
	unsigned int sphereVAO, sphereVBO, sphereEBO;
	glGenVertexArrays(1, &sphereVAO);
	glBindVertexArray(sphereVAO);
	glGenBuffers(1, &sphereVBO);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphereVertices), sphereVertices, GL_STATIC_DRAW);
	glGenBuffers(1, &sphereEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphereIndices), sphereIndices, GL_STATIC_DRAW);
	// Tell OpenGL how to use vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); //Uses whatever VBO is bound to GL_ARRAY_BUFFER
	glEnableVertexAttribArray(0);
	// shader
	Shader grillShader("baseShader.vert", "baseShader.frag");

	// render loop ----------------------------
	while (!glfwWindowShouldClose(window))
	{
		// Set deltaT
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		// processing
		
		// Spawn new spawners (spread the fire)
		if (numSpawners < maxSpawners)
		{
			if (((float)rand() / RAND_MAX) < 0.5f * deltaTime)
			{
				int i = numSpawners;
				ParticleSpawner &s = spawnerContainer[rand() % numSpawners]; //Spawner to split from

				if (s.wetness < 1.0f)
				{
					float rX, rY, rZ;
					rX = ((float)rand() / RAND_MAX) * 6.0f - 3.0f;
					rZ = ((float)rand() / RAND_MAX) * 6.0f - 3.0f;
					if ((s.pos[0] + rX) > -7.5f && (s.pos[0] + rX) < 7.5f && (s.pos[2] + rZ) > -7.5f && (s.pos[2] + rZ) < 7.5f)
					{
						spawnerContainer[i].pos = glm::vec3(s.pos[0] + rX, -0.5f, s.pos[2] + rZ);
						spawnerContainer[i].dim = glm::vec3(0.5f, 0.0f, 0.5f);
						spawnerContainer[i].startVel = glm::vec3(0.25, 1.0f, 0.0f);
						spawnerContainer[i].velRange = 3.0f;
						spawnerContainer[i].particleRate = 50;
						spawnerContainer[i].particleLifetime = 3.0f;
						spawnerContainer[i].size = 1.0f;
						spawnerContainer[i].startCol = glm::vec4(0.682f, 0.306f, 0.0f, 0.8f);
						spawnerContainer[i].endCol = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
						spawnerContainer[i].wetness = 0.0f;

						spawnerContainer[i + 1].pos = glm::vec3(s.pos[0] + rX, -0.5f, s.pos[2] + rZ);
						spawnerContainer[i + 1].dim = glm::vec3(0.5f, 0.0f, 0.5f);
						spawnerContainer[i + 1].startVel = glm::vec3(0.25, 1.0f, 0.0f);
						spawnerContainer[i + 1].velRange = 1.0f;
						spawnerContainer[i + 1].particleRate = 200;
						spawnerContainer[i + 1].particleLifetime = 0.5f;
						spawnerContainer[i + 1].size = 0.5f;
						spawnerContainer[i + 1].startCol = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
						spawnerContainer[i + 1].endCol = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);
						spawnerContainer[i + 1].wetness = 0.0f;

						numSpawners += 2;
					}
				}
			}
		}

		// Spawn particles
		// Water
		if (spaceHeld) {
			int toSpawn = (int)(deltaTime*1000.0f);
			for (int i = 0; i < toSpawn; i++)
			{
				int index = findUnusedParticle();
				if (index >= 0)
				{
					Particle& p = particleContainer[index];

					p.life = 2.0f;
					p.type = 1;
					float rX = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
					float rY = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
					float rZ = ((float)rand() / RAND_MAX) * 1.0f - 0.5f;
					p.pos = cameraPos + glm::vec3(rX,rY,rZ) + cameraUp;
					p.vel = cameraFront * 10.0f;
					p.maxLife = p.life;

					float rR = ((float)rand() / RAND_MAX) * 0.1f - 0.05f;
					float rG = ((float)rand() / RAND_MAX) * 0.1f - 0.05f;
					float rB = ((float)rand() / RAND_MAX) * 0.1f - 0.05f;
					float rA = ((float)rand() / RAND_MAX) * 0.1f - 0.05f;
					p.col = glm::vec4(0.0, 0.2, 0.9, 0.8f) + glm::vec4(rR,rG,rB,rA);
					p.startCol = p.col;
					p.endCol = glm::vec4(0.7, 0.9, 1.0, 0.1f);
					p.size = 0.5f;
				}
			}
		}
		// Fire
		for (int i = 0; i < numSpawners; i++)
		{
			ParticleSpawner &s = spawnerContainer[i];

			int toSpawn = (int)(deltaTime*s.particleRate*(1.0f - s.wetness));

			float r = ((float)rand() / RAND_MAX);
			if (r < (deltaTime*s.particleRate*(1.0f - s.wetness) - (float)toSpawn))
			{ // use non-integers to determine chance of spawning particle
				toSpawn++;
			}

			for (int i = 0; i < toSpawn; i++)
			{
				int index = findUnusedParticle();
				if (index >= 0)
				{
					Particle& p = particleContainer[index];

					p.life = s.particleLifetime;
					p.type = 0;
					float offX = s.dim[0] * ((float)rand() / RAND_MAX);
					float posX = s.pos[0] - (s.dim[0] / 2.0f) + offX;
					float offY = s.dim[1] * ((float)rand() / RAND_MAX);
					float posY = s.pos[1] - (s.dim[1] / 2.0f) + offY;
					float offZ = s.dim[2] * ((float)rand() / RAND_MAX);
					float posZ = s.pos[2] - (s.dim[2] / 2.0f) + offZ;
					p.pos = glm::vec3(posX, posY, posZ);

					float rTheta = ((float)rand() / RAND_MAX) * 360.0f;
					float rPhi = ((float)rand() / RAND_MAX) * 10.0f;
					float rMag = ((float)rand() / RAND_MAX) * 2.0f;

					glm::mat4 rot = glm::mat4(1.0f);
					rot = glm::rotate(rot, glm::radians(rPhi), glm::vec3(0.0f, 0.0f, 1.0f));
					rot = glm::rotate(rot, glm::radians(rTheta), glm::vec3(0.0f, 1.0f, 0.0f));
					p.vel = glm::vec3(rot * glm::vec4(rMag * s.startVel, 1.0f));

					p.maxLife = s.particleLifetime;

					p.col = s.startCol;
					p.startCol = s.startCol;
					p.endCol = s.endCol;

					//p.size = (s.maxSize - s.minSize) * ((float)rand() / RAND_MAX);
					p.size = s.size;
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
				p.life -= deltaTime;
				if (p.life > 0.0f)
				{ // If the particle didn't die this frame
					if (p.type == 1)
					{
						p.vel += grav * deltaTime;
					}
					p.pos += deltaTime * p.vel;

					float t = (p.life / p.maxLife);
					p.col = t * p.startCol + (1 - t) * p.endCol;

					p.cameraDist = glm::dot(p.pos, cameraFront);
					
					if (p.type == 1) //Only for water
					{
						bool coll = false;
						// Wall collisions
						if (p.pos[1] < -1.0f)
						{
							p.pos[1] = -1.0;
							p.vel[1] = -p.vel[1] * 0.5;
							coll = true;
						}
						else if (p.pos[1] > 5.0f)
						{
							p.pos[1] = 5.0;
							p.vel[1] = -p.vel[1] * 0.5;
							coll = true;
						}
						if (p.pos[0] < -7.5f)
						{
							p.pos[0] = -7.5;
							p.vel[0] = -p.vel[0] * 0.5;
							coll = true;
						}
						else if (p.pos[0] > 7.5f)
						{
							p.pos[0] = 7.5;
							p.vel[0] = -p.vel[0] * 0.5;
							coll = true;
						}
						if (p.pos[2] < -7.5f)
						{
							p.pos[2] = -7.5;
							p.vel[2] = -p.vel[2] * 0.5;
							coll = true;
						}
						else if (p.pos[2] > 7.5f)
						{
							p.pos[2] = 7.5;
							p.vel[2] = -p.vel[2] * 0.5;
							coll = true;
						}
						// Grill collision
						if (p.pos[1] < 0.0f)
						{
							glm::vec3 grillOff = p.pos - glm::vec3(2.0f, 0.0f, 2.0f);
							if (glm::length(grillOff) < 1.0f)
							{
								// Collision with grill detected, decide if we should bounce off top or side
								if (p.pos[1] > -0.05)
								{// top
									p.pos[1] = 0.0;
									p.vel[1] = -p.vel[1] * 0.5f;
									coll = true;
								}
								else
								{// side
									p.pos = glm::vec3(2.0f, 0.0f, 2.0f) + glm::normalize(grillOff);
									p.vel = glm::reflect(p.vel, glm::normalize(grillOff)) * 0.5f;
									coll = true;
								}
							}
						}

						if (coll)
						{
							// If there was a collision randomize velocity a bit
							float rX = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
							float rY = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
							float rZ = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
							p.vel += glm::vec3(rX, rY, rZ);
							p.cameraDist = glm::dot(p.pos, cameraFront);
						}

						// Check if you hit a fire spawner
						for (int j = 0; j < numSpawners; j ++)
						{
							if (glm::length(p.pos - spawnerContainer[j].pos) < 1.0f)
							{
								if (spawnerContainer[j].wetness < 1.0f)
								{
									// Kill this particle, add wetness to spawner
									p.life = -1.0f;
									p.cameraDist = -INFINITY;
									spawnerContainer[j].wetness += 0.001f;
									//spawnerContainer[j + 1].wetness += 0.001f;
								}
							}
						}
					}

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
		sortParticles();

		// put particle info into arrays for gpu
		for (int i = 0; i < numParticles; i++)
		{
			Particle& p = particleContainer[i];
			if (p.life > 0.0f)
			{ // For each currently alive particle
				particlePositionData[i] = glm::vec4(p.pos, p.size);
				particleColorData[i] = p.col;
			}
		}

		// rendering commands here
		glClearColor(0.592f, 0.808f, 0.922f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, textures[2]);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, textures[3]);
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, textures[4]);

		// set up transformation matrices
		glm::mat4 view;
		view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
		glm::mat4 projection = glm::mat4(1.0f);
		projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		
		// room 
		// floor
		glBindVertexArray(floor_VAO);
		texturedShader.use();
		texturedShader.setInt("texture1", 1);
		texturedShader.setMat4("view", view);
		texturedShader.setMat4("projection", projection);
		texturedShader.setMat4("model", glm::mat4(0.5f));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		// ceiling
		texturedShader.setInt("texture1", 2);
		glm::mat4 model = glm::mat4(0.5f);
		model = glm::translate(model, glm::vec3(0.0f, 5.5f, 0.0f));
		texturedShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		// walls
		glBindVertexArray(wall_VAO);
		model = glm::mat4(0.5f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -7.5f));
		texturedShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		model = glm::mat4(0.5f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 7.5f));
		texturedShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		model = glm::mat4(0.5f);
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 7.5f));
		texturedShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		model = glm::mat4(0.5f);
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -7.5f));
		texturedShader.setMat4("model", model);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		// grill
		grillShader.use();
		grillShader.setMat4("view", view);
		grillShader.setMat4("projection", projection);
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, 0.0f, 2.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
		grillShader.setMat4("model", model);
		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, (xSegments) * (ySegments) * 6, GL_UNSIGNED_INT, 0);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// particles
		glBindVertexArray(particle_VAO);

		// Update openGl buffers for particles
		glBindBuffer(GL_ARRAY_BUFFER, particle_position_buffer);
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 4, particlePositionData);

		glBindBuffer(GL_ARRAY_BUFFER, particle_color_buffer);
		glBufferData(GL_ARRAY_BUFFER, maxParticles * 4 * sizeof(GLfloat), NULL, GL_STREAM_DRAW); // Buffer orphaning
		glBufferSubData(GL_ARRAY_BUFFER, 0, numParticles * sizeof(GLfloat) * 4, particleColorData);

		particleShader.use();
		particleShader.setInt("texture1", 0);
		particleShader.setMat4("view", view);
		particleShader.setMat4("projection", projection);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, numParticles);


		// check and call events and swap the buffers
		glfwPollEvents();
		glfwSwapBuffers(window);

	}

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

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		spaceHeld = true;
	else
		spaceHeld = false;
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