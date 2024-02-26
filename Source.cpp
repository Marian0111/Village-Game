#if defined (__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#define GL_SILENCE_DEPRECATION
#else
#define GLEW_STATIC
#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"

#include <iostream>
#include <chrono>

extern "C" {
	_declspec(dllexport) double NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

int glWindowWidth = 800;
int glWindowHeight = 600;
float yaw = 0.0f;
float pitch = 0.0f;
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
GLuint lightColorLoc;

gps::SkyBox mySkyBox;
gps::Camera myCamera(
	glm::vec3(50.0f, 10.0f, -40.0f),
	glm::vec3(-200.0f, 10.0f, -45.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.5f;

GLfloat angle = 0.0f;
int timeScene = 0;

bool pressedKeys[1024];
float distX = 0.0f;
float angleX = 0.0f;
float angleTeapot = 0.0f;
float distDir = 0.0f;

gps::Model3D scene;
gps::Model3D trees;
gps::Model3D cart;
gps::Model3D cart_wheels;
gps::Model3D teapot;
gps::Model3D screenQuad;
gps::Model3D drop;
gps::Model3D zepelin;

gps::Shader myCustomShader;
gps::Shader screenQuadShader;
gps::Shader shadowMapShader;
gps::Shader skyboxShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

const GLfloat near_plane = -50.0f, far_plane = 30.0f;

bool showDepthMap;

bool wireframeView = false;
bool smoothView = false;
bool solidView = false;
bool itsRaining = false;
bool night = false;
bool presentation = false;


GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	glWindowWidth = width;
	glWindowHeight = height;

	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glViewport(0, 0, retina_width, retina_height);

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
}



void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS) {

		wireframeView = !wireframeView;

		if (wireframeView) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS) {

		smoothView = !smoothView;

		if (smoothView) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_LINE_SMOOTH);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDisable(GL_LINE_SMOOTH);
		}
	}

	if (key == GLFW_KEY_U && action == GLFW_PRESS) {

		solidView = !solidView;

		if (solidView) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		itsRaining = !itsRaining;
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		night = !night;

		if (night) {
			lightColor = glm::vec3(0.3f, 0.3f, 0.5f);
			lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		}
		else {
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		}
	}

	if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		presentation = !presentation;

		if (presentation) {
			myCamera = gps::Camera(
				glm::vec3(50.0f, 10.0f, -40.0f),
				glm::vec3(-200.0f, 10.0f, -45.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			myCamera.resetCameraFrontDirection();
			pitch = 0.0f;
			angle = 0.0f;
			angleX = 0.0f;
			distX = 0.0f;
			yaw = 180.0f;
			timeScene = 0;
		}
		else {
			pressedKeys[GLFW_KEY_S] = false;
			pressedKeys[GLFW_KEY_W] = false;
			pressedKeys[GLFW_KEY_E] = false;
			night = false;
			itsRaining = false;
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
			pitch = 0.0f;
			angle = 0.0f;
			angleX = 0.0f;
			distX = 0.0f;
			yaw = 0.0f;
			presentation = false;
		}
	}

}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
	double yawMouse, pitchMouse;
	glfwGetCursorPos(window, &yawMouse, &pitchMouse);
	pitchMouse = -pitchMouse;
	myCamera.rotate(pitchMouse / 4, yawMouse / 4);
}

void processMovement() {
	if (pressedKeys[GLFW_KEY_UP]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_DOWN]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
	}
	if (pressedKeys[GLFW_KEY_Q] && distX <5) {
		angleX -= 10.0f;
		distX += 0.2f;
	}
	if (pressedKeys[GLFW_KEY_E] && distX > -85) {
		angleX += 10.0f;
		distX -= 0.2f;
	}
	if (pressedKeys[GLFW_KEY_B]) {
		angleTeapot -= 0.1f;
	}
	if (pressedKeys[GLFW_KEY_N]) {
		angleTeapot += 0.1f;
	}
	if (pressedKeys[GLFW_KEY_O]) {
		angle -= 0.01f;
	}
	if (pressedKeys[GLFW_KEY_P]) {
		angle += 0.01f;
	}
	if (pressedKeys[GLFW_KEY_J]) {
		distDir += 1.0f;
	}
	if (pressedKeys[GLFW_KEY_K]) {
		distDir -= 1.0f;
	}
}

void animation() {
	if (presentation) {
		if (timeScene <= 50) {
			timeScene++;
			pressedKeys[GLFW_KEY_E] = true;
		}
		else if (timeScene <= 100) {
			timeScene++;
			pressedKeys[GLFW_KEY_W] = true;
		}
		else if (timeScene <= 150) {
			timeScene++;
			pressedKeys[GLFW_KEY_E] = false;
			pressedKeys[GLFW_KEY_W] = true;
		}
		else if (timeScene <= 200) {
			timeScene++;
			itsRaining = true;
			pressedKeys[GLFW_KEY_W] = false;
		}
		else if (timeScene <= 250) {
			timeScene++;
			itsRaining = false;
			night = true;
			lightColor = glm::vec3(0.3f, 0.3f, 0.5f);
			lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
		}
		else if (timeScene <= 450) {
			timeScene++;
			pitch += 0.05f;
			yaw -= 1.0f;
			myCamera.rotate(pitch, yaw);
			pressedKeys[GLFW_KEY_W] = true;
		}
		else if (timeScene <= 550) {
			timeScene++;
			pitch -= 0.2f;
			yaw -= 1.0f;
			myCamera.rotate(pitch, yaw);
			pressedKeys[GLFW_KEY_W] = false;
			pressedKeys[GLFW_KEY_S] = true;
		}
		else {
			pressedKeys[GLFW_KEY_S] = false;
			night = false;
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
			lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
			glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
			pitch = 0.0f;
			angle = 0.0f;
			angleX = 0.0f;
			distX = 0.0f;
			yaw = 0.0f;
			presentation = false;
			myCamera = gps::Camera(
				myCamera.getCameraPosition(),
				myCamera.getCameraTarget(),
				glm::vec3(0.0f, 1.0f, 0.0f));
			myCamera.resetCameraFrontDirection();
		}
	}
}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	//window scaling for HiDPI displays
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	//for sRBG framebuffer
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	//for antialising
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (__APPLE__)
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initSkyBox() {
	std::vector<const GLchar*> faces; 
	
	faces.push_back("skybox/right.tga"); 
	faces.push_back("skybox/left.tga"); 
	faces.push_back("skybox/top.tga"); 
	faces.push_back("skybox/bottom.tga"); 
	faces.push_back("skybox/back.tga"); 
	faces.push_back("skybox/front.tga");  
	
	mySkyBox.Load(faces);

}

void initObjects() {
	scene.LoadModel("objects/scene/scene.obj");
	trees.LoadModel("objects/scene/trees.obj");
	cart.LoadModel("objects/cart/cart.obj");
	cart_wheels.LoadModel("objects/cart/cart_wheels.obj");
	teapot.LoadModel("objects/teapots/teapot.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
	drop.LoadModel("objects/drop/drop.obj");
	zepelin.LoadModel("objects/zepelin/zepelin.obj");
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag"); 
	skyboxShader.useShaderProgram();
	shadowMapShader.loadShader("shaders/shadowMap.vert", "shaders/shadowMap.frag");
	shadowMapShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightDir = glm::vec3(20.0f, 20.0f, -45.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

}

void initFBO() {
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture); 
	glBindTexture(GL_TEXTURE_2D, depthMapTexture); 
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };  
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO); 
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE); 
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	glm::mat4 lightView = glm::lookAt(lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView;
	return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();

	model = glm::translate(glm::mat4(1.0f), glm::vec3(distX, 0.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	cart.Draw(shader);


	glm::vec3 centerPoint(40.397f, 5.23f, -45.0f);
	model = glm::rotate(model, glm::radians(angleX), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(40.397f + distX, 5.23f, -45.0f));
	model = glm::rotate(model, glm::radians(angleX), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::translate(model, -centerPoint);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	cart_wheels.Draw(shader);

	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	scene.Draw(shader);

	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	trees.Draw(shader);
}

void renderZepelin(gps::Shader shader) {
	shader.useShaderProgram();
	float delta = glfwGetTime() / 10;
	glm::mat4 movement = glm::rotate(glm::mat4(1.0f), delta, glm::vec3(0, 1, 0));
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), glm::vec3(40.0f, 25.0f, 0.0f));
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), 1.57f, glm::vec3(0, 1, 0));

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(shader.shaderProgram, "model");

	model = movement * translate * scale * rotate;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	zepelin.Draw(shader);

}

void renderTeapot(gps::Shader shader) {

	shader.useShaderProgram();

	model = glm::translate(glm::mat4(1.0f), glm::vec3(25.4532f, 5.46923f, -49.0393f));
	model = glm::rotate(model, angleTeapot, glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	teapot.Draw(shader);
}

void renderRain(gps::Shader shader, GLfloat x, GLfloat y, GLfloat z) {
	shader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(shader.shaderProgram, "model");

	model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z)) *
		glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	drop.Draw(shader);
}

void renderScene() {

	shadowMapShader.useShaderProgram();

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);

	glUniformMatrix4fv(glGetUniformLocation(shadowMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(shadowMapShader, false);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	mySkyBox.Draw(skyboxShader, view, projection);


	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
	}
	else {

		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		myCustomShader.useShaderProgram();

		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));


		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(myCustomShader, false);

		if (itsRaining) {
			GLfloat x = rand() % 200 - 100;
			GLfloat z = rand() % 200 - 100;
			GLfloat x1 = rand() % 200 - 100;
			GLfloat z1 = rand() % 200 - 100;
			GLfloat x2 = rand() % 200 - 100;
			GLfloat z2 = rand() % 200 - 100;
			GLfloat x3 = rand() % 200 - 100;
			GLfloat z3 = rand() % 200 - 100;
			GLfloat x4 = rand() % 200 - 100;
			GLfloat z4 = rand() % 200 - 100;
			GLfloat x5 = rand() % 200 - 100;
			GLfloat z5 = rand() % 200 - 100;

			for (GLfloat y = 50.0f; y > -1.0f; y -= 15.0f) {
				renderRain(myCustomShader, x, y, z);
				renderRain(myCustomShader, x1, y, z1);
				renderRain(myCustomShader, x, y, x1);
				renderRain(myCustomShader, z, y, z1);
				renderRain(myCustomShader, x2, y, z2);
				renderRain(myCustomShader, x1, y, z2);
				renderRain(myCustomShader, x, y, x2);
				renderRain(myCustomShader, z, y, z3);
				renderRain(myCustomShader, x3, y, z1);
				renderRain(myCustomShader, x4, y, z2);
				renderRain(myCustomShader, x2, y, x3);
				renderRain(myCustomShader, z3, y, z3);
				renderRain(myCustomShader, x, y, z1);
				renderRain(myCustomShader, x2, y, z3);
				renderRain(myCustomShader, x3, y, x2);
				renderRain(myCustomShader, z3, y, z1);
				renderRain(myCustomShader, x4, y, z5);
				renderRain(myCustomShader, x5, y, z4);
				renderRain(myCustomShader, x3, y, x5);
				renderRain(myCustomShader, z5, y, z4);
				for (int j = 0; j < 100; j++) {
					j++;
					if (!itsRaining) break;
				}
				if (!itsRaining) break;
			}
		}

		mySkyBox.Draw(skyboxShader, view, projection);

		renderTeapot(myCustomShader);
		renderZepelin(myCustomShader);

	}
}

void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	glfwTerminate();
}

int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}


	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms();
	initSkyBox();
	initFBO();

	glCheckError();

	while (!glfwWindowShouldClose(glWindow)) {

		processMovement();
		renderScene();
		animation();
		lightDir = glm::vec3(35.0f + distDir, 6.0f, -40.0f);
		lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view)) * lightDir));

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();

	return 0;
}
