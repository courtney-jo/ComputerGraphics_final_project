//Author: Courtney Boortz
//This project is influnced by https://learnopengl.com tutorials

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void animate(float time);

unsigned int groundVBO, groundVAO, lightVBO, lightVAO;

const int SunVertices = 362;
float vertices[SunVertices * 3];
float groundV[12 * 3];

// lighting
glm::vec3 lightPos(0.0f, 0.0f, 0.9999f);
glm::vec3 lightColor(1.0f, 1.0f, 0.8f);
glm::vec3 groundColor(0.2f, 0.7f, 0.2f);
glm::vec3 skyColor(0.6f, 0.8f, 1.0f);
glm::vec3 sunColor(1.0f, 1.0f, 0.8f);
float ambientStrength = 0.5f;

const char *groundVShaderSource =	"#version 330 core\n"
									"layout(location = 0) in vec3 aPos;\n"
									"layout(location = 1) in vec3 aNormal;\n"
									"out vec3 FragPos;\n"
									"out vec3 Normal;\n"
									"uniform mat4 model;\n"
									"void main()\n"
									"{\n"
									"	FragPos = vec3(model * vec4(aPos, 1.0));\n"
									"	Normal = mat3(transpose(inverse(model))) * aNormal;\n"
									"	gl_Position = vec4(FragPos, 1.0);\n"
									"}\n\0";

const char *lightVShaderSource =	"#version 330 core\n"
									"layout(location = 0) in vec3 aPos;\n"
									"uniform mat4 model;\n"
									"void main()\n"
									"{\n"
									"	gl_Position = model * vec4(aPos, 1.0);\n"
									"}\n\0";

const char *groundShaderSource = "#version 330 core\n"
									"out vec4 FragColor;\n"
									"in vec3 Normal;\n"
									"in vec3 FragPos;\n"
									"uniform vec3 lightPos;\n"
									"uniform vec3 lightColor;\n"
									"uniform vec3 groundColor;\n"
									"uniform float ambientStrength;\n"
									"uniform float cutOff;\n"
									"uniform float outerCutOff;\n"
									"void main()\n"
									"{\n"
									//ambient
									"	vec3 ambient = ambientStrength * lightColor;\n"

									//diffuse
									"	vec3 norm = normalize(Normal);\n"
									"	vec3 lightDir = normalize(lightPos - FragPos);\n"
									"	float diff = max(dot(norm, lightDir), 0.0f);\n"
									"	vec3 diffuse = diff * lightColor;\n"

									//specular
									"	float specularStrength = 0.1f;\n"
									"	vec3 viewPos = vec3(0.0f,0.0f,-1.0f);"
									"	vec3 viewDir = normalize(viewPos - FragPos);\n"
									"	vec3 reflectDir = reflect(-lightDir, norm);\n"
									"	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), 32);\n"
									"	vec3 specular = specularStrength * spec * lightColor;\n"

									//soft edges
									"	float theta = dot(lightDir, normalize(-viewDir));\n"
									"	float epsilon = (cutOff - outerCutOff);\n"
									"	float intensity = clamp((theta - outerCutOff) / epsilon, 0.0f, 1.0f);\n"
									"	diffuse *= intensity;\n"
									"	specular *= intensity;\n"

									//attenuation
									"	float distance = length(viewPos - FragPos);\n"
									"	float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * (distance * distance));\n"

									//result
									"	ambient *= attenuation;\n"
									"	diffuse *= attenuation;\n"
									"	specular *= attenuation;\n"
									"	vec3 result = (ambient + diffuse + specular) * groundColor;\n"
									"	FragColor = vec4(result, 1.0f);\n"
									"}\n\0";

const char *lightShaderSource =		"#version 330 core\n"
									"out vec4 FragColor;\n"
									"uniform vec3 sunColor;\n"
									"void main()\n"
									"{\n"
									"   FragColor = vec4(sunColor,1.0f);\n"
									"}\n\0";


int main()
{
    //glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    //glfw window creation
    GLFWwindow* window = glfwCreateWindow(1600, 1600, "Dawn to Dusk", NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

#ifndef __APPLE__
	glewExperimental = true;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
	fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

	// build and compile our shader program
		// ground vertex shader
		int groundVShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(groundVShader, 1, &groundVShaderSource, NULL);
		glCompileShader(groundVShader);

		// ground shader
		int groundShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(groundShader, 1, &groundShaderSource, NULL);
		glCompileShader(groundShader);

		// light vertex shader
		int lightVShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(lightVShader, 1, &lightVShaderSource, NULL);
		glCompileShader(lightVShader);

		// light shader
		int lightShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(lightShader, 1, &lightShaderSource, NULL);
		glCompileShader(lightShader);

		// link shaders
		int groundShaderProgram = glCreateProgram();
		glAttachShader(groundShaderProgram, groundVShader);
		glAttachShader(groundShaderProgram, groundShader);
		glLinkProgram(groundShaderProgram);

		int lightShaderProgram = glCreateProgram();
		glAttachShader(lightShaderProgram, lightVShader);
		glAttachShader(lightShaderProgram, lightShader);
		glLinkProgram(lightShaderProgram);

	glDeleteShader(lightVShader);
	glDeleteShader(groundVShader);
	glDeleteShader(groundShader);
	glDeleteShader(lightShader);

	//draw ground vertex data
		groundV[0] = 1.0f;
		groundV[1] = -1.0f;
		groundV[2] = -1.0f;

		groundV[3] = 1.0f;
		groundV[4] = 0.0f;
		groundV[5] = 1.0f;

		groundV[6] = -1.0f;
		groundV[7] = -1.0f;
		groundV[8] = -1.0f;

		groundV[9] = -1.0f;
		groundV[10] = 0.0f;
		groundV[11] = 1.0f;

		groundV[12] = 1.0f;
		groundV[13] = 0.0f;
		groundV[14] = 1.0f;

		groundV[15] = -1.0f;
		groundV[16] = -1.0f;
		groundV[17] = -1.0f;

	//sky
		groundV[18] = 1.0f;
		groundV[19] = 1.0f;
		groundV[20] = 0.99999f;

		groundV[21] = 1.0f;
		groundV[22] = 0.0f;
		groundV[23] = 0.99999f;

		groundV[24] = -1.0f;
		groundV[25] = 1.0f;
		groundV[26] = 0.99999f;

		groundV[27] = -1.0f;
		groundV[28] = 1.0f;
		groundV[29] = 0.99999f;

		groundV[30] = 1.0f;
		groundV[31] = 0.0f;
		groundV[32] = 0.99999f;

		groundV[33] = -1.0f;
		groundV[34] = 0.0f;
		groundV[35] = 0.99999f;

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glGenBuffers(1, &groundVBO);
	glGenVertexArrays(1, &groundVAO);
		glBindVertexArray(groundVAO);
		//bind to the VBO
		glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(groundV), groundV, GL_STATIC_DRAW);
		//set the vertex attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	glGenBuffers(1, &lightVBO);
	glGenVertexArrays(1, &lightVAO);
		glBindVertexArray(lightVAO);
		//bind to the VBO
		glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
		//set the vertex attributes
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// Save the current time --- it will be used to dynamically change the triangle color
	auto t_start = std::chrono::high_resolution_clock::now();

	//render loop
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		// Set the uniform value depending on the time difference
		auto t_now = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();

		// Enable depth test
		glEnable(GL_DEPTH_TEST);

		// Accept fragment if it closer to the camera than the former one
		glDepthFunc(GL_LESS);

		// Clear the framebuffer (sky color based off of originY)
		glClearColor(skyColor[0], skyColor[1], skyColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 model = glm::mat4(1.0f);

		/*** Draw Sun ***/
		glBindVertexArray(lightVAO);
			glUseProgram(lightShaderProgram);
			glUniform3fv(glGetUniformLocation(lightShaderProgram, "sunColor"), 1, &sunColor[0]);
			glUniformMatrix4fv(glGetUniformLocation(lightShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
			animate(time);
			glDrawArrays(GL_TRIANGLE_FAN, 0, SunVertices);
		glBindVertexArray(0);

		/*** Draw Ground ***/
		glBindVertexArray(groundVAO);
			glUseProgram(groundShaderProgram);
			glUniform3fv(glGetUniformLocation(groundShaderProgram, "groundColor"), 1, &groundColor[0]);
			glUniform3fv(glGetUniformLocation(groundShaderProgram, "lightColor"), 1, &lightColor[0]);
			glUniform3fv(glGetUniformLocation(groundShaderProgram, "lightPos"), 1, &lightPos[0]);
			glUniformMatrix4fv(glGetUniformLocation(groundShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
			glUniform1f(glGetUniformLocation(groundShaderProgram, "ambientStrength"), ambientStrength);
			glUniform1f(glGetUniformLocation(groundShaderProgram, "cutOff"), glm::cos(glm::radians(12.5f)));
			glUniform1f(glGetUniformLocation(groundShaderProgram, "outerCutOff"), glm::cos(glm::radians(17.5f)));
			glDrawArrays(GL_TRIANGLES, 0, 6);

			//glUniform3fv(glGetUniformLocation(groundShaderProgram, "groundColor"), 1, &skyColor[0]);
			//glDrawArrays(GL_TRIANGLES, 6, 12);
		glBindVertexArray(0);

		//swap buffers and poll IO events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	//de-allocate all resources
	glDeleteVertexArrays(1, &groundVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &groundVBO);
	glDeleteBuffers(1, &lightVBO);

	//clearing all previously allocated GLFW resources
	glfwTerminate();
	return 0;
}

//process all input: query GLFW whether relevant keys are pressed/released
void processInput(GLFWwindow *window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

//glfw: whenever the window size changed (by OS or user resize)
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	//make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);
}

void animate(float time) {
	const float doublePI = 2.0f * 3.14159f;
	const float radius = 0.2f;

	//animate sun
	lightPos[0] = -radius * (cos(time/3)*3.8f);
	lightPos[1] = radius * (sin(time/3)*4.0f);

	for (int i = 0; i < SunVertices; ++i) {
		vertices[i * 3] = lightPos[0] + (radius * cos(i* doublePI / 360));
		vertices[(i * 3) + 1] = lightPos[1] + (radius * sin(i* doublePI / 360));
		vertices[(i * 3) + 2] = lightPos[2];
	}

	if (lightPos[1] < 0.5 && lightPos[1] >= radius*-1) {
		if (lightPos[0] < 0) {
			lightColor[0] = (lightPos[1] * .5f) + 1.0f;
			lightColor[1] = (lightPos[1] * .5f) + 0.8f;
			lightColor[2] = (lightPos[1] * .5f) + 0.8f;
		}
		else {
			lightColor[0] = (lightPos[1] * .5f) + 1.0f;
			lightColor[1] = (lightPos[1] * .5f) + 0.8f;
			lightColor[2] = (lightPos[1] * .5f) + 1.0f;
		}
	}

	//time of day
	skyColor[0] = (lightPos[1] * .5f) + 0.3f;
	skyColor[1] = (lightPos[1] * .5f) + 0.425f;
	skyColor[2] = (lightPos[1] * .5f) + 0.65f;

	//sun strength
	sunColor[2] = (lightPos[1] * .5f) + 0.65f;
	ambientStrength = (lightPos[1] * .5f) + 0.6f;

	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
}